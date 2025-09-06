#ifndef COMPONENTS_TREEMAPWIDGET_H
#define COMPONENTS_TREEMAPWIDGET_H

#include <QWidget>
#include <memory>
#include <QFileInfo>
#include "core/gfx/treemap.h"

class TreemapWidget : public QWidget {
    Q_OBJECT

public:
    explicit TreemapWidget(QWidget *parent = nullptr);
    ~TreemapWidget();

    void setTreemap(std::unique_ptr<TreemapNode> root);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    std::unique_ptr<TreemapNode> m_root;
    TreemapNode* m_hoveredNode;
    double m_zoomLevel;
    QPoint m_panOffset;
    QPoint m_lastMousePos;
    bool m_isPanning;
    
    void drawNode(QPainter& painter, const TreemapNode* node);
    TreemapNode* hitTest(const TreemapNode* node, const QPoint& pos) const;
    void updateZoomAndPan();
    QRectF nodeRectToWidgetRect(const Rect& nodeRect) const;
    QString formatFileSize(uint64_t size) const;
};

#endif // COMPONENTS_TREEMAPWIDGET_H
