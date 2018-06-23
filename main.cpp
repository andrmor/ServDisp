#include "aserveroverseer.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Dispatcher for ANTS2 web socket server(s)");
    parser.addHelpOption();
    parser.addPositionalArgument("config", QCoreApplication::translate("main", "Json file with settings"));
    parser.addPositionalArgument("port", QCoreApplication::translate("main", "Port for requests (web socket protocol)"));
    parser.addPositionalArgument("sockets", QCoreApplication::translate("main", "List of ports which can be given to the servers (e.g. 1111;3333;...)"));
    parser.addPositionalArgument("threads", QCoreApplication::translate("main", "Maximum number of sim/rec threads to use"));
    parser.addPositionalArgument("exec", QCoreApplication::translate("main", "Server file to execute"));
    parser.addPositionalArgument("ip", QCoreApplication::translate("main", "IP for requests"));

    QCommandLineOption configOption(QStringList() << "f" << "file",
                                  QCoreApplication::translate("main", "Sets the config file (command line arguments override settings from file)."),
                                  QCoreApplication::translate("main", "config"));
    parser.addOption(configOption);

    QCommandLineOption execOption(QStringList() << "e" << "exec",
                                  QCoreApplication::translate("main", "Sets the server file to execute."),
                                  QCoreApplication::translate("main", "exec"));
    parser.addOption(execOption);

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  QCoreApplication::translate("main", "Sets port for requests."),
                                  QCoreApplication::translate("main", "port"));
    parser.addOption(portOption);

    QCommandLineOption socketsOption(QStringList() << "s" << "sockets",
                                  QCoreApplication::translate("main", "Sets pool of allowed sockets."),
                                  QCoreApplication::translate("main", "sockets"));
    parser.addOption(socketsOption);

    QCommandLineOption threadsOption(QStringList() << "t" << "threads",
                                  QCoreApplication::translate("main", "Sets max number of sim/rec threads."),
                                  QCoreApplication::translate("main", "threads"));
    parser.addOption(threadsOption);

    QCommandLineOption ipOption(QStringList() << "i" << "ip",
                                  QCoreApplication::translate("main", "Sets IP for requests."),
                                  QCoreApplication::translate("main", "ip"));
    parser.addOption(ipOption);


    AServerOverseer overseer;

  // ----- processing command line input -----
    parser.process(a);

    if (parser.isSet(configOption))
    {
        QString fileName = parser.value(configOption);
        bool bOK = overseer.ConfigureFromFile( fileName );
        if (!bOK)
        {
            qDebug() << "Cannot access the file with settings: " << fileName;
            exit(1);
        }
    }

    if (parser.isSet(execOption))
    {
        QString sa = parser.value(execOption);
        overseer.SetServerApplication(sa);
    }
    if (parser.isSet(portOption))
    {
        quint16 Port = parser.value(portOption).toUShort();
        overseer.SetPort(Port);
    }
    if (parser.isSet(socketsOption))
    {
        QString serverPorts = parser.value(socketsOption);
        QStringList sockets = serverPorts.split(";", QString::SkipEmptyParts);
        QSet<quint16> ws;
        for (const QString s : sockets) ws << s.toUShort();
        overseer.SetServerPorts(ws);
    }
    if (parser.isSet(socketsOption))
    {
        int maxThreads = parser.value(threadsOption).toInt();
        overseer.SetMaxThreads(maxThreads);
    }
    if (parser.isSet(ipOption))
    {
        QString ip = parser.value(ipOption);
        overseer.SetIP(ip);
    }

    bool bOK = overseer.StartListen();

    if (bOK) return a.exec();
    else return -1;
}
