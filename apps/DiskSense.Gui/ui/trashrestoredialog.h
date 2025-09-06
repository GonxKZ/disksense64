#ifndef UI_TRASHRESTOREDIALOG_H
#define UI_TRASHRESTOREDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <vector>
#include "platform/util/trash.h"

class TrashRestoreDialog : public QDialog {
    Q_OBJECT
public:
    explicit TrashRestoreDialog(QWidget* parent=nullptr);

private slots:
    void onRestore();

private:
    void loadTrash();
    QTableWidget* m_table;
    QPushButton* m_restoreBtn;
    std::vector<platform::TrashEntry> m_entries;
};

#endif // UI_TRASHRESTOREDIALOG_H

