#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

class Scanner;
class LSMIndex;
class TreemapWidget;
class ResultsDisplay;

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

private:
    void setupUI();
    void setupDeduplicationTab();
    void setupVisualizationTab();
    void setupResidueTab();
    void setupSimilarityTab();
    
    QTabWidget* m_tabWidget;
    
    // Deduplication tab widgets
    QWidget* m_dedupTab;
    ResultsDisplay* m_dedupResults;
    
    // Visualization tab widgets
    QWidget* m_vizTab;
    TreemapWidget* m_treemapWidget;
    ResultsDisplay* m_vizResults;
    
    // Residue detection tab widgets
    QWidget* m_residueTab;
    ResultsDisplay* m_residueResults;
    
    // Similarity detection tab widgets
    QWidget* m_similarityTab;
    ResultsDisplay* m_similarityResults;
    
    // Core components
    std::unique_ptr<Scanner> m_scanner;
    std::unique_ptr<LSMIndex> m_index;
    
    // Status tracking
    bool m_isScanning;
};

#endif // UI_MAINWINDOW_H