#ifndef UI_PLUGINDIALOG_H
#define UI_PLUGINDIALOG_H

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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>
#include "core/ext/pluginmanager.h"

class PluginManager;

class PluginDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginDialog(PluginManager* pluginManager, QWidget *parent = nullptr);
    ~PluginDialog() override;

public slots:
    void refreshPlugins();
    void refreshPluginStore();

private slots:
    void onLoadPlugin();
    void onUnloadPlugin();
    void onActivatePlugin();
    void onDeactivatePlugin();
    void onInstallPlugin();
    void onUninstallPlugin();
    void onUpdatePlugin();
    void onConfigurePlugin();
    void onViewPluginDetails();
    void onEnableAutoUpdate(bool enabled);
    void onCheckForUpdates();
    void onExecuteScript();
    void onLoadCustomRules();
    void onSaveCustomRules();
    void onAddCustomRule();
    void onRemoveCustomRule();
    
    void onPluginLoaded(const QString& pluginId);
    void onPluginUnloaded(const QString& pluginId);
    void onPluginActivated(const QString& pluginId);
    void onPluginDeactivated(const QString& pluginId);
    void onPluginError(const QString& pluginId, const QString& error);
    void onPluginInstalled(const QString& pluginId);
    void onPluginUninstalled(const QString& pluginId);
    void onScriptExecuted(const QString& scriptPath);
    void onScriptError(const QString& error);

private:
    void setupUI();
    void setupConnections();
    void populatePlugins();
    void populatePluginStore();
    void populateCustomRules();
    void updatePluginDetails();
    void updatePluginActions();
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Plugins tab
    QTreeWidget* m_pluginsTree;
    QPushButton* m_loadPluginButton;
    QPushButton* m_unloadPluginButton;
    QPushButton* m_activatePluginButton;
    QPushButton* m_deactivatePluginButton;
    QPushButton* m_configurePluginButton;
    QPushButton* m_viewDetailsButton;
    
    // Plugin store tab
    QTableWidget* m_storeTable;
    QPushButton* m_installPluginButton;
    QPushButton* m_uninstallPluginButton;
    QPushButton* m_updatePluginButton;
    QPushButton* m_checkUpdatesButton;
    QProgressBar* m_storeProgress;
    
    // Scripts tab
    QListWidget* m_scriptsList;
    QPushButton* m_executeScriptButton;
    QPushButton* m_addScriptButton;
    QPushButton* m_removeScriptButton;
    QTextEdit* m_scriptEditor;
    
    // Custom rules tab
    QTableWidget* m_rulesTable;
    QPushButton* m_addRuleButton;
    QPushButton* m_removeRuleButton;
    QPushButton* m_editRuleButton;
    QPushButton* m_loadRulesButton;
    QPushButton* m_saveRulesButton;
    
    // Settings tab
    QGroupBox* m_generalSettingsGroup;
    QCheckBox* m_autoLoadCheck;
    QCheckBox* m_autoUpdateCheck;
    QLineEdit* m_pluginDirectoryEdit;
    QPushButton* m_browsePluginDirButton;
    
    QGroupBox* m_securitySettingsGroup;
    QCheckBox* m_verifySignaturesCheck;
    QCheckBox* m_strictValidationCheck;
    QPushButton* m_manageTrustedButton;
    
    QGroupBox* m_performanceSettingsGroup;
    QSpinBox* m_maxConcurrentPluginsSpin;
    QCheckBox* m_enableCachingCheck;
    
    // Plugin details
    QTextEdit* m_pluginDetailsEdit;
    QLabel* m_pluginStatus;
    
    // Core components
    std::unique_ptr<PluginManager> m_pluginManager;
    std::unique_ptr<ScriptEngine> m_scriptEngine;
    std::unique_ptr<CustomRulesManager> m_rulesManager;
    std::unique_ptr<PluginValidator> m_pluginValidator;
    
    // Network components
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    
    // Progress tracking
    QTimer* m_refreshTimer;
};

// Plugin details dialog
class PluginDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginDetailsDialog(const PluginManager::PluginInfo& pluginInfo, QWidget *parent = nullptr);
    ~PluginDetailsDialog() override;

private:
    void setupUI();
    void setupConnections();
    
    PluginManager::PluginInfo m_pluginInfo;
    
    // UI components
    QLabel* m_nameLabel;
    QLabel* m_versionLabel;
    QLabel* m_authorLabel;
    QLabel* m_websiteLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_dependenciesLabel;
    QLabel* m_filePathLabel;
    QLabel* m_loadTimeLabel;
    QLabel* m_errorLabel;
    
    QTextEdit* m_metadataEdit;
    
    QPushButton* m_closeButton;
};

// Plugin configuration dialog
class PluginConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginConfigDialog(const QString& pluginId, PluginManager* pluginManager, QWidget *parent = nullptr);
    ~PluginConfigDialog() override;

private slots:
    void onSave();
    void onCancel();
    void onReset();

private:
    void setupUI();
    void setupConnections();
    void loadPluginConfig();
    void savePluginConfig();
    
    QString m_pluginId;
    PluginManager* m_pluginManager;
    
    // UI components
    QTreeWidget* m_configTree;
    QPushButton* m_addConfigButton;
    QPushButton* m_removeConfigButton;
    QPushButton* m_editConfigButton;
    
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_resetButton;
};

// Script editor dialog
class ScriptEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScriptEditorDialog(QWidget *parent = nullptr);
    ~ScriptEditorDialog() override;

    void setScriptContent(const QString& content);
    QString scriptContent() const;
    void setScriptLanguage(const QString& language);
    QString scriptLanguage() const;

private slots:
    void onSave();
    void onCancel();
    void onExecute();
    void onLoadScript();
    void onSaveScript();
    void onSyntaxCheck();

private:
    void setupUI();
    void setupConnections();
    
    QString m_content;
    QString m_language;
    
    // UI components
    QTextEdit* m_editor;
    QComboBox* m_languageCombo;
    QPushButton* m_syntaxCheckButton;
    QPushButton* m_executeButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_loadButton;
    QPushButton* m_saveAsButton;
};

// Custom rule editor dialog
class CustomRuleEditorDialog : public QDialog {
    Q_OBJECT

public:
    struct RuleData {
        QString name;
        QString description;
        QString condition;
        QString action;
        QVariantMap parameters;
        bool enabled;
        int priority;
    };

public:
    explicit CustomRuleEditorDialog(QWidget *parent = nullptr);
    ~CustomRuleEditorDialog() override;

    void setRuleData(const RuleData& data);
    RuleData ruleData() const;

private slots:
    void onSave();
    void onCancel();
    void onAddParameter();
    void onRemoveParameter();
    void onTestRule();

private:
    void setupUI();
    void setupConnections();
    
    RuleData m_ruleData;
    
    // UI components
    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;
    QComboBox* m_conditionCombo;
    QComboBox* m_actionCombo;
    QTableWidget* m_parametersTable;
    QCheckBox* m_enabledCheck;
    QSpinBox* m_prioritySpin;
    
    QPushButton* m_addParamButton;
    QPushButton* m_removeParamButton;
    QPushButton* m_testButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

// Plugin store browser
class PluginStoreBrowser : public QDialog {
    Q_OBJECT

public:
    struct PluginStoreItem {
        QString id;
        QString name;
        QString description;
        QString version;
        QString author;
        QString website;
        QString downloadUrl;
        qint64 fileSize;
        double rating;
        int downloadCount;
        QStringList screenshots;
        QStringList tags;
        QDateTime publishedDate;
        QDateTime updatedDate;
    };

public:
    explicit PluginStoreBrowser(PluginManager* pluginManager, QWidget *parent = nullptr);
    ~PluginStoreBrowser() override;

public slots:
    void refreshStore();
    void searchPlugins(const QString& searchTerm);
    void filterByCategory(const QString& category);
    void sortBy(const QString& sortField);
    void installPlugin(const QString& pluginId);

private slots:
    void onPluginSelected();
    void onInstallClicked();
    void onUpdateClicked();
    void onUninstallClicked();
    void onViewDetailsClicked();
    void onSearchTextChanged(const QString& text);
    void onCategoryChanged(const QString& category);
    void onSortChanged(const QString& sortField);

private:
    void setupUI();
    void setupConnections();
    void loadStoreData();
    void populatePluginList();
    void updatePluginActions();
    
    PluginManager* m_pluginManager;
    
    // UI components
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_sortCombo;
    QTreeWidget* m_pluginTree;
    QPushButton* m_installButton;
    QPushButton* m_updateButton;
    QPushButton* m_uninstallButton;
    QPushButton* m_detailsButton;
    QPushButton* m_refreshButton;
    QProgressBar* m_progressBar;
    
    // Store data
    QList<PluginStoreItem> m_storeItems;
    QList<PluginStoreItem> m_filteredItems;
};

#endif // UI_PLUGINDIALOG_H