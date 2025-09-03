#include "automationdialog.h"
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

// AutomationDialog implementation
AutomationDialog::AutomationDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_taskList(nullptr)
    , m_addTaskButton(nullptr)
    , m_editTaskButton(nullptr)
    , m_deleteTaskButton(nullptr)
    , m_enableTaskButton(nullptr)
    , m_disableTaskButton(nullptr)
    , m_runTaskButton(nullptr)
    , m_viewHistoryButton(nullptr)
    , m_taskNameEdit(nullptr)
    , m_taskDescriptionEdit(nullptr)
    , m_taskFrequencyCombo(nullptr)
    , m_taskScheduledTimeEdit(nullptr)
    , m_taskEnabledCheck(nullptr)
    , m_taskParametersTable(nullptr)
    , m_ruleList(nullptr)
    , m_addRuleButton(nullptr)
    , m_editRuleButton(nullptr)
    , m_deleteRuleButton(nullptr)
    , m_enableRuleButton(nullptr)
    , m_disableRuleButton(nullptr)
    , m_ruleNameEdit(nullptr)
    , m_ruleDescriptionEdit(nullptr)
    , m_ruleConditionEdit(nullptr)
    , m_ruleActionEdit(nullptr)
    , m_ruleParametersTable(nullptr)
    , m_ruleEnabledCheck(nullptr)
    , m_notificationList(nullptr)
    , m_markAsReadButton(nullptr)
    , m_deleteNotificationButton(nullptr)
    , m_clearAllButton(nullptr)
    , m_sendTestButton(nullptr)
    , m_notificationTitleEdit(nullptr)
    , m_notificationMessageEdit(nullptr)
    , m_notificationRecipientEdit(nullptr)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    
    // Initialize core components
    m_scheduler = std::make_unique<TaskScheduler>();
    m_scanTask = std::make_unique<ScanTask>();
    m_notificationManager = std::make_unique<NotificationManager>();
    m_ruleEngine = std::make_unique<AutomationRuleEngine>();
    
    // Connect scheduler signals
    connect(m_scheduler.get(), &TaskScheduler::taskScheduled, 
            this, &AutomationDialog::onTaskScheduled);
    connect(m_scheduler.get(), &TaskScheduler::taskStarted, 
            this, &AutomationDialog::onTaskStarted);
    connect(m_scheduler.get(), &TaskScheduler::taskCompleted, 
            this, &AutomationDialog::onTaskCompleted);
    connect(m_scheduler.get(), &TaskScheduler::taskFailed, 
            this, &AutomationDialog::onTaskFailed);
    
    // Set up refresh timer
    connect(m_refreshTimer, &QTimer::timeout, this, &AutomationDialog::refreshTasks);
    m_refreshTimer->setInterval(30000); // Refresh every 30 seconds
    
    // Load initial data
    refreshTasks();
    refreshNotifications();
    refreshRules();
    
    // Start scheduler
    m_scheduler->start();
}

AutomationDialog::~AutomationDialog() {
    if (m_scheduler) {
        m_scheduler->stop();
    }
}

void AutomationDialog::setupUI() {
    setWindowTitle("Automation Manager");
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    
    // Scheduler tab
    QWidget* schedulerTab = new QWidget();
    QVBoxLayout* schedulerLayout = new QVBoxLayout(schedulerTab);
    
    // Task list
    m_taskList = new QTreeWidget();
    m_taskList->setHeaderLabels(QStringList() << "Name" << "Type" << "Schedule" << "Status" << "Last Run");
    m_taskList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Task buttons
    QHBoxLayout* taskButtonLayout = new QHBoxLayout();
    m_addTaskButton = new QPushButton("Add Task");
    m_editTaskButton = new QPushButton("Edit Task");
    m_deleteTaskButton = new QPushButton("Delete Task");
    m_enableTaskButton = new QPushButton("Enable");
    m_disableTaskButton = new QPushButton("Disable");
    m_runTaskButton = new QPushButton("Run Now");
    m_viewHistoryButton = new QPushButton("View History");
    
    taskButtonLayout->addWidget(m_addTaskButton);
    taskButtonLayout->addWidget(m_editTaskButton);
    taskButtonLayout->addWidget(m_deleteTaskButton);
    taskButtonLayout->addWidget(m_enableTaskButton);
    taskButtonLayout->addWidget(m_disableTaskButton);
    taskButtonLayout->addWidget(m_runTaskButton);
    taskButtonLayout->addWidget(m_viewHistoryButton);
    taskButtonLayout->addStretch();
    
    schedulerLayout->addWidget(m_taskList);
    schedulerLayout->addLayout(taskButtonLayout);
    
    // Rules tab
    QWidget* rulesTab = new QWidget();
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesTab);
    
    m_ruleList = new QTableWidget(0, 5);
    m_ruleList->setHorizontalHeaderLabels(QStringList() << "Name" << "Condition" << "Action" << "Status" << "Last Triggered");
    m_ruleList->horizontalHeader()->setStretchLastSection(true);
    m_ruleList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ruleList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* ruleButtonLayout = new QHBoxLayout();
    m_addRuleButton = new QPushButton("Add Rule");
    m_editRuleButton = new QPushButton("Edit Rule");
    m_deleteRuleButton = new QPushButton("Delete Rule");
    m_enableRuleButton = new QPushButton("Enable");
    m_disableRuleButton = new QPushButton("Disable");
    
    ruleButtonLayout->addWidget(m_addRuleButton);
    ruleButtonLayout->addWidget(m_editRuleButton);
    ruleButtonLayout->addWidget(m_deleteRuleButton);
    ruleButtonLayout->addWidget(m_enableRuleButton);
    ruleButtonLayout->addWidget(m_disableRuleButton);
    ruleButtonLayout->addStretch();
    
    rulesLayout->addWidget(m_ruleList);
    rulesLayout->addLayout(ruleButtonLayout);
    
    // Notifications tab
    QWidget* notificationsTab = new QWidget();
    QVBoxLayout* notificationsLayout = new QVBoxLayout(notificationsTab);
    
    m_notificationList = new QTreeWidget();
    m_notificationList->setHeaderLabels(QStringList() << "Title" << "Message" << "Time" << "Status" << "Task");
    m_notificationList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_notificationList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    QHBoxLayout* notificationButtonLayout = new QHBoxLayout();
    m_markAsReadButton = new QPushButton("Mark as Read");
    m_deleteNotificationButton = new QPushButton("Delete");
    m_clearAllButton = new QPushButton("Clear All");
    m_sendTestButton = new QPushButton("Send Test");
    
    notificationButtonLayout->addWidget(m_markAsReadButton);
    notificationButtonLayout->addWidget(m_deleteNotificationButton);
    notificationButtonLayout->addWidget(m_clearAllButton);
    notificationButtonLayout->addWidget(m_sendTestButton);
    notificationButtonLayout->addStretch();
    
    notificationsLayout->addWidget(m_notificationList);
    notificationsLayout->addLayout(notificationButtonLayout);
    
    // Add tabs
    m_tabWidget->addTab(schedulerTab, "Scheduled Tasks");
    m_tabWidget->addTab(rulesTab, "Automation Rules");
    m_tabWidget->addTab(notificationsTab, "Notifications");
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
}

void AutomationDialog::setupConnections() {
    // Task buttons
    connect(m_addTaskButton, &QPushButton::clicked, this, &AutomationDialog::onAddTask);
    connect(m_editTaskButton, &QPushButton::clicked, this, &AutomationDialog::onEditTask);
    connect(m_deleteTaskButton, &QPushButton::clicked, this, &AutomationDialog::onDeleteTask);
    connect(m_enableTaskButton, &QPushButton::clicked, this, &AutomationDialog::onEnableTask);
    connect(m_disableTaskButton, &QPushButton::clicked, this, &AutomationDialog::onDisableTask);
    connect(m_runTaskButton, &QPushButton::clicked, this, &AutomationDialog::onRunTaskNow);
    connect(m_viewHistoryButton, &QPushButton::clicked, this, &AutomationDialog::onViewTaskHistory);
    
    // Rule buttons
    connect(m_addRuleButton, &QPushButton::clicked, this, &AutomationDialog::onAddRule);
    connect(m_editRuleButton, &QPushButton::clicked, this, &AutomationDialog::onEditRule);
    connect(m_deleteRuleButton, &QPushButton::clicked, this, &AutomationDialog::onDeleteRule);
    connect(m_enableRuleButton, &QPushButton::clicked, this, &AutomationDialog::onEnableRule);
    connect(m_disableRuleButton, &QPushButton::clicked, this, &AutomationDialog::onDisableRule);
    
    // Notification buttons
    connect(m_markAsReadButton, &QPushButton::clicked, this, &AutomationDialog::onMarkAsRead);
    connect(m_deleteNotificationButton, &QPushButton::clicked, this, &AutomationDialog::onDeleteNotification);
    connect(m_clearAllButton, &QPushButton::clicked, this, &AutomationDialog::onClearAllNotifications);
    connect(m_sendTestButton, &QPushButton::clicked, this, &AutomationDialog::onSendTestNotification);
}

void AutomationDialog::refreshTasks() {
    populateTaskList();
}

void AutomationDialog::refreshNotifications() {
    populateNotificationList();
}

void AutomationDialog::refreshRules() {
    populateRuleList();
}

void AutomationDialog::onAddTask() {
    TaskEditorDialog editor(this);
    if (editor.exec() == QDialog::Accepted) {
        TaskScheduler::ScheduledTask task = editor.task();
        m_scheduler->scheduleTask(task);
        refreshTasks();
    }
}

void AutomationDialog::onEditTask() {
    QList<QTreeWidgetItem*> selected = m_taskList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to edit.");
        return;
    }
    
    QString taskId = selected.first()->data(0, Qt::UserRole).toString();
    TaskScheduler::ScheduledTask task = m_scheduler->getTask(taskId);
    
    TaskEditorDialog editor(this);
    editor.setTask(task);
    if (editor.exec() == QDialog::Accepted) {
        TaskScheduler::ScheduledTask updatedTask = editor.task();
        updatedTask.id = taskId; // Preserve the original ID
        m_scheduler->updateTask(taskId, updatedTask);
        refreshTasks();
    }
}

void AutomationDialog::onDeleteTask() {
    QList<QTreeWidgetItem*> selected = m_taskList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to delete.");
        return;
    }
    
    QString taskId = selected.first()->data(0, Qt::UserRole).toString();
    QString taskName = selected.first()->text(0);
    
    int result = QMessageBox::question(this, "Confirm Delete", 
                                     QString("Are you sure you want to delete the task '%1'?").arg(taskName));
    if (result == QMessageBox::Yes) {
        m_scheduler->unscheduleTask(taskId);
        refreshTasks();
    }
}

void AutomationDialog::onEnableTask() {
    QList<QTreeWidgetItem*> selected = m_taskList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to enable.");
        return;
    }
    
    QString taskId = selected.first()->data(0, Qt::UserRole).toString();
    m_scheduler->enableTask(taskId);
    refreshTasks();
}

void AutomationDialog::onDisableTask() {
    QList<QTreeWidgetItem*> selected = m_taskList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to disable.");
        return;
    }
    
    QString taskId = selected.first()->data(0, Qt::UserRole).toString();
    m_scheduler->disableTask(taskId);
    refreshTasks();
}

void AutomationDialog::onRunTaskNow() {
    QList<QTreeWidgetItem*> selected = m_taskList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a task to run.");
        return;
    }
    
    QString taskId = selected.first()->data(0, Qt::UserRole).toString();
    m_scheduler->runTaskNow(taskId);
    
    QMessageBox::information(this, "Task Started", "The task has been started.");
}

void AutomationDialog::onViewTaskHistory() {
    QMessageBox::information(this, "Task History", 
                           "Task history would be displayed here in a full implementation.");
}

void AutomationDialog::onAddRule() {
    RuleEditorDialog editor(this);
    if (editor.exec() == QDialog::Accepted) {
        AutomationRuleEngine::Rule rule = editor.rule();
        m_ruleEngine->addRule(rule);
        refreshRules();
    }
}

void AutomationDialog::onEditRule() {
    QList<QTableWidgetItem*> selected = m_ruleList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a rule to edit.");
        return;
    }
    
    int row = selected.first()->row();
    QString ruleId = m_ruleList->item(row, 0)->data(Qt::UserRole).toString();
    AutomationRuleEngine::Rule rule = m_ruleEngine->getRule(ruleId);
    
    RuleEditorDialog editor(this);
    editor.setRule(rule);
    if (editor.exec() == QDialog::Accepted) {
        AutomationRuleEngine::Rule updatedRule = editor.rule();
        updatedRule.id = ruleId; // Preserve the original ID
        m_ruleEngine->updateRule(ruleId, updatedRule);
        refreshRules();
    }
}

void AutomationDialog::onDeleteRule() {
    QList<QTableWidgetItem*> selected = m_ruleList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a rule to delete.");
        return;
    }
    
    int row = selected.first()->row();
    QString ruleId = m_ruleList->item(row, 0)->data(Qt::UserRole).toString();
    QString ruleName = m_ruleList->item(row, 0)->text();
    
    int result = QMessageBox::question(this, "Confirm Delete", 
                                     QString("Are you sure you want to delete the rule '%1'?").arg(ruleName));
    if (result == QMessageBox::Yes) {
        m_ruleEngine->removeRule(ruleId);
        refreshRules();
    }
}

void AutomationDialog::onEnableRule() {
    QList<QTableWidgetItem*> selected = m_ruleList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a rule to enable.");
        return;
    }
    
    int row = selected.first()->row();
    QString ruleId = m_ruleList->item(row, 0)->data(Qt::UserRole).toString();
    m_ruleEngine->enableRule(ruleId);
    refreshRules();
}

void AutomationDialog::onDisableRule() {
    QList<QTableWidgetItem*> selected = m_ruleList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a rule to disable.");
        return;
    }
    
    int row = selected.first()->row();
    QString ruleId = m_ruleList->item(row, 0)->data(Qt::UserRole).toString();
    m_ruleEngine->disableRule(ruleId);
    refreshRules();
}

void AutomationDialog::onMarkAsRead() {
    QList<QTreeWidgetItem*> selected = m_notificationList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a notification to mark as read.");
        return;
    }
    
    QString notificationId = selected.first()->data(0, Qt::UserRole).toString();
    m_notificationManager->markAsRead(notificationId);
    refreshNotifications();
}

void AutomationDialog::onDeleteNotification() {
    QList<QTreeWidgetItem*> selected = m_notificationList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a notification to delete.");
        return;
    }
    
    QString notificationId = selected.first()->data(0, Qt::UserRole).toString();
    QString notificationTitle = selected.first()->text(0);
    
    int result = QMessageBox::question(this, "Confirm Delete", 
                                     QString("Are you sure you want to delete the notification '%1'?").arg(notificationTitle));
    if (result == QMessageBox::Yes) {
        m_notificationManager->removeNotification(notificationId);
        refreshNotifications();
    }
}

void AutomationDialog::onClearAllNotifications() {
    int result = QMessageBox::question(this, "Confirm Clear", 
                                     "Are you sure you want to delete all notifications?");
    if (result == QMessageBox::Yes) {
        // In a real implementation, we would clear all notifications
        QMessageBox::information(this, "Notifications Cleared", 
                               "All notifications have been cleared.");
        refreshNotifications();
    }
}

void AutomationDialog::onSendTestNotification() {
    QString notificationId = m_notificationManager->sendNotification(
        "Test Notification", 
        "This is a test notification from DiskSense64.",
        QString(), // No specific task
        QVariantMap() // No additional data
    );
    
    QMessageBox::information(this, "Notification Sent", 
                           QString("Test notification sent with ID: %1").arg(notificationId));
    refreshNotifications();
}

void AutomationDialog::onTaskScheduled(const QString& taskId) {
    Q_UNUSED(taskId);
    refreshTasks();
}

void AutomationDialog::onTaskStarted(const QString& taskId) {
    Q_UNUSED(taskId);
    refreshTasks();
}

void AutomationDialog::onTaskCompleted(const QString& taskId, bool success) {
    Q_UNUSED(taskId);
    Q_UNUSED(success);
    refreshTasks();
    
    // Send notification
    m_notificationManager->sendNotification(
        "Task Completed", 
        QString("Scheduled task completed %1").arg(success ? "successfully" : "with errors"),
        taskId
    );
    
    refreshNotifications();
}

void AutomationDialog::onTaskFailed(const QString& taskId, const QString& error) {
    Q_UNUSED(taskId);
    Q_UNUSED(error);
    refreshTasks();
    
    // Send notification
    m_notificationManager->sendNotification(
        "Task Failed", 
        QString("Scheduled task failed: %1").arg(error),
        taskId
    );
    
    refreshNotifications();
}

void AutomationDialog::populateTaskList() {
    m_taskList->clear();
    
    QList<TaskScheduler::ScheduledTask> tasks = m_scheduler->getAllTasks();
    for (const TaskScheduler::ScheduledTask& task : tasks) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_taskList);
        item->setText(0, task.name);
        item->setData(0, Qt::UserRole, task.id);
        
        // Task type
        QString type;
        switch (task.frequency) {
            case TaskScheduler::Once: type = "One-time"; break;
            case TaskScheduler::Daily: type = "Daily"; break;
            case TaskScheduler::Weekly: type = "Weekly"; break;
            case TaskScheduler::Monthly: type = "Monthly"; break;
            case TaskScheduler::Custom: type = "Custom"; break;
        }
        item->setText(1, type);
        
        // Schedule
        item->setText(2, task.scheduledTime.toString("yyyy-MM-dd HH:mm"));
        
        // Status
        QString status;
        switch (task.status) {
            case TaskScheduler::Scheduled: status = "Scheduled"; break;
            case TaskScheduler::Running: status = "Running"; break;
            case TaskScheduler::Completed: status = "Completed"; break;
            case TaskScheduler::Failed: status = "Failed"; break;
            case TaskScheduler::Cancelled: status = "Cancelled"; break;
        }
        item->setText(3, status);
        
        // Last run
        if (task.lastRun.isValid()) {
            item->setText(4, task.lastRun.toString("yyyy-MM-dd HH:mm"));
        }
        
        // Set item color based on status
        switch (task.status) {
            case TaskScheduler::Running:
                item->setForeground(3, QBrush(Qt::blue));
                break;
            case TaskScheduler::Failed:
                item->setForeground(3, QBrush(Qt::red));
                break;
            case TaskScheduler::Completed:
                item->setForeground(3, QBrush(Qt::darkGreen));
                break;
            default:
                break;
        }
    }
    
    m_taskList->resizeColumnToContents(0);
    m_taskList->resizeColumnToContents(1);
    m_taskList->resizeColumnToContents(2);
    m_taskList->resizeColumnToContents(3);
}

void AutomationDialog::populateNotificationList() {
    m_notificationList->clear();
    
    QList<NotificationManager::Notification> notifications = m_notificationManager->getAllNotifications();
    for (const NotificationManager::Notification& notification : notifications) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_notificationList);
        item->setText(0, notification.title);
        item->setData(0, Qt::UserRole, notification.id);
        item->setText(1, notification.message);
        item->setText(2, notification.timestamp.toString("yyyy-MM-dd HH:mm"));
        item->setText(3, notification.read ? "Read" : "Unread");
        item->setText(4, notification.taskId);
        
        // Set font weight for unread notifications
        if (!notification.read) {
            QFont font = item->font(0);
            font.setBold(true);
            for (int i = 0; i < 5; ++i) {
                item->setFont(i, font);
            }
        }
    }
    
    m_notificationList->resizeColumnToContents(0);
    m_notificationList->resizeColumnToContents(2);
    m_notificationList->resizeColumnToContents(3);
}

void AutomationDialog::populateRuleList() {
    m_ruleList->setRowCount(0);
    
    QList<AutomationRuleEngine::Rule> rules = m_ruleEngine->getAllRules();
    for (const AutomationRuleEngine::Rule& rule : rules) {
        int row = m_ruleList->rowCount();
        m_ruleList->insertRow(row);
        
        QTableWidgetItem* nameItem = new QTableWidgetItem(rule.name);
        nameItem->setData(Qt::UserRole, rule.id);
        m_ruleList->setItem(row, 0, nameItem);
        
        m_ruleList->setItem(row, 1, new QTableWidgetItem(rule.condition));
        m_ruleList->setItem(row, 2, new QTableWidgetItem(rule.action));
        m_ruleList->setItem(row, 3, new QTableWidgetItem(rule.enabled ? "Enabled" : "Disabled"));
        
        if (rule.lastTriggered.isValid()) {
            m_ruleList->setItem(row, 4, new QTableWidgetItem(rule.lastTriggered.toString("yyyy-MM-dd HH:mm")));
        }
        
        // Set row color based on status
        if (!rule.enabled) {
            for (int i = 0; i < 5; ++i) {
                QTableWidgetItem* item = m_ruleList->item(row, i);
                if (item) {
                    item->setForeground(QBrush(Qt::gray));
                }
            }
        }
    }
    
    m_ruleList->resizeColumnsToContents();
}

TaskScheduler::ScheduledTask AutomationDialog::createTaskFromUI() const {
    TaskScheduler::ScheduledTask task;
    task.name = m_taskNameEdit->text();
    task.description = m_taskDescriptionEdit->toPlainText();
    task.frequency = static_cast<TaskScheduler::Frequency>(m_taskFrequencyCombo->currentIndex());
    task.scheduledTime = m_taskScheduledTimeEdit->dateTime();
    task.enabled = m_taskEnabledCheck->isChecked();
    
    // Get parameters from table
    for (int i = 0; i < m_taskParametersTable->rowCount(); ++i) {
        QTableWidgetItem* keyItem = m_taskParametersTable->item(i, 0);
        QTableWidgetItem* valueItem = m_taskParametersTable->item(i, 1);
        if (keyItem && valueItem) {
            task.parameters[keyItem->text()] = valueItem->text();
        }
    }
    
    return task;
}

void AutomationDialog::loadTaskToUI(const TaskScheduler::ScheduledTask& task) {
    m_taskNameEdit->setText(task.name);
    m_taskDescriptionEdit->setPlainText(task.description);
    m_taskFrequencyCombo->setCurrentIndex(static_cast<int>(task.frequency));
    m_taskScheduledTimeEdit->setDateTime(task.scheduledTime);
    m_taskEnabledCheck->setChecked(task.enabled);
    
    // Load parameters to table
    m_taskParametersTable->setRowCount(0);
    for (auto it = task.parameters.begin(); it != task.parameters.end(); ++it) {
        int row = m_taskParametersTable->rowCount();
        m_taskParametersTable->insertRow(row);
        m_taskParametersTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_taskParametersTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
    }
}

// TaskEditorDialog implementation
TaskEditorDialog::TaskEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_frequencyCombo(nullptr)
    , m_scheduledTimeEdit(nullptr)
    , m_enabledCheck(nullptr)
    , m_parametersTable(nullptr)
    , m_addParamButton(nullptr)
    , m_removeParamButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
    
    // Set default values
    m_scheduledTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    m_enabledCheck->setChecked(true);
}

TaskEditorDialog::~TaskEditorDialog() {
}

void TaskEditorDialog::setupUI() {
    setWindowTitle("Edit Scheduled Task");
    setMinimumSize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Task properties
    QGroupBox* propertiesGroup = new QGroupBox("Task Properties");
    QFormLayout* propertiesLayout = new QFormLayout(propertiesGroup);
    
    m_nameEdit = new QLineEdit();
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(60);
    
    m_frequencyCombo = new QComboBox();
    m_frequencyCombo->addItem("One-time");
    m_frequencyCombo->addItem("Daily");
    m_frequencyCombo->addItem("Weekly");
    m_frequencyCombo->addItem("Monthly");
    m_frequencyCombo->addItem("Custom");
    
    m_scheduledTimeEdit = new QDateTimeEdit();
    m_scheduledTimeEdit->setCalendarPopup(true);
    m_scheduledTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    
    m_enabledCheck = new QCheckBox("Task is enabled");
    
    propertiesLayout->addRow("Name:", m_nameEdit);
    propertiesLayout->addRow("Description:", m_descriptionEdit);
    propertiesLayout->addRow("Frequency:", m_frequencyCombo);
    propertiesLayout->addRow("Scheduled Time:", m_scheduledTimeEdit);
    propertiesLayout->addRow("", m_enabledCheck);
    
    // Parameters
    QGroupBox* parametersGroup = new QGroupBox("Parameters");
    QVBoxLayout* parametersLayout = new QVBoxLayout(parametersGroup);
    
    m_parametersTable = new QTableWidget(0, 2);
    m_parametersTable->setHorizontalHeaderLabels(QStringList() << "Key" << "Value");
    m_parametersTable->horizontalHeader()->setStretchLastSection(true);
    m_parametersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    QHBoxLayout* paramButtonLayout = new QHBoxLayout();
    m_addParamButton = new QPushButton("Add Parameter");
    m_removeParamButton = new QPushButton("Remove Parameter");
    paramButtonLayout->addWidget(m_addParamButton);
    paramButtonLayout->addWidget(m_removeParamButton);
    paramButtonLayout->addStretch();
    
    parametersLayout->addWidget(m_parametersTable);
    parametersLayout->addLayout(paramButtonLayout);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_cancelButton = new QPushButton("Cancel");
    m_saveButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    // Add to main layout
    mainLayout->addWidget(propertiesGroup);
    mainLayout->addWidget(parametersGroup);
    mainLayout->addLayout(buttonLayout);
}

void TaskEditorDialog::setupConnections() {
    connect(m_addParamButton, &QPushButton::clicked, this, &TaskEditorDialog::onAddParameter);
    connect(m_removeParamButton, &QPushButton::clicked, this, &TaskEditorDialog::onRemoveParameter);
    connect(m_saveButton, &QPushButton::clicked, this, &TaskEditorDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &TaskEditorDialog::onCancel);
}

void TaskEditorDialog::setTask(const TaskScheduler::ScheduledTask& task) {
    m_task = task;
    
    m_nameEdit->setText(task.name);
    m_descriptionEdit->setPlainText(task.description);
    m_frequencyCombo->setCurrentIndex(static_cast<int>(task.frequency));
    m_scheduledTimeEdit->setDateTime(task.scheduledTime);
    m_enabledCheck->setChecked(task.enabled);
    
    // Load parameters
    m_parametersTable->setRowCount(0);
    for (auto it = task.parameters.begin(); it != task.parameters.end(); ++it) {
        int row = m_parametersTable->rowCount();
        m_parametersTable->insertRow(row);
        m_parametersTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_parametersTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
    }
}

TaskScheduler::ScheduledTask TaskEditorDialog::task() const {
    TaskScheduler::ScheduledTask task = m_task;
    
    task.name = m_nameEdit->text();
    task.description = m_descriptionEdit->toPlainText();
    task.frequency = static_cast<TaskScheduler::Frequency>(m_frequencyCombo->currentIndex());
    task.scheduledTime = m_scheduledTimeEdit->dateTime();
    task.enabled = m_enabledCheck->isChecked();
    
    // Get parameters from table
    task.parameters.clear();
    for (int i = 0; i < m_parametersTable->rowCount(); ++i) {
        QTableWidgetItem* keyItem = m_parametersTable->item(i, 0);
        QTableWidgetItem* valueItem = m_parametersTable->item(i, 1);
        if (keyItem && valueItem) {
            task.parameters[keyItem->text()] = valueItem->text();
        }
    }
    
    return task;
}

void TaskEditorDialog::onAddParameter() {
    int row = m_parametersTable->rowCount();
    m_parametersTable->insertRow(row);
    m_parametersTable->setItem(row, 0, new QTableWidgetItem("key"));
    m_parametersTable->setItem(row, 1, new QTableWidgetItem("value"));
}

void TaskEditorDialog::onRemoveParameter() {
    QList<QTableWidgetItem*> selected = m_parametersTable->selectedItems();
    if (!selected.isEmpty()) {
        int row = selected.first()->row();
        m_parametersTable->removeRow(row);
    }
}

void TaskEditorDialog::onSave() {
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a task name.");
        return;
    }
    
    accept();
}

void TaskEditorDialog::onCancel() {
    reject();
}

// RuleEditorDialog implementation
RuleEditorDialog::RuleEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_conditionEdit(nullptr)
    , m_actionEdit(nullptr)
    , m_parametersTable(nullptr)
    , m_enabledCheck(nullptr)
    , m_addParamButton(nullptr)
    , m_removeParamButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setupConnections();
    
    // Set default values
    m_enabledCheck->setChecked(true);
}

RuleEditorDialog::~RuleEditorDialog() {
}

void RuleEditorDialog::setupUI() {
    setWindowTitle("Edit Automation Rule");
    setMinimumSize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Rule properties
    QGroupBox* propertiesGroup = new QGroupBox("Rule Properties");
    QFormLayout* propertiesLayout = new QFormLayout(propertiesGroup);
    
    m_nameEdit = new QLineEdit();
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(60);
    
    m_conditionEdit = new QLineEdit();
    m_conditionEdit->setPlaceholderText("e.g., file_size > 100MB");
    
    m_actionEdit = new QLineEdit();
    m_actionEdit->setPlaceholderText("e.g., send_notification, cleanup, archive");
    
    m_enabledCheck = new QCheckBox("Rule is enabled");
    
    propertiesLayout->addRow("Name:", m_nameEdit);
    propertiesLayout->addRow("Description:", m_descriptionEdit);
    propertiesLayout->addRow("Condition:", m_conditionEdit);
    propertiesLayout->addRow("Action:", m_actionEdit);
    propertiesLayout->addRow("", m_enabledCheck);
    
    // Parameters
    QGroupBox* parametersGroup = new QGroupBox("Parameters");
    QVBoxLayout* parametersLayout = new QVBoxLayout(parametersGroup);
    
    m_parametersTable = new QTableWidget(0, 2);
    m_parametersTable->setHorizontalHeaderLabels(QStringList() << "Key" << "Value");
    m_parametersTable->horizontalHeader()->setStretchLastSection(true);
    m_parametersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    QHBoxLayout* paramButtonLayout = new QHBoxLayout();
    m_addParamButton = new QPushButton("Add Parameter");
    m_removeParamButton = new QPushButton("Remove Parameter");
    paramButtonLayout->addWidget(m_addParamButton);
    paramButtonLayout->addWidget(m_removeParamButton);
    paramButtonLayout->addStretch();
    
    parametersLayout->addWidget(m_parametersTable);
    parametersLayout->addLayout(paramButtonLayout);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save");
    m_cancelButton = new QPushButton("Cancel");
    m_saveButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    // Add to main layout
    mainLayout->addWidget(propertiesGroup);
    mainLayout->addWidget(parametersGroup);
    mainLayout->addLayout(buttonLayout);
}

void RuleEditorDialog::setupConnections() {
    connect(m_addParamButton, &QPushButton::clicked, this, &RuleEditorDialog::onAddParameter);
    connect(m_removeParamButton, &QPushButton::clicked, this, &RuleEditorDialog::onRemoveParameter);
    connect(m_saveButton, &QPushButton::clicked, this, &RuleEditorDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &RuleEditorDialog::onCancel);
}

void RuleEditorDialog::setRule(const AutomationRuleEngine::Rule& rule) {
    m_rule = rule;
    
    m_nameEdit->setText(rule.name);
    m_descriptionEdit->setPlainText(rule.description);
    m_conditionEdit->setText(rule.condition);
    m_actionEdit->setText(rule.action);
    m_enabledCheck->setChecked(rule.enabled);
    
    // Load parameters
    m_parametersTable->setRowCount(0);
    for (auto it = rule.parameters.begin(); it != rule.parameters.end(); ++it) {
        int row = m_parametersTable->rowCount();
        m_parametersTable->insertRow(row);
        m_parametersTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_parametersTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
    }
}

AutomationRuleEngine::Rule RuleEditorDialog::rule() const {
    AutomationRuleEngine::Rule rule = m_rule;
    
    rule.name = m_nameEdit->text();
    rule.description = m_descriptionEdit->toPlainText();
    rule.condition = m_conditionEdit->text();
    rule.action = m_actionEdit->text();
    rule.enabled = m_enabledCheck->isChecked();
    
    // Get parameters from table
    rule.parameters.clear();
    for (int i = 0; i < m_parametersTable->rowCount(); ++i) {
        QTableWidgetItem* keyItem = m_parametersTable->item(i, 0);
        QTableWidgetItem* valueItem = m_parametersTable->item(i, 1);
        if (keyItem && valueItem) {
            rule.parameters[keyItem->text()] = valueItem->text();
        }
    }
    
    return rule;
}

void RuleEditorDialog::onAddParameter() {
    int row = m_parametersTable->rowCount();
    m_parametersTable->insertRow(row);
    m_parametersTable->setItem(row, 0, new QTableWidgetItem("key"));
    m_parametersTable->setItem(row, 1, new QTableWidgetItem("value"));
}

void RuleEditorDialog::onRemoveParameter() {
    QList<QTableWidgetItem*> selected = m_parametersTable->selectedItems();
    if (!selected.isEmpty()) {
        int row = selected.first()->row();
        m_parametersTable->removeRow(row);
    }
}

void RuleEditorDialog::onSave() {
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a rule name.");
        return;
    }
    
    if (m_conditionEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a condition.");
        return;
    }
    
    if (m_actionEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter an action.");
        return;
    }
    
    accept();
}

void RuleEditorDialog::onCancel() {
    reject();
}