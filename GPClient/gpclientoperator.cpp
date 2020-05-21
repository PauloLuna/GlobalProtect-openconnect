#include "gpclientoperator.h"
#include <QUrlQuery>
#include <QHostInfo>
#include <QtXml/QDomDocument>
#include <QThread>
#include <QProcess>
#include "bestavailablethread.h"
#include <QTemporaryFile>
#include <QDir>

GPClientOperator::GPClientOperator(QObject *parent) : QObject(parent)
{
    // QNetworkAccessManager setup
    networkManager = new QNetworkAccessManager(this);
}

void GPClientOperator::getConfig(QJsonObject loginResult, QString phpsessid)
{
    QString portal = loginResult.value("server").toString();
    QString preloginUrl = "https://" + portal + "/global-protect/getconfig.esp";
    QString preloginCookie = loginResult.value("prelogin-cookie").toString();
    QUrl url(preloginUrl);
    QUrlQuery query;

    user = loginResult.value("saml-username").toString();

    query.addQueryItem("jnlpReady", "jnlpReady");
    query.addQueryItem("ok", "Login");
    query.addQueryItem("direct", "yes");
    query.addQueryItem("clientVer", "4100");
    query.addQueryItem("prot", "https:");
    query.addQueryItem("ipv6-support", "no");
    query.addQueryItem("clientos", "Linux");
    query.addQueryItem("os-version", "linux-64");
    query.addQueryItem("server", portal);
    query.addQueryItem("computer", QHostInfo::localHostName());
    query.addQueryItem("user", user);
    query.addQueryItem("prelogin-cookie", preloginCookie);


    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "PAN GlobalProtect");
    request.setRawHeader("Cookie", phpsessid.toUtf8());
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Host", portal.toStdString().c_str());
    reply = networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &GPClientOperator::getconfigResultFinished);
}

void GPClientOperator::getconfigResultFinished()
{
    QNetworkReply::NetworkError err = reply->error();
    if (err) {
        qWarning() << "Get config request error: " << err;
        emit connectFailed();
        return;
    }

    QByteArray xmlBytes = reply->readAll();
    QDomDocument doc;
    doc.setContent(xmlBytes);

    // Get Gateways names
    QDomElement docElem = doc.documentElement();
    QDomNodeList usercookie = docElem.elementsByTagName("portal-userauthcookie");
    QString usercookiestr = usercookie.item(0).toElement().text();

    QDomNodeList gateways = docElem.elementsByTagName("gateways")
            .item(0).toElement().elementsByTagName("list")
            .item(0).toElement().childNodes();
    QStringList gatewaynames;
    gatewaynames.append("Best Available");
    for(int i = 0; i < gateways.length(); i++){
        gatewaynames.append(gateways.item(i).toElement().attribute("name"));
    }

    // Get Certificates
    QDomNodeList root_cas = docElem.elementsByTagName("root-ca")
            .item(0).toElement().childNodes();
    QStringList cas;
    for(int i = 0; i < root_cas.length(); i++){
        QDomElement elem = root_cas.at(i).toElement();
        cas.append(elem.elementsByTagName("cert")
                .item(0).toElement().text());
    }

    emit getConfigSuccess(user, usercookiestr, gatewaynames, cas);
}

void GPClientOperator::getBestAvaialble(const QStringList gatewaysList)
{
    BestAvailableThread *bathread = new BestAvailableThread(gatewaysList);
    connect(bathread, &BestAvailableThread::resultReady, this, &GPClientOperator::getBestAvaialbleFinished);
    bathread->start();
}

void GPClientOperator::installCertificates(const QStringList cas)
{
    QTemporaryFile script;
    QString scriptName;
    QStringList args;
    QString envBin = "/usr/bin/env";
    QString ca_dir;
    QString scripttext;
    QString scriptend;

    QDir ubuntuDir("/usr/local/share/ca-certificates/");
    QDir fedoraDir("/etc/pki/ca-trust/source/anchors/");
    if(ubuntuDir.exists()){
        ca_dir = ubuntuDir.path()+"/gpclient";
        scriptend = "\nupdate-ca-certificates\n";
    } else if(fedoraDir.exists()){
        ca_dir = fedoraDir.path()+"/gpclient";
        scriptend = "\nupdate-ca-trust extract\n";
    } else {
        notify("Unsupported system.");
        return;
    }

    for(int i = 0; i<cas.length(); i++){
        QTemporaryFile certificate;
        if(certificate.open()){
            certificate.write(cas.at(i).trimmed().toUtf8());
            certificate.write("\n");
            scripttext.append("\nmv \""+certificate.fileName()
                              +"\" \""+ca_dir+certificate.fileName().split("/").last()+".crt\"");
            certificate.setAutoRemove(false);
            certificate.close();
        }
    }
    qInfo(scripttext.toUtf8());
    scripttext.append(scriptend);
    if(script.open()){
        script.write(scripttext.toUtf8());
        script.setAutoRemove(false);
        script.close();
    }


    args << "pkexec"
     << "bash"
     << script.fileName();
    QProcess* install = new QProcess();
    if(install->execute(envBin, args) == 0){
        notify("Successfully installed.");
    } else {
        notify("Install error.");
    }
}

void GPClientOperator::uninstallCertificates()
{
    QTemporaryFile certificates, script;
    QString scriptName;
    QStringList args;
    QString envBin = "/usr/bin/env";
    QString scripttext;

    QDir ubuntuDir("/usr/local/share/ca-certificates/");
    QDir fedoraDir("/etc/pki/ca-trust/source/anchors/");
    if(ubuntuDir.exists()){
        scripttext = "rm -rf " + ubuntuDir.path()+"/gpclient*\nupdate-ca-certificates -f\n";
    } else if(fedoraDir.exists()){
        scripttext = "rm -rf " + fedoraDir.path()+"/gpclient*\nupdate-ca-trust extract\n";
    } else {
        notify("Unsupported system.");
        return;
    }

    if(script.open()){
        script.write(scripttext.toUtf8());
        script.close();
    }


    args << "pkexec"
     << "bash"
     << script.fileName();
    QProcess* uninstall = new QProcess();
    if(uninstall->execute(envBin, args) == 0){
        notify("Successfully uninstalled.");
    } else {
        notify("Uninstall error.");
    }

}

void GPClientOperator::notify(QString text)
{
    QString envBin = "/usr/bin/env";
    QStringList args;
    args << "notify-send" << text;
    QProcess* notify = new QProcess();
    notify->execute(envBin, args);
}
