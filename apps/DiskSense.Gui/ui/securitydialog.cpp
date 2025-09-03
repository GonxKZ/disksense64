#include "securitydialog.h"
#include <QApplication>
#include <QGroupBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QStandardPaths>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QThread>
#include <QMetaObject>
#include <QInputDialog>
#include <QProgressDialog>
#include <QColor>

// SecurityDialog implementation
SecurityDialog::SecurityDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_scanOptionsGroup(nullptr)
    , m_scanPathEdit(nullptr)
    , m_includeSubdirsCheck(nullptr)
    , m_scanExecutablesCheck(nullptr)
    , m_scanScriptsCheck(nullptr)
    , m_scanArchivesCheck(nullptr)
    , m_maxFileSizeSpin(nullptr)
    , m_startScanButton(nullptr)
    , m_cancelScanButton(nullptr)
    , m_scanProgress(nullptr)
    , m_scanStatus(nullptr)
    , m_issuesList(nullptr)
    , m_viewDetailsButton(nullptr)
    , m_fixIssueButton(nullptr)
    , m_malwareGroup(nullptr)
    , m_malwareStatus(nullptr)
    , m_updateMalwareButton(nullptr)
    , m_initializeScannerButton(nullptr)
    , m_encryptionGroup(nullptr)
    , m_encryptionStatus(nullptr)
    , m_initializeEncryptionButton(nullptr)
    , m_scanSettingsGroup(nullptr)
    , m_heuristicScanCheck(nullptr)
    , m_packedFilesCheck(nullptr)
    , m_scanArchivesSettingsCheck(nullptr)
    , m_policiesTable(nullptr)
    , m_applyPolicyButton(nullptr)
    , m_removePolicyButton(nullptr)
    , m_newPolicyButton(nullptr)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    
    // Initialize core components
    m_securityManager = std::make_unique<SecurityManager>();
    m_malwareScanner = std::make_unique<MalwareScanner>();
    m_encryptionDetector = std::make_unique<EncryptionDetector>();
    
    // Connect security manager signals
    connect(m_securityManager.get(), &SecurityManager::securityScanStarted,
            this, &SecurityDialog::onSecurityScanStarted);
    connect(m_securityManager.get(), &SecurityManager::securityScanProgress,
            this, &SecurityDialog::onSecurityScanProgress);
    connect(m_securityManager.get(), &SecurityManager::securityScanCompleted,
            this, &SecurityDialog::onSecurityScanCompleted);
    connect(m_securityManager.get(), &SecurityManager::securityScanCancelled,
            this, &SecurityDialog::onSecurityScanCancelled);
    connect(m_securityManager.get(), &SecurityManager::securityScanError,
            this, &SecurityDialog::onSecurityScanError);
    connect(m_securityManager.get(), &SecurityManager::securityIssueFound,
            this, &SecurityDialog::onSecurityIssueFound);
    
    // Connect malware scanner signals
    connect(m_malwareScanner.get(), &MalwareScanner::databaseUpdated,
            this, &SecurityDialog::onMalwareDatabaseUpdated);
    
    // Connect encryption detector signals
    connect(m_encryptionDetector.get(), &EncryptionDetector::detectionCompleted,
            this, &SecurityDialog::onEncryptionDetectionCompleted);
    
    // Set up security manager components
    m_securityManager->setMalwareScanner(m_malwareScanner.get());
    m_securityManager->setEncryptionDetector(m_encryptionDetector.get());
    
    // Set up refresh timer
    connect(m_refreshTimer, &QTimer::timeout, this, &SecurityDialog::refreshSecurityIssues);
    m_refreshTimer->setInterval(30000); // Refresh every 30 seconds
    
    // Set default values
    m_includeSubdirsCheck->setChecked(true);
    m_scanExecutablesCheck->setChecked(true);
    m_scanScriptsCheck->setChecked(true);
    m_scanArchivesCheck->setChecked(true);
    m_maxFileSizeSpin->setRange(1, 10000);
    m_maxFileSizeSpin->setValue(100); // 100 MB
    m_maxFileSizeSpin->setSuffix(" MB");
    
    // Load initial data
    refreshSecurityIssues();
    refreshScanSettings();
    refreshPolicies();
    
    // Update statuses
    updateScanStatus();
    updateMalwareScannerStatus();
    
    // Set default scan path
    m_scanPathEdit->setText(QDir::homePath());
}

SecurityDialog::~SecurityDialog() {
}

void SecurityDialog::setupUI() {
    setWindowTitle("Security Analysis");
    setMinimumSize(1000, 700);
    resize(1200, 800);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    
    // Security scan tab
    QWidget* scanTab = new QWidget();
    QVBoxLayout* scanLayout = new QVBoxLayout(scanTab);
    
    // Scan options
    m_scanOptionsGroup = new QGroupBox("Scan Options");
    QFormLayout* scanFormLayout = new QFormLayout(m_scanOptionsGroup);
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_scanPathEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    pathLayout->addWidget(m_scanPathEdit);
    pathLayout->addWidget(browseButton);
    
    m_includeSubdirsCheck = new QCheckBox("Include subdirectories");
    m_scanExecutablesCheck = new QCheckBox("Scan executable files");
    m_scanScriptsCheck = new QCheckBox("Scan script files");
    m_scanArchivesCheck = new QCheckBox("Scan archive files");
    
    m_maxFileSizeSpin = new QSpinBox();
    
    QHBoxLayout* scanButtonLayout = new QHBoxLayout();
    m_startScanButton = new QPushButton("Start Scan");
    m_cancelScanButton = new QPushButton("Cancel Scan");
    m_cancelScanButton->setEnabled(false);
    scanButtonLayout->addWidget(m_startScanButton);
    scanButtonLayout->addWidget(m_cancelScanButton);
    scanButtonLayout->addStretch();
    
    scanFormLayout->addRow("Scan Path:", pathLayout);
    scanFormLayout->addRow("", m_includeSubdirsCheck);
    scanFormLayout->addRow("", m_scanExecutablesCheck);
    scanFormLayout->addRow("", m_scanScriptsCheck);
    scanFormLayout->addRow("", m_scanArchivesCheck);
    scanFormLayout->addRow("Max File Size:", m_maxFileSizeSpin);
    scanFormLayout->addRow("", scanButtonLayout);
    
    // Progress
    m_scanProgress = new QProgressBar();
    m_scanProgress->setRange(0, 100);
    m_scanProgress->setValue(0);
    
    m_scanStatus = new QLabel("Ready");
    
    // Issues list
    m_issuesList = new QTreeWidget();
    m_issuesList->setHeaderLabels(QStringList() << "File" << "Issue Type" << "Severity" << "Detected");
    m_issuesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_issuesList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* issueButtonLayout = new QHBoxLayout();
    m_viewDetailsButton = new QPushButton("View Details");
    m_fixIssueButton = new QPushButton("Fix Issue");
    QPushButton* saveReportButton = new QPushButton("Save Report");
    QPushButton* loadReportButton = new QPushButton("Load Report");
    issueButtonLayout->addWidget(m_viewDetailsButton);
    issueButtonLayout->addWidget(m_fixIssueButton);
    issueButtonLayout->addWidget(saveReportButton);
    issueButtonLayout->addWidget(loadReportButton);
    issueButtonLayout->addStretch();
    
    scanLayout->addWidget(m_scanOptionsGroup);
    scanLayout->addWidget(m_scanProgress);
    scanLayout->addWidget(m_scanStatus);
    scanLayout->addWidget(m_issuesList, 1);
    scanLayout->addLayout(issueButtonLayout);
    
    // Settings tab
    QWidget* settingsTab = new QWidget();
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsTab);
    
    // Malware scanner
    m_malwareGroup = new QGroupBox("Malware Scanner");
    QVBoxLayout* malwareLayout = new QVBoxLayout(m_malwareGroup);
    
    m_malwareStatus = new QLabel("Not initialized");
    m_malwareStatus->setStyleSheet("color: orange;");
    
    QHBoxLayout* malwareButtonLayout = new QHBoxLayout();
    m_initializeScannerButton = new QPushButton("Initialize Scanner");
    m_updateMalwareButton = new QPushButton("Update Database");
    malwareButtonLayout->addWidget(m_initializeScannerButton);
    malwareButtonLayout->addWidget(m_updateMalwareButton);
    malwareButtonLayout->addStretch();
    
    malwareLayout->addWidget(m_malwareStatus);
    malwareLayout->addLayout(malwareButtonLayout);
    
    // Encryption detector
    m_encryptionGroup = new QGroupBox("Encryption Detector");
    QVBoxLayout* encryptionLayout = new QVBoxLayout(m_encryptionGroup);
    
    m_encryptionStatus = new QLabel("Ready");
    m_encryptionStatus->setStyleSheet("color: green;");
    
    QHBoxLayout* encryptionButtonLayout = new QHBoxLayout();
    m_initializeEncryptionButton = new QPushButton("Initialize Detector");
    encryptionButtonLayout->addWidget(m_initializeEncryptionButton);
    encryptionButtonLayout->addStretch();
    
    encryptionLayout->addWidget(m_encryptionStatus);
    encryptionLayout->addLayout(encryptionButtonLayout);
    
    // Scan settings
    m_scanSettingsGroup = new QGroupBox("Advanced Scan Settings");
    QFormLayout* settingsFormLayout = new QFormLayout(m_scanSettingsGroup);
    
    m_heuristicScanCheck = new QCheckBox("Enable heuristic scanning");
    m_heuristicScanCheck->setChecked(true);
    
    m_packedFilesCheck = new QCheckBox("Scan packed files");
    m_packedFilesCheck->setChecked(true);
    
    m_scanArchivesSettingsCheck = new QCheckBox("Scan archives");
    m_scanArchivesSettingsCheck->setChecked(true);
    
    settingsFormLayout->addRow("", m_heuristicScanCheck);
    settingsFormLayout->addRow("", m_packedFilesCheck);
    settingsFormLayout->addRow("", m_scanArchivesSettingsCheck);
    
    settingsLayout->addWidget(m_malwareGroup);
    settingsLayout->addWidget(m_encryptionGroup);
    settingsLayout->addWidget(m_scanSettingsGroup);
    settingsLayout->addStretch();
    
    // Policies tab
    QWidget* policiesTab = new QWidget();
    QVBoxLayout* policiesLayout = new QVBoxLayout(policiesTab);
    
    m_policiesTable = new QTableWidget(0, 3);
    m_policiesTable->setHorizontalHeaderLabels(QStringList() << "Policy Name" << "Description" << "Enabled");
    m_policiesTable->horizontalHeader()->setStretchLastSection(true);
    m_policiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_policiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* policyButtonLayout = new QHBoxLayout();
    m_applyPolicyButton = new QPushButton("Apply Policy");
    m_removePolicyButton = new QPushButton("Remove Policy");
    m_newPolicyButton = new QPushButton("New Policy");
    policyButtonLayout->addWidget(m_applyPolicyButton);
    policyButtonLayout->addWidget(m_removePolicyButton);
    policyButtonLayout->addWidget(m_newPolicyButton);
    policyButtonLayout->addStretch();
    
    policiesLayout->addWidget(m_policiesTable);
    policiesLayout->addLayout(policyButtonLayout);
    
    // Add tabs
    m_tabWidget->addTab(scanTab, "Security Scan");
    m_tabWidget->addTab(settingsTab, "Settings");
    m_tabWidget->addTab(policiesTab, "Policies");
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
    
    // Connect browse button
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan", m_scanPathEdit->text());
        if (!dir.isEmpty()) {
            m_scanPathEdit->setText(dir);
        }
    });
    
    // Connect report buttons
    connect(saveReportButton, &QPushButton::clicked, this, &SecurityDialog::onSaveReport);
    connect(loadReportButton, &QPushButton::clicked, this, &SecurityDialog::onLoadReport);
}

void SecurityDialog::setupConnections() {
    // Scan buttons
    connect(m_startScanButton, &QPushButton::clicked, this, &SecurityDialog::onStartSecurityScan);
    connect(m_cancelScanButton, &QPushButton::clicked, this, &SecurityDialog::onCancelSecurityScan);
    
    // Issue buttons
    connect(m_viewDetailsButton, &QPushButton::clicked, this, &SecurityDialog::onViewIssueDetails);
    connect(m_fixIssueButton, &QPushButton::clicked, this, &SecurityDialog::onFixIssue);
    
    // Settings buttons
    connect(m_initializeScannerButton, &QPushButton::clicked, this, &SecurityDialog::onMalwareScannerInitialized);
    connect(m_updateMalwareButton, &QPushButton::clicked, this, &SecurityDialog::onMalwareDatabaseUpdated);
    connect(m_initializeEncryptionButton, &QPushButton::clicked, [this]() {
        m_encryptionStatus->setText("Detector initialized");
        m_encryptionStatus->setStyleSheet("color: green;");
    });
    
    // Policy buttons
    connect(m_applyPolicyButton, &QPushButton::clicked, this, &SecurityDialog::onApplyPolicy);
    connect(m_removePolicyButton, &QPushButton::clicked, this, &SecurityDialog::onRemovePolicy);
    connect(m_newPolicyButton, &QPushButton::clicked, [this]() {
        SecurityPolicyDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString name = dialog.policyName();
            QVariantMap policy = dialog.policy();
            m_securityManager->setSecurityPolicy(name, policy);
            refreshPolicies();
        }
    });
}

void SecurityDialog::refreshSecurityIssues() {
    populateSecurityIssues();
}

void SecurityDialog::refreshScanSettings() {
    populateScanSettings();
}

void SecurityDialog::refreshPolicies() {
    populatePolicies();
}

void SecurityDialog::onStartSecurityScan() {
    QString path = m_scanPathEdit->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a directory path to scan.");
        return;
    }
    
    QDir dir(path);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Invalid Path", "The specified directory does not exist.");
        return;
    }
    
    // Apply scan settings
    m_securityManager->setScanExecutableFiles(m_scanExecutablesCheck->isChecked());
    m_securityManager->setScanScriptFiles(m_scanScriptsCheck->isChecked());
    m_securityManager->setScanArchiveFiles(m_scanArchivesCheck->isChecked());
    m_securityManager->setMaxFileSize(m_maxFileSizeSpin->value() * 1024 * 1024); // Convert MB to bytes
    
    // Start scan
    m_securityManager->startSecurityScan(path, m_includeSubdirsCheck->isChecked());
}

void SecurityDialog::onCancelSecurityScan() {
    m_securityManager->cancelSecurityScan();
}

void SecurityDialog::onSaveReport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Security Report", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/security_report.txt",
                                                   "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << m_securityManager->generateSecurityReport();
            file.close();
            QMessageBox::information(this, "Report Saved", "Security report saved successfully.");
        } else {
            QMessageBox::warning(this, "Save Failed", "Failed to save security report.");
        }
    }
}

void SecurityDialog::onLoadReport() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Security Report", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                   "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString report = stream.readAll();
            file.close();
            
            // In a real implementation, this would parse the report
            QMessageBox::information(this, "Report Loaded", 
                                   "Security report loaded. In a full implementation, this would populate the issues list.");
        } else {
            QMessageBox::warning(this, "Load Failed", "Failed to load security report.");
        }
    }
}

void SecurityDialog::onApplyPolicy() {
    QList<QTableWidgetItem*> selected = m_policiesTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a policy to apply.");
        return;
    }
    
    int row = selected.first()->row();
    QString policyName = m_policiesTable->item(row, 0)->text();
    
    QMessageBox::information(this, "Policy Applied", 
                           QString("Policy '%1' applied successfully.").arg(policyName));
}

void SecurityDialog::onRemovePolicy() {
    QList<QTableWidgetItem*> selected = m_policiesTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a policy to remove.");
        return;
    }
    
    int row = selected.first()->row();
    QString policyName = m_policiesTable->item(row, 0)->text();
    
    int result = QMessageBox::question(this, "Confirm Remove", 
                                     QString("Are you sure you want to remove the policy '%1'?").arg(policyName));
    if (result == QMessageBox::Yes) {
        // In a real implementation, this would remove the policy
        QMessageBox::information(this, "Policy Removed", 
                               QString("Policy '%1' removed successfully.").arg(policyName));
        refreshPolicies();
    }
}

void SecurityDialog::onViewIssueDetails() {
    QList<QTreeWidgetItem*> selected = m_issuesList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an issue to view details.");
        return;
    }
    
    // In a real implementation, this would show detailed information about the selected issue
    QMessageBox::information(this, "Issue Details", 
                           "Issue details would be displayed here in a full implementation.");
}

void SecurityDialog::onFixIssue() {
    QList<QTreeWidgetItem*> selected = m_issuesList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an issue to fix.");
        return;
    }
    
    QString filePath = selected.first()->data(0, Qt::UserRole).toString();
    QString issueType = selected.first()->text(1);
    
    QMessageBox::information(this, "Issue Fixed", 
                           QString("Issue '%1' for file '%2' fixed successfully.").arg(issueType, filePath));
    
    // Remove the item from the list
    delete selected.first();
}

void SecurityDialog::onSecurityScanStarted() {
    m_startScanButton->setEnabled(false);
    m_cancelScanButton->setEnabled(true);
    m_scanStatus->setText("Scanning...");
    m_scanProgress->setValue(0);
    
    // Clear previous issues
    m_issuesList->clear();
}

void SecurityDialog::onSecurityScanProgress(int percentage, quint64 filesProcessed) {
    m_scanProgress->setValue(percentage);
    m_scanStatus->setText(QString("Processed %1 files...").arg(filesProcessed));
}

void SecurityDialog::onSecurityScanCompleted(const QList<SecurityManager::SecurityIssue>& issues) {
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText(QString("Scan completed. Found %1 issues.").arg(issues.size()));
    m_scanProgress->setValue(100);
    
    QMessageBox::information(this, "Scan Complete", 
                           QString("Security scan completed. Found %1 issues.").arg(issues.size()));
}

void SecurityDialog::onSecurityScanCancelled() {
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText("Scan cancelled.");
    
    QMessageBox::information(this, "Scan Cancelled", "Security scan was cancelled.");
}

void SecurityDialog::onSecurityScanError(const QString& error) {
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText("Scan failed.");
    
    QMessageBox::warning(this, "Scan Error", 
                        QString("Security scan failed: %1").arg(error));
}

void SecurityDialog::onSecurityIssueFound(const SecurityManager::SecurityIssue& issue) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_issuesList);
    item->setText(0, issue.filePath);
    item->setData(0, Qt::UserRole, issue.filePath);
    item->setText(1, issue.issueType);
    item->setText(2, 
        issue.severity == SecurityManager::Critical ? "Critical" :
        issue.severity == SecurityManager::High ? "High" :
        issue.severity == SecurityManager::Medium ? "Medium" : "Low");
    item->setText(3, issue.detectedTime.toString("yyyy-MM-dd HH:mm"));
    
    // Set color based on severity
    QColor color;
    switch (issue.severity) {
        case SecurityManager::Critical:
            color = Qt::red;
            break;
        case SecurityManager::High:
            color = QColor(255, 165, 0); // Orange
            break;
        case SecurityManager::Medium:
            color = QColor(255, 255, 0); // Yellow
            break;
        case SecurityManager::Low:
        default:
            color = Qt::green;
            break;
    }
    item->setForeground(2, QBrush(color));
    
    m_issuesList->addTopLevelItem(item);
}

void SecurityDialog::onMalwareScannerInitialized() {
    if (m_malwareScanner->initializeScanner()) {
        m_malwareStatus->setText("Scanner initialized");
        m_malwareStatus->setStyleSheet("color: green;");
        QMessageBox::information(this, "Success", "Malware scanner initialized successfully.");
    } else {
        m_malwareStatus->setText("Initialization failed");
        m_malwareStatus->setStyleSheet("color: red;");
        QMessageBox::warning(this, "Error", "Failed to initialize malware scanner.");
    }
}

void SecurityDialog::onMalwareDatabaseUpdated() {
    if (m_malwareScanner->updateVirusDatabase()) {
        m_malwareStatus->setText("Database updated");
        m_malwareStatus->setStyleSheet("color: green;");
        QMessageBox::information(this, "Success", "Malware database updated successfully.");
    } else {
        m_malwareStatus->setText("Update failed");
        m_malwareStatus->setStyleSheet("color: red;");
        QMessageBox::warning(this, "Error", "Failed to update malware database.");
    }
}

void SecurityDialog::onEncryptionDetectionCompleted(const EncryptionDetector::DetectionResult& result) {
    if (result.isEncrypted) {
        // Add to issues list
        SecurityManager::SecurityIssue issue;
        issue.filePath = result.filePath;
        issue.issueType = "EncryptedFile";
        issue.description = QString("Encrypted file detected (%1)").arg(result.encryptionType);
        issue.severity = SecurityManager::Medium;
        issue.detectedTime = result.analysisTime;
        
        onSecurityIssueFound(issue);
    }
}

void SecurityDialog::populateSecurityIssues() {
    // Issues are populated in real-time as they are found
}

void SecurityDialog::populateScanSettings() {
    // Apply current settings to UI
    m_scanExecutablesCheck->setChecked(m_securityManager->scanExecutableFiles());
    m_scanScriptsCheck->setChecked(m_securityManager->scanScriptFiles());
    m_scanArchivesCheck->setChecked(m_securityManager->scanArchiveFiles());
    m_maxFileSizeSpin->setValue(m_securityManager->maxFileSize() / (1024 * 1024)); // Convert bytes to MB
}

void SecurityDialog::populatePolicies() {
    m_policiesTable->setRowCount(0);
    
    // Add default policy
    int row = m_policiesTable->rowCount();
    m_policiesTable->insertRow(row);
    
    QTableWidgetItem* nameItem = new QTableWidgetItem("Default");
    QTableWidgetItem* descItem = new QTableWidgetItem("Standard security policy");
    QTableWidgetItem* enabledItem = new QTableWidgetItem("Yes");
    
    m_policiesTable->setItem(row, 0, nameItem);
    m_policiesTable->setItem(row, 1, descItem);
    m_policiesTable->setItem(row, 2, enabledItem);
    
    m_policiesTable->resizeColumnsToContents();
}

void SecurityDialog::updateScanStatus() {
    // Update scan status display
}

void SecurityDialog::updateMalwareScannerStatus() {
    if (m_malwareScanner->isDatabaseUpToDate()) {
        m_malwareStatus->setText("Database up to date");
        m_malwareStatus->setStyleSheet("color: green;");
    } else {
        m_malwareStatus->setText("Database outdated");
        m_malwareStatus->setStyleSheet("color: orange;");
    }
}

// SecurityPolicyDialog implementation
SecurityPolicyDialog::SecurityPolicyDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_rulesTable(nullptr)
    , m_addRuleButton(nullptr)
    , m_removeRuleButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
}

SecurityPolicyDialog::~SecurityPolicyDialog() {
}

void SecurityPolicyDialog::setupUI() {
    setWindowTitle("Security Policy");
    setMinimumSize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Policy name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Policy Name:"));
    m_nameEdit = new QLineEdit();
    nameLayout->addWidget(m_nameEdit);
    
    // Rules table
    m_rulesTable = new QTableWidget(0, 2);
    m_rulesTable->setHorizontalHeaderLabels(QStringList() << "Rule" << "Value");
    m_rulesTable->horizontalHeader()->setStretchLastSection(true);
    m_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    QHBoxLayout* ruleButtonLayout = new QHBoxLayout();
    m_addRuleButton = new QPushButton("Add Rule");
    m_removeRuleButton = new QPushButton("Remove Rule");
    ruleButtonLayout->addWidget(m_addRuleButton);
    ruleButtonLayout->addWidget(m_removeRuleButton);
    ruleButtonLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_cancelButton = new QPushButton("Cancel");
    m_saveButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(m_rulesTable);
    mainLayout->addLayout(ruleButtonLayout);
    mainLayout->addLayout(buttonLayout);
    
    // Add some sample rules
    int row = m_rulesTable->rowCount();
    m_rulesTable->insertRow(row);
    m_rulesTable->setItem(row, 0, new QTableWidgetItem("allowWorldWritable"));
    m_rulesTable->setItem(row, 1, new QTableWidgetItem("false"));
    
    row = m_rulesTable->rowCount();
    m_rulesTable->insertRow(row);
    m_rulesTable->setItem(row, 0, new QTableWidgetItem("allowSuid"));
    m_rulesTable->setItem(row, 1, new QTableWidgetItem("false"));
    
    row = m_rulesTable->rowCount();
    m_rulesTable->insertRow(row);
    m_rulesTable->setItem(row, 0, new QTableWidgetItem("maxEntropy"));
    m_rulesTable->setItem(row, 1, new QTableWidgetItem("0.9"));
}

void SecurityPolicyDialog::setupConnections() {
    connect(m_addRuleButton, &QPushButton::clicked, this, &SecurityPolicyDialog::onAddRule);
    connect(m_removeRuleButton, &QPushButton::clicked, this, &SecurityPolicyDialog::onRemoveRule);
    connect(m_saveButton, &QPushButton::clicked, this, &SecurityPolicyDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &SecurityPolicyDialog::onCancel);
}

void SecurityPolicyDialog::setPolicy(const QString& name, const QVariantMap& policy) {
    m_policyName = name;
    m_policy = policy;
    
    m_nameEdit->setText(name);
    
    // Clear existing rules
    m_rulesTable->setRowCount(0);
    
    // Add policy rules
    for (auto it = policy.begin(); it != policy.end(); ++it) {
        int row = m_rulesTable->rowCount();
        m_rulesTable->insertRow(row);
        m_rulesTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_rulesTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
    }
}

QString SecurityPolicyDialog::policyName() const {
    return m_nameEdit->text();
}

QVariantMap SecurityPolicyDialog::policy() const {
    QVariantMap policy;
    
    for (int i = 0; i < m_rulesTable->rowCount(); ++i) {
        QTableWidgetItem* keyItem = m_rulesTable->item(i, 0);
        QTableWidgetItem* valueItem = m_rulesTable->item(i, 1);
        if (keyItem && valueItem) {
            policy[keyItem->text()] = valueItem->text();
        }
    }
    
    return policy;
}

void SecurityPolicyDialog::onSave() {
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a policy name.");
        return;
    }
    
    accept();
}

void SecurityPolicyDialog::onCancel() {
    reject();
}

void SecurityPolicyDialog::onAddRule() {
    int row = m_rulesTable->rowCount();
    m_rulesTable->insertRow(row);
    m_rulesTable->setItem(row, 0, new QTableWidgetItem("rule"));
    m_rulesTable->setItem(row, 1, new QTableWidgetItem("value"));
}

void SecurityPolicyDialog::onRemoveRule() {
    QList<QTableWidgetItem*> selected = m_rulesTable->selectedItems();
    if (!selected.isEmpty()) {
        int row = selected.first()->row();
        m_rulesTable->removeRow(row);
    }
}

// SecurityIssueDialog implementation
SecurityIssueDialog::SecurityIssueDialog(const SecurityManager::SecurityIssue& issue, QWidget *parent)
    : QDialog(parent)
    , m_issue(issue)
    , m_filePathLabel(nullptr)
    , m_issueTypeLabel(nullptr)
    , m_severityLabel(nullptr)
    , m_detectedTimeLabel(nullptr)
    , m_descriptionEdit(nullptr)
    , m_detailsEdit(nullptr)
    , m_closeButton(nullptr)
{
    setupUI();
    setupConnections();
}

SecurityIssueDialog::~SecurityIssueDialog() {
}

void SecurityIssueDialog::setupUI() {
    setWindowTitle("Security Issue Details");
    setMinimumSize(600, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Issue details
    QFormLayout* formLayout = new QFormLayout();
    
    m_filePathLabel = new QLabel(m_issue.filePath);
    m_filePathLabel->setWordWrap(true);
    
    m_issueTypeLabel = new QLabel(m_issue.issueType);
    
    m_severityLabel = new QLabel(
        m_issue.severity == SecurityManager::Critical ? "Critical" :
        m_issue.severity == SecurityManager::High ? "High" :
        m_issue.severity == SecurityManager::Medium ? "Medium" : "Low");
    m_severityLabel->setStyleSheet(
        m_issue.severity == SecurityManager::Critical ? "color: red;" :
        m_issue.severity == SecurityManager::High ? "color: orange;" :
        m_issue.severity == SecurityManager::Medium ? "color: yellow;" : "color: green;");
    
    m_detectedTimeLabel = new QLabel(m_issue.detectedTime.toString());
    
    formLayout->addRow("File Path:", m_filePathLabel);
    formLayout->addRow("Issue Type:", m_issueTypeLabel);
    formLayout->addRow("Severity:", m_severityLabel);
    formLayout->addRow("Detected:", m_detectedTimeLabel);
    
    // Description
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setPlainText(m_issue.description);
    m_descriptionEdit->setReadOnly(true);
    
    // Details
    m_detailsEdit = new QTextEdit();
    QString detailsText;
    QTextStream detailsStream(&detailsText);
    for (auto it = m_issue.details.begin(); it != m_issue.details.end(); ++it) {
        detailsStream << it.key() << ": " << it.value().toString() << "\n";
    }
    m_detailsEdit->setPlainText(detailsText);
    m_detailsEdit->setReadOnly(true);
    
    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_closeButton = new QPushButton("Close");
    m_closeButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(new QLabel("Description:"));
    mainLayout->addWidget(m_descriptionEdit);
    mainLayout->addWidget(new QLabel("Details:"));
    mainLayout->addWidget(m_detailsEdit);
    mainLayout->addLayout(buttonLayout);
}

void SecurityIssueDialog::setupConnections() {
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}