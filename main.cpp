#include "aserveroverseer.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

const quint16 DefaultPort = 1234;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

       QCommandLineParser parser;
       parser.setApplicationDescription("Dispatcher for ANTS2 web socket server(s). Use webs socket protocol!\nIf white list is not provided, all IPs are allowed.");
       parser.addHelpOption();
       parser.addPositionalArgument("port", QCoreApplication::translate("main", "Port for requests (web socket protocol)"));
       parser.addPositionalArgument("file", QCoreApplication::translate("main", "File with allowed IP addresses"));

       QCommandLineOption portOption(QStringList() << "p" << "port",
               QCoreApplication::translate("main", "Sets server port."),
               QCoreApplication::translate("main", "port"));
       parser.addOption(portOption);

       QCommandLineOption listOption(QStringList() << "l" << "list",
               QCoreApplication::translate("main", "Sets white IP list."),
               QCoreApplication::translate("main", "file"));
       parser.addOption(listOption);

       parser.process(a);

       quint16 Port = parser .value(portOption).toUShort();
       if (Port == 0) Port = DefaultPort;
       QString FileName = parser.value(listOption);
       qDebug() << "Port =" << Port <<"White list file ="<< FileName;

    AServerOverseer overseer(Port);

    return a.exec();
}
