#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QGroupBox>
#include <QFormLayout>
#include <QSplitter>
#include <QStyleFactory>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QFont>
#include <QApplication>

MainWindow::~MainWindow() {
    delete discovery;
    delete clientMgr;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      tabWidget(nullptr),
      connectionTab(nullptr),
      hostsList(nullptr),
      discoverButton(nullptr),
      connectButton(nullptr),
      statusLabel(nullptr),
      usersTab(nullptr),
      userListWidget(nullptr),
      addUserButton(nullptr),
      removeUserButton(nullptr),
      changePasswordButton(nullptr),
      filesTab(nullptr),
      fileSystemTree(nullptr),
      fileSelectButton(nullptr),
      uploadButton(nullptr),
      downloadButton(nullptr),
      setPermissionsButton(nullptr),
      filePathLabel(nullptr),
      currentPathLabel(nullptr),
      systemTab(nullptr),
      osNameLabel(nullptr),
      kernelLabel(nullptr),
      uptimeLabel(nullptr),
      cpuModelLabel(nullptr),
      cpuCoresLabel(nullptr),
      cpuUsageLabel(nullptr),
      ramUsageLabel(nullptr),
      diskList(nullptr),
      servicesTab(nullptr),
      serviceList(nullptr),
      serviceControlButton(nullptr),
      discovery(nullptr),
      clientMgr(nullptr),
      currentFilePath("")
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Тёмная темка
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);
    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    setWindowTitle("OS-Client");
    resize(1000, 700);

    discovery = new NetworkDiscovery(45454, this);
    clientMgr  = new ClientManager(this);

    //Основной виджет и компоновка
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    //Панель инструментов
    QToolBar *toolBar = new QToolBar("Main Toolbar", this);
    toolBar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, toolBar);

    //Элементы на панель инструментов
    QAction* discoverAction = new QAction("Обнаружить серверы", this);
    QAction* connectAction = new QAction("Подключиться", this);
    toolBar->addAction(discoverAction);
    toolBar->addAction(connectAction);
    toolBar->addSeparator();

    //Вкладки
    tabWidget = new QTabWidget(centralWidget);
    tabWidget->setTabPosition(QTabWidget::North);
    tabWidget->setDocumentMode(true);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #444; }"
        "QTabBar::tab { background: #444; color: white; padding: 8px; }"
        "QTabBar::tab:selected { background: #2a82da; }"
        "QTabBar::tab:hover { background: #555; }"
    );

    mainLayout->addWidget(tabWidget);

    // Настраиваем вкладки
    setupConnectionTab();
    setupUsersTab();
    setupFilesTab();
    setupSystemTab();
    setupServicesTab();

    // Прогресс-бар в статусной строке
    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setFixedWidth(200);

    // Статусная строка
    statusBar()->addWidget(new QLabel("Статус:", this));
    statusLabel = new QLabel("Готов к работе", this);
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(progressBar);

    // Подключение сигналов
    connect(discoverAction, &QAction::triggered, this, &MainWindow::onDiscoverClicked);
    connect(connectAction, &QAction::triggered, this, &MainWindow::onConnectClicked);
    connect(discovery, &NetworkDiscovery::hostDiscovered, this, &MainWindow::onHostDiscovered);

    connect(clientMgr, &ClientManager::connected, this, &MainWindow::onConnected);
    connect(clientMgr, &ClientManager::connectionError, this, &MainWindow::onConnectionError);
    connect(clientMgr, &ClientManager::userListReceived, this, &MainWindow::onUserListReceived);
    connect(clientMgr, &ClientManager::systemInfoReceived, this, &MainWindow::onSystemInfoReceived);
    connect(clientMgr, &ClientManager::fileSystemReceived, this, &MainWindow::onFileSystemReceived);
    connect(clientMgr, &ClientManager::fileUploadFinished,
            this, &MainWindow::onFileUploadFinished);
    connect(clientMgr, &ClientManager::fileDownloadFinished,
            this, [this](bool success, const QString& message) {
        if (success) {
            QMessageBox::information(this, "Успех", "Файл успешно скачан");
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка скачивания: " + message);
        }
    });

    // Установка шрифтов
    QFont appFont("Segoe UI", 10);
    qApp->setFont(appFont);

    // Стилизация списков
    QString listStyle = "QListWidget, QTreeWidget { background: #252525; border: 1px solid #444; }"
                        "QListWidget::item, QTreeWidget::item { padding: 5px; }"
                        "QListWidget::item:selected, QTreeWidget::item:selected { background: #2a82da; }";

    hostsList->setStyleSheet(listStyle);
    userListWidget->setStyleSheet(listStyle);
    fileSystemTree->setStyleSheet(listStyle);
    diskList->setStyleSheet(listStyle);
    serviceList->setStyleSheet(listStyle);

    // Заголовки для дерева файлов
    fileSystemTree->setHeaderLabels({"Имя", "Тип", "Размер", "Права", "Владелец", "Группа"});
    fileSystemTree->setColumnWidth(0, 250);
    fileSystemTree->setColumnWidth(1, 80);
    fileSystemTree->setColumnWidth(2, 100);
    fileSystemTree->setColumnWidth(3, 80);
    fileSystemTree->setColumnWidth(4, 120);
    fileSystemTree->setColumnWidth(5, 120);

    // Включить чередование цветов строк
    hostsList->setAlternatingRowColors(true);
    userListWidget->setAlternatingRowColors(true);
    fileSystemTree->setAlternatingRowColors(true);
    diskList->setAlternatingRowColors(true);
    serviceList->setAlternatingRowColors(true);
}

void MainWindow::setupConnectionTab() {
    connectionTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(connectionTab);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Обнаружение серверов", connectionTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    // Список хостов
    hostsList = new QListWidget(connectionTab);
    hostsList->setMinimumHeight(200);
    layout->addWidget(hostsList);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    discoverButton = new QPushButton("Обновить список", connectionTab);
    connectButton = new QPushButton("Подключиться", connectionTab);

    // Стилизация кнопок
    QString buttonStyle = "QPushButton { background: #444; color: white; padding: 8px; border: none; }"
                          "QPushButton:hover { background: #555; }"
                          "QPushButton:pressed { background: #2a82da; }";

    discoverButton->setStyleSheet(buttonStyle);
    connectButton->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(discoverButton);
    buttonLayout->addWidget(connectButton);
    layout->addLayout(buttonLayout);

    // Подключение кнопок
    connect(discoverButton, &QPushButton::clicked, this, &MainWindow::onDiscoverClicked);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    tabWidget->addTab(connectionTab, "Подключение");
}

void MainWindow::setupUsersTab() {
    usersTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(usersTab);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Управление пользователями", usersTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    userListWidget = new QListWidget(usersTab);
    userListWidget->setMinimumHeight(300);
    layout->addWidget(userListWidget);

    // Кнопки управления пользователями
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addUserButton = new QPushButton("Добавить", usersTab);
    removeUserButton = new QPushButton("Удалить", usersTab);
    changePasswordButton = new QPushButton("Сменить пароль", usersTab);

    // Стилизация кнопок
    QString buttonStyle = "QPushButton { background: #444; color: white; padding: 8px; border: none; }"
                          "QPushButton:hover { background: #555; }"
                          "QPushButton:pressed { background: #2a82da; }";

    addUserButton->setStyleSheet(buttonStyle);
    removeUserButton->setStyleSheet(buttonStyle);
    changePasswordButton->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(addUserButton);
    buttonLayout->addWidget(removeUserButton);
    buttonLayout->addWidget(changePasswordButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    // Подключение кнопок
    connect(addUserButton, &QPushButton::clicked, this, &MainWindow::onManageUser);
    connect(removeUserButton, &QPushButton::clicked, this, &MainWindow::onManageUser);
    connect(changePasswordButton, &QPushButton::clicked, this, &MainWindow::onManageUser);

    tabWidget->addTab(usersTab, "Пользователи");
}

void MainWindow::setupFilesTab() {
    filesTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(filesTab);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Файловая система", filesTab);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2a82da;");
    layout->addWidget(titleLabel);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("Текущий путь:", filesTab);
    currentPathLabel = new QLabel("/", filesTab);
    currentPathLabel->setStyleSheet("font-weight: bold; color: #2a82da;");

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(currentPathLabel);
    pathLayout->addStretch();
    layout->addLayout(pathLayout);

    fileSystemTree = new QTreeWidget(filesTab);
    fileSystemTree->setMinimumHeight(300);
    fileSystemTree->setIconSize(QSize(24, 24));
    layout->addWidget(fileSystemTree, 1);

    QGridLayout *fileGridLayout = new QGridLayout();

    fileSelectButton = new QPushButton("Выбрать файл", filesTab);
    filePathLabel = new QLabel("Файл не выбран", filesTab);
    filePathLabel->setStyleSheet("color: #aaa;");

    uploadButton = new QPushButton("Загрузить", filesTab);
    downloadButton = new QPushButton("Скачать", filesTab);
    setPermissionsButton = new QPushButton("Права доступа", filesTab);

    QString buttonStyle = "QPushButton { background: #444; color: white; padding: 8px; border: none; }"
                          "QPushButton:hover { background: #555; }"
                          "QPushButton:pressed { background: #2a82da; }";

    fileSelectButton->setStyleSheet(buttonStyle);
    uploadButton->setStyleSheet(buttonStyle);
    downloadButton->setStyleSheet(buttonStyle);
    setPermissionsButton->setStyleSheet(buttonStyle);

    fileGridLayout->addWidget(fileSelectButton, 0, 0);
    fileGridLayout->addWidget(filePathLabel, 0, 1, 1, 3);
    fileGridLayout->addWidget(uploadButton, 1, 0);
    fileGridLayout->addWidget(downloadButton, 1, 1);
    fileGridLayout->addWidget(setPermissionsButton, 1, 2);

    layout->addLayout(fileGridLayout);

    connect(fileSelectButton, &QPushButton::clicked, this, &MainWindow::onFileSelected);
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::onUploadFile);
    connect(downloadButton, &QPushButton::clicked, this, &MainWindow::onDownloadFile);
    connect(setPermissionsButton, &QPushButton::clicked, this, &MainWindow::onSetPermissions);

    tabWidget->addTab(filesTab, "Файловая система");
}

void MainWindow::setupSystemTab() {
    systemTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(systemTab);

    // Информация об ОС
    QGroupBox *osGroup = new QGroupBox("Операционная система", systemTab);
    QFormLayout *osLayout = new QFormLayout(osGroup);
    osNameLabel = new QLabel("Неизвестно", osGroup);
    kernelLabel = new QLabel("Неизвестно", osGroup);
    uptimeLabel = new QLabel("Неизвестно", osGroup);
    osLayout->addRow("ОС:", osNameLabel);
    osLayout->addRow("Ядро:", kernelLabel);
    osLayout->addRow("Время работы:", uptimeLabel);

    // Информация о процессоре
    QGroupBox *cpuGroup = new QGroupBox("Процессор", systemTab);
    QFormLayout *cpuLayout = new QFormLayout(cpuGroup);
    cpuModelLabel = new QLabel("Неизвестно", cpuGroup);
    cpuCoresLabel = new QLabel("Неизвестно", cpuGroup);
    cpuUsageLabel = new QLabel("Неизвестно", cpuGroup);
    cpuLayout->addRow("Модель:", cpuModelLabel);
    cpuLayout->addRow("Ядра:", cpuCoresLabel);
    cpuLayout->addRow("Использование/Темп.:", cpuUsageLabel);

    // Информация о памяти
    QGroupBox *ramGroup = new QGroupBox("Память", systemTab);
    QFormLayout *ramLayout = new QFormLayout(ramGroup);
    ramUsageLabel = new QLabel("Неизвестно", ramGroup);
    ramLayout->addRow("Использование:", ramUsageLabel);

    // Диски
    QGroupBox *diskGroup = new QGroupBox("Диски", systemTab);
    QVBoxLayout *diskLayout = new QVBoxLayout(diskGroup);
    diskList = new QListWidget(diskGroup);
    diskLayout->addWidget(diskList);

    // Компоновка вкладки
    QSplitter *splitter = new QSplitter(Qt::Vertical, systemTab);
    splitter->addWidget(osGroup);
    splitter->addWidget(cpuGroup);
    splitter->addWidget(ramGroup);
    splitter->addWidget(diskGroup);
    layout->addWidget(splitter);

    tabWidget->addTab(systemTab, "Система");
}

void MainWindow::setupServicesTab() {
    servicesTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(servicesTab);

    serviceList = new QListWidget(servicesTab);
    layout->addWidget(serviceList);

    serviceControlButton = new QPushButton("Управление службой", servicesTab);
    layout->addWidget(serviceControlButton);

    connect(serviceControlButton, &QPushButton::clicked, this, &MainWindow::onManageService);

    tabWidget->addTab(servicesTab, "Службы");
}

// Реализации слотов
void MainWindow::onDiscoverClicked() {
    hostsList->clear();
    discoveredHosts.clear();
    discovery->startListening();
    statusLabel->setText("Поиск серверов...");
}

void MainWindow::onHostDiscovered(const HostInfo& host) {
    discoveredHosts.append(host);
    hostsList->addItem(QString("%1:%2").arg(host.address).arg(host.port));
    statusLabel->setText(QString("Найдено серверов: %1").arg(discoveredHosts.size()));
}

void MainWindow::onConnectClicked() {
    int idx = hostsList->currentRow();
    if (idx < 0 || idx >= discoveredHosts.size()) {
        QMessageBox::warning(this, "Ошибка", "Выберите сервер из списка.");
        return;
    }
    HostInfo h = discoveredHosts.at(idx);
    clientMgr->connectToServer(h.address, h.port);
    statusLabel->setText(QString("Подключение к %1:%2...").arg(h.address).arg(h.port));
}

void MainWindow::onConnected() {
    userListWidget->clear();
    clientMgr->requestUserList();
    clientMgr->requestSystemInfo();
    clientMgr->requestFileSystem("/");
    statusLabel->setText("Подключено. Загрузка данных...");
    tabWidget->setCurrentIndex(1); // Переключение на вкладку пользователей
}

void MainWindow::onConnectionError(const QString& errorString) {
    QMessageBox::critical(this, "Ошибка подключения", errorString);
    statusLabel->setText("Ошибка подключения: " + errorString);
}

void MainWindow::onUserListReceived(const QStringList& users) {
    userListWidget->clear();
    userListWidget->addItems(users);
}

void MainWindow::onSystemInfoReceived(const QJsonObject& info) {
    updateSystemInfo(info);
    statusLabel->setText("Системная информация обновлена");
}

void MainWindow::updateSystemInfo(const QJsonObject& info) {
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

void MainWindow::onFileSystemReceived(const QJsonArray& files) {
    fileSystemTree->clear();

    for (const QJsonValue& fileVal : files) {
        QJsonObject file = fileVal.toObject();
        QTreeWidgetItem* item = new QTreeWidgetItem(fileSystemTree);

        item->setText(0, file["name"].toString());
        item->setText(1, file["type"].toString());
        item->setText(2, QString::number(file["size"].toDouble() / 1024, 'f', 1) + " KB");
        item->setText(3, file["permissions"].toString());
        item->setText(4, file["owner"].toString());
        item->setText(5, file["group"].toString());

        // Сохраняем полный путь в данных
        item->setData(0, Qt::UserRole, file["path"].toString());
    }
}

void MainWindow::onFileUploadFinished(bool success, const QString& message) {
    if (success) {
        QMessageBox::information(this, "Успех", "Файл успешно загружен");
    } else {
        QMessageBox::warning(this, "Ошибка", "Ошибка загрузки: " + message);
    }
}

void MainWindow::onFileSelected() {
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл");
    if (!filePath.isEmpty()) {
        currentFilePath = filePath;
        filePathLabel->setText(QFileInfo(filePath).fileName());
    }
}

void MainWindow::onUploadFile() {
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Файл не выбран");
        return;
    }

    QString remotePath = currentPathLabel->text();
    if (remotePath.isEmpty()) remotePath = "/";

    // Показываем прогресс
    QProgressBar* progressBar = qobject_cast<QProgressBar*>(statusBar()->children().last());
    if (progressBar) {
        progressBar->setVisible(true);
        progressBar->setRange(0, 0);
    }
    statusLabel->setText("Загрузка файла на сервер...");

    clientMgr->uploadFile(currentFilePath, remotePath);
}

void MainWindow::onDownloadFile() {
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

void MainWindow::onSetPermissions() {
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

void MainWindow::onManageUser() {
    QString action = QInputDialog::getItem(this, "Управление пользователями",
        "Действие:", {"Добавить", "Удалить", "Изменить пароль"}, 0, false);

    if (action.isEmpty()) return;

    QString username;
    if (action != "Добавить") {
        username = QInputDialog::getText(this, "Пользователь", "Имя пользователя:");
        if (username.isEmpty()) return;
    }

    if (action == "Добавить") {
        username = QInputDialog::getText(this, "Новый пользователь", "Имя пользователя:");
        if (username.isEmpty()) return;

        QString password = QInputDialog::getText(this, "Пароль", "Пароль:", QLineEdit::Password);
        clientMgr->addUser(username, password);
    }
    else if (action == "Удалить") {
        clientMgr->removeUser(username);
    }
    else if (action == "Изменить пароль") {
        QString password = QInputDialog::getText(this, "Новый пароль", "Пароль:", QLineEdit::Password);
        clientMgr->changeUserPassword(username, password);
    }

    statusLabel->setText(action + " пользователя...");
}

void MainWindow::onManageService() {
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
