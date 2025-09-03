#ifndef CORE_GFX_QTREEMAP_H
#define CORE_GFX_QTREEMAP_H

#include <QWidget>
#include <memory>
#include <QFileInfo>
#include "core/gfx/treemap.h"

class QTreemap : public QWidget {
    Q_OBJECT

public:
    explicit QTreemap(QWidget *parent = nullptr);
    ~QTreemap();

    void setTreemap(std::unique_ptr<TreemapNode> root);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::unique_ptr<TreemapNode> m_root;
    void drawNode(QPainter& painter, const TreemapNode* node);
};

#endif // CORE_GFX_QTREEMAP_H
