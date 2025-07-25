#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <QTcpSocket>
#include <QObject>

class Server;

class ClientConnection : public QObject
{
    Q_OBJECT
public:
    explicit ClientConnection(Server* server, QObject* parent = nullptr);
    virtual ~ClientConnection() = default;
    bool setSocketDescriptor(qintptr socketDescriptor);

signals:
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processRequest(const QJsonObject& request);
    void sendResponse(const QJsonObject& response);

    QTcpSocket* socket;
    Server* server;
    quint32 blockSize;
};

#endif // CLIENTCONNECTION_H
