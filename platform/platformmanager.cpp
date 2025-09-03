#include "platformmanager.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QTranslator>
#include <QLibraryInfo>
#include <QStyleFactory>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStorageInfo>
#include <QSysInfo>
#include <QOperatingSystemVersion>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QScreen>
#include <QFont>
#include <QPalette>
#include <QDesktopWidget>
#include <QMainWindow>
#include <algorithm>
#include <memory>

// Static variables
bool FileSystemUtils::s_platformInitialized = false;
bool ProcessUtils::s_platformInitialized = false;
bool NetworkUtils::s_platformInitialized = false;

// PlatformManager implementation
PlatformManager::PlatformManager(QObject *parent)
    : QObject(parent)
    , m_platform(Unknown)
    , m_architecture(UnknownArch)
    , m_fontSize(QApplication::font().pointSize())
    , m_highContrastMode(false)
    , m_translator(nullptr)
    , m_settings(nullptr)
    , m_fileWatcher(nullptr)
{
    setupPlatform();
    setupTranslations();
    setupThemes();
    setupFileSystemWatcher();
    loadSystemSettings();
    
    // Start periodic system info updates
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PlatformManager::updateSystemInfo);
    timer->start(30000); // Update every 30 seconds
    
    // Start battery status checks
    QTimer* batteryTimer = new QTimer(this);
    connect(batteryTimer, &QTimer::timeout, this, &PlatformManager::checkBatteryStatus);
    batteryTimer->start(60000); // Check every minute
}

PlatformManager::~PlatformManager() {
    saveSystemSettings();
}

PlatformManager::Platform PlatformManager::currentPlatform() {
#ifdef Q_OS_WIN
    return Windows;
#elif defined(Q_OS_LINUX)
    return Linux;
#elif defined(Q_OS_MACOS)
    return MacOS;
#else
    return Unknown;
#endif
}

PlatformManager::Architecture PlatformManager::currentArchitecture() {
    QString arch = QSysInfo::currentCpuArchitecture();
    if (arch == "i386" || arch == "i686") {
        return X86_32;
    } else if (arch == "x86_64" || arch == "amd64") {
        return X86_64;
    } else if (arch.startsWith("arm")) {
        if (arch.contains("64")) {
            return ARM64;
        } else {
            return ARM32;
        }
    }
    return UnknownArch;
}

QString PlatformManager::platformName(Platform platform) {
    switch (platform) {
        case Windows: return "Windows";
        case Linux: return "Linux";
        case MacOS: return "macOS";
        case Unknown:
        default: return "Unknown";
    }
}

QString PlatformManager::architectureName(Architecture arch) {
    switch (arch) {
        case X86_32: return "x86 (32-bit)";
        case X86_64: return "x86_64 (64-bit)";
        case ARM32: return "ARM (32-bit)";
        case ARM64: return "ARM64 (64-bit)";
        case UnknownArch:
        default: return "Unknown";
    }
}

PlatformManager::SystemInfo PlatformManager::getSystemInfo() {
    SystemInfo info;
    info.platform = currentPlatform();
    info.architecture = currentArchitecture();
    info.osName = QSysInfo::prettyProductName();
    info.osVersion = QSysInfo::productVersion();
    info.hostname = QSysInfo::machineHostName();
    info.username = qgetenv("USER").isEmpty() ? qgetenv("USERNAME") : qgetenv("USER");
    
#ifdef Q_OS_WIN
    // Get memory info on Windows
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    info.totalMemory = memInfo.ullTotalPhys;
    info.availableMemory = memInfo.ullAvailPhys;
#elif defined(Q_OS_LINUX)
    // Get memory info on Linux
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        QTextStream stream(&meminfo);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("MemTotal:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) {
                    info.totalMemory = parts[1].toULongLong() * 1024; // Convert KB to bytes
                }
            } else if (line.startsWith("MemAvailable:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) {
                    info.availableMemory = parts[1].toULongLong() * 1024; // Convert KB to bytes
                }
            }
        }
        meminfo.close();
    }
#endif
    
    // Get drives
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            info.drives.append(storage.rootPath());
        }
    }
    
    // Get language info
    info.defaultLanguage = QLocale::system().name();
    info.availableLanguages = QLocale::matchingLocales(
        QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry
    ).size();
    
    return info;
}

void PlatformManager::applyNativeTheme() {
    // Apply platform-specific styling
    switch (m_platform) {
        case Windows:
            QApplication::setStyle(QStyleFactory::create("WindowsVista"));
            break;
        case Linux:
            QApplication::setStyle(QStyleFactory::create("Fusion"));
            break;
        case MacOS:
            QApplication::setStyle(QStyleFactory::create("Macintosh"));
            break;
        default:
            QApplication::setStyle(QStyleFactory::create("Fusion"));
            break;
    }
}

void PlatformManager::setApplicationStyle(QStyle* style) {
    QApplication::setStyle(style);
}

QStyle* PlatformManager::applicationStyle() const {
    return QApplication::style();
}

QList<PlatformManager::ThemeInfo> PlatformManager::availableThemes() const {
    return m_themes.values();
}

bool PlatformManager::loadTheme(const QString& themeName) {
    if (!m_themes.contains(themeName)) {
        return false;
    }
    
    const ThemeInfo& theme = m_themes[themeName];
    m_currentTheme = themeName;
    
    // Apply theme
    if (!theme.styleSheet.isEmpty()) {
        qApp->setStyleSheet(theme.styleSheet);
    }
    
    emit themeChanged(themeName);
    return true;
}

bool PlatformManager::saveTheme(const QString& themeName, const ThemeInfo& theme) {
    m_themes[themeName] = theme;
    return true;
}

QString PlatformManager::currentTheme() const {
    return m_currentTheme;
}

void PlatformManager::setCurrentTheme(const QString& themeName) {
    loadTheme(themeName);
}

bool PlatformManager::loadLanguage(const QString& languageCode) {
    if (m_translator) {
        QApplication::removeTranslator(m_translator.get());
    }
    
    m_translator = std::make_unique<QTranslator>();
    
    // Try to load the translation file
    QString translationFile = QString(":/translations/disksense_%1.qm").arg(languageCode);
    if (m_translator->load(translationFile)) {
        QApplication::installTranslator(m_translator.get());
        m_currentLanguage = languageCode;
        emit languageChanged(languageCode);
        return true;
    }
    
    // Try system translations
    if (m_translator->load(QLocale(languageCode), "disksense", "_", ":/translations")) {
        QApplication::installTranslator(m_translator.get());
        m_currentLanguage = languageCode;
        emit languageChanged(languageCode);
        return true;
    }
    
    return false;
}

QString PlatformManager::currentLanguage() const {
    return m_currentLanguage;
}

void PlatformManager::setCurrentLanguage(const QString& languageCode) {
    loadLanguage(languageCode);
}

QStringList PlatformManager::availableLanguages() const {
    QStringList languages;
    languages << "en" << "es" << "fr" << "de" << "it" << "pt" << "ru" << "zh" << "ja";
    return languages;
}

QString PlatformManager::translate(const QString& context, const QString& sourceText) const {
    return QCoreApplication::translate(context.toUtf8().constData(), sourceText.toUtf8().constData());
}

void PlatformManager::applyPlatformOptimizations() {
    // Apply platform-specific optimizations
    switch (m_platform) {
        case Windows:
            // Enable file system caching
            qputenv("QT_FILESYSTEM_CACHE", "1");
            break;
        case Linux:
            // Optimize for Linux file systems
            qputenv("QT_LINUX_USE_EPOLL", "1");
            break;
        case MacOS:
            // Optimize for macOS
            qputenv("QT_MAC_WANTS_LAYER", "1");
            break;
        default:
            break;
    }
}

QVariant PlatformManager::getPlatformSetting(const QString& key, const QVariant& defaultValue) const {
    if (m_settings) {
        return m_settings->value(key, defaultValue);
    }
    return defaultValue;
}

void PlatformManager::setPlatformSetting(const QString& key, const QVariant& value) {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

QStringList PlatformManager::getSystemDrives() const {
    QStringList drives;
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            drives.append(storage.rootPath());
        }
    }
    return drives;
}

QString PlatformManager::getHomeDirectory() const {
    return QDir::homePath();
}

QString PlatformManager::getDocumentsDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString PlatformManager::getDownloadsDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString PlatformManager::getTempDirectory() const {
    return QDir::tempPath();
}

QString PlatformManager::getConfigDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

void PlatformManager::showNotification(const QString& title, const QString& message) {
    // Show a system notification
#ifdef Q_OS_WIN
    // Windows notification
    Q_UNUSED(title);
    Q_UNUSED(message);
#elif defined(Q_OS_MACOS)
    // macOS notification
    QProcess::execute("osascript", QStringList() << "-e" 
        << QString("display notification \"%1\" with title \"%2\"").arg(message, title));
#elif defined(Q_OS_LINUX)
    // Linux notification using notify-send
    QProcess::execute("notify-send", QStringList() << title << message);
#endif
}

void PlatformManager::showSystemTrayMessage(const QString& title, const QString& message) {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
        trayIcon->show();
        trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

bool PlatformManager::isBatteryPowered() const {
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus)) {
        return (powerStatus.BatteryFlag & 128) == 0; // Not no battery
    }
#elif defined(Q_OS_LINUX)
    QFile file("/sys/class/power_supply/BAT0/status");
    if (file.open(QIODevice::ReadOnly)) {
        QString status = file.readAll().trimmed();
        file.close();
        return status != "Unknown";
    }
#endif
    return false;
}

int PlatformManager::batteryLevel() const {
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus)) {
        if (powerStatus.BatteryLifePercent != 255) {
            return powerStatus.BatteryLifePercent;
        }
    }
#elif defined(Q_OS_LINUX)
    QFile file("/sys/class/power_supply/BAT0/capacity");
    if (file.open(QIODevice::ReadOnly)) {
        int level = file.readAll().trimmed().toInt();
        file.close();
        return level;
    }
#endif
    return -1; // Unknown
}

bool PlatformManager::isSavingPower() const {
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus)) {
        return powerStatus.SystemStatusFlag == 1;
    }
#elif defined(Q_OS_LINUX)
    QFile file("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    if (file.open(QIODevice::ReadOnly)) {
        QString governor = file.readAll().trimmed();
        file.close();
        return governor == "powersave";
    }
#endif
    return false;
}

void PlatformManager::enableHighContrastMode(bool enable) {
    m_highContrastMode = enable;
    
    if (enable) {
        // Apply high contrast palette
        QPalette palette;
        palette.setColor(QPalette::Window, Qt::black);
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, Qt::black);
        palette.setColor(QPalette::AlternateBase, Qt::darkGray);
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, Qt::darkGray);
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, Qt::cyan);
        palette.setColor(QPalette::Highlight, Qt::cyan);
        palette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(palette);
    } else {
        // Restore default palette
        qApp->setPalette(qApp->style()->standardPalette());
    }
}

bool PlatformManager::isHighContrastModeEnabled() const {
    return m_highContrastMode;
}

void PlatformManager::setFontSize(int size) {
    m_fontSize = size;
    
    QFont font = qApp->font();
    font.setPointSize(size);
    qApp->setFont(font);
}

int PlatformManager::fontSize() const {
    return m_fontSize;
}

void PlatformManager::updateSystemInfo() {
    m_systemInfo = getSystemInfo();
    emit systemSettingsChanged();
}

void PlatformManager::checkBatteryStatus() {
    int level = batteryLevel();
    if (level >= 0) {
        emit batteryLevelChanged(level);
    }
    
    bool savingPower = isSavingPower();
    emit powerSavingModeChanged(savingPower);
}

void PlatformManager::onSystemSettingsChanged() {
    emit systemSettingsChanged();
}

void PlatformManager::onFileSystemChanged(const QString& path) {
    Q_UNUSED(path);
    // Handle file system changes
}

void PlatformManager::setupPlatform() {
    m_platform = currentPlatform();
    m_architecture = currentArchitecture();
}

void PlatformManager::setupTranslations() {
    // Set up translation system
    m_settings = std::make_unique<QSettings>(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/settings.ini",
        QSettings::IniFormat
    );
    
    // Load saved language
    QString savedLanguage = m_settings->value("language", QLocale::system().name().left(2)).toString();
    loadLanguage(savedLanguage);
}

void PlatformManager::setupThemes() {
    // Set up built-in themes
    ThemeInfo lightTheme;
    lightTheme.name = "light";
    lightTheme.displayName = "Light";
    lightTheme.isDark = false;
    m_themes["light"] = lightTheme;
    
    ThemeInfo darkTheme;
    darkTheme.name = "dark";
    darkTheme.displayName = "Dark";
    darkTheme.isDark = true;
    m_themes["dark"] = darkTheme;
    
    // Load saved theme
    QString savedTheme = m_settings->value("theme", "light").toString();
    loadTheme(savedTheme);
}

void PlatformManager::setupFileSystemWatcher() {
    m_fileWatcher = std::make_unique<QFileSystemWatcher>();
    connect(m_fileWatcher.get(), &QFileSystemWatcher::directoryChanged,
            this, &PlatformManager::onFileSystemChanged);
    connect(m_fileWatcher.get(), &QFileSystemWatcher::fileChanged,
            this, &PlatformManager::onFileSystemChanged);
}

void PlatformManager::loadSystemSettings() {
    if (m_settings) {
        m_currentTheme = m_settings->value("theme", "light").toString();
        m_currentLanguage = m_settings->value("language", QLocale::system().name().left(2)).toString();
        m_fontSize = m_settings->value("fontSize", QApplication::font().pointSize()).toInt();
        m_highContrastMode = m_settings->value("highContrast", false).toBool();
    }
}

void PlatformManager::saveSystemSettings() {
    if (m_settings) {
        m_settings->setValue("theme", m_currentTheme);
        m_settings->setValue("language", m_currentLanguage);
        m_settings->setValue("fontSize", m_fontSize);
        m_settings->setValue("highContrast", m_highContrastMode);
        m_settings->sync();
    }
}

// FileSystemUtils implementation
FileSystemUtils::FileSystemUtils(QObject *parent)
    : QObject(parent)
{
    initializePlatform();
}

FileSystemUtils::~FileSystemUtils() {
}

QString FileSystemUtils::toNativePath(const QString& path) {
    return QDir::toNativeSeparators(path);
}

QString FileSystemUtils::fromNativePath(const QString& path) {
    return QDir::fromNativeSeparators(path);
}

QString FileSystemUtils::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

bool FileSystemUtils::isAbsolutePath(const QString& path) {
    return QDir::isAbsolutePath(path);
}

QString FileSystemUtils::joinPaths(const QString& path1, const QString& path2) {
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

bool FileSystemUtils::copyFile(const QString& source, const QString& destination) {
    return QFile::copy(source, destination);
}

bool FileSystemUtils::moveFile(const QString& source, const QString& destination) {
    return QFile::rename(source, destination);
}

bool FileSystemUtils::deleteFile(const QString& path) {
    return QFile::remove(path);
}

bool FileSystemUtils::createDirectory(const QString& path) {
    return QDir().mkpath(path);
}

bool FileSystemUtils::deleteDirectory(const QString& path) {
    return QDir(path).removeRecursively();
}

quint64 FileSystemUtils::getFileSize(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.size();
}

QDateTime FileSystemUtils::getLastModified(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.lastModified();
}

QString FileSystemUtils::getFileOwner(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.owner();
}

QString FileSystemUtils::getFileGroup(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.group();
}

QString FileSystemUtils::getFilePermissions(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.permissions().toString();
}

bool FileSystemUtils::createSymbolicLink(const QString& target, const QString& linkPath) {
#ifdef Q_OS_UNIX
    return symlink(target.toLocal8Bit().constData(), linkPath.toLocal8Bit().constData()) == 0;
#else
    Q_UNUSED(target);
    Q_UNUSED(linkPath);
    return false;
#endif
}

bool FileSystemUtils::isSymbolicLink(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.isSymLink();
}

QString FileSystemUtils::readSymbolicLink(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.symLinkTarget();
}

bool FileSystemUtils::createHardLink(const QString& target, const QString& linkPath) {
#ifdef Q_OS_UNIX
    return link(target.toLocal8Bit().constData(), linkPath.toLocal8Bit().constData()) == 0;
#else
    Q_UNUSED(target);
    Q_UNUSED(linkPath);
    return false;
#endif
}

bool FileSystemUtils::isHardLink(const QString& path) {
    return getHardLinkCount(path) > 1;
}

int FileSystemUtils::getHardLinkCount(const QString& path) {
#ifdef Q_OS_UNIX
    struct stat statBuf;
    if (stat(path.toLocal8Bit().constData(), &statBuf) == 0) {
        return statBuf.st_nlink;
    }
#else
    Q_UNUSED(path);
#endif
    return 1;
}

quint64 FileSystemUtils::getVolumeTotalSpace(const QString& path) {
    QStorageInfo storage(path);
    return storage.bytesTotal();
}

quint64 FileSystemUtils::getVolumeFreeSpace(const QString& path) {
    QStorageInfo storage(path);
    return storage.bytesAvailable();
}

QString FileSystemUtils::getVolumeName(const QString& path) {
    QStorageInfo storage(path);
    return storage.name();
}

QString FileSystemUtils::getVolumeFileSystem(const QString& path) {
    QStorageInfo storage(path);
    return storage.fileSystemType();
}

void FileSystemUtils::initializePlatform() {
    if (!s_platformInitialized) {
        s_platformInitialized = true;
        // Platform-specific initialization
    }
}

// ProcessUtils implementation
ProcessUtils::ProcessUtils(QObject *parent)
    : QObject(parent)
{
    initializePlatform();
}

ProcessUtils::~ProcessUtils() {
}

qint64 ProcessUtils::launchProcess(const QString& command, const QStringList& arguments) {
    QProcess* process = new QProcess();
    process->start(command, arguments);
    
    if (process->waitForStarted(5000)) {
        qint64 pid = process->processId();
        process->deleteLater();
        return pid;
    }
    
    process->deleteLater();
    return -1;
}

bool ProcessUtils::terminateProcess(qint64 pid) {
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (hProcess != nullptr) {
        BOOL result = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return result != FALSE;
    }
    return false;
#else
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
#endif
}

bool ProcessUtils::killProcess(qint64 pid) {
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (hProcess != nullptr) {
        BOOL result = TerminateProcess(hProcess, 1);
        CloseHandle(hProcess);
        return result != FALSE;
    }
    return false;
#else
    return kill(static_cast<pid_t>(pid), SIGKILL) == 0;
#endif
}

bool ProcessUtils::isProcessRunning(qint64 pid) {
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (hProcess != nullptr) {
        DWORD exitCode;
        BOOL result = GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess);
        return result != FALSE && exitCode == STILL_ACTIVE;
    }
    return false;
#else
    return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

ProcessUtils::ProcessInfo ProcessUtils::getProcessInfo(qint64 pid) {
    ProcessInfo info;
    info.pid = pid;
    
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
    if (hProcess != nullptr) {
        // Get process name
        char buffer[MAX_PATH];
        DWORD size = sizeof(buffer);
        if (QueryFullProcessImageNameA(hProcess, 0, buffer, &size)) {
            info.name = QString::fromLocal8Bit(buffer);
            QStringList parts = info.name.split("\\");
            if (!parts.isEmpty()) {
                info.name = parts.last();
            }
        }
        
        // Get start time
        FILETIME creationTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
            // Convert FILETIME to QDateTime
            ULARGE_INTEGER ull;
            ull.LowPart = creationTime.dwLowDateTime;
            ull.HighPart = creationTime.dwHighDateTime;
            info.startTime = QDateTime::fromMSecsSinceEpoch(ull.QuadPart / 10000 - 11644473600000LL);
        }
        
        CloseHandle(hProcess);
    }
#elif defined(Q_OS_LINUX)
    QString statPath = QString("/proc/%1/stat").arg(pid);
    QFile statFile(statPath);
    if (statFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&statFile);
        QString line = stream.readLine();
        statFile.close();
        
        QStringList parts = line.split(" ");
        if (parts.size() > 21) {
            info.name = parts[1].remove('(').remove(')');
            info.startTime = QDateTime::fromSecsSinceEpoch(parts[21].toLongLong() / sysconf(_SC_CLK_TCK));
        }
    }
#endif
    
    return info;
}

QList<ProcessUtils::ProcessInfo> ProcessUtils::getAllProcesses() {
    QList<ProcessInfo> processes;
    
#ifdef Q_OS_WIN
    // Windows implementation would use EnumProcesses
    Q_UNUSED(processes);
#elif defined(Q_OS_LINUX)
    QDir procDir("/proc");
    QStringList dirs = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : dirs) {
        bool ok;
        qint64 pid = dir.toLongLong(&ok);
        if (ok && pid > 0) {
            ProcessInfo info = getProcessInfo(pid);
            if (!info.name.isEmpty()) {
                processes.append(info);
            }
        }
    }
#endif
    
    return processes;
}

QList<ProcessUtils::ProcessInfo> ProcessUtils::findProcessesByName(const QString& name) {
    QList<ProcessInfo> matchingProcesses;
    QList<ProcessInfo> allProcesses = getAllProcesses();
    
    for (const ProcessInfo& process : allProcesses) {
        if (process.name.contains(name, Qt::CaseInsensitive)) {
            matchingProcesses.append(process);
        }
    }
    
    return matchingProcesses;
}

int ProcessUtils::getProcessorCount() {
    return QThread::idealThreadCount();
}

quint64 ProcessUtils::getTotalMemory() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys;
#elif defined(Q_OS_LINUX)
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        QTextStream stream(&meminfo);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("MemTotal:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) {
                    return parts[1].toULongLong() * 1024; // Convert KB to bytes
                }
            }
        }
        meminfo.close();
    }
#endif
    return 0;
}

quint64 ProcessUtils::getAvailableMemory() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
#elif defined(Q_OS_LINUX)
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        QTextStream stream(&meminfo);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("MemAvailable:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) {
                    return parts[1].toULongLong() * 1024; // Convert KB to bytes
                }
            }
        }
        meminfo.close();
    }
#endif
    return 0;
}

double ProcessUtils::getCpuUsage() {
    // Simplified CPU usage calculation
    return 0.0;
}

QString ProcessUtils::getSystemLoad() {
#ifdef Q_OS_LINUX
    QFile loadFile("/proc/loadavg");
    if (loadFile.open(QIODevice::ReadOnly)) {
        QString load = loadFile.readAll().trimmed();
        loadFile.close();
        return load;
    }
#endif
    return QString();
}

void ProcessUtils::initializePlatform() {
    if (!s_platformInitialized) {
        s_platformInitialized = true;
        // Platform-specific initialization
    }
}

// NetworkUtils implementation
NetworkUtils::NetworkUtils(QObject *parent)
    : QObject(parent)
{
    initializePlatform();
}

NetworkUtils::~NetworkUtils() {
}

QList<NetworkUtils::NetworkInterface> NetworkUtils::getNetworkInterfaces() {
    QList<NetworkInterface> interfaces;
    
    // This is a simplified implementation
    // A full implementation would use platform-specific APIs
    
    return interfaces;
}

NetworkUtils::NetworkInterface NetworkUtils::getInterfaceByName(const QString& name) {
    Q_UNUSED(name);
    NetworkInterface interface;
    return interface;
}

QString NetworkUtils::getPrimaryIpAddress() {
    // Return the primary IP address
    return "127.0.0.1";
}

QList<NetworkUtils::NetworkDrive> NetworkUtils::getNetworkDrives() {
    QList<NetworkDrive> drives;
    
    // This is a simplified implementation
    // A full implementation would use platform-specific APIs
    
    return drives;
}

bool NetworkUtils::mountNetworkDrive(const QString& server, const QString& share, 
                                   const QString& mountPoint, const QString& username, 
                                   const QString& password) {
    Q_UNUSED(server);
    Q_UNUSED(share);
    Q_UNUSED(mountPoint);
    Q_UNUSED(username);
    Q_UNUSED(password);
    
    // This is a simplified implementation
    // A full implementation would use platform-specific APIs
    
    return true;
}

bool NetworkUtils::unmountNetworkDrive(const QString& mountPoint) {
    Q_UNUSED(mountPoint);
    
    // This is a simplified implementation
    // A full implementation would use platform-specific APIs
    
    return true;
}

QString NetworkUtils::getHostname() {
    return QSysInfo::machineHostName();
}

QString NetworkUtils::getDomainName() {
    // This is a simplified implementation
    return "localdomain";
}

QList<QString> NetworkUtils::getDnsServers() {
    QList<QString> servers;
    // This is a simplified implementation
    servers << "8.8.8.8" << "8.8.4.4";
    return servers;
}

QString NetworkUtils::getGateway() {
    // This is a simplified implementation
    return "192.168.1.1";
}

bool NetworkUtils::isInternetConnected() {
    // This is a simplified implementation
    // A real implementation would check for actual connectivity
    return true;
}

int NetworkUtils::getNetworkLatency(const QString& host) {
    Q_UNUSED(host);
    // This is a simplified implementation
    // A real implementation would perform an actual ping
    return 10; // ms
}

void NetworkUtils::initializePlatform() {
    if (!s_platformInitialized) {
        s_platformInitialized = true;
        // Platform-specific initialization
    }
}