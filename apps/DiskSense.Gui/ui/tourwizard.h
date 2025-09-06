#ifndef UI_TOURWIZARD_H
#define UI_TOURWIZARD_H

#include <QWizard>

class TourWizard : public QWizard {
    Q_OBJECT
public:
    explicit TourWizard(QWidget* parent=nullptr);
};

#endif // UI_TOURWIZARD_H

