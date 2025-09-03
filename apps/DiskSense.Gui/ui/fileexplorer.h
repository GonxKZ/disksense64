#ifndef UI_FILEEXPLORER_H
#define UI_FILEEXPLORER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget *parent = nullptr);
    ~FileExplorer() override;

    // File selection
    QStringList selectedFiles() const;
    void setSelectedFiles(const QStringList& files);
    
    // Directory navigation
    QString currentPath() const;
    void setCurrentPath(const QString& path);
    
    // Filtering
    void setFileFilter(const QString& filter);
    QString fileFilter() const;
    
    void setSizeFilter(quint64 minSize, quint64 maxSize);
    void getSizeFilter(quint64& minSize, quint64& maxSize) const;
    
    void setDateFilter(const QDateTime& fromDate, const QDateTime& toDate);
    void getDateFilter(QDateTime& fromDate, QDateTime& toDate) const;
    
    void setRegexFilter(const QString& pattern);
    QString regexFilter() const;

signals:
    void fileSelectionChanged();
    void directoryChanged(const QString& path);
    void fileDoubleClicked(const QString& path);

public slots:
    void refresh();
    void goUp();
    void goHome();
    void goBack();
    void goForward();

private slots:
    void onPathChanged();
    void onFileSelected();
    void onFileDoubleClicked(QTreeWidgetItem* item, int column);
    void onFilterChanged();
    void onItemExpanded(QTreeWidgetItem* item);

private:
    void setupUI();
    void connectSignals();
    void updateNavigationHistory();
    void populateFileSystemTree();
    void populateFileTable(const QString& path);
    void updateFileProperties(const QString& path);
    
    // UI components
    QLineEdit* m_pathEdit;
    QPushButton* m_upButton;
    QPushButton* m_homeButton;
    QPushButton* m_backButton;
    QPushButton* m_forwardButton;
    
    QTreeWidget* m_fileTree;
    QTableWidget* m_fileTable;
    
    // Filter widgets
    QLineEdit* m_nameFilterEdit;
    QSpinBox* m_minSizeSpin;
    QSpinBox* m_maxSizeSpin;
    QDateTimeEdit* m_fromDateEdit;
    QDateTimeEdit* m_toDateEdit;
    QLineEdit* m_regexFilterEdit;
    QPushButton* m_applyFilterButton;
    QPushButton* m_clearFilterButton;
    
    // Properties panel
    QGroupBox* m_propertiesGroup;
    QLabel* m_nameLabel;
    QLabel* m_pathLabel;
    QLabel* m_sizeLabel;
    QLabel* m_typeLabel;
    QLabel* m_modifiedLabel;
    QLabel* m_permissionsLabel;
    
    // Navigation history
    QStringList m_navigationHistory;
    int m_currentHistoryIndex;
    
    // Current state
    QString m_currentPath;
    QStringList m_selectedFiles;
    
    // File system model
    QFileSystemModel* m_fileSystemModel;
    QSortFilterProxyModel* m_proxyModel;
};

#endif // UI_FILEEXPLORER_H