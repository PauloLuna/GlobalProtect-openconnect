#ifndef BESTAVAILABLETHREAD_H
#define BESTAVAILABLETHREAD_H
#include <QThread>

class BestAvailableThread : public QThread
{
    Q_OBJECT
    void run() override;
public:
    BestAvailableThread(QStringList gateways);
private:
    QStringList gatewaysList;
signals:
    void resultReady(const QString result);
};

#endif // BESTAVAILABLETHREAD_H
