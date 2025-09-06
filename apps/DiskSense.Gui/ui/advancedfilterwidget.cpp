#include "advancedfilterwidget.h"
#include <QApplication>
#include <QGroupBox>
#include <QFormLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QDir>

AdvancedFilterWidget::AdvancedFilterWidget(QWidget *parent)
    : QWidget(parent)
    , m_namePatternEdit(nullptr)
    , m_fileTypesList(nullptr)
    , m_fileTypeEdit(nullptr)
    , m_addFileTypeButton(nullptr)
    , m_removeFileTypeButton(nullptr)
    , m_minSizeSpin(nullptr)
    , m_maxSizeSpin(nullptr)
    , m_sizeUnitCombo(nullptr)
    , m_fromDateEdit(nullptr)
    , m_toDateEdit(nullptr)
    , m_regexPatternEdit(nullptr)
    , m_includeSubdirsCheck(nullptr)
    , m_showHiddenCheck(nullptr)
    , m_applyButton(nullptr)
    , m_resetButton(nullptr)
{
    setupUI();
    connectSignals();
    
    // Set default values
    resetFilters();
}

AdvancedFilterWidget::~AdvancedFilterWidget() {
}

void AdvancedFilterWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Name pattern filter
    QGroupBox* nameGroup = new QGroupBox("Name Pattern");
    QHBoxLayout* nameLayout = new QHBoxLayout(nameGroup);
    m_namePatternEdit = new QLineEdit();
    m_namePatternEdit->setPlaceholderText("e.g., *.txt, report_*.pdf");
    nameLayout->addWidget(m_namePatternEdit);
    
    // File types filter
    QGroupBox* typesGroup = new QGroupBox("File Types");
    QVBoxLayout* typesLayout = new QVBoxLayout(typesGroup);
    
    m_fileTypesList = new QListWidget();
    m_fileTypesList->setMaximumHeight(100);
    
    QHBoxLayout* typeEditLayout = new QHBoxLayout();
    m_fileTypeEdit = new QLineEdit();
    m_fileTypeEdit->setPlaceholderText("Enter file extension (e.g., txt, pdf)");
    m_addFileTypeButton = new QPushButton("Add");
    m_removeFileTypeButton = new QPushButton("Remove");
    typeEditLayout->addWidget(m_fileTypeEdit);
    typeEditLayout->addWidget(m_addFileTypeButton);
    typeEditLayout->addWidget(m_removeFileTypeButton);
    
    typesLayout->addWidget(m_fileTypesList);
    typesLayout->addLayout(typeEditLayout);
    
    // Size range filter
    QGroupBox* sizeGroup = new QGroupBox("File Size");
    QGridLayout* sizeLayout = new QGridLayout(sizeGroup);
    
    m_minSizeSpin = new QSpinBox();
    m_minSizeSpin->setRange(0, 1000000);
    m_minSizeSpin->setValue(0);
    
    m_maxSizeSpin = new QSpinBox();
    m_maxSizeSpin->setRange(0, 1000000);
    m_maxSizeSpin->setValue(0);
    m_maxSizeSpin->setSpecialValueText("No limit");
    
    m_sizeUnitCombo = new QComboBox();
    m_sizeUnitCombo->addItems(QStringList() << "Bytes" << "KB" << "MB" << "GB");
    m_sizeUnitCombo->setCurrentIndex(2); // MB
    
    sizeLayout->addWidget(new QLabel("Min Size:"), 0, 0);
    sizeLayout->addWidget(m_minSizeSpin, 0, 1);
    sizeLayout->addWidget(m_sizeUnitCombo, 0, 2);
    
    sizeLayout->addWidget(new QLabel("Max Size:"), 1, 0);
    sizeLayout->addWidget(m_maxSizeSpin, 1, 1);
    sizeLayout->addWidget(m_sizeUnitCombo, 1, 2);
    
    // Date range filter
    QGroupBox* dateGroup = new QGroupBox("Date Modified");
    QGridLayout* dateLayout = new QGridLayout(dateGroup);
    
    m_fromDateEdit = new QDateTimeEdit();
    m_fromDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-30));
    m_fromDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_fromDateEdit->setCalendarPopup(true);
    
    m_toDateEdit = new QDateTimeEdit();
    m_toDateEdit->setDateTime(QDateTime::currentDateTime());
    m_toDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_toDateEdit->setCalendarPopup(true);
    
    dateLayout->addWidget(new QLabel("From:"), 0, 0);
    dateLayout->addWidget(m_fromDateEdit, 0, 1);
    dateLayout->addWidget(new QLabel("To:"), 1, 0);
    dateLayout->addWidget(m_toDateEdit, 1, 1);
    
    // Regex pattern filter
    QGroupBox* regexGroup = new QGroupBox("Regular Expression");
    QHBoxLayout* regexLayout = new QHBoxLayout(regexGroup);
    m_regexPatternEdit = new QLineEdit();
    m_regexPatternEdit->setPlaceholderText("Enter regex pattern");
    regexLayout->addWidget(m_regexPatternEdit);
    
    // Options
    QWidget* optionsWidget = new QWidget();
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsWidget);
    m_includeSubdirsCheck = new QCheckBox("Include subdirectories");
    m_includeSubdirsCheck->setChecked(true);
    m_showHiddenCheck = new QCheckBox("Show hidden files");
    optionsLayout->addWidget(m_includeSubdirsCheck);
    optionsLayout->addWidget(m_showHiddenCheck);
    optionsLayout->addStretch();
    
    // Buttons
    QWidget* buttonsWidget = new QWidget();
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsWidget);
    m_applyButton = new QPushButton("Apply Filters");
    m_resetButton = new QPushButton("Reset Filters");
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_applyButton);
    buttonsLayout->addWidget(m_resetButton);
    
    // Add all to main layout
    mainLayout->addWidget(nameGroup);
    mainLayout->addWidget(typesGroup);
    mainLayout->addWidget(sizeGroup);
    mainLayout->addWidget(dateGroup);
    mainLayout->addWidget(regexGroup);
    mainLayout->addWidget(optionsWidget);
    mainLayout->addWidget(buttonsWidget);
}

void AdvancedFilterWidget::connectSignals() {
    connect(m_namePatternEdit, &QLineEdit::textChanged, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_minSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_maxSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_sizeUnitCombo, &QComboBox::currentTextChanged, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_fromDateEdit, &QDateTimeEdit::dateTimeChanged, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_toDateEdit, &QDateTimeEdit::dateTimeChanged, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_regexPatternEdit, &QLineEdit::textChanged, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_includeSubdirsCheck, &QCheckBox::toggled, this, &AdvancedFilterWidget::onFilterChanged);
    connect(m_showHiddenCheck, &QCheckBox::toggled, this, &AdvancedFilterWidget::onFilterChanged);
    
    connect(m_addFileTypeButton, &QPushButton::clicked, this, &AdvancedFilterWidget::onAddFileType);
    connect(m_removeFileTypeButton, &QPushButton::clicked, this, &AdvancedFilterWidget::onRemoveFileType);
    
    connect(m_applyButton, &QPushButton::clicked, this, &AdvancedFilterWidget::applyFilters);
    connect(m_resetButton, &QPushButton::clicked, this, &AdvancedFilterWidget::resetFilters);
}

AdvancedFilterWidget::FilterCriteria AdvancedFilterWidget::filterCriteria() const {
    FilterCriteria criteria;
    
    criteria.namePattern = m_namePatternEdit->text();
    criteria.minSize = m_minSizeSpin->value();
    criteria.maxSize = m_maxSizeSpin->value();
    
    // Convert to bytes based on unit
    int multiplier = 1;
    switch (m_sizeUnitCombo->currentIndex()) {
        case 0: multiplier = 1; break;             // Bytes
        case 1: multiplier = 1024; break;          // KB
        case 2: multiplier = 1024 * 1024; break;   // MB
        case 3: multiplier = 1024 * 1024 * 1024; break; // GB
    }
    
    criteria.minSize *= multiplier;
    if (criteria.maxSize > 0) {
        criteria.maxSize *= multiplier;
    }
    
    criteria.fromDate = m_fromDateEdit->dateTime();
    criteria.toDate = m_toDateEdit->dateTime();
    criteria.regexPattern = m_regexPatternEdit->text();
    criteria.includeSubdirs = m_includeSubdirsCheck->isChecked();
    criteria.showHidden = m_showHiddenCheck->isChecked();
    
    // Get file types
    for (int i = 0; i < m_fileTypesList->count(); ++i) {
        criteria.fileTypes.append(m_fileTypesList->item(i)->text());
    }
    
    return criteria;
}

void AdvancedFilterWidget::setFilterCriteria(const FilterCriteria& criteria) {
    m_namePatternEdit->setText(criteria.namePattern);
    
    // Set file types
    m_fileTypesList->clear();
    m_fileTypesList->addItems(criteria.fileTypes);
    
    // Convert from bytes to selected unit for display
    int unitIndex = 2; // Default to MB
    quint64 displayMin = criteria.minSize;
    quint64 displayMax = criteria.maxSize;
    
    if (criteria.minSize > 0 || criteria.maxSize > 0) {
        if (criteria.maxSize < 1024 && criteria.maxSize > 0) {
            unitIndex = 0; // Bytes
        } else if (criteria.maxSize < 1024 * 1024 && criteria.maxSize > 0) {
            unitIndex = 1; // KB
        } else if (criteria.maxSize < 1024 * 1024 * 1024 && criteria.maxSize > 0) {
            unitIndex = 2; // MB
        } else {
            unitIndex = 3; // GB
        }
        
        int divisor = 1;
        switch (unitIndex) {
            case 0: divisor = 1; break;
            case 1: divisor = 1024; break;
            case 2: divisor = 1024 * 1024; break;
            case 3: divisor = 1024 * 1024 * 1024; break;
        }
        
        if (divisor > 0) {
            displayMin = criteria.minSize / divisor;
            if (criteria.maxSize > 0) {
                displayMax = criteria.maxSize / divisor;
            }
        }
    }
    
    m_minSizeSpin->setValue(displayMin);
    m_maxSizeSpin->setValue(displayMax);
    m_sizeUnitCombo->setCurrentIndex(unitIndex);
    
    m_fromDateEdit->setDateTime(criteria.fromDate);
    m_toDateEdit->setDateTime(criteria.toDate);
    m_regexPatternEdit->setText(criteria.regexPattern);
    m_includeSubdirsCheck->setChecked(criteria.includeSubdirs);
    m_showHiddenCheck->setChecked(criteria.showHidden);
    
    m_criteria = criteria;
}

bool AdvancedFilterWidget::matches(const QFileInfo& fileInfo) const {
    // Name pattern filter
    if (!m_criteria.namePattern.isEmpty()) {
        QRegularExpression pattern = QRegularExpression::fromWildcard(m_criteria.namePattern, Qt::CaseInsensitive);
        if (!pattern.match(fileInfo.fileName()).hasMatch()) {
            return false;
        }
    }
    
    // File type filter
    if (!m_criteria.fileTypes.isEmpty()) {
        bool typeMatch = false;
        QString suffix = fileInfo.suffix().toLower();
        for (const QString& type : m_criteria.fileTypes) {
            if (type.toLower() == suffix) {
                typeMatch = true;
                break;
            }
        }
        if (!typeMatch) {
            return false;
        }
    }
    
    // Size filter
    if (m_criteria.minSize > 0 && static_cast<quint64>(fileInfo.size()) < m_criteria.minSize) {
        return false;
    }
    if (m_criteria.maxSize > 0 && static_cast<quint64>(fileInfo.size()) > m_criteria.maxSize) {
        return false;
    }
    
    // Date filter
    if (fileInfo.lastModified() < m_criteria.fromDate || fileInfo.lastModified() > m_criteria.toDate) {
        return false;
    }
    
    // Regex filter
    if (!m_criteria.regexPattern.isEmpty()) {
        QRegularExpression regex(m_criteria.regexPattern);
        if (!regex.isValid() || !regex.match(fileInfo.fileName()).hasMatch()) {
            return false;
        }
    }
    
    return true;
}

bool AdvancedFilterWidget::validateFilters(QString& errorMessage) const {
    // Validate regex pattern
    if (!m_regexPatternEdit->text().isEmpty()) {
        QRegularExpression regex(m_regexPatternEdit->text());
        if (!regex.isValid()) {
            errorMessage = "Invalid regular expression pattern";
            return false;
        }
    }
    
    // Validate date range
    if (m_fromDateEdit->dateTime() > m_toDateEdit->dateTime()) {
        errorMessage = "From date must be before To date";
        return false;
    }
    
    return true;
}

void AdvancedFilterWidget::applyFilters() {
    QString errorMessage;
    if (!validateFilters(errorMessage)) {
        QMessageBox::warning(this, "Invalid Filters", errorMessage);
        return;
    }
    
    m_criteria = filterCriteria();
    emit filtersChanged();
    emit applyRequested();
}

void AdvancedFilterWidget::resetFilters() {
    m_namePatternEdit->clear();
    m_fileTypesList->clear();
    m_minSizeSpin->setValue(0);
    m_maxSizeSpin->setValue(0);
    m_sizeUnitCombo->setCurrentIndex(2); // MB
    m_fromDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-30));
    m_toDateEdit->setDateTime(QDateTime::currentDateTime());
    m_regexPatternEdit->clear();
    m_includeSubdirsCheck->setChecked(true);
    m_showHiddenCheck->setChecked(false);
    
    m_criteria = FilterCriteria();
    emit filtersChanged();
    emit resetRequested();
}

void AdvancedFilterWidget::onFilterChanged() {
    // Emit signal that filters have changed
    emit filtersChanged();
}

void AdvancedFilterWidget::onAddFileType() {
    QString fileType = m_fileTypeEdit->text().trimmed();
    if (!fileType.isEmpty()) {
        // Check if already exists
        bool exists = false;
        for (int i = 0; i < m_fileTypesList->count(); ++i) {
            if (m_fileTypesList->item(i)->text().toLower() == fileType.toLower()) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            m_fileTypesList->addItem(fileType.toLower());
            m_fileTypeEdit->clear();
        }
    }
}

void AdvancedFilterWidget::onRemoveFileType() {
    QListWidgetItem* item = m_fileTypesList->currentItem();
    if (item) {
        delete item;
    }
}