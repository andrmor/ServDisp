#ifndef AWEBSOCKETSERVER_H
#define AWEBSOCKETSERVER_H

#include <QObject>
#include <QTimer>

class QWebSocketServer;
class QWebSocket;
class QJsonObject;

class AWebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit AWebSocketServer(QObject *parent = 0);
    ~AWebSocketServer();

    bool StartListen(quint16 port);

    bool CanReply();
    void ReplyAndCloseConnection(const QJsonObject& json);
    void ReplyAndCloseConnection(const QString& message);

    const QString GetUrl() const;
    int GetPort() const;

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onSocketDisconnected();
    void onWatchdogTriggered();

signals:
    void textMessageReceived(const QString message);

private:
    QWebSocketServer *server = 0;
    QWebSocket* client = 0;

    QTimer* watchdog;
    int watchdogInterval = 5000; // 5 sec

private:
    void ProcessNewClient();

};

#endif // AWEBSOCKETSERVER_H
