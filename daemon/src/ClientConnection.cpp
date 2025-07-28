#include "ClientConnection.h"
#include "Server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDebug>
#include <QDataStream>

ClientConnection::ClientConnection(Server* server, QObject* parent)
    : QObject(parent), socket(nullptr), server(server), blockSize(0)
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ClientConnection::disconnected);
}

bool ClientConnection::setSocketDescriptor(qintptr socketDescriptor)
{
    return socket->setSocketDescriptor(socketDescriptor);
}

void ClientConnection::onReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_14);
    in.setByteOrder(QDataStream::BigEndian);

    while (socket->bytesAvailable() > 0) {
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
            continue;
        }

        if (doc.isObject()) {
            processRequest(doc.object());
        }
    }
}

void ClientConnection::processRequest(const QJsonObject& request) // Процессинг запроса
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = request["id"];

    QString method = request["method"].toString();
    QJsonObject params = request["params"].toObject();

	if (method == "getSystemInfo") { // Лучше наверное через switch case реализовать
        response["result"] = server->getSystemInfo();
    }
    else if (method == "getUserList") {
        response["result"] = QJsonArray::fromStringList(server->getUserList());
    }
    else if (method == "getFileSystem") {
        QString path = params["path"].toString();
        response["result"] = server->getFileSystem(path);
    }
    else if (method == "getProcessList") {
        response["result"] = server->getProcessList();
    }
    else if (method == "addUser") {
        bool success = server->addUser(
            params["username"].toString(),
            params["password"].toString()
        );
        response["result"] = success;
        if (!success) response["error"] = "Failed to add user";
    }
    else if (method == "removeUser") {
        bool success = server->removeUser(params["username"].toString());
        response["result"] = success;
        if (!success) response["error"] = "Failed to remove user";
    }
    else if (method == "changeUserPassword") {
        bool success = server->changeUserPassword(
            params["username"].toString(),
            params["password"].toString()
        );
        response["result"] = success;
        if (!success) response["error"] = "Failed to change password";
    }
    else if (method == "setFilePermissions") {
        bool success = server->setFilePermissions(
            params["path"].toString(),
            params["permissions"].toString()
        );
        response["result"] = success;
        if (!success) response["error"] = "Failed to set permissions";
    }
    else if (method == "manageService") {
        bool success = server->manageService(
            params["service"].toString(),
            params["action"].toString()
        );
        response["result"] = success;
        if (!success) response["error"] = "Failed to manage service";
    }
    else if (method == "uploadFile") {
        QByteArray fileData = QByteArray::fromBase64(params["data"].toString().toUtf8());
        bool success = server->uploadFile(
            params["remotePath"].toString(),
            fileData
        );
        response["result"] = success;
        if (!success) response["error"] = "Failed to upload file";
    }
    else if (method == "downloadFile") {
        QByteArray fileData = server->downloadFile(params["remotePath"].toString());
        QJsonObject result;
        result["data"] = QString::fromUtf8(fileData.toBase64());
        response["result"] = result;
    }
    else if (method == "getServiceList") {
        response["result"] = server->getServiceList();
    }
    else {
        response["error"] = "Unknown method";
    }

    sendResponse(response);
}

void ClientConnection::sendResponse(const QJsonObject& response)
{
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray packet;
    quint32 len = data.size();
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << len;
    packet.append(data);

    socket->write(packet);
}

void ClientConnection::onDisconnected()
{
    emit disconnected();
    socket->deleteLater();
}
