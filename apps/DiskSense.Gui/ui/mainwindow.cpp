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
#include <QDirIterator>
#include <QThread>
#include <QTimer>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDebug>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFrame>
#include "core/safety/safety.h"

#include "components/treemapwidget.h"
#include "components/resultsdisplay.h"
#include "components/visualizationwidget.h"
#include "components/sunburstwidget.h"
#include "core/gfx/charts.h"
#include "core/scan/scanner.h"
#include "core/scan/monitor.h"
#include "core/index/lsm_index.h"
#include "libs/utils/utils.h"
#include "core/ops/cleanup.h"
#include "core/ops/dedupe.h"
#include "core/ops/secure_delete.h"
#include "integrations/yara_bridge.h"
#include <QProcess>
#include "ui/tourwizard.h"

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
    , m_fsMonitor(nullptr)
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

void MainWindow::setupUI() {
    setWindowTitle("DiskSense64 - Disk Analysis Suite");
    resize(1200, 800);
    
    // Create central widget and layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Safety banner (non-destructive guarantee)
    {
        QHBoxLayout* safetyLayout = new QHBoxLayout();
        m_safetyBanner = new QLabel();
        m_safetyBanner->setObjectName("safetyBanner");
        m_safetyBanner->setText("⚠️  Modo seguro activo: la aplicación NUNCA borra archivos.");
        m_safetyBanner->setToolTip("Se puede desactivar en desarrollo con DISKSENSE_ALLOW_DELETE=1. En esta build está bloqueado.");
        m_safetyToggle = new QCheckBox("No borrar");
        m_safetyToggle->setChecked(true);
        m_safetyToggle->setEnabled(false);
        safetyLayout->addWidget(m_safetyBanner, 1);
        safetyLayout->addWidget(m_safetyToggle, 0);
        QFrame* safetyFrame = new QFrame();
        safetyFrame->setLayout(safetyLayout);
        safetyFrame->setFrameShape(QFrame::NoFrame);
        safetyFrame->setStyleSheet("#safetyBanner { color: #ffd166; font-weight: 600; }");
        mainLayout->addWidget(safetyFrame);
    }
    
    // Menu bar is built below after creating tabs
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Setup all tabs
    setupDashboardTab();
    setupDeduplicationTab();
    setupVisualizationTab();
    setupResidueTab();
    setupSimilarityTab();
    // Trends tab
    m_trendsTab = new QWidget();
    {
        QVBoxLayout* l = new QVBoxLayout(m_trendsTab);
        QHBoxLayout* hl = new QHBoxLayout();
        m_trendsRefresh = new QPushButton("Refresh Trends"); hl->addStretch(); hl->addWidget(m_trendsRefresh);
        m_trendsChart = new ChartWidget(ChartFactory::Line);
        l->addLayout(hl); l->addWidget(m_trendsChart, 1);
        connect(m_trendsRefresh, &QPushButton::clicked, [this](){
            // Load snapshots and render line chart
            QString path = QDir::homePath()+"/.disksense/snapshots.json";
            QFile f(path);
            ChartData cd; if (f.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
                if (doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (auto v: arr) {
                        auto o=v.toObject(); QString ts=o.value("ts").toString(); double total=o.value("total").toDouble();
                        cd.addDataPoint(ts, total);
                    }
                }
            }
            m_trendsChart->setChartType(ChartFactory::Line);
            m_trendsChart->setChartData(cd);
        });
    }
    // New tabs
    // Health (SMART)
    m_healthTab = new QWidget();
    m_healthResults = new ResultsDisplay();
    {
        QVBoxLayout* l = new QVBoxLayout(m_healthTab);
        QPushButton* scanBtn = new QPushButton("Scan Disks (SMART)");
        l->addWidget(scanBtn);
        l->addWidget(m_healthResults, 1);
        connect(scanBtn, &QPushButton::clicked, [this]() {
            m_healthResults->clear();
            m_healthResults->addMessage("Enumerating disks and running smartctl...");
            // Linux: list /dev/sdX; Windows: smartctl --scan
            QThread* th = QThread::create([this]() {
                QStringList devices;
#ifdef __linux__
                // Try smartctl --scan first
                QProcess p; p.start("smartctl", {"--scan"}); p.waitForFinished(3000);
                QString out = p.readAllStandardOutput();
                for (const QString& line : out.split('\n')) {
                    if (line.contains("/dev/")) devices << line.section(' ', 0, 0);
                }
                if (devices.isEmpty()) {
                    // fallback: /dev/sd[a-z]
                    for (char c='a'; c<='z'; ++c) {
                        QString d = QString("/dev/sd%1").arg(c);
                        if (QFile::exists(d)) devices << d;
                    }
                }
#else
                QProcess p; p.start("smartctl", {"--scan"}); p.waitForFinished(3000);
                QString out = p.readAllStandardOutput();
                for (const QString& line : out.split('\n')) {
                    if (!line.trimmed().isEmpty()) devices << line.section(' ', 0, 0);
                }
#endif
                if (devices.isEmpty()) {
                    QMetaObject::invokeMethod(this, [this]() {
                        m_healthResults->addMessage("No devices found or smartctl not available.");
                    }, Qt::QueuedConnection);
                    return;
                }
                for (const QString& dev : devices) {
                    QProcess s; s.start("smartctl", {"-i", "-H", "-A", dev}); s.waitForFinished(5000);
                    QString out = s.readAllStandardOutput();
                    QMetaObject::invokeMethod(this, [this, dev, out]() {
                        m_healthResults->addMessage(QString("Device: %1").arg(dev));
                        // Extract a few key lines
                        for (const QString& line : out.split('\n')) {
                            if (line.contains("Model") || line.contains("Serial") || line.contains("SMART overall-health") || line.contains("Reallocated_Sector_Ct") || line.contains("Power_On_Hours") || line.contains("Temperature")) {
                                m_healthResults->addMessage("  " + line.trimmed());
                            }
                        }
                    }, Qt::QueuedConnection);
                }
            });
            connect(th, &QThread::finished, th, &QThread::deleteLater);
            th->start();
        });
    }

    // Security/YARA
    m_securityTab = new QWidget();
    m_securityResults = new ResultsDisplay();
    {
        QVBoxLayout* l = new QVBoxLayout(m_securityTab);
        QFormLayout* f = new QFormLayout();
        m_yaraRulesEdit = new QLineEdit();
        QPushButton* browseRules = new QPushButton("Browse rules...");
        QHBoxLayout* hl = new QHBoxLayout(); hl->addWidget(m_yaraRulesEdit); hl->addWidget(browseRules);
        QWidget* c = new QWidget(); c->setLayout(hl);
        f->addRow("Rules file:", c);
        l->addLayout(f);
        QPushButton* runScan = new QPushButton("Run YARA Scan...");
        l->addWidget(runScan);
        l->addWidget(m_securityResults, 1);
        connect(browseRules, &QPushButton::clicked, [this]() {
            QString p = QFileDialog::getOpenFileName(this, "Select YARA rules file", QDir::homePath(), "Rules (*.yar *.yara *.*)");
            if (!p.isEmpty()) m_yaraRulesEdit->setText(p);
        });
        connect(runScan, &QPushButton::clicked, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
            if (dir.isEmpty()) return;
            QString rules = m_yaraRulesEdit ? m_yaraRulesEdit->text() : QString();
            if (rules.isEmpty()) {
                QMessageBox::warning(this, "YARA", "Select a rules file first.");
                return;
            }
            m_securityResults->clear();
            m_securityResults->addMessage(QString("Loading rules: %1").arg(rules));
            QThread* th = QThread::create([this, dir, rules]() {
                if (yara_load_rules(rules.toUtf8().constData()) != 0) {
                    QMetaObject::invokeMethod(this, [this]() { m_securityResults->addMessage("Failed to load rules"); }, Qt::QueuedConnection);
                    return;
                }
                yara_options_t opt; yara_options_init(&opt);
                yara_result_t res; int rc = yara_scan_directory(dir.toUtf8().constData(), &opt, &res);
                size_t matchCount = res.count;
                // Copy matches pointer for UI thread read (we will free after invoke)
                yara_match_t* matchPtr = res.matches;
                QMetaObject::invokeMethod(this, [this, rc, matchCount, matchPtr]() {
                    if (rc != 0) { m_securityResults->addMessage("Scan failed."); return; }
                    if (matchCount == 0) { m_securityResults->addMessage("No matches."); return; }
                    m_securityResults->addMessage(QString("Matches: %1").arg(matchCount));
                    for (size_t i=0;i<matchCount;i++) {
                        auto& m = matchPtr[i];
                        m_securityResults->addMessage(QString("Rule %1 [%2]: '%3' at %4 len %5 sev %6")
                            .arg(m.rule_name).arg(m.rule_namespace).arg(m.matched_string).arg((qulonglong)m.offset).arg((qulonglong)m.length).arg(m.severity));
                    }
                }, Qt::QueuedConnection);
                yara_result_free(&res);
            });
            connect(th, &QThread::finished, th, &QThread::deleteLater);
            th->start();
        });
    }

    // Add tabs to widget
    m_tabWidget->addTab(m_dedupTab, "Deduplication");
    m_tabWidget->addTab(m_vizTab, "Visualization");
    m_tabWidget->addTab(m_residueTab, "Residue");
    m_tabWidget->addTab(m_similarityTab, "Similarity");
    m_tabWidget->addTab(m_healthTab, "Health");
    m_tabWidget->addTab(m_securityTab, "Security");
    m_tabWidget->addTab(m_topNTab, "Top N");
    m_tabWidget->addTab(m_automationTab, "Automation");
    m_tabWidget->addTab(m_remoteTab, "Remote");
    m_tabWidget->addTab(m_searchTab, "Search");
    m_tabWidget->addTab(m_trendsTab, "Trends");

    // Top N
    m_topNTab = new QWidget();
    {
        QVBoxLayout* l = new QVBoxLayout(m_topNTab);
        QHBoxLayout* top = new QHBoxLayout();
        m_topNDirEdit = new QLineEdit();
        QPushButton* browse = new QPushButton("Browse...");
        QPushButton* run = new QPushButton("Analyze Top Files");
        top->addWidget(new QLabel("Directory:")); top->addWidget(m_topNDirEdit,1); top->addWidget(browse); top->addWidget(run);
        m_topNTable = new QTableWidget();
        m_topNTable->setColumnCount(3);
        m_topNTable->setHorizontalHeaderLabels({"Size","Path","Name"});
        m_topNTable->horizontalHeader()->setStretchLastSection(true);
        l->addLayout(top);
        l->addWidget(m_topNTable,1);
        connect(browse,&QPushButton::clicked,[this](){ auto d=QFileDialog::getExistingDirectory(this,"Select Directory"); if(!d.isEmpty()) m_topNDirEdit->setText(d);});
        connect(run,&QPushButton::clicked,[this](){
            QString d=m_topNDirEdit->text(); if(d.isEmpty()) return; 
            std::vector<std::pair<qulonglong, QString>> files; files.reserve(10000);
            QDirIterator it(d, QDir::Files, QDirIterator::Subdirectories);
            while(it.hasNext()){ it.next(); QFileInfo fi=it.fileInfo(); files.emplace_back(fi.size(), fi.absoluteFilePath()); }
            std::sort(files.begin(), files.end(), [](auto&a,auto&b){return a.first>b.first;});
            int rows = std::min<size_t>(1000, files.size());
            m_topNTable->setRowCount(rows);
            for(int i=0;i<rows;i++){ const auto& p=files[i]; 
                m_topNTable->setItem(i,0,new QTableWidgetItem(QString::number(p.first)));
                m_topNTable->setItem(i,1,new QTableWidgetItem(p.second));
                m_topNTable->setItem(i,2,new QTableWidgetItem(QFileInfo(p.second).fileName()));
            }
        });
    }

    // Automation (simple scheduler)
    m_automationTab = new QWidget();
    {
        QVBoxLayout* l = new QVBoxLayout(m_automationTab);
        QHBoxLayout* controls = new QHBoxLayout();
        QLineEdit* dirEdit = new QLineEdit();
        QSpinBox* everyMin = new QSpinBox(); everyMin->setRange(1, 10080); everyMin->setValue(60);
        QPushButton* addTask = new QPushButton("Add Scheduled Scan");
        controls->addWidget(new QLabel("Directory:")); controls->addWidget(dirEdit,1);
        controls->addWidget(new QLabel("Every (min):")); controls->addWidget(everyMin);
        controls->addWidget(addTask);
        m_tasksTable = new QTableWidget(); m_tasksTable->setColumnCount(3); m_tasksTable->setHorizontalHeaderLabels({"Directory","Every(min)","Next Run"}); m_tasksTable->horizontalHeader()->setStretchLastSection(true);
        l->addLayout(controls); l->addWidget(m_tasksTable,1);
        connect(addTask,&QPushButton::clicked,[this,dirEdit,everyMin](){
            QString d=dirEdit->text(); if(d.isEmpty()) return; int m=everyMin->value();
            int row=m_tasksTable->rowCount(); m_tasksTable->insertRow(row);
            m_tasksTable->setItem(row,0,new QTableWidgetItem(d));
            m_tasksTable->setItem(row,1,new QTableWidgetItem(QString::number(m)));
            m_tasksTable->setItem(row,2,new QTableWidgetItem(QDateTime::currentDateTime().addSecs(m*60).toString()));
        });
        if(!m_schedulerTimer){ m_schedulerTimer=new QTimer(this); m_schedulerTimer->start(60*1000); }
        connect(m_schedulerTimer,&QTimer::timeout,[this](){
            QDateTime now=QDateTime::currentDateTime();
            for(int r=0;r<m_tasksTable->rowCount();++r){
                auto* item=m_tasksTable->item(r,2); if(!item) continue; QDateTime next=QDateTime::fromString(item->text());
                if(next.isValid() && now>=next){
                    QString dir=m_tasksTable->item(r,0)->text(); int mins=m_tasksTable->item(r,1)->text().toInt();
                    // perform scan and export JSON into ~/.disksense64/scheduled_exports
                    QString outDir=QDir::homePath()+"/.disksense64/scheduled_exports"; QDir().mkpath(outDir);
                    QString out=outDir+"/export_"+QString::number(now.toSecsSinceEpoch())+".json";
                    // Reuse CLI-like export within GUI
                    auto files = m_index->getByVolume(1);
                    QFile f(out); if(f.open(QIODevice::WriteOnly|QIODevice::Truncate)){
                        f.write("[\n"); for (int i=0;i<(int)files.size();++i){ const auto& fe=files[(size_t)i]; QByteArray line = QByteArray("  {\"path\":\"")+QFile::encodeName(QString::fromStdString(fe.fullPath))+QByteArray("\",\"size\":")+QByteArray::number((qlonglong)fe.sizeLogical)+"}"; if(i+1<(int)files.size()) line+=","; line+="\n"; f.write(line);} f.write("]\n"); f.close(); }
                    // schedule next
                    m_tasksTable->item(r,2)->setText(now.addSecs(mins*60).toString());
                }
            }
        });
    }

    // Remote
    m_remoteTab = new QWidget(); m_remoteResults = new ResultsDisplay();
    {
        QVBoxLayout* l=new QVBoxLayout(m_remoteTab);
        QFormLayout* f=new QFormLayout();
        m_remoteHostEdit=new QLineEdit(); m_remotePathEdit=new QLineEdit();
        f->addRow("Host (user@host):", m_remoteHostEdit);
        f->addRow("Path:", m_remotePathEdit);
        QPushButton* run=new QPushButton("Scan Remote (SSH)");
        l->addLayout(f); l->addWidget(run); l->addWidget(m_remoteResults,1);
        connect(run,&QPushButton::clicked,[this](){
            QString host=m_remoteHostEdit->text(); QString path=m_remotePathEdit->text(); if(host.isEmpty()||path.isEmpty()) return; 
            m_remoteResults->clear(); m_remoteResults->addMessage("Running remote scan via ssh find ...");
            QThread* th=QThread::create([this,host,path](){
                QProcess p; QString pathEsc=path; pathEsc.replace("'","'\\''"); QString cmd=QString("find '%1' -type f -printf '%s %p\\n'").arg(pathEsc);
                p.start("ssh", {host, cmd}); p.waitForFinished(600000);
                QString out=p.readAllStandardOutput();
                QMetaObject::invokeMethod(this,[this,out](){
                    QStringList lines=out.split('\n'); qulonglong total=0; int count=0;
                    for(const QString& line: lines){ if(line.trimmed().isEmpty()) continue; count++; auto size=line.section(' ',0,0).toULongLong(); total+=size; }
                    m_remoteResults->addMessage(QString("Files: %1, Total size: %2 bytes").arg(count).arg(total));
                }, Qt::QueuedConnection);
            }); connect(th,&QThread::finished,th,&QThread::deleteLater); th->start();
        });
    }
    
    // Add tabs to tab widget
    m_tabWidget->addTab(m_dashboardTab, "Dashboard");
    m_tabWidget->addTab(m_dedupTab, "Deduplication");
    m_tabWidget->addTab(m_vizTab, "Visualization");
    m_tabWidget->addTab(m_residueTab, "Residue Detection");
    m_tabWidget->addTab(m_similarityTab, "Similarity Detection");
    m_tabWidget->addTab(m_healthTab, "Disk Health");
    m_tabWidget->addTab(m_securityTab, "Security Scan");
    m_tabWidget->addTab(m_topNTab, "Top N Files");
    m_tabWidget->addTab(m_automationTab, "Automation");
    m_tabWidget->addTab(m_remoteTab, "Remote Scan");
    m_tabWidget->addTab(m_trendsTab, "Trends");
    
    mainLayout->addWidget(m_tabWidget);
    
    // Create menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    QMenu* fileMenu = menuBar->addMenu("&File");
    QAction* scanAction = fileMenu->addAction("&Scan Directory...");
    scanAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(scanAction, &QAction::triggered, this, &MainWindow::onScanDirectory);
    
    QAction* exportAction = fileMenu->addAction("&Export Results...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportResults);

    QAction* settingsAction = fileMenu->addAction("&Settings...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsRequested);
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    QMenu* toolsMenu = menuBar->addMenu("&Tools");
    QAction* secureDeleteAction = toolsMenu->addAction("Secure Delete File...");
    connect(secureDeleteAction, &QAction::triggered, [this]() {
        if (!safety::deletion_allowed()) {
            QMessageBox::warning(this, tr("Secure Delete"), tr("Blocked by Safety Mode. Enable DISKSENSE_ALLOW_DELETE=1 for development."));
            return;
        }
        QString f = QFileDialog::getOpenFileName(this, "Select file to secure delete");
        if (f.isEmpty()) return;
        QStringList profiles = {"Clear (1 pass, zeros)", "Purge (3 passes, random + verify)"};
        bool ok=true; QString sel = QInputDialog::getItem(this, tr("Profile"), tr("Select NIST-like profile:"), profiles, 0, false, &ok);
        if (!ok) return;
        SecureDeleteOptions opt = (sel.startsWith("Purge")) ? secure_delete_preset(SecureDeleteProfile::Purge) : secure_delete_preset(SecureDeleteProfile::Clear);
        std::string err; if (!secure_delete_file(f.toStdString(), opt, &err)) {
            QMessageBox::warning(this, "Secure Delete", QString::fromStdString(err));
        } else {
            QMessageBox::information(this, "Secure Delete", "File securely deleted.");
        }
    });
    
    QMenu* helpMenu = menuBar->addMenu("&Help");
    QAction* tourAct = helpMenu->addAction("Guided Tour...");
    connect(tourAct, &QAction::triggered, [this]() { TourWizard wiz(this); wiz.exec(); });
    QAction* tourAct = helpMenu->addAction("Guided Tour...");
    connect(tourAct, &QAction::triggered, [this]() { TourWizard wiz(this); wiz.exec(); });
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About DiskSense64", 
                          "DiskSense64 - Cross-Platform Disk Analysis Suite\n\n"
                          "A comprehensive disk analysis tool with:\n"
                          "- Exact File Deduplication\n"
                          "- Disk Space Visualization\n"
                          "- Residue Detection and Cleanup\n"
                          "- Perceptual Duplicate Detection");
    });
    
    // Create toolbar
    QToolBar* toolBar = addToolBar("Main");
    toolBar->addAction(scanAction);
    
    QPushButton* scanButton = new QPushButton("Scan Directory");
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDirectory);
    toolBar->addWidget(scanButton);
    
    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    toolBar->addWidget(cancelButton);
    
    // Create status bar widgets
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedWidth(200);
    statusBar()->addPermanentWidget(progressBar);
}

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
    // Switch to Dedup tab, set path and start scan
    if (m_dedupeDirEdit) m_dedupeDirEdit->setText(path);
    m_tabWidget->setCurrentWidget(m_dedupTab);
    // Trigger scan directly using helper
    if (!path.isEmpty()) {
        // Use Dedup results display for progress output
        ResultsDisplay* out = m_dedupResults ? m_dedupResults : nullptr;
        if (out) {
            out->clear(); out->addMessage(QString("Starting scan of directory: %1").arg(path));
        }
        // Reuse same scanning flow
        // Build a minimal ScanOptions and run in a worker thread
        if (!m_isScanning) {
            m_isScanning = true; statusBar()->showMessage("Scanning...");
            QString dirPath = path;
            QThread* scanThread = QThread::create([this, dirPath, out]() {
                try {
                    ScanOptions options; options.computeHeadTail = true; options.computeFullHash = false; options.useMftReader = (m_useMftCheck && m_useMftCheck->isChecked());
                    std::string scanPath = FileUtils::to_platform_path(dirPath.toStdString());
                    size_t fileCount = 0;
                    m_scanner->scanVolume(scanPath, options, [this, &fileCount, out](const ScanEvent& event){
                        if (event.type == ScanEventType::FileAdded) { m_index->put(event.fileEntry); fileCount++; }
                        if (fileCount % 250 == 0 && out) {
                            QMetaObject::invokeMethod(out, "addMessage", Qt::QueuedConnection, Q_ARG(QString, QString("Processed %1 files...").arg(fileCount)));
                        }
                    });
                    m_index->flush();
                    if (out) QMetaObject::invokeMethod(out, "addMessage", Qt::QueuedConnection, Q_ARG(QString, QString("Scan complete. Files: %1").arg(fileCount)));
                } catch (...) {}
                QMetaObject::invokeMethod(this, [this](){ m_isScanning=false; statusBar()->showMessage("Ready"); }, Qt::QueuedConnection);
            });
            connect(scanThread, &QThread::finished, scanThread, &QThread::deleteLater);
            scanThread->start();
        }
    }
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
    
    m_dedupeDirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            m_dedupeDirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(m_dedupeDirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Scan Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_useMftCheck = new QCheckBox("Use MFT Reader (Windows only, requires admin)");
    QCheckBox* followLinksCheck = new QCheckBox("Follow Reparse Points/Symlinks");
    QCheckBox* computeHeadTailCheck = new QCheckBox("Compute Head/Tail Signatures");
    computeHeadTailCheck->setChecked(true);
    m_dedupeFullHashCheck = new QCheckBox("Compute Full File Hash (Slow)");
    
    optionsLayout->addWidget(m_useMftCheck);
    optionsLayout->addWidget(followLinksCheck);
    optionsLayout->addWidget(computeHeadTailCheck);
    optionsLayout->addWidget(m_dedupeFullHashCheck);
    // Minimum file size (KB)
    QHBoxLayout* minSizeLayout = new QHBoxLayout();
    QLabel* minSizeLbl = new QLabel("Min size (KB):");
    m_dedupeMinSizeSpin = new QSpinBox();
    m_dedupeMinSizeSpin->setRange(0, 1024*1024);
    m_dedupeMinSizeSpin->setValue(1024);
    minSizeLayout->addWidget(minSizeLbl);
    minSizeLayout->addWidget(m_dedupeMinSizeSpin);
    optionsLayout->addLayout(minSizeLayout);
    
    // Action options
    QGroupBox* actionGroup = new QGroupBox("Action");
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
    m_dedupeSimulateRadio = new QRadioButton("Simulate (Show potential savings)");
    m_dedupeHardlinkRadio = new QRadioButton("Hardlink (Create filesystem hardlinks)");
    m_dedupeMoveRadio = new QRadioButton("Move to Recycle Bin (Safe removal)");
    m_dedupeDeleteRadio = new QRadioButton("Delete (Permanent removal)");
    m_dedupeSimulateRadio->setChecked(true);
    actionLayout->addWidget(m_dedupeSimulateRadio);
    actionLayout->addWidget(m_dedupeHardlinkRadio);
    actionLayout->addWidget(m_dedupeMoveRadio);
    if (safety::deletion_allowed()) {
        actionLayout->addWidget(m_dedupeDeleteRadio);
    } else {
        m_dedupeDeleteRadio->setEnabled(false);
        m_dedupeDeleteRadio->setToolTip("Disabled by Safety Mode");
        m_dedupeDeleteRadio->hide();
    }

    // Live monitor toggle
    m_liveMonitorCheck = new QCheckBox("Live monitor changes (incremental)");
    actionLayout->addWidget(m_liveMonitorCheck);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Start Scan");
    QPushButton* dupesButton = new QPushButton("Find Duplicates");
    QPushButton* applyDupesButton = new QPushButton("Apply Action");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(dupesButton);
    buttonLayout->addWidget(applyDupesButton);
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
    connect(dupesButton, &QPushButton::clicked, this, &MainWindow::onFindDuplicates);
    connect(applyDupesButton, &QPushButton::clicked, this, &MainWindow::onApplyDedupe);
    connect(m_dedupResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
}

void MainWindow::setupVisualizationTab() {
    m_vizTab = new QWidget();
    m_vizResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_vizTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    m_vizDirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Visualize");
        if (!dir.isEmpty()) {
            m_vizDirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(m_vizDirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Treemap visualization
    m_treemapWidget = new TreemapWidget();
    m_treemapWidget->setMinimumHeight(400);
    // Simple chart widget (bar by default)
    m_chartWidget = new ChartWidget(ChartFactory::Bar);
    m_chartWidget->setMinimumHeight(260);
    // Sunburst (toggleable)
    m_sunburstWidget = new SunburstWidget();
    m_sunburstWidget->setVisible(false);
    
    // Controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    QPushButton* refreshButton = new QPushButton("Refresh");
    QPushButton* zoomInButton = new QPushButton("Zoom In");
    QPushButton* zoomOutButton = new QPushButton("Zoom Out");
    QLabel* chartLbl = new QLabel("Chart:");
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItems({"Bar", "Pie", "Line"});
    QLabel* filterLbl = new QLabel("Filter:");
    m_vizFilterEdit = new QLineEdit();
    m_showSunburstCheck = new QCheckBox("Show Sunburst");
    controlsLayout->addWidget(chartLbl);
    controlsLayout->addWidget(m_chartTypeCombo);
    controlsLayout->addSpacing(12);
    controlsLayout->addWidget(filterLbl);
    controlsLayout->addWidget(m_vizFilterEdit, 1);
    controlsLayout->addSpacing(12);
    controlsLayout->addWidget(m_showSunburstCheck);
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
    layout->addWidget(m_chartWidget);
    layout->addWidget(m_sunburstWidget);
    layout->addLayout(controlsLayout);
    layout->addWidget(m_vizResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onGenerateVisualization);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_vizResults, &ResultsDisplay::cancelRequested, this, &MainWindow::onCancelScan);
    connect(m_chartTypeCombo, &QComboBox::currentTextChanged, this, &MainWindow::onGenerateVisualization);
    connect(m_vizFilterEdit, &QLineEdit::textChanged, this, &MainWindow::onGenerateVisualization);
    connect(m_showSunburstCheck, &QCheckBox::toggled, [this](bool on){ if (m_sunburstWidget) m_sunburstWidget->setVisible(on); });
}

void MainWindow::setupResidueTab() {
    m_residueTab = new QWidget();
    m_residueResults = new ResultsDisplay();
    
    QVBoxLayout* layout = new QVBoxLayout(m_residueTab);
    
    // Directory selection
    QGroupBox* dirGroup = new QGroupBox("Directory Selection");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    m_residueDirEdit = new QLineEdit();
    QPushButton* browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
        if (!dir.isEmpty()) {
            m_residueDirEdit->setText(dir);
        }
    });
    
    QHBoxLayout* dirHLayout = new QHBoxLayout();
    dirHLayout->addWidget(m_residueDirEdit);
    dirHLayout->addWidget(browseButton);
    
    dirLayout->addRow("Directory:", dirHLayout);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Detection Options");
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    m_residueExtEdit = new QLineEdit(".tmp,.log");
    m_residueDaysSpin = new QSpinBox();
    m_residueDaysSpin->setRange(0, 3650);
    m_residueDaysSpin->setValue(0);
    m_residueRemoveEmptyCheck = new QCheckBox("Remove Empty Directories");
    m_residueRemoveEmptyCheck->setChecked(true);
    m_residueSimulateCheck = new QCheckBox("Simulation Mode (no changes)");
    m_residueSimulateCheck->setChecked(true);
    
    optionsLayout->addRow("Extensions:", m_residueExtEdit);
    optionsLayout->addRow("Older than (days):", m_residueDaysSpin);
    optionsLayout->addWidget(m_residueRemoveEmptyCheck);
    optionsLayout->addWidget(m_residueSimulateCheck);
    
    // Action options
    QGroupBox* actionGroup = new QGroupBox("Action");
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
    QRadioButton* simulateRadio = new QRadioButton("Simulate (Show potential cleanup)");
    QRadioButton* moveRadio = new QRadioButton("Move to Quarantine (Safe)");
    QRadioButton* deleteRadio = new QRadioButton("Delete (Permanent removal)");
    simulateRadio->setChecked(true);
    
    actionLayout->addWidget(simulateRadio);
    actionLayout->addWidget(moveRadio);
    if (safety::deletion_allowed()) {
        actionLayout->addWidget(deleteRadio);
    } else {
        deleteRadio->setEnabled(false);
        deleteRadio->setToolTip("Disabled by Safety Mode");
        deleteRadio->hide();
    }
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* scanButton = new QPushButton("Analyze");
    QPushButton* applyButton = new QPushButton("Apply Cleanup");
    QPushButton* undoButton = new QPushButton("Undo Cleanup...");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(undoButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all to main layout
    layout->addWidget(dirGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(actionGroup);
    layout->addWidget(m_residueResults, 1);
    layout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onResidueDetect);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(applyButton, &QPushButton::clicked, this, &MainWindow::onResidueApply);
    connect(undoButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Quarantine Directory", QDir::homePath());
        if (dir.isEmpty()) return;
        size_t restored = cleanup_undo_quarantine(dir.toStdString());
        m_residueResults->addMessage(QString("Restored %1 items from %2").arg(restored).arg(dir));
    });
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
            options.useMftReader = (m_useMftCheck && m_useMftCheck->isChecked());
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
                // Append snapshot for trends
                uint64_t total=0; for (const auto& fe: allFiles) total += fe.sizeLogical;
                QMetaObject::invokeMethod(this, [total]() {
                    QDir().mkpath(QDir::homePath()+"/.disksense");
                    QFile f(QDir::homePath()+"/.disksense/snapshots.json");
                    QJsonArray arr;
                    if (f.open(QIODevice::ReadOnly)) { auto doc=QJsonDocument::fromJson(f.readAll()); if (doc.isArray()) arr=doc.array(); f.close(); }
                    if (f.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
                        QJsonObject o; o["ts"]=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"); o["total"]=double(total);
                        arr.append(o); f.write(QJsonDocument(arr).toJson()); f.close();
                    }
                }, Qt::QueuedConnection);
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
            // If live monitor is enabled, start monitoring root
            if (m_liveMonitorCheck && m_liveMonitorCheck->isChecked()) {
                if (m_fsMonitor) { m_fsMonitor->stop(); delete m_fsMonitor; m_fsMonitor=nullptr; }
                m_fsMonitor = new FsMonitor();
                QString root = m_dedupeDirEdit ? m_dedupeDirEdit->text() : QString();
                if (!root.isEmpty()) {
                    auto cb = [this](const ScanEvent& ev){
                        if (ev.type == ScanEventType::FileAdded) {
                            m_index->put(ev.fileEntry);
                            QMetaObject::invokeMethod(this, [this, ev](){
                                if (m_dedupResults) m_dedupResults->addMessage(QString("[monitor] added: %1").arg(QString::fromStdString(ev.fileEntry.fullPath)));
                            }, Qt::QueuedConnection);
                        } else if (ev.type == ScanEventType::FileRemoved) {
                            m_index->remove(ev.fileEntry.volumeId, ev.fileEntry.fileId);
                            QMetaObject::invokeMethod(this, [this, ev](){
                                if (m_dedupResults) m_dedupResults->addMessage(QString("[monitor] removed: %1").arg(QString::fromStdString(ev.fileEntry.fullPath)));
                            }, Qt::QueuedConnection);
                        }
                    };
                    m_fsMonitor->start(FileUtils::to_platform_path(root.toStdString()), cb);
                }
            }
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

void MainWindow::onResidueDetect() {
    QString dirPath = m_residueDirEdit ? m_residueDirEdit->text() : QString();
    if (dirPath.isEmpty()) {
        dirPath = QFileDialog::getExistingDirectory(this, "Select Directory to Analyze");
        if (dirPath.isEmpty()) return;
        if (m_residueDirEdit) m_residueDirEdit->setText(dirPath);
    }

    m_residueResults->clear();
    m_residueResults->addMessage(QString("Analyzing residue in: %1").arg(dirPath));
    m_residueResults->showProgress(true);

    // Run in worker thread
    QThread* th = QThread::create([this, dirPath]() {
        CleanupOptions opts;
        opts.simulateOnly = m_residueSimulateCheck && m_residueSimulateCheck->isChecked();
        opts.removeEmptyDirs = m_residueRemoveEmptyCheck && m_residueRemoveEmptyCheck->isChecked();
        opts.olderThanDays = m_residueDaysSpin ? m_residueDaysSpin->value() : 0;
        // parse extensions
        std::vector<std::string> exts;
        if (m_residueExtEdit) {
            QStringList parts = m_residueExtEdit->text().split(',', Qt::SkipEmptyParts);
            for (const QString& p : parts) exts.emplace_back(p.trimmed().toStdString());
        }
        opts.extensions = std::move(exts);

        auto rep = cleanup_analyze(FileUtils::to_platform_path(dirPath.toStdString()), opts);
        QMetaObject::invokeMethod(this, [this, rep]() {
            // store last report
            this->m_residueResults->clear();
            m_residueResults->addMessage(QString("Found %1 candidates, potential cleanup %2 bytes")
                                         .arg(rep.candidates.size()).arg(rep.totalSize));
            for (const auto& c : rep.candidates) {
                QString line = QString("%1%2 (%3)")
                                 .arg(c.isDirectory ? "[DIR]  " : "[FILE] ")
                                 .arg(QString::fromStdString(c.path))
                                 .arg(c.sizeBytes);
                m_residueResults->addMessage(line);
            }
            m_residueResults->showProgress(false);
            // save report for apply step
            // We keep a copy by assigning to a member captured in a lambda
        }, Qt::QueuedConnection);
        // store the report in main thread-safe way
        QMetaObject::invokeMethod(this, [this, rep]() {
            this->m_lastCleanupReport = rep;
        }, Qt::QueuedConnection);
    });
    connect(th, &QThread::finished, th, &QThread::deleteLater);
    th->start();
}

void MainWindow::onResidueApply() {
    if (m_lastCleanupReport.candidates.empty()) {
        QMessageBox::information(this, "Cleanup", "Run Analyze first to find candidates.");
        return;
    }
    bool ok = true;
    if (safety::deletion_allowed()) {
        QString text = QInputDialog::getText(this, "Confirm Cleanup",
            "Type DELETE to confirm applying cleanup (files may be moved or deleted):",
            QLineEdit::Normal, "", &ok);
        if (!ok || text.trimmed() != "DELETE") {
            QMessageBox::information(this, "Cleanup", "Cleanup cancelled.");
            return;
        }
    }
    CleanupOptions opts;
    // In Safety Mode we still apply, but only to Quarantine (no deletions)
    opts.simulateOnly = false;
    opts.removeEmptyDirs = m_residueRemoveEmptyCheck && m_residueRemoveEmptyCheck->isChecked();
    opts.olderThanDays = m_residueDaysSpin ? m_residueDaysSpin->value() : 0;
    // choose quarantine by default for safety
    opts.useQuarantine = true;
    opts.quarantineDir = (QDir::homePath() + "/.disksense_quarantine").toStdString();
    size_t affected = cleanup_apply(m_lastCleanupReport, opts);
    m_residueResults->addMessage(QString("Cleanup applied. Items moved to quarantine: %1. Quarantine: %2")
                                 .arg(affected).arg(QString::fromStdString(opts.quarantineDir)));
}

void MainWindow::onExportResults() {
    QString out = QFileDialog::getSaveFileName(this, "Export Results", QDir::homePath() + "/disksense_export.json", "JSON (*.json);;CSV (*.csv)");
    if (out.isEmpty()) return;
    bool json = out.endsWith(".json", Qt::CaseInsensitive);
    auto files = m_index->getByVolume(1);
    QFile f(out);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Export", "Cannot open output file");
        return;
    }
    if (json) {
        f.write("[\n");
        for (int i = 0; i < (int)files.size(); ++i) {
            const auto& fe = files[(size_t)i];
            QByteArray line = QByteArray::fromStdString("  {\"path\":\"") + QFile::encodeName(QString::fromStdString(fe.fullPath)) + QByteArray::fromStdString("\",\"size\":") + QByteArray::number((qlonglong)fe.sizeLogical) + "}";
            if (i + 1 < (int)files.size()) line += ",";
            line += "\n";
            f.write(line);
        }
        f.write("]\n");
    } else {
        f.write("path,size\n");
        for (const auto& fe : files) {
            QByteArray line = QByteArray("\"") + QFile::encodeName(QString::fromStdString(fe.fullPath)) + QByteArray("\",");
            line += QByteArray::number((qlonglong)fe.sizeLogical) + "\n";
            f.write(line);
        }
    }
    f.close();
    statusBar()->showMessage(QString("Exported %1 entries to %2").arg(files.size()).arg(out), 5000);
}

void MainWindow::onGenerateVisualization() {
    QString dirPath = m_vizDirEdit ? m_vizDirEdit->text() : QString();
    if (dirPath.isEmpty()) {
        dirPath = QFileDialog::getExistingDirectory(this, "Select Directory to Visualize");
        if (dirPath.isEmpty()) return;
        if (m_vizDirEdit) m_vizDirEdit->setText(dirPath);
    }

    // Build visualization data
    const QString filter = m_vizFilterEdit ? m_vizFilterEdit->text().trimmed() : QString();
    ChartFactory::ChartType t = ChartFactory::Bar;
    if (m_chartTypeCombo) {
        const QString s = m_chartTypeCombo->currentText();
        if (s == "Pie") t = ChartFactory::Pie; else if (s == "Line") t = ChartFactory::Line; else t = ChartFactory::Bar;
    }

    ChartData data;
    if (t == ChartFactory::Line) {
        // Age distribution over last 12 months
        QMap<int, double> byMonth; // 0..11 (0=oldest)
        QDate today = QDate::currentDate();
        for (int i = 0; i < 12; ++i) byMonth[i] = 0.0;
        QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo fi = it.fileInfo();
            if (!filter.isEmpty() && !fi.fileName().contains(filter, Qt::CaseInsensitive)) continue;
            int monthsAgo = fi.lastModified().date().monthsTo(today);
            if (monthsAgo < 0) monthsAgo = 0;
            if (monthsAgo > 11) monthsAgo = 11;
            byMonth[11 - monthsAgo] += fi.size();
        }
        for (int i = 0; i < 12; ++i) {
            QString label = today.addMonths(-(11 - i)).toString("MMM");
            data.addDataPoint(label, byMonth[i]);
        }
    } else {
        // File-type distribution (size by extension)
        QMap<QString, double> totals;
        QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo fi = it.fileInfo();
            if (!filter.isEmpty() && !fi.fileName().contains(filter, Qt::CaseInsensitive)) continue;
            QString ext = fi.suffix().toLower();
            if (ext.isEmpty()) ext = "(none)";
            totals[ext] += fi.size();
        }
        int idx = 0;
        for (auto it = totals.constBegin(); it != totals.constEnd(); ++it, ++idx) {
            data.addDataPoint(it.key(), it.value(), QColor::fromHsv((idx * 360) / qMax(1, totals.size()), 200, 220));
        }
    }

    if (m_chartWidget) {
        m_chartWidget->setChartType(t);
        m_chartWidget->setChartData(data);
    }
    if (m_sunburstWidget && m_showSunburstCheck && m_showSunburstCheck->isChecked()) {
        m_sunburstWidget->setRootDirectory(dirPath);
    }
    if (m_vizResults) {
        m_vizResults->clear();
        m_vizResults->addMessage(QString("Built %1 chart from %2")
            .arg(t == ChartFactory::Line ? "age distribution" : "file-type")
            .arg(dirPath));
    }
}

void MainWindow::onFindDuplicates() {
    if (!m_index) return;
    m_dedupResults->clear();
    m_dedupResults->addMessage("Finding duplicates...");
    DedupeOptions opt;
    opt.simulateOnly = m_dedupeSimulateRadio && m_dedupeSimulateRadio->isChecked();
    opt.useHardlinks = m_dedupeHardlinkRadio && m_dedupeHardlinkRadio->isChecked();
    opt.moveToRecycleBin = m_dedupeMoveRadio && m_dedupeMoveRadio->isChecked();
    opt.computeFullHash = m_dedupeFullHashCheck && m_dedupeFullHashCheck->isChecked();
    opt.minFileSize = m_dedupeMinSizeSpin ? (uint64_t)m_dedupeMinSizeSpin->value() : 1024;

    Deduplicator d(*m_index);
    auto groups = d.findDuplicates(opt);
    auto stats = d.getStats();
    m_dedupResults->addMessage(QString("Groups: %1, duplicate files: %2, potential savings: %3 bytes")
                               .arg(groups.size()).arg(stats.duplicateFiles).arg(stats.potentialSavings));
}

void MainWindow::onApplyDedupe() {
    if (!m_index) return;
    DedupeOptions opt;
    // Enforce Safety Mode: simulate when deletions are not allowed
    opt.simulateOnly = !safety::deletion_allowed();
    opt.useHardlinks = m_dedupeHardlinkRadio && m_dedupeHardlinkRadio->isChecked();
    opt.moveToRecycleBin = m_dedupeMoveRadio && m_dedupeMoveRadio->isChecked();
    opt.computeFullHash = m_dedupeFullHashCheck && m_dedupeFullHashCheck->isChecked();
    opt.minFileSize = m_dedupeMinSizeSpin ? (uint64_t)m_dedupeMinSizeSpin->value() : 1024;

    Deduplicator d(*m_index);
    auto groups = d.findDuplicates(opt);
    if (groups.empty()) {
        m_dedupResults->addMessage("No duplicates to apply action.");
        return;
    }
    auto finalStats = d.deduplicate(groups, opt);
    if (!safety::deletion_allowed()) {
        m_dedupResults->addMessage("Safety Mode active: simulated savings only; no files were modified.");
    }
    m_dedupResults->addMessage(QString("Applied action. Actual savings (est.): %1 bytes, hardlinks: %2")
    QAction* restoreTrashAction = toolsMenu->addAction("Restore from Trash...");
    connect(restoreTrashAction, &QAction::triggered, [this]() {
        TrashRestoreDialog dlg(this); dlg.exec();
    });
                               .arg(finalStats.actualSavings).arg(finalStats.hardlinksCreated));
}
