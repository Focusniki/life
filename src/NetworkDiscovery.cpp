#include "NetworkDiscovery.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface>
#include <QAbstractSocket>

NetworkDiscovery::NetworkDiscovery(quint16 discoveryPort, QObject* parent)
    : QObject(parent), port_(discoveryPort)
{
    udpSocket = new QUdpSocket(this);
    // Убираем AnyIPv4 - слушаем все адреса
    if (!udpSocket->bind(port_,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "UDP bind error:" << udpSocket->errorString();
    }
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &NetworkDiscovery::processPendingDatagrams);
}

NetworkDiscovery::~NetworkDiscovery() { }

void NetworkDiscovery::startListening() {
    qDebug() << "Sending discovery broadcast";

    // Создаем низкоуровневый UDP-сокет для широковещательной отправки
    QUdpSocket broadcastSocket;
    #ifdef Q_OS_WIN
        #include <winsock2.h>
        broadcastSocket.setSocketOption(QAbstractSocket::SocketOption, SO_BROADCAST, 1);
    #else
        broadcastSocket.setSocketOption(QAbstractSocket::SocketOption, SO_BROADCAST, 1);
    #endif

    QByteArray query = "Открытие_обзора_ОС";
    if (broadcastSocket.writeDatagram(query, QHostAddress::Broadcast, port_) == -1) {
        qWarning() << "Broadcast send error:" << broadcastSocket.errorString();
    } else {
        qDebug() << "Broadcast sent successfully";
    }

    // Дополнительно: сканирование локальной подсети
    QString localIp = getLocalIp();
    if (!localIp.isEmpty()) {
        QString baseIp = localIp.left(localIp.lastIndexOf('.') + 1);
        for (int i = 1; i < 255; ++i) {
            QString ip = baseIp + QString::number(i);
            udpSocket->writeDatagram(query, QHostAddress(ip), port_);
        }
    }
}

QString NetworkDiscovery::getLocalIp() {
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return "";
}

void NetworkDiscovery::processPendingDatagrams() {
    qDebug() << "Received datagram";
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();

        // Исправляем условие проверки ответа
        if (data.startsWith("Обзор_ОС:")) {
            bool ok = false;
            quint16 tcpPort = data.mid(9).toUShort(&ok); // Берем данные после префикса
            if (ok) {
                HostInfo host{ datagram.senderAddress().toString(), tcpPort };
                emit hostDiscovered(host);
            }
        }
    }
}
