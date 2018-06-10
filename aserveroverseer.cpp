#include "aserveroverseer.h"
#include "awebsocketserver.h"

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>

AServerOverseer::AServerOverseer(quint16 Port)
{
    AllocatedPorts = {1111,2222};

    server = new AWebSocketServer(this);

    QObject::connect(server, &AWebSocketServer::textMessageReceived, this, &AServerOverseer::onMessageReceived);

    server->StartListen(Port);
}

void AServerOverseer::onMessageReceived(const QString message)
{
    if ( !server->CanReply() ) return; // cannot reply so do not start a new ANTS2 server

    QJsonObject jsIn = objectFromString(message);
    qDebug() << jsIn;

    QJsonObject jsOut;
    if (jsIn.contains("command"))
    {
        QString com = jsIn["command"].toString();
        if (com == "new") processCommandNew(jsIn, jsOut);
        else if (com == "abort") processCommandAbort(jsIn, jsOut);
        else if (com == "report") processCommandReport(jsIn, jsOut);
        else if (com == "help") processCommandHelp(jsIn, jsOut);
        else
        {
            jsOut["result"] = false;
            jsOut["error"] = "bad format of request. use {\"command\":\"help\"} for info";
        }
    }
    else
    {
        jsOut["result"] = false;
        jsOut["error"] = "bad format of request. example of valid one: {\"command\":\"new\", \"cpus\":1}";
    }

    server->ReplyAndCloseConnection(jsOut);
}

void AServerOverseer::processCommandNew(const QJsonObject& jsIn, QJsonObject& jsOut)
{
    int requestedCPUs = 1;
    if (jsIn.contains("cpus"))
            requestedCPUs = jsIn["cpus"].toInt();

    if (requestedCPUs > CPUpool)
    {
        jsOut["result"] = false;
        jsOut["error"] = "available cpus=" + QString::number(CPUpool);
    }
    else
    {
        int port = findFreePort();
        if (port == -1)
        {
            jsOut["result"] = false;
            jsOut["error"] = "no free ports";
        }
        else
        {
            AServerRecord* sr = startProcess(port, requestedCPUs);

            jsOut["result"] = true;
            jsOut["port"] = port;
            jsOut["ticket"] = sr->Ticket;
        }
    }
}

void AServerOverseer::processCommandAbort(const QJsonObject &jsIn, QJsonObject &jsOut)
{
    QString Ticket = jsIn["ticket"].toString();

    for (AServerRecord* sr : RunningServers)
        if (Ticket == sr->Ticket)
        {
            sr->Process->close();
            jsOut["result"] = true;
            return;
        }

    jsOut["result"] = false;
    jsOut["error"] = "server with this ticket not found";
}

void AServerOverseer::processCommandReport(const QJsonObject &jsIn, QJsonObject &jsOut)
{
    jsOut["result"] = true;
    jsOut["cpus"] = CPUpool;
    jsOut["ports"] = AllocatedPorts.size() - RunningServers.size();

}

void AServerOverseer::processCommandHelp(const QJsonObject &jsIn, QJsonObject &jsOut)
{
    jsOut["result"] = true;
    jsOut["new"]  =   "{\"command\":\"new\", \"cpus\":x} - cpus=number of cpus to use; reply contains port and ticket";
    jsOut["abort"]  = "{\"command\":\"abort\"} - ticket=id of the server sesson to abort";
    jsOut["report"] = "{\"command\":\"report\"} - returns the available resources (CPUs and ports)";
}

AServerRecord* AServerOverseer::startProcess(int port, int numCPUs)
{
    QString command = "calc";//"ants2";
    QStringList arguments;

    QString ticket = generateTicket();

    QProcess *process = new QProcess(this);

    AServerRecord* sr = new AServerRecord(port, numCPUs, ticket, 0, process);
    CPUpool -= numCPUs;
    RunningServers << sr;

    QString str = command + " ";
    for (const QString &s : arguments) str += s + " ";
    qDebug() << "Executing command:" << str;

    QObject::connect(process, SIGNAL(finished(int)), sr, SLOT(processTerminated()));
    QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    QObject::connect(sr, &AServerRecord::finished, this, &AServerOverseer::oneProcessFinished);

    process->start(command, arguments);

    qDebug() << "-->New server started"<<sr;
    qDebug() << "  Available CPUs:"<<CPUpool;

    return sr;
}

void AServerOverseer::oneProcessFinished(AServerRecord *record)
{
    qDebug() << "<--Server process finished" << record;

    RunningServers.removeAll(record);
    CPUpool += record->NumCPUs;

    qDebug() << "  Available CPUs:"<<CPUpool;

    delete record;
}

int AServerOverseer::findFreePort()
{
    for (int port : AllocatedPorts)
    {
        bool bFound = false;
        for (AServerRecord* sr : RunningServers)
            if (port == sr->Port)
            {
                bFound = true;
                break;
            }
        if (!bFound) return port;
    }
    return -1;
}

void AServerRecord::processTerminated()
{
    qDebug() << "finished!";
    emit finished(this);
}

const QJsonObject AServerOverseer::objectFromString(const QString& in)
{
    QJsonObject obj;
    QJsonDocument doc = QJsonDocument::fromJson(in.toUtf8());
    if ( !doc.isNull() && doc.isObject() ) obj = doc.object();
    return obj;
}

const QString AServerOverseer::generateTicket()
{
    const QString possibleSymbols("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString Ticket;
    bool bFound = false;
    do
    {
        for (int i=0; i<4; i++)
        {
            int index = qrand() % possibleSymbols.length();
            QChar nextChar = possibleSymbols.at(index);
            Ticket.append(nextChar);
        }

        for (AServerRecord* sr : RunningServers)
            if (Ticket == sr->Ticket)
            {
                bFound = true;
                break;
            }
        if (!bFound) return Ticket;
    }
    while (bFound);
}
