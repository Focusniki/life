#include "Server.h"
#include "ClientConnection.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QStorageInfo>
#include <QDateTime>
#include <QNetworkDatagram>
#include <QAbstractSocket>

Server::Server(QObject* parent)
    : QTcpServer(parent),
      discoverySocket(nullptr),
      tcpPort(0)
{}

void Server::startServer(quint16 port)
{
    tcpPort = port;

    discoverySocket = new QUdpSocket(this);

    discoverySocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);

    if (!discoverySocket->bind(QHostAddress::Any, port,
                              QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "Discovery socket bind error:" << discoverySocket->errorString();
    } else {
        connect(discoverySocket, &QUdpSocket::readyRead,
                this, &Server::handleDiscoveryRequest);
        qInfo() << "Discovery UDP socket bound to port" << port;
    }

    qInfo() << "Available IP addresses:";
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            qInfo() << " - " << address.toString();
        }
    }

    // Запуск TCP-сервера
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "TCP server listen error:" << this->errorString();
    }
    else {
        qInfo() << "TCP server successfully started on port" << port;
    }
}

void Server::handleDiscoveryRequest()   {
    qDebug() << "Available interfaces:";
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            qDebug() << "Interface:" << interface.name();
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                qDebug() << " - IP:" << entry.ip().toString()
                         << "Broadcast:" << entry.broadcast().toString();
            }
        }
    }

    while (discoverySocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = discoverySocket->receiveDatagram();
        QByteArray data = datagram.data();

        qDebug() << "Received UDP datagram from"
                 << datagram.senderAddress().toString()
                 << ":" << datagram.senderPort()
                 << "data:" << data;

        if (data == "Открытие_обзора_ОС") {
            QByteArray response = "Обзор_ОС:" + QByteArray::number(tcpPort);
            qint64 bytesSent = discoverySocket->writeDatagram(
                response,
                datagram.senderAddress(),
                datagram.senderPort()
            );

            qDebug() << "Sent response to"
                     << datagram.senderAddress().toString()
                     << ":" << datagram.senderPort()
                     << "bytes:" << bytesSent;
        }
    }
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    ClientConnection* connection = new ClientConnection(this);
    if (connection->setSocketDescriptor(socketDescriptor)) {
        clients.append(connection);
        connect(connection, &ClientConnection::disconnected, this, [this, connection]() {
            clients.removeOne(connection);
            connection->deleteLater();
        });
    }
    else {
        delete connection;
    }
}

// ========== System Information Methods ==========

QJsonObject Server::getSystemInfo() const
{
    QJsonObject info;

    // OS information
    info["os_name"] = QSysInfo::prettyProductName();
    info["kernel_version"] = QSysInfo::kernelVersion();

    // Uptime
    info["uptime"] = getUptimeInfo()["formatted"].toString();

    // CPU
    info["cpu"] = getCpuInfo();

    // Memory
    info["memory"] = getMemoryInfo();

    // Disks
    info["disks"] = getDiskInfo();

    return info;
}

QStringList Server::getUserList() const
{
    QStringList users;
#ifdef Q_OS_UNIX
    QFile file("/etc/passwd");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            QList<QByteArray> parts = line.split(':');
            if (parts.size() > 0 && parts[0] != "root") {
                users << QString::fromUtf8(parts[0]);
            }
        }
        file.close();
    }
#endif
    return users;
}

QJsonArray Server::getFileSystem(const QString& path) const
{
    QJsonArray files;
    QDir dir(path.isEmpty() ? QDir::rootPath() : path);

    if (!dir.exists()) return files;

    for (const QFileInfo& info : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden)) {
        QJsonObject file;
        file["name"] = info.fileName();
        file["path"] = info.absoluteFilePath();
        file["type"] = info.isDir() ? "Directory" : "File";
        file["size"] = static_cast<qint64>(info.size());
        file["permissions"] = QString::number(info.permissions(), 8); // Convert to octal
        file["owner"] = info.owner();
        file["group"] = info.group();
        file["created"] = info.birthTime().toString(Qt::ISODate);
        file["modified"] = info.lastModified().toString(Qt::ISODate);

        files.append(file);
    }
    return files;
}

QJsonArray Server::getProcessList() const
{
    QJsonArray processes;
#ifdef Q_OS_UNIX
    QProcess ps;
    ps.start("ps", {"-eo", "pid,user,pcpu,pmem,vsz,rss,tty,stat,start,time,comm"});
    ps.waitForFinished();

    QString output = ps.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (lines.size() > 0) lines.removeFirst(); // Удаляем заголовок

    for (const QString& line : lines) {
        QStringList parts = line.simplified().split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 11) continue;

        QJsonObject process;
        process["pid"] = parts[0];
        process["user"] = parts[1];
        process["cpu"] = parts[2];
        process["mem"] = parts[3];
        process["vsz"] = parts[4];
        process["rss"] = parts[5];
        process["tty"] = parts[6];
        process["stat"] = parts[7];
        process["start"] = parts[8];
        process["time"] = parts[9];

        // Команда может содержать пробелы
        process["command"] = parts.mid(10).join(' ');

        processes.append(process);
    }
#endif
    return processes;
}

// ========== System Management Methods ==========

bool Server::addUser(const QString& username, const QString& password)
{
#ifdef Q_OS_UNIX
    QProcess process;
    process.start("useradd", { "-m", username });
    process.waitForFinished();

    if (process.exitCode() != 0) return false;

    process.start("passwd", { username });
    process.write((password + "\n" + password + "\n").toUtf8());
    process.waitForFinished();

    return process.exitCode() == 0;
#else
    Q_UNUSED(username);
    Q_UNUSED(password);
    return false;
#endif
}

bool Server::removeUser(const QString& username)
{
#ifdef Q_OS_UNIX
    QProcess process;
    process.start("userdel", { "-r", username });
    process.waitForFinished();
    return process.exitCode() == 0;
#else
    Q_UNUSED(username);
    return false;
#endif
}

bool Server::changeUserPassword(const QString& username, const QString& password)
{
#ifdef Q_OS_UNIX
    QProcess process;
    process.start("passwd", { username });
    process.write((password + "\n" + password + "\n").toUtf8());
    process.waitForFinished();
    return process.exitCode() == 0;
#else
    Q_UNUSED(username);
    Q_UNUSED(password);
    return false;
#endif
}

bool Server::setFilePermissions(const QString& path, const QString& permissions)
{
    QFile file(path);
    if (!file.exists()) return false;

    bool ok;
    uint perm = permissions.toUInt(&ok, 8); // Read as octal
    if (!ok) return false;

    return file.setPermissions(static_cast<QFile::Permissions>(perm));
}

bool Server::manageService(const QString& service, const QString& action)
{
#ifdef Q_OS_UNIX
    QStringList args;
    if (action == "start") args << "start";
    else if (action == "stop") args << "stop";
    else if (action == "restart") args << "restart";
    else return false;

    args << service;

    QProcess process;
    process.start("systemctl", args);
    process.waitForFinished();
    return process.exitCode() == 0;
#else
    Q_UNUSED(service);
    Q_UNUSED(action);
    return false;
#endif
}

// ========== File Operations ==========

bool Server::uploadFile(const QString& remotePath, const QByteArray& data)
{
    QFile file(remotePath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    qint64 bytesWritten = file.write(data);
    file.close();

    return bytesWritten == data.size();
}

QByteArray Server::downloadFile(const QString& remotePath) const
{
    QFile file(remotePath);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray();

    QByteArray data = file.readAll();
    file.close();

    return data;
}

// ========== Private Helper Methods ==========

QJsonObject Server::getCpuInfo() const
{
    QJsonObject cpu;

#ifdef Q_OS_UNIX
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly)) {
        int cores = 0;
        QString model;

        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (line.startsWith("model name")) {
                model = QString::fromUtf8(line.split(':').last().trimmed());
                cores++;
            }
        }
        file.close();

        cpu["model"] = model;
        cpu["cores"] = cores;

        // Get usage
        QFile statFile("/proc/stat");
        if (statFile.open(QIODevice::ReadOnly)) {
            QByteArray firstLine = statFile.readLine();
            statFile.close();

            QList<QByteArray> values = firstLine.split(' ').mid(1);
            if (values.size() > 7) {
                qulonglong total = 0;
                for (const QByteArray& val : values) total += val.toULongLong();
                qulonglong idle = values[3].toULongLong();
                qulonglong usage = total - idle;

                cpu["usage_percent"] = usage * 100.0 / total;
            }
        }

        // Get temperature
        QDir thermalDir("/sys/class/thermal");
        QStringList thermalZones = thermalDir.entryList({ "thermal_zone*" }, QDir::Dirs);
        for (const QString& zone : thermalZones) {
            QFile typeFile(thermalDir.filePath(zone + "/type"));
            if (typeFile.open(QIODevice::ReadOnly) && typeFile.readLine().trimmed() == "x86_pkg_temp") {
                QFile tempFile(thermalDir.filePath(zone + "/temp"));
                if (tempFile.open(QIODevice::ReadOnly)) {
                    double temp = tempFile.readLine().toDouble() / 1000.0;
                    cpu["temperature"] = temp;
                    tempFile.close();
                    break;
                }
            }
        }
    }
#endif

    return cpu;
}

QJsonObject Server::getMemoryInfo() const
{
    QJsonObject mem;

#ifdef Q_OS_UNIX
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly)) {
        qulonglong total = 0, free = 0, available = 0;

        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (line.startsWith("MemTotal")) {
                total = line.split(':').last().trimmed().split(' ').first().toULongLong();
            }
            else if (line.startsWith("MemFree")) {
                free = line.split(':').last().trimmed().split(' ').first().toULongLong();
            }
            else if (line.startsWith("MemAvailable")) {
                available = line.split(':').last().trimmed().split(' ').first().toULongLong();
            }
        }
        file.close();

        mem["total"] = total * 1024; // Convert to bytes
        mem["free"] = free * 1024;
        mem["available"] = available * 1024;
        mem["used"] = (total - free) * 1024;
        mem["usage_percent"] = (total - available) * 100.0 / total;
    }
#endif

    return mem;
}

QJsonArray Server::getDiskInfo() const
{
    QJsonArray disks;
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            QJsonObject disk;
            disk["name"] = storage.displayName();
            disk["mount_point"] = storage.rootPath();
            disk["filesystem"] = QString::fromUtf8(storage.fileSystemType());
            disk["total"] = static_cast<qint64>(storage.bytesTotal());
            disk["free"] = static_cast<qint64>(storage.bytesFree());
            disk["available"] = static_cast<qint64>(storage.bytesAvailable());
            disk["used"] = static_cast<qint64>(storage.bytesTotal() - storage.bytesFree());

            if (storage.bytesTotal() > 0) {
                disk["usage_percent"] = (1.0 - static_cast<double>(storage.bytesFree()) / storage.bytesTotal()) * 100.0;
            } else {
                disk["usage_percent"] = 0.0;
            }

            disks.append(disk);
        }
    }
    return disks;
}

QJsonObject Server::getUptimeInfo() const
{
    QJsonObject uptime;

#ifdef Q_OS_UNIX
    QFile file("/proc/uptime");
    if (file.open(QIODevice::ReadOnly)) {
        QList<QByteArray> values = file.readAll().split(' ');
        if (!values.isEmpty()) {
            double seconds = values[0].toDouble();

            int days = static_cast<int>(seconds / (24 * 3600));
            seconds = std::fmod(seconds, 24 * 3600);
            int hours = static_cast<int>(seconds / 3600);
            seconds = std::fmod(seconds, 3600);
            int minutes = static_cast<int>(seconds / 60);

            uptime["seconds"] = values[0].toDouble();
            uptime["formatted"] = QString("%1 days, %2 hours, %3 minutes").arg(days).arg(hours).arg(minutes);
        }
        file.close();
    }
#endif    
    return uptime;
}

QJsonArray Server::getServiceList() const
{
    QJsonArray services;
#ifdef Q_OS_UNIX
    QProcess systemctl;
    systemctl.start("systemctl", {"list-units", "--type=service", "--all", "--no-legend", "--plain"});
    systemctl.waitForFinished();

    QString output = systemctl.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.simplified().split(' ');
        if (parts.size() > 0) {
            services.append(parts[0]);
        }
    }
#endif
    return services;
}
