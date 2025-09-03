#ifndef UI_AUTOMATIONDIALOG_H
#define UI_AUTOMATIONDIALOG_H

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
#include <QDateTimeEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <memory>
#include "core/automation/scheduler.h"

class TaskScheduler;
class ScanTask;
class NotificationManager;
class AutomationRuleEngine;

class AutomationDialog : public QDialog {
    Q_OBJECT

public:
    explicit AutomationDialog(QWidget *parent = nullptr);
    ~AutomationDialog() override;

public slots:
    void refreshTasks();
    void refreshNotifications();
    void refreshRules();

private slots:
    void onAddTask();
    void onEditTask();
    void onDeleteTask();
    void onEnableTask();
    void onDisableTask();
    void onRunTaskNow();
    void onViewTaskHistory();
    
    void onAddRule();
    void onEditRule();
    void onDeleteRule();
    void onEnableRule();
    void onDisableRule();
    
    void onMarkAsRead();
    void onDeleteNotification();
    void onClearAllNotifications();
    void onSendTestNotification();
    
    void onTaskScheduled(const QString& taskId);
    void onTaskStarted(const QString& taskId);
    void onTaskCompleted(const QString& taskId, bool success);
    void onTaskFailed(const QString& taskId, const QString& error);

private:
    void setupUI();
    void setupConnections();
    void populateTaskList();
    void populateNotificationList();
    void populateRuleList();
    TaskScheduler::ScheduledTask createTaskFromUI() const;
    void loadTaskToUI(const TaskScheduler::ScheduledTask& task);
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Scheduler tab
    QTreeWidget* m_taskList;
    QPushButton* m_addTaskButton;
    QPushButton* m_editTaskButton;
    QPushButton* m_deleteTaskButton;
    QPushButton* m_enableTaskButton;
    QPushButton* m_disableTaskButton;
    QPushButton* m_runTaskButton;
    QPushButton* m_viewHistoryButton;
    
    // Task editor widgets
    QLineEdit* m_taskNameEdit;
    QTextEdit* m_taskDescriptionEdit;
    QComboBox* m_taskFrequencyCombo;
    QDateTimeEdit* m_taskScheduledTimeEdit;
    QCheckBox* m_taskEnabledCheck;
    QTableWidget* m_taskParametersTable;
    
    // Rules tab
    QTableWidget* m_ruleList;
    QPushButton* m_addRuleButton;
    QPushButton* m_editRuleButton;
    QPushButton* m_deleteRuleButton;
    QPushButton* m_enableRuleButton;
    QPushButton* m_disableRuleButton;
    
    // Rule editor widgets
    QLineEdit* m_ruleNameEdit;
    QTextEdit* m_ruleDescriptionEdit;
    QLineEdit* m_ruleConditionEdit;
    QLineEdit* m_ruleActionEdit;
    QTableWidget* m_ruleParametersTable;
    QCheckBox* m_ruleEnabledCheck;
    
    // Notifications tab
    QTreeWidget* m_notificationList;
    QPushButton* m_markAsReadButton;
    QPushButton* m_deleteNotificationButton;
    QPushButton* m_clearAllButton;
    QPushButton* m_sendTestButton;
    
    // Notification editor widgets
    QLineEdit* m_notificationTitleEdit;
    QTextEdit* m_notificationMessageEdit;
    QLineEdit* m_notificationRecipientEdit;
    
    // Core components
    std::unique_ptr<TaskScheduler> m_scheduler;
    std::unique_ptr<ScanTask> m_scanTask;
    std::unique_ptr<NotificationManager> m_notificationManager;
    std::unique_ptr<AutomationRuleEngine> m_ruleEngine;
    
    // Progress tracking
    QTimer* m_refreshTimer;
};

// Scheduled task editor dialog
class TaskEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit TaskEditorDialog(QWidget *parent = nullptr);
    ~TaskEditorDialog() override;

    void setTask(const TaskScheduler::ScheduledTask& task);
    TaskScheduler::ScheduledTask task() const;

private slots:
    void onAddParameter();
    void onRemoveParameter();
    void onSave();
    void onCancel();

private:
    void setupUI();
    void setupConnections();
    
    TaskScheduler::ScheduledTask m_task;
    
    // UI components
    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;
    QComboBox* m_frequencyCombo;
    QDateTimeEdit* m_scheduledTimeEdit;
    QCheckBox* m_enabledCheck;
    QTableWidget* m_parametersTable;
    
    QPushButton* m_addParamButton;
    QPushButton* m_removeParamButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

// Automation rule editor dialog
class RuleEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit RuleEditorDialog(QWidget *parent = nullptr);
    ~RuleEditorDialog() override;

    void setRule(const AutomationRuleEngine::Rule& rule);
    AutomationRuleEngine::Rule rule() const;

private slots:
    void onAddParameter();
    void onRemoveParameter();
    void onSave();
    void onCancel();

private:
    void setupUI();
    void setupConnections();
    
    AutomationRuleEngine::Rule m_rule;
    
    // UI components
    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_conditionEdit;
    QLineEdit* m_actionEdit;
    QTableWidget* m_parametersTable;
    QCheckBox* m_enabledCheck;
    
    QPushButton* m_addParamButton;
    QPushButton* m_removeParamButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // UI_AUTOMATIONDIALOG_H