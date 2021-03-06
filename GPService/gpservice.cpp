#include "gpservice.h"
#include "gpservice_adaptor.h"

#include <QFileInfo>
#include <QtDBus>
#include <QDateTime>
#include <QVariant>

GPService::GPService(QObject *parent)
    : QObject(parent)
    , openconnect(new QProcess)
{
    // Register the DBus service
    new GPServiceAdaptor(this);
    QDBusConnection dbus = QDBusConnection::systemBus();
    dbus.registerObject("/", this);
    dbus.registerService("com.yuezk.qt.GPService");

    // Setup the openconnect process
    QObject::connect(openconnect, &QProcess::started, this, &GPService::onProcessStarted);
    QObject::connect(openconnect, &QProcess::errorOccurred, this, &GPService::onProcessError);
    QObject::connect(openconnect, &QProcess::readyReadStandardOutput, this, &GPService::onProcessStdout);
    QObject::connect(openconnect, &QProcess::readyReadStandardError, this, &GPService::onProcessStderr);
    QObject::connect(openconnect, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &GPService::onProcessFinished);
}

GPService::~GPService()
{
    delete openconnect;
}

QString GPService::findBinary()
{
    for (int i = 0; i < binaryPaths->length(); i++) {
        if (QFileInfo::exists(binaryPaths[i])) {
            return binaryPaths[i];
        }
    }
    return nullptr;
}

void GPService::quit()
{
    if (openconnect->state() == QProcess::NotRunning) {
        exit(0);
    } else {
        aboutToQuit = true;
        openconnect->terminate();
    }
}

void GPService::connect(QString server, QString username, QString passwd)
{
    if (vpnStatus != GPService::VpnNotConnected) {
        log("VPN status is: " + QVariant::fromValue(vpnStatus).toString());
        return;
    }

    QString bin = findBinary();
    if (bin == nullptr) {
        log("Could not found openconnect binary, make sure openconnect is installed, exiting.");
        return;
    }

    QStringList args;
    args << QCoreApplication::arguments().mid(1)
     << "--interface=gptun"
     << "--protocol=gp"
     << "-u" << username
     << "--passwd-on-stdin"
     << "--timestamp"
     << server;

    openconnect->start(bin, args);
    openconnect->write(passwd.toUtf8());
    openconnect->closeWriteChannel();
}

void GPService::connect_gw(QString server, QString username, QString passwd, QString gateway)
{
    if (vpnStatus != GPService::VpnNotConnected) {
        log("VPN status is: " + QVariant::fromValue(vpnStatus).toString());
        return;
    }

    QString bin = findBinary();
    if (bin == nullptr) {
        log("Could not found openconnect binary, make sure openconnect is installed, exiting.");
        return;
    }

    QStringList args;
    args << QCoreApplication::arguments().mid(1)            
     << "--interface=gptun"
     << "--protocol=gp"
     << "-u" << username
     << "--passwd-on-stdin"
     << "--timestamp"
     << server;

    openconnect->start(bin, args);
    openconnect->write(passwd.toUtf8()+'\n'+gateway.toUtf8()+'\n'+passwd.toUtf8());
    openconnect->closeWriteChannel();
}

void GPService::disconnect()
{
    if (openconnect->state() != QProcess::NotRunning) {
        vpnStatus = GPService::VpnDisconnecting;
        openconnect->terminate();
    }
}

int GPService::status()
{
    return vpnStatus;
}

void GPService::onProcessStarted()
{
    log("Openconnect started successfully, PID=" + QString::number(openconnect->processId()));
    vpnStatus = GPService::VpnConnecting;
}

void GPService::onProcessError(QProcess::ProcessError error)
{
    log("Error occurred: " + QVariant::fromValue(error).toString());
    vpnStatus = GPService::VpnNotConnected;
    emit disconnected();
}

void GPService::onProcessStdout()
{
    QString output = openconnect->readAllStandardOutput();

    log(output);
    if (output.indexOf("Connected as") >= 0) {
        vpnStatus = GPService::VpnConnected;
        emit connected();        
        leaveNetworkManager();
    }
}

void GPService::onProcessStderr()
{
    log(openconnect->readAllStandardError());
}

void GPService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    log("Openconnect process exited with code " + QString::number(exitCode) + " and exit status " + QVariant::fromValue(exitStatus).toString());
    vpnStatus = GPService::VpnNotConnected;
    emit disconnected();

    if (aboutToQuit) {
        exit(0);
    };
}

void GPService::log(QString msg)
{
    qInfo() << msg;
    emit logAvailable(msg);
}

void GPService::leaveNetworkManager()
{
    QStringList args;
    args << "device"
     << "set"
     << "gptun"
     << "managed"
     << "no";
    struct timespec ts = { 5, 0};
    nanosleep(&ts, NULL);
    QProcess* nmcli = new QProcess();
    log("Remove gptun from network manager out: " + QString::number(nmcli->execute(binNmcli, args)));
}
