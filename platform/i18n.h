#ifndef PLATFORM_I18N_H
#define PLATFORM_I18N_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <memory>

class I18nManager : public QObject {
    Q_OBJECT

public:
    struct LanguageInfo {
        QString code;
        QString name;
        QString nativeName;
        bool isRTL; // Right-to-left
        QStringList translators;
    };

public:
    explicit I18nManager(QObject *parent = nullptr);
    ~I18nManager() override;

    // Language management
    QList<LanguageInfo> availableLanguages() const;
    bool isLanguageSupported(const QString& languageCode) const;
    bool loadLanguage(const QString& languageCode);
    QString currentLanguage() const;
    void setCurrentLanguage(const QString& languageCode);
    
    // Translation
    QString translate(const QString& context, const QString& sourceText, const QString& disambiguation = QString(), int n = -1) const;
    QString translatePlural(const QString& context, const QString& singular, const QString& plural, int n) const;
    
    // Locale information
    QLocale currentLocale() const;
    void setLocale(const QLocale& locale);
    QString formatNumber(double number, int decimals = 2) const;
    QString formatCurrency(double amount, const QString& currencyCode = "USD") const;
    QString formatDate(const QDate& date, QLocale::FormatType format = QLocale::ShortFormat) const;
    QString formatTime(const QTime& time, QLocale::FormatType format = QLocale::ShortFormat) const;
    QString formatDateTime(const QDateTime& dateTime, QLocale::FormatType format = QLocale::ShortFormat) const;
    
    // Text direction
    Qt::LayoutDirection textDirection() const;
    bool isRightToLeft() const;
    
    // Number formatting
    QLocale::MeasurementSystem measurementSystem() const;
    void setMeasurementSystem(QLocale::MeasurementSystem system);
    QString formatFileSize(quint64 size) const;
    
    // String utilities
    QString toTitleCase(const QString& text) const;
    QString toSentenceCase(const QString& text) const;
    QString toLowerCase(const QString& text) const;
    QString toUpperCase(const QString& text) const;

signals:
    void languageChanged(const QString& languageCode);
    void localeChanged(const QLocale& locale);

private:
    void setupLanguages();
    void loadLanguageData();
    void saveLanguageData();
    QString findTranslationFile(const QString& languageCode) const;
    
    QString m_currentLanguage;
    QLocale m_currentLocale;
    std::unique_ptr<QTranslator> m_translator;
    std::unique_ptr<QSettings> m_settings;
    QMap<QString, LanguageInfo> m_languages;
};

// Translation context helper
class Tr : public QObject
{
    Q_OBJECT

public:
    // Common UI strings
    static QString applicationName() { return tr("DiskSense64"); }
    static QString applicationDescription() { return tr("Cross-Platform Disk Analysis Suite"); }
    
    // Menu strings
    static QString menuFile() { return tr("&File"); }
    static QString menuEdit() { return tr("&Edit"); }
    static QString menuView() { return tr("&View"); }
    static QString menuTools() { return tr("&Tools"); }
    static QString menuHelp() { return tr("&Help"); }
    
    // Action strings
    static QString actionScanDirectory() { return tr("&Scan Directory..."); }
    static QString actionSettings() { return tr("&Settings..."); }
    static QString actionExit() { return tr("E&xit"); }
    static QString actionAbout() { return tr("&About"); }
    
    // Dashboard strings
    static QString dashboardTitle() { return tr("Dashboard"); }
    static QString dashboardSystemInfo() { return tr("System Information"); }
    static QString dashboardQuickStats() { return tr("Quick Statistics"); }
    static QString dashboardRecentScans() { return tr("Recent Scans"); }
    static QString dashboardQuickAccess() { return tr("Quick Access"); }
    
    // Settings strings
    static QString settingsTitle() { return tr("Settings"); }
    static QString settingsUserPreferences() { return tr("User Preferences"); }
    static QString settingsPerformance() { return tr("Performance"); }
    static QString settingsVisualization() { return tr("Visualization"); }
    static QString settingsExclusions() { return tr("Exclusions"); }
    
    // File explorer strings
    static QString fileExplorerTitle() { return tr("File Explorer"); }
    static QString fileExplorerTree() { return tr("Directory Tree"); }
    static QString fileExplorerProperties() { return tr("File Properties"); }
    
    // Error messages
    static QString errorScanInProgress() { return tr("A scan is already in progress. Please wait or cancel the current scan."); }
    static QString errorInvalidPath() { return tr("The specified path does not exist."); }
    static QString errorPermissionDenied() { return tr("Permission denied. Please check your access rights."); }
    
    // Confirmation messages
    static QString confirmCancelScan() { return tr("Are you sure you want to cancel the current scan?"); }
    static QString confirmDeleteFiles() { return tr("Are you sure you want to delete the selected files?"); }
    
    // Status messages
    static QString statusReady() { return tr("Ready"); }
    static QString statusScanning() { return tr("Scanning..."); }
    static QString statusProcessing() { return tr("Processing..."); }
    static QString statusComplete() { return tr("Complete"); }
    
private:
    Tr() {}
};

// Language data loader
class LanguageDataLoader : public QObject
{
    Q_OBJECT

public:
    static QMap<QString, I18nManager::LanguageInfo> loadLanguageData();
    static QString getLanguageName(const QString& code);
    static QString getNativeLanguageName(const QString& code);
    static bool isRightToLeftLanguage(const QString& code);
    
private:
    static void initializeLanguageData();
    static QMap<QString, I18nManager::LanguageInfo> s_languageData;
    static bool s_initialized;
};

#endif // PLATFORM_I18N_H