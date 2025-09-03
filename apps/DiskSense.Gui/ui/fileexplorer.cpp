#include "fileexplorer.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTimeEdit>
#include <QRegularExpressionValidator>
#include <QProcess>
#include <QMenu>
#include <QAction>
#include <QFileDialog>

FileExplorer::FileExplorer(QWidget *parent)
    : QWidget(parent)
    , m_pathEdit(nullptr)
    , m_upButton(nullptr)
    , m_homeButton(nullptr)
    , m_backButton(nullptr)
    , m_forwardButton(nullptr)
    , m_fileTree(nullptr)
    , m_fileTable(nullptr)
    , m_nameFilterEdit(nullptr)
    , m_minSizeSpin(nullptr)
    , m_maxSizeSpin(nullptr)
    , m_fromDateEdit(nullptr)
    , m_toDateEdit(nullptr)
    , m_regexFilterEdit(nullptr)
    , m_applyFilterButton(nullptr)
    , m_clearFilterButton(nullptr)
    , m_propertiesGroup(nullptr)
    , m_nameLabel(nullptr)
    , m_pathLabel(nullptr)
    , m_sizeLabel(nullptr)
    , m_typeLabel(nullptr)
    , m_modifiedLabel(nullptr)
    , m_permissionsLabel(nullptr)
    , m_currentHistoryIndex(-1)
    , m_currentPath(QDir::homePath())
    , m_fileSystemModel(nullptr)
    , m_proxyModel(nullptr)
{
    setupUI();
    connectSignals();
    
    // Initialize navigation history
    updateNavigationHistory();
    
    // Populate initial views
    populateFileSystemTree();
    populateFileTable(m_currentPath);
    updateFileProperties("");
}

FileExplorer::~FileExplorer() {
}

void FileExplorer::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Navigation bar
    QHBoxLayout* navLayout = new QHBoxLayout();
    m_backButton = new QPushButton("←");
    m_forwardButton = new QPushButton("→");
    m_upButton = new QPushButton("↑");
    m_homeButton = new QPushButton("⌂");
    m_pathEdit = new QLineEdit();
    
    navLayout->addWidget(m_backButton);
    navLayout->addWidget(m_forwardButton);
    navLayout->addWidget(m_upButton);
    navLayout->addWidget(m_homeButton);
    navLayout->addWidget(m_pathEdit);
    
    // Main splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel - File tree
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    m_fileTree = new QTreeWidget();
    m_fileTree->setHeaderLabels(QStringList() << "Name" << "Size" << "Type" << "Modified");
    m_fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    
    leftLayout->addWidget(m_fileTree);
    
    // Right panel - File table and properties
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    // File table
    m_fileTable = new QTableWidget(0, 5);
    m_fileTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Type" << "Modified" << "Permissions");
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Filter panel
    QGroupBox* filterGroup = new QGroupBox("Filters");
    QGridLayout* filterLayout = new QGridLayout(filterGroup);
    
    m_nameFilterEdit = new QLineEdit();
    m_nameFilterEdit->setPlaceholderText("File name pattern (e.g., *.txt)");
    
    m_minSizeSpin = new QSpinBox();
    m_minSizeSpin->setRange(0, 1000000);
    m_minSizeSpin->setValue(0);
    m_minSizeSpin->setSuffix(" KB");
    
    m_maxSizeSpin = new QSpinBox();
    m_maxSizeSpin->setRange(0, 1000000);
    m_maxSizeSpin->setValue(0);
    m_maxSizeSpin->setSuffix(" KB");
    m_maxSizeSpin->setSpecialValueText("No limit");
    
    m_fromDateEdit = new QDateTimeEdit();
    m_fromDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-30));
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    
    m_toDateEdit = new QDateTimeEdit();
    m_toDateEdit->setDateTime(QDateTime::currentDateTime());
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    
    m_regexFilterEdit = new QLineEdit();
    m_regexFilterEdit->setPlaceholderText("Regular expression pattern");
    
    m_applyFilterButton = new QPushButton("Apply Filters");
    m_clearFilterButton = new QPushButton("Clear Filters");
    
    filterLayout->addWidget(new QLabel("Name:"), 0, 0);
    filterLayout->addWidget(m_nameFilterEdit, 0, 1, 1, 3);
    
    filterLayout->addWidget(new QLabel("Size Range:"), 1, 0);
    filterLayout->addWidget(new QLabel("Min:"), 1, 1);
    filterLayout->addWidget(m_minSizeSpin, 1, 2);
    filterLayout->addWidget(new QLabel("Max:"), 1, 3);
    filterLayout->addWidget(m_maxSizeSpin, 1, 4);
    
    filterLayout->addWidget(new QLabel("Date Range:"), 2, 0);
    filterLayout->addWidget(new QLabel("From:"), 2, 1);
    filterLayout->addWidget(m_fromDateEdit, 2, 2);
    filterLayout->addWidget(new QLabel("To:"), 2, 3);
    filterLayout->addWidget(m_toDateEdit, 2, 4);
    
    filterLayout->addWidget(new QLabel("Regex:"), 3, 0);
    filterLayout->addWidget(m_regexFilterEdit, 3, 1, 1, 3);
    
    filterLayout->addWidget(m_applyFilterButton, 4, 2);
    filterLayout->addWidget(m_clearFilterButton, 4, 3);
    
    // Properties panel
    m_propertiesGroup = new QGroupBox("File Properties");
    QFormLayout* propsLayout = new QFormLayout(m_propertiesGroup);
    
    m_nameLabel = new QLabel();
    m_pathLabel = new QLabel();
    m_sizeLabel = new QLabel();
    m_typeLabel = new QLabel();
    m_modifiedLabel = new QLabel();
    m_permissionsLabel = new QLabel();
    
    propsLayout->addRow("Name:", m_nameLabel);
    propsLayout->addRow("Path:", m_pathLabel);
    propsLayout->addRow("Size:", m_sizeLabel);
    propsLayout->addRow("Type:", m_typeLabel);
    propsLayout->addRow("Modified:", m_modifiedLabel);
    propsLayout->addRow("Permissions:", m_permissionsLabel);
    
    // Add widgets to right layout
    rightLayout->addWidget(m_fileTable, 2);
    rightLayout->addWidget(filterGroup);
    rightLayout->addWidget(m_propertiesGroup);
    
    // Add panels to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes(QList<int>() << 300 << 700);
    
    // Add to main layout
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(mainSplitter);
    
    // Set initial path
    m_pathEdit->setText(m_currentPath);
}

void FileExplorer::connectSignals() {
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &FileExplorer::onPathChanged);
    connect(m_upButton, &QPushButton::clicked, this, &FileExplorer::goUp);
    connect(m_homeButton, &QPushButton::clicked, this, &FileExplorer::goHome);
    connect(m_backButton, &QPushButton::clicked, this, &FileExplorer::goBack);
    connect(m_forwardButton, &QPushButton::clicked, this, &FileExplorer::goForward);
    
    connect(m_fileTree, &QTreeWidget::itemClicked, this, &FileExplorer::onFileSelected);
    connect(m_fileTree, &QTreeWidget::itemDoubleClicked, this, &FileExplorer::onFileDoubleClicked);
    connect(m_fileTable, &QTableWidget::itemSelectionChanged, this, &FileExplorer::onFileSelected);
    connect(m_fileTable, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        Q_UNUSED(column);
        if (row >= 0 && row < m_fileTable->rowCount()) {
            QTableWidgetItem* item = m_fileTable->item(row, 0);
            if (item) {
                QString fileName = item->text();
                QString fullPath = QDir(m_currentPath).filePath(fileName);
                emit fileDoubleClicked(fullPath);
            }
        }
    });
    
    connect(m_applyFilterButton, &QPushButton::clicked, this, &FileExplorer::onFilterChanged);
    connect(m_clearFilterButton, &QPushButton::clicked, [this]() {
        m_nameFilterEdit->clear();
        m_minSizeSpin->setValue(0);
        m_maxSizeSpin->setValue(0);
        m_fromDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-30));
        m_toDateEdit->setDateTime(QDateTime::currentDateTime());
        m_regexFilterEdit->clear();
        onFilterChanged();
    });
}

// File selection
QStringList FileExplorer::selectedFiles() const {
    return m_selectedFiles;
}

void FileExplorer::setSelectedFiles(const QStringList& files) {
    m_selectedFiles = files;
    emit fileSelectionChanged();
}

// Directory navigation
QString FileExplorer::currentPath() const {
    return m_currentPath;
}

void FileExplorer::setCurrentPath(const QString& path) {
    if (QDir(path).exists()) {
        m_currentPath = path;
        m_pathEdit->setText(path);
        populateFileTable(path);
        updateNavigationHistory();
        emit directoryChanged(path);
    }
}

// Filtering
void FileExplorer::setFileFilter(const QString& filter) {
    m_nameFilterEdit->setText(filter);
}

QString FileExplorer::fileFilter() const {
    return m_nameFilterEdit->text();
}

void FileExplorer::setSizeFilter(quint64 minSize, quint64 maxSize) {
    m_minSizeSpin->setValue(minSize / 1024); // Convert to KB
    if (maxSize == 0) {
        m_maxSizeSpin->setValue(0); // No limit
    } else {
        m_maxSizeSpin->setValue(maxSize / 1024); // Convert to KB
    }
}

void FileExplorer::getSizeFilter(quint64& minSize, quint64& maxSize) const {
    minSize = m_minSizeSpin->value() * 1024; // Convert to bytes
    int maxValue = m_maxSizeSpin->value();
    maxSize = (maxValue == 0) ? 0 : maxValue * 1024; // Convert to bytes or 0 for no limit
}

void FileExplorer::setDateFilter(const QDateTime& fromDate, const QDateTime& toDate) {
    m_fromDateEdit->setDateTime(fromDate);
    m_toDateEdit->setDateTime(toDate);
}

void FileExplorer::getDateFilter(QDateTime& fromDate, QDateTime& toDate) const {
    fromDate = m_fromDateEdit->dateTime();
    toDate = m_toDateEdit->dateTime();
}

void FileExplorer::setRegexFilter(const QString& pattern) {
    m_regexFilterEdit->setText(pattern);
}

QString FileExplorer::regexFilter() const {
    return m_regexFilterEdit->text();
}

void FileExplorer::refresh() {
    populateFileSystemTree();
    populateFileTable(m_currentPath);
}

void FileExplorer::goUp() {
    QDir dir(m_currentPath);
    if (dir.cdUp()) {
        setCurrentPath(dir.absolutePath());
    }
}

void FileExplorer::goHome() {
    setCurrentPath(QDir::homePath());
}

void FileExplorer::goBack() {
    if (m_currentHistoryIndex > 0) {
        m_currentHistoryIndex--;
        QString path = m_navigationHistory.at(m_currentHistoryIndex);
        m_currentPath = path;
        m_pathEdit->setText(path);
        populateFileTable(path);
        emit directoryChanged(path);
    }
}

void FileExplorer::goForward() {
    if (m_currentHistoryIndex < m_navigationHistory.size() - 1) {
        m_currentHistoryIndex++;
        QString path = m_navigationHistory.at(m_currentHistoryIndex);
        m_currentPath = path;
        m_pathEdit->setText(path);
        populateFileTable(path);
        emit directoryChanged(path);
    }
}

void FileExplorer::onPathChanged() {
    QString path = m_pathEdit->text();
    if (QDir(path).exists()) {
        setCurrentPath(path);
    } else {
        QMessageBox::warning(this, "Invalid Path", "The specified path does not exist.");
        m_pathEdit->setText(m_currentPath);
    }
}

void FileExplorer::onFileSelected() {
    // Get selected files from table
    QList<QTableWidgetItem*> selectedItems = m_fileTable->selectedItems();
    QStringList selectedFiles;
    
    // Get unique row numbers
    QSet<int> selectedRows;
    for (QTableWidgetItem* item : selectedItems) {
        selectedRows.insert(item->row());
    }
    
    // Get file paths for selected rows
    for (int row : selectedRows) {
        QTableWidgetItem* nameItem = m_fileTable->item(row, 0);
        if (nameItem) {
            QString fileName = nameItem->text();
            QString fullPath = QDir(m_currentPath).filePath(fileName);
            selectedFiles.append(fullPath);
        }
    }
    
    m_selectedFiles = selectedFiles;
    emit fileSelectionChanged();
    
    // Update properties panel with first selected file
    if (!selectedFiles.isEmpty()) {
        updateFileProperties(selectedFiles.first());
    } else {
        updateFileProperties("");
    }
}

void FileExplorer::onFileDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        QString fileName = item->text(0);
        QString fullPath = QDir(m_currentPath).filePath(fileName);
        
        QFileInfo fileInfo(fullPath);
        if (fileInfo.isDir()) {
            setCurrentPath(fullPath);
        } else {
            emit fileDoubleClicked(fullPath);
        }
    }
}

void FileExplorer::onFilterChanged() {
    // In a real implementation, this would re-filter the file listings
    // For now, we'll just refresh the current view
    populateFileTable(m_currentPath);
}

void FileExplorer::onItemExpanded(QTreeWidgetItem* item) {
    Q_UNUSED(item);
    // In a real implementation, this would populate subdirectories lazily
}

void FileExplorer::updateNavigationHistory() {
    // Remove forward history if we're not at the end
    while (m_navigationHistory.size() > m_currentHistoryIndex + 1) {
        m_navigationHistory.removeLast();
    }
    
    // Add current path to history
    m_navigationHistory.append(m_currentPath);
    m_currentHistoryIndex = m_navigationHistory.size() - 1;
    
    // Enable/disable navigation buttons
    m_backButton->setEnabled(m_currentHistoryIndex > 0);
    m_forwardButton->setEnabled(m_currentHistoryIndex < m_navigationHistory.size() - 1);
}

void FileExplorer::populateFileSystemTree() {
    m_fileTree->clear();
    
    // Add root directories
#ifdef Q_OS_WIN
    // On Windows, add drive letters
    QFileInfoList drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QTreeWidgetItem* driveItem = new QTreeWidgetItem(m_fileTree);
        driveItem->setText(0, drive.absolutePath());
        driveItem->setText(1, "");
        driveItem->setText(2, "Drive");
        driveItem->setText(3, "");
    }
#else
    // On Unix-like systems, add common root directories
    QStringList rootDirs = {"/", "/home", "/usr", "/var", "/etc"};
    for (const QString& dirPath : rootDirs) {
        if (QDir(dirPath).exists()) {
            QTreeWidgetItem* dirItem = new QTreeWidgetItem(m_fileTree);
            dirItem->setText(0, dirPath);
            dirItem->setText(1, "");
            dirItem->setText(2, "Directory");
            dirItem->setText(3, "");
        }
    }
#endif
}

void FileExplorer::populateFileTable(const QString& path) {
    m_fileTable->setRowCount(0);
    
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }
    
    // Get file filter
    QString nameFilter = m_nameFilterEdit->text();
    QStringList nameFilters;
    if (!nameFilter.isEmpty()) {
        nameFilters << nameFilter;
    }
    
    // Get files
    QFileInfoList fileInfos = dir.entryInfoList(nameFilters, QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
    
    // Apply size and date filters
    quint64 minSize, maxSize;
    getSizeFilter(minSize, maxSize);
    
    QDateTime fromDate, toDate;
    getDateFilter(fromDate, toDate);
    
    QString regexPattern = m_regexFilterEdit->text();
    QRegularExpression regex;
    if (!regexPattern.isEmpty()) {
        regex.setPattern(regexPattern);
    }
    
    for (const QFileInfo& fileInfo : fileInfos) {
        // Size filter
        if (minSize > 0 && static_cast<quint64>(fileInfo.size()) < minSize) {
            continue;
        }
        if (maxSize > 0 && static_cast<quint64>(fileInfo.size()) > maxSize) {
            continue;
        }
        
        // Date filter
        if (fileInfo.lastModified() < fromDate || fileInfo.lastModified() > toDate) {
            continue;
        }
        
        // Regex filter
        if (!regexPattern.isEmpty() && !regex.match(fileInfo.fileName()).hasMatch()) {
            continue;
        }
        
        // Add to table
        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        
        // Format file size
        QString sizeStr;
        if (fileInfo.isDir()) {
            sizeStr = "<DIR>";
        } else {
            qint64 size = fileInfo.size();
            if (size < 1024) {
                sizeStr = QString("%1 B").arg(size);
            } else if (size < 1024 * 1024) {
                sizeStr = QString("%1 KB").arg(size / 1024);
            } else if (size < 1024 * 1024 * 1024) {
                sizeStr = QString("%1 MB").arg(size / (1024 * 1024));
            } else {
                sizeStr = QString("%1 GB").arg(size / (1024 * 1024 * 1024));
            }
        }
        
        // Format file type
        QString typeStr = fileInfo.isDir() ? "Directory" : fileInfo.suffix().toUpper() + " File";
        
        // Format permissions
        QString permStr;
        QFile::Permissions perms = fileInfo.permissions();
        permStr += (perms & QFile::ReadOwner) ? "R" : "-";
        permStr += (perms & QFile::WriteOwner) ? "W" : "-";
        permStr += (perms & QFile::ExeOwner) ? "X" : "-";
        permStr += (perms & QFile::ReadGroup) ? "R" : "-";
        permStr += (perms & QFile::WriteGroup) ? "W" : "-";
        permStr += (perms & QFile::ExeGroup) ? "X" : "-";
        permStr += (perms & QFile::ReadOther) ? "R" : "-";
        permStr += (perms & QFile::WriteOther) ? "W" : "-";
        permStr += (perms & QFile::ExeOther) ? "X" : "-";
        
        m_fileTable->setItem(row, 0, new QTableWidgetItem(fileInfo.fileName()));
        m_fileTable->setItem(row, 1, new QTableWidgetItem(sizeStr));
        m_fileTable->setItem(row, 2, new QTableWidgetItem(typeStr));
        m_fileTable->setItem(row, 3, new QTableWidgetItem(fileInfo.lastModified().toString("yyyy-MM-dd HH:mm")));
        m_fileTable->setItem(row, 4, new QTableWidgetItem(permStr));
    }
    
    m_fileTable->resizeColumnsToContents();
}

void FileExplorer::updateFileProperties(const QString& path) {
    if (path.isEmpty()) {
        m_nameLabel->setText("");
        m_pathLabel->setText("");
        m_sizeLabel->setText("");
        m_typeLabel->setText("");
        m_modifiedLabel->setText("");
        m_permissionsLabel->setText("");
        return;
    }
    
    QFileInfo fileInfo(path);
    
    m_nameLabel->setText(fileInfo.fileName());
    m_pathLabel->setText(fileInfo.absolutePath());
    
    if (fileInfo.isDir()) {
        m_sizeLabel->setText("<DIR>");
    } else {
        qint64 size = fileInfo.size();
        QString sizeStr;
        if (size < 1024) {
            sizeStr = QString("%1 B").arg(size);
        } else if (size < 1024 * 1024) {
            sizeStr = QString("%1 KB").arg(size / 1024);
        } else if (size < 1024 * 1024 * 1024) {
            sizeStr = QString("%1 MB").arg(size / (1024 * 1024));
        } else {
            sizeStr = QString("%1 GB").arg(size / (1024 * 1024 * 1024));
        }
        m_sizeLabel->setText(sizeStr);
    }
    
    QString typeStr = fileInfo.isDir() ? "Directory" : fileInfo.suffix().toUpper() + " File";
    m_typeLabel->setText(typeStr);
    
    m_modifiedLabel->setText(fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss"));
    
    QFile::Permissions perms = fileInfo.permissions();
    QString permStr;
    permStr += (perms & QFile::ReadOwner) ? "r" : "-";
    permStr += (perms & QFile::WriteOwner) ? "w" : "-";
    permStr += (perms & QFile::ExeOwner) ? "x" : "-";
    permStr += (perms & QFile::ReadGroup) ? "r" : "-";
    permStr += (perms & QFile::WriteGroup) ? "w" : "-";
    permStr += (perms & QFile::ExeGroup) ? "x" : "-";
    permStr += (perms & QFile::ReadOther) ? "r" : "-";
    permStr += (perms & QFile::WriteOther) ? "w" : "-";
    permStr += (perms & QFile::ExeOther) ? "x" : "-";
    
    m_permissionsLabel->setText(permStr);
}