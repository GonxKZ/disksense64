#ifndef TESTS_PERF_BENCHMARK_H
#define TESTS_PERF_BENCHMARK_H

#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <memory>
#include <chrono>

// Performance benchmark suite
class Benchmark : public QObject
{
    Q_OBJECT

private slots:
    // Initialization
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // File system scanning benchmarks
    void benchmarkFileScanningSmall_data();
    void benchmarkFileScanningSmall();
    
    void benchmarkFileScanningMedium_data();
    void benchmarkFileScanningMedium();
    
    void benchmarkFileScanningLarge_data();
    void benchmarkFileScanningLarge();
    
    // Hashing benchmarks
    void benchmarkBlake3Hashing_data();
    void benchmarkBlake3Hashing();
    
    void benchmarkSha256Hashing_data();
    void benchmarkSha256Hashing();
    
    // Indexing benchmarks
    void benchmarkLSMIndexInsertion_data();
    void benchmarkLSMIndexInsertion();
    
    void benchmarkLSMIndexLookup_data();
    void benchmarkLSMIndexLookup();
    
    void benchmarkLSMIndexRangeQuery_data();
    void benchmarkLSMIndexRangeQuery();
    
    // Cache benchmarks
    void benchmarkFileCacheHit_data();
    void benchmarkFileCacheHit();
    
    void benchmarkFileCacheMiss_data();
    void benchmarkFileCacheMiss();
    
    // Database benchmarks
    void benchmarkScanDatabaseInsert_data();
    void benchmarkScanDatabaseInsert();
    
    void benchmarkScanDatabaseQuery_data();
    void benchmarkScanDatabaseQuery();
    
    // Analysis benchmarks
    void benchmarkFileTypeAnalysis_data();
    void benchmarkFileTypeAnalysis();
    
    void benchmarkFileAgeAnalysis_data();
    void benchmarkFileAgeAnalysis();
    
    void benchmarkDuplicateClustering_data();
    void benchmarkDuplicateClustering();
    
    // Visualization benchmarks
    void benchmarkTreemapGeneration_data();
    void benchmarkTreemapGeneration();
    
    void benchmarkTreemapRendering_data();
    void benchmarkTreemapRendering();
    
    // UI benchmarks
    void benchmarkUIRendering_data();
    void benchmarkUIRendering();
    
    void benchmarkUIInteraction_data();
    void benchmarkUIInteraction();

private:
    QString createTestData(int fileCount, int fileSizeKB) const;
    void generateTestFiles(const QString& dirPath, int fileCount, int fileSizeKB) const;
    qint64 measureTime(std::function<void()> func) const;
    
    std::unique_ptr<QTemporaryDir> m_testDir;
};

// High-performance benchmark runner
class BenchmarkRunner
{
public:
    static int run(int argc, char *argv[]);
    static void setupBenchmarkEnvironment();
    static void teardownBenchmarkEnvironment();
private:
    static void printBenchmarkResults(const QString& testName, qint64 duration, const QString& metrics);
};

#endif // TESTS_PERF_BENCHMARK_H