#define WIN32_LEAN_AND_MEAN
#include "ClientManager.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QDebug>

ClientManager::ClientManager(QObject* parent)
    : QObject(parent), blockSize(0), nextId(1)
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &ClientManager::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &ClientManager::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ClientManager::disconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &ClientManager::onErrorOccurred);
}

void ClientManager::uploadFile(const QString& localPath, const QString& remotePath) {
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for upload:" << localPath;
        emit fileUploadFinished(false, "Failed to open file");
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QJsonObject request;
    request["method"] = "uploadFile";
    QJsonObject params;
    params["remotePath"] = remotePath;
    params["data"] = QString::fromUtf8(fileData.toBase64());
    request["params"] = params;
    sendJson(request, "uploadFile");
}

void ClientManager::downloadFile(const QString& remotePath, const QString& localPath) {
    QJsonObject request;
    request["method"] = "downloadFile";
    QJsonObject params;
    params["remotePath"] = remotePath;
    params["savePath"] = localPath;
    request["params"] = params;
    sendJson(request, "downloadFile");
}

void ClientManager::setFilePermissions(const QString& filePath, const QString& permissions) {
    QJsonObject request;
    request["method"] = "setFilePermissions";
    QJsonObject params;
    params["path"] = filePath;
    params["permissions"] = permissions;
    request["params"] = params;
    sendJson(request, "setFilePermissions");
}

void ClientManager::manageService(const QString& serviceName, const QString& action) {
    QJsonObject request;
    request["method"] = "manageService";
    QJsonObject params;
    params["service"] = serviceName;
    params["action"] = action;
    request["params"] = params;
    sendJson(request, "manageService");
}

void ClientManager::removeUser(const QString& username) {
    QJsonObject request;
    request["method"] = "removeUser";
    QJsonObject params;
    params["username"] = username;
    request["params"] = params;
    sendJson(request, "removeUser");
}

void ClientManager::addUser(const QString& username, const QString& password) {
    QJsonObject request;
    request["method"] = "addUser";
    QJsonObject params;
    params["username"] = username;
    params["password"] = password;
    request["params"] = params;
    sendJson(request, "addUser");
}

void ClientManager::changeUserPassword(const QString& username, const QString& newPassword) {
    QJsonObject request;
    request["method"] = "changeUserPassword";
    QJsonObject params;
    params["username"] = username;
    params["password"] = newPassword;
    request["params"] = params;
    sendJson(request, "changeUserPassword");
}

void ClientManager::connectToServer(const QString& host, quint16 port) {
    socket->connectToHost(host, port);
}

void ClientManager::onConnected() {
    emit connected();
}

void ClientManager::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    QString errorMsg;
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        errorMsg = "Сервер отверг подключение";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorMsg = "Сервер не найден";
        break;
    case QAbstractSocket::NetworkError:
        errorMsg = "Сетевая ошибка";
        break;
    default:
        errorMsg = socket->errorString();
    }

    qWarning() << "Connection error:" << errorMsg;
    emit connectionError(errorMsg);
}

void ClientManager::sendJson(const QJsonObject& baseObj, const QString& methodName) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Trying to send data while not connected";
        return;
    }
    QJsonObject obj = baseObj;
    int id = nextId++;
    obj["id"] = id;
    pendingRequests[id] = methodName;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray packet;
    quint32 len = data.size();
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << len;
    packet.append(data);
    socket->write(packet);
}

void ClientManager::requestUserList() {
    QJsonObject request;
    request["method"] = "getUserList";
    request["params"] = QJsonObject();
    sendJson(request, "getUserList");
}

void ClientManager::requestSystemInfo() {
    QJsonObject request;
    request["method"] = "getSystemInfo";
    sendJson(request, "getSystemInfo");
}

void ClientManager::requestFileSystem(const QString& path) {
    QJsonObject request;
    request["method"] = "getFileSystem";
    request["params"] = QJsonObject{{"path", path}};
    sendJson(request, "getFileSystem");
}

void ClientManager::requestProcessList() {
    QJsonObject request;
    request["method"] = "getProcessList";
    sendJson(request, "getProcessList");
}



void ClientManager::onReadyRead() {
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_14);
    in.setByteOrder(QDataStream::BigEndian);

    if (blockSize == 0) {
        if (socket->bytesAvailable() < static_cast<int>(sizeof(quint32))) return;
        in >> blockSize;
    }
    if (socket->bytesAvailable() < blockSize) return;

    QByteArray data;
    data.resize(blockSize);
    in.readRawData(data.data(), blockSize);
    blockSize = 0;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject response = doc.object();
    if (!response.contains("id")) {
        qWarning() << "JSON-RPC response without id";
        return;
    }
    int id = response["id"].toInt(-1);
    if (!pendingRequests.contains(id)) {
        qWarning() << "Unknown id in response:" << id;
        return;
    }
    QString method = pendingRequests.take(id);
    if (response.contains("error")) {
        QJsonObject err = response["error"].toObject();
        qWarning() << "Server returned error for" << method << ":" << err["message"].toString();
        return;
    }
    if (!response.contains("result")) {
        qWarning() << "Response for" << method << "has no result";
        return;
    }

    if (method == "getUserList") {
        QJsonArray array = response["result"].toArray();
        QStringList list;
        for (const auto& val : array) list << val.toString();
        emit userListReceived(list);
    } else if (method == "getSystemInfo") {
        emit systemInfoReceived(response["result"].toObject());
    } else if (method == "getFileSystem") {
        emit fileSystemReceived(response["result"].toArray());
    } else if (method == "getProcessList") {
        emit processListReceived(response["result"].toArray());
    } else if (method == "getServiceList") {
        emit serviceListReceived(response["result"].toArray());
    } else if (method == "downloadFile") {
        QJsonObject result = response["result"].toObject();
        QString savePath = result["savePath"].toString();
        QByteArray fileData = QByteArray::fromBase64(result["data"].toString().toUtf8());

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(fileData);
            file.close();
            emit fileDownloadFinished(true, "Download completed");
        } else {
            qWarning() << "Failed to save file to" << savePath;
        }
    } else if (method == "uploadFile") {
        emit fileUploadFinished(true, "Upload completed");
    } else {
        emit operationFinished(method, response["result"].toObject());
    }
}

bool ClientManager::isConnected() const {
    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

void ClientManager::requestServiceList()
{
    QJsonObject request;
    request["method"] = "getServiceList";
    sendJson(request, "getServiceList");
}
