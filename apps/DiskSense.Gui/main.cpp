#include <QApplication>
#include "ui/mainwindow.h"
#include "platform/i18n.h"
#include "ui/onboardingdialog.h"
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("DiskSense64");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("DiskSense");
    
    // Lightweight modern dark theme
    QString qss = R"(
        QMainWindow { background: #111315; }
        QWidget { color: #EAEAEA; font-size: 10.5pt; }
        QToolBar, QMenuBar { background: #1A1D20; color: #EAEAEA; }
        QStatusBar { background: #1A1D20; color: #A8ADB2; }
        QTabWidget::pane { border: 1px solid #2A2E33; }
        QTabBar::tab { background: #1A1D20; padding: 7px 12px; border: 1px solid #2A2E33; border-bottom: none; }
        QTabBar::tab:selected { background: #23282D; }
        QGroupBox { border: 1px solid #2A2E33; margin-top: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }
        QProgressBar { border: 1px solid #2A2E33; background: #1A1D20; }
        QProgressBar::chunk { background: #00A3FF; }
        QPushButton { background: #1A1D20; border: 1px solid #2A2E33; padding: 7px 12px; border-radius: 4px; }
        QPushButton:hover { background: #23282D; }
        QPushButton:disabled { color: #6C737A; }
        QTextEdit { background: #0F1113; border: 1px solid #2A2E33; }
        QLineEdit, QComboBox, QSpinBox, QDateTimeEdit { background: #0F1113; border: 1px solid #2A2E33; padding: 5px; border-radius: 3px; }
        QCheckBox::indicator, QRadioButton::indicator { width: 14px; height: 14px; }
        QLabel#safetyBanner { color: #FFD166; font-weight: 600; }
    )";
    app.setStyleSheet(qss);

    // Load i18n (loads saved language and installs translator)
    I18nManager i18n;
    
    MainWindow window;

    // Onboarding al inicio
    QSettings s;
    bool showOnboarding = s.value("onboarding/showOnStartup", true).toBool();
    bool completed = s.value("onboarding/completed", false).toBool();
    if (showOnboarding || !completed) {
        OnboardingDialog wizard(&window);
        QObject::connect(&wizard, &OnboardingDialog::startScanRequested, &window, &MainWindow::onDashboardScanRequested);
        wizard.exec();
    }

    window.show();
    
    return app.exec();
}
