#ifndef GLIDEMAINWINDOW_H
#define GLIDEMAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDateTime> // Added for QDateTime
#include <QMenuBar> // Added for QMenuBar
#include <QTranslator> // Added for QTranslator

class GlideMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    GlideMainWindow(QWidget *parent = nullptr);
    ~GlideMainWindow();

private slots:
    void startServer();
    void stopServer();
    void connectToServer();
    void disconnectFromServer();
    void updateConnectionStatus();
    void onServerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void showAbout();
    void minimizeToTray();
    void restoreFromTray();
    void browseFile();
    void sendFile();
    void showActivityLog(); // New slot for Activity Log
    void showSettingsDialog(); // New slot for Settings Dialog
    void handleLanguageChange(const QString& langCode); // New slot to handle language changes

private:
    void setupUI();
    void retranslateUi(); // Declare retranslateUi
    void setupTrayIcon();
    void createMenus(); // New function to create menus
    void loadSettings();
    void saveSettings();
    void logMessage(const QString& message, const QString& type = "INFO");
    QString getLocalIPAddress();

    // UI Setup Helpers
    void setupServerGroupUI(QHBoxLayout* parentLayout);
    void setupClientGroupUI(QHBoxLayout* parentLayout);
    void setupFileTransferGroupUI(QHBoxLayout* parentLayout);
    void setupLogGroupUI(QVBoxLayout* parentLayout);
    
    // UI Elements
    QWidget* m_centralWidget;
    
    // Server Group
    QGroupBox* m_serverGroup;
    QPushButton* m_startServerBtn;
    QPushButton* m_stopServerBtn;
    QLabel* m_serverStatusLabel;
    QLabel* m_connectedClientsLabel;
    
    // Client Group
    QGroupBox* m_clientGroup;
    QLineEdit* m_serverIPEdit;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel* m_clientStatusLabel;
    QLabel* m_latencyLabel;
    
    // File Transfer Group
    QGroupBox* m_fileTransferGroup;
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseFileBtn;
    QPushButton* m_sendFileBtn;

    // Status and Log
    QTextEdit* m_logTextEdit;
    QLabel* m_localIPLabel;
    
    // System Tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // Processes
    QProcess* m_serverProcess;
    QProcess* m_clientProcess;
    
    // Status tracking
    bool m_serverRunning;
    bool m_clientConnected;
    int m_connectedClients;
    
    // Timer for updates
    QTimer* m_statusTimer;
    QTimer* m_latencyTimer;
    
    // Settings
    QSettings* m_settings;
    QTranslator* m_translator; // For dynamic language switching
};

#endif // GlideMAINWINDOW_H
