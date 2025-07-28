#define WIN32_LEAN_AND_MEAN
#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QGroupBox>
#include <QFormLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenuBar>
#include <QActionGroup>
#include <QPalette>
#include <QSpacerItem>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      discovery(nullptr),
      clientMgr(nullptr),
      statusLabel(nullptr)
{
    initUI();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete discovery;
    delete clientMgr;
}

void MainWindow::initUI()
{
    setDarkTheme();

    setWindowTitle("ОС Клиент");
    resize(1200, 800);
    setMinimumSize(1000, 700);

    // Создание центрального виджета
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Основной лейаут
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Виджет вкладок
    tabWidget = new QTabWidget(centralWidget);
    tabWidget->setTabPosition(QTabWidget::North);
    tabWidget->setDocumentMode(true);
    mainLayout->addWidget(tabWidget);

    // Создание вкладок
    createConnectionTab();
    createUsersTab();
    createFilesTab();
    createSystemTab();
    createServicesTab();

    // Создание тулбара
    QToolBar *toolBar = new QToolBar("Main Toolbar", this);
    toolBar->setIconSize(QSize(28, 28));
    toolBar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolBar);

    // Действия
    QAction* discoverAction = new QAction("Обнаружить", this);
    QAction* connectAction = new QAction("Подключиться", this);
    QAction* refreshAction = new QAction("Обновить", this);
    QAction* testAction = new QAction("Тест связи", this);

    toolBar->addAction(testAction);
    toolBar->addAction(discoverAction);
    toolBar->addAction(connectAction);
    toolBar->addSeparator();
    toolBar->addAction(refreshAction);

    // Создание статусбара
    statusLabel = new QLabel("Готов к работе", this);
    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setFixedWidth(220);
    progressBar->setTextVisible(true);

    statusBar()->addWidget(new QLabel("Статус:", this));
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(progressBar);

    // Установка шрифтов
    QFont appFont("Segoe UI", 10);
    QApplication::setFont(appFont);

    // Инициализация управляющих объектов
    discovery = new NetworkDiscovery(45454, this);
    clientMgr = new ClientManager(this);

    // Подключение действий тулбара
    connect(testAction, &QAction::triggered, this, &MainWindow::onTestConnectionClicked);
    connect(discoverAction, &QAction::triggered, this, &MainWindow::onDiscoverClicked);
    connect(connectAction, &QAction::triggered, this, &MainWindow::onConnectClicked);
    connect(refreshAction, &QAction::triggered, this, [this]() {
        if (clientMgr && clientMgr->isConnected()) {
            clientMgr->requestSystemInfo();
            clientMgr->requestFileSystem(currentPathLabel->text());
        }
    });
}

void MainWindow::onTestConnectionClicked()
{
    QString testHost = QInputDialog::getText(this, "Тест связи", "Введите IP сервера:");
    if (!testHost.isEmpty()) {
        qDebug() << "Testing connection to" << testHost;
        QTcpSocket testSocket;
        testSocket.connectToHost(testHost, 45454);

        if (testSocket.waitForConnected(2000)) {
            QMessageBox::information(this, "Успех", "Соединение установлено!");
            testSocket.disconnectFromHost();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось подключиться: " + testSocket.errorString());
        }
    }
}

void MainWindow::onServiceListReceived(const QJsonArray& services)
{
    serviceList->clear();
    for (const QJsonValue& service : services) {
        serviceList->addItem(service.toString());
    }
}

void MainWindow::initConnections()
{
    connect(discoverButton, &QPushButton::clicked, this, &MainWindow::onDiscoverClicked);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(discovery, &NetworkDiscovery::hostDiscovered, this, &MainWindow::onHostDiscovered);
    connect(clientMgr, &ClientManager::connected, this, &MainWindow::onConnected);
    connect(clientMgr, &ClientManager::connectionError, this, &MainWindow::onConnectionError);
    connect(clientMgr, &ClientManager::userListReceived, this, &MainWindow::onUserListReceived);
    connect(clientMgr, &ClientManager::systemInfoReceived, this, &MainWindow::onSystemInfoReceived);
    connect(clientMgr, &ClientManager::fileSystemReceived, this, &MainWindow::onFileSystemReceived);
    connect(clientMgr, &ClientManager::fileUploadFinished, this, &MainWindow::onFileUploadFinished);
    connect(clientMgr, &ClientManager::serviceListReceived, this, &MainWindow::onServiceListReceived);
    connect(clientMgr, &ClientManager::fileDownloadFinished, this, [this](bool success, const QString& message) {
        if (success) {
            QMessageBox::information(this, "Успех", "Файл успешно скачан");
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка скачивания: " + message);
        }
    });
    connect(fileSelectButton, &QPushButton::clicked, this, &MainWindow::onFileSelected);
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::onUploadFile);
    connect(downloadButton, &QPushButton::clicked, this, &MainWindow::onDownloadFile);
    connect(setPermissionsButton, &QPushButton::clicked, this, &MainWindow::onSetPermissions);
    connect(addUserButton, &QPushButton::clicked, this, &MainWindow::onManageUser);
    connect(removeUserButton, &QPushButton::clicked, this, &MainWindow::onManageUser);
    connect(changePasswordButton, &QPushButton::clicked, this, &MainWindow::onManageUser);
    connect(serviceControlButton, &QPushButton::clicked, this, &MainWindow::onManageService);

}

void MainWindow::setDarkTheme()
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35,35,35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(60,60,60));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);

    QString globalStyle = R"(
        QWidget {
            background-color: #353535;
            color: #ffffff;
            font-family: 'Segoe UI';
            font-size: 10pt;
        }
        QTabWidget::pane {
            border: 1px solid #444;
            border-radius: 8px;
            background: #2a2a2a;
        }
        QTabBar::tab {
            background: #444;
            color: white;
            padding: 8px 16px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #2a82da;
        }
        QTabBar::tab:hover {
            background: #555;
        }
        QGroupBox {
            border: 1px solid #444;
            border-radius: 8px;
            margin-top: 1.5ex;
            padding-top: 10px;
        }
        QGroupBox::title {
            color: #2a82da;
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 0 8px;
        }
        QPushButton {
            background-color: #444;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            border: none;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #555;
        }
        QPushButton:pressed {
            background-color: #2a82da;
        }
        QListWidget, QTreeWidget {
            background: #252525;
            border: 1px solid #444;
            border-radius: 6px;
            alternate-background-color: #2d2d2d;
        }
        QListWidget::item, QTreeWidget::item {
            padding: 8px;
            border-radius: 4px;
        }
        QListWidget::item:selected, QTreeWidget::item:selected {
            background: #2a82da;
        }
        QHeaderView::section {
            background-color: #3a3a3a;
            color: white;
            padding: 4px;
            border: none;
        }
        QLabel {
            color: #ffffff;
        }
        QProgressBar {
            border: 1px solid #444;
            border-radius: 3px;
            text-align: center;
            background: #252525;
        }
        QProgressBar::chunk {
            background: #2a82da;
            border-radius: 2px;
        }
    )";
    qApp->setStyleSheet(globalStyle);
}

void MainWindow::createConnectionTab()
{
    connectionTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(connectionTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    QLabel *titleLabel = new QLabel("Обнаружение серверов", connectionTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    hostsList = new QListWidget(connectionTab);
    hostsList->setMinimumHeight(200);
    hostsList->setAlternatingRowColors(true);
    layout->addWidget(hostsList, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    discoverButton = new QPushButton("Обновить список", connectionTab);
    connectButton = new QPushButton("Подключиться", connectionTab);

    buttonLayout->addWidget(discoverButton);
    buttonLayout->addWidget(connectButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    tabWidget->addTab(connectionTab, "Подключение");
}

void MainWindow::createUsersTab()
{
    usersTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(usersTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    QLabel *titleLabel = new QLabel("Управление пользователями", usersTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    userListWidget = new QListWidget(usersTab);
    userListWidget->setMinimumHeight(300);
    userListWidget->setAlternatingRowColors(true);
    layout->addWidget(userListWidget, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addUserButton = new QPushButton("Добавить пользователя", usersTab);
    removeUserButton = new QPushButton("Удалить пользователя", usersTab);
    changePasswordButton = new QPushButton("Сменить пароль", usersTab);

    buttonLayout->addWidget(addUserButton);
    buttonLayout->addWidget(removeUserButton);
    buttonLayout->addWidget(changePasswordButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    tabWidget->addTab(usersTab, "Пользователи");
}

void MainWindow::createFilesTab()
{
    filesTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(filesTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    QLabel *titleLabel = new QLabel("Файловая система", filesTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("Текущий путь:", filesTab);
    currentPathLabel = new QLabel("/", filesTab);
    currentPathLabel->setStyleSheet("font-weight: bold; color: #2a82da;");

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(currentPathLabel, 1);
    pathLayout->addStretch();
    layout->addLayout(pathLayout);

    fileSystemTree = new QTreeWidget(filesTab);
    fileSystemTree->setMinimumHeight(300);
    fileSystemTree->setHeaderLabels({"Имя", "Тип", "Размер", "Права", "Владелец", "Группа"});
    fileSystemTree->setColumnWidth(0, 250);
    fileSystemTree->setAlternatingRowColors(true);
    layout->addWidget(fileSystemTree, 1);

    QGridLayout *fileGridLayout = new QGridLayout();
    fileSelectButton = new QPushButton("Выбрать файл", filesTab);
    filePathLabel = new QLabel("Файл не выбран", filesTab);
    filePathLabel->setStyleSheet("color: #aaa;");

    uploadButton = new QPushButton("Загрузить", filesTab);
    downloadButton = new QPushButton("Скачать", filesTab);
    setPermissionsButton = new QPushButton("Изменить права", filesTab);

    fileGridLayout->addWidget(fileSelectButton, 0, 0);
    fileGridLayout->addWidget(filePathLabel, 0, 1, 1, 3);
    fileGridLayout->addWidget(uploadButton, 1, 0);
    fileGridLayout->addWidget(downloadButton, 1, 1);
    fileGridLayout->addWidget(setPermissionsButton, 1, 2);
    fileGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3);

    layout->addLayout(fileGridLayout);
    tabWidget->addTab(filesTab, "Файловая система");
}

void MainWindow::createSystemTab()
{
    systemTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(systemTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    // Информация об ОС
    QGroupBox *osGroup = new QGroupBox("Операционная система", systemTab);
    QFormLayout *osLayout = new QFormLayout(osGroup);
    osLayout->setContentsMargins(15, 20, 15, 15);
    osLayout->setSpacing(10);
    osNameLabel = new QLabel("Неизвестно", osGroup);
    kernelLabel = new QLabel("Неизвестно", osGroup);
    uptimeLabel = new QLabel("Неизвестно", osGroup);
    osLayout->addRow("ОС:", osNameLabel);
    osLayout->addRow("Ядро:", kernelLabel);
    osLayout->addRow("Время работы:", uptimeLabel);

    // Информация о процессоре
    QGroupBox *cpuGroup = new QGroupBox("Процессор", systemTab);
    QFormLayout *cpuLayout = new QFormLayout(cpuGroup);
    cpuLayout->setContentsMargins(15, 20, 15, 15);
    cpuLayout->setSpacing(10);
    cpuModelLabel = new QLabel("Неизвестно", cpuGroup);
    cpuCoresLabel = new QLabel("Неизвестно", cpuGroup);
    cpuUsageLabel = new QLabel("Неизвестно", cpuGroup);
    cpuLayout->addRow("Модель:", cpuModelLabel);
    cpuLayout->addRow("Ядра:", cpuCoresLabel);
    cpuLayout->addRow("Использование/Темп.:", cpuUsageLabel);

    // Информация о памяти
    QGroupBox *ramGroup = new QGroupBox("Память", systemTab);
    QFormLayout *ramLayout = new QFormLayout(ramGroup);
    ramLayout->setContentsMargins(15, 20, 15, 15);
    ramLayout->setSpacing(10);
    ramUsageLabel = new QLabel("Неизвестно", ramGroup);
    ramLayout->addRow("Использование:", ramUsageLabel);

    // Диски
    QGroupBox *diskGroup = new QGroupBox("Диски", systemTab);
    QVBoxLayout *diskLayout = new QVBoxLayout(diskGroup);
    diskLayout->setContentsMargins(10, 20, 10, 10);
    diskList = new QListWidget(diskGroup);
    diskList->setAlternatingRowColors(true);
    diskLayout->addWidget(diskList);

    // Компоновка вкладки
    QSplitter *splitter = new QSplitter(Qt::Vertical, systemTab);
    splitter->addWidget(osGroup);
    splitter->addWidget(cpuGroup);
    splitter->addWidget(ramGroup);
    splitter->addWidget(diskGroup);
    splitter->setSizes({200, 150, 100, 200});
    layout->addWidget(splitter, 1);

    tabWidget->addTab(systemTab, "Система");
}

void MainWindow::createServicesTab()
{
    servicesTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(servicesTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    QLabel *titleLabel = new QLabel("Управление службами", servicesTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    serviceList = new QListWidget(servicesTab);
    serviceList->setMinimumHeight(300);
    serviceList->setAlternatingRowColors(true);
    layout->addWidget(serviceList, 1);

    serviceControlButton = new QPushButton("Управление службой", servicesTab);
    layout->addWidget(serviceControlButton);

    tabWidget->addTab(servicesTab, "Службы");
}

// Реализация слотов
void MainWindow::onDiscoverClicked() {
    hostsList->clear();
    discoveredHosts.clear();

    // Тестовый хост для ручной проверки
    HostInfo testHost;
    testHost.address = "127.0.0.1"; // или реальный IP сервера
    testHost.port = 45454;
    discoveredHosts.append(testHost);
    hostsList->addItem(QString("%1:%2").arg(testHost.address).arg(testHost.port));

    // Стандартное обнаружение
    discovery->startListening();
}

void MainWindow::onHostDiscovered(const HostInfo& host)
{
    discoveredHosts.append(host);
    hostsList->addItem(QString("%1:%2").arg(host.address).arg(host.port));
    statusLabel->setText(QString("Найдено серверов: %1").arg(discoveredHosts.size()));
}

void MainWindow::onConnectClicked()
{
    int idx = hostsList->currentRow();
    if (idx < 0 || idx >= discoveredHosts.size()) {
        QMessageBox::warning(this, "Ошибка", "Выберите сервер из списка.");
        return;
    }
    HostInfo h = discoveredHosts.at(idx);
    clientMgr->connectToServer(h.address, h.port);
    statusLabel->setText(QString("Подключение к %1:%2...").arg(h.address).arg(h.port));
}

void MainWindow::onConnected()
{
    userListWidget->clear();
    clientMgr->requestUserList();
    clientMgr->requestSystemInfo();
    clientMgr->requestServiceList();
    clientMgr->requestFileSystem("/");
    statusLabel->setText("Подключено. Загрузка данных...");
    tabWidget->setCurrentIndex(1); // Переключение на вкладку пользователей
}

void MainWindow::onConnectionError(const QString& errorString)
{
    QMessageBox::critical(this, "Ошибка подключения", errorString);
    statusLabel->setText("Ошибка подключения: " + errorString);
}

void MainWindow::onUserListReceived(const QStringList& users)
{
    userListWidget->clear();
    userListWidget->addItems(users);
}

void MainWindow::onSystemInfoReceived(const QJsonObject& info)
{
    updateSystemInfo(info);
    statusLabel->setText("Системная информация обновлена");
}

void MainWindow::updateSystemInfo(const QJsonObject& info)
{
    // Общая информация
    osNameLabel->setText(info["os_name"].toString());
    kernelLabel->setText(info["kernel_version"].toString());
    uptimeLabel->setText(info["uptime"].toString());

    // CPU
    cpuModelLabel->setText(info["cpu_model"].toString());
    cpuCoresLabel->setText(QString::number(info["cpu_cores"].toInt()));

    QJsonObject cpuLoad = info["cpu_load"].toObject();
    cpuUsageLabel->setText(
        QString("%1% / %2°C")
        .arg(cpuLoad["usage"].toDouble(), 0, 'f', 1)
        .arg(cpuLoad["temperature"].toDouble(), 0, 'f', 1)
    );

    // RAM
    QJsonObject memInfo = info["memory"].toObject();
    double totalMB = memInfo["total"].toDouble() / (1024 * 1024);
    double usedMB = memInfo["used"].toDouble() / (1024 * 1024);
    double usagePercent = (usedMB / totalMB) * 100;

    ramUsageLabel->setText(
        QString("%1 MB / %2 MB (%3%)")
        .arg(usedMB, 0, 'f', 1)
        .arg(totalMB, 0, 'f', 1)
        .arg(usagePercent, 0, 'f', 1)
    );

    // Диски
    QJsonArray disks = info["disks"].toArray();
    diskList->clear();
    for (const QJsonValue& diskVal : disks) {
        QJsonObject disk = diskVal.toObject();
        QString text = QString("%1: %2/%3 GB (%4%)")
            .arg(disk["mount_point"].toString())
            .arg(disk["used_gb"].toDouble(), 0, 'f', 1)
            .arg(disk["total_gb"].toDouble(), 0, 'f', 1)
            .arg(disk["usage_percent"].toDouble(), 0, 'f', 1);
        diskList->addItem(text);
    }
}

void MainWindow::onFileSystemReceived(const QJsonArray& files)
{
    fileSystemTree->clear();

    for (const QJsonValue& fileVal : files) {
        QJsonObject file = fileVal.toObject();
        QTreeWidgetItem* item = new QTreeWidgetItem(fileSystemTree);

        // Правильный формат размера
        double sizeKB = file["size"].toDouble() / 1024.0;
        QString sizeText = sizeKB > 1024 ?
            QString("%1 MB").arg(sizeKB/1024, 0, 'f', 1) :
            QString("%1 KB").arg(sizeKB, 0, 'f', 1);

        item->setText(0, file["name"].toString());
        item->setText(1, file["type"].toString());
        item->setText(2, sizeText);
        item->setText(3, file["permissions"].toString());
        item->setText(4, file["owner"].toString());
        item->setText(5, file["group"].toString());

        // Сохраняем полный путь
        item->setData(0, Qt::UserRole, file["path"].toString());
    }
}

void MainWindow::onFileUploadFinished(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "Успех", "Файл успешно загружен");
    } else {
        QMessageBox::warning(this, "Ошибка", "Ошибка загрузки: " + message);
    }
}

void MainWindow::onFileSelected()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл");
    if (!filePath.isEmpty()) {
        currentFilePath = filePath;
        filePathLabel->setText(QFileInfo(filePath).fileName());
    }
}

void MainWindow::onUploadFile()
{
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Файл не выбран");
        return;
    }

    QString remotePath = currentPathLabel->text();
    if (remotePath.isEmpty()) remotePath = "/";

    clientMgr->uploadFile(currentFilePath, remotePath);
}

void MainWindow::onDownloadFile()
{
    QTreeWidgetItem* item = fileSystemTree->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Ошибка", "Файл не выбран");
        return;
    }

    QString remotePath = item->data(0, Qt::UserRole).toString();
    QString savePath = QFileDialog::getSaveFileName(this, "Сохранить файл");

    if (!savePath.isEmpty()) {
        clientMgr->downloadFile(remotePath, savePath);
        statusLabel->setText("Скачивание файла с сервера...");
    }
}

void MainWindow::onSetPermissions()
{
    QTreeWidgetItem* item = fileSystemTree->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Ошибка", "Файл не выбран");
        return;
    }

    QString path = item->data(0, Qt::UserRole).toString();
    QString currentPerms = item->text(3);

    bool ok;
    QString newPerms = QInputDialog::getText(this, "Установка прав",
        "Новые права (например, 755):", QLineEdit::Normal, currentPerms, &ok);

    if (ok && !newPerms.isEmpty()) {
        clientMgr->setFilePermissions(path, newPerms);
        statusLabel->setText("Установка прав доступа...");
    }
}

void MainWindow::onManageUser()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    QString action;
    if (button == addUserButton) {
        action = "Добавить";
    } else if (button == removeUserButton) {
        action = "Удалить";
    } else if (button == changePasswordButton) {
        action = "Изменить пароль";
    } else {
        return;
    }

    if (action == "Добавить") {
        QString username = QInputDialog::getText(this, "Новый пользователь", "Имя пользователя:");
        if (username.isEmpty()) return;
        QString password = QInputDialog::getText(this, "Пароль", "Пароль:", QLineEdit::Password);
        clientMgr->addUser(username, password);
    } else {
        int idx = userListWidget->currentRow();
        if (idx < 0) {
            QMessageBox::warning(this, "Ошибка", "Выберите пользователя");
            return;
        }
        QString username = userListWidget->item(idx)->text();
        if (action == "Удалить") {
            clientMgr->removeUser(username);
        } else if (action == "Изменить пароль") {
            QString password = QInputDialog::getText(this, "Новый пароль", "Пароль:", QLineEdit::Password);
            clientMgr->changeUserPassword(username, password);
        }
    }
}

void MainWindow::onManageService()
{
    QListWidgetItem* item = serviceList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Ошибка", "Служба не выбрана");
        return;
    }

    QString service = item->text();
    QString action = QInputDialog::getItem(this, "Управление службой",
        "Действие:", {"Запустить", "Остановить", "Перезапустить"}, 0, false);

    if (!action.isEmpty()) {
        clientMgr->manageService(service, action);
        statusLabel->setText(action + " службы " + service + "...");
    }
}
