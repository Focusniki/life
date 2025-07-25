#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStorageInfo>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <cmath>

class ClientConnection;

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr);
    virtual ~Server() = default;
    void startServer(quint16 port);

    // System information methods
    QJsonObject getSystemInfo() const;
    QStringList getUserList() const;
    QJsonArray getFileSystem(const QString& path) const;
    QJsonArray getProcessList() const;

    // System management methods
    bool addUser(const QString& username, const QString& password);
    bool removeUser(const QString& username);
    bool changeUserPassword(const QString& username, const QString& password);
    bool setFilePermissions(const QString& path, const QString& permissions);
    bool manageService(const QString& service, const QString& action);

    // File operations
    bool uploadFile(const QString& remotePath, const QByteArray& data);
    QByteArray downloadFile(const QString& remotePath) const;

private slots:
    void handleDiscoveryRequest();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QJsonObject getCpuInfo() const;
    QJsonObject getMemoryInfo() const;
    QJsonArray getDiskInfo() const;
    QJsonObject getUptimeInfo() const;

    QList<ClientConnection*> clients;

    QUdpSocket* discoverySocket;
    quint16 tcpPort;
};

#endif // SERVER_H
