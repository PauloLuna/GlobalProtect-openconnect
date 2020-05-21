#include "bestavailablethread.h"
#include <QProcess>

BestAvailableThread::BestAvailableThread(QStringList gateways)
{
    gatewaysList = gateways;
}

void BestAvailableThread::run(){
    QStringList args;
    int bestGateway = 1;
    double bestGatewayTime = 10000.00;
    QString envBin = "/usr/bin/env";
    args << "ping"
     << "-q"
     << "-c3"
     << "-W5"
     << "";
    QProcess* ping = new QProcess();
    for(int i = 1; i < gatewaysList.length(); i++)
    {
        args[4] = gatewaysList[i];
        ping->start(envBin, args);
        ping->waitForFinished();
        if(ping->exitCode() == 0)
        {
            QString out(ping->readAll());
            QString data = out.split("=")[1].split('/')[1]
                    .trimmed();
            double parcial = data.toDouble();
            if(parcial < bestGatewayTime)
            {
                bestGatewayTime = parcial;
                bestGateway = i;
            }
        }
    }
    emit resultReady(gatewaysList.at(bestGateway));
}
