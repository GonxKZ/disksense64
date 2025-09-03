#include "mainwindow.h"
#include "ui/dashboardtab.h"
#include "ui/settingsdialog.h"
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QSplitter>
#include <QDir>
#include <QThread>
#include <QTimer>
#include <QSpinBox>
#include <QDebug>

#include "components/treemapwidget.h"
#include "components/resultsdisplay.h"
#include "core/scan/scanner.h"
#include "core/index/lsm_index.h"
#include "libs/utils/utils.h"

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_dashboardTab(nullptr)
    , m_dedupTab(nullptr)
    , m_vizTab(nullptr)
    , m_treemapWidget(nullptr)
    , m_residueTab(nullptr)
    , m_similarityTab(nullptr)
    , m_settingsDialog(nullptr)
    , m_scanner(std::make_unique<Scanner>())
    , m_index(std::make_unique<LSMIndex>("disksense_index"))
    , m_isScanning(false)
{
    setupUI();
    
    // Set up status bar
    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() {
    if (m_scanner) {
        m_scanner->cancel();
    }
}

void MainWindow::setupUI() {\n    setWindowTitle(\"DiskSense64 - Disk Analysis Suite\");\n    resize(1200, 800);\n    \n    // Create central widget and layout\n    QWidget* centralWidget = new QWidget(this);\n    setCentralWidget(centralWidget);\n    \n    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);\n    \n    // Create tab widget\n    m_tabWidget = new QTabWidget(this);\n    \n    // Setup all tabs\n    setupDashboardTab();\n    setupDeduplicationTab();\n    setupVisualizationTab();\n    setupResidueTab();\n    setupSimilarityTab();\n    \n    // Add tabs to tab widget\n    m_tabWidget->addTab(m_dashboardTab, \"Dashboard\");\n    m_tabWidget->addTab(m_dedupTab, \"Deduplication\");\n    m_tabWidget->addTab(m_vizTab, \"Visualization\");\n    m_tabWidget->addTab(m_residueTab, \"Residue Detection\");\n    m_tabWidget->addTab(m_similarityTab, \"Similarity Detection\");\n    \n    mainLayout->addWidget(m_tabWidget);\n    \n    // Create menu bar\n    QMenuBar* menuBar = new QMenuBar(this);\n    setMenuBar(menuBar);\n    \n    QMenu* fileMenu = menuBar->addMenu(\"&File\");\n    QAction* scanAction = fileMenu->addAction(\"&Scan Directory...\");\n    scanAction->setShortcut(QKeySequence(\"Ctrl+S\"));\n    connect(scanAction, &QAction::triggered, this, &MainWindow::onScanDirectory);\n    \n    QAction* settingsAction = fileMenu->addAction(\"&Settings...\");\n    settingsAction->setShortcut(QKeySequence(\"Ctrl+,\"));\n    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsRequested);\n    \n    QAction* exitAction = fileMenu->addAction(\"E&xit\");\n    exitAction->setShortcut(QKeySequence(\"Ctrl+Q\"));\n    connect(exitAction, &QAction::triggered, this, &QApplication::quit);\n    \n    QMenu* helpMenu = menuBar->addMenu(\"&Help\");\n    QAction* aboutAction = helpMenu->addAction(\"&About\");\n    connect(aboutAction, &QAction::triggered, [this]() {\n        QMessageBox::about(this, \"About DiskSense64\", \n                          \"DiskSense64 - Cross-Platform Disk Analysis Suite\\n\\n\"\n                          \"A comprehensive disk analysis tool with:\\n\"\n                          \"- Exact File Deduplication\\n\"\n                          \"- Disk Space Visualization\\n\"\n                          \"- Residue Detection and Cleanup\\n\"\n                          \"- Perceptual Duplicate Detection\");\n    });\n    \n    // Create toolbar\n    QToolBar* toolBar = addToolBar(\"Main\");\n    toolBar->addAction(scanAction);\n    \n    QPushButton* scanButton = new QPushButton(\"Scan Directory\");\n    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);\n    toolBar->addWidget(scanButton);\n    \n    QPushButton* cancelButton = new QPushButton(\"Cancel\");\n    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);\n    toolBar->addWidget(cancelButton);\n    \n    // Create status bar widgets\n    QProgressBar* progressBar = new QProgressBar();\n    progressBar->setRange(0, 100);\n    progressBar->setValue(0);\n    progressBar->setFixedWidth(200);\n    statusBar()->addPermanentWidget(progressBar);\n}

void MainWindow::setupDashboardTab() {
    m_dashboardTab = new DashboardTab();
    
    // Connect signals
    connect(m_dashboardTab, &DashboardTab::scanRequested, 
            this, &MainWindow::onDashboardScanRequested);
    connect(m_dashboardTab, &DashboardTab::settingsRequested, 
            this, &MainWindow::onDashboardSettingsRequested);
    connect(m_dashboardTab, &DashboardTab::exportRequested, 
            this, &MainWindow::onDashboardExportRequested);
}

void MainWindow::onDashboardScanRequested(const QString& path) {
    // Switch to the deduplication tab and start scanning
    m_tabWidget->setCurrentWidget(m_dedupTab);
    // In a real implementation, we would set the path in the deduplication tab
    // and trigger the scan
    QMessageBox::information(this, "Scan Requested", 
                            QString("Scan requested for: %1\n\n"
                                   "In a full implementation, this would switch to the Deduplication tab "
                                   "and start scanning the selected directory.").arg(path));
}

void MainWindow::onDashboardSettingsRequested() {
    // In a real implementation, this would open the settings dialog
    QMessageBox::information(this, "Settings", 
                            "Settings dialog would open here in a full implementation.");
}

void MainWindow::onDashboardExportRequested() {
    // In a real implementation, this would open the export dialog
    QMessageBox::information(this, "Export", 
                            "Export dialog would open here in a full implementation.");
}

void MainWindow::onSettingsRequested() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    
    m_settingsDialog->loadSettings();
    m_settingsDialog->exec();
}

void MainWindow::setupDeduplicationTab() {
    m_dedupTab = new QWidget();
    m_dedupResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_dedupTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QLineEdit* dirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [dirEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            dirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(dirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Scan Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    QCheckBox* useMftCheck = new QCheckBox("Use MFT Reader (Windows only, requires admin)");
    QCheckBox* followLinksCheck = new QCheckBox("Follow Reparse Points/Symlinks");
    QCheckBox* computeHeadTailCheck = new QCheckBox("Compute Head/Tail Signatures");
    computeHeadTailCheck->setChecked(true);
    QCheckBox* computeFullHashCheck = new QCheckBox("Compute Full File Hash (Slow)");
    
    optionsLayout->addWidget(useMftCheck);
    optionsLayout->addWidget(followLinksCheck);
    optionsLayout->addWidget(computeHeadTailCheck);
    optionsLayout->addWidget(computeFullHashCheck);
    
    // Action options
    QGroupBox* actionGroup = new QGroupBox("Action");
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
    QRadioButton* simulateRadio = new QRadioButton("Simulate (Show potential savings)");
    QRadioButton* hardlinkRadio = new QRadioButton("Hardlink (Create filesystem hardlinks)");
    QRadioButton* moveRadio = new QRadioButton("Move to Recycle Bin (Safe removal)");
    QRadioButton* deleteRadio = new QRadioButton("Delete (Permanent removal)");
    simulateRadio->setChecked(true);
    
    actionLayout->addWidget(simulateRadio);
    actionLayout->addWidget(hardlinkRadio);
    actionLayout->addWidget(moveRadio);
    actionLayout->addWidget(deleteRadio);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Start Scan");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all to main layout
    layout->addWidget(dirGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(actionGroup);
    layout->addWidget(m_dedupResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_dedupResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
}

void MainWindow::setupVisualizationTab() {
    m_vizTab = new QWidget();
    m_vizResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_vizTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QLineEdit* dirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [dirEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Visualize");
        if (!dir.isEmpty()) {
            dirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(dirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Treemap visualization
    m_treemapWidget = new TreemapWidget();
    m_treemapWidget->setMinimumHeight(400);
    
    // Controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    QPushButton* refreshButton = new QPushButton("Refresh");
    QPushButton* zoomInButton = new QPushButton("Zoom In");
    QPushButton* zoomOutButton = new QPushButton("Zoom Out");
    controlsLayout->addStretch();
    controlsLayout->addWidget(refreshButton);
    controlsLayout->addWidget(zoomInButton);
    controlsLayout->addWidget(zoomOutButton);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Generate Visualization");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all to main layout
    layout->addWidget(dirGroup);
    layout->addWidget(m_treemapWidget, 1);
    layout->addLayout(controlsLayout);
    layout->addWidget(m_vizResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_vizResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
}

void MainWindow::setupResidueTab() {
    m_residueTab = new QWidget();
    m_residueResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_residueTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QLineEdit* dirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [dirEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            dirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(dirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Detection Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    QCheckBox* detectOrphanedCheck = new QCheckBox("Detect Orphaned Files");
    detectOrphanedCheck->setChecked(true);
    QCheckBox* detectTempCheck = new QCheckBox("Detect Temporary Files");
    detectTempCheck->setChecked(true);
    QCheckBox* detectEmptyDirsCheck = new QCheckBox("Detect Empty Directories");
    detectEmptyDirsCheck->setChecked(true);
    QCheckBox* detectRegistryCheck = new QCheckBox("Detect Registry Residue (Windows only)");
    
    optionsLayout->addWidget(detectOrphanedCheck);
    optionsLayout->addWidget(detectTempCheck);
    optionsLayout->addWidget(detectEmptyDirsCheck);
    optionsLayout->addWidget(detectRegistryCheck);
    
    // Action options
    QGroupBox* actionGroup = new QGroupBox("Action");
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
    QRadioButton* simulateRadio = new QRadioButton("Simulate (Show potential cleanup)");
    QRadioButton* moveRadio = new QRadioButton("Move to Recycle Bin (Safe removal)");
    QRadioButton* deleteRadio = new QRadioButton("Delete (Permanent removal)");
    simulateRadio->setChecked(true);
    
    actionLayout->addWidget(simulateRadio);
    actionLayout->addWidget(moveRadio);
    actionLayout->addWidget(deleteRadio);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Start Detection");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all to main layout
    layout->addWidget(dirGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(actionGroup);
    layout->addWidget(m_residueResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_residueResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
}

void MainWindow::setupSimilarityTab() {
    m_similarityTab = new QWidget();
    m_similarityResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_similarityTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QLineEdit* dirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [dirEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            dirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(dirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Detection Options");
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    QCheckBox* detectImagesCheck = new QCheckBox("Detect Similar Images");
    detectImagesCheck->setChecked(true);
    QCheckBox* detectAudioCheck = new QCheckBox("Detect Similar Audio");
    detectAudioCheck->setChecked(true);
    
    QSpinBox* thresholdSpin = new QSpinBox();
    thresholdSpin->setRange(0, 100);
    thresholdSpin->setValue(90);
    thresholdSpin->setSuffix("%");
    
    optionsLayout->addRow(detectImagesCheck);
    optionsLayout->addRow(detectAudioCheck);
    optionsLayout->addRow("Similarity Threshold:", thresholdSpin);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Start Detection");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all to main layout
    layout->addWidget(dirGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(m_similarityResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_similarityResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
}

void MainWindow::onScanDirectory() {
    if (m_isScanning) {
        QMessageBox::warning(this, "Scan in Progress", 
                            "A scan is already in progress. Please wait or cancel the current scan.");
        return;
    }
    
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
    if (dirPath.isEmpty()) {
        return;
    }
    
    m_isScanning = true;
    statusBar()->showMessage("Scanning...");
    
    // Determine which tab is active to show results in the correct display
    ResultsDisplay* currentResults = nullptr;
    int currentTab = m_tabWidget->currentIndex();
    switch (currentTab) {
        case 0: // Deduplication
            currentResults = m_dedupResults;
            break;
        case 1: // Visualization
            currentResults = m_vizResults;
            break;
        case 2: // Residue detection
            currentResults = m_residueResults;
            break;
        case 3: // Similarity detection
            currentResults = m_similarityResults;
            break;
    }
    
    if (currentResults) {
        currentResults->clear();
        currentResults->addMessage(QString("Starting scan of directory: %1").arg(dirPath));
        currentResults->showProgress(true);
        currentResults->setProgress(0);
    }
    
    // Start scan in a separate thread
    QThread* scanThread = QThread::create([this, dirPath, currentResults]() {
        try {
            ScanOptions options;
            options.computeHeadTail = true;
            options.computeFullHash = false;
            options.minFileSize = 0;
            options.maxFileSize = 0;
            options.includeExtensions.clear();
            
            std::string scanPath = FileUtils::to_platform_path(dirPath.toStdString());
            
            size_t fileCount = 0;
            m_scanner->scanVolume(scanPath, options,
                                 [this, &fileCount, currentResults](const ScanEvent& event) {
                if (event.type == ScanEventType::FileAdded) {
                    m_index->put(event.fileEntry);
                    fileCount++;
                    
                    // Update progress every 100 files
                    if (fileCount % 100 == 0) {
                        QString message = QString("Processed %1 files...").arg(fileCount);
                        QMetaObject::invokeMethod(this, "onUpdateStatus", Qt::QueuedConnection,
                                                Q_ARG(QString, message));
                        
                        if (currentResults) {
                            QMetaObject::invokeMethod(currentResults, "addMessage", Qt::QueuedConnection,
                                                    Q_ARG(QString, message));
                        }
                    }
                }
            });
            
            m_index->flush();
            
            QString completionMessage = QString("Scan complete! Processed %1 files.").arg(fileCount);
            QMetaObject::invokeMethod(this, "onUpdateStatus", Qt::QueuedConnection,
                                    Q_ARG(QString, completionMessage));
            
            if (currentResults) {
                QMetaObject::invokeMethod(currentResults, "addMessage", Qt::QueuedConnection,
                                        Q_ARG(QString, completionMessage));
                QMetaObject::invokeMethod(currentResults, "showProgress", Qt::QueuedConnection,
                                        Q_ARG(bool, false));
            }
            
            // After scan, get all files from index and update treemap
            std::vector<FileEntry> allFiles = m_index->getByVolume(1);
            
            if (!allFiles.empty()) {
                // Limit to first 1000 files to avoid memory issues
                std::vector<FileEntry> limitedFiles;
                size_t limit = std::min(static_cast<size_t>(1000), allFiles.size());
                for (size_t i = 0; i < limit; ++i) {
                    limitedFiles.push_back(allFiles[i]);
                }
                
                // Create treemap on the main thread
                QMetaObject::invokeMethod(this, "onUpdateStatus", Qt::QueuedConnection,
                                        Q_ARG(QString, QString("Creating visualization...")));
                
                if (currentResults) {
                    QMetaObject::invokeMethod(currentResults, "addMessage", Qt::QueuedConnection,
                                            Q_ARG(QString, "Creating visualization..."));
                }
                
                std::unique_ptr<TreemapNode> rootNode = TreemapLayout::createTreemap(limitedFiles, Rect(0, 0, 800, 600));
                if (rootNode) {
                    QMetaObject::invokeMethod(this, [this, rootNode = std::move(rootNode)]() mutable {
                        m_treemapWidget->setTreemap(std::move(rootNode));
                        onUpdateStatus("Visualization complete!");
                    }, Qt::QueuedConnection);
                }
            }
        } catch (const std::exception& e) {
            QString errorMessage = QString("Error: %1").arg(e.what());
            QMetaObject::invokeMethod(this, "onUpdateStatus", Qt::QueuedConnection,
                                    Q_ARG(QString, errorMessage));
            
            if (currentResults) {
                QMetaObject::invokeMethod(currentResults, "addMessage", Qt::QueuedConnection,
                                        Q_ARG(QString, errorMessage));
                QMetaObject::invokeMethod(currentResults, "showProgress", Qt::QueuedConnection,
                                        Q_ARG(bool, false));
            }
        }
        
        QMetaObject::invokeMethod(this, [this]() {
            m_isScanning = false;
            statusBar()->showMessage("Ready");
        }, Qt::QueuedConnection);
    });
    
    connect(scanThread, &QThread::finished, scanThread, &QThread::deleteLater);
    scanThread->start();
}

void MainWindow::onCancelScan() {
    if (m_isScanning) {
        m_scanner->cancel();
        statusBar()->showMessage("Scan cancelled");
        m_isScanning = false;
    }
}

void MainWindow::onUpdateStatus(const QString& message) {
    statusBar()->showMessage(message);
}

void MainWindow::onUpdateProgress(int value) {
    // Progress bar update would go here
}