#ifndef GPCLIENTOPERATOR_H
#define GPCLIENTOPERATOR_H
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QJsonObject>


class GPClientOperator : public QObject
{
    Q_OBJECT
public:
    explicit GPClientOperator(QObject *parent = nullptr);    
    void getConfig(QJsonObject loginResult, QString phpsessid);
    void getBestAvaialble(const QStringList gatewaysList);
    void installCertificates(const QStringList cas);
    void uninstallCertificates();

signals:
    void getConfigSuccess(QString user, QString usercookie, QStringList gateways, QStringList cas);
    void connectFailed();
    void getBestAvaialbleFinished(const QString gateway);
private slots:
    void getconfigResultFinished();
private:
    QString user;
    QNetworkAccessManager *networkManager;
    QNetworkReply *reply;
    void notify(QString text);

};

#endif // GPCLIENTOPERATOR_H
