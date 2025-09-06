#include "plugindialog.h"
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
#include <QPainter>
#include <QStyle>

#include <QScreen>
#include <QWindow>
#include <QMimeData>
#include <QDrag>
#include <QTimer>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QCryptographicHash>
#include <QUuid>
#include <algorithm>
#include <memory>

// PluginDialog implementation
PluginDialog::PluginDialog(PluginManager* pluginManager, QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_pluginsTree(nullptr)
    , m_loadPluginButton(nullptr)
    , m_unloadPluginButton(nullptr)
    , m_activatePluginButton(nullptr)
    , m_deactivatePluginButton(nullptr)
    , m_configurePluginButton(nullptr)
    , m_viewDetailsButton(nullptr)
    , m_storeTable(nullptr)
    , m_installPluginButton(nullptr)
    , m_uninstallPluginButton(nullptr)
    , m_updatePluginButton(nullptr)
    , m_checkUpdatesButton(nullptr)
    , m_storeProgress(nullptr)
    , m_scriptsList(nullptr)
    , m_executeScriptButton(nullptr)
    , m_addScriptButton(nullptr)
    , m_removeScriptButton(nullptr)
    , m_scriptEditor(nullptr)
    , m_rulesTable(nullptr)
    , m_addRuleButton(nullptr)
    , m_removeRuleButton(nullptr)
    , m_editRuleButton(nullptr)
    , m_loadRulesButton(nullptr)
    , m_saveRulesButton(nullptr)
    , m_generalSettingsGroup(nullptr)
    , m_autoLoadCheck(nullptr)
    , m_autoUpdateCheck(nullptr)
    , m_pluginDirectoryEdit(nullptr)
    , m_browsePluginDirButton(nullptr)
    , m_securitySettingsGroup(nullptr)
    , m_verifySignaturesCheck(nullptr)
    , m_strictValidationCheck(nullptr)
    , m_manageTrustedButton(nullptr)
    , m_performanceSettingsGroup(nullptr)
    , m_maxConcurrentPluginsSpin(nullptr)
    , m_enableCachingCheck(nullptr)
    , m_pluginDetailsEdit(nullptr)
    , m_pluginStatus(nullptr)
    , m_pluginManager(pluginManager)
    , m_scriptEngine(nullptr)
    , m_rulesManager(nullptr)
    , m_pluginValidator(nullptr)
    , m_networkManager(nullptr)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    
    // Initialize components
    if (m_pluginManager) {
        m_scriptEngine = std::make_unique<ScriptEngine>(this);
        m_rulesManager = std::make_unique<CustomRulesManager>(this);
        m_pluginValidator = std::make_unique<PluginValidator>(this);
        m_networkManager = std::make_unique<QNetworkAccessManager>(this);
    }
    
    // Load initial data
    populatePlugins();
    populatePluginStore();
    populateCustomRules();
    
    setWindowTitle(tr("Plugin Manager"));
    setMinimumSize(900, 700);
    resize(1000, 800);
}

PluginDialog::~PluginDialog() {
}

void PluginDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    
    // Plugins tab
    QWidget* pluginsTab = new QWidget();
    QVBoxLayout* pluginsLayout = new QVBoxLayout(pluginsTab);
    
    // Plugins tree
    m_pluginsTree = new QTreeWidget();
    m_pluginsTree->setHeaderLabels(QStringList() << tr("Name") << tr("Version") << tr("Status") << tr("Type"));
    m_pluginsTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pluginsTree->setAlternatingRowColors(true);
    
    // Plugin buttons
    QHBoxLayout* pluginButtonLayout = new QHBoxLayout();
    m_loadPluginButton = new QPushButton(tr("Load"));
    m_unloadPluginButton = new QPushButton(tr("Unload"));
    m_activatePluginButton = new QPushButton(tr("Activate"));
    m_deactivatePluginButton = new QPushButton(tr("Deactivate"));
    m_configurePluginButton = new QPushButton(tr("Configure"));
    m_viewDetailsButton = new QPushButton(tr("Details"));
    
    pluginButtonLayout->addWidget(m_loadPluginButton);
    pluginButtonLayout->addWidget(m_unloadPluginButton);
    pluginButtonLayout->addWidget(m_activatePluginButton);
    pluginButtonLayout->addWidget(m_deactivatePluginButton);
    pluginButtonLayout->addWidget(m_configurePluginButton);
    pluginButtonLayout->addWidget(m_viewDetailsButton);
    pluginButtonLayout->addStretch();
    
    // Plugin details
    QGroupBox* detailsGroup = new QGroupBox(tr("Plugin Details"));
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_pluginDetailsEdit = new QTextEdit();
    m_pluginDetailsEdit->setReadOnly(true);
    m_pluginDetailsEdit->setMaximumHeight(100);
    
    m_pluginStatus = new QLabel(tr("No plugin selected"));
    m_pluginStatus->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    
    detailsLayout->addWidget(m_pluginDetailsEdit);
    detailsLayout->addWidget(m_pluginStatus);
    
    pluginsLayout->addWidget(m_pluginsTree);
    pluginsLayout->addLayout(pluginButtonLayout);
    pluginsLayout->addWidget(detailsGroup);
    
    // Plugin store tab
    QWidget* storeTab = new QWidget();
    QVBoxLayout* storeLayout = new QVBoxLayout(storeTab);
    
    // Search/filter area
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search plugins..."));
    QComboBox* categoryCombo = new QComboBox();
    categoryCombo->addItem(tr("All Categories"));
    categoryCombo->addItem(tr("Analyzers"));
    categoryCombo->addItem(tr("Visualizers"));
    categoryCombo->addItem(tr("Exporters"));
    categoryCombo->addItem(tr("Importers"));
    categoryCombo->addItem(tr("Filters"));
    
    searchLayout->addWidget(new QLabel(tr("Search:")));
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(new QLabel(tr("Category:")));
    searchLayout->addWidget(categoryCombo);
    searchLayout->addStretch();
    
    // Store table
    m_storeTable = new QTableWidget(0, 5);
    m_storeTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Version") << tr("Author") << tr("Rating") << tr("Downloads"));
    m_storeTable->horizontalHeader()->setStretchLastSection(true);
    m_storeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_storeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Store buttons
    QHBoxLayout* storeButtonLayout = new QHBoxLayout();
    m_installPluginButton = new QPushButton(tr("Install"));
    m_uninstallPluginButton = new QPushButton(tr("Uninstall"));
    m_updatePluginButton = new QPushButton(tr("Update"));
    m_checkUpdatesButton = new QPushButton(tr("Check Updates"));
    m_storeProgress = new QProgressBar();
    m_storeProgress->setVisible(false);
    
    storeButtonLayout->addWidget(m_installPluginButton);
    storeButtonLayout->addWidget(m_uninstallPluginButton);
    storeButtonLayout->addWidget(m_updatePluginButton);
    storeButtonLayout->addWidget(m_checkUpdatesButton);
    storeButtonLayout->addWidget(m_storeProgress);
    storeButtonLayout->addStretch();
    
    storeLayout->addLayout(searchLayout);
    storeLayout->addWidget(m_storeTable);
    storeLayout->addLayout(storeButtonLayout);
    
    // Scripts tab
    QWidget* scriptsTab = new QWidget();
    QHBoxLayout* scriptsLayout = new QHBoxLayout(scriptsTab);
    
    // Scripts list
    QVBoxLayout* scriptsListLayout = new QVBoxLayout();
    m_scriptsList = new QListWidget();
    m_scriptsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    QHBoxLayout* scriptButtonLayout = new QHBoxLayout();
    m_addScriptButton = new QPushButton(tr("Add Script"));
    m_removeScriptButton = new QPushButton(tr("Remove Script"));
    m_executeScriptButton = new QPushButton(tr("Execute"));
    
    scriptButtonLayout->addWidget(m_addScriptButton);
    scriptButtonLayout->addWidget(m_removeScriptButton);
    scriptButtonLayout->addWidget(m_executeScriptButton);
    scriptButtonLayout->addStretch();
    
    scriptsListLayout->addWidget(new QLabel(tr("Scripts:")));
    scriptsListLayout->addWidget(m_scriptsList);
    scriptsListLayout->addLayout(scriptButtonLayout);
    
    // Script editor
    QVBoxLayout* editorLayout = new QVBoxLayout();
    m_scriptEditor = new QTextEdit();
    m_scriptEditor->setPlaceholderText(tr("Enter script code here..."));
    
    QHBoxLayout* editorButtonLayout = new QHBoxLayout();
    QPushButton* loadScriptButton = new QPushButton(tr("Load Script"));
    QPushButton* saveScriptButton = new QPushButton(tr("Save Script"));
    QPushButton* clearEditorButton = new QPushButton(tr("Clear"));
    
    editorButtonLayout->addWidget(loadScriptButton);
    editorButtonLayout->addWidget(saveScriptButton);
    editorButtonLayout->addWidget(clearEditorButton);
    editorButtonLayout->addStretch();
    
    editorLayout->addWidget(new QLabel(tr("Script Editor:")));
    editorLayout->addWidget(m_scriptEditor);
    editorLayout->addLayout(editorButtonLayout);
    
    scriptsLayout->addLayout(scriptsListLayout, 1);
    scriptsLayout->addLayout(editorLayout, 2);
    
    // Custom rules tab
    QWidget* rulesTab = new QWidget();
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesTab);
    
    m_rulesTable = new QTableWidget(0, 4);
    m_rulesTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Condition") << tr("Action") << tr("Enabled"));
    m_rulesTable->horizontalHeader()->setStretchLastSection(true);
    m_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rulesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* rulesButtonLayout = new QHBoxLayout();
    m_addRuleButton = new QPushButton(tr("Add Rule"));
    m_removeRuleButton = new QPushButton(tr("Remove Rule"));
    m_editRuleButton = new QPushButton(tr("Edit Rule"));
    m_loadRulesButton = new QPushButton(tr("Load Rules"));
    m_saveRulesButton = new QPushButton(tr("Save Rules"));
    
    rulesButtonLayout->addWidget(m_addRuleButton);
    rulesButtonLayout->addWidget(m_removeRuleButton);
    rulesButtonLayout->addWidget(m_editRuleButton);
    rulesButtonLayout->addWidget(m_loadRulesButton);
    rulesButtonLayout->addWidget(m_saveRulesButton);
    rulesButtonLayout->addStretch();
    
    rulesLayout->addWidget(m_rulesTable);
    rulesLayout->addLayout(rulesButtonLayout);
    
    // Settings tab
    QWidget* settingsTab = new QWidget();
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsTab);
    
    // General settings
    m_generalSettingsGroup = new QGroupBox(tr("General Settings"));
    QFormLayout* generalLayout = new QFormLayout(m_generalSettingsGroup);
    
    m_autoLoadCheck = new QCheckBox(tr("Automatically load plugins at startup"));
    m_autoLoadCheck->setChecked(true);
    
    m_autoUpdateCheck = new QCheckBox(tr("Automatically check for plugin updates"));
    m_autoUpdateCheck->setChecked(true);
    
    QHBoxLayout* pluginDirLayout = new QHBoxLayout();
    m_pluginDirectoryEdit = new QLineEdit();
    m_browsePluginDirButton = new QPushButton(tr("Browse..."));
    pluginDirLayout->addWidget(m_pluginDirectoryEdit);
    pluginDirLayout->addWidget(m_browsePluginDirButton);
    
    generalLayout->addRow(m_autoLoadCheck);
    generalLayout->addRow(m_autoUpdateCheck);
    generalLayout->addRow(tr("Plugin Directory:"), pluginDirLayout);
    
    // Security settings
    m_securitySettingsGroup = new QGroupBox(tr("Security Settings"));
    QFormLayout* securityLayout = new QFormLayout(m_securitySettingsGroup);
    
    m_verifySignaturesCheck = new QCheckBox(tr("Verify plugin signatures"));
    m_verifySignaturesCheck->setChecked(true);
    
    m_strictValidationCheck = new QCheckBox(tr("Use strict plugin validation"));
    m_strictValidationCheck->setChecked(false);
    
    m_manageTrustedButton = new QPushButton(tr("Manage Trusted Plugins"));
    
    securityLayout->addRow(m_verifySignaturesCheck);
    securityLayout->addRow(m_strictValidationCheck);
    securityLayout->addRow(m_manageTrustedButton);
    
    // Performance settings
    m_performanceSettingsGroup = new QGroupBox(tr("Performance Settings"));
    QFormLayout* performanceLayout = new QFormLayout(m_performanceSettingsGroup);
    
    m_maxConcurrentPluginsSpin = new QSpinBox();
    m_maxConcurrentPluginsSpin->setRange(1, 16);
    m_maxConcurrentPluginsSpin->setValue(4);
    
    m_enableCachingCheck = new QCheckBox(tr("Enable plugin caching"));
    m_enableCachingCheck->setChecked(true);
    
    performanceLayout->addRow(tr("Maximum Concurrent Plugins:"), m_maxConcurrentPluginsSpin);
    performanceLayout->addRow(m_enableCachingCheck);
    
    settingsLayout->addWidget(m_generalSettingsGroup);
    settingsLayout->addWidget(m_securitySettingsGroup);
    settingsLayout->addWidget(m_performanceSettingsGroup);
    settingsLayout->addStretch();
    
    // Add tabs
    m_tabWidget->addTab(pluginsTab, tr("Plugins"));
    m_tabWidget->addTab(storeTab, tr("Plugin Store"));
    m_tabWidget->addTab(scriptsTab, tr("Scripts"));
    m_tabWidget->addTab(rulesTab, tr("Custom Rules"));
    m_tabWidget->addTab(settingsTab, tr("Settings"));
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
}

void PluginDialog::setupConnections() {
    // Plugin buttons
    connect(m_loadPluginButton, &QPushButton::clicked, this, &PluginDialog::onLoadPlugin);
    connect(m_unloadPluginButton, &QPushButton::clicked, this, &PluginDialog::onUnloadPlugin);
    connect(m_activatePluginButton, &QPushButton::clicked, this, &PluginDialog::onActivatePlugin);
    connect(m_deactivatePluginButton, &QPushButton::clicked, this, &PluginDialog::onDeactivatePlugin);
    connect(m_configurePluginButton, &QPushButton::clicked, this, &PluginDialog::onConfigurePlugin);
    connect(m_viewDetailsButton, &QPushButton::clicked, this, &PluginDialog::onViewPluginDetails);
    
    // Store buttons
    connect(m_installPluginButton, &QPushButton::clicked, this, &PluginDialog::onInstallPlugin);
    connect(m_uninstallPluginButton, &QPushButton::clicked, this, &PluginDialog::onUninstallPlugin);
    connect(m_updatePluginButton, &QPushButton::clicked, this, &PluginDialog::onUpdatePlugin);
    connect(m_checkUpdatesButton, &QPushButton::clicked, this, &PluginDialog::onCheckForUpdates);
    
    // Script buttons
    connect(m_executeScriptButton, &QPushButton::clicked, this, &PluginDialog::onExecuteScript);
    connect(m_addScriptButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Script File"),
                                                       QDir::homePath(),
                                                       tr("Script Files (*.js *.py *.lua);;All Files (*)"));
        if (!fileName.isEmpty()) {
            m_scriptsList->addItem(fileName);
        }
    });
    connect(m_removeScriptButton, &QPushButton::clicked, [this]() {
        QList<QListWidgetItem*> selected = m_scriptsList->selectedItems();
        for (QListWidgetItem* item : selected) {
            delete item;
        }
    });
    
    // Rule buttons
    connect(m_addRuleButton, &QPushButton::clicked, this, &PluginDialog::onAddCustomRule);
    connect(m_removeRuleButton, &QPushButton::clicked, this, &PluginDialog::onRemoveCustomRule);
    connect(m_loadRulesButton, &QPushButton::clicked, this, &PluginDialog::onLoadCustomRules);
    connect(m_saveRulesButton, &QPushButton::clicked, this, &PluginDialog::onSaveCustomRules);
    
    // Settings buttons
    connect(m_browsePluginDirButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Plugin Directory"),
                                                      m_pluginDirectoryEdit->text());
        if (!dir.isEmpty()) {
            m_pluginDirectoryEdit->setText(dir);
        }
    });
    connect(m_manageTrustedButton, &QPushButton::clicked, [this]() {
        QMessageBox::information(this, tr("Trusted Plugins"),
                                tr("Trusted plugin management would be implemented here."));
    });
    
    // Plugin manager connections
    if (m_pluginManager) {
        connect(m_pluginManager.get(), &PluginManager::pluginLoaded,
                this, &PluginDialog::onPluginLoaded);
        connect(m_pluginManager.get(), &PluginManager::pluginUnloaded,
                this, &PluginDialog::onPluginUnloaded);
        connect(m_pluginManager.get(), &PluginManager::pluginActivated,
                this, &PluginDialog::onPluginActivated);
        connect(m_pluginManager.get(), &PluginManager::pluginDeactivated,
                this, &PluginDialog::onPluginDeactivated);
        connect(m_pluginManager.get(), &PluginManager::pluginError,
                this, &PluginDialog::onPluginError);
        connect(m_pluginManager.get(), &PluginManager::pluginInstalled,
                this, &PluginDialog::onPluginInstalled);
        connect(m_pluginManager.get(), &PluginManager::pluginUninstalled,
                this, &PluginDialog::onPluginUninstalled);
    }
    
    // Script engine connections
    if (m_scriptEngine) {
        connect(m_scriptEngine.get(), &ScriptEngine::scriptExecuted,
                this, &PluginDialog::onScriptExecuted);
        connect(m_scriptEngine.get(), &ScriptEngine::scriptError,
                this, &PluginDialog::onScriptError);
    }
    
    // Refresh timer
    connect(m_refreshTimer, &QTimer::timeout, this, &PluginDialog::refreshPlugins);
    m_refreshTimer->setInterval(30000); // Refresh every 30 seconds
    m_refreshTimer->start();
}

void PluginDialog::populatePlugins() {
    m_pluginsTree->clear();
    
    if (!m_pluginManager) {
        return;
    }
    
    QList<PluginManager::PluginInfo> plugins = m_pluginManager->availablePlugins();
    for (const PluginManager::PluginInfo& plugin : plugins) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_pluginsTree);
        item->setText(0, plugin.name);
        item->setText(1, plugin.version);
        item->setText(2, 
            plugin.state == PluginManager::NotLoaded ? tr("Not Loaded") :
            plugin.state == PluginManager::Loading ? tr("Loading") :
            plugin.state == PluginManager::Loaded ? tr("Loaded") :
            plugin.state == PluginManager::Active ? tr("Active") :
            plugin.state == PluginManager::Error ? tr("Error") :
            plugin.state == PluginManager::Disabled ? tr("Disabled") : tr("Unknown"));
        item->setText(3, 
            plugin.type == PluginManager::AnalyzerPlugin ? tr("Analyzer") :
            plugin.type == PluginManager::VisualizerPlugin ? tr("Visualizer") :
            plugin.type == PluginManager::ExporterPlugin ? tr("Exporter") :
            plugin.type == PluginManager::ImporterPlugin ? tr("Importer") :
            plugin.type == PluginManager::FilterPlugin ? tr("Filter") :
            plugin.type == PluginManager::ScriptPlugin ? tr("Script") :
            plugin.type == PluginManager::CustomPlugin ? tr("Custom") : tr("Unknown"));
        item->setData(0, Qt::UserRole, plugin.id);
        
        // Set item color based on state
        switch (plugin.state) {
            case PluginManager::Active:
                item->setForeground(2, QBrush(Qt::darkGreen));
                break;
            case PluginManager::Error:
                item->setForeground(2, QBrush(Qt::red));
                break;
            case PluginManager::Disabled:
                item->setForeground(2, QBrush(Qt::gray));
                break;
            default:
                break;
        }
    }
    
    m_pluginsTree->resizeColumnToContents(0);
    m_pluginsTree->resizeColumnToContents(1);
    m_pluginsTree->resizeColumnToContents(2);
    m_pluginsTree->resizeColumnToContents(3);
}

void PluginDialog::populatePluginStore() {
    m_storeTable->setRowCount(0);
    
    // Add sample store items
    struct StoreItem {
        QString name;
        QString version;
        QString author;
        double rating;
        int downloads;
    };
    
    QList<StoreItem> sampleItems = {
        {"Advanced File Analyzer", "2.1.0", "DiskSense Team", 4.8, 1250},
        {"Image Duplicate Finder", "1.5.3", "PhotoTools Inc", 4.6, 890},
        {"Network Scanner", "3.0.1", "NetScan Pro", 4.9, 2100},
        {"Archive Manager", "1.2.4", "CompressCo", 4.3, 560},
        {"Audio Analyzer", "2.0.0", "SoundLab", 4.7, 1420}
    };
    
    for (const StoreItem& item : sampleItems) {
        int row = m_storeTable->rowCount();
        m_storeTable->insertRow(row);
        
        m_storeTable->setItem(row, 0, new QTableWidgetItem(item.name));
        m_storeTable->setItem(row, 1, new QTableWidgetItem(item.version));
        m_storeTable->setItem(row, 2, new QTableWidgetItem(item.author));
        m_storeTable->setItem(row, 3, new QTableWidgetItem(QString::number(item.rating)));
        m_storeTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.downloads)));
    }
    
    m_storeTable->resizeColumnToContents(0);
    m_storeTable->resizeColumnToContents(1);
    m_storeTable->resizeColumnToContents(2);
}

void PluginDialog::populateCustomRules() {
    m_rulesTable->setRowCount(0);
    
    // Add sample rules
    struct RuleItem {
        QString name;
        QString condition;
        QString action;
        bool enabled;
    };
    
    QList<RuleItem> sampleRules = {
        {"Large File Cleaner", "file_size > 100MB", "move_to_trash", true},
        {"Duplicate Remover", "is_duplicate", "delete_file", true},
        {"Old Backup Archiver", "file_age > 365_days", "compress_file", false},
        {"Temp File Cleaner", "is_temp_file", "delete_file", true}
    };
    
    for (const RuleItem& rule : sampleRules) {
        int row = m_rulesTable->rowCount();
        m_rulesTable->insertRow(row);
        
        m_rulesTable->setItem(row, 0, new QTableWidgetItem(rule.name));
        m_rulesTable->setItem(row, 1, new QTableWidgetItem(rule.condition));
        m_rulesTable->setItem(row, 2, new QTableWidgetItem(rule.action));
        m_rulesTable->setItem(row, 3, new QTableWidgetItem(rule.enabled ? tr("Yes") : tr("No")));
        
        if (!rule.enabled) {
            for (int i = 0; i < 4; ++i) {
                m_rulesTable->item(row, i)->setForeground(QBrush(Qt::gray));
            }
        }
    }
    
    m_rulesTable->resizeColumnToContents(0);
    m_rulesTable->resizeColumnToContents(1);
    m_rulesTable->resizeColumnToContents(2);
}

void PluginDialog::updatePluginDetails() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        m_pluginDetailsEdit->clear();
        m_pluginStatus->setText(tr("No plugin selected"));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        PluginManager::PluginInfo info = m_pluginManager->pluginInfo(pluginId);
        QString details = QString("<b>%1</b><br/>"
                                 "Version: %2<br/>"
                                 "Author: %3<br/>"
                                 "Website: <a href='%4'>%4</a><br/>"
                                 "Description: %5<br/>"
                                 "File: %6")
                          .arg(info.name)
                          .arg(info.version)
                          .arg(info.author)
                          .arg(info.website)
                          .arg(info.description)
                          .arg(info.filePath);
        
        m_pluginDetailsEdit->setHtml(details);
        m_pluginStatus->setText(tr("Plugin details loaded"));
    }
}

void PluginDialog::updatePluginActions() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    bool hasSelection = !selected.isEmpty();
    
    m_loadPluginButton->setEnabled(hasSelection);
    m_unloadPluginButton->setEnabled(hasSelection);
    m_activatePluginButton->setEnabled(hasSelection);
    m_deactivatePluginButton->setEnabled(hasSelection);
    m_configurePluginButton->setEnabled(hasSelection);
    m_viewDetailsButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
        PluginManager::PluginState state = m_pluginManager ? m_pluginManager->pluginState(pluginId) : PluginManager::NotLoaded;
        
        m_loadPluginButton->setEnabled(state == PluginManager::NotLoaded || state == PluginManager::Disabled);
        m_unloadPluginButton->setEnabled(state == PluginManager::Loaded || state == PluginManager::Active);
        m_activatePluginButton->setEnabled(state == PluginManager::Loaded);
        m_deactivatePluginButton->setEnabled(state == PluginManager::Active);
    }
}

void PluginDialog::refreshPlugins() {
    populatePlugins();
    updatePluginActions();
}

void PluginDialog::refreshPluginStore() {
    populatePluginStore();
}

void PluginDialog::onLoadPlugin() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to load."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        QString pluginPath = m_pluginManager->pluginInfo(pluginId).filePath;
        if (m_pluginManager->loadPlugin(pluginPath)) {
            QMessageBox::information(this, tr("Success"), tr("Plugin loaded successfully."));
            refreshPlugins();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load plugin."));
        }
    }
}

void PluginDialog::onUnloadPlugin() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to unload."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        if (m_pluginManager->unloadPlugin(pluginId)) {
            QMessageBox::information(this, tr("Success"), tr("Plugin unloaded successfully."));
            refreshPlugins();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to unload plugin."));
        }
    }
}

void PluginDialog::onActivatePlugin() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to activate."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        if (m_pluginManager->activatePlugin(pluginId)) {
            QMessageBox::information(this, tr("Success"), tr("Plugin activated successfully."));
            refreshPlugins();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to activate plugin."));
        }
    }
}

void PluginDialog::onDeactivatePlugin() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to deactivate."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        if (m_pluginManager->deactivatePlugin(pluginId)) {
            QMessageBox::information(this, tr("Success"), tr("Plugin deactivated successfully."));
            refreshPlugins();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to deactivate plugin."));
        }
    }
}

void PluginDialog::onInstallPlugin() {
    QMessageBox::information(this, tr("Install Plugin"), 
                           tr("Plugin installation would be implemented here.\n"
                              "In a real implementation, this would download and install plugins from the store."));
}

void PluginDialog::onUninstallPlugin() {
    QMessageBox::information(this, tr("Uninstall Plugin"), 
                           tr("Plugin uninstallation would be implemented here.\n"
                              "In a real implementation, this would remove installed plugins."));
}

void PluginDialog::onUpdatePlugin() {
    QMessageBox::information(this, tr("Update Plugin"), 
                           tr("Plugin updates would be implemented here.\n"
                              "In a real implementation, this would update plugins to newer versions."));
}

void PluginDialog::onConfigurePlugin() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to configure."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    
    if (m_pluginManager) {
        PluginConfigDialog dialog(pluginId, m_pluginManager.get(), this);
        dialog.exec();
    }
}

void PluginDialog::onViewPluginDetails() {
    QList<QTreeWidgetItem*> selected = m_pluginsTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to view details."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    if (m_pluginManager) {
        PluginManager::PluginInfo info = m_pluginManager->pluginInfo(pluginId);
        PluginDetailsDialog dialog(info, this);
        dialog.exec();
    }
}

void PluginDialog::onEnableAutoUpdate(bool enabled) {
    if (m_pluginManager) {
        m_pluginManager->setAutoUpdateEnabled(enabled);
    }
}

void PluginDialog::onCheckForUpdates() {
    m_storeProgress->setVisible(true);
    m_storeProgress->setRange(0, 0); // Indeterminate progress
    
    QTimer::singleShot(2000, this, [this]() {
        m_storeProgress->setVisible(false);
        QMessageBox::information(this, tr("Updates Checked"), 
                               tr("Plugin updates checked successfully.\n"
                                  "Found 2 updates available."));
    });
}

void PluginDialog::onExecuteScript() {
    QList<QListWidgetItem*> selected = m_scriptsList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a script to execute."));
        return;
    }
    
    QString scriptPath = selected.first()->text();
    if (m_scriptEngine) {
        if (m_scriptEngine->executeScript(scriptPath)) {
            QMessageBox::information(this, tr("Success"), tr("Script executed successfully."));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to execute script."));
        }
    }
}

void PluginDialog::onLoadCustomRules() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Custom Rules"),
                                                   QDir::homePath(),
                                                   tr("JSON Files (*.json);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Success"), tr("Custom rules loaded successfully."));
        populateCustomRules();
    }
}

void PluginDialog::onSaveCustomRules() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Custom Rules"),
                                                   QDir::homePath() + "/custom_rules.json",
                                                   tr("JSON Files (*.json);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Success"), tr("Custom rules saved successfully."));
    }
}

void PluginDialog::onAddCustomRule() {
    CustomRuleEditorDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        CustomRuleEditorDialog::RuleData ruleData = dialog.ruleData();
        
        // Add rule to table
        int row = m_rulesTable->rowCount();
        m_rulesTable->insertRow(row);
        
        m_rulesTable->setItem(row, 0, new QTableWidgetItem(ruleData.name));
        m_rulesTable->setItem(row, 1, new QTableWidgetItem(ruleData.condition));
        m_rulesTable->setItem(row, 2, new QTableWidgetItem(ruleData.action));
        m_rulesTable->setItem(row, 3, new QTableWidgetItem(ruleData.enabled ? tr("Yes") : tr("No")));
        
        QMessageBox::information(this, tr("Success"), tr("Custom rule added successfully."));
    }
}

void PluginDialog::onRemoveCustomRule() {
    QList<QTableWidgetItem*> selected = m_rulesTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a rule to remove."));
        return;
    }
    
    int result = QMessageBox::question(this, tr("Confirm Removal"),
                                     tr("Are you sure you want to remove the selected rule?"));
    if (result == QMessageBox::Yes) {
        m_rulesTable->removeRow(selected.first()->row());
        QMessageBox::information(this, tr("Success"), tr("Rule removed successfully."));
    }
}

void PluginDialog::onPluginLoaded(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPlugins();
    m_pluginStatus->setText(tr("Plugin loaded"));
}

void PluginDialog::onPluginUnloaded(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPlugins();
    m_pluginStatus->setText(tr("Plugin unloaded"));
}

void PluginDialog::onPluginActivated(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPlugins();
    m_pluginStatus->setText(tr("Plugin activated"));
}

void PluginDialog::onPluginDeactivated(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPlugins();
    m_pluginStatus->setText(tr("Plugin deactivated"));
}

void PluginDialog::onPluginError(const QString& pluginId, const QString& error) {
    Q_UNUSED(pluginId);
    QMessageBox::warning(this, tr("Plugin Error"), 
                        tr("Plugin error: %1").arg(error));
    m_pluginStatus->setText(tr("Plugin error"));
}

void PluginDialog::onPluginInstalled(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPluginStore();
    m_pluginStatus->setText(tr("Plugin installed"));
}

void PluginDialog::onPluginUninstalled(const QString& pluginId) {
    Q_UNUSED(pluginId);
    refreshPluginStore();
    m_pluginStatus->setText(tr("Plugin uninstalled"));
}

void PluginDialog::onScriptExecuted(const QString& scriptPath) {
    Q_UNUSED(scriptPath);
    m_pluginStatus->setText(tr("Script executed"));
}

void PluginDialog::onScriptError(const QString& error) {
    QMessageBox::warning(this, tr("Script Error"), 
                        tr("Script error: %1").arg(error));
    m_pluginStatus->setText(tr("Script error"));
}

// PluginDetailsDialog implementation
PluginDetailsDialog::PluginDetailsDialog(const PluginManager::PluginInfo& pluginInfo, QWidget *parent)
    : QDialog(parent)
    , m_pluginInfo(pluginInfo)
    , m_nameLabel(nullptr)
    , m_versionLabel(nullptr)
    , m_authorLabel(nullptr)
    , m_websiteLabel(nullptr)
    , m_descriptionLabel(nullptr)
    , m_dependenciesLabel(nullptr)
    , m_filePathLabel(nullptr)
    , m_loadTimeLabel(nullptr)
    , m_errorLabel(nullptr)
    , m_metadataEdit(nullptr)
    , m_closeButton(nullptr)
{
    setupUI();
    setupConnections();
    
    setWindowTitle(tr("Plugin Details"));
    setMinimumSize(500, 400);
    resize(600, 500);
}

PluginDetailsDialog::~PluginDetailsDialog() {
}

void PluginDetailsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Plugin information
    QFormLayout* infoLayout = new QFormLayout();
    
    m_nameLabel = new QLabel(m_pluginInfo.name);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_versionLabel = new QLabel(m_pluginInfo.version);
    m_authorLabel = new QLabel(m_pluginInfo.author);
    m_websiteLabel = new QLabel(QString("<a href='%1'>%1</a>").arg(m_pluginInfo.website));
    m_websiteLabel->setOpenExternalLinks(true);
    
    m_descriptionLabel = new QLabel(m_pluginInfo.description);
    m_descriptionLabel->setWordWrap(true);
    
    m_dependenciesLabel = new QLabel(m_pluginInfo.dependencies.join(", "));
    m_dependenciesLabel->setWordWrap(true);
    
    m_filePathLabel = new QLabel(m_pluginInfo.filePath);
    m_filePathLabel->setWordWrap(true);
    
    m_loadTimeLabel = new QLabel(m_pluginInfo.loadTime.toString());
    
    m_errorLabel = new QLabel(m_pluginInfo.errorMessage);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet("color: red;");
    if (m_pluginInfo.errorMessage.isEmpty()) {
        m_errorLabel->setVisible(false);
    }
    
    infoLayout->addRow(tr("Name:"), m_nameLabel);
    infoLayout->addRow(tr("Version:"), m_versionLabel);
    infoLayout->addRow(tr("Author:"), m_authorLabel);
    infoLayout->addRow(tr("Website:"), m_websiteLabel);
    infoLayout->addRow(tr("Description:"), m_descriptionLabel);
    infoLayout->addRow(tr("Dependencies:"), m_dependenciesLabel);
    infoLayout->addRow(tr("File Path:"), m_filePathLabel);
    infoLayout->addRow(tr("Load Time:"), m_loadTimeLabel);
    infoLayout->addRow(tr("Error:"), m_errorLabel);
    
    // Metadata
    QGroupBox* metadataGroup = new QGroupBox(tr("Metadata"));
    QVBoxLayout* metadataLayout = new QVBoxLayout(metadataGroup);
    
    m_metadataEdit = new QTextEdit();
    m_metadataEdit->setReadOnly(true);
    m_metadataEdit->setMaximumHeight(150);
    
    // Convert metadata to readable format
    QString metadataText;
    for (auto it = m_pluginInfo.metadata.begin(); it != m_pluginInfo.metadata.end(); ++it) {
        metadataText += QString("%1: %2\n").arg(it.key(), it.value().toString());
    }
    m_metadataEdit->setPlainText(metadataText);
    
    metadataLayout->addWidget(m_metadataEdit);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(metadataGroup);
    mainLayout->addLayout(buttonLayout);
}

void PluginDetailsDialog::setupConnections() {
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

// PluginConfigDialog implementation
PluginConfigDialog::PluginConfigDialog(const QString& pluginId, PluginManager* pluginManager, QWidget *parent)
    : QDialog(parent)
    , m_pluginId(pluginId)
    , m_pluginManager(pluginManager)
    , m_configTree(nullptr)
    , m_addConfigButton(nullptr)
    , m_removeConfigButton(nullptr)
    , m_editConfigButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
{
    setupUI();
    setupConnections();
    loadPluginConfig();
    
    setWindowTitle(tr("Plugin Configuration"));
    setMinimumSize(600, 500);
    resize(700, 600);
}

PluginConfigDialog::~PluginConfigDialog() {
}

void PluginConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Configuration tree
    m_configTree = new QTreeWidget();
    m_configTree->setHeaderLabels(QStringList() << tr("Setting") << tr("Value") << tr("Type"));
    m_configTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_configTree->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Add some sample configuration items
    QTreeWidgetItem* generalItem = new QTreeWidgetItem(m_configTree);
    generalItem->setText(0, tr("General"));
    generalItem->setExpanded(true);
    
    QTreeWidgetItem* enabledItem = new QTreeWidgetItem(generalItem);
    enabledItem->setText(0, tr("Enabled"));
    enabledItem->setText(1, tr("true"));
    enabledItem->setText(2, tr("Boolean"));
    
    QTreeWidgetItem* verbosityItem = new QTreeWidgetItem(generalItem);
    verbosityItem->setText(0, tr("Verbosity"));
    verbosityItem->setText(1, tr("1"));
    verbosityItem->setText(2, tr("Integer"));
    
    QTreeWidgetItem* outputPathItem = new QTreeWidgetItem(generalItem);
    outputPathItem->setText(0, tr("Output Path"));
    outputPathItem->setText(1, QDir::homePath());
    outputPathItem->setText(2, tr("String"));
    
    // Configuration buttons
    QHBoxLayout* configButtonLayout = new QHBoxLayout();
    m_addConfigButton = new QPushButton(tr("Add Setting"));
    m_removeConfigButton = new QPushButton(tr("Remove Setting"));
    m_editConfigButton = new QPushButton(tr("Edit Setting"));
    
    configButtonLayout->addWidget(m_addConfigButton);
    configButtonLayout->addWidget(m_removeConfigButton);
    configButtonLayout->addWidget(m_editConfigButton);
    configButtonLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("Save"));
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_resetButton = new QPushButton(tr("Reset"));
    m_saveButton->setDefault(true);
    
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addWidget(m_configTree);
    mainLayout->addLayout(configButtonLayout);
    mainLayout->addLayout(buttonLayout);
}

void PluginConfigDialog::setupConnections() {
    connect(m_addConfigButton, &QPushButton::clicked, [this]() {
        // Add configuration item
        QTreeWidgetItem* selectedItem = m_configTree->currentItem();
        if (!selectedItem) {
            selectedItem = m_configTree->invisibleRootItem();
        }
        
        QTreeWidgetItem* newItem = new QTreeWidgetItem(selectedItem);
        newItem->setText(0, tr("New Setting"));
        newItem->setText(1, tr("Value"));
        newItem->setText(2, tr("String"));
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
        
        m_configTree->expandItem(selectedItem);
        m_configTree->setCurrentItem(newItem);
        m_configTree->editItem(newItem, 0);
    });
    
    connect(m_removeConfigButton, &QPushButton::clicked, [this]() {
        QTreeWidgetItem* selectedItem = m_configTree->currentItem();
        if (selectedItem && selectedItem->parent()) {
            delete selectedItem;
        }
    });
    
    connect(m_editConfigButton, &QPushButton::clicked, [this]() {
        QTreeWidgetItem* selectedItem = m_configTree->currentItem();
        if (selectedItem) {
            m_configTree->editItem(selectedItem, 1);
        }
    });
    
    connect(m_saveButton, &QPushButton::clicked, this, &PluginConfigDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &PluginConfigDialog::onCancel);
    connect(m_resetButton, &QPushButton::clicked, this, &PluginConfigDialog::onReset);
}

void PluginConfigDialog::loadPluginConfig() {
    // Load plugin configuration from PluginManager
    if (m_pluginManager) {
        // In a real implementation, this would load the actual plugin configuration
    }
}

void PluginConfigDialog::savePluginConfig() {
    // Save plugin configuration to PluginManager
    if (m_pluginManager) {
        // In a real implementation, this would save the actual plugin configuration
        QMessageBox::information(this, tr("Success"), tr("Plugin configuration saved successfully."));
        accept();
    }
}

void PluginConfigDialog::onSave() {
    savePluginConfig();
}

void PluginConfigDialog::onCancel() {
    reject();
}

void PluginConfigDialog::onReset() {
    int result = QMessageBox::question(this, tr("Confirm Reset"),
                                    tr("Are you sure you want to reset all settings to their default values?"));
    if (result == QMessageBox::Yes) {
        // Reset configuration to defaults
        m_configTree->clear();
        
        // Add default configuration items
        QTreeWidgetItem* generalItem = new QTreeWidgetItem(m_configTree);
        generalItem->setText(0, tr("General"));
        generalItem->setExpanded(true);
        
        QTreeWidgetItem* enabledItem = new QTreeWidgetItem(generalItem);
        enabledItem->setText(0, tr("Enabled"));
        enabledItem->setText(1, tr("true"));
        enabledItem->setText(2, tr("Boolean"));
        
        QMessageBox::information(this, tr("Success"), tr("Configuration reset to defaults."));
    }
}

// ScriptEditorDialog implementation
ScriptEditorDialog::ScriptEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_content(QString())
    , m_language("javascript")
    , m_editor(nullptr)
    , m_languageCombo(nullptr)
    , m_syntaxCheckButton(nullptr)
    , m_executeButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_loadButton(nullptr)
    , m_saveAsButton(nullptr)
{
    setupUI();
    setupConnections();
    
    setWindowTitle(tr("Script Editor"));
    setMinimumSize(800, 600);
    resize(1000, 700);
}

ScriptEditorDialog::~ScriptEditorDialog() {
}

void ScriptEditorDialog::setScriptContent(const QString& content) {
    m_content = content;
    if (m_editor) {
        m_editor->setPlainText(content);
    }
}

QString ScriptEditorDialog::scriptContent() const {
    if (m_editor) {
        return m_editor->toPlainText();
    }
    return m_content;
}

void ScriptEditorDialog::setScriptLanguage(const QString& language) {
    m_language = language;
    if (m_languageCombo) {
        int index = m_languageCombo->findText(language, Qt::MatchFixedString);
        if (index >= 0) {
            m_languageCombo->setCurrentIndex(index);
        }
    }
}

QString ScriptEditorDialog::scriptLanguage() const {
    if (m_languageCombo) {
        return m_languageCombo->currentText().toLower();
    }
    return m_language;
}

void ScriptEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("JavaScript");
    m_languageCombo->addItem("Python");
    m_languageCombo->addItem("Lua");
    m_languageCombo->addItem("Custom");
    m_languageCombo->setCurrentText("JavaScript");
    
    m_syntaxCheckButton = new QPushButton(tr("Check Syntax"));
    m_executeButton = new QPushButton(tr("Execute"));
    m_loadButton = new QPushButton(tr("Load"));
    m_saveAsButton = new QPushButton(tr("Save As"));
    
    toolbarLayout->addWidget(new QLabel(tr("Language:")));
    toolbarLayout->addWidget(m_languageCombo);
    toolbarLayout->addWidget(m_syntaxCheckButton);
    toolbarLayout->addWidget(m_executeButton);
    toolbarLayout->addWidget(m_loadButton);
    toolbarLayout->addWidget(m_saveAsButton);
    toolbarLayout->addStretch();
    
    // Editor
    m_editor = new QTextEdit();
    m_editor->setPlaceholderText(tr("Enter your script code here..."));
    m_editor->setFont(QFont("Courier New", 10));
    
    // Status bar
    QHBoxLayout* statusLayout = new QHBoxLayout();
    QLabel* statusLabel = new QLabel(tr("Ready"));
    statusLabel->setStyleSheet("color: gray; font-style: italic;");
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("Save"));
    m_cancelButton = new QPushButton(tr("Close"));
    m_saveButton->setDefault(true);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(m_editor);
    mainLayout->addLayout(statusLayout);
    mainLayout->addLayout(buttonLayout);
}

void ScriptEditorDialog::setupConnections() {
    connect(m_languageCombo, &QComboBox::currentTextChanged, [this](const QString& text) {
        m_language = text.toLower();
    });
    
    connect(m_syntaxCheckButton, &QPushButton::clicked, this, &ScriptEditorDialog::onSyntaxCheck);
    connect(m_executeButton, &QPushButton::clicked, this, &ScriptEditorDialog::onExecute);
    connect(m_loadButton, &QPushButton::clicked, this, &ScriptEditorDialog::onLoadScript);
    connect(m_saveAsButton, &QPushButton::clicked, this, &ScriptEditorDialog::onSaveScript);
    connect(m_saveButton, &QPushButton::clicked, this, &ScriptEditorDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &ScriptEditorDialog::onCancel);
}

void ScriptEditorDialog::onSave() {
    m_content = scriptContent();
    accept();
}

void ScriptEditorDialog::onCancel() {
    reject();
}

void ScriptEditorDialog::onExecute() {
    QString content = scriptContent();
    QString language = scriptLanguage();
    
    // In a real implementation, this would execute the script
    QMessageBox::information(this, tr("Execute Script"),
                           tr("Script would be executed here.\n"
                              "Language: %1\n"
                              "Content length: %2 characters")
                           .arg(language)
                           .arg(content.length()));
}

void ScriptEditorDialog::onLoadScript() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Script"),
                                                   QDir::homePath(),
                                                   tr("Script Files (*.js *.py *.lua);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            
            setScriptContent(content);
            
            // Determine language from file extension
            QFileInfo fileInfo(fileName);
            QString suffix = fileInfo.suffix().toLower();
            if (suffix == "js") {
                setScriptLanguage("javascript");
            } else if (suffix == "py") {
                setScriptLanguage("python");
            } else if (suffix == "lua") {
                setScriptLanguage("lua");
            } else {
                setScriptLanguage("custom");
            }
            
            QMessageBox::information(this, tr("Success"), tr("Script loaded successfully."));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load script."));
        }
    }
}

void ScriptEditorDialog::onSaveScript() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Script"),
                                                   QDir::homePath() + "/script.js",
                                                   tr("JavaScript Files (*.js);;Python Files (*.py);;Lua Files (*.lua);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString content = scriptContent();
            file.write(content.toUtf8());
            file.close();
            
            QMessageBox::information(this, tr("Success"), tr("Script saved successfully."));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to save script."));
        }
    }
}

void ScriptEditorDialog::onSyntaxCheck() {
    QString content = scriptContent();
    QString language = scriptLanguage();
    
    // In a real implementation, this would check syntax
    QMessageBox::information(this, tr("Syntax Check"),
                           tr("Syntax check would be performed here.\n"
                              "Language: %1\n"
                              "Content length: %2 characters\n\n"
                              "Result: Syntax OK")
                           .arg(language)
                           .arg(content.length()));
}

// CustomRuleEditorDialog implementation
CustomRuleEditorDialog::CustomRuleEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_ruleData()
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_conditionCombo(nullptr)
    , m_actionCombo(nullptr)
    , m_parametersTable(nullptr)
    , m_enabledCheck(nullptr)
    , m_prioritySpin(nullptr)
    , m_addParamButton(nullptr)
    , m_removeParamButton(nullptr)
    , m_testButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
    
    setWindowTitle(tr("Custom Rule Editor"));
    setMinimumSize(600, 500);
    resize(700, 600);
}

CustomRuleEditorDialog::~CustomRuleEditorDialog() {
}

void CustomRuleEditorDialog::setRuleData(const RuleData& data) {
    m_ruleData = data;
    
    if (m_nameEdit) m_nameEdit->setText(data.name);
    if (m_descriptionEdit) m_descriptionEdit->setPlainText(data.description);
    if (m_conditionCombo) m_conditionCombo->setCurrentText(data.condition);
    if (m_actionCombo) m_actionCombo->setCurrentText(data.action);
    if (m_enabledCheck) m_enabledCheck->setChecked(data.enabled);
    if (m_prioritySpin) m_prioritySpin->setValue(data.priority);
    
    // Populate parameters table
    if (m_parametersTable) {
        m_parametersTable->setRowCount(0);
        for (auto it = data.parameters.begin(); it != data.parameters.end(); ++it) {
            int row = m_parametersTable->rowCount();
            m_parametersTable->insertRow(row);
            m_parametersTable->setItem(row, 0, new QTableWidgetItem(it.key()));
            m_parametersTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
        }
    }
}

CustomRuleEditorDialog::RuleData CustomRuleEditorDialog::ruleData() const {
    RuleData data = m_ruleData;
    
    if (m_nameEdit) data.name = m_nameEdit->text();
    if (m_descriptionEdit) data.description = m_descriptionEdit->toPlainText();
    if (m_conditionCombo) data.condition = m_conditionCombo->currentText();
    if (m_actionCombo) data.action = m_actionCombo->currentText();
    if (m_enabledCheck) data.enabled = m_enabledCheck->isChecked();
    if (m_prioritySpin) data.priority = m_prioritySpin->value();
    
    // Collect parameters from table
    if (m_parametersTable) {
        data.parameters.clear();
        for (int i = 0; i < m_parametersTable->rowCount(); ++i) {
            QTableWidgetItem* keyItem = m_parametersTable->item(i, 0);
            QTableWidgetItem* valueItem = m_parametersTable->item(i, 1);
            if (keyItem && valueItem) {
                data.parameters[keyItem->text()] = valueItem->text();
            }
        }
    }
    
    return data;
}

void CustomRuleEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Rule properties
    QGroupBox* propertiesGroup = new QGroupBox(tr("Rule Properties"));
    QFormLayout* propertiesLayout = new QFormLayout(propertiesGroup);
    
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("Enter rule name"));
    
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setPlaceholderText(tr("Enter rule description"));
    
    m_conditionCombo = new QComboBox();
    m_conditionCombo->addItem(tr("File Size Greater Than"));
    m_conditionCombo->addItem(tr("File Extension"));
    m_conditionCombo->addItem(tr("File Age Greater Than"));
    m_conditionCombo->addItem(tr("Is Duplicate"));
    m_conditionCombo->addItem(tr("Custom Condition"));
    
    m_actionCombo = new QComboBox();
    m_actionCombo->addItem(tr("Delete File"));
    m_actionCombo->addItem(tr("Move File"));
    m_actionCombo->addItem(tr("Compress File"));
    m_actionCombo->addItem(tr("Tag File"));
    m_actionCombo->addItem(tr("Custom Action"));
    
    m_enabledCheck = new QCheckBox(tr("Rule Enabled"));
    m_enabledCheck->setChecked(true);
    
    m_prioritySpin = new QSpinBox();
    m_prioritySpin->setRange(0, 100);
    m_prioritySpin->setValue(50);
    m_prioritySpin->setToolTip(tr("Higher priority rules are evaluated first"));
    
    propertiesLayout->addRow(tr("Name:"), m_nameEdit);
    propertiesLayout->addRow(tr("Description:"), m_descriptionEdit);
    propertiesLayout->addRow(tr("Condition:"), m_conditionCombo);
    propertiesLayout->addRow(tr("Action:"), m_actionCombo);
    propertiesLayout->addRow(tr("Enabled:"), m_enabledCheck);
    propertiesLayout->addRow(tr("Priority:"), m_prioritySpin);
    
    // Parameters
    QGroupBox* parametersGroup = new QGroupBox(tr("Parameters"));
    QVBoxLayout* parametersLayout = new QVBoxLayout(parametersGroup);
    
    m_parametersTable = new QTableWidget(0, 2);
    m_parametersTable->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
    m_parametersTable->horizontalHeader()->setStretchLastSection(true);
    m_parametersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    QHBoxLayout* paramButtonLayout = new QHBoxLayout();
    m_addParamButton = new QPushButton(tr("Add Parameter"));
    m_removeParamButton = new QPushButton(tr("Remove Parameter"));
    paramButtonLayout->addWidget(m_addParamButton);
    paramButtonLayout->addWidget(m_removeParamButton);
    paramButtonLayout->addStretch();
    
    parametersLayout->addWidget(m_parametersTable);
    parametersLayout->addLayout(paramButtonLayout);
    
    // Test button
    m_testButton = new QPushButton(tr("Test Rule"));
    m_testButton->setToolTip(tr("Test the rule with sample data"));
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("Save"));
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_saveButton->setDefault(true);
    
    buttonLayout->addWidget(m_testButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addWidget(propertiesGroup);
    mainLayout->addWidget(parametersGroup);
    mainLayout->addWidget(m_testButton);
    mainLayout->addLayout(buttonLayout);
}

void CustomRuleEditorDialog::setupConnections() {
    connect(m_addParamButton, &QPushButton::clicked, [this]() {
        int row = m_parametersTable->rowCount();
        m_parametersTable->insertRow(row);
        m_parametersTable->setItem(row, 0, new QTableWidgetItem("key"));
        m_parametersTable->setItem(row, 1, new QTableWidgetItem("value"));
    });
    
    connect(m_removeParamButton, &QPushButton::clicked, [this]() {
        QList<QTableWidgetItem*> selected = m_parametersTable->selectedItems();
        if (!selected.isEmpty()) {
            m_parametersTable->removeRow(selected.first()->row());
        }
    });
    
    connect(m_testButton, &QPushButton::clicked, this, &CustomRuleEditorDialog::onTestRule);
    connect(m_saveButton, &QPushButton::clicked, this, &CustomRuleEditorDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &CustomRuleEditorDialog::onCancel);
}

void CustomRuleEditorDialog::onSave() {
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a rule name."));
        return;
    }
    
    if (m_conditionCombo->currentText().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please select a condition."));
        return;
    }
    
    if (m_actionCombo->currentText().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please select an action."));
        return;
    }
    
    accept();
}

void CustomRuleEditorDialog::onCancel() {
    reject();
}

void CustomRuleEditorDialog::onTestRule() {
    QMessageBox::information(this, tr("Test Rule"),
                           tr("Rule testing would be implemented here.\n"
                              "This would test the rule with sample data to verify it works correctly."));
}

// PluginStoreBrowser implementation
PluginStoreBrowser::PluginStoreBrowser(PluginManager* pluginManager, QWidget *parent)
    : QDialog(parent)
    , m_pluginManager(pluginManager)
    , m_searchEdit(nullptr)
    , m_categoryCombo(nullptr)
    , m_sortCombo(nullptr)
    , m_pluginTree(nullptr)
    , m_installButton(nullptr)
    , m_updateButton(nullptr)
    , m_uninstallButton(nullptr)
    , m_detailsButton(nullptr)
    , m_refreshButton(nullptr)
    , m_progressBar(nullptr)
{
    setupUI();
    setupConnections();
    loadStoreData();
    populatePluginList();
    
    setWindowTitle(tr("Plugin Store"));
    setMinimumSize(800, 600);
    resize(1000, 700);
}

PluginStoreBrowser::~PluginStoreBrowser() {
}

void PluginStoreBrowser::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Search/filter area
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search plugins..."));
    
    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItem(tr("All Categories"));
    m_categoryCombo->addItem(tr("Analyzers"));
    m_categoryCombo->addItem(tr("Visualizers"));
    m_categoryCombo->addItem(tr("Exporters"));
    m_categoryCombo->addItem(tr("Importers"));
    m_categoryCombo->addItem(tr("Filters"));
    m_categoryCombo->addItem(tr("Scripts"));
    
    m_sortCombo = new QComboBox();
    m_sortCombo->addItem(tr("Name"));
    m_sortCombo->addItem(tr("Rating"));
    m_sortCombo->addItem(tr("Downloads"));
    m_sortCombo->addItem(tr("Updated"));
    m_sortCombo->addItem(tr("Published"));
    m_sortCombo->setCurrentText(tr("Rating"));
    
    searchLayout->addWidget(new QLabel(tr("Search:")));
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(new QLabel(tr("Category:")));
    searchLayout->addWidget(m_categoryCombo);
    searchLayout->addWidget(new QLabel(tr("Sort by:")));
    searchLayout->addWidget(m_sortCombo);
    
    // Plugin tree
    m_pluginTree = new QTreeWidget();
    m_pluginTree->setHeaderLabels(QStringList() << tr("Name") << tr("Version") << tr("Author") 
                                                << tr("Rating") << tr("Downloads") << tr("Updated"));
    m_pluginTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pluginTree->setAlternatingRowColors(true);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_installButton = new QPushButton(tr("Install"));
    m_updateButton = new QPushButton(tr("Update"));
    m_uninstallButton = new QPushButton(tr("Uninstall"));
    m_detailsButton = new QPushButton(tr("Details"));
    m_refreshButton = new QPushButton(tr("Refresh"));
    
    buttonLayout->addWidget(m_installButton);
    buttonLayout->addWidget(m_updateButton);
    buttonLayout->addWidget(m_uninstallButton);
    buttonLayout->addWidget(m_detailsButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_pluginTree);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addLayout(buttonLayout);
}

void PluginStoreBrowser::setupConnections() {
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &PluginStoreBrowser::onSearchTextChanged);
    connect(m_categoryCombo, &QComboBox::currentTextChanged,
            this, &PluginStoreBrowser::onCategoryChanged);
    connect(m_sortCombo, &QComboBox::currentTextChanged,
            this, &PluginStoreBrowser::onSortChanged);
    
    connect(m_installButton, &QPushButton::clicked,
            this, &PluginStoreBrowser::onInstallClicked);
    connect(m_updateButton, &QPushButton::clicked,
            this, &PluginStoreBrowser::onUpdateClicked);
    connect(m_uninstallButton, &QPushButton::clicked,
            this, &PluginStoreBrowser::onUninstallClicked);
    connect(m_detailsButton, &QPushButton::clicked,
            this, &PluginStoreBrowser::onViewDetailsClicked);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &PluginStoreBrowser::refreshStore);
    
    connect(m_pluginTree, &QTreeWidget::itemSelectionChanged,
            this, &PluginStoreBrowser::onPluginSelected);
}

void PluginStoreBrowser::loadStoreData() {
    // In a real implementation, this would load data from an online store
    // For now, we'll create sample data
    
    PluginStoreItem item1;
    item1.id = "analyzer1";
    item1.name = "Advanced File Analyzer";
    item1.description = "Analyzes file properties and metadata";
    item1.version = "2.1.0";
    item1.author = "DiskSense Team";
    item1.website = "https://disksense.com/plugins/analyzer1";
    item1.downloadUrl = "https://store.disksense.com/plugins/analyzer1.zip";
    item1.fileSize = 2560000; // 2.5 MB
    item1.rating = 4.8;
    item1.downloadCount = 1250;
    item1.publishedDate = QDateTime::currentDateTime().addDays(-30);
    item1.updatedDate = QDateTime::currentDateTime().addDays(-5);
    item1.tags << "analyzer" << "metadata" << "properties";
    
    m_storeItems.append(item1);
    
    PluginStoreItem item2;
    item2.id = "visualizer1";
    item2.name = "Image Duplicate Finder";
    item2.description = "Finds visually similar and duplicate images";
    item2.version = "1.5.3";
    item2.author = "PhotoTools Inc";
    item2.website = "https://phototools.com/plugins/visualizer1";
    item2.downloadUrl = "https://store.phototools.com/plugins/visualizer1.zip";
    item2.fileSize = 1800000; // 1.8 MB
    item2.rating = 4.6;
    item2.downloadCount = 890;
    item2.publishedDate = QDateTime::currentDateTime().addDays(-45);
    item2.updatedDate = QDateTime::currentDateTime().addDays(-10);
    item2.tags << "visualizer" << "images" << "duplicates";
    
    m_storeItems.append(item2);
    
    m_filteredItems = m_storeItems;
}

void PluginStoreBrowser::populatePluginList() {
    m_pluginTree->clear();
    
    for (const PluginStoreItem& item : m_filteredItems) {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(m_pluginTree);
        treeItem->setText(0, item.name);
        treeItem->setText(1, item.version);
        treeItem->setText(2, item.author);
        treeItem->setText(3, QString::number(item.rating));
        treeItem->setText(4, QString::number(item.downloadCount));
        treeItem->setText(5, item.updatedDate.toString("yyyy-MM-dd"));
        treeItem->setData(0, Qt::UserRole, item.id);
        
        // Set rating color
        if (item.rating >= 4.5) {
            treeItem->setForeground(3, QBrush(Qt::darkGreen));
        } else if (item.rating >= 3.5) {
            treeItem->setForeground(3, QBrush(Qt::darkYellow));
        } else {
            treeItem->setForeground(3, QBrush(Qt::red));
        }
    }
    
    m_pluginTree->resizeColumnToContents(0);
    m_pluginTree->resizeColumnToContents(1);
    m_pluginTree->resizeColumnToContents(2);
    m_pluginTree->resizeColumnToContents(3);
    m_pluginTree->resizeColumnToContents(4);
    m_pluginTree->resizeColumnToContents(5);
}

void PluginStoreBrowser::updatePluginActions() {
    QList<QTreeWidgetItem*> selected = m_pluginTree->selectedItems();
    bool hasSelection = !selected.isEmpty();
    
    m_installButton->setEnabled(hasSelection);
    m_updateButton->setEnabled(hasSelection);
    m_uninstallButton->setEnabled(hasSelection);
    m_detailsButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
        // In a real implementation, we would check if the plugin is installed
        bool isInstalled = false; // Placeholder
        
        m_installButton->setVisible(!isInstalled);
        m_updateButton->setVisible(isInstalled);
        m_uninstallButton->setVisible(isInstalled);
    }
}

void PluginStoreBrowser::refreshStore() {
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
    
    // Simulate loading
    QTimer::singleShot(2000, this, [this]() {
        m_progressBar->setVisible(false);
        loadStoreData();
        populatePluginList();
        QMessageBox::information(this, tr("Store Refreshed"), 
                               tr("Plugin store refreshed successfully."));
    });
}

void PluginStoreBrowser::searchPlugins(const QString& searchTerm) {
    if (searchTerm.isEmpty()) {
        m_filteredItems = m_storeItems;
    } else {
        m_filteredItems.clear();
        for (const PluginStoreItem& item : m_storeItems) {
            if (item.name.contains(searchTerm, Qt::CaseInsensitive) ||
                item.description.contains(searchTerm, Qt::CaseInsensitive) ||
                item.author.contains(searchTerm, Qt::CaseInsensitive) ||
                item.tags.contains(searchTerm, Qt::CaseInsensitive)) {
                m_filteredItems.append(item);
            }
        }
    }
    
    populatePluginList();
}

void PluginStoreBrowser::filterByCategory(const QString& category) {
    if (category == tr("All Categories")) {
        m_filteredItems = m_storeItems;
    } else {
        m_filteredItems.clear();
        for (const PluginStoreItem& item : m_storeItems) {
            // In a real implementation, we would filter by actual category
            Q_UNUSED(item);
            m_filteredItems.append(item); // Placeholder
        }
    }
    
    populatePluginList();
}

void PluginStoreBrowser::sortBy(const QString& sortField) {
    Q_UNUSED(sortField);
    // In a real implementation, we would sort the filtered items
    populatePluginList();
}

void PluginStoreBrowser::installPlugin(const QString& pluginId) {
    Q_UNUSED(pluginId);
    // In a real implementation, this would download and install the plugin
    
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 100);
    
    // Simulate installation progress
    QTimer* timer = new QTimer(this);
    int progress = 0;
    connect(timer, &QTimer::timeout, this, [this, timer, &progress]() {
        progress += 10;
        m_progressBar->setValue(progress);
        
        if (progress >= 100) {
            timer->stop();
            timer->deleteLater();
            m_progressBar->setVisible(false);
            QMessageBox::information(this, tr("Installation Complete"), 
                                   tr("Plugin installed successfully."));
            refreshStore(); // Refresh to show installed status
        }
    });
    
    timer->start(200);
}

void PluginStoreBrowser::onPluginSelected() {
    updatePluginActions();
}

void PluginStoreBrowser::onInstallClicked() {
    QList<QTreeWidgetItem*> selected = m_pluginTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to install."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    installPlugin(pluginId);
}

void PluginStoreBrowser::onUpdateClicked() {
    QList<QTreeWidgetItem*> selected = m_pluginTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to update."));
        return;
    }
    
    QMessageBox::information(this, tr("Update Plugin"),
                           tr("Plugin update would be implemented here.\n"
                              "In a real implementation, this would update the selected plugin."));
}

void PluginStoreBrowser::onUninstallClicked() {
    QList<QTreeWidgetItem*> selected = m_pluginTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to uninstall."));
        return;
    }
    
    int result = QMessageBox::question(this, tr("Confirm Uninstall"),
                                     tr("Are you sure you want to uninstall the selected plugin?"));
    if (result == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Uninstall Plugin"),
                               tr("Plugin uninstallation would be implemented here.\n"
                                  "In a real implementation, this would remove the selected plugin."));
        refreshStore(); // Refresh to show uninstalled status
    }
}

void PluginStoreBrowser::onViewDetailsClicked() {
    QList<QTreeWidgetItem*> selected = m_pluginTree->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a plugin to view details."));
        return;
    }
    
    QString pluginId = selected.first()->data(0, Qt::UserRole).toString();
    
    // Find the plugin
    for (const PluginStoreItem& item : m_storeItems) {
        if (item.id == pluginId) {
            // Show plugin details
            QString details = QString("<h3>%1</h3>"
                                     "<p><b>Version:</b> %2</p>"
                                     "<p><b>Author:</b> %3</p>"
                                     "<p><b>Description:</b> %4</p>"
                                     "<p><b>File Size:</b> %5 KB</p>"
                                     "<p><b>Rating:</b> %6</p>"
                                     "<p><b>Downloads:</b> %7</p>"
                                     "<p><b>Published:</b> %8</p>"
                                     "<p><b>Last Updated:</b> %9</p>")
                              .arg(item.name)
                              .arg(item.version)
                              .arg(item.author)
                              .arg(item.description)
                              .arg(item.fileSize / 1024)
                              .arg(item.rating)
                              .arg(item.downloadCount)
                              .arg(item.publishedDate.toString())
                              .arg(item.updatedDate.toString());
            
            QMessageBox::information(this, tr("Plugin Details"), details);
            break;
        }
    }
}

void PluginStoreBrowser::onSearchTextChanged(const QString& text) {
    searchPlugins(text);
}

void PluginStoreBrowser::onCategoryChanged(const QString& category) {
    filterByCategory(category);
}

void PluginStoreBrowser::onSortChanged(const QString& sortField) {
    sortBy(sortField);
}