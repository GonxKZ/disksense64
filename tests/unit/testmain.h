#ifndef TESTS_UNIT_TESTMAIN_H
#define TESTS_UNIT_TESTMAIN_H

#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <memory>

// Test suite for the DiskSense64 application
class TestMain : public QObject
{
    Q_OBJECT

private slots:
    // Initialization
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Core functionality tests
    void testFileScanner();
    void testFileScanner_data();
    
    void testTreemapLayout();
    void testTreemapLayout_data();
    
    void testLSMIndex();
    void testLSMIndex_data();
    
    void testFileCache();
    void testFileCache_data();
    
    void testScanDatabase();
    void testScanDatabase_data();
    
    void testAnalysisUtils();
    void testAnalysisUtils_data();
    
    void testSecurityManager();
    void testSecurityManager_data();
    
    void testNetworkManager();
    void testNetworkManager_data();
    
    void testTaskScheduler();
    void testTaskScheduler_data();

    // Compression tests
    void testCompressionAnalysis();
    void testCompressionAnalysis_data();
    void testCompressionExtraction();
    void testCompressionExtraction_data();
    
    // UI component tests
    void testTreemapWidget();
    void testTreemapWidget_data();
    
    void testResultsDisplay();
    void testResultsDisplay_data();
    
    void testDashboardTab();
    void testDashboardTab_data();
    
    void testSettingsDialog();
    void testSettingsDialog_data();
    
    void testFileExplorer();
    void testFileExplorer_data();
    
    void testAdvancedFilterWidget();
    void testAdvancedFilterWidget_data();
    
    void testExportDialog();
    void testExportDialog_data();
    
    void testComparisonDialog();
    void testComparisonDialog_data();
    
    void testVisualizationWidget();
    void testVisualizationWidget_data();
    
    void testAutomationDialog();
    void testAutomationDialog_data();
    
    void testNetworkDialog();
    void testNetworkDialog_data();
    
    void testSecurityDialog();
    void testSecurityDialog_data();
    
    // Performance tests
    void testPerformanceScanning_data();
    void testPerformanceScanning();
    
    void testPerformanceIndexing_data();
    void testPerformanceIndexing();
    
    void testPerformanceVisualization_data();
    void testPerformanceVisualization();
    
    // Integration tests
    void testIntegrationScanAndDedupe();
    void testIntegrationScanAndVisualize();
    void testIntegrationScanAndAnalyze();
    void testIntegrationFullWorkflow();
    
    // Edge case tests
    void testEdgeCasesEmptyDirectory();
    void testEdgeCasesLargeFiles();
    void testEdgeCasesSpecialCharacters();
    void testEdgeCasesNetworkDrives();
    void testEdgeCasesEncryptedFiles();
    
    // Error handling tests
    void testErrorHandlingInvalidPaths();
    void testErrorHandlingPermissionDenied();
    void testErrorHandlingDiskFull();
    void testErrorHandlingCorruptedFiles();
    void testErrorHandlingNetworkFailures();

    // New unit tests
    void testChartData();
    void testPluginValidatorSettings();
    void testResidueAnalyze();
    void testResidueApply();
    
private:
    QString createTestDirectory() const;
    void populateTestDirectory(const QString& dirPath) const;
    void cleanTestDirectory(const QString& dirPath) const;
    
    std::unique_ptr<QTemporaryDir> m_testDir;
};

// Test runner
class TestRunner
{
public:
    static int run(int argc, char *argv[]);
};

void setupTestEnvironment();
void teardownTestEnvironment();

#endif // TESTS_UNIT_TESTMAIN_H
