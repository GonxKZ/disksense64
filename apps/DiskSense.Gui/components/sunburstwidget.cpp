#include "sunburstwidget.h"
#include <QPainter>
#include <QDirIterator>

SunburstWidget::SunburstWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(240);
}

void SunburstWidget::setRootDirectory(const QString& dir) {
    m_root = Node(); m_root.name = dir; m_root.size = 0; m_root.children.clear();
    buildTree(dir, m_root, 0, 3);
    update();
}

void SunburstWidget::clear() {
    m_root = Node(); update();
}

void SunburstWidget::buildTree(const QString& dir, Node& out, int depth, int maxDepth) {
    if (depth > maxDepth) return;
    QDir d(dir);
    QFileInfoList entries = d.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot);
    QMap<QString, quint64> sizes;
    for (const QFileInfo& fi : entries) {
        if (fi.isDir()) {
            quint64 subSize = 0;
            QDirIterator it(fi.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) { it.next(); subSize += it.fileInfo().size(); }
            sizes[fi.fileName()] += subSize;
        } else {
            sizes["(archivos)"] += fi.size();
        }
    }
    for (auto it = sizes.constBegin(); it != sizes.constEnd(); ++it) {
        Node child; child.name = it.key(); child.size = it.value();
        if (depth+1 <= maxDepth && it.key() != "(archivos)") {
            buildTree(d.absoluteFilePath(it.key()), child, depth+1, maxDepth);
        }
        out.children.push_back(child);
        out.size += child.size;
    }
}

void SunburstWidget::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(15,17,19));
    if (m_root.name.isEmpty() || m_root.children.isEmpty()) {
        p.setPen(Qt::gray); p.drawText(rect(), Qt::AlignCenter, tr("Sin datos")); return;
    }
    QRectF bounds = rect().adjusted(10,10,-10,-10);
    double total = double(m_root.size) + 1.0;
    drawRing(p, bounds, m_root, 0, 3, 0, 360.0);
}

void SunburstWidget::drawRing(QPainter& p, const QRectF& rect, const Node& node, int depth, int maxDepth, double startAngle, double spanAngle) {
    if (depth > maxDepth || node.children.isEmpty()) return;
    double innerMargin = 8.0;
    double ringWidth = rect.width() < rect.height() ? rect.width()/ (2*(maxDepth+1)) : rect.height()/ (2*(maxDepth+1));
    ringWidth = qMax(14.0, ringWidth);
    double radius = qMin(rect.width(), rect.height())/2.0 - depth*ringWidth;
    QPointF center = rect.center();
    QRectF ringRect(center.x()-radius, center.y()-radius, radius*2, radius*2);
    double total = double(node.size) + 1.0;
    double angle = startAngle;
    int idx = 0; int n = node.children.size();
    for (const auto& ch : node.children) {
        double frac = (total>0.0)? (double(ch.size)/total) : 0.0;
        double span = spanAngle * frac;
        QColor col = QColor::fromHsv((idx*360)/qMax(1,n), 200, 220);
        p.setPen(Qt::NoPen); p.setBrush(col);
        p.drawPie(ringRect, int(angle*16), int(span*16));
        // recurse to inner ring
        drawRing(p, QRectF(rect.adjusted(innerMargin, innerMargin, -innerMargin, -innerMargin)), ch, depth+1, maxDepth, angle, span);
        angle += span; idx++;
    }
}

