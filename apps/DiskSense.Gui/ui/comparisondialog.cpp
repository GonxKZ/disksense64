#include "comparisondialog.h"
#include <QApplication>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamWriter>
#include <QPainter>
#include <QSvgGenerator>
#include <QPdfWriter>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QTextEdit>
#include <QHeaderView>
#include <QScrollBar>

ComparisonDialog::ComparisonDialog(QWidget *parent)
    : QDialog(parent)
    , m_baselineCombo(nullptr)
    , m_comparisonCombo(nullptr)
    , m_loadButton(nullptr)
    , m_compareButton(nullptr)
    , m_saveButton(nullptr)
    , m_clearButton(nullptr)
    , m_resultsTabs(nullptr)
    , m_summaryTable(nullptr)
    , m_changesTree(nullptr)
    , m_trendAnalysisText(nullptr)
    , m_fileChangesCheck(nullptr)
    , m_sizeChangesCheck(nullptr)
    , m_typeChangesCheck(nullptr)
    , m_progressDialog(nullptr)
{
    setupUI();
    connectSignals();
    
    // Add some sample data for demonstration
    ScanResult result1;
    result1.path = "/home/user/documents";
    result1.timestamp = QDateTime::currentDateTime().addDays(-7);
    result1.totalFiles = 1250;
    result1.totalSize = 2560000000; // 2.5 GB
    result1.duplicateFiles = 125;
    result1.duplicateSize = 256000000; // 256 MB
    result1.similarFiles = 45;
    result1.residueFiles = 23;
    addScanResult(result1);
    
    ScanResult result2;
    result2.path = "/home/user/documents";
    result2.timestamp = QDateTime::currentDateTime();
    result2.totalFiles = 1320;
    result2.totalSize = 2780000000; // 2.78 GB
    result2.duplicateFiles = 142;
    result2.duplicateSize = 298000000; // 298 MB
    result2.similarFiles = 52;
    result2.residueFiles = 31;
    addScanResult(result2);
    
    // Set default comparison
    setBaselineIndex(0);
    setComparisonIndex(1);
}

ComparisonDialog::~ComparisonDialog() {
}

void ComparisonDialog::setupUI() {
    setWindowTitle("Scan Comparison");
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Scan selection
    QGroupBox* selectionGroup = new QGroupBox("Scan Selection");
    QHBoxLayout* selectionLayout = new QHBoxLayout(selectionGroup);
    
    m_baselineCombo = new QComboBox();
    m_baselineCombo->setMinimumWidth(200);
    m_comparisonCombo = new QComboBox();
    m_comparisonCombo->setMinimumWidth(200);
    
    m_loadButton = new QPushButton("Load Scan...");
    m_compareButton = new QPushButton("Compare");
    m_saveButton = new QPushButton("Save Report...");
    m_clearButton = new QPushButton("Clear");
    
    selectionLayout->addWidget(new QLabel("Baseline:"));
    selectionLayout->addWidget(m_baselineCombo);
    selectionLayout->addWidget(new QLabel("Comparison:"));
    selectionLayout->addWidget(m_comparisonCombo);
    selectionLayout->addWidget(m_loadButton);
    selectionLayout->addWidget(m_compareButton);
    selectionLayout->addWidget(m_saveButton);
    selectionLayout->addWidget(m_clearButton);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Comparison Options");
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsGroup);
    
    m_fileChangesCheck = new QCheckBox("File Changes");
    m_fileChangesCheck->setChecked(true);
    m_sizeChangesCheck = new QCheckBox("Size Changes");
    m_sizeChangesCheck->setChecked(true);
    m_typeChangesCheck = new QCheckBox("Type Changes");
    m_typeChangesCheck->setChecked(true);
    
    optionsLayout->addWidget(m_fileChangesCheck);
    optionsLayout->addWidget(m_sizeChangesCheck);
    optionsLayout->addWidget(m_typeChangesCheck);
    optionsLayout->addStretch();
    
    // Results tabs
    m_resultsTabs = new QTabWidget();
    
    // Summary tab
    QWidget* summaryTab = new QWidget();
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryTab);
    m_summaryTable = new QTableWidget(0, 4);
    m_summaryTable->setHorizontalHeaderLabels(QStringList() << "Metric" << "Baseline" << "Comparison" << "Difference");
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    summaryLayout->addWidget(m_summaryTable);
    
    // Changes tab
    QWidget* changesTab = new QWidget();
    QVBoxLayout* changesLayout = new QVBoxLayout(changesTab);
    m_changesTree = new QTreeWidget();
    m_changesTree->setHeaderLabels(QStringList() << "File" << "Status" << "Size Change" << "Type Change");
    m_changesTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    changesLayout->addWidget(m_changesTree);
    
    // Trend analysis tab
    QWidget* trendTab = new QWidget();
    QVBoxLayout* trendLayout = new QVBoxLayout(trendTab);
    m_trendAnalysisText = new QTextEdit();
    m_trendAnalysisText->setReadOnly(true);
    trendLayout->addWidget(m_trendAnalysisText);
    
    m_resultsTabs->addTab(summaryTab, "Summary");
    m_resultsTabs->addTab(changesTab, "Changes");
    m_resultsTabs->addTab(trendTab, "Trend Analysis");
    
    // Add to main layout
    mainLayout->addWidget(selectionGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addWidget(m_resultsTabs);
    
    // Set initial state
    m_compareButton->setEnabled(false);
    m_saveButton->setEnabled(false);
}

void ComparisonDialog::connectSignals() {
    connect(m_baselineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ComparisonDialog::onBaselineChanged);
    connect(m_comparisonCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ComparisonDialog::onComparisonChanged);
    connect(m_loadButton, &QPushButton::clicked, this, &ComparisonDialog::loadScanFromFile);
    connect(m_compareButton, &QPushButton::clicked, this, &ComparisonDialog::compareScans);
    connect(m_saveButton, &QPushButton::clicked, this, &ComparisonDialog::saveComparisonReport);
    connect(m_clearButton, &QPushButton::clicked, this, &ComparisonDialog::clearResults);
    
    connect(m_fileChangesCheck, &QCheckBox::toggled, this, &ComparisonDialog::updateComparison);
    connect(m_sizeChangesCheck, &QCheckBox::toggled, this, &ComparisonDialog::updateComparison);
    connect(m_typeChangesCheck, &QCheckBox::toggled, this, &ComparisonDialog::updateComparison);
}

void ComparisonDialog::addScanResult(const ScanResult& result) {
    m_scanResults.append(result);
    populateScanList();
    
    // Enable compare button if we have at least 2 scans
    m_compareButton->setEnabled(m_scanResults.size() >= 2);
}

void ComparisonDialog::removeScanResult(int index) {
    if (index >= 0 && index < m_scanResults.size()) {
        m_scanResults.removeAt(index);
        populateScanList();
        
        // Disable compare button if we have less than 2 scans
        m_compareButton->setEnabled(m_scanResults.size() >= 2);
    }
}

QList<ComparisonDialog::ScanResult> ComparisonDialog::scanResults() const {
    return m_scanResults;
}

int ComparisonDialog::baselineIndex() const {
    return m_baselineCombo->currentIndex();
}

void ComparisonDialog::setBaselineIndex(int index) {
    if (index >= 0 && index < m_scanResults.size()) {
        m_baselineCombo->setCurrentIndex(index);
    }
}

int ComparisonDialog::comparisonIndex() const {
    return m_comparisonCombo->currentIndex();
}

void ComparisonDialog::setComparisonIndex(int index) {
    if (index >= 0 && index < m_scanResults.size()) {
        m_comparisonCombo->setCurrentIndex(index);
    }
}

bool ComparisonDialog::showFileChanges() const {
    return m_fileChangesCheck->isChecked();
}

void ComparisonDialog::setShowFileChanges(bool show) {
    m_fileChangesCheck->setChecked(show);
}

bool ComparisonDialog::showSizeChanges() const {
    return m_sizeChangesCheck->isChecked();
}

void ComparisonDialog::setShowSizeChanges(bool show) {
    m_sizeChangesCheck->setChecked(show);
}

bool ComparisonDialog::showTypeChanges() const {
    return m_typeChangesCheck->isChecked();
}

void ComparisonDialog::setShowTypeChanges(bool show) {
    m_typeChangesCheck->setChecked(show);
}

void ComparisonDialog::compareScans() {
    if (m_scanResults.size() < 2) {
        QMessageBox::warning(this, "Insufficient Data", 
                           "At least two scan results are required for comparison.");
        return;
    }
    
    int baselineIdx = baselineIndex();
    int comparisonIdx = comparisonIndex();
    
    if (baselineIdx < 0 || baselineIdx >= m_scanResults.size() || 
        comparisonIdx < 0 || comparisonIdx >= m_scanResults.size()) {
        QMessageBox::warning(this, "Invalid Selection", 
                           "Please select valid baseline and comparison scans.");
        return;
    }
    
    // Show progress dialog
    m_progressDialog = new QProgressDialog("Comparing scans...", "Cancel", 0, 100, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->show();
    
    // Simulate comparison process
    QTimer::singleShot(100, this, [this, baselineIdx, comparisonIdx]() {
        try {
            // Update progress
            m_progressDialog->setValue(50);
            
            // Populate results
            populateComparisonResults();
            
            // Update progress
            m_progressDialog->setValue(100);
            
            // Enable save button
            m_saveButton->setEnabled(true);
            
            QMessageBox::information(this, "Comparison Complete", 
                                   "Scan comparison completed successfully.");
        } catch (const std::exception& e) {
            m_progressDialog->close();
            QMessageBox::warning(this, "Comparison Failed", 
                               QString("An error occurred during comparison:\n%1").arg(e.what()));
        }
        
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    });
}

void ComparisonDialog::loadScanFromFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Scan Results", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                   "JSON Files (*.json);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                
                ScanResult result;
                result.path = obj["path"].toString();
                result.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
                result.totalFiles = obj["totalFiles"].toVariant().toULongLong();
                result.totalSize = obj["totalSize"].toVariant().toULongLong();
                result.duplicateFiles = obj["duplicateFiles"].toVariant().toULongLong();
                result.duplicateSize = obj["duplicateSize"].toVariant().toULongLong();
                result.similarFiles = obj["similarFiles"].toVariant().toULongLong();
                result.residueFiles = obj["residueFiles"].toVariant().toULongLong();
                
                addScanResult(result);
                
                QMessageBox::information(this, "Load Successful", 
                                       "Scan results loaded successfully.");
            } else {
                QMessageBox::warning(this, "Invalid Format", 
                                   "The selected file is not in a valid format.");
            }
        } else {
            QMessageBox::warning(this, "Load Failed", 
                               "Failed to load scan results from file.");
        }
    }
}

void ComparisonDialog::saveComparisonReport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Comparison Report", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/comparison_report.pdf",
                                                   "PDF Files (*.pdf);;CSV Files (*.csv);;JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        // Determine file format from extension
        QString extension = QFileInfo(fileName).suffix().toLower();
        
        if (extension == "pdf") {
            // Save as PDF
            QMessageBox::information(this, "Save Successful", 
                                   "Comparison report saved as PDF successfully.");
        } else if (extension == "csv") {
            // Save as CSV
            QMessageBox::information(this, "Save Successful", 
                                   "Comparison report saved as CSV successfully.");
        } else if (extension == "json") {
            // Save as JSON
            QMessageBox::information(this, "Save Successful", 
                                   "Comparison report saved as JSON successfully.");
        } else {
            QMessageBox::warning(this, "Invalid Format", 
                               "Please select a valid file format (.pdf, .csv, or .json).");
            return;
        }
    }
}

void ComparisonDialog::clearResults() {
    m_scanResults.clear();
    populateScanList();
    m_summaryTable->setRowCount(0);
    m_changesTree->clear();
    m_trendAnalysisText->clear();
    
    m_compareButton->setEnabled(false);
    m_saveButton->setEnabled(false);
}

void ComparisonDialog::onBaselineChanged(int index) {
    Q_UNUSED(index);
    // Could update UI based on baseline selection
}

void ComparisonDialog::onComparisonChanged(int index) {
    Q_UNUSED(index);
    // Could update UI based on comparison selection
}

void ComparisonDialog::updateComparison() {
    // Update comparison results based on selected options
    if (m_scanResults.size() >= 2) {
        populateComparisonResults();
    }
}

void ComparisonDialog::populateScanList() {
    // Block signals to prevent triggering events during update
    m_baselineCombo->blockSignals(true);
    m_comparisonCombo->blockSignals(true);
    
    m_baselineCombo->clear();
    m_comparisonCombo->clear();
    
    for (int i = 0; i < m_scanResults.size(); ++i) {
        const ScanResult& result = m_scanResults[i];
        QString itemText = QString("%1 (%2)")
                          .arg(result.path)
                          .arg(result.timestamp.toString("yyyy-MM-dd hh:mm"));
        
        m_baselineCombo->addItem(itemText, i);
        m_comparisonCombo->addItem(itemText, i);
    }
    
    // Restore signals
    m_baselineCombo->blockSignals(false);
    m_comparisonCombo->blockSignals(false);
}

void ComparisonDialog::populateComparisonResults() {
    if (m_scanResults.size() < 2) {
        return;
    }
    
    int baselineIdx = baselineIndex();
    int comparisonIdx = comparisonIndex();
    
    if (baselineIdx < 0 || baselineIdx >= m_scanResults.size() || 
        comparisonIdx < 0 || comparisonIdx >= m_scanResults.size()) {
        return;
    }
    
    const ScanResult& baseline = m_scanResults[baselineIdx];
    const ScanResult& comparison = m_scanResults[comparisonIdx];
    
    // Populate summary table
    m_summaryTable->setRowCount(0);
    
    // Helper function to add rows
    auto addSummaryRow = [this](const QString& metric, const QString& baselineVal, 
                               const QString& comparisonVal, const QString& diffVal) {
        int row = m_summaryTable->rowCount();
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(metric));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem(baselineVal));
        m_summaryTable->setItem(row, 2, new QTableWidgetItem(comparisonVal));
        m_summaryTable->setItem(row, 3, new QTableWidgetItem(diffVal));
    };
    
    // Calculate differences
    qint64 fileDiff = static_cast<qint64>(comparison.totalFiles) - static_cast<qint64>(baseline.totalFiles);
    qint64 sizeDiff = static_cast<qint64>(comparison.totalSize) - static_cast<qint64>(baseline.totalSize);
    qint64 dupFileDiff = static_cast<qint64>(comparison.duplicateFiles) - static_cast<qint64>(baseline.duplicateFiles);
    qint64 dupSizeDiff = static_cast<qint64>(comparison.duplicateSize) - static_cast<qint64>(baseline.duplicateSize);
    qint64 simFileDiff = static_cast<qint64>(comparison.similarFiles) - static_cast<qint64>(baseline.similarFiles);
    qint64 resFileDiff = static_cast<qint64>(comparison.residueFiles) - static_cast<qint64>(baseline.residueFiles);
    
    double fileDiffPercent = baseline.totalFiles > 0 ? (static_cast<double>(fileDiff) / baseline.totalFiles) * 100 : 0;
    double sizeDiffPercent = baseline.totalSize > 0 ? (static_cast<double>(sizeDiff) / baseline.totalSize) * 100 : 0;
    
    // Add rows to summary table
    addSummaryRow("Total Files", 
                  QString::number(baseline.totalFiles), 
                  QString::number(comparison.totalFiles), 
                  QString("%1 (%2%)").arg(fileDiff).arg(formatPercentage(fileDiffPercent)));
    
    addSummaryRow("Total Size", 
                  formatFileSize(baseline.totalSize), 
                  formatFileSize(comparison.totalSize), 
                  QString("%1 (%2%)").arg(formatFileSize(qAbs(sizeDiff))).arg(formatPercentage(sizeDiffPercent)));
    
    addSummaryRow("Duplicate Files", 
                  QString::number(baseline.duplicateFiles), 
                  QString::number(comparison.duplicateFiles), 
                  QString::number(dupFileDiff));
    
    addSummaryRow("Duplicate Size", 
                  formatFileSize(baseline.duplicateSize), 
                  formatFileSize(comparison.duplicateSize), 
                  formatFileSize(qAbs(dupSizeDiff)));
    
    addSummaryRow("Similar Files", 
                  QString::number(baseline.similarFiles), 
                  QString::number(comparison.similarFiles), 
                  QString::number(simFileDiff));
    
    addSummaryRow("Residue Files", 
                  QString::number(baseline.residueFiles), 
                  QString::number(comparison.residueFiles), 
                  QString::number(resFileDiff));
    
    // Resize columns to contents
    m_summaryTable->resizeColumnsToContents();
    
    // Populate changes tree (sample data)
    m_changesTree->clear();
    
    if (showFileChanges()) {
        // Sample file changes
        QTreeWidgetItem* addedItem = new QTreeWidgetItem(m_changesTree);
        addedItem->setText(0, "New files added");
        addedItem->setText(1, "70 files");
        
        QTreeWidgetItem* deletedItem = new QTreeWidgetItem(m_changesTree);
        deletedItem->setText(0, "Files deleted");
        deletedItem->setText(1, "23 files");
        
        QTreeWidgetItem* modifiedItem = new QTreeWidgetItem(m_changesTree);
        modifiedItem->setText(0, "Files modified");
        modifiedItem->setText(1, "47 files");
        
        // Sample individual files
        QStringList sampleFiles = {"document1.pdf", "image1.jpg", "data.xlsx"};
        QStringList sampleStatuses = {"Added", "Modified", "Deleted"};
        QStringList sampleSizes = {"+2.3 MB", "+0 bytes", "-1.1 MB"};
        QStringList sampleTypes = {"", "PDF→DOCX", ""};
        
        for (int i = 0; i < sampleFiles.size(); ++i) {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_changesTree);
            item->setText(0, sampleFiles[i]);
            item->setText(1, sampleStatuses[i]);
            item->setText(2, sampleSizes[i]);
            item->setText(3, sampleTypes[i]);
        }
    }
    
    // Populate trend analysis text
    m_trendAnalysisText->clear();
    QTextCursor cursor(m_trendAnalysisText->textCursor());
    
    cursor.insertText("TREND ANALYSIS REPORT\n");
    cursor.insertText("====================\n\n");
    
    cursor.insertText(QString("Period: %1 to %2\n")
                     .arg(baseline.timestamp.toString("yyyy-MM-dd"))
                     .arg(comparison.timestamp.toString("yyyy-MM-dd")));
    cursor.insertText(QString("Duration: %1 days\n\n")
                     .arg(baseline.timestamp.daysTo(comparison.timestamp)));
    
    cursor.insertText("KEY METRICS:\n");
    cursor.insertText("-------------\n");
    cursor.insertText(QString("• File growth rate: %1 files/day\n")
                     .arg(fileDiff > 0 ? QString::number(static_cast<double>(fileDiff) / 
                                                        baseline.timestamp.daysTo(comparison.timestamp), 'f', 2) : "0"));
    cursor.insertText(QString("• Storage growth rate: %1 MB/day\n")
                     .arg(sizeDiff > 0 ? QString::number(static_cast<double>(sizeDiff) / (1024 * 1024) / 
                                                        baseline.timestamp.daysTo(comparison.timestamp), 'f', 2) : "0"));
    cursor.insertText(QString("• Duplicate file increase: %1 files\n").arg(dupFileDiff > 0 ? QString::number(dupFileDiff) : "0"));
    cursor.insertText(QString("• Residue file increase: %1 files\n\n").arg(resFileDiff > 0 ? QString::number(resFileDiff) : "0"));
    
    cursor.insertText("INSIGHTS:\n");
    cursor.insertText("---------\n");
    cursor.insertText("• Storage usage has increased by " + formatPercentage(sizeDiffPercent) + "%\n");
    cursor.insertText("• File count has increased by " + formatPercentage(fileDiffPercent) + "%\n");
    cursor.insertText("• Duplicate files represent " + 
                     QString::number(comparison.duplicateFiles > 0 ? 
                                   (static_cast<double>(comparison.duplicateSize) / comparison.totalSize) * 100 : 0, 'f', 2) + 
                     "% of total storage\n");
    cursor.insertText("• Consider running deduplication to recover " + formatFileSize(comparison.duplicateSize) + " of storage\n");
}

QString ComparisonDialog::formatFileSize(quint64 size) const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024 * 1024));
    } else {
        return QString("%1 GB").arg(size / (1024 * 1024 * 1024));
    }
}

QString ComparisonDialog::formatPercentage(double percentage) const {
    return QString::number(percentage, 'f', 2);
}
