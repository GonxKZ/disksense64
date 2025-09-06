#include "onboardingdialog.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QDir>
#include "core/safety/safety.h"

OnboardingDialog::OnboardingDialog(QWidget* parent)
    : QDialog(parent)
    , m_pathEdit(new QLineEdit())
    , m_showOnStartupCheck(new QCheckBox(tr("Mostrar este asistente al inicio")))
    , m_safetyLabel(new QLabel())
    , m_startButton(new QPushButton(tr("Comenzar")))
    , m_cancelButton(new QPushButton(tr("Cancelar")))
{
    setWindowTitle(tr("Bienvenido a DiskSense64"));
    resize(560, 260);

    QVBoxLayout* main = new QVBoxLayout(this);

    QLabel* title = new QLabel(tr("Configura tu primera experiencia"));
    QFont f = title->font(); f.setPointSize(f.pointSize()+2); f.setBold(true); title->setFont(f);
    main->addWidget(title);

    QLabel* desc = new QLabel(tr("DiskSense64 analiza tu disco de forma segura. Por defecto, NUNCA borra archivos (Modo Seguro). Puedes cambiar preferencias más tarde en Ajustes."));
    desc->setWordWrap(true);
    main->addWidget(desc);

    QFormLayout* form = new QFormLayout();
    QWidget* pathWidget = new QWidget();
    QHBoxLayout* pathLayout = new QHBoxLayout(pathWidget); pathLayout->setContentsMargins(0,0,0,0);
    QPushButton* browse = new QPushButton(tr("Examinar..."));
    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(browse);
    QLabel* pathLabel = new QLabel(tr("&Carpeta inicial:"));
    pathLabel->setBuddy(m_pathEdit);
    form->addRow(pathLabel, pathWidget);
    main->addLayout(form);

    // Safety banner text (informativo)
    bool delAllowed = safety::deletion_allowed();
    QString safetyText = delAllowed
        ? tr("Advertencia: el Modo Seguro está DESACTIVADO (variable DISKSENSE_ALLOW_DELETE=1). Evita acciones destructivas.")
        : tr("Modo Seguro ACTIVO: la aplicación no realizará borrados permanentes.");
    m_safetyLabel->setText(safetyText);
    m_safetyLabel->setObjectName("safetyLabel");
    main->addWidget(m_safetyLabel);

    // Options
    m_showOnStartupCheck->setChecked(true);
    main->addWidget(m_showOnStartupCheck);

    // Buttons
    QHBoxLayout* btns = new QHBoxLayout();
    btns->addStretch();
    btns->addWidget(m_startButton);
    btns->addWidget(m_cancelButton);
    main->addLayout(btns);

    connect(browse, &QPushButton::clicked, this, &OnboardingDialog::onBrowse);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_startButton, &QPushButton::clicked, this, &OnboardingDialog::onAccept);

    loadSettings();

    // Accessibility & keyboard
    m_pathEdit->setAccessibleName(tr("Carpeta inicial"));
    m_startButton->setDefault(true);
    m_startButton->setAutoDefault(true);
    setTabOrder(m_pathEdit, m_startButton);
    setTabOrder(m_startButton, m_cancelButton);
}

OnboardingDialog::~OnboardingDialog() {}

void OnboardingDialog::loadSettings() {
    QSettings s;
    QString defPath = s.value("ui/defaultPath", QDir::homePath()).toString();
    m_pathEdit->setText(defPath);
    bool show = s.value("onboarding/showOnStartup", true).toBool();
    m_showOnStartupCheck->setChecked(show);
}

void OnboardingDialog::saveSettings() {
    QSettings s;
    s.setValue("ui/defaultPath", m_pathEdit->text());
    s.setValue("onboarding/showOnStartup", m_showOnStartupCheck->isChecked());
    // Marca el asistente como completado si el usuario decide no mostrarlo más
    s.setValue("onboarding/completed", !m_showOnStartupCheck->isChecked());
}

void OnboardingDialog::onBrowse() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Selecciona carpeta"), m_pathEdit->text());
    if (!dir.isEmpty()) m_pathEdit->setText(dir);
}

void OnboardingDialog::onAccept() {
    saveSettings();
    emit startScanRequested(m_pathEdit->text());
    accept();
}
