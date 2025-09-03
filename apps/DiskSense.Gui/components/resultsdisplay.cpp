#include "resultsdisplay.h"
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QDateTime>
#include <QScrollArea>

ResultsDisplay::ResultsDisplay(QWidget *parent) 
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Status label
    m_statusLabel = new QLabel("Ready");
    mainLayout->addWidget(m_statusLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Text display with scroll area
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    
    m_textDisplay = new QTextEdit();
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setLineWrapMode(QTextEdit::NoWrap);
    
    scrollArea->setWidget(m_textDisplay);
    mainLayout->addWidget(scrollArea, 1);
    
    // Cancel button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setVisible(false);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // Connect cancel button
    connect(m_cancelButton, &QPushButton::clicked, this, &ResultsDisplay::cancelRequested);
}

ResultsDisplay::~ResultsDisplay() {
}

void ResultsDisplay::addMessage(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] %2").arg(timestamp, message);
    m_textDisplay->append(formattedMessage);
    
    // Scroll to bottom
    QScrollBar* scrollBar = m_textDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ResultsDisplay::clear() {
    m_textDisplay->clear();
}

void ResultsDisplay::setProgress(int value) {
    m_progressBar->setValue(value);
}

void ResultsDisplay::setProgressRange(int min, int max) {
    m_progressBar->setRange(min, max);
}

void ResultsDisplay::showProgress(bool show) {
    m_progressBar->setVisible(show);
    m_cancelButton->setVisible(show);
    m_statusLabel->setVisible(!show);
}