#include "aserveroverseer.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Dispatcher for ANTS2 web socket server(s). Use webs socket protocol!\nIf white list is not provided, all IPs are allowed.");
    parser.addHelpOption();
    parser.addPositionalArgument("port", QCoreApplication::translate("main", "Port for requests (web socket protocol)"));
    //parser.addPositionalArgument("file", QCoreApplication::translate("main", "File with allowed IP addresses"));
    parser.addPositionalArgument("sockets", QCoreApplication::translate("main", "List of ports which can be given to the servers (e.g. 1111;3333;...)"));
    parser.addPositionalArgument("cpus", QCoreApplication::translate("main", "Maximum number of CPUs to use"));

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  QCoreApplication::translate("main", "Sets port for requests."),
                                  QCoreApplication::translate("main", "port"));
    parser.addOption(portOption);

    QCommandLineOption socketsOption(QStringList() << "s" << "sockets",
                                  QCoreApplication::translate("main", "Sets pool of allowed sockets."),
                                  QCoreApplication::translate("main", "sockets"));
    parser.addOption(socketsOption);

    QCommandLineOption cpusOption(QStringList() << "c" << "cpus",
                                  QCoreApplication::translate("main", "Sets max number of CPUs."),
                                  QCoreApplication::translate("main", "cpus"));
    parser.addOption(cpusOption);

    //QCommandLineOption listOption(QStringList() << "l" << "list",
    //                              QCoreApplication::translate("main", "Sets white IP list."),
    //                              QCoreApplication::translate("main", "file"));
    //parser.addOption(listOption);

    parser.process(a);

    quint16 Port = parser.value(portOption).toUShort();
    //QString FileName = parser.value(listOption);
    QString serverPorts = parser.value(socketsOption);
    QStringList sockets = serverPorts.split(";", QString::SkipEmptyParts);
    QSet<quint16> ws;
    for (const QString s : sockets) ws << s.toUShort();
    int MaxCPUs = parser.value(cpusOption).toInt();

    qDebug() << "Port for requests =" << Port << " Ports for servers="<<ws << " CPU pool="<<MaxCPUs;//<<"White list file ="<< FileName;

    AServerOverseer overseer(Port, ws, MaxCPUs);

    return a.exec();
}
