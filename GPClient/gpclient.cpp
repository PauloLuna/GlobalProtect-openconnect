#include "gpclient.h"
#include "ui_gpclient.h"
#include "samlloginwindow.h"

#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QStyle>
#include <QMessageBox>
#include <QHostInfo>
#include <QtXml/QDomDocument>


GPClient::GPClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GPClient)
{
    ui->setupUi(this);
    
    setFixedSize(width(), height());
    moveCenter();

    // Restore portal from the previous settings
    settings = new QSettings("com.yuezk.qt", "GPClient");
    QObject::connect(this, &GPClient::connectFailed, [this]() {
        updateConnectionStatus("not_connected");
    });

    // QNetworkAccessManager setup
    networkManager = new QNetworkAccessManager(this);
    clientoperator = new GPClientOperator(this);
    systemTrayIcon = new QSystemTrayIcon(this);
    systemTrayIcon->setVisible(true);
    systemTrayIcon->setContextMenu(ui->menuOptions);
    connect(systemTrayIcon, &QSystemTrayIcon::activated, this, &GPClient::onTrayIconActivated);
    // DBus service setup
    vpn = new com::yuezk::qt::GPService("com.yuezk.qt.GPService", "/", QDBusConnection::systemBus(), this);
    QObject::connect(vpn, &com::yuezk::qt::GPService::connected, this, &GPClient::onVPNConnected);
    QObject::connect(vpn, &com::yuezk::qt::GPService::disconnected, this, &GPClient::onVPNDisconnected);
    QObject::connect(vpn, &com::yuezk::qt::GPService::logAvailable, this, &GPClient::onVPNLogAvailable);

    initVpnStatus();
}

GPClient::~GPClient()
{
    delete ui;
    delete networkManager;
    delete reply;
    delete vpn;
    delete settings;
}

void GPClient::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason){
    if(reason == QSystemTrayIcon::Trigger){
        if(isVisible()){
            hide();
        } else {
            show();
            activateWindow();
        }
    }
}

void GPClient::on_connectButton_clicked()
{
    QString btnText = ui->connectButton->text();
    QString usertoken = settings->value("userauthcookie", "").toString();
    QString user = settings->value("user", "").toString();

    if (btnText.endsWith("Login")) {
        QString portal = ui->portalInput->text();
        settings->setValue("portal", portal);
        ui->statusLabel->setText("Authenticating...");
        updateConnectionStatus("pending");
        doAuth(portal);
    } else if (btnText.endsWith("Cancel")) {
        ui->statusLabel->setText("Canceling...");
        updateConnectionStatus("pending");

        if (reply->isRunning()) {
            reply->abort();
        }
        vpn->disconnect();
    } else if (btnText.endsWith("Connect")) {
        QString portal = settings->value("portal", "").toString();
        QString gateway = ui->gatewaysComboBox->currentText();
        QStringList gatewaynames = settings->value("gatewaynames", QStringList()).toStringList();

        ui->statusLabel->setText("Connecting...");
        updateConnectionStatus("pending");

        settings->setValue("lastGateway", gateway);
        //Get best available gateway
        if(gateway == "Best Available")
        {
            connect(clientoperator, &GPClientOperator::getBestAvaialbleFinished, this, &GPClient::connectToGateway);
            clientoperator->getBestAvaialble(gatewaynames);
        } else {
            connectToGateway(gateway);
        }

    } else {
        ui->statusLabel->setText("Disconnecting...");
        updateConnectionStatus("pending");
        vpn->disconnect();
    }
}

void GPClient::preloginResultFinished()
{
    QNetworkReply::NetworkError err = reply->error();
    if (err) {
        qWarning() << "Prelogin request error: " << err;
        emit connectFailed();
        return;
    }

    QByteArray xmlBytes = reply->readAll();
    const QString tagMethod = "saml-auth-method";
    const QString tagRequest = "saml-request";
    QString samlMethod;
    QString samlRequest;

    // Save phpsessid
    if(reply->hasRawHeader("Set-Cookie")){
        phpsessid = reply->rawHeader("Set-Cookie");
        phpsessid.truncate(phpsessid.indexOf(';'));
    }

    QXmlStreamReader xml(xmlBytes);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == xml.StartElement) {
            if (xml.name() == tagMethod) {
                samlMethod = xml.readElementText();
            } else if (xml.name() == tagRequest) {
                samlRequest = QByteArray::fromBase64(QByteArray::fromStdString(xml.readElementText().toStdString()));
            }
        }
    }

    if (samlMethod == nullptr || samlRequest == nullptr) {
        qWarning("This does not appear to be a SAML prelogin response (<saml-auth-method> or <saml-request> tags missing)");
        emit connectFailed();
        return;
    }

    if (samlMethod == "POST") {
        samlLogin(reply->url().toString(), samlRequest);
    } else if (samlMethod == "REDIRECT") {
        samlLogin(samlRequest);
    }
}

void GPClient::onLoginSuccess(QJsonObject loginResult)
{
    clientoperator->getConfig(loginResult, phpsessid);
    connect(clientoperator, &GPClientOperator::getConfigSuccess, this, &GPClient::getConfigSuccess);
    connect(clientoperator, &GPClientOperator::connectFailed, this, &GPClient::connectFailed);
}

void GPClient::getConfigSuccess(QString user, QString usercookie, QStringList gateways, QStringList cas)
{
    settings->setValue("userauthcookie", usercookie);
    settings->setValue("user", user);
    settings->setValue("gatewaynames", gateways);
    settings->setValue("cas", cas);
    updateConnectionStatus("not_connected");
}

void GPClient::updateConnectionStatus(QString status)
{    
    const QStringList gatewaynames = settings->value("gatewaynames", QStringList()).toStringList();
    const QString lastGateway = settings->value("lastGateway", "").toString();
    int index = gatewaynames.indexOf(lastGateway);
    if(index==-1) index=0;
    // Update combobox data
    ui->gatewaysComboBox->clear();
    ui->gatewaysComboBox->addItems(settings->value("gatewaynames", QStringList()).toStringList());
    ui->gatewaysComboBox->setCurrentIndex(index);
    // Update Portal Text
    ui->portalInput->setText(settings->value("portal", "").toString());

    if (status == "not_connected") {
        ui->connectButton->setDisabled(false);
        if(settings->value("userauthcookie").toString().length() == 0
                || settings->value("gatewaynames", QStringList()).toStringList().length() ==0){
            ui->statusLabel->setText("Not logged in");
            ui->statusImage->setStyleSheet("image: url(:/images/not_connected.png); padding: 15;");
            ui->connectButton->setText("Login");
            ui->gatewaysComboBox->setDisabled(true);
            ui->portalInput->setDisabled(false);

            //update tray icon
            ui->actionConnect->setText("Login");
            systemTrayIcon->setIcon(QIcon(":/images/not_connected.png"));
        } else {
            ui->statusLabel->setText("Not Connected");
            ui->statusImage->setStyleSheet("image: url(:/images/not_connected.png); padding: 15;");
            ui->connectButton->setText("Connect");
            ui->gatewaysComboBox->setDisabled(false);
            ui->portalInput->setDisabled(true);
            //update tray icon
            ui->actionConnect->setText("Connect");
            systemTrayIcon->setIcon(QIcon(":/images/not_connected.png"));
        }
    } else if (status == "pending") {
        ui->statusImage->setStyleSheet("image: url(:/images/pending.png); padding: 15;");
        ui->connectButton->setText("Cancel");
        ui->portalInput->setDisabled(true);
        ui->gatewaysComboBox->setDisabled(true);
        ui->connectButton->setDisabled(false);
        //update tray icon
        ui->actionConnect->setText("Cancel");
        systemTrayIcon->setIcon(QIcon(":/images/not_connected.png"));
    } else if (status == "connected") {
        ui->statusLabel->setText("Connected");
        ui->statusImage->setStyleSheet("image: url(:/images/connected.png); padding: 15;");
        ui->connectButton->setText("Disconnect");
        ui->portalInput->setDisabled(true);
        ui->gatewaysComboBox->setDisabled(true);
        ui->connectButton->setDisabled(false);
        //update tray icon
        ui->actionConnect->setText("Disconnect");
        systemTrayIcon->setIcon(QIcon(":/images/connected.png"));
    }
}

void GPClient::onVPNConnected()
{
    updateConnectionStatus("connected");
}

void GPClient::onVPNDisconnected()
{
    updateConnectionStatus("not_connected");
}

void GPClient::onVPNLogAvailable(QString log)
{
    qInfo() << log;
    if (log.indexOf("Unexpected 512") >= 0) {
        on_actionLogout_triggered();
    }
}

void GPClient::initVpnStatus() {
    int status = vpn->status();
    if (status == 0) {
        updateConnectionStatus("not_connected");
    } else if (status == 1) {
        ui->statusLabel->setText("Connecting...");
        updateConnectionStatus("pending");
    } else if (status == 2) {
        updateConnectionStatus("connected");
    } else if (status == 3) {
        ui->statusLabel->setText("Disconnecting...");
        updateConnectionStatus("pending");
    }
}

void GPClient::moveCenter()
{
    QDesktopWidget *desktop = QApplication::desktop();

    int screenWidth, width;
    int screenHeight, height;
    int x, y;
    QSize windowSize;

    screenWidth = desktop->width();
    screenHeight = desktop->height();

    windowSize = size();
    width = windowSize.width();
    height = windowSize.height();

    x = (screenWidth - width) / 2;
    y = (screenHeight - height) / 2;
    y -= 50;
    move(x, y);
}

void GPClient::doAuth(const QString portal)
{
    const QString preloginUrl = "https://" + portal + "/global-protect/prelogin.esp";
    QUrl url(preloginUrl);
    QUrlQuery query;

    query.addQueryItem("tmp", "tmp");
    query.addQueryItem("clientVer", "4100");
    query.addQueryItem("clientos", "Linux");

    url.setQuery(query.query());

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "PAN GlobalProtect");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Content-Length", "0");
    request.setRawHeader("Host", portal.toStdString().c_str());

    reply = networkManager->post(request, (QByteArray) nullptr);
    connect(reply, &QNetworkReply::finished, this, &GPClient::preloginResultFinished);
}

void GPClient::samlLogin(const QString loginUrl, const QString html)
{
    SAMLLoginWindow *loginWindow = new SAMLLoginWindow(this);

    QObject::connect(loginWindow, &SAMLLoginWindow::success, this, &GPClient::onLoginSuccess);
    QObject::connect(loginWindow, &SAMLLoginWindow::rejected, this, &GPClient::connectFailed);

    loginWindow->login(loginUrl, html);
    loginWindow->exec();
    delete loginWindow;
}

void GPClient::on_actionLogout_triggered()
{
    settings->setValue("userauthcookie", "");
    vpn->disconnect();
    updateConnectionStatus("not_connected");
}

void GPClient::on_actionClear_data_triggered()
{
    settings->setValue("portal", "");
    settings->setValue("gatewaynames", QStringList());
    settings->setValue("lastGateway", "");
    vpn->disconnect();
    updateConnectionStatus("not_connected");
}

void GPClient::connectToGateway(const QString gateway)
{
    QString portal = settings->value("portal", "").toString();
    QStringList gatewaynames = settings->value("gatewaynames", QStringList()).toStringList();
    QString usertoken = settings->value("userauthcookie", "").toString();
    QString user = settings->value("user", "").toString();
    ui->statusBar->showMessage("Gateway: " + gateway);

    QString host = QString("https://%1/%2:%3").arg(portal, "portal", "portal-userauthcookie");
    qInfo("Connection data %s %s %s %s", host.toStdString().c_str(), user.toStdString().c_str(), usertoken.toStdString().c_str(), gateway.toStdString().c_str());
    vpn->connect_gw(host, user, usertoken, gateway);
}

void GPClient::on_actionInstall_Root_CA_s_triggered()
{
    clientoperator->installCertificates(settings->value("cas", "").toStringList());
}

void GPClient::on_actionUninstall_Root_CA_s_triggered()
{
    clientoperator->uninstallCertificates();
}

void GPClient::on_actionConnect_triggered()
{
    on_connectButton_clicked();
}
