#ifndef GPCLIENT_H
#define GPCLIENT_H

#include "gpservice_interface.h"
#include "gpclientoperator.h"
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class GPClient; }
QT_END_NAMESPACE

class GPClient : public QMainWindow
{
    Q_OBJECT

public:
    GPClient(QWidget *parent = nullptr);
    ~GPClient();

signals:
    void connectFailed();

private slots:
    void on_connectButton_clicked();
    void preloginResultFinished();
    void getConfigSuccess(QString user, QString usercookie, QStringList gateways, QStringList cas);

    void onLoginSuccess(QJsonObject loginResult);

    void onVPNConnected();
    void onVPNDisconnected();
    void onVPNLogAvailable(QString log);
    void connectToGateway(const QString gateway);

    void on_actionLogout_triggered();

    void on_actionClear_data_triggered();

    void on_actionInstall_Root_CA_s_triggered();

    void on_actionUninstall_Root_CA_s_triggered();

    void on_actionConnect_triggered();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    Ui::GPClient *ui;
    QNetworkAccessManager *networkManager;
    QNetworkReply *reply;
    com::yuezk::qt::GPService *vpn;
    QSettings *settings;
    GPClientOperator *clientoperator;
    QString phpsessid;
    QString preloginCookie;
    QSystemTrayIcon * systemTrayIcon;

    void initVpnStatus();
    void moveCenter();
    void updateConnectionStatus(QString status);
    void doAuth(const QString portal);
    void samlLogin(const QString loginUrl, const QString html = "");
};
#endif // GPCLIENT_H
