#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include <QRandomGenerator> // Added for QRandomGenerator
#include <QFileDialog> // Added for file dialog
#include <thread> // Added for std::thread
#include "file_transfer.h" // Include for file transfer functions
#include "constants.h"
#include <QIcon> // Added for QIcon
#include "settingsdialog.h" // Include the new settings dialog
#include <QTranslator> // Required for QTranslator
#include <QScrollBar> // Required for QScrollBar

GlideMainWindow::GlideMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_serverProcess(nullptr)
    , m_clientProcess(nullptr)
    , m_serverRunning(false)
    , m_clientConnected(false)
    , m_connectedClients(0)
    , m_settings(new QSettings(QString::fromStdString(Constants::APP_NAME), QString::fromStdString(Constants::SETTINGS_FILE_NAME), this))
    , m_translator(nullptr) // Initialize translator
{
    setWindowTitle(tr(Constants::APP_NAME.c_str()) + " - " + tr("Cross-Device Input Sharing"));
    setWindowIcon(QIcon(":/icons/Glide.png")); // You'll need to add this resource
    resize(800, 600);
    
    createMenus(); // Call the new menu creation function
    setupUI();
    setupTrayIcon();
    loadSettings();
    
    // Setup timers
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &GlideMainWindow::updateConnectionStatus);
    m_statusTimer->start(1000); // Update every second
    
    m_latencyTimer = new QTimer(this);
    m_latencyTimer->start(2000); // Update latency every 2 seconds
    
    logMessage(tr("%1 initialized successfully").arg(QString::fromStdString(Constants::APP_NAME)));
    logMessage(tr("Local IP: %1").arg(getLocalIPAddress()));
}

GlideMainWindow::~GlideMainWindow()
{
    saveSettings();
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        m_serverProcess->terminate();
        m_serverProcess->waitForFinished(3000);
    }
    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        m_clientProcess->terminate();
        m_clientProcess->waitForFinished(3000);
    }
}

void GlideMainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    setupServerGroupUI(contentLayout);
    setupClientGroupUI(contentLayout);
    setupFileTransferGroupUI(contentLayout);
    
    mainLayout->addLayout(contentLayout);
    
    setupLogGroupUI(mainLayout);
    
    // Status bar
    statusBar()->showMessage(tr("Ready"));
    
    // Local IP display
    m_localIPLabel = new QLabel(tr("Local IP: %1").arg(getLocalIPAddress()));
    m_localIPLabel->setStyleSheet("QLabel { font-weight: bold; color: blue; }");
    statusBar()->addPermanentWidget(m_localIPLabel);
    
    // Connect signals
    connect(m_startServerBtn, &QPushButton::clicked, this, &GlideMainWindow::startServer);
    connect(m_stopServerBtn, &QPushButton::clicked, this, &GlideMainWindow::stopServer);
    connect(m_connectBtn, &QPushButton::clicked, this, &GlideMainWindow::connectToServer);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &GlideMainWindow::disconnectFromServer);
    connect(m_browseFileBtn, &QPushButton::clicked, this, &GlideMainWindow::browseFile);
    connect(m_sendFileBtn, &QPushButton::clicked, this, &GlideMainWindow::sendFile);
}

void GlideMainWindow::setupServerGroupUI(QHBoxLayout* mainLayout)
{
    m_serverGroup = new QGroupBox(tr("🖥️ Server Mode"), this);
    m_serverGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* serverLayout = new QVBoxLayout(m_serverGroup);
    
    
    m_startServerBtn = new QPushButton(tr("Start Server"));
    m_startServerBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    m_stopServerBtn = new QPushButton(tr("Stop Server"));
    m_stopServerBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_stopServerBtn->setEnabled(false);
    
    serverLayout->addWidget(m_startServerBtn);
    serverLayout->addWidget(m_stopServerBtn);
    
    serverLayout->addWidget(new QLabel(tr("Status:")));
    m_serverStatusLabel = new QLabel(tr("Stopped"));
    m_serverStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    serverLayout->addWidget(m_serverStatusLabel);
    
    serverLayout->addWidget(new QLabel(tr("Connected Clients:")));
    m_connectedClientsLabel = new QLabel(tr("0"));
    m_connectedClientsLabel->setStyleSheet("QLabel { font-weight: bold; }");
    serverLayout->addWidget(m_connectedClientsLabel);
    
    mainLayout->addWidget(m_serverGroup);
}

void GlideMainWindow::setupClientGroupUI(QHBoxLayout* mainLayout)
{
    m_clientGroup = new QGroupBox(tr("📱 Client Mode"), this);
    m_clientGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* clientLayout = new QVBoxLayout(m_clientGroup);
    
    clientLayout->addWidget(new QLabel(tr("Server IP Address:")));
    m_serverIPEdit = new QLineEdit();
    m_serverIPEdit->setPlaceholderText(tr("e.g., 192.168.1.100"));
    clientLayout->addWidget(m_serverIPEdit);
    
    
    m_connectBtn = new QPushButton(tr("Connect to Server"));
    m_connectBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    m_disconnectBtn = new QPushButton(tr("Disconnect"));
    m_disconnectBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_disconnectBtn->setEnabled(false);
    
    clientLayout->addWidget(m_connectBtn);
    clientLayout->addWidget(m_disconnectBtn);
    
    clientLayout->addWidget(new QLabel(tr("Status:")));
    m_clientStatusLabel = new QLabel(tr("Disconnected"));
    m_clientStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    clientLayout->addWidget(m_clientStatusLabel);
    
    clientLayout->addWidget(new QLabel(tr("Latency:")));
    m_latencyLabel = new QLabel(tr("- ms"));
    m_latencyLabel->setStyleSheet("QLabel { font-weight: bold; }");
    clientLayout->addWidget(m_latencyLabel);
    
    mainLayout->addWidget(m_clientGroup);
}

void GlideMainWindow::setupFileTransferGroupUI(QHBoxLayout* mainLayout)
{
    m_fileTransferGroup = new QGroupBox(tr("📁 File Transfer"), this);
    m_fileTransferGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* fileTransferLayout = new QVBoxLayout(m_fileTransferGroup);

    QHBoxLayout* filePathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText(tr("Path to file to send..."));
    filePathLayout->addWidget(m_filePathEdit);
    m_browseFileBtn = new QPushButton(tr("Browse"));
    filePathLayout->addWidget(m_browseFileBtn);
    fileTransferLayout->addLayout(filePathLayout);

    m_sendFileBtn = new QPushButton(tr("Send File (TCP)"));
    m_sendFileBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    fileTransferLayout->addWidget(m_sendFileBtn);

    mainLayout->addWidget(m_fileTransferGroup);
}

void GlideMainWindow::setupLogGroupUI(QVBoxLayout* mainLayout)
{
    QGroupBox* logGroup = new QGroupBox(tr("📊 Activity Log"), this);
    logGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(200);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet("QTextEdit { background-color: #f5f5f5; font-family: Consolas, monospace; }");
    m_logTextEdit->setPlaceholderText(tr("Activity log will appear here...")); // Added placeholder text
    logLayout->addWidget(m_logTextEdit);
    
    mainLayout->addWidget(logGroup);
}

void GlideMainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(this, tr("System Tray"),
                              tr("System tray is not available on this system."));
        return;
    }

    // Create tray icon menu
    m_trayMenu = new QMenu(this);
    
    QAction* showAction = m_trayMenu->addAction(tr("Show"));
    connect(showAction, &QAction::triggered, this, &GlideMainWindow::restoreFromTray);
    
    QAction* aboutAction = m_trayMenu->addAction(tr("About"));
    connect(aboutAction, &QAction::triggered, this, &GlideMainWindow::showAbout);
    
    m_trayMenu->addSeparator();
    
    QAction* quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Create and setup tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/icons/Glide.png"));
    m_trayIcon->setToolTip(tr(Constants::APP_NAME.c_str()));

    connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            restoreFromTray();
        }
    });

    m_trayIcon->show();
}

void GlideMainWindow::startServer()
{
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        return;
    }

    m_serverProcess = new QProcess(this);
    connect(m_serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GlideMainWindow::onServerProcessFinished);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput, [this]() {
        logMessage("SERVER: " + m_serverProcess->readAllStandardOutput());
    });
    connect(m_serverProcess, &QProcess::readyReadStandardError, [this]() {
        logMessage("SERVER ERROR: " + m_serverProcess->readAllStandardError(), QString::fromStdString(Constants::LogTypes::ERROR));
    });

    QString program = "./build/server"; // Adjust path as needed
#ifdef _WIN32
    program = "build/server.exe";
#endif

    QStringList arguments;
    // The server needs to know the client's IP to send UDP packets.
    // For now, we'll assume the client IP is entered in the client's server IP field.
    // In a real scenario, this might be discovered or configured differently.
    QString clientIPForServer = m_serverIPEdit->text().trimmed(); 
    if (clientIPForServer.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter the Client IP Address in the Client Mode section to start the server!"));
        return;
    }
    arguments << clientIPForServer;

    int serverUdpPort = m_settings->value(Constants::SettingsKeys::SERVER_UDP_PORT, Constants::DEFAULT_UDP_PORT).toInt();
    logMessage(tr("Starting server, sending input to %1:%2...").arg(clientIPForServer).arg(serverUdpPort));
    
    m_serverProcess->start(program, arguments);
    
    if (m_serverProcess->waitForStarted()) {
        m_serverRunning = true;
        m_startServerBtn->setEnabled(false);
        m_stopServerBtn->setEnabled(true);
        m_serverStatusLabel->setText(tr("Running"));
        m_serverStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        logMessage(tr("Server started successfully!"), QString::fromStdString(Constants::LogTypes::SUCCESS));
        statusBar()->showMessage(tr("Server running, sending to %1").arg(clientIPForServer));
    } else {
        logMessage(tr("Failed to start server: %1").arg(m_serverProcess->errorString()), QString::fromStdString(Constants::LogTypes::ERROR));
        QMessageBox::critical(this, tr("Error"), tr("Failed to start server process!"));
    }
}

void GlideMainWindow::stopServer()
{
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        logMessage("Stopping server...");
        m_serverProcess->terminate();
        if (!m_serverProcess->waitForFinished(3000)) {
            m_serverProcess->kill();
        }
    }
    
    m_serverRunning = false;
    m_startServerBtn->setEnabled(true);
    m_stopServerBtn->setEnabled(false);
    m_serverStatusLabel->setText("Stopped");
    m_serverStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_connectedClientsLabel->setText("0");
    m_connectedClients = 0;
    logMessage(tr("Server stopped"), QString::fromStdString(Constants::LogTypes::SUCCESS));
    statusBar()->showMessage(tr("Server stopped"));
}

void GlideMainWindow::connectToServer()
{
    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address!"));
        return;
    }
    
    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        return;
    }

    m_clientProcess = new QProcess(this);
    connect(m_clientProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GlideMainWindow::onClientProcessFinished);
    connect(m_clientProcess, &QProcess::readyReadStandardOutput, [this]() {
        logMessage("CLIENT: " + m_clientProcess->readAllStandardOutput());
    });
    connect(m_clientProcess, &QProcess::readyReadStandardError, [this]() {
        logMessage("CLIENT ERROR: " + m_clientProcess->readAllStandardError(), QString::fromStdString(Constants::LogTypes::ERROR));
    });

    QString program = "./build/client"; // Adjust path as needed
#ifdef _WIN32
    program = "build/client.exe";
#endif

    QStringList arguments;
    arguments << serverIP; // Client needs server IP to bind its UDP socket to listen for server's UDP packets

    int clientUdpPort = m_settings->value(Constants::SettingsKeys::CLIENT_UDP_PORT, Constants::DEFAULT_UDP_PORT).toInt();
    logMessage(tr("Connecting to server at %1:%2 (UDP)...").arg(serverIP).arg(clientUdpPort));
    
    m_clientProcess->start(program, arguments);
    
    if (m_clientProcess->waitForStarted()) {
        m_clientConnected = true;
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
        m_clientStatusLabel->setText(tr("Connected"));
        m_clientStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        logMessage(tr("Connected to server successfully!"), QString::fromStdString(Constants::LogTypes::SUCCESS));
        statusBar()->showMessage(tr("Connected to %1 (UDP)").arg(serverIP));
    } else {
        logMessage(tr("Failed to connect to server: %1").arg(m_clientProcess->errorString()), QString::fromStdString(Constants::LogTypes::ERROR));
        QMessageBox::critical(this, tr("Error"), tr("Failed to connect to server process!"));
    }
}

void GlideMainWindow::disconnectFromServer()
{
    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        logMessage("Disconnecting from server...");
        m_clientProcess->terminate();
        if (!m_clientProcess->waitForFinished(3000)) {
            m_clientProcess->kill();
        }
    }
    
    m_clientConnected = false;
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
    m_clientStatusLabel->setText("Disconnected");
    m_clientStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_latencyLabel->setText(tr("- ms"));
    logMessage(tr("Disconnected from server"), QString::fromStdString(Constants::LogTypes::SUCCESS));
    statusBar()->showMessage(tr("Disconnected"));
}

void GlideMainWindow::browseFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select File to Send"));
    if (!filePath.isEmpty()) {
        m_filePathEdit->setText(filePath);
    }
}

void GlideMainWindow::sendFile()
{
    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address in the Client Mode section before sending a file!"));
        return;
    }

    QString filePath = m_filePathEdit->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a file to send!"));
        return;
    }

    logMessage(tr("Attempting to send file: %1 to %2 via TCP...").arg(filePath).arg(serverIP));
    
    // Run file sending in a separate thread to avoid blocking the GUI
    std::thread([serverIP, filePath, this]() {
#ifdef _WIN32
        send_file_tcp(serverIP.toStdString(), filePath.toStdString());
#else
        send_file_tcp_linux(serverIP.toStdString(), filePath.toStdString());
#endif
        // Log completion or error back on the main thread if needed
        // QMetaObject::invokeMethod(this, [this]() { logMessage("File transfer complete/failed."); });
    }).detach();
}


#include <QRandomGenerator> // Added for QRandomGenerator

void GlideMainWindow::updateConnectionStatus()
{
    // Update connected clients count (simulate for now)
    if (m_serverRunning) {
        // In a real implementation, you'd query the server process for this info
        static int lastCount = 0;
        if (QRandomGenerator::global()->bounded(10) == 0) { // Occasionally change the count
            lastCount = QRandomGenerator::global()->bounded(3);
            m_connectedClientsLabel->setText(QString::number(lastCount));
        }
    }
    
    // Update latency (simulate for now)
    if (m_clientConnected) {
        int latency = 5 + (QRandomGenerator::global()->bounded(20)); // 5-25ms
        m_latencyLabel->setText(QString::number(latency) + " ms");
    }
}

void GlideMainWindow::onServerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus); // Mark as unused to avoid warning if not used
    logMessage(tr("Server process finished with exit code: %1").arg(QString::number(exitCode)));
    if (exitCode != 0) {
        logMessage(tr("Server process ended unexpectedly"), QString::fromStdString(Constants::LogTypes::ERROR));
    }
    stopServer();
}

void GlideMainWindow::onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus); // Mark as unused to avoid warning if not used
    logMessage(tr("Client process finished with exit code: %1").arg(QString::number(exitCode)));
    if (exitCode != 0) {
        logMessage(tr("Client process ended unexpectedly"), QString::fromStdString(Constants::LogTypes::ERROR));
    }
    disconnectFromServer();
}

void GlideMainWindow::logMessage(const QString& message, const QString& type)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, type, message);
    
    QString color = "black";
    if (type == QString::fromStdString(Constants::LogTypes::ERROR)) color = "red";
    else if (type == QString::fromStdString(Constants::LogTypes::SUCCESS)) color = "green";
    else if (type == QString::fromStdString(Constants::LogTypes::WARNING)) color = "orange";
    
    m_logTextEdit->append(QString("<span style='color: %1'>%2</span>").arg(color, logEntry));
    
    // Keep log size manageable
    QTextDocument* doc = m_logTextEdit->document();
    if (doc->blockCount() > 100) {
        QTextCursor cursor = m_logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 10);
        cursor.removeSelectedText();
    }
}

QString GlideMainWindow::getLocalIPAddress()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address != QHostAddress::LocalHost && 
            address.toIPv4Address() && 
            !address.isLoopback()) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

void GlideMainWindow::loadSettings()
{
    m_serverIPEdit->setText(m_settings->value(Constants::SettingsKeys::CLIENT_SERVER_IP, "").toString());
    
    // Restore window geometry
    restoreGeometry(m_settings->value(Constants::SettingsKeys::WINDOW_GEOMETRY).toByteArray());
}

void GlideMainWindow::saveSettings()
{
    m_settings->setValue(Constants::SettingsKeys::CLIENT_SERVER_IP, m_serverIPEdit->text());
    m_settings->setValue(Constants::SettingsKeys::WINDOW_GEOMETRY, saveGeometry());
}

void GlideMainWindow::showAbout()
{
    QString aboutText = tr("%1 v1.0\n\n"
                           "Cross-platform input sharing application\n"
                           "Share mouse and keyboard across multiple devices seamlessly\n\n"
                           "Built with Qt and C++\n\n"
                           "Build Date: %2")
                            .arg(QString::fromStdString(Constants::APP_NAME))
                            .arg(QString(BUILD_DATETIME));

    QMessageBox::about(this, tr("About %1").arg(QString::fromStdString(Constants::APP_NAME)), aboutText);
}

void GlideMainWindow::minimizeToTray()
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        m_trayIcon->showMessage(tr(Constants::APP_NAME.c_str()), 
                                tr("Application minimized to system tray"),
                                QSystemTrayIcon::Information, 2000);
    }
}

void GlideMainWindow::restoreFromTray()
{
    show();
    raise();
    activateWindow();
}

void GlideMainWindow::createMenus()
{
    QMenu* glideMenu = menuBar()->addMenu(tr("&Glide")); // New menu named "Glide"
    
    QAction* activityLogAction = glideMenu->addAction(tr("Activity &Log"));
    activityLogAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(activityLogAction, &QAction::triggered, this, &GlideMainWindow::showActivityLog);

    QAction* settingsAction = glideMenu->addAction(tr("&Settings"));
    settingsAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(settingsAction, &QAction::triggered, this, &GlideMainWindow::showSettingsDialog);

    QAction* aboutAction = glideMenu->addAction(tr("&About Glide"));
    connect(aboutAction, &QAction::triggered, this, &GlideMainWindow::showAbout);
}

void GlideMainWindow::showActivityLog()
{
    // Ensure the main window is visible and brought to front
    if (isMinimized()) {
        showNormal();
    }
    raise();
    activateWindow();
    // Optionally, scroll the log to the bottom
    m_logTextEdit->verticalScrollBar()->setValue(m_logTextEdit->verticalScrollBar()->maximum());
}

void GlideMainWindow::showSettingsDialog()
{
    SettingsDialog settingsDialog(m_settings, this);
    connect(&settingsDialog, &SettingsDialog::languageChanged, this, &GlideMainWindow::handleLanguageChange);
    settingsDialog.exec(); // Show the dialog modally
}

void GlideMainWindow::retranslateUi()
{
    // Retranslate main window title
    setWindowTitle(tr(Constants::APP_NAME.c_str()) + " - " + tr("Cross-Device Input Sharing"));

    // Retranslate menu items
    menuBar()->clear(); // Clear existing menus
    createMenus(); // Recreate menus with new translations

    // Retranslate group box titles
    m_serverGroup->setTitle(tr("🖥️ Server Mode"));
    m_clientGroup->setTitle(tr("📱 Client Mode"));
    m_fileTransferGroup->setTitle(tr("📁 File Transfer"));
    // Retranslate labels and buttons
    m_startServerBtn->setText(tr("Start Server"));
    m_stopServerBtn->setText(tr("Stop Server"));
    m_serverStatusLabel->setText(tr("Stopped")); // Initial state
    m_connectedClientsLabel->setText(tr("0")); // Initial state
    m_connectBtn->setText(tr("Connect to Server"));
    m_disconnectBtn->setText(tr("Disconnect"));
    m_clientStatusLabel->setText(tr("Disconnected")); // Initial state
    m_latencyLabel->setText(tr("- ms")); // Initial state
    m_browseFileBtn->setText(tr("Browse"));
    m_sendFileBtn->setText(tr("Send File (TCP)"));
    m_logTextEdit->setPlaceholderText(tr("Activity log will appear here..."));
    m_localIPLabel->setText(tr("Local IP: %1").arg(getLocalIPAddress()));
    statusBar()->showMessage(tr("Ready")); // Initial state

    // Retranslate other labels in setup functions if they are not dynamic
    // For simplicity, I'm only retranslating the main window elements here.
    // A more robust solution would involve a base class for translatable widgets
    // or iterating through all children and calling retranslateUi on them.
}

void GlideMainWindow::handleLanguageChange(const QString& langCode)
{
    logMessage(tr("Language changed to: %1").arg(langCode), QString::fromStdString(Constants::LogTypes::INFO));

    if (m_translator) {
        QApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    m_translator = new QTranslator(this);
    QString translationsPath = QApplication::applicationDirPath() + "/translations"; // Translations are in a subdirectory
    if (m_translator->load("glide_" + langCode, translationsPath)) {
        QApplication::installTranslator(m_translator);
        logMessage(tr("Loaded translation for %1").arg(langCode), QString::fromStdString(Constants::LogTypes::SUCCESS));
    } else {
        logMessage(tr("Failed to load translation for %1").arg(langCode), QString::fromStdString(Constants::LogTypes::ERROR));
        delete m_translator;
        m_translator = nullptr;
    }

    // Retranslate the UI
    retranslateUi();
}
