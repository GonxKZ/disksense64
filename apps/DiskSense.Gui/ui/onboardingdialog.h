#ifndef UI_ONBOARDINGDIALOG_H
#define UI_ONBOARDINGDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSettings>

class OnboardingDialog : public QDialog {
    Q_OBJECT
public:
    explicit OnboardingDialog(QWidget* parent=nullptr);
    ~OnboardingDialog() override;

signals:
    void startScanRequested(const QString& path);

private slots:
    void onBrowse();
    void onAccept();

private:
    void loadSettings();
    void saveSettings();

    QLineEdit* m_pathEdit;
    QCheckBox* m_showOnStartupCheck;
    QLabel* m_safetyLabel;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
};

#endif // UI_ONBOARDINGDIALOG_H

