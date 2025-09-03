#ifndef UI_EXPORTDIALOG_H
#define UI_EXPORTDIALOG_H

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
#include <QFileDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <memory>

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    enum ExportFormat {
        CSV,
        JSON,
        XML,
        PNG,
        SVG,
        PDF
    };

public:
    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog() override;

    // Export settings
    ExportFormat format() const;
    void setFormat(ExportFormat format);
    
    QString filePath() const;
    void setFilePath(const QString& path);
    
    QString fileName() const;
    void setFileName(const QString& name);
    
    bool includeHeaders() const;
    void setIncludeHeaders(bool include);
    
    bool includeTimestamp() const;
    void setIncludeTimestamp(bool include);
    
    int imageWidth() const;
    void setImageWidth(int width);
    
    int imageHeight() const;
    void setImageHeight(int height);
    
    bool includeLegend() const;
    void setIncludeLegend(bool include);
    
    // Export data
    void setData(const QVariant& data);
    QVariant data() const;

public slots:
    void exportData();
    void browseFile();

private slots:
    void onFormatChanged(int index);
    void updateFileName();

private:
    void setupUI();
    void connectSignals();
    bool validateInputs();
    QString generateFileName() const;
    
    // UI components
    QComboBox* m_formatCombo;
    QLineEdit* m_filePathEdit;
    QLineEdit* m_fileNameEdit;
    QPushButton* m_browseButton;
    
    // CSV/JSON/XML options
    QCheckBox* m_headersCheck;
    QCheckBox* m_timestampCheck;
    
    // Image options
    QSpinBox* m_widthSpin;
    QSpinBox* m_heightSpin;
    QCheckBox* m_legendCheck;
    
    // Buttons
    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
    
    // Data to export
    QVariant m_data;
    
    // Progress dialog for long operations
    QProgressDialog* m_progressDialog;
};

#endif // UI_EXPORTDIALOG_H