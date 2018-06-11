#include "aserveroverseer.h"
#include "awebsocketserver.h"

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>

bool AServerOverseer::ConfigureFromFile(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly))
      {
        qDebug() << "Reading options from config file:" << fileName;
        QByteArray ba = f.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(ba));
        QJsonObject json = loadDoc.object();

        ServerFileName = json["server"].toString();

        QJsonArray ar = json["server_arguments"].toArray();
        for (int i=0; i<ar.size(); i++) Arguments << ar[i].toString();

        Port = json["port"].toInt();

        ar = json["server_ports"].toArray();
        for (int i=0; i<ar.size(); i++) AllocatedPorts << ar[i].toInt();

        MaxThreads = json["threads"].toInt();

        f.close();
        return true;
      }
    else return false;
}

bool AServerOverseer::StartListen()
{
    if ( !isReady() ) return false;

    server = new AWebSocketServer(this);

    QObject::connect(server, &AWebSocketServer::textMessageReceived, this, &AServerOverseer::onMessageReceived);

    server->StartListen(Port);
    return true;
}

const QString AServerOverseer::GetListOfPorts()
{
    QString res;
    QList<int> l;
    for (auto i : AllocatedPorts) l << i;
    qSort(l.begin(), l.end());
    for (auto i : l)
    {
        if ( !res.isEmpty() ) res += ',';
        res += QString::number(i);
    }
    return res;
}

bool AServerOverseer::isReady()
{
    if (ServerFileName.isEmpty())
    {
        qDebug() << "Server application was not provided";
        return false;
    }
    QFileInfo fi(ServerFileName);
    if (!fi.exists())
    {
        qDebug() << "Not found server application:" << ServerFileName;
        return false;
    }
    if ( !fi.permission(QFile::ExeUser) )
    {
        qDebug() << "Server application: have no permission to execute:" << ServerFileName;
        return false;
    }
    if (Port == 0)
    {
        qDebug() << "Port for requests is not defined!";
        return false;
    }
    if (AllocatedPorts.isEmpty())
    {
        qDebug() << "No ports were allocated for the servers!";
        return false;
    }
    if (MaxThreads < 1) MaxThreads = 1;

    qDebug() << "Server executable:" << ServerFileName;
    if (!Arguments.isEmpty()) qDebug() << "Extra arguments:" << Arguments;

    qDebug() << "Port for requests:" << Port;
    qDebug() << "Allocated server ports:"<< GetListOfPorts();
    qDebug() << "Maximum number of heavy threads:"<<MaxThreads;
    return true;
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
    if (jsIn.contains("threads"))
            requestedCPUs = jsIn["threads"].toInt();
    if (requestedCPUs < 1) requestedCPUs = 1;

    if (requestedCPUs > MaxThreads)
    {
        jsOut["result"] = false;
        jsOut["error"] = "currently available threads=" + QString::number(MaxThreads);
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
    jsOut["threads"] = MaxThreads;
    jsOut["ports"] = AllocatedPorts.size() - RunningServers.size();
    jsOut["users"] = RunningServers.size();
}

void AServerOverseer::processCommandHelp(const QJsonObject &jsIn, QJsonObject &jsOut)
{
    jsOut["result"] = true;
    jsOut["new"]  =   "{\"command\":\"new\", \"cpus\":x} - threads=number of sim/rec threads to use; reply contains port and ticket";
    jsOut["abort"]  = "{\"command\":\"abort\"} - ticket=id of the server sesson to abort";
    jsOut["report"] = "{\"command\":\"report\"} - returns the available resources (threads and ports)";
}

AServerRecord* AServerOverseer::startProcess(int port, int numCPUs)
{
    QString command = "D:/QtProjects/ANTS2git/ANTS2/build-ants2-Desktop_Qt_5_8_0_MSVC2013_32bit-Release/release/ants2";  //"calc";
    QStringList arguments;

    QString ticket = generateTicket();

    arguments << "-o" << "d:/tmp/a.txt" << "-s" << "-p" << QString::number(port) << "-t" << ticket;

    QProcess *process = new QProcess(this);

    AServerRecord* sr = new AServerRecord(port, numCPUs, ticket, 0, process);
    MaxThreads -= numCPUs;
    RunningServers << sr;

    QString str = command + " ";
    for (const QString &s : arguments) str += s + " ";
    qDebug() << "Executing command:" << str;

    QObject::connect(process, SIGNAL(finished(int)), sr, SLOT(processTerminated()));
    QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    QObject::connect(sr, &AServerRecord::finished, this, &AServerOverseer::oneProcessFinished);

    process->start(command, arguments);

    qDebug() << "-->New server started"<<sr;
    qDebug() << "  Available CPUs:"<<MaxThreads;

    return sr;
}

void AServerOverseer::oneProcessFinished(AServerRecord *record)
{
    qDebug() << "<--Server process finished" << record;

    RunningServers.removeAll(record);
    MaxThreads += record->NumCPUs;

    qDebug() << "  Available CPUs:"<<MaxThreads;

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
