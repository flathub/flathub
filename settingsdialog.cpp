#include "settingsdialog.h"
#include "settingsdialog.h"
#include "constants.h" // For Constants::APP_NAME and other settings keys

#include <QDialogButtonBox> // For OK/Cancel buttons
#include <QIcon> // Added for QIcon

SettingsDialog::SettingsDialog(QSettings* settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(tr("Glide Settings"));
    setWindowIcon(QIcon(":/icons/Glide.png")); // Use the same icon as main window
    setMinimumSize(400, 300);

    setupUI();
    loadSettings();

    // Connect the accepted signal to save settings
    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);

    // Connect language combo box to emit signal
    connect(m_languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        emit languageChanged(m_languageComboBox->itemData(index).toString());
    });
}

SettingsDialog::~SettingsDialog()
{
    // Settings object is owned by MainWindow, so no deletion here
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    setupGeneralTab();
    setupNetworkingTab();
    setupLoggingTab();

    mainLayout->addWidget(m_tabWidget);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::setupGeneralTab()
{
    QWidget* generalTab = new QWidget(this);
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);

    QGroupBox* languageGroup = new QGroupBox(tr("Language"), generalTab);
    QHBoxLayout* languageLayout = new QHBoxLayout(languageGroup);
    m_languageComboBox = new QComboBox(languageGroup);
    m_languageComboBox->addItem(tr("English"), "en");
    m_languageComboBox->addItem(tr("Arabic"), "ar");
    m_languageComboBox->addItem(tr("Russian"), "ru");
    languageLayout->addWidget(new QLabel(tr("Select Language:")));
    languageLayout->addWidget(m_languageComboBox);
    generalLayout->addWidget(languageGroup);

    QGroupBox* appBehaviorGroup = new QGroupBox(tr("Application Behavior"), generalTab);
    QVBoxLayout* appBehaviorLayout = new QVBoxLayout(appBehaviorGroup);
    
    QHBoxLayout* screenNameLayout = new QHBoxLayout();
    m_screenNameLineEdit = new QLineEdit(appBehaviorGroup);
    m_screenNameLineEdit->setPlaceholderText(tr("Enter screen name"));
    screenNameLayout->addWidget(new QLabel(tr("Screen Name:")));
    screenNameLayout->addWidget(m_screenNameLineEdit);
    appBehaviorLayout->addLayout(screenNameLayout);

    m_minimizeToTrayCheckBox = new QCheckBox(tr("Minimize to system tray"), appBehaviorGroup);
    // m_minimizeToTrayCheckBox->setEnabled(false); // Enabled as per user request
    appBehaviorLayout->addWidget(m_minimizeToTrayCheckBox);

    m_hideOnStartupCheckBox = new QCheckBox(tr("Hide on startup"), appBehaviorGroup);
    // m_hideOnStartupCheckBox->setEnabled(false); // Enabled as per user request
    appBehaviorLayout->addWidget(m_hideOnStartupCheckBox);

    m_startGlideOnStartupCheckBox = new QCheckBox(tr("Start Glide on system startup"), appBehaviorGroup);
    // m_startGlideOnStartupCheckBox->setEnabled(false); // Enabled as per user request
    appBehaviorLayout->addWidget(m_startGlideOnStartupCheckBox);

    generalLayout->addWidget(appBehaviorGroup);
    generalLayout->addStretch(); // Push content to top

    m_tabWidget->addTab(generalTab, tr("General"));
}

void SettingsDialog::setupNetworkingTab()
{
    QWidget* networkingTab = new QWidget(this);
    QVBoxLayout* networkingLayout = new QVBoxLayout(networkingTab);

    QGroupBox* connectionGroup = new QGroupBox(tr("Connection Settings"), networkingTab);
    QGridLayout* connectionLayout = new QGridLayout(connectionGroup);

    connectionLayout->addWidget(new QLabel(tr("Port:")), 0, 0);
    m_portSpinBox = new QSpinBox(connectionGroup);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(Constants::DEFAULT_UDP_PORT); // Use default UDP port from Constants
    connectionLayout->addWidget(m_portSpinBox, 0, 1);

    connectionLayout->addWidget(new QLabel(tr("Address:")), 1, 0);
    m_addressLineEdit = new QLineEdit(connectionGroup);
    m_addressLineEdit->setPlaceholderText(tr("e.g., 192.168.1.100"));
    connectionLayout->addWidget(m_addressLineEdit, 1, 1);

    networkingLayout->addWidget(connectionGroup);

    QGroupBox* securityGroup = new QGroupBox(tr("Security"), networkingTab);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityGroup);
    m_enableSslCheckBox = new QCheckBox(tr("Enable SSL/TLS"), securityGroup);
    securityLayout->addWidget(m_enableSslCheckBox);
    m_requireClientCertCheckBox = new QCheckBox(tr("Require client certification"), securityGroup);
    securityLayout->addWidget(m_requireClientCertCheckBox);
    networkingLayout->addWidget(securityGroup);
    networkingLayout->addStretch();

    m_tabWidget->addTab(networkingTab, tr("Networking"));
}

void SettingsDialog::setupLoggingTab()
{
    QWidget* loggingTab = new QWidget(this);
    QVBoxLayout* loggingLayout = new QVBoxLayout(loggingTab);

    QGroupBox* logLevelGroup = new QGroupBox(tr("Log Level"), loggingTab);
    QHBoxLayout* logLevelLayout = new QHBoxLayout(logLevelGroup);
    m_logLevelComboBox = new QComboBox(logLevelGroup);
    m_logLevelComboBox->addItem(tr("Error"), "ERROR");
    m_logLevelComboBox->addItem(tr("Warning"), "WARNING");
    m_logLevelComboBox->addItem(tr("Debug"), "DEBUG");
    m_logLevelComboBox->addItem(tr("Info"), "INFO");
    m_logLevelComboBox->addItem(tr("Note"), "NOTE");
    logLevelLayout->addWidget(new QLabel(tr("Minimum Log Level:")));
    logLevelLayout->addWidget(m_logLevelComboBox);
    loggingLayout->addWidget(logLevelGroup);

    QGroupBox* logFileGroup = new QGroupBox(tr("Log File"), loggingTab);
    QHBoxLayout* logFileLayout = new QHBoxLayout(logFileGroup);
    m_logFilePathLineEdit = new QLineEdit(logFileGroup);
    m_logFilePathLineEdit->setPlaceholderText(tr("/var/log/glide.log"));
    logFileLayout->addWidget(m_logFilePathLineEdit);
    m_browseLogFileButton = new QPushButton(tr("Browse..."), logFileGroup);
    connect(m_browseLogFileButton, &QPushButton::clicked, this, &SettingsDialog::browseLogFile);
    logFileLayout->addWidget(m_browseLogFileButton);
    loggingLayout->addWidget(logFileGroup);
    loggingLayout->addStretch();

    m_tabWidget->addTab(loggingTab, tr("Logging"));
}

void SettingsDialog::loadSettings()
{
    // General Tab
    m_languageComboBox->setCurrentIndex(m_languageComboBox->findData(m_settings->value(Constants::SettingsKeys::LANGUAGE, "en")));
    m_screenNameLineEdit->setText(m_settings->value(Constants::SettingsKeys::SCREEN_NAME, "").toString());
    m_minimizeToTrayCheckBox->setChecked(m_settings->value(Constants::SettingsKeys::MINIMIZE_TO_TRAY, false).toBool());
    m_hideOnStartupCheckBox->setChecked(m_settings->value(Constants::SettingsKeys::HIDE_ON_STARTUP, false).toBool());
    m_startGlideOnStartupCheckBox->setChecked(m_settings->value(Constants::SettingsKeys::START_ON_STARTUP, false).toBool());

    // Networking Tab
    m_portSpinBox->setValue(m_settings->value(Constants::SettingsKeys::NETWORK_PORT, Constants::DEFAULT_UDP_PORT).toInt());
    m_addressLineEdit->setText(m_settings->value(Constants::SettingsKeys::NETWORK_ADDRESS, "").toString());
    m_enableSslCheckBox->setChecked(m_settings->value(Constants::SettingsKeys::ENABLE_SSL, false).toBool());
    m_requireClientCertCheckBox->setChecked(m_settings->value(Constants::SettingsKeys::REQUIRE_CLIENT_CERT, false).toBool());

    // Logging Tab
    m_logLevelComboBox->setCurrentIndex(m_logLevelComboBox->findData(m_settings->value(Constants::SettingsKeys::LOG_LEVEL, "INFO")));
    m_logFilePathLineEdit->setText(m_settings->value(Constants::SettingsKeys::LOG_FILE_PATH, "/var/log/glide.log").toString());
}

void SettingsDialog::saveSettings()
{
    // General Tab
    m_settings->setValue(Constants::SettingsKeys::LANGUAGE, m_languageComboBox->currentData());
    m_settings->setValue(Constants::SettingsKeys::SCREEN_NAME, m_screenNameLineEdit->text());
    m_settings->setValue(Constants::SettingsKeys::MINIMIZE_TO_TRAY, m_minimizeToTrayCheckBox->isChecked());
    m_settings->setValue(Constants::SettingsKeys::HIDE_ON_STARTUP, m_hideOnStartupCheckBox->isChecked());
    m_settings->setValue(Constants::SettingsKeys::START_ON_STARTUP, m_startGlideOnStartupCheckBox->isChecked());

    // Networking Tab
    m_settings->setValue(Constants::SettingsKeys::NETWORK_PORT, m_portSpinBox->value());
    m_settings->setValue(Constants::SettingsKeys::NETWORK_ADDRESS, m_addressLineEdit->text());
    m_settings->setValue(Constants::SettingsKeys::ENABLE_SSL, m_enableSslCheckBox->isChecked());
    m_settings->setValue(Constants::SettingsKeys::REQUIRE_CLIENT_CERT, m_requireClientCertCheckBox->isChecked());

    // Logging Tab
    m_settings->setValue(Constants::SettingsKeys::LOG_LEVEL, m_logLevelComboBox->currentData());
    m_settings->setValue(Constants::SettingsKeys::LOG_FILE_PATH, m_logFilePathLineEdit->text());
}

void SettingsDialog::browseLogFile()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Select Log File Location"),
                                                    m_logFilePathLineEdit->text(),
                                                    tr("Log Files (*.log);;All Files (*)"));
    if (!filePath.isEmpty()) {
        m_logFilePathLineEdit->setText(filePath);
    }
}
