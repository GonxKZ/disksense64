#include "trashrestoredialog.h"
#include <QHeaderView>
#include <QMessageBox>

TrashRestoreDialog::TrashRestoreDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Restore from Trash")); resize(700, 420);
    QVBoxLayout* l = new QVBoxLayout(this);
    m_table = new QTableWidget(); m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Trashed"), tr("Original"), tr("Deleted")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    l->addWidget(m_table);
    m_restoreBtn = new QPushButton(tr("Restore Selected"));
    l->addWidget(m_restoreBtn);
    connect(m_restoreBtn, &QPushButton::clicked, this, &TrashRestoreDialog::onRestore);
    m_restoreBtn->setDefault(true);
    setTabOrder(m_table, m_restoreBtn);
    loadTrash();
}

void TrashRestoreDialog::loadTrash() {
    m_entries = platform::list_trash();
    m_table->setRowCount((int)m_entries.size());
    for (int i=0;i<(int)m_entries.size();++i) {
        const auto& e = m_entries[i];
        m_table->setItem(i,0,new QTableWidgetItem(QString::fromStdString(e.trashed_path)));
        m_table->setItem(i,1,new QTableWidgetItem(QString::fromStdString(e.original_path)));
        m_table->setItem(i,2,new QTableWidgetItem(QString::fromStdString(e.deletion_date)));
    }
}

void TrashRestoreDialog::onRestore() {
    auto ranges = m_table->selectedRanges(); if (ranges.isEmpty()) return;
    int row = ranges.first().topRow(); if (row<0 || row >= (int)m_entries.size()) return;
    auto e = m_entries[row];
    bool ok = platform::restore_from_trash(e);
    if (ok) QMessageBox::information(this, tr("Restore"), tr("Restored to original location."));
    else QMessageBox::warning(this, tr("Restore"), tr("Failed to restore."));
    loadTrash();
}
