#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include "core/ops/cleanup.h"

class Scanner;
class LSMIndex;
class TreemapWidget;
class ResultsDisplay;
class DashboardTab;
class SettingsDialog;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QRadioButton;
class ChartWidget;
struct SecureDeleteOptions;
class QTableWidget;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onScanDirectory();
    void onCancelScan();
    void onUpdateStatus(const QString& message);
    void onUpdateProgress(int value);
    void onResidueDetect();
    void onExportResults();
    void onGenerateVisualization();
    void onFindDuplicates();
    void onApplyDedupe();
    void onResidueApply();
    void onDashboardScanRequested(const QString& path);
    void onDashboardSettingsRequested();
    void onDashboardExportRequested();
    void onSettingsRequested();

private:
    void setupUI();
    void setupDashboardTab();
    void setupDeduplicationTab();
    void setupVisualizationTab();
    void setupResidueTab();
    void setupSimilarityTab();
    
    QTabWidget* m_tabWidget;
    
    // Dashboard tab widgets
    DashboardTab* m_dashboardTab;
    
    // Deduplication tab widgets
    QWidget* m_dedupTab;
    ResultsDisplay* m_dedupResults;
    
    // Visualization tab widgets
    QWidget* m_vizTab;
    TreemapWidget* m_treemapWidget;
    ResultsDisplay* m_vizResults;
    QLineEdit* m_vizDirEdit;
    ChartWidget* m_chartWidget;
    class QComboBox* m_chartTypeCombo;
    QLineEdit* m_vizFilterEdit;
    class SunburstWidget* m_sunburstWidget;
    class QCheckBox* m_showSunburstCheck;
    
    // Residue detection tab widgets
    QWidget* m_residueTab;
    ResultsDisplay* m_residueResults;
    class QLineEdit* m_residueDirEdit;
    QLineEdit* m_residueExtEdit;
    QSpinBox* m_residueDaysSpin;
    QCheckBox* m_residueRemoveEmptyCheck;
    QCheckBox* m_residueSimulateCheck;

    // Dedup controls
    QCheckBox* m_dedupeFullHashCheck;
    QSpinBox* m_dedupeMinSizeSpin;
    QRadioButton* m_dedupeSimulateRadio;
    QRadioButton* m_dedupeHardlinkRadio;
    QRadioButton* m_dedupeMoveRadio;
    QRadioButton* m_dedupeDeleteRadio;
    QLineEdit* m_dedupeDirEdit;
    QCheckBox* m_useMftCheck;
    QCheckBox* m_liveMonitorCheck;

    // Monitor runtime
    class FsMonitor* m_fsMonitor;

    // Safety banner/widgets
    class QLabel* m_safetyBanner;
    class QCheckBox* m_safetyToggle; // read-only indicator (locked ON)
    
    // Similarity detection tab widgets
    QWidget* m_similarityTab;
    ResultsDisplay* m_similarityResults;

    // Health tab (SMART)
    QWidget* m_healthTab;
    ResultsDisplay* m_healthResults;

    // Security/YARA tab
    QWidget* m_securityTab;
    ResultsDisplay* m_securityResults;
    QLineEdit* m_yaraRulesEdit;

    // Top N tab
    QWidget* m_topNTab;
    QTableWidget* m_topNTable;
    QLineEdit* m_topNDirEdit;

    // Automation tab
    QWidget* m_automationTab;
    QTableWidget* m_tasksTable;
    QTimer* m_schedulerTimer;

    // Remote tab
    QWidget* m_remoteTab;
    QLineEdit* m_remoteHostEdit;
    QLineEdit* m_remotePathEdit;
    ResultsDisplay* m_remoteResults;

    // Search tab
    QWidget* m_searchTab;
    QTableWidget* m_searchTable;
    QLineEdit* m_searchEdit;

    // Trends tab
    QWidget* m_trendsTab;
    ChartWidget* m_trendsChart;
    class QPushButton* m_trendsRefresh;
    
    // Settings dialog
    SettingsDialog* m_settingsDialog;
    
    // Core components
    std::unique_ptr<Scanner> m_scanner;
    std::unique_ptr<LSMIndex> m_index;
    
    // Status tracking
    bool m_isScanning;

    // Cached data
    CleanupReport m_lastCleanupReport;
};

#endif // UI_MAINWINDOW_H
