#ifndef UI_DASHBOARDTAB_H
#define UI_DASHBOARDTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QDateTime>
#include <QTimer>
#include <memory>

class SystemInfoWidget;
class QuickStatsWidget;
class RecentScansWidget;
class QuickAccessWidget;

class DashboardTab : public QWidget {
    Q_OBJECT

public:
    explicit DashboardTab(QWidget *parent = nullptr);
    ~DashboardTab() override;

public slots:
    void refreshSystemInfo();
    void refreshQuickStats();
    void refreshRecentScans();
    void refreshAll();

signals:
    void scanRequested(const QString& path);
    void settingsRequested();
    void exportRequested();

private:
    void setupUI();
    void connectSignals();
    
    SystemInfoWidget* m_systemInfoWidget;
    QuickStatsWidget* m_quickStatsWidget;
    RecentScansWidget* m_recentScansWidget;
    QuickAccessWidget* m_quickAccessWidget;
    
    QTimer* m_refreshTimer;
};

// Widget to display system information
class SystemInfoWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit SystemInfoWidget(QWidget *parent = nullptr);
    void updateInfo();

private:
    void setupUI();
    
    QLabel* m_osLabel;
    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    QLabel* m_diskSpaceLabel;
    QLabel* m_lastUpdatedLabel;
};

// Widget to display quick statistics
class QuickStatsWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit QuickStatsWidget(QWidget *parent = nullptr);
    void updateStats();

private:
    void setupUI();
    
    QLabel* m_totalFilesLabel;
    QLabel* m_totalSizeLabel;
    QLabel* m_duplicateFilesLabel;
    QLabel* m_duplicateSizeLabel;
    QLabel* m_similarFilesLabel;
    QLabel* m_residueFilesLabel;
};

// Widget to display recent scan history
class RecentScansWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit RecentScansWidget(QWidget *parent = nullptr);
    void updateScans();

private:
    void setupUI();
    
    QTableWidget* m_scansTable;
};

// Widget for quick access buttons
class QuickAccessWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit QuickAccessWidget(QWidget *parent = nullptr);

signals:
    void scanRequested(const QString& path);
    void settingsRequested();
    void exportRequested();

private:
    void setupUI();
};

#endif // UI_DASHBOARDTAB_H