#include "settingsdialog.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_themeCombo(nullptr)
    , m_languageCombo(nullptr)
    , m_defaultPathEdit(nullptr)
    , m_browsePathButton(nullptr)
    , m_threadCountSpin(nullptr)
    , m_memoryLimitSpin(nullptr)
    , m_colorSchemeCombo(nullptr)
    , m_showFileIconsCheck(nullptr)
    , m_treemapAlgorithmCombo(nullptr)
    , m_exclusionsList(nullptr)
    , m_exclusionEdit(nullptr)
    , m_addExclusionButton(nullptr)
    , m_removeExclusionButton(nullptr)
    , m_fileTypesList(nullptr)
    , m_fileTypeEdit(nullptr)
    , m_addFileTypeButton(nullptr)
    , m_removeFileTypeButton(nullptr)
    , m_minFileSizeSpin(nullptr)
    , m_maxFileSizeSpin(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_defaultsButton(nullptr)
    , m_settings(nullptr)
{
    setupUI();
    connectSignals();
    
    // Initialize settings
    m_settings = new QSettings("DiskSense", "DiskSense64");
    loadSettings();
    
    // Set dialog properties
    setWindowTitle("Settings");
    setMinimumSize(600, 400);
    resize(700, 500);
}

SettingsDialog::~SettingsDialog() {
    delete m_settings;
}

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    QTabWidget* tabWidget = new QTabWidget();
    
    // User preferences tab
    QWidget* userTab = new QWidget();
    QFormLayout* userLayout = new QFormLayout(userTab);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems(QStringList() << "Light" << "Dark" << "System");
    userLayout->addRow("Theme:", m_themeCombo);
    
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems(QStringList() << "English" << "Spanish" << "French" << "German");
    userLayout->addRow("Language:", m_languageCombo);
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_defaultPathEdit = new QLineEdit();
    m_browsePathButton = new QPushButton("Browse...");
    pathLayout->addWidget(m_defaultPathEdit);
    pathLayout->addWidget(m_browsePathButton);
    userLayout->addRow("Default Path:", pathLayout);
    
    tabWidget->addTab(userTab, "User Preferences");
    
    // Performance settings tab
    QWidget* perfTab = new QWidget();
    QFormLayout* perfLayout = new QFormLayout(perfTab);
    
    m_threadCountSpin = new QSpinBox();
    m_threadCountSpin->setRange(1, 64);
    m_threadCountSpin->setValue(QThread::idealThreadCount());
    perfLayout->addRow("Thread Count:", m_threadCountSpin);
    
    m_memoryLimitSpin = new QSpinBox();
    m_memoryLimitSpin->setRange(256, 10240);
    m_memoryLimitSpin->setSingleStep(256);
    m_memoryLimitSpin->setValue(2048);
    m_memoryLimitSpin->setSuffix(" MB");
    perfLayout->addRow("Memory Limit:", m_memoryLimitSpin);
    
    tabWidget->addTab(perfTab, "Performance");
    
    // Visualization options tab
    QWidget* vizTab = new QWidget();
    QFormLayout* vizLayout = new QFormLayout(vizTab);
    
    m_colorSchemeCombo = new QComboBox();
    m_colorSchemeCombo->addItems(QStringList() << "Default" << "Rainbow" << "Heatmap" << "Monochrome");
    vizLayout->addRow("Color Scheme:", m_colorSchemeCombo);
    
    m_showFileIconsCheck = new QCheckBox("Show file icons in visualization");
    m_showFileIconsCheck->setChecked(true);
    vizLayout->addRow("", m_showFileIconsCheck);
    
    m_treemapAlgorithmCombo = new QComboBox();
    m_treemapAlgorithmCombo->addItems(QStringList() << "Squarified" << "Slice and Dice" << "Strip");
    vizLayout->addRow("Treemap Algorithm:", m_treemapAlgorithmCombo);
    
    tabWidget->addTab(vizTab, "Visualization");
    
    // Scan exclusions tab
    QWidget* exclusionsTab = new QWidget();
    QVBoxLayout* exclusionsLayout = new QVBoxLayout(exclusionsTab);
    
    m_exclusionsList = new QListWidget();
    exclusionsLayout->addWidget(new QLabel("Excluded Paths:"));
    exclusionsLayout->addWidget(m_exclusionsList);
    
    QHBoxLayout* exclusionButtonsLayout = new QHBoxLayout();
    m_exclusionEdit = new QLineEdit();
    m_addExclusionButton = new QPushButton("Add");
    m_removeExclusionButton = new QPushButton("Remove");
    exclusionButtonsLayout->addWidget(m_exclusionEdit);
    exclusionButtonsLayout->addWidget(m_addExclusionButton);
    exclusionButtonsLayout->addWidget(m_removeExclusionButton);
    exclusionsLayout->addLayout(exclusionButtonsLayout);
    
    tabWidget->addTab(exclusionsTab, "Exclusions");
    
    // Filter settings tab
    QWidget* filterTab = new QWidget();
    QVBoxLayout* filterLayout = new QVBoxLayout(filterTab);
    
    m_fileTypesList = new QListWidget();
    filterLayout->addWidget(new QLabel("File Types:"));
    filterLayout->addWidget(m_fileTypesList);
    
    QHBoxLayout* fileTypeButtonsLayout = new QHBoxLayout();
    m_fileTypeEdit = new QLineEdit();
    m_addFileTypeButton = new QPushButton("Add");
    m_removeFileTypeButton = new QPushButton("Remove");
    fileTypeButtonsLayout->addWidget(m_fileTypeEdit);
    fileTypeButtonsLayout->addWidget(m_addFileTypeButton);
    fileTypeButtonsLayout->addWidget(m_removeFileTypeButton);
    filterLayout->addLayout(fileTypeButtonsLayout);
    
    QWidget* sizeWidget = new QWidget();
    QHBoxLayout* sizeLayout = new QHBoxLayout(sizeWidget);
    m_minFileSizeSpin = new QSpinBox();
    m_minFileSizeSpin->setRange(0, 1000000);
    m_minFileSizeSpin->setValue(0);
    m_minFileSizeSpin->setSuffix(" KB");
    m_maxFileSizeSpin = new QSpinBox();
    m_maxFileSizeSpin->setRange(0, 1000000);
    m_maxFileSizeSpin->setValue(0);
    m_maxFileSizeSpin->setSuffix(" KB");
    m_maxFileSizeSpin->setSpecialValueText("No limit");
    sizeLayout->addWidget(new QLabel("Min File Size:"));
    sizeLayout->addWidget(m_minFileSizeSpin);
    sizeLayout->addWidget(new QLabel("Max File Size:"));
    sizeLayout->addWidget(m_maxFileSizeSpin);
    filterLayout->addWidget(sizeWidget);
    
    tabWidget->addTab(filterTab, "Filters");
    
    // Add tab widget to main layout
    mainLayout->addWidget(tabWidget);
    
    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_defaultsButton = new QPushButton("Restore Defaults");
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    m_applyButton = new QPushButton("Apply");
    
    buttonLayout->addWidget(m_defaultsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Set default button
    m_okButton->setDefault(true);
}

void SettingsDialog::connectSignals() {
    connect(m_browsePathButton, &QPushButton::clicked, 
            this, &SettingsDialog::onBrowseDefaultPath);
    connect(m_addExclusionButton, &QPushButton::clicked, 
            this, &SettingsDialog::onAddExclusion);
    connect(m_removeExclusionButton, &QPushButton::clicked, 
            this, &SettingsDialog::onRemoveExclusion);
    connect(m_addFileTypeButton, &QPushButton::clicked, 
            this, &SettingsDialog::onAddFileType);
    connect(m_removeFileTypeButton, &QPushButton::clicked, 
            this, &SettingsDialog::onRemoveFileType);
    connect(m_colorSchemeCombo, &QComboBox::currentTextChanged, 
            this, &SettingsDialog::onColorSchemeChanged);
    
    connect(m_okButton, &QPushButton::clicked, [this]() {
        saveSettings();
        accept();
    });
    
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(m_defaultsButton, &QPushButton::clicked, this, &SettingsDialog::restoreDefaults);
}

// User preferences
QString SettingsDialog::theme() const {
    return m_themeCombo->currentText();
}

void SettingsDialog::setTheme(const QString& theme) {
    int index = m_themeCombo->findText(theme);
    if (index >= 0) {
        m_themeCombo->setCurrentIndex(index);
    }
}

QString SettingsDialog::language() const {
    return m_languageCombo->currentText();
}

void SettingsDialog::setLanguage(const QString& language) {
    int index = m_languageCombo->findText(language);
    if (index >= 0) {
        m_languageCombo->setCurrentIndex(index);
    }
}

QString SettingsDialog::defaultPath() const {
    return m_defaultPathEdit->text();
}

void SettingsDialog::setDefaultPath(const QString& path) {
    m_defaultPathEdit->setText(path);
}

// Performance settings
int SettingsDialog::threadCount() const {
    return m_threadCountSpin->value();
}

void SettingsDialog::setThreadCount(int count) {
    m_threadCountSpin->setValue(count);
}

int SettingsDialog::memoryLimit() const {
    return m_memoryLimitSpin->value();
}

void SettingsDialog::setMemoryLimit(int limit) {
    m_memoryLimitSpin->setValue(limit);
}

// Visualization options
QString SettingsDialog::colorScheme() const {
    return m_colorSchemeCombo->currentText();
}

void SettingsDialog::setColorScheme(const QString& scheme) {
    int index = m_colorSchemeCombo->findText(scheme);
    if (index >= 0) {
        m_colorSchemeCombo->setCurrentIndex(index);
    }
}

bool SettingsDialog::showFileIcons() const {
    return m_showFileIconsCheck->isChecked();
}

void SettingsDialog::setShowFileIcons(bool show) {
    m_showFileIconsCheck->setChecked(show);
}

int SettingsDialog::treemapAlgorithm() const {
    return m_treemapAlgorithmCombo->currentIndex();
}

void SettingsDialog::setTreemapAlgorithm(int algorithm) {
    m_treemapAlgorithmCombo->setCurrentIndex(algorithm);
}

// Scan exclusions
QStringList SettingsDialog::exclusions() const {
    QStringList result;
    for (int i = 0; i < m_exclusionsList->count(); ++i) {
        result.append(m_exclusionsList->item(i)->text());
    }
    return result;
}

void SettingsDialog::setExclusions(const QStringList& exclusions) {
    m_exclusionsList->clear();
    m_exclusionsList->addItems(exclusions);
}

// Filter settings
QStringList SettingsDialog::fileTypes() const {
    QStringList result;
    for (int i = 0; i < m_fileTypesList->count(); ++i) {
        result.append(m_fileTypesList->item(i)->text());
    }
    return result;
}

void SettingsDialog::setFileTypes(const QStringList& types) {
    m_fileTypesList->clear();
    m_fileTypesList->addItems(types);
}

quint64 SettingsDialog::minFileSize() const {
    return m_minFileSizeSpin->value() * 1024; // Convert KB to bytes
}

void SettingsDialog::setMinFileSize(quint64 size) {
    m_minFileSizeSpin->setValue(size / 1024); // Convert bytes to KB
}

quint64 SettingsDialog::maxFileSize() const {
    int value = m_maxFileSizeSpin->value();
    if (value == 0) {
        return 0; // No limit
    }
    return value * 1024; // Convert KB to bytes
}

void SettingsDialog::setMaxFileSize(quint64 size) {
    if (size == 0) {
        m_maxFileSizeSpin->setValue(0); // No limit
    } else {
        m_maxFileSizeSpin->setValue(size / 1024); // Convert bytes to KB
    }
}

void SettingsDialog::loadSettings() {
    // User preferences
    setTheme(m_settings->value("theme", "Light").toString());
    setLanguage(m_settings->value("language", "English").toString());
    setDefaultPath(m_settings->value("defaultPath", QDir::homePath()).toString());
    
    // Performance settings
    setThreadCount(m_settings->value("threadCount", QThread::idealThreadCount()).toInt());
    setMemoryLimit(m_settings->value("memoryLimit", 2048).toInt());
    
    // Visualization options
    setColorScheme(m_settings->value("colorScheme", "Default").toString());
    setShowFileIcons(m_settings->value("showFileIcons", true).toBool());
    setTreemapAlgorithm(m_settings->value("treemapAlgorithm", 0).toInt());
    
    // Scan exclusions
    setExclusions(m_settings->value("exclusions", QStringList()).toStringList());
    
    // Filter settings
    setFileTypes(m_settings->value("fileTypes", QStringList() << "*.jpg" << "*.png" << "*.mp3" << "*.mp4").toStringList());
    setMinFileSize(m_settings->value("minFileSize", 0).toULongLong());
    setMaxFileSize(m_settings->value("maxFileSize", 0).toULongLong());
}

void SettingsDialog::saveSettings() {
    // User preferences
    m_settings->setValue("theme", theme());
    m_settings->setValue("language", language());
    m_settings->setValue("defaultPath", defaultPath());
    
    // Performance settings
    m_settings->setValue("threadCount", threadCount());
    m_settings->setValue("memoryLimit", memoryLimit());
    
    // Visualization options
    m_settings->setValue("colorScheme", colorScheme());
    m_settings->setValue("showFileIcons", showFileIcons());
    m_settings->setValue("treemapAlgorithm", treemapAlgorithm());
    
    // Scan exclusions
    m_settings->setValue("exclusions", exclusions());
    
    // Filter settings
    m_settings->setValue("fileTypes", fileTypes());
    m_settings->setValue("minFileSize", minFileSize());
    m_settings->setValue("maxFileSize", maxFileSize());
    
    m_settings->sync();
}

void SettingsDialog::restoreDefaults() {
    // User preferences
    setTheme("Light");
    setLanguage("English");
    setDefaultPath(QDir::homePath());
    
    // Performance settings
    setThreadCount(QThread::idealThreadCount());
    setMemoryLimit(2048);
    
    // Visualization options
    setColorScheme("Default");
    setShowFileIcons(true);
    setTreemapAlgorithm(0);
    
    // Scan exclusions
    setExclusions(QStringList());
    
    // Filter settings
    setFileTypes(QStringList() << "*.jpg" << "*.png" << "*.mp3" << "*.mp4");
    setMinFileSize(0);
    setMaxFileSize(0);
}

void SettingsDialog::onBrowseDefaultPath() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Default Directory", defaultPath());
    if (!dir.isEmpty()) {
        m_defaultPathEdit->setText(dir);
    }
}

void SettingsDialog::onAddExclusion() {
    QString exclusion = m_exclusionEdit->text().trimmed();
    if (!exclusion.isEmpty()) {
        m_exclusionsList->addItem(exclusion);
        m_exclusionEdit->clear();
    }
}

void SettingsDialog::onRemoveExclusion() {
    QListWidgetItem* item = m_exclusionsList->currentItem();
    if (item) {
        delete item;
    }
}

void SettingsDialog::onAddFileType() {
    QString fileType = m_fileTypeEdit->text().trimmed();
    if (!fileType.isEmpty()) {
        m_fileTypesList->addItem(fileType);
        m_fileTypeEdit->clear();
    }
}

void SettingsDialog::onRemoveFileType() {
    QListWidgetItem* item = m_fileTypesList->currentItem();
    if (item) {
        delete item;
    }
}

void SettingsDialog::onColorSchemeChanged(const QString& scheme) {
    // In a real implementation, this would update the preview
    Q_UNUSED(scheme);
}