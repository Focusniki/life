#include "NetworkDiscovery.h"
#include <QNetworkDatagram>
#include <QDebug>

NetworkDiscovery::NetworkDiscovery(quint16 discoveryPort, QObject* parent)
    : QObject(parent), port_(discoveryPort)
{
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4, port_,
                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkDiscovery::processPendingDatagrams);
}

NetworkDiscovery::~NetworkDiscovery() { }

void NetworkDiscovery::startListening() {
    // Шлём broadcast-пакет «DISCOVER_OS_OVERVIEW»
    QByteArray query = "DISCOVER_OS_OVERVIEW";
    udpSocket->writeDatagram(query, QHostAddress::Broadcast, port_);
    // Теперь ждём, пока readPendingDatagrams() поймает ответ
}

void NetworkDiscovery::processPendingDatagrams() {
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        // Ожидаем «OS_OVERVIEW:<PORT>»
        if (data.startsWith("OS_OVERVIEW:")) {
            bool ok = false;
            quint16 tcpPort = data.mid(strlen("OS_OVERVIEW:")).toUShort(&ok);
            if (ok) {
                HostInfo host{ datagram.senderAddress().toString(), tcpPort };
                emit hostDiscovered(host);
            }
        }
        // иначе игнорируем
    }
}
