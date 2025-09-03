#include "networkdialog.h"
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

// NetworkDialog implementation
NetworkDialog::NetworkDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_serverGroup(nullptr)
    , m_serverEdit(nullptr)
    , m_portSpin(nullptr)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_connectButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_connectionStatus(nullptr)
    , m_networkDrivesList(nullptr)
    , m_mountDriveButton(nullptr)
    , m_unmountDriveButton(nullptr)
    , m_cloudProviderCombo(nullptr)
    , m_cloudAccessTokenEdit(nullptr)
    , m_cloudClientIdEdit(nullptr)
    , m_cloudClientSecretEdit(nullptr)
    , m_connectCloudButton(nullptr)
    , m_disconnectCloudButton(nullptr)
    , m_cloudStatus(nullptr)
    , m_remoteFilesList(nullptr)
    , m_uploadButton(nullptr)
    , m_downloadButton(nullptr)
    , m_deleteRemoteButton(nullptr)
    , m_scanOptionsGroup(nullptr)
    , m_remotePathEdit(nullptr)
    , m_includeSubdirsCheck(nullptr)
    , m_maxDepthSpin(nullptr)
    , m_startScanButton(nullptr)
    , m_cancelScanButton(nullptr)
    , m_scanProgress(nullptr)
    , m_scanStatus(nullptr)
    , m_uploadSpeedLabel(nullptr)
    , m_downloadSpeedLabel(nullptr)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    
    // Initialize core components
    m_networkManager = std::make_unique<NetworkManager>();
    m_remoteScanner = std::make_unique<RemoteScanner>(m_networkManager.get());
    m_cloudManager = std::make_unique<CloudStorageManager>(m_networkManager.get());
    
    // Connect network manager signals
    connect(m_networkManager.get(), &NetworkManager::connectionStatusChanged,
            this, &NetworkDialog::onConnectionStatusChanged);
    connect(m_networkManager.get(), &NetworkManager::networkDriveMounted,
            this, &NetworkDialog::onNetworkDriveMounted);
    connect(m_networkManager.get(), &NetworkManager::networkDriveUnmounted,
            this, &NetworkDialog::onNetworkDriveUnmounted);
    connect(m_networkManager.get(), &NetworkManager::cloudStorageConnected,
            this, &NetworkDialog::onCloudStorageConnected);
    connect(m_networkManager.get(), &NetworkManager::cloudStorageDisconnected,
            this, &NetworkDialog::onCloudStorageDisconnected);
    connect(m_networkManager.get(), &NetworkManager::fileUploaded,
            this, &NetworkDialog::onFileUploaded);
    connect(m_networkManager.get(), &NetworkManager::fileDownloaded,
            this, &NetworkDialog::onFileDownloaded);
    connect(m_networkManager.get(), &NetworkManager::fileUploadFailed,
            this, &NetworkDialog::onFileUploadFailed);
    connect(m_networkManager.get(), &NetworkManager::fileDownloadFailed,
            this, &NetworkDialog::onFileDownloadFailed);
    connect(m_networkManager.get(), &NetworkManager::bandwidthUpdated,
            this, &NetworkDialog::onBandwidthUpdated);
    
    // Connect remote scanner signals
    connect(m_remoteScanner.get(), &RemoteScanner::scanStarted,
            this, &NetworkDialog::onRemoteScanStarted);
    connect(m_remoteScanner.get(), &RemoteScanner::scanProgress,
            this, &NetworkDialog::onRemoteScanProgress);
    connect(m_remoteScanner.get(), &RemoteScanner::scanCompleted,
            this, &NetworkDialog::onRemoteScanCompleted);
    connect(m_remoteScanner.get(), &RemoteScanner::scanCancelled,
            this, &NetworkDialog::onRemoteScanCancelled);
    connect(m_remoteScanner.get(), &RemoteScanner::scanError,
            this, &NetworkDialog::onRemoteScanError);
    
    // Set up refresh timer
    connect(m_refreshTimer, &QTimer::timeout, this, &NetworkDialog::refreshConnections);
    m_refreshTimer->setInterval(30000); // Refresh every 30 seconds
    
    // Set default values
    m_portSpin->setValue(445); // Default SMB port
    m_maxDepthSpin->setValue(-1); // Unlimited depth
    m_includeSubdirsCheck->setChecked(true);
    
    // Load initial data
    refreshConnections();
    refreshCloudStorage();
    refreshRemoteFiles();
    
    // Update connection status
    updateConnectionStatus();
    updateBandwidthDisplay();
}

NetworkDialog::~NetworkDialog() {
}

void NetworkDialog::setupUI() {
    setWindowTitle("Network Manager");
    setMinimumSize(900, 700);
    resize(1100, 800);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    
    // Connections tab
    QWidget* connectionsTab = new QWidget();
    QVBoxLayout* connectionsLayout = new QVBoxLayout(connectionsTab);
    
    // Server connection
    m_serverGroup = new QGroupBox("Server Connection");
    QFormLayout* serverLayout = new QFormLayout(m_serverGroup);
    
    m_serverEdit = new QLineEdit();
    m_serverEdit->setPlaceholderText("e.g., 192.168.1.100 or server.example.com");
    
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(445);
    
    m_usernameEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    
    QHBoxLayout* serverButtonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton("Connect");
    m_disconnectButton = new QPushButton("Disconnect");
    serverButtonLayout->addWidget(m_connectButton);
    serverButtonLayout->addWidget(m_disconnectButton);
    serverButtonLayout->addStretch();
    
    m_connectionStatus = new QLabel("Disconnected");
    m_connectionStatus->setStyleSheet("color: red;");
    
    serverLayout->addRow("Server:", m_serverEdit);
    serverLayout->addRow("Port:", m_portSpin);
    serverLayout->addRow("Username:", m_usernameEdit);
    serverLayout->addRow("Password:", m_passwordEdit);
    serverLayout->addRow("", serverButtonLayout);
    serverLayout->addRow("Status:", m_connectionStatus);
    
    // Network drives
    QGroupBox* drivesGroup = new QGroupBox("Network Drives");
    QVBoxLayout* drivesLayout = new QVBoxLayout(drivesGroup);
    
    m_networkDrivesList = new QTreeWidget();
    m_networkDrivesList->setHeaderLabels(QStringList() << "Name" << "Server" << "Path" << "Status");
    m_networkDrivesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_networkDrivesList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* driveButtonLayout = new QHBoxLayout();
    m_mountDriveButton = new QPushButton("Mount Drive");
    m_unmountDriveButton = new QPushButton("Unmount Drive");
    driveButtonLayout->addWidget(m_mountDriveButton);
    driveButtonLayout->addWidget(m_unmountDriveButton);
    driveButtonLayout->addStretch();
    
    drivesLayout->addWidget(m_networkDrivesList);
    drivesLayout->addLayout(driveButtonLayout);
    
    // Bandwidth monitoring
    QGroupBox* bandwidthGroup = new QGroupBox("Bandwidth");
    QHBoxLayout* bandwidthLayout = new QHBoxLayout(bandwidthGroup);
    
    m_uploadSpeedLabel = new QLabel("Upload: 0 KB/s");
    m_downloadSpeedLabel = new QLabel("Download: 0 KB/s");
    bandwidthLayout->addWidget(m_uploadSpeedLabel);
    bandwidthLayout->addWidget(m_downloadSpeedLabel);
    bandwidthLayout->addStretch();
    
    connectionsLayout->addWidget(m_serverGroup);
    connectionsLayout->addWidget(drivesGroup);
    connectionsLayout->addWidget(bandwidthGroup);
    
    // Cloud storage tab
    QWidget* cloudTab = new QWidget();
    QVBoxLayout* cloudLayout = new QVBoxLayout(cloudTab);
    
    QGroupBox* cloudGroup = new QGroupBox("Cloud Storage");
    QFormLayout* cloudFormLayout = new QFormLayout(cloudGroup);
    
    m_cloudProviderCombo = new QComboBox();
    m_cloudProviderCombo->addItem("Dropbox");
    m_cloudProviderCombo->addItem("Google Drive");
    m_cloudProviderCombo->addItem("OneDrive");
    m_cloudProviderCombo->addItem("Custom");
    
    m_cloudAccessTokenEdit = new QLineEdit();
    m_cloudAccessTokenEdit->setEchoMode(QLineEdit::Password);
    
    m_cloudClientIdEdit = new QLineEdit();
    m_cloudClientSecretEdit = new QLineEdit();
    m_cloudClientSecretEdit->setEchoMode(QLineEdit::Password);
    
    QHBoxLayout* cloudButtonLayout = new QHBoxLayout();
    m_connectCloudButton = new QPushButton("Connect");
    m_disconnectCloudButton = new QPushButton("Disconnect");
    cloudButtonLayout->addWidget(m_connectCloudButton);
    cloudButtonLayout->addWidget(m_disconnectCloudButton);
    cloudButtonLayout->addStretch();
    
    m_cloudStatus = new QLabel("Disconnected");
    m_cloudStatus->setStyleSheet("color: red;");
    
    cloudFormLayout->addRow("Provider:", m_cloudProviderCombo);
    cloudFormLayout->addRow("Access Token:", m_cloudAccessTokenEdit);
    cloudFormLayout->addRow("Client ID:", m_cloudClientIdEdit);
    cloudFormLayout->addRow("Client Secret:", m_cloudClientSecretEdit);
    cloudFormLayout->addRow("", cloudButtonLayout);
    cloudFormLayout->addRow("Status:", m_cloudStatus);
    
    cloudLayout->addWidget(cloudGroup);
    cloudLayout->addStretch();
    
    // Remote files tab
    QWidget* filesTab = new QWidget();
    QVBoxLayout* filesLayout = new QVBoxLayout(filesTab);
    
    m_remoteFilesList = new QTreeWidget();
    m_remoteFilesList->setHeaderLabels(QStringList() << "Name" << "Size" << "Modified" << "Type");
    m_remoteFilesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_remoteFilesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    QHBoxLayout* fileButtonLayout = new QHBoxLayout();
    m_uploadButton = new QPushButton("Upload File");
    m_downloadButton = new QPushButton("Download File");
    m_deleteRemoteButton = new QPushButton("Delete");
    fileButtonLayout->addWidget(m_uploadButton);
    fileButtonLayout->addWidget(m_downloadButton);
    fileButtonLayout->addWidget(m_deleteRemoteButton);
    fileButtonLayout->addStretch();
    
    filesLayout->addWidget(m_remoteFilesList);
    filesLayout->addLayout(fileButtonLayout);
    
    // Remote scanning tab
    QWidget* scanTab = new QWidget();
    QVBoxLayout* scanLayout = new QVBoxLayout(scanTab);
    
    m_scanOptionsGroup = new QGroupBox("Scan Options");
    QFormLayout* scanFormLayout = new QFormLayout(m_scanOptionsGroup);
    
    m_remotePathEdit = new QLineEdit();
    m_remotePathEdit->setPlaceholderText("e.g., /shared/folder");
    
    m_includeSubdirsCheck = new QCheckBox("Include subdirectories");
    m_includeSubdirsCheck->setChecked(true);
    
    m_maxDepthSpin = new QSpinBox();
    m_maxDepthSpin->setRange(-1, 100);
    m_maxDepthSpin->setValue(-1);
    m_maxDepthSpin->setSpecialValueText("Unlimited");
    
    QHBoxLayout* scanButtonLayout = new QHBoxLayout();
    m_startScanButton = new QPushButton("Start Scan");
    m_cancelScanButton = new QPushButton("Cancel Scan");
    m_cancelScanButton->setEnabled(false);
    scanButtonLayout->addWidget(m_startScanButton);
    scanButtonLayout->addWidget(m_cancelScanButton);
    scanButtonLayout->addStretch();
    
    scanFormLayout->addRow("Remote Path:", m_remotePathEdit);
    scanFormLayout->addRow("", m_includeSubdirsCheck);
    scanFormLayout->addRow("Max Depth:", m_maxDepthSpin);
    scanFormLayout->addRow("", scanButtonLayout);
    
    m_scanProgress = new QProgressBar();
    m_scanProgress->setRange(0, 100);
    m_scanProgress->setValue(0);
    
    m_scanStatus = new QLabel("Ready");
    
    scanLayout->addWidget(m_scanOptionsGroup);
    scanLayout->addWidget(m_scanProgress);
    scanLayout->addWidget(m_scanStatus);
    scanLayout->addStretch();
    
    // Add tabs
    m_tabWidget->addTab(connectionsTab, "Connections");
    m_tabWidget->addTab(cloudTab, "Cloud Storage");
    m_tabWidget->addTab(filesTab, "Remote Files");
    m_tabWidget->addTab(scanTab, "Remote Scanning");
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
}

void NetworkDialog::setupConnections() {
    // Connection buttons
    connect(m_connectButton, &QPushButton::clicked, this, &NetworkDialog::onConnectToServer);
    connect(m_disconnectButton, &QPushButton::clicked, this, &NetworkDialog::onDisconnectFromServer);
    connect(m_mountDriveButton, &QPushButton::clicked, this, &NetworkDialog::onMountNetworkDrive);
    connect(m_unmountDriveButton, &QPushButton::clicked, this, &NetworkDialog::onUnmountNetworkDrive);
    
    // Cloud storage buttons
    connect(m_connectCloudButton, &QPushButton::clicked, this, &NetworkDialog::onConnectToCloudStorage);
    connect(m_disconnectCloudButton, &QPushButton::clicked, this, &NetworkDialog::onDisconnectFromCloudStorage);
    
    // File buttons
    connect(m_uploadButton, &QPushButton::clicked, this, &NetworkDialog::onUploadFile);
    connect(m_downloadButton, &QPushButton::clicked, this, &NetworkDialog::onDownloadFile);
    connect(m_deleteRemoteButton, &QPushButton::clicked, this, &NetworkDialog::onDeleteRemoteFile);
    
    // Scan buttons
    connect(m_startScanButton, &QPushButton::clicked, this, &NetworkDialog::onStartRemoteScan);
    connect(m_cancelScanButton, &QPushButton::clicked, this, &NetworkDialog::onCancelRemoteScan);
}

void NetworkDialog::refreshConnections() {
    populateConnectionList();
}

void NetworkDialog::refreshCloudStorage() {
    populateCloudStorageList();
}

void NetworkDialog::refreshRemoteFiles() {
    populateRemoteFileList();
}

void NetworkDialog::onConnectToServer() {
    QString server = m_serverEdit->text();
    int port = m_portSpin->value();
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();
    
    if (server.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a server address.");
        return;
    }
    
    m_networkManager->connectToServer(server, port, username, password);
}

void NetworkDialog::onDisconnectFromServer() {
    m_networkManager->disconnectFromServer();
}

void NetworkDialog::onMountNetworkDrive() {
    NetworkDriveDialog editor(this);
    if (editor.exec() == QDialog::Accepted) {
        NetworkManager::NetworkDrive drive = editor.drive();
        if (drive.name.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Please enter a drive name.");
            return;
        }
        
        if (m_networkManager->mountNetworkDrive(drive)) {
            QMessageBox::information(this, "Success", "Network drive mounted successfully.");
            refreshConnections();
        } else {
            QMessageBox::warning(this, "Error", "Failed to mount network drive.");
        }
    }
}

void NetworkDialog::onUnmountNetworkDrive() {
    QList<QTreeWidgetItem*> selected = m_networkDrivesList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a network drive to unmount.");
        return;
    }
    
    QString driveId = selected.first()->data(0, Qt::UserRole).toString();
    QString driveName = selected.first()->text(0);
    
    int result = QMessageBox::question(this, "Confirm Unmount", 
                                     QString("Are you sure you want to unmount the drive '%1'?").arg(driveName));
    if (result == QMessageBox::Yes) {
        if (m_networkManager->unmountNetworkDrive(driveId)) {
            QMessageBox::information(this, "Success", "Network drive unmounted successfully.");
            refreshConnections();
        } else {
            QMessageBox::warning(this, "Error", "Failed to unmount network drive.");
        }
    }
}

void NetworkDialog::onConnectToCloudStorage() {
    NetworkManager::CloudStorageConfig config;
    config.provider = static_cast<NetworkManager::CloudProvider>(m_cloudProviderCombo->currentIndex());
    config.accessToken = m_cloudAccessTokenEdit->text();
    config.clientId = m_cloudClientIdEdit->text();
    config.clientSecret = m_cloudClientSecretEdit->text();
    config.enabled = true;
    
    m_networkManager->setCloudStorageConfig(config);
    
    if (m_networkManager->connectToCloudStorage()) {
        QMessageBox::information(this, "Success", "Connected to cloud storage.");
        refreshCloudStorage();
    } else {
        QMessageBox::warning(this, "Error", "Failed to connect to cloud storage.");
    }
}

void NetworkDialog::onDisconnectFromCloudStorage() {
    if (m_networkManager->disconnectFromCloudStorage()) {
        QMessageBox::information(this, "Success", "Disconnected from cloud storage.");
        refreshCloudStorage();
    } else {
        QMessageBox::warning(this, "Error", "Failed to disconnect from cloud storage.");
    }
}

void NetworkDialog::onStartRemoteScan() {
    if (m_networkManager->connectionStatus() != NetworkManager::Connected) {
        QMessageBox::warning(this, "Not Connected", "Please connect to a server first.");
        return;
    }
    
    QString remotePath = m_remotePathEdit->text();
    if (remotePath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a remote path.");
        return;
    }
    
    RemoteScanner::ScanOptions options;
    options.remotePath = remotePath;
    options.includeSubdirs = m_includeSubdirsCheck->isChecked();
    options.maxDepth = m_maxDepthSpin->value();
    
    m_remoteScanner->startScan(options);
}

void NetworkDialog::onCancelRemoteScan() {
    m_remoteScanner->cancelScan();
}

void NetworkDialog::onUploadFile() {
    if (!m_networkManager->isCloudStorageConnected()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to cloud storage first.");
        return;
    }
    
    QString localPath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (localPath.isEmpty()) {
        return;
    }
    
    QString remotePath = QInputDialog::getText(this, "Upload File", "Enter remote path:");
    if (remotePath.isEmpty()) {
        return;
    }
    
    QProgressDialog progress("Uploading file...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    if (m_networkManager->uploadFile(localPath, remotePath)) {
        progress.setValue(100);
        QMessageBox::information(this, "Success", "File uploaded successfully.");
        refreshRemoteFiles();
    } else {
        QMessageBox::warning(this, "Error", "Failed to upload file.");
    }
}

void NetworkDialog::onDownloadFile() {
    if (!m_networkManager->isCloudStorageConnected()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to cloud storage first.");
        return;
    }
    
    QList<QTreeWidgetItem*> selected = m_remoteFilesList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a file to download.");
        return;
    }
    
    QString remotePath = selected.first()->data(0, Qt::UserRole).toString();
    QString fileName = selected.first()->text(0);
    
    QString localPath = QFileDialog::getSaveFileName(this, "Save File As", fileName);
    if (localPath.isEmpty()) {
        return;
    }
    
    QProgressDialog progress("Downloading file...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    if (m_networkManager->downloadFile(remotePath, localPath)) {
        progress.setValue(100);
        QMessageBox::information(this, "Success", "File downloaded successfully.");
    } else {
        QMessageBox::warning(this, "Error", "Failed to download file.");
    }
}

void NetworkDialog::onDeleteRemoteFile() {
    if (!m_networkManager->isCloudStorageConnected()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to cloud storage first.");
        return;
    }
    
    QList<QTreeWidgetItem*> selected = m_remoteFilesList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a file to delete.");
        return;
    }
    
    QString remotePath = selected.first()->data(0, Qt::UserRole).toString();
    QString fileName = selected.first()->text(0);
    
    int result = QMessageBox::question(this, "Confirm Delete", 
                                     QString("Are you sure you want to delete the file '%1'?").arg(fileName));
    if (result == QMessageBox::Yes) {
        if (m_networkManager->deleteRemoteFile(remotePath)) {
            QMessageBox::information(this, "Success", "File deleted successfully.");
            refreshRemoteFiles();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete file.");
        }
    }
}

void NetworkDialog::onConnectionStatusChanged(NetworkManager::ConnectionStatus status) {
    updateConnectionStatus();
    
    switch (status) {
        case NetworkManager::Connected:
            QMessageBox::information(this, "Connected", "Successfully connected to server.");
            break;
        case NetworkManager::Error:
            QMessageBox::warning(this, "Connection Error", 
                               m_networkManager->connectionErrorMessage());
            break;
        default:
            break;
    }
}

void NetworkDialog::onNetworkDriveMounted(const QString& driveId) {
    Q_UNUSED(driveId);
    refreshConnections();
    QMessageBox::information(this, "Success", "Network drive mounted successfully.");
}

void NetworkDialog::onNetworkDriveUnmounted(const QString& driveId) {
    Q_UNUSED(driveId);
    refreshConnections();
    QMessageBox::information(this, "Success", "Network drive unmounted successfully.");
}

void NetworkDialog::onCloudStorageConnected() {
    refreshCloudStorage();
    QMessageBox::information(this, "Success", "Connected to cloud storage.");
}

void NetworkDialog::onCloudStorageDisconnected() {
    refreshCloudStorage();
    QMessageBox::information(this, "Success", "Disconnected from cloud storage.");
}

void NetworkDialog::onFileUploaded(const QString& localPath, const QString& remotePath) {
    Q_UNUSED(localPath);
    Q_UNUSED(remotePath);
    refreshRemoteFiles();
}

void NetworkDialog::onFileDownloaded(const QString& remotePath, const QString& localPath) {
    Q_UNUSED(remotePath);
    Q_UNUSED(localPath);
    QMessageBox::information(this, "Success", "File downloaded successfully.");
}

void NetworkDialog::onFileUploadFailed(const QString& localPath, const QString& error) {
    Q_UNUSED(localPath);
    QMessageBox::warning(this, "Upload Failed", 
                        QString("Failed to upload file: %1").arg(error));
}

void NetworkDialog::onFileDownloadFailed(const QString& remotePath, const QString& error) {
    Q_UNUSED(remotePath);
    QMessageBox::warning(this, "Download Failed", 
                        QString("Failed to download file: %1").arg(error));
}

void NetworkDialog::onBandwidthUpdated(quint64 uploadSpeed, quint64 downloadSpeed) {
    Q_UNUSED(uploadSpeed);
    Q_UNUSED(downloadSpeed);
    updateBandwidthDisplay();
}

void NetworkDialog::onRemoteScanStarted() {
    m_startScanButton->setEnabled(false);
    m_cancelScanButton->setEnabled(true);
    m_scanStatus->setText("Scanning...");
    m_scanProgress->setValue(0);
}

void NetworkDialog::onRemoteScanProgress(int percentage, quint64 filesProcessed) {
    m_scanProgress->setValue(percentage);
    m_scanStatus->setText(QString("Processed %1 files...").arg(filesProcessed));
}

void NetworkDialog::onRemoteScanCompleted(const QList<RemoteScanner::RemoteFileEntry>& results) {
    Q_UNUSED(results);
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText("Scan completed.");
    m_scanProgress->setValue(100);
    
    QMessageBox::information(this, "Scan Complete", 
                           QString("Remote scan completed. Found %1 files.").arg(results.size()));
}

void NetworkDialog::onRemoteScanCancelled() {
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText("Scan cancelled.");
    
    QMessageBox::information(this, "Scan Cancelled", "Remote scan was cancelled.");
}

void NetworkDialog::onRemoteScanError(const QString& error) {
    m_startScanButton->setEnabled(true);
    m_cancelScanButton->setEnabled(false);
    m_scanStatus->setText("Scan failed.");
    
    QMessageBox::warning(this, "Scan Error", 
                        QString("Remote scan failed: %1").arg(error));
}

void NetworkDialog::populateConnectionList() {
    m_networkDrivesList->clear();
    
    QList<NetworkManager::NetworkDrive> drives = m_networkManager->getNetworkDrives();
    for (const NetworkManager::NetworkDrive& drive : drives) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_networkDrivesList);
        item->setText(0, drive.name);
        item->setData(0, Qt::UserRole, drive.id);
        item->setText(1, drive.server);
        item->setText(2, drive.path);
        item->setText(3, drive.connected ? "Connected" : "Disconnected");
        
        if (drive.connected) {
            item->setForeground(3, QBrush(Qt::darkGreen));
        } else {
            item->setForeground(3, QBrush(Qt::red));
        }
    }
    
    m_networkDrivesList->resizeColumnToContents(0);
    m_networkDrivesList->resizeColumnToContents(1);
    m_networkDrivesList->resizeColumnToContents(3);
}

void NetworkDialog::populateCloudStorageList() {
    NetworkManager::CloudStorageConfig config = m_networkManager->cloudStorageConfig();
    m_cloudProviderCombo->setCurrentIndex(static_cast<int>(config.provider));
    m_cloudAccessTokenEdit->setText(config.accessToken);
    m_cloudClientIdEdit->setText(config.clientId);
    m_cloudClientSecretEdit->setText(config.clientSecret);
    
    bool connected = m_networkManager->isCloudStorageConnected();
    m_cloudStatus->setText(connected ? "Connected" : "Disconnected");
    m_cloudStatus->setStyleSheet(connected ? "color: green;" : "color: red;");
    
    m_connectCloudButton->setEnabled(!connected);
    m_disconnectCloudButton->setEnabled(connected);
}

void NetworkDialog::populateRemoteFileList() {
    m_remoteFilesList->clear();
    
    // In a real implementation, this would populate with actual remote files
    // For now, we'll add some sample data
    
    QStringList sampleFiles = {"document.pdf", "image.jpg", "data.xlsx", "report.docx"};
    QStringList sampleSizes = {"1.2 MB", "3.4 MB", "567 KB", "2.1 MB"};
    QStringList sampleDates = {"2023-05-15", "2023-05-16", "2023-05-17", "2023-05-18"};
    QStringList sampleTypes = {"PDF", "Image", "Spreadsheet", "Document"};
    
    for (int i = 0; i < sampleFiles.size(); ++i) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_remoteFilesList);
        item->setText(0, sampleFiles[i]);
        item->setData(0, Qt::UserRole, "/remote/" + sampleFiles[i]);
        item->setText(1, sampleSizes[i]);
        item->setText(2, sampleDates[i]);
        item->setText(3, sampleTypes[i]);
    }
    
    m_remoteFilesList->resizeColumnToContents(0);
    m_remoteFilesList->resizeColumnToContents(1);
    m_remoteFilesList->resizeColumnToContents(2);
}

void NetworkDialog::updateConnectionStatus() {
    NetworkManager::ConnectionStatus status = m_networkManager->connectionStatus();
    
    switch (status) {
        case NetworkManager::Disconnected:
            m_connectionStatus->setText("Disconnected");
            m_connectionStatus->setStyleSheet("color: red;");
            m_connectButton->setEnabled(true);
            m_disconnectButton->setEnabled(false);
            break;
        case NetworkManager::Connecting:
            m_connectionStatus->setText("Connecting...");
            m_connectionStatus->setStyleSheet("color: orange;");
            m_connectButton->setEnabled(false);
            m_disconnectButton->setEnabled(true);
            break;
        case NetworkManager::Connected:
            m_connectionStatus->setText("Connected");
            m_connectionStatus->setStyleSheet("color: green;");
            m_connectButton->setEnabled(false);
            m_disconnectButton->setEnabled(true);
            break;
        case NetworkManager::Error:
            m_connectionStatus->setText("Error: " + m_networkManager->connectionErrorMessage());
            m_connectionStatus->setStyleSheet("color: red;");
            m_connectButton->setEnabled(true);
            m_disconnectButton->setEnabled(false);
            break;
    }
}

void NetworkDialog::updateBandwidthDisplay() {
    quint64 uploadSpeed = m_networkManager->uploadSpeed();
    quint64 downloadSpeed = m_networkManager->downloadSpeed();
    
    // Format speeds
    QString uploadText, downloadText;
    
    if (uploadSpeed < 1024) {
        uploadText = QString("%1 B/s").arg(uploadSpeed);
    } else if (uploadSpeed < 1024 * 1024) {
        uploadText = QString("%1 KB/s").arg(uploadSpeed / 1024);
    } else {
        uploadText = QString("%1 MB/s").arg(uploadSpeed / (1024 * 1024));
    }
    
    if (downloadSpeed < 1024) {
        downloadText = QString("%1 B/s").arg(downloadSpeed);
    } else if (downloadSpeed < 1024 * 1024) {
        downloadText = QString("%1 KB/s").arg(downloadSpeed / 1024);
    } else {
        downloadText = QString("%1 MB/s").arg(downloadSpeed / (1024 * 1024));
    }
    
    m_uploadSpeedLabel->setText("Upload: " + uploadText);
    m_downloadSpeedLabel->setText("Download: " + downloadText);
}

// NetworkDriveDialog implementation
NetworkDriveDialog::NetworkDriveDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_serverEdit(nullptr)
    , m_portSpin(nullptr)
    , m_shareEdit(nullptr)
    , m_mountPointEdit(nullptr)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
    
    // Set default values
    m_portSpin->setValue(445); // Default SMB port
}

NetworkDriveDialog::~NetworkDriveDialog() {
}

void NetworkDriveDialog::setupUI() {
    setWindowTitle("Network Drive");
    setMinimumSize(400, 300);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* driveGroup = new QGroupBox("Drive Settings");
    QFormLayout* driveLayout = new QFormLayout(driveGroup);
    
    m_nameEdit = new QLineEdit();
    m_serverEdit = new QLineEdit();
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_shareEdit = new QLineEdit();
    m_mountPointEdit = new QLineEdit();
    m_usernameEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    
    driveLayout->addRow("Name:", m_nameEdit);
    driveLayout->addRow("Server:", m_serverEdit);
    driveLayout->addRow("Port:", m_portSpin);
    driveLayout->addRow("Share:", m_shareEdit);
    driveLayout->addRow("Mount Point:", m_mountPointEdit);
    driveLayout->addRow("Username:", m_usernameEdit);
    driveLayout->addRow("Password:", m_passwordEdit);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_cancelButton = new QPushButton("Cancel");
    m_saveButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addWidget(driveGroup);
    mainLayout->addLayout(buttonLayout);
}

void NetworkDriveDialog::setupConnections() {
    connect(m_saveButton, &QPushButton::clicked, this, &NetworkDriveDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &NetworkDriveDialog::onCancel);
}

void NetworkDriveDialog::setDrive(const NetworkManager::NetworkDrive& drive) {
    m_drive = drive;
    
    m_nameEdit->setText(drive.name);
    m_serverEdit->setText(drive.server);
    m_portSpin->setValue(drive.port);
    m_shareEdit->setText(drive.path);
    m_mountPointEdit->setText(drive.path);
    m_usernameEdit->setText(drive.username);
}

NetworkManager::NetworkDrive NetworkDriveDialog::drive() const {
    NetworkManager::NetworkDrive drive = m_drive;
    
    drive.name = m_nameEdit->text();
    drive.server = m_serverEdit->text();
    drive.port = m_portSpin->value();
    drive.path = m_shareEdit->text();
    drive.username = m_usernameEdit->text();
    // Note: We don't set the password here for security reasons
    
    return drive;
}

void NetworkDriveDialog::onSave() {
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a drive name.");
        return;
    }
    
    if (m_serverEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a server address.");
        return;
    }
    
    accept();
}

void NetworkDriveDialog::onCancel() {
    reject();
}

// CloudStorageDialog implementation
CloudStorageDialog::CloudStorageDialog(QWidget *parent)
    : QDialog(parent)
    , m_providerCombo(nullptr)
    , m_accessTokenEdit(nullptr)
    , m_refreshTokenEdit(nullptr)
    , m_clientIdEdit(nullptr)
    , m_clientSecretEdit(nullptr)
    , m_folderIdEdit(nullptr)
    , m_enabledCheck(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
}

CloudStorageDialog::~CloudStorageDialog() {
}

void CloudStorageDialog::setupUI() {
    setWindowTitle("Cloud Storage Configuration");
    setMinimumSize(400, 350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* configGroup = new QGroupBox("Configuration");
    QFormLayout* configLayout = new QFormLayout(configGroup);
    
    m_providerCombo = new QComboBox();
    m_providerCombo->addItem("Dropbox");
    m_providerCombo->addItem("Google Drive");
    m_providerCombo->addItem("OneDrive");
    m_providerCombo->addItem("Custom");
    
    m_accessTokenEdit = new QLineEdit();
    m_accessTokenEdit->setEchoMode(QLineEdit::Password);
    
    m_refreshTokenEdit = new QLineEdit();
    m_refreshTokenEdit->setEchoMode(QLineEdit::Password);
    
    m_clientIdEdit = new QLineEdit();
    m_clientSecretEdit = new QLineEdit();
    m_clientSecretEdit->setEchoMode(QLineEdit::Password);
    
    m_folderIdEdit = new QLineEdit();
    
    m_enabledCheck = new QCheckBox("Enabled");
    m_enabledCheck->setChecked(true);
    
    configLayout->addRow("Provider:", m_providerCombo);
    configLayout->addRow("Access Token:", m_accessTokenEdit);
    configLayout->addRow("Refresh Token:", m_refreshTokenEdit);
    configLayout->addRow("Client ID:", m_clientIdEdit);
    configLayout->addRow("Client Secret:", m_clientSecretEdit);
    configLayout->addRow("Folder ID:", m_folderIdEdit);
    configLayout->addRow("", m_enabledCheck);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_cancelButton = new QPushButton("Cancel");
    m_saveButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addWidget(configGroup);
    mainLayout->addLayout(buttonLayout);
}

void CloudStorageDialog::setupConnections() {
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CloudStorageDialog::onProviderChanged);
    connect(m_saveButton, &QPushButton::clicked, this, &CloudStorageDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &CloudStorageDialog::onCancel);
}

void CloudStorageDialog::setConfig(const NetworkManager::CloudStorageConfig& config) {
    m_config = config;
    
    m_providerCombo->setCurrentIndex(static_cast<int>(config.provider));
    m_accessTokenEdit->setText(config.accessToken);
    m_refreshTokenEdit->setText(config.refreshToken);
    m_clientIdEdit->setText(config.clientId);
    m_clientSecretEdit->setText(config.clientSecret);
    m_folderIdEdit->setText(config.folderId);
    m_enabledCheck->setChecked(config.enabled);
}

NetworkManager::CloudStorageConfig CloudStorageDialog::config() const {
    NetworkManager::CloudStorageConfig config = m_config;
    
    config.provider = static_cast<NetworkManager::CloudProvider>(m_providerCombo->currentIndex());
    config.accessToken = m_accessTokenEdit->text();
    config.refreshToken = m_refreshTokenEdit->text();
    config.clientId = m_clientIdEdit->text();
    config.clientSecret = m_clientSecretEdit->text();
    config.folderId = m_folderIdEdit->text();
    config.enabled = m_enabledCheck->isChecked();
    
    return config;
}

void CloudStorageDialog::onProviderChanged(int index) {
    Q_UNUSED(index);
    // Update UI based on selected provider
}

void CloudStorageDialog::onSave() {
    accept();
}

void CloudStorageDialog::onCancel() {
    reject();
}