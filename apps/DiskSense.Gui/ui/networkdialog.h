#ifndef UI_NETWORKDIALOG_H
#define UI_NETWORKDIALOG_H

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
#include "core/network/networkmanager.h"

class NetworkManager;
class RemoteScanner;
class CloudStorageManager;

class NetworkDialog : public QDialog {
    Q_OBJECT

public:
    explicit NetworkDialog(QWidget *parent = nullptr);
    ~NetworkDialog() override;

public slots:
    void refreshConnections();
    void refreshCloudStorage();
    void refreshRemoteFiles();

private slots:
    void onConnectToServer();
    void onDisconnectFromServer();
    void onMountNetworkDrive();
    void onUnmountNetworkDrive();
    void onConnectToCloudStorage();
    void onDisconnectFromCloudStorage();
    void onStartRemoteScan();
    void onCancelRemoteScan();
    void onUploadFile();
    void onDownloadFile();
    void onDeleteRemoteFile();
    
    void onConnectionStatusChanged(NetworkManager::ConnectionStatus status);
    void onNetworkDriveMounted(const QString& driveId);
    void onNetworkDriveUnmounted(const QString& driveId);
    void onCloudStorageConnected();
    void onCloudStorageDisconnected();
    void onFileUploaded(const QString& localPath, const QString& remotePath);
    void onFileDownloaded(const QString& remotePath, const QString& localPath);
    void onFileUploadFailed(const QString& localPath, const QString& error);
    void onFileDownloadFailed(const QString& remotePath, const QString& error);
    void onBandwidthUpdated(quint64 uploadSpeed, quint64 downloadSpeed);
    
    void onRemoteScanStarted();
    void onRemoteScanProgress(int percentage, quint64 filesProcessed);
    void onRemoteScanCompleted(const QList<RemoteScanner::RemoteFileEntry>& results);
    void onRemoteScanCancelled();
    void onRemoteScanError(const QString& error);

private:
    void setupUI();
    void setupConnections();
    void populateConnectionList();
    void populateCloudStorageList();
    void populateRemoteFileList();
    void updateConnectionStatus();
    void updateBandwidthDisplay();
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Connections tab
    QGroupBox* m_serverGroup;
    QLineEdit* m_serverEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QLabel* m_connectionStatus;
    
    QTreeWidget* m_networkDrivesList;
    QPushButton* m_mountDriveButton;
    QPushButton* m_unmountDriveButton;
    
    // Cloud storage tab
    QComboBox* m_cloudProviderCombo;
    QLineEdit* m_cloudAccessTokenEdit;
    QLineEdit* m_cloudClientIdEdit;
    QLineEdit* m_cloudClientSecretEdit;
    QPushButton* m_connectCloudButton;
    QPushButton* m_disconnectCloudButton;
    QLabel* m_cloudStatus;
    
    // Remote files tab
    QTreeWidget* m_remoteFilesList;
    QPushButton* m_uploadButton;
    QPushButton* m_downloadButton;
    QPushButton* m_deleteRemoteButton;
    
    // Remote scanning tab
    QGroupBox* m_scanOptionsGroup;
    QLineEdit* m_remotePathEdit;
    QCheckBox* m_includeSubdirsCheck;
    QSpinBox* m_maxDepthSpin;
    QPushButton* m_startScanButton;
    QPushButton* m_cancelScanButton;
    QProgressBar* m_scanProgress;
    QLabel* m_scanStatus;
    
    // Bandwidth monitoring
    QLabel* m_uploadSpeedLabel;
    QLabel* m_downloadSpeedLabel;
    
    // Core components
    std::unique_ptr<NetworkManager> m_networkManager;
    std::unique_ptr<RemoteScanner> m_remoteScanner;
    std::unique_ptr<CloudStorageManager> m_cloudManager;
    
    // Progress tracking
    QTimer* m_refreshTimer;
};

// Network drive editor dialog
class NetworkDriveDialog : public QDialog {
    Q_OBJECT

public:
    explicit NetworkDriveDialog(QWidget *parent = nullptr);
    ~NetworkDriveDialog() override;

    void setDrive(const NetworkManager::NetworkDrive& drive);
    NetworkManager::NetworkDrive drive() const;

private slots:
    void onSave();
    void onCancel();

private:
    void setupUI();
    void setupConnections();
    
    NetworkManager::NetworkDrive m_drive;
    
    // UI components
    QLineEdit* m_nameEdit;
    QLineEdit* m_serverEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_shareEdit;
    QLineEdit* m_mountPointEdit;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

// Cloud storage configuration dialog
class CloudStorageDialog : public QDialog {
    Q_OBJECT

public:
    explicit CloudStorageDialog(QWidget *parent = nullptr);
    ~CloudStorageDialog() override;

    void setConfig(const NetworkManager::CloudStorageConfig& config);
    NetworkManager::CloudStorageConfig config() const;

private slots:
    void onProviderChanged(int index);
    void onSave();
    void onCancel();

private:
    void setupUI();
    void setupConnections();
    
    NetworkManager::CloudStorageConfig m_config;
    
    // UI components
    QComboBox* m_providerCombo;
    QLineEdit* m_accessTokenEdit;
    QLineEdit* m_refreshTokenEdit;
    QLineEdit* m_clientIdEdit;
    QLineEdit* m_clientSecretEdit;
    QLineEdit* m_folderIdEdit;
    QCheckBox* m_enabledCheck;
    
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // UI_NETWORKDIALOG_H