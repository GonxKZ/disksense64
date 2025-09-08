#ifndef PLATFORM_PLATFORMMANAGER_H
#define PLATFORM_PLATFORMMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QStyle>
#include <QLocale>
#include <QTranslator>
#include <QDateTime>
#include <memory>

class QFileSystemWatcher;
class QSettings;

class PlatformManager : public QObject {
    Q_OBJECT

public:
    enum Platform {
        Windows,
        Linux,
        MacOS,
        Unknown
    };
    
    enum Architecture {
        X86_32,
        X86_64,
        ARM32,
        ARM64,
        UnknownArch
    };
    
    struct SystemInfo {
        Platform platform;
        Architecture architecture;
        QString osName;
        QString osVersion;
        QString hostname;
        QString username;
        quint64 totalMemory;
        quint64 availableMemory;
        QStringList drives;
        QString defaultLanguage;
        int availableLanguages;
    };
    
    struct ThemeInfo {
        QString name;
        QString displayName;
        bool isDark;
        QString styleSheet;
        QMap<QString, QString> colors;
    };

public:
    explicit PlatformManager(QObject *parent = nullptr);
    ~PlatformManager() override;

    // Platform detection
    static Platform currentPlatform();
    static Architecture currentArchitecture();
    static QString platformName(Platform platform);
    static QString architectureName(Architecture arch);
    static SystemInfo getSystemInfo();
    
    // Native look and feel
    void applyNativeTheme();
    void setApplicationStyle(QStyle* style);
    QStyle* applicationStyle() const;
    
    // Theme management
    QList<ThemeInfo> availableThemes() const;
    bool loadTheme(const QString& themeName);
    bool saveTheme(const QString& themeName, const ThemeInfo& theme);
    QString currentTheme() const;
    void setCurrentTheme(const QString& themeName);
    
    // Internationalization
    bool loadLanguage(const QString& languageCode);
    QString currentLanguage() const;
    void setCurrentLanguage(const QString& languageCode);
    QStringList availableLanguages() const;
    QString translate(const QString& context, const QString& sourceText) const;
    
    // Platform-specific optimizations
    void applyPlatformOptimizations();
    QVariant getPlatformSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setPlatformSetting(const QString& key, const QVariant& value);
    
    // File system integration
    QStringList getSystemDrives() const;
    QString getHomeDirectory() const;
    QString getDocumentsDirectory() const;
    QString getDownloadsDirectory() const;
    QString getTempDirectory() const;
    QString getConfigDirectory() const;
    
    // System notifications
    void showNotification(const QString& title, const QString& message);
    void showSystemTrayMessage(const QString& title, const QString& message);
    
    // Power management
    bool isBatteryPowered() const;
    int batteryLevel() const;
    bool isSavingPower() const;
    
    // Accessibility
    void enableHighContrastMode(bool enable);
    bool isHighContrastModeEnabled() const;
    void setFontSize(int size);
    int fontSize() const;

signals:
    void themeChanged(const QString& themeName);
    void languageChanged(const QString& languageCode);
    void systemSettingsChanged();
    void batteryLevelChanged(int level);
    void powerSavingModeChanged(bool enabled);

public slots:
    void updateSystemInfo();
    void checkBatteryStatus();

private slots:
    void onSystemSettingsChanged();
    void onFileSystemChanged(const QString& path);

private:
    void setupPlatform();
    void setupTranslations();
    void setupThemes();
    void setupFileSystemWatcher();
    void loadSystemSettings();
    void saveSystemSettings();
    
    Platform m_platform;
    Architecture m_architecture;
    QString m_currentTheme;
    QString m_currentLanguage;
    int m_fontSize;
    bool m_highContrastMode;
    
    std::unique_ptr<QTranslator> m_translator;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<QFileSystemWatcher> m_fileWatcher;
    
    QMap<QString, ThemeInfo> m_themes;
    SystemInfo m_systemInfo;
};

// Cross-platform file system utilities
class FileSystemUtils : public QObject {
    Q_OBJECT

public:
    explicit FileSystemUtils(QObject *parent = nullptr);
    ~FileSystemUtils() override;

    // Path utilities
    static QString toNativePath(const QString& path);
    static QString fromNativePath(const QString& path);
    static QString normalizePath(const QString& path);
    static bool isAbsolutePath(const QString& path);
    static QString joinPaths(const QString& path1, const QString& path2);
    
    // File operations
    static bool copyFile(const QString& source, const QString& destination);
    static bool moveFile(const QString& source, const QString& destination);
    static bool deleteFile(const QString& path);
    static bool createDirectory(const QString& path);
    static bool deleteDirectory(const QString& path);
    
    // File attributes
    static quint64 getFileSize(const QString& path);
    static QDateTime getLastModified(const QString& path);
    static QString getFileOwner(const QString& path);
    static QString getFileGroup(const QString& path);
    static QString getFilePermissions(const QString& path);
    
    // Symbolic links
    static bool createSymbolicLink(const QString& target, const QString& linkPath);
    static bool isSymbolicLink(const QString& path);
    static QString readSymbolicLink(const QString& path);
    
    // Hard links
    static bool createHardLink(const QString& target, const QString& linkPath);
    static bool isHardLink(const QString& path);
    static int getHardLinkCount(const QString& path);
    
    // Volume information
    static quint64 getVolumeTotalSpace(const QString& path);
    static quint64 getVolumeFreeSpace(const QString& path);
    static QString getVolumeName(const QString& path);
    static QString getVolumeFileSystem(const QString& path);

private:
    static void initializePlatform();
    static bool s_platformInitialized;
};

// Cross-platform process utilities
class ProcessUtils : public QObject {
    Q_OBJECT

public:
    struct ProcessInfo {
        qint64 pid;
        QString name;
        QString commandLine;
        quint64 memoryUsage;
        double cpuUsage;
        QDateTime startTime;
        QString user;
    };

public:
    explicit ProcessUtils(QObject *parent = nullptr);
    ~ProcessUtils() override;

    // Process management
    static qint64 launchProcess(const QString& command, const QStringList& arguments = QStringList());
    static bool terminateProcess(qint64 pid);
    static bool killProcess(qint64 pid);
    static bool isProcessRunning(qint64 pid);
    
    // Process information
    static ProcessInfo getProcessInfo(qint64 pid);
    static QList<ProcessInfo> getAllProcesses();
    static QList<ProcessInfo> findProcessesByName(const QString& name);
    
    // System information
    static int getProcessorCount();
    static quint64 getTotalMemory();
    static quint64 getAvailableMemory();
    static double getCpuUsage();
    static QString getSystemLoad();

private:
    static void initializePlatform();
    static bool s_platformInitialized;
};

// Cross-platform network utilities
class NetworkUtils : public QObject {
    Q_OBJECT

public:
    struct NetworkInterface {
        QString name;
        QString hardwareAddress;
        QStringList ipAddresses;
        QStringList subnetMasks;
        bool isUp;
        bool isLoopback;
        QString type;
    };
    
    struct NetworkDrive {
        QString letter;
        QString path;
        QString server;
        QString share;
        bool isConnected;
        QString username;
    };

public:
    explicit NetworkUtils(QObject *parent = nullptr);
    ~NetworkUtils() override;

    // Network interfaces
    static QList<NetworkInterface> getNetworkInterfaces();
    static NetworkInterface getInterfaceByName(const QString& name);
    static QString getPrimaryIpAddress();
    
    // Network drives
    static QList<NetworkDrive> getNetworkDrives();
    static bool mountNetworkDrive(const QString& server, const QString& share, 
                                const QString& mountPoint, const QString& username = QString(), 
                                const QString& password = QString());
    static bool unmountNetworkDrive(const QString& mountPoint);
    
    // Network information
    static QString getHostname();
    static QString getDomainName();
    static QList<QString> getDnsServers();
    static QString getGateway();
    
    // Internet connectivity
    static bool isInternetConnected();
    static int getNetworkLatency(const QString& host = "8.8.8.8");

private:
    static void initializePlatform();
    static bool s_platformInitialized;
};

#endif // PLATFORM_PLATFORMMANAGER_H