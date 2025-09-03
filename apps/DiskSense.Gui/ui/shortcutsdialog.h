#ifndef UI_SHORTCUTSDIALOG_H
#define UI_SHORTCUTSDIALOG_H

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
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QKeySequenceEdit>
#include <QShortcut>
#include <QAction>
#include <memory>
#include "core/ux/uxmanager.h"

class UXManager;

class ShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutsDialog(UXManager* uxManager, QWidget *parent = nullptr);
    ~ShortcutsDialog() override;

public slots:
    void refreshShortcuts();
    void applyChanges();
    void resetToDefaults();

private slots:
    void onAddShortcut();
    void onEditShortcut();
    void onDeleteShortcut();
    void onImportShortcuts();
    void onExportShortcuts();
    void onShortcutDoubleClicked(QTreeWidgetItem* item, int column);
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    void setupUI();
    void setupConnections();
    void populateShortcuts();
    void updateShortcutPreview();
    
    UXManager* m_uxManager;
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Shortcuts tab
    QTreeWidget* m_shortcutsTree;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_importButton;
    QPushButton* m_exportButton;
    
    // Shortcut editor
    QLineEdit* m_nameEdit;
    QLineEdit* m_idEdit;
    QKeySequenceEdit* m_keySequenceEdit;
    QTextEdit* m_descriptionEdit;
    QCheckBox* m_globalCheck;
    QCheckBox* m_enabledCheck;
    
    // Preview
    QLabel* m_previewLabel;
    
    // Buttons
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    QPushButton* m_closeButton;
    
    // Current shortcut being edited
    QString m_currentShortcutId;
};

// Shortcut editor dialog
class ShortcutEditorDialog : public QDialog {
    Q_OBJECT

public:
    struct ShortcutData {
        QString id;
        QString name;
        QKeySequence keySequence;
        QString description;
        bool isGlobal;
        bool isEnabled;
    };

public:
    explicit ShortcutEditorDialog(QWidget *parent = nullptr);
    ~ShortcutEditorDialog() override;

    void setShortcutData(const ShortcutData& data);
    ShortcutData shortcutData() const;

private slots:
    void onSave();
    void onCancel();
    void onClearShortcut();

private:
    void setupUI();
    void setupConnections();
    
    ShortcutData m_shortcutData;
    
    // UI components
    QLineEdit* m_nameEdit;
    QLineEdit* m_idEdit;
    QKeySequenceEdit* m_keySequenceEdit;
    QTextEdit* m_descriptionEdit;
    QCheckBox* m_globalCheck;
    QCheckBox* m_enabledCheck;
    
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_clearButton;
};

// Keyboard navigation helper
class KeyboardNavigationHelper : public QObject {
    Q_OBJECT

public:
    explicit KeyboardNavigationHelper(QObject *parent = nullptr);
    ~KeyboardNavigationHelper() override;

    // Focus navigation
    static void focusNextWidget(QWidget* currentWidget);
    static void focusPreviousWidget(QWidget* currentWidget);
    static void focusFirstWidget(QWidget* parentWidget);
    static void focusLastWidget(QWidget* parentWidget);
    
    // Spatial navigation
    static QWidget* findWidgetInDirection(QWidget* currentWidget, Qt::Key direction);
    
    // Shortcut handling
    static bool handleGlobalShortcut(const QKeySequence& keySequence);
    static void registerGlobalShortcut(const QString& id, const QKeySequence& keySequence);
    static void unregisterGlobalShortcut(const QString& id);
    
    // Focus highlighting
    static void enableFocusHighlighting(bool enable);
    static bool isFocusHighlightingEnabled();
    static void setFocusBorderColor(const QColor& color);
    static QColor focusBorderColor();

signals:
    void globalShortcutTriggered(const QString& id);
    void focusChanged(QWidget* oldWidget, QWidget* newWidget);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    static void initializeKeyboardNavigation();
    static bool s_focusHighlightingEnabled;
    static QColor s_focusBorderColor;
    static QMap<QString, QKeySequence> s_globalShortcuts;
};

// Accessibility overlay
class AccessibilityOverlay : public QWidget {
    Q_OBJECT

public:
    explicit AccessibilityOverlay(QWidget *parent = nullptr);
    ~AccessibilityOverlay() override;

    // Screen reader support
    void enableScreenReaderSupport(bool enable);
    bool isScreenReaderSupportEnabled() const;
    
    // High contrast mode
    void enableHighContrastMode(bool enable);
    bool isHighContrastModeEnabled() const;
    
    // Magnification
    void setMagnificationLevel(double level); // 1.0 = normal, 2.0 = 2x zoom
    double magnificationLevel() const;
    
    // Text-to-speech
    void speakText(const QString& text);
    void stopSpeaking();
    bool isSpeaking() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void updateAccessibilityFeatures();

private:
    void setupAccessibility();
    void updateMagnification();
    void updateHighContrast();
    
    bool m_screenReaderSupport;
    bool m_highContrastMode;
    double m_magnificationLevel;
    bool m_speaking;
};

#endif // UI_SHORTCUTSDIALOG_H