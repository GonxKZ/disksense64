#ifndef UI_ADVANCEDFILTERWIDGET_H
#define UI_ADVANCEDFILTERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QListWidget>
#include <QRegularExpression>
#include <QFileInfo>

class AdvancedFilterWidget : public QWidget {
    Q_OBJECT

public:
    struct FilterCriteria {
        QString namePattern;
        QStringList fileTypes;
        quint64 minSize;
        quint64 maxSize;
        QDateTime fromDate;
        QDateTime toDate;
        QString regexPattern;
        bool includeSubdirs;
        bool showHidden;
        
        FilterCriteria() 
            : minSize(0), maxSize(0), includeSubdirs(true), showHidden(false) {}
    };

public:
    explicit AdvancedFilterWidget(QWidget *parent = nullptr);
    ~AdvancedFilterWidget() override;

    // Get current filter criteria
    FilterCriteria filterCriteria() const;
    
    // Set filter criteria
    void setFilterCriteria(const FilterCriteria& criteria);
    
    // Check if a file matches the current filters
    bool matches(const QFileInfo& fileInfo) const;
    
    // Validate filter criteria
    bool validateFilters(QString& errorMessage) const;

signals:
    void filtersChanged();
    void applyRequested();
    void resetRequested();

public slots:
    void applyFilters();
    void resetFilters();

private slots:
    void onFilterChanged();
    void onAddFileType();
    void onRemoveFileType();

private:
    void setupUI();
    void connectSignals();
    
    // UI components
    QLineEdit* m_namePatternEdit;
    QListWidget* m_fileTypesList;
    QLineEdit* m_fileTypeEdit;
    QPushButton* m_addFileTypeButton;
    QPushButton* m_removeFileTypeButton;
    
    QSpinBox* m_minSizeSpin;
    QSpinBox* m_maxSizeSpin;
    QComboBox* m_sizeUnitCombo;
    
    QDateTimeEdit* m_fromDateEdit;
    QDateTimeEdit* m_toDateEdit;
    
    QLineEdit* m_regexPatternEdit;
    
    QCheckBox* m_includeSubdirsCheck;
    QCheckBox* m_showHiddenCheck;
    
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    
    // Current criteria
    FilterCriteria m_criteria;
};

#endif // UI_ADVANCEDFILTERWIDGET_H