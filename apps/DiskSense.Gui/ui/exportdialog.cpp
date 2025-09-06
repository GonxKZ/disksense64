#include "exportdialog.h"
#include <QApplication>
#include <QGroupBox>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamWriter>
#include <QPainter>
#include <QSvgGenerator>
#include <QPdfWriter>
#include <QPageSize>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
    , m_formatCombo(nullptr)
    , m_filePathEdit(nullptr)
    , m_fileNameEdit(nullptr)
    , m_browseButton(nullptr)
    , m_headersCheck(nullptr)
    , m_timestampCheck(nullptr)
    , m_widthSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_legendCheck(nullptr)
    , m_exportButton(nullptr)
    , m_cancelButton(nullptr)
    , m_progressDialog(nullptr)
{
    setupUI();
    connectSignals();
    
    // Set default values
    setFilePath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    updateFileName();
}

ExportDialog::~ExportDialog() {
}

void ExportDialog::setupUI() {
    setWindowTitle("Export Data");
    setMinimumSize(400, 300);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Format selection
    QGroupBox* formatGroup = new QGroupBox("Export Format");
    QHBoxLayout* formatLayout = new QHBoxLayout(formatGroup);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItems(QStringList() << "CSV" << "JSON" << "XML" << "PNG Image" << "SVG Image" << "PDF Report");
    formatLayout->addWidget(m_formatCombo);
    
    // File path and name
    QGroupBox* fileGroup = new QGroupBox("File Location");
    QFormLayout* fileLayout = new QFormLayout(fileGroup);
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit();
    m_browseButton = new QPushButton("Browse...");
    pathLayout->addWidget(m_filePathEdit);
    pathLayout->addWidget(m_browseButton);
    
    m_fileNameEdit = new QLineEdit();
    
    fileLayout->addRow("Directory:", pathLayout);
    fileLayout->addRow("File Name:", m_fileNameEdit);
    
    // Options based on format
    QGroupBox* optionsGroup = new QGroupBox("Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    // CSV/JSON/XML options
    QWidget* textOptionsWidget = new QWidget();
    QVBoxLayout* textOptionsLayout = new QVBoxLayout(textOptionsWidget);
    m_headersCheck = new QCheckBox("Include column headers");
    m_headersCheck->setChecked(true);
    m_timestampCheck = new QCheckBox("Include timestamp in filename");
    m_timestampCheck->setChecked(true);
    textOptionsLayout->addWidget(m_headersCheck);
    textOptionsLayout->addWidget(m_timestampCheck);
    
    // Image options
    QWidget* imageOptionsWidget = new QWidget();
    QFormLayout* imageOptionsLayout = new QFormLayout(imageOptionsWidget);
    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(100, 5000);
    m_widthSpin->setValue(800);
    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(100, 5000);
    m_heightSpin->setValue(600);
    m_legendCheck = new QCheckBox("Include legend");
    m_legendCheck->setChecked(true);
    imageOptionsLayout->addRow("Width:", m_widthSpin);
    imageOptionsLayout->addRow("Height:", m_heightSpin);
    imageOptionsLayout->addRow("", m_legendCheck);
    
    optionsLayout->addWidget(textOptionsWidget);
    optionsLayout->addWidget(imageOptionsWidget);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_exportButton = new QPushButton("Export");
    m_cancelButton = new QPushButton("Cancel");
    m_exportButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addWidget(m_cancelButton);
    
    // Add to main layout
    mainLayout->addWidget(formatGroup);
    mainLayout->addWidget(fileGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addLayout(buttonLayout);
    
    // Set initial visibility
    onFormatChanged(0);
}

void ExportDialog::connectSignals() {
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ExportDialog::onFormatChanged);
    connect(m_browseButton, &QPushButton::clicked, this, &ExportDialog::browseFile);
    connect(m_exportButton, &QPushButton::clicked, this, &ExportDialog::exportData);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_timestampCheck, &QCheckBox::toggled, this, &ExportDialog::updateFileName);
}

ExportDialog::ExportFormat ExportDialog::format() const {
    return static_cast<ExportFormat>(m_formatCombo->currentIndex());
}

void ExportDialog::setFormat(ExportFormat format) {
    m_formatCombo->setCurrentIndex(static_cast<int>(format));
}

QString ExportDialog::filePath() const {
    return m_filePathEdit->text();
}

void ExportDialog::setFilePath(const QString& path) {
    m_filePathEdit->setText(path);
}

QString ExportDialog::fileName() const {
    return m_fileNameEdit->text();
}

void ExportDialog::setFileName(const QString& name) {
    m_fileNameEdit->setText(name);
}

bool ExportDialog::includeHeaders() const {
    return m_headersCheck->isChecked();
}

void ExportDialog::setIncludeHeaders(bool include) {
    m_headersCheck->setChecked(include);
}

bool ExportDialog::includeTimestamp() const {
    return m_timestampCheck->isChecked();
}

void ExportDialog::setIncludeTimestamp(bool include) {
    m_timestampCheck->setChecked(include);
}

int ExportDialog::imageWidth() const {
    return m_widthSpin->value();
}

void ExportDialog::setImageWidth(int width) {
    m_widthSpin->setValue(width);
}

int ExportDialog::imageHeight() const {
    return m_heightSpin->value();
}

void ExportDialog::setImageHeight(int height) {
    m_heightSpin->setValue(height);
}

bool ExportDialog::includeLegend() const {
    return m_legendCheck->isChecked();
}

void ExportDialog::setIncludeLegend(bool include) {
    m_legendCheck->setChecked(include);
}

void ExportDialog::setData(const QVariant& data) {
    m_data = data;
}

QVariant ExportDialog::data() const {
    return m_data;
}

void ExportDialog::exportData() {
    if (!validateInputs()) {
        return;
    }
    
    QString fullPath = QDir(filePath()).filePath(fileName());
    
    // Show progress dialog
    m_progressDialog = new QProgressDialog("Exporting data...", "Cancel", 0, 100, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->show();
    
    // Simulate export process
    QTimer::singleShot(100, this, [this, fullPath]() {
        bool success = false;
        QString errorMessage;
        
        try {
            switch (format()) {
                case CSV: {
                    QFile file(fullPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream stream(&file);
                        
                        // Write headers if requested
                        if (includeHeaders()) {
                            stream << "Name,Size,Type,Modified\n";
                        }
                        
                        // Write data (sample data for demonstration)
                        stream << "file1.txt,1024,Text,2023-05-15\n";
                        stream << "file2.jpg,204800,Image,2023-05-16\n";
                        stream << "file3.pdf,512000,PDF,2023-05-17\n";
                        
                        file.close();
                        success = true;
                    } else {
                        errorMessage = "Failed to create CSV file";
                    }
                    break;
                }
                
                case JSON: {
                    QFile file(fullPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QJsonArray array;
                        
                        // Sample data
                        QJsonObject obj1;
                        obj1["name"] = "file1.txt";
                        obj1["size"] = 1024;
                        obj1["type"] = "Text";
                        obj1["modified"] = "2023-05-15";
                        array.append(obj1);
                        
                        QJsonObject obj2;
                        obj2["name"] = "file2.jpg";
                        obj2["size"] = 204800;
                        obj2["type"] = "Image";
                        obj2["modified"] = "2023-05-16";
                        array.append(obj2);
                        
                        QJsonObject obj3;
                        obj3["name"] = "file3.pdf";
                        obj3["size"] = 512000;
                        obj3["type"] = "PDF";
                        obj3["modified"] = "2023-05-17";
                        array.append(obj3);
                        
                        QJsonDocument doc(array);
                        file.write(doc.toJson());
                        file.close();
                        success = true;
                    } else {
                        errorMessage = "Failed to create JSON file";
                    }
                    break;
                }
                
                case XML: {
                    QFile file(fullPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QXmlStreamWriter stream(&file);
                        stream.setAutoFormatting(true);
                        stream.writeStartDocument();
                        stream.writeStartElement("files");
                        
                        // Sample data
                        stream.writeStartElement("file");
                        stream.writeTextElement("name", "file1.txt");
                        stream.writeTextElement("size", "1024");
                        stream.writeTextElement("type", "Text");
                        stream.writeTextElement("modified", "2023-05-15");
                        stream.writeEndElement(); // file
                        
                        stream.writeStartElement("file");
                        stream.writeTextElement("name", "file2.jpg");
                        stream.writeTextElement("size", "204800");
                        stream.writeTextElement("type", "Image");
                        stream.writeTextElement("modified", "2023-05-16");
                        stream.writeEndElement(); // file
                        
                        stream.writeStartElement("file");
                        stream.writeTextElement("name", "file3.pdf");
                        stream.writeTextElement("size", "512000");
                        stream.writeTextElement("type", "PDF");
                        stream.writeTextElement("modified", "2023-05-17");
                        stream.writeEndElement(); // file
                        
                        stream.writeEndElement(); // files
                        stream.writeEndDocument();
                        file.close();
                        success = true;
                    } else {
                        errorMessage = "Failed to create XML file";
                    }
                    break;
                }
                
                case PNG: {
                    QImage image(imageWidth(), imageHeight(), QImage::Format_ARGB32);
                    image.fill(Qt::white);
                    
                    QPainter painter(&image);
                    painter.setPen(Qt::black);
                    painter.setFont(QFont("Arial", 12));
                    painter.drawText(10, 20, "Sample Visualization Export");
                    painter.drawText(10, 40, QString("Size: %1x%2").arg(imageWidth()).arg(imageHeight()));
                    painter.end();
                    
                    success = image.save(fullPath, "PNG");
                    if (!success) {
                        errorMessage = "Failed to create PNG image";
                    }
                    break;
                }
                
                case SVG: {
                    QSvgGenerator generator;
                    generator.setFileName(fullPath);
                    generator.setSize(QSize(imageWidth(), imageHeight()));
                    generator.setViewBox(QRect(0, 0, imageWidth(), imageHeight()));
                    generator.setTitle("DiskSense64 Visualization");
                    generator.setDescription("Exported visualization from DiskSense64");
                    
                    QPainter painter;
                    painter.begin(&generator);
                    painter.setPen(Qt::black);
                    painter.setFont(QFont("Arial", 12));
                    painter.drawText(10, 20, "Sample SVG Visualization Export");
                    painter.drawText(10, 40, QString("Size: %1x%2").arg(imageWidth()).arg(imageHeight()));
                    painter.end();
                    
                    success = true;
                    break;
                }
                
                case PDF: {
                    QPdfWriter writer(fullPath);
                    writer.setPageSize(QPageSize(QPageSize::A4));
                    writer.setPageMargins(QMargins(20, 20, 20, 20));
                    
                    QPainter painter;
                    painter.begin(&writer);
                    painter.setFont(QFont("Arial", 12));
                    painter.drawText(100, 100, "DiskSense64 Analysis Report");
                    painter.drawText(100, 130, QString("Exported on: %1").arg(QDateTime::currentDateTime().toString()));
                    
                    // Sample data table
                    painter.drawText(100, 180, "File Analysis Results:");
                    painter.drawText(100, 200, "----------------------------------------");
                    painter.drawText(100, 220, "file1.txt     1.0 KB   Text     2023-05-15");
                    painter.drawText(100, 240, "file2.jpg   200.0 KB   Image    2023-05-16");
                    painter.drawText(100, 260, "file3.pdf   500.0 KB   PDF      2023-05-17");
                    painter.end();
                    
                    success = true;
                    break;
                }
            }
            
            // Update progress
            m_progressDialog->setValue(100);
            
            if (success) {
                QMessageBox::information(this, "Export Successful", 
                                       QString("Data successfully exported to:\n%1").arg(fullPath));
                accept();
            } else {
                QMessageBox::warning(this, "Export Failed", 
                                   QString("Failed to export data:\n%1").arg(errorMessage));
            }
        } catch (const std::exception& e) {
            m_progressDialog->close();
            QMessageBox::warning(this, "Export Failed", 
                               QString("An error occurred during export:\n%1").arg(e.what()));
        }
        
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    });
}

void ExportDialog::browseFile() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory", filePath());
    if (!dir.isEmpty()) {
        setFilePath(dir);
    }
}

void ExportDialog::onFormatChanged(int index) {
    // Show/hide options based on format
    bool isTextFormat = (index == CSV || index == JSON || index == XML);
    bool isImageFormat = (index == PNG || index == SVG || index == PDF);
    
    // Find the options group box and its child widgets
    QLayout* optionsLayout = this->layout()->itemAt(2)->widget()->layout();
    QWidget* textOptionsWidget = optionsLayout->itemAt(0)->widget();
    QWidget* imageOptionsWidget = optionsLayout->itemAt(1)->widget();
    
    textOptionsWidget->setVisible(isTextFormat);
    imageOptionsWidget->setVisible(isImageFormat);
    
    // Update file extension
    updateFileName();
}

void ExportDialog::updateFileName() {
    QString baseName = "disksense_export";
    
    if (includeTimestamp()) {
        baseName += "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    }
    
    QString extension;
    switch (format()) {
        case CSV: extension = "csv"; break;
        case JSON: extension = "json"; break;
        case XML: extension = "xml"; break;
        case PNG: extension = "png"; break;
        case SVG: extension = "svg"; break;
        case PDF: extension = "pdf"; break;
    }
    
    setFileName(baseName + "." + extension);
}

bool ExportDialog::validateInputs() {
    if (filePath().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please select a directory for export.");
        return false;
    }
    
    if (fileName().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please specify a file name for export.");
        return false;
    }
    
    QDir dir(filePath());
    if (!dir.exists()) {
        QMessageBox::warning(this, "Invalid Directory", "The specified directory does not exist.");
        return false;
    }
    
    return true;
}

QString ExportDialog::generateFileName() const {
    QString baseName = "disksense_export";
    
    if (includeTimestamp()) {
        baseName += "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    }
    
    QString extension;
    switch (format()) {
        case CSV: extension = "csv"; break;
        case JSON: extension = "json"; break;
        case XML: extension = "xml"; break;
        case PNG: extension = "png"; break;
        case SVG: extension = "svg"; break;
        case PDF: extension = "pdf"; break;
    }
    
    return baseName + "." + extension;
}