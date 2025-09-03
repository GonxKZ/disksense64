#ifndef COMPONENTS_RESULTSDISPLAY_H
#define COMPONENTS_RESULTSDISPLAY_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <memory>

class ResultsDisplay : public QWidget {
    Q_OBJECT

public:
    explicit ResultsDisplay(QWidget *parent = nullptr);
    ~ResultsDisplay();

    void addMessage(const QString& message);
    void clear();
    void setProgress(int value);
    void setProgressRange(int min, int max);
    void showProgress(bool show);

signals:
    void cancelRequested();

private:
    QTextEdit* m_textDisplay;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_cancelButton;
};

#endif // COMPONENTS_RESULTSDISPLAY_H