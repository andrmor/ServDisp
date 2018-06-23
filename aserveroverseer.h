#ifndef ASERVEROVERSEER_H
#define ASERVEROVERSEER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QJsonObject>
#include <QHostAddress>

class AServerRecord;
class AWebSocketServer;
class QProcess;

class AServerOverseer : public QObject
{
    Q_OBJECT

public:
    ~AServerOverseer();

    void  SetPort(quint16 port) {Port = port;}
    void  SetServerPorts(QSet<quint16> serverPorts) {AllocatedPorts = serverPorts;}
    void  SetMaxThreads(int maxThreads) {MaxThreads = maxThreads;}
    void  SetServerApplication(const QString& command) {ServerApp = command;}
    void  SetArguments(const QStringList& arguments) {Arguments = arguments;}
    void  SetIP(const QString& ip);

    bool  ConfigureFromFile(const QString& fileName); //overrides the settings!

    bool  StartListen();

    const QString GetListOfPorts();

public slots:
    void onMessageReceived(const QString message);

private:
    AServerRecord* startProcess(int port, int maxThreads);
    int findFreePort();

private slots:
    void oneProcessFinished(AServerRecord* record);

signals:
    void replyWith(const QString message);

private:
    AWebSocketServer* server = 0;
    QHostAddress IP = QHostAddress::LocalHost;
    quint16 Port = 0;
    int MaxThreads = 1;
    QSet<quint16> AllocatedPorts;

    QVector<AServerRecord*> RunningServers;
    int MaxRunningTime;

    QString ServerApp;
    QStringList Arguments;

private:
    const QJsonObject objectFromString(const QString &in);
    const QString     generateTicket();
    void              processCommandNew(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandAbort(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandReport(const QJsonObject &jsIn, QJsonObject &jsOut);
    void              processCommandHelp(const QJsonObject &jsIn, QJsonObject &jsOut);

    bool              isReady();
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
