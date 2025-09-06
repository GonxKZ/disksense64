#include "tourwizard.h"
#include <QWizardPage>
#include <QLabel>
#include <QVBoxLayout>

static QWizardPage* makePage(const QString& title, const QString& body) {
    QWizardPage* p = new QWizardPage(); p->setTitle(title);
    QVBoxLayout* l = new QVBoxLayout(p); QLabel* lab = new QLabel(body); lab->setWordWrap(true); l->addWidget(lab); return p;
}

TourWizard::TourWizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle(tr("Tour guiado")); resize(640, 420);
    addPage(makePage(tr("Bienvenido"), tr("Este tour te muestra las áreas clave: Deduplicación, Residuo, Visualización y Seguridad. Todas las acciones son seguras por defecto gracias al Modo Seguro.")));
    addPage(makePage(tr("Deduplicación"), tr("Encuentra archivos duplicados por tamaño, firma head/tail y hash completo. En Modo Seguro, las acciones son simuladas o movidas a Papelera.")));
    addPage(makePage(tr("Residuo"), tr("Detecta archivos temporales/logs/dir vacíos. Aplica limpieza moviendo a Papelera o a Cuartentena con deshacer.")));
    addPage(makePage(tr("Visualización"), tr("Treemap, Sunburst y gráficos por tipo/edad. Usa el filtro para ajustar el análisis.")));
    addPage(makePage(tr("Seguridad"), tr("Integra YARA para escaneo de reglas. Opcionalmente, perfiles de borrado seguro conforme a NIST (Clear/Purge).")));
}

