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
                jsOut["result"] = true;
                AServerRecord* sr = startProcess(port, requestedCPUs);
                jsOut["port"] = port;
            }
        }
    }
    else
    {
        jsOut["result"] = true;
        jsOut["error"] = "bad format of request. example of valid one: {\"command\":\"new\", \"cpus\":1}";
    }

    QJsonDocument doc(jsOut);
    QString reply(doc.toJson(QJsonDocument::Compact));
    server->ReplyAndCloseConnection(reply);
}

AServerRecord* AServerOverseer::startProcess(int port, int numCPUs)
{
    QString command = "calc";//"ants2";
    QStringList arguments;

    AServerRecord* sr = new AServerRecord(port, numCPUs, 0);
    CPUpool -= numCPUs;
    RunningServers << sr;

    QString str = command + " ";
    for (const QString &s : arguments) str += s + " ";
    qDebug() << "Executing command:" << str;

    QProcess *process = new QProcess(this);

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
    CPUpool += record->numCPUs;

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
