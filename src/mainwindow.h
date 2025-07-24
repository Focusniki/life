#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidget>
#include <QGroupBox>
#include <QSplitter>
#include "NetworkDiscovery.h"
#include "ClientManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

private slots:
    void onDiscoverClicked();
    void onHostDiscovered(const HostInfo& host);
    void onConnectClicked();
    void onConnected();
    void onConnectionError(const QString& errorString);
    void onUserListReceived(const QStringList& users);
    void onSystemInfoReceived(const QJsonObject& info);
    void onFileSystemReceived(const QJsonArray& files);
    void onFileUploadFinished(bool success, const QString& message);

    void onFileSelected();
    void onUploadFile();
    void onDownloadFile();
    void onSetPermissions();
    void onManageUser();
    void onManageService();

private:
    // Основные виджеты
    QTabWidget *tabWidget;

    // Вкладка подключения
    QWidget *connectionTab;
    QListWidget *hostsList;
    QPushButton *discoverButton;
    QPushButton *connectButton;
    QLabel *statusLabel;

    // Вкладка пользователей
    QWidget *usersTab;
    QListWidget *userListWidget;
    QPushButton *addUserButton;
    QPushButton *removeUserButton;
    QPushButton *changePasswordButton;

    // Вкладка файловой системы
    QWidget *filesTab;
    QTreeWidget *fileSystemTree;
    QPushButton *fileSelectButton;
    QPushButton *uploadButton;
    QPushButton *downloadButton;
    QPushButton *setPermissionsButton;
    QLabel *filePathLabel;
    QLabel *currentPathLabel;

    // Вкладка системы
    QWidget *systemTab;
    QLabel *osNameLabel;
    QLabel *kernelLabel;
    QLabel *uptimeLabel;
    QLabel *cpuModelLabel;
    QLabel *cpuCoresLabel;
    QLabel *cpuUsageLabel;
    QLabel *ramUsageLabel;
    QListWidget *diskList;

    // Вкладка служб
    QWidget *servicesTab;
    QListWidget *serviceList;
    QPushButton *serviceControlButton;

    NetworkDiscovery* discovery;
    ClientManager* clientMgr;
    QList<HostInfo> discoveredHosts;
    QString currentFilePath;

    void setupConnectionTab();
    void setupUsersTab();
    void setupFilesTab();
    void setupSystemTab();
    void setupServicesTab();
    void updateSystemInfo(const QJsonObject& info);
};

#endif // MAINWINDOW_H
