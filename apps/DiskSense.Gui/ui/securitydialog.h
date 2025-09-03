#ifndef UI_SECURITYDIALOG_H
#define UI_SECURITYDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <memory>
#include "core/security/securitymanager.h"

class SecurityManager;
class MalwareScanner;
class EncryptionDetector;

class SecurityDialog : public QDialog {
    Q_OBJECT

public:
    explicit SecurityDialog(QWidget *parent = nullptr);
    ~SecurityDialog() override;

public slots:
    void refreshSecurityIssues();
    void refreshScanSettings();
    void refreshPolicies();

private slots:
    void onStartSecurityScan();
    void onCancelSecurityScan();
    void onSaveReport();
    void onLoadReport();
    void onApplyPolicy();
    void onRemovePolicy();
    void onViewIssueDetails();
    void onFixIssue();
    
    void onSecurityScanStarted();
    void onSecurityScanProgress(int percentage, quint64 filesProcessed);
    void onSecurityScanCompleted(const QList<SecurityManager::SecurityIssue>& issues);
    void onSecurityScanCancelled();
    void onSecurityScanError(const QString& error);
    void onSecurityIssueFound(const SecurityManager::SecurityIssue& issue);
    
    void onMalwareScannerInitialized();
    void onMalwareDatabaseUpdated();
    
    void onEncryptionDetectionCompleted(const EncryptionDetector::DetectionResult& result);

private:
    void setupUI();
    void setupConnections();
    void populateSecurityIssues();
    void populateScanSettings();
    void populatePolicies();
    void updateScanStatus();
    void updateMalwareScannerStatus();
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Security scan tab
    QGroupBox* m_scanOptionsGroup;
    QLineEdit* m_scanPathEdit;
    QCheckBox* m_includeSubdirsCheck;
    QCheckBox* m_scanExecutablesCheck;
    QCheckBox* m_scanScriptsCheck;
    QCheckBox* m_scanArchivesCheck;
    QSpinBox* m_maxFileSizeSpin;
    QPushButton* m_startScanButton;
    QPushButton* m_cancelScanButton;
    QProgressBar* m_scanProgress;
    QLabel* m_scanStatus;
    
    QTreeWidget* m_issuesList;
    QPushButton* m_viewDetailsButton;
    QPushButton* m_fixIssueButton;
    
    // Settings tab
    QGroupBox* m_malwareGroup;
    QLabel* m_malwareStatus;
    QPushButton* m_updateMalwareButton;
    QPushButton* m_initializeScannerButton;
    
    QGroupBox* m_encryptionGroup;
    QLabel* m_encryptionStatus;
    QPushButton* m_initializeEncryptionButton;
    
    QGroupBox* m_scanSettingsGroup;
    QCheckBox* m_heuristicScanCheck;
    QCheckBox* m_packedFilesCheck;
    QCheckBox* m_scanArchivesSettingsCheck;
    
    // Policies tab
    QTableWidget* m_policiesTable;
    QPushButton* m_applyPolicyButton;
    QPushButton* m_removePolicyButton;
    QPushButton* m_newPolicyButton;
    
    // Core components
    std::unique_ptr<SecurityManager> m_securityManager;
    std::unique_ptr<MalwareScanner> m_malwareScanner;
    std::unique_ptr<EncryptionDetector> m_encryptionDetector;
    
    // Progress tracking
    QTimer* m_refreshTimer;
};

// Security policy editor dialog
class SecurityPolicyDialog : public QDialog {
    Q_OBJECT

public:
    explicit SecurityPolicyDialog(QWidget *parent = nullptr);
    ~SecurityPolicyDialog() override;

    void setPolicy(const QString& name, const QVariantMap& policy);
    QString policyName() const;
    QVariantMap policy() const;

private slots:
    void onSave();
    void onCancel();
    void onAddRule();
    void onRemoveRule();

private:
    void setupUI();
    void setupConnections();
    
    QString m_policyName;
    QVariantMap m_policy;
    
    // UI components
    QLineEdit* m_nameEdit;
    QTableWidget* m_rulesTable;
    
    QPushButton* m_addRuleButton;
    QPushButton* m_removeRuleButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

// Security issue details dialog
class SecurityIssueDialog : public QDialog {
    Q_OBJECT

public:
    explicit SecurityIssueDialog(const SecurityManager::SecurityIssue& issue, QWidget *parent = nullptr);
    ~SecurityIssueDialog() override;

private:
    void setupUI();
    void setupConnections();
    
    SecurityManager::SecurityIssue m_issue;
    
    // UI components
    QLabel* m_filePathLabel;
    QLabel* m_issueTypeLabel;
    QLabel* m_severityLabel;
    QLabel* m_detectedTimeLabel;
    QTextEdit* m_descriptionEdit;
    QTextEdit* m_detailsEdit;
    
    QPushButton* m_closeButton;
};

#endif // UI_SECURITYDIALOG_H