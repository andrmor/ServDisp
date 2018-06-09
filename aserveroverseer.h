#ifndef ASERVEROVERSEER_H
#define ASERVEROVERSEER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QStringList>
#include <QJsonObject>

class AServerRecord;
class AWebSocketServer;

class AServerOverseer : public QObject
{
    Q_OBJECT

public:
    AServerOverseer(quint16 Port);

public slots:
    void onMessageReceived(const QString message);

private:
    AServerRecord* startProcess(int port, int numCPUs);
    int findFreePort();

private slots:
    void oneProcessFinished(AServerRecord* record);

signals:
    void replyWith(const QString message);

private:
    AWebSocketServer* server;
    int CPUpool = 4;
    QVector<int> AllocatedPorts;

    QVector<AServerRecord*> RunningServers;
    int MaxRunningTime;

private:
    const QJsonObject objectFromString(const QString &in);
};

class AServerRecord : public QObject
{
    Q_OBJECT

public:
    AServerRecord(int Port, int numCPUs, int TimeOfStart) :
        Port(Port), numCPUs(numCPUs), TimeOfStart(TimeOfStart) {}

    int Port;
    int numCPUs;

    int TimeOfStart;

public slots:
    void processTerminated();

signals:
    void finished(AServerRecord*);
};


#endif // ASERVEROVERSEER_H
