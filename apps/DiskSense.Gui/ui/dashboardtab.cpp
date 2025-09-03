#include "dashboardtab.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QSysInfo>
#include <QStorageInfo>
#include <QFileInfoList>
#include <QDir>
#include <QDateTime>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif

// DashboardTab implementation
DashboardTab::DashboardTab(QWidget *parent)
    : QWidget(parent)
    , m_systemInfoWidget(nullptr)
    , m_quickStatsWidget(nullptr)
    , m_recentScansWidget(nullptr)
    , m_quickAccessWidget(nullptr)
    , m_refreshTimer(nullptr)
{
    setupUI();
    connectSignals();
    
    // Set up auto-refresh timer (every 30 seconds)
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DashboardTab::refreshAll);
    m_refreshTimer->start(30000); // 30 seconds
    
    // Initial refresh
    refreshAll();
}

DashboardTab::~DashboardTab() {
}

void DashboardTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create widgets
    m_systemInfoWidget = new SystemInfoWidget();
    m_quickStatsWidget = new QuickStatsWidget();
    m_recentScansWidget = new RecentScansWidget();
    m_quickAccessWidget = new QuickAccessWidget();
    
    // Add widgets to layout
    mainLayout->addWidget(m_systemInfoWidget);
    mainLayout->addWidget(m_quickStatsWidget);
    mainLayout->addWidget(m_recentScansWidget);
    mainLayout->addWidget(m_quickAccessWidget);
    
    // Set stretch factors
    mainLayout->setStretchFactor(m_systemInfoWidget, 1);
    mainLayout->setStretchFactor(m_quickStatsWidget, 1);
    mainLayout->setStretchFactor(m_recentScansWidget, 2);
    mainLayout->setStretchFactor(m_quickAccessWidget, 1);
}

void DashboardTab::connectSignals() {
    connect(m_quickAccessWidget, &QuickAccessWidget::scanRequested, 
            this, &DashboardTab::scanRequested);
    connect(m_quickAccessWidget, &QuickAccessWidget::settingsRequested, 
            this, &DashboardTab::settingsRequested);
    connect(m_quickAccessWidget, &QuickAccessWidget::exportRequested, 
            this, &DashboardTab::exportRequested);
}

void DashboardTab::refreshSystemInfo() {
    m_systemInfoWidget->updateInfo();
}

void DashboardTab::refreshQuickStats() {
    m_quickStatsWidget->updateStats();
}

void DashboardTab::refreshRecentScans() {
    m_recentScansWidget->updateScans();
}

void DashboardTab::refreshAll() {
    refreshSystemInfo();
    refreshQuickStats();
    refreshRecentScans();
}

// SystemInfoWidget implementation
SystemInfoWidget::SystemInfoWidget(QWidget *parent)
    : QGroupBox("System Information", parent)
{
    setupUI();
    updateInfo();
}

void SystemInfoWidget::setupUI() {
    QGridLayout* layout = new QGridLayout(this);
    
    m_osLabel = new QLabel();
    m_cpuLabel = new QLabel();
    m_memoryLabel = new QLabel();
    m_diskSpaceLabel = new QLabel();
    m_lastUpdatedLabel = new QLabel();
    
    layout->addWidget(new QLabel("Operating System:"), 0, 0);
    layout->addWidget(m_osLabel, 0, 1);
    layout->addWidget(new QLabel("CPU:"), 1, 0);
    layout->addWidget(m_cpuLabel, 1, 1);
    layout->addWidget(new QLabel("Memory:"), 2, 0);
    layout->addWidget(m_memoryLabel, 2, 1);
    layout->addWidget(new QLabel("Disk Space:"), 3, 0);
    layout->addWidget(m_diskSpaceLabel, 3, 1);
    layout->addWidget(m_lastUpdatedLabel, 4, 0, 1, 2);
    
    layout->setColumnStretch(1, 1);
}

void SystemInfoWidget::updateInfo() {
    // OS Information
    QString osInfo = QString("%1 %2 (%3)")
        .arg(QSysInfo::prettyProductName())
        .arg(QSysInfo::currentCpuArchitecture())
        .arg(QSysInfo::buildAbi());
    m_osLabel->setText(osInfo);
    
    // CPU Information
#ifdef Q_OS_WIN
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    QString cpuInfo = QString("%1 cores").arg(sysInfo.dwNumberOfProcessors);
    m_cpuLabel->setText(cpuInfo);
#elif defined(Q_OS_LINUX)
    QFile cpuInfoFile("/proc/cpuinfo");
    if (cpuInfoFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&cpuInfoFile);
        QString line;
        while (in.readLineInto(&line)) {
            if (line.startsWith("model name")) {
                QStringList parts = line.split(":");
                if (parts.size() > 1) {
                    m_cpuLabel->setText(parts[1].trimmed());
                    break;
                }
            }
        }
        cpuInfoFile.close();
    }
#else
    m_cpuLabel->setText("Unknown");
#endif
    
    // Memory Information
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    QString memoryInfo = QString("%1 GB / %2 GB")
        .arg(memInfo.ullAvailPhys / (1024 * 1024 * 1024))
        .arg(memInfo.ullTotalPhys / (1024 * 1024 * 1024));
    m_memoryLabel->setText(memoryInfo);
#elif defined(Q_OS_LINUX)
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    QString memoryInfo = QString("%1 GB / %2 GB")
        .arg((memInfo.freeram * memInfo.mem_unit) / (1024 * 1024 * 1024))
        .arg((memInfo.totalram * memInfo.mem_unit) / (1024 * 1024 * 1024));
    m_memoryLabel->setText(memoryInfo);
#else
    m_memoryLabel->setText("Unknown");
#endif
    
    // Disk Space Information
    QStorageInfo storage = QStorageInfo::root();
    QString diskInfo = QString("%1 GB / %2 GB")
        .arg(storage.bytesAvailable() / (1024 * 1024 * 1024))
        .arg(storage.bytesTotal() / (1024 * 1024 * 1024));
    m_diskSpaceLabel->setText(diskInfo);
    
    // Last updated
    m_lastUpdatedLabel->setText(QString("Last updated: %1")
        .arg(QDateTime::currentDateTime().toString()));
}

// QuickStatsWidget implementation
QuickStatsWidget::QuickStatsWidget(QWidget *parent)
    : QGroupBox("Quick Statistics", parent)
{
    setupUI();
    updateStats();
}

void QuickStatsWidget::setupUI() {
    QGridLayout* layout = new QGridLayout(this);
    
    m_totalFilesLabel = new QLabel();
    m_totalSizeLabel = new QLabel();
    m_duplicateFilesLabel = new QLabel();
    m_duplicateSizeLabel = new QLabel();
    m_similarFilesLabel = new QLabel();
    m_residueFilesLabel = new QLabel();
    
    layout->addWidget(new QLabel("Total Files:"), 0, 0);
    layout->addWidget(m_totalFilesLabel, 0, 1);
    layout->addWidget(new QLabel("Total Size:"), 0, 2);
    layout->addWidget(m_totalSizeLabel, 0, 3);
    
    layout->addWidget(new QLabel("Duplicate Files:"), 1, 0);
    layout->addWidget(m_duplicateFilesLabel, 1, 1);
    layout->addWidget(new QLabel("Duplicate Size:"), 1, 2);
    layout->addWidget(m_duplicateSizeLabel, 1, 3);
    
    layout->addWidget(new QLabel("Similar Files:"), 2, 0);
    layout->addWidget(m_similarFilesLabel, 2, 1);
    layout->addWidget(new QLabel("Residue Files:"), 2, 2);
    layout->addWidget(m_residueFilesLabel, 2, 3);
    
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);
}

void QuickStatsWidget::updateStats() {
    // Placeholder values - in a real implementation, these would come from the application's data
    m_totalFilesLabel->setText("1,245,678");
    m_totalSizeLabel->setText("456.78 GB");
    m_duplicateFilesLabel->setText("12,345");
    m_duplicateSizeLabel->setText("23.45 GB");
    m_similarFilesLabel->setText("5,678");
    m_residueFilesLabel->setText("8,901");
}

// RecentScansWidget implementation
RecentScansWidget::RecentScansWidget(QWidget *parent)
    : QGroupBox("Recent Scans", parent)
{
    setupUI();
    updateScans();
}

void RecentScansWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    m_scansTable = new QTableWidget(0, 4);
    m_scansTable->setHorizontalHeaderLabels(QStringList() << "Date" << "Path" << "Status" << "Results");
    m_scansTable->horizontalHeader()->setStretchLastSection(true);
    m_scansTable->verticalHeader()->setVisible(false);
    m_scansTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scansTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    layout->addWidget(m_scansTable);
}

void RecentScansWidget::updateScans() {
    // Clear existing items
    m_scansTable->setRowCount(0);
    
    // Add sample data - in a real implementation, this would come from a database or log file
    QStringList dates = {"2023-05-15 14:30", "2023-05-14 09:15", "2023-05-12 16:45", "2023-05-10 11:20"};
    QStringList paths = {"/home/user/Documents", "C:\\Users\\John\\Pictures", "/media/data", "D:\\Projects"};
    QStringList statuses = {"Completed", "Completed", "Cancelled", "Completed"};
    QStringList results = {"125 duplicates found", "45 similar images", "89 residue files", "234 duplicates found"};
    
    for (int i = 0; i < dates.size(); ++i) {
        int row = m_scansTable->rowCount();
        m_scansTable->insertRow(row);
        
        m_scansTable->setItem(row, 0, new QTableWidgetItem(dates[i]));
        m_scansTable->setItem(row, 1, new QTableWidgetItem(paths[i]));
        m_scansTable->setItem(row, 2, new QTableWidgetItem(statuses[i]));
        m_scansTable->setItem(row, 3, new QTableWidgetItem(results[i]));
    }
    
    m_scansTable->resizeColumnsToContents();
}

// QuickAccessWidget implementation
QuickAccessWidget::QuickAccessWidget(QWidget *parent)
    : QGroupBox("Quick Access", parent)
{
    setupUI();
}

void QuickAccessWidget::setupUI() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    
    QPushButton* scanButton = new QPushButton("Scan Directory");
    QPushButton* settingsButton = new QPushButton("Settings");
    QPushButton* exportButton = new QPushButton("Export Results");
    
    layout->addWidget(scanButton);
    layout->addWidget(settingsButton);
    layout->addWidget(exportButton);
    layout->addStretch();
    
    connect(scanButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(nullptr, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            emit scanRequested(dir);
        }
    });
    
    connect(settingsButton, &QPushButton::clicked, [this]() {
        emit settingsRequested();
    });
    
    connect(exportButton, &QPushButton::clicked, [this]() {
        emit exportRequested();
    });
}