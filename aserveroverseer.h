#ifndef ASERVEROVERSEER_H
#define ASERVEROVERSEER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QJsonObject>

class AServerRecord;
class AWebSocketServer;
class QProcess;

class AServerOverseer : public QObject
{
    Q_OBJECT

public:
    AServerOverseer(quint16 Port, QSet<quint16> ServerPorts, int MaxCPUs);

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
    QSet<quint16> AllocatedPorts;

    QVector<AServerRecord*> RunningServers;
    int MaxRunningTime;

private:
    const QJsonObject objectFromString(const QString &in);
    const QString     generateTicket();
    void              processCommandNew(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandAbort(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandReport(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandHelp(const QJsonObject &jsIn, QJsonObject &jsOut);
};

class AServerRecord : public QObject
{
    Q_OBJECT

public:
    AServerRecord(int Port, int NumCPUs, const QString Ticket, int TimeOfStart, QProcess* pProcess) :
        Port(Port), NumCPUs(NumCPUs), Ticket(Ticket), TimeOfStart(TimeOfStart), Process(pProcess) {}

    int Port;
    int NumCPUs;
    QString Ticket;
    int TimeOfStart;

    QProcess* Process;

public slots:
    void processTerminated();

signals:
    void finished(AServerRecord*);
};


#endif // ASERVEROVERSEER_H
