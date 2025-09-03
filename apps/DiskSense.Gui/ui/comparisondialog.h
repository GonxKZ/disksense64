#ifndef UI_COMPARISONDIALOG_H
#define UI_COMPARISONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <memory>
#include <QVariant>

class ComparisonDialog : public QDialog {
    Q_OBJECT

public:
    struct ScanResult {
        QString path;
        QDateTime timestamp;
        quint64 totalFiles;
        quint64 totalSize;
        quint64 duplicateFiles;
        quint64 duplicateSize;
        quint64 similarFiles;
        quint64 residueFiles;
        QVariant data;
    };

public:
    explicit ComparisonDialog(QWidget *parent = nullptr);
    ~ComparisonDialog() override;

    // Scan management
    void addScanResult(const ScanResult& result);
    void removeScanResult(int index);
    QList<ScanResult> scanResults() const;
    
    // Comparison settings
    int baselineIndex() const;
    void setBaselineIndex(int index);
    
    int comparisonIndex() const;
    void setComparisonIndex(int index);
    
    bool showFileChanges() const;
    void setShowFileChanges(bool show);
    
    bool showSizeChanges() const;
    void setShowSizeChanges(bool show);
    
    bool showTypeChanges() const;
    void setShowTypeChanges(bool show);

public slots:
    void compareScans();
    void loadScanFromFile();
    void saveComparisonReport();
    void clearResults();

private slots:
    void onBaselineChanged(int index);
    void onComparisonChanged(int index);
    void updateComparison();

private:
    void setupUI();
    void connectSignals();
    void populateScanList();
    void populateComparisonResults();
    QString formatFileSize(quint64 size) const;
    QString formatPercentage(double percentage) const;
    
    // UI components
    QComboBox* m_baselineCombo;
    QComboBox* m_comparisonCombo;
    QPushButton* m_loadButton;
    QPushButton* m_compareButton;
    QPushButton* m_saveButton;
    QPushButton* m_clearButton;
    
    // Results display
    QTabWidget* m_resultsTabs;
    QTableWidget* m_summaryTable;
    QTreeWidget* m_changesTree;
    QTextEdit* m_trendAnalysisText;
    
    // Options
    QCheckBox* m_fileChangesCheck;
    QCheckBox* m_sizeChangesCheck;
    QCheckBox* m_typeChangesCheck;
    
    // Data
    QList<ScanResult> m_scanResults;
    
    // Progress dialog
    QProgressDialog* m_progressDialog;
};

#endif // UI_COMPARISONDIALOG_H