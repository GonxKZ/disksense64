#ifndef UI_SETTINGSDIALOG_H
#define UI_SETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QColorDialog>
#include <QSettings>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

    // User preferences
    QString theme() const;
    void setTheme(const QString& theme);
    
    QString language() const;
    void setLanguage(const QString& language);
    
    QString defaultPath() const;
    void setDefaultPath(const QString& path);
    
    // Performance settings
    int threadCount() const;
    void setThreadCount(int count);
    
    int memoryLimit() const; // in MB
    void setMemoryLimit(int limit);
    
    // Visualization options
    QString colorScheme() const;
    void setColorScheme(const QString& scheme);
    
    bool showFileIcons() const;
    void setShowFileIcons(bool show);
    
    int treemapAlgorithm() const;
    void setTreemapAlgorithm(int algorithm);
    
    // Scan exclusions
    QStringList exclusions() const;
    void setExclusions(const QStringList& exclusions);
    
    // Filter settings
    QStringList fileTypes() const;
    void setFileTypes(const QStringList& types);
    
    quint64 minFileSize() const;
    void setMinFileSize(quint64 size);
    
    quint64 maxFileSize() const;
    void setMaxFileSize(quint64 size);

public slots:
    void loadSettings();
    void saveSettings();
    void restoreDefaults();

private slots:
    void onBrowseDefaultPath();
    void onAddExclusion();
    void onRemoveExclusion();
    void onAddFileType();
    void onRemoveFileType();
    void onColorSchemeChanged(const QString& scheme);

private:
    void setupUI();
    void connectSignals();
    
    // User preferences tab
    QComboBox* m_themeCombo;
    QComboBox* m_languageCombo;
    QLineEdit* m_defaultPathEdit;
    QPushButton* m_browsePathButton;
    
    // Performance settings tab
    QSpinBox* m_threadCountSpin;
    QSpinBox* m_memoryLimitSpin;
    
    // Visualization options tab
    QComboBox* m_colorSchemeCombo;
    QCheckBox* m_showFileIconsCheck;
    QComboBox* m_treemapAlgorithmCombo;
    
    // Scan exclusions tab
    QListWidget* m_exclusionsList;
    QLineEdit* m_exclusionEdit;
    QPushButton* m_addExclusionButton;
    QPushButton* m_removeExclusionButton;
    
    // Filter settings tab
    QListWidget* m_fileTypesList;
    QLineEdit* m_fileTypeEdit;
    QPushButton* m_addFileTypeButton;
    QPushButton* m_removeFileTypeButton;
    QSpinBox* m_minFileSizeSpin;
    QSpinBox* m_maxFileSizeSpin;
    
    // Buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_defaultsButton;
    
    QSettings* m_settings;
};

#endif // UI_SETTINGSDIALOG_H