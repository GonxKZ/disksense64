#include "i18n.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTranslator>
#include <QLibraryInfo>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>
#include <memory>

// Static variables
QMap<QString, I18nManager::LanguageInfo> LanguageDataLoader::s_languageData;
bool LanguageDataLoader::s_initialized = false;

// I18nManager implementation
I18nManager::I18nManager(QObject *parent)
    : QObject(parent)
    , m_currentLanguage(QLocale::system().name().left(2))
    , m_currentLocale(QLocale::system())
    , m_translator(nullptr)
    , m_settings(nullptr)
{
    setupLanguages();
    loadLanguageData();
    
    // Set up settings
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    m_settings = std::make_unique<QSettings>(configPath + "/i18n.ini", QSettings::IniFormat);
    
    // Load saved language
    QString savedLanguage = m_settings->value("language", m_currentLanguage).toString();
    loadLanguage(savedLanguage);
}

I18nManager::~I18nManager() {
    saveLanguageData();
}

QList<I18nManager::LanguageInfo> I18nManager::availableLanguages() const {
    return m_languages.values();
}

bool I18nManager::isLanguageSupported(const QString& languageCode) const {
    return m_languages.contains(languageCode);
}

bool I18nManager::loadLanguage(const QString& languageCode) {
    if (!m_languages.contains(languageCode)) {
        return false;
    }
    
    // Remove existing translator
    if (m_translator) {
        QApplication::removeTranslator(m_translator.get());
    }
    
    // Create new translator
    m_translator = std::make_unique<QTranslator>();
    
    // Try to load translation file
    QString translationFile = findTranslationFile(languageCode);
    if (!translationFile.isEmpty() && m_translator->load(translationFile)) {
        QApplication::installTranslator(m_translator.get());
    } else {
        // Try system translator
        if (!m_translator->load(QLocale(languageCode), "disksense", "_", ":/translations")) {
            // If that fails, use English as fallback
            m_translator.reset();
        }
    }
    
    m_currentLanguage = languageCode;
    m_currentLocale = QLocale(languageCode);
    
    emit languageChanged(languageCode);
    emit localeChanged(m_currentLocale);
    
    return true;
}

QString I18nManager::currentLanguage() const {
    return m_currentLanguage;
}

void I18nManager::setCurrentLanguage(const QString& languageCode) {
    loadLanguage(languageCode);
}

QString I18nManager::translate(const QString& context, const QString& sourceText, const QString& disambiguation, int n) const {
    return QCoreApplication::translate(context.toUtf8().constData(), sourceText.toUtf8().constData(), disambiguation.toUtf8().constData(), n);
}

QString I18nManager::translatePlural(const QString& context, const QString& singular, const QString& plural, int n) const {
    return QCoreApplication::translate(context.toUtf8().constData(), singular.toUtf8().constData(), plural.toUtf8().constData(), n);
}

QLocale I18nManager::currentLocale() const {
    return m_currentLocale;
}

void I18nManager::setLocale(const QLocale& locale) {
    m_currentLocale = locale;
    emit localeChanged(locale);
}

QString I18nManager::formatNumber(double number, int decimals) const {
    return m_currentLocale.toString(number, 'f', decimals);
}

QString I18nManager::formatCurrency(double amount, const QString& currencyCode) const {
    // This is a simplified implementation
    // A full implementation would use proper currency formatting
    return m_currentLocale.toCurrencyString(amount) + " " + currencyCode;
}

QString I18nManager::formatDate(const QDate& date, QLocale::FormatType format) const {
    return m_currentLocale.toString(date, format);
}

QString I18nManager::formatTime(const QTime& time, QLocale::FormatType format) const {
    return m_currentLocale.toString(time, format);
}

QString I18nManager::formatDateTime(const QDateTime& dateTime, QLocale::FormatType format) const {
    return m_currentLocale.toString(dateTime, format);
}

Qt::LayoutDirection I18nManager::textDirection() const {
    return m_currentLocale.textDirection();
}

bool I18nManager::isRightToLeft() const {
    return m_currentLocale.textDirection() == Qt::RightToLeft;
}

QLocale::MeasurementSystem I18nManager::measurementSystem() const {
    return m_currentLocale.measurementSystem();
}

void I18nManager::setMeasurementSystem(QLocale::MeasurementSystem system) {
    m_currentLocale = QLocale(m_currentLocale.language(), m_currentLocale.country());
    // Note: QLocale doesn't have a direct setter for measurement system
    // This would need to be handled differently in a real implementation
}

QString I18nManager::formatFileSize(quint64 size) const {
    if (size < 1024) {
        return tr("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return tr("%1 KB").arg(m_currentLocale.toString(static_cast<double>(size) / 1024, 'f', 2));
    } else if (size < 1024 * 1024 * 1024) {
        return tr("%1 MB").arg(m_currentLocale.toString(static_cast<double>(size) / (1024 * 1024), 'f', 2));
    } else {
        return tr("%1 GB").arg(m_currentLocale.toString(static_cast<double>(size) / (1024 * 1024 * 1024), 'f', 2));
    }
}

QString I18nManager::toTitleCase(const QString& text) const {
    QString result = text.toLower();
    QStringList words = result.split(" ");
    for (int i = 0; i < words.size(); ++i) {
        if (!words[i].isEmpty()) {
            words[i][0] = words[i][0].toUpper();
        }
    }
    return words.join(" ");
}

QString I18nManager::toSentenceCase(const QString& text) const {
    if (text.isEmpty()) {
        return text;
    }
    QString result = text.toLower();
    result[0] = result[0].toUpper();
    return result;
}

QString I18nManager::toLowerCase(const QString& text) const {
    return text.toLower();
}

QString I18nManager::toUpperCase(const QString& text) const {
    return text.toUpper();
}

void I18nManager::setupLanguages() {
    m_languages = LanguageDataLoader::loadLanguageData();
}

void I18nManager::loadLanguageData() {
    if (m_settings) {
        m_currentLanguage = m_settings->value("language", m_currentLanguage).toString();
    }
}

void I18nManager::saveLanguageData() {
    if (m_settings) {
        m_settings->setValue("language", m_currentLanguage);
        m_settings->sync();
    }
}

QString I18nManager::findTranslationFile(const QString& languageCode) const {
    // Look for translation files in standard locations
    QStringList searchPaths;
    searchPaths << ":/translations"
                << QCoreApplication::applicationDirPath() + "/translations"
                << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations";
    
    for (const QString& path : searchPaths) {
        QString translationFile = path + "/disksense_" + languageCode + ".qm";
        if (QFile::exists(translationFile)) {
            return translationFile;
        }
    }
    
    return QString();
}

// LanguageDataLoader implementation
QMap<QString, I18nManager::LanguageInfo> LanguageDataLoader::loadLanguageData() {
    if (!s_initialized) {
        initializeLanguageData();
    }
    return s_languageData;
}

QString LanguageDataLoader::getLanguageName(const QString& code) {
    if (!s_initialized) {
        initializeLanguageData();
    }
    return s_languageData.value(code).name;
}

QString LanguageDataLoader::getNativeLanguageName(const QString& code) {
    if (!s_initialized) {
        initializeLanguageData();
    }
    return s_languageData.value(code).nativeName;
}

bool LanguageDataLoader::isRightToLeftLanguage(const QString& code) {
    if (!s_initialized) {
        initializeLanguageData();
    }
    return s_languageData.value(code).isRTL;
}

void LanguageDataLoader::initializeLanguageData() {
    if (s_initialized) {
        return;
    }
    
    // Define supported languages
    LanguageInfo english;
    english.code = "en";
    english.name = "English";
    english.nativeName = "English";
    english.isRTL = false;
    s_languageData["en"] = english;
    
    LanguageInfo spanish;
    spanish.code = "es";
    spanish.name = "Spanish";
    spanish.nativeName = "Español";
    spanish.isRTL = false;
    s_languageData["es"] = spanish;
    
    LanguageInfo french;
    french.code = "fr";
    french.name = "French";
    french.nativeName = "Français";
    french.isRTL = false;
    s_languageData["fr"] = french;
    
    LanguageInfo german;
    german.code = "de";
    german.name = "German";
    german.nativeName = "Deutsch";
    german.isRTL = false;
    s_languageData["de"] = german;
    
    LanguageInfo italian;
    italian.code = "it";
    italian.name = "Italian";
    italian.nativeName = "Italiano";
    italian.isRTL = false;
    s_languageData["it"] = italian;
    
    LanguageInfo portuguese;
    portuguese.code = "pt";
    portuguese.name = "Portuguese";
    portuguese.nativeName = "Português";
    portuguese.isRTL = false;
    s_languageData["pt"] = portuguese;
    
    LanguageInfo russian;
    russian.code = "ru";
    russian.name = "Russian";
    russian.nativeName = "Русский";
    russian.isRTL = false;
    s_languageData["ru"] = russian;
    
    LanguageInfo chinese;
    chinese.code = "zh";
    chinese.name = "Chinese";
    chinese.nativeName = "中文";
    chinese.isRTL = false;
    s_languageData["zh"] = chinese;
    
    LanguageInfo japanese;
    japanese.code = "ja";
    japanese.name = "Japanese";
    japanese.nativeName = "日本語";
    japanese.isRTL = false;
    s_languageData["ja"] = japanese;
    
    LanguageInfo arabic;
    arabic.code = "ar";
    arabic.name = "Arabic";
    arabic.nativeName = "العربية";
    arabic.isRTL = true;
    s_languageData["ar"] = arabic;
    
    s_initialized = true;
}