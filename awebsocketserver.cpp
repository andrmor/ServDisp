#include "awebsocketserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>

AWebSocketServer::AWebSocketServer(QObject *parent) :
    QObject(parent),
    server(new QWebSocketServer(QStringLiteral("Dispatcher for Ants2 server"), QWebSocketServer::NonSecureMode, this))
{
    connect(server, &QWebSocketServer::newConnection, this, &AWebSocketServer::onNewConnection, Qt::QueuedConnection);

    watchdog = new QTimer(this);
    watchdog->setSingleShot(true);
    watchdog->setInterval(watchdogInterval);
}

AWebSocketServer::~AWebSocketServer()
{
    server->close();
}

bool AWebSocketServer::StartListen(quint16 port)
{
    //if ( !server->listen(QHostAddress::Any, port) )
    if ( !server->listen(QHostAddress::AnyIPv4, port) )
    {
        qCritical("WebSocket server was unable to start!");
        exit(1);
    }

    qDebug() << "Dispatcher for ANTS2 server is listening.";
    qDebug() << "To connect use URL:" << GetUrl();

    return true;
}

bool AWebSocketServer::CanReply()
{
    if (client) return true;

    qDebug() << "AWebSocketSessionServer: cannot reply - connection not established";
    return false;
}

void AWebSocketServer::ReplyAndCloseConnection(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QString reply(doc.toJson(QJsonDocument::Compact));
    ReplyAndCloseConnection(reply);
}

void AWebSocketServer::ReplyAndCloseConnection(const QString &message)
{
    if ( !CanReply() ) return;

    qDebug() << "Reply text message:"<<message;

    client->sendTextMessage(message);
    client->close();  //QWebSocketProtocol::CloseCodeNormal, "test");
}

void AWebSocketServer::onNewConnection()
{
    qDebug() << "New connection attempt";
    QWebSocket *pSocket = server->nextPendingConnection();

    if (client)
    { //paranoic
        //deny - exclusive connections!
        qDebug() << "Connection denied: another client is already connected";
        pSocket->sendTextMessage("{\"result\":false, \"error\":\"another session is not yet finished\"}");
        pSocket->close();
    }
    else
    {
        qDebug() << "Connection established with" << pSocket->peerAddress().toString();
        client = pSocket;

        connect(pSocket, &QWebSocket::textMessageReceived, this, &AWebSocketServer::onTextMessageReceived);
        connect(pSocket, &QWebSocket::disconnected, this, &AWebSocketServer::onSocketDisconnected);

        client->sendTextMessage("{\"result\":true}");

        watchdog->start();
    }
}

void AWebSocketServer::onTextMessageReceived(const QString &message)
{
    qDebug() << "Text message received:\n--->\n"<<message << "\n<---";

    if (message.isEmpty())
    {
        if (client)
            client->sendTextMessage("PONG"); //used for watchdogs
    }
    else
       emit textMessageReceived(message);
}

void AWebSocketServer::onSocketDisconnected()
{
    qDebug() << "Client disconnected!";

    if (client)
    {
        client->close();
        client->deleteLater();
    }
    client = 0;
}

void AWebSocketServer::onWatchdogTriggered()
{
    onSocketDisconnected();
}

const QString AWebSocketServer::GetUrl() const
{
  //return server->serverUrl().toString();

    QString pre = "ws://";
    QString aft = QString(":") + QString::number(server->serverPort());

    QString local = QHostAddress(QHostAddress::LocalHost).toString();

    QString url = pre + local + aft;

    QString ip;
    for (const QHostAddress &address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
            ip = address.toString();
            break;
        }
    }
    if ( !ip.isEmpty() )
        url += QStringLiteral("  or  ") + pre + ip + aft;

    return url;
}

int AWebSocketServer::GetPort() const
{
    return server->serverPort();
}
