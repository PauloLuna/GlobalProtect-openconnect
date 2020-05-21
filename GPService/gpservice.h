#ifndef GLOBALPROTECTSERVICE_H
#define GLOBALPROTECTSERVICE_H

#include <QObject>
#include <QProcess>

static const QString binaryPaths[] {
    "/usr/local/bin/openconnect",
    "/usr/local/sbin/openconnect",
    "/usr/bin/openconnect",
    "/usr/sbin/openconnect",
    "/opt/bin/openconnect",
    "/opt/sbin/openconnect"
};

static const QString binNmcli = "/usr/bin/nmcli";

class GPService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.yuezk.qt.GPService")
public:
    explicit GPService(QObject *parent = nullptr);
    ~GPService();

    enum VpnStatus {
        VpnNotConnected,
        VpnConnecting,
        VpnConnected,
        VpnDisconnecting,
    };

signals:
    void connected();
    void disconnected();
    void logAvailable(QString log);

public slots:
    void connect(QString server, QString username, QString passwd);
    void connect_gw(QString server, QString username, QString passwd, QString gateway);
    void disconnect();
    int status();
    void quit();

private slots:
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError error);
    void onProcessStdout();
    void onProcessStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *openconnect;
    bool aboutToQuit = false;
    int vpnStatus = GPService::VpnNotConnected;

    void log(QString msg);
    void leaveNetworkManager();
    static QString findBinary();
};

#endif // GLOBALPROTECTSERVICE_H
