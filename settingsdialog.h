#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog> // For browse button

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QSettings* settings, QWidget *parent = nullptr);
    ~SettingsDialog();

signals:
    void languageChanged(const QString& langCode); // Signal emitted when language changes

private slots:
    void browseLogFile();
    void saveSettings(); // Slot to save settings when dialog is accepted

private:
    void setupUI();
    void setupGeneralTab();
    void setupNetworkingTab();
    void setupLoggingTab();
    void loadSettings();
    
    QSettings* m_settings;

    QTabWidget* m_tabWidget;

    // General Tab UI elements
    QComboBox* m_languageComboBox;
    QLineEdit* m_screenNameLineEdit;
    QCheckBox* m_minimizeToTrayCheckBox;
    QCheckBox* m_hideOnStartupCheckBox;
    QCheckBox* m_startGlideOnStartupCheckBox;

    // Networking Tab UI elements
    QSpinBox* m_portSpinBox;
    QLineEdit* m_addressLineEdit;
    QCheckBox* m_enableSslCheckBox;
    QCheckBox* m_requireClientCertCheckBox;

    // Logging Tab UI elements
    QComboBox* m_logLevelComboBox;
    QLineEdit* m_logFilePathLineEdit;
    QPushButton* m_browseLogFileButton;
};

#endif // SETTINGSDIALOG_H
