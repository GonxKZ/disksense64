#ifndef COMPONENTS_SUNBURSTWIDGET_H
#define COMPONENTS_SUNBURSTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QFileInfo>

class SunburstWidget : public QWidget {
    Q_OBJECT
public:
    explicit SunburstWidget(QWidget* parent=nullptr);
    void setRootDirectory(const QString& dir);
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    struct Node { QString name; quint64 size=0; QList<Node> children; };
    Node m_root;
    void buildTree(const QString& dir, Node& out, int depth, int maxDepth);
    void drawRing(QPainter& p, const QRectF& rect, const Node& node, int depth, int maxDepth, double startAngle, double spanAngle);
};

#endif // COMPONENTS_SUNBURSTWIDGET_H

