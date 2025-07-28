#include "Server.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtInfoMsg:
        txt = QString("Info: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        break;
    }

    // Логирование в файл
    QFile outFile("os_server.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << QDateTime::currentDateTime().toString() << " - " << txt << "\n";
    outFile.close();

    // Также выводим в консоль для отладки
    QTextStream(stdout) << txt << "\n";
}

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    // Установка метаданных приложения
    a.setApplicationName("OS_Overview_Server");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("YourCompany");

    // Настройка логирования
    qInstallMessageHandler(messageHandler);
    qInfo() << "Starting OS Overview Server...";

    // Загрузка конфигурации
    QSettings settings;
    quint16 port = settings.value("server/port", 45454).toUInt();
    qInfo() << "Configuration loaded. Port:" << port;

    // Создание и запуск сервера
    Server server;
    server.startServer(port);
    qInfo() << "Server initialized. Ready for connections.";

    return a.exec();
}
