#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

class ClientManager : public QObject {
    Q_OBJECT
public:
    explicit ClientManager(QObject* parent = nullptr);

    bool isConnected() const;

    void connectToServer(const QString& host, quint16 port);

    void requestUserList();
    void requestSystemInfo();
    void requestFileSystem(const QString& path);
    void requestProcessList();
    void requestServiceList();
    void addUser(const QString& username, const QString& password);
    void removeUser(const QString& username);
    void changeUserPassword(const QString& username, const QString& password);
    void setFilePermissions(const QString& path, const QString& permissions);
    void manageService(const QString& service, const QString& action);

    void uploadFile(const QString& localPath, const QString& remotePath);
    void downloadFile(const QString& remotePath, const QString& localPath);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& errorString);

    void userListReceived(const QStringList& users);
    void systemInfoReceived(const QJsonObject& info);
    void fileSystemReceived(const QJsonArray& files);
    void processListReceived(const QJsonArray& processes);
    void operationFinished(const QString& methodName, const QJsonObject& result);
    void serviceListReceived(const QJsonArray& services);

    void fileDownloadFinished(bool success, const QString& message);
    void fileUploadFinished(bool success, const QString& message);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError);

private:
    void sendJson(const QJsonObject& obj, const QString& methodName);

    QTcpSocket* socket;
    quint32 blockSize;

    QMap<int, QString> pendingRequests;
    int nextId;
};

#endif // CLIENTMANAGER_H

