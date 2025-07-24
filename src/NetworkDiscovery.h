#ifndef NETWORKDISCOVERY_H
#define NETWORKDISCOVERY_H

#include <QObject>
#include <QUdpSocket>

struct HostInfo {
    QString address;
    quint16 port;
};

class NetworkDiscovery : public QObject {
    Q_OBJECT
public:
    explicit NetworkDiscovery(quint16 discoveryPort, QObject* parent = nullptr);
    ~NetworkDiscovery();
    void startListening();   // запустит «запрос в эфир»
signals:
    void hostDiscovered(const HostInfo& host);
private slots:
    void processPendingDatagrams();
private:
    QUdpSocket* udpSocket;
    quint16 port_;   // порт для Discovery (тот же, что у сервера)
    quint16 lastReceivedTcpPort_;
};

#endif // NETWORKDISCOVERY_H
