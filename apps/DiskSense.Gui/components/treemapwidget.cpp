#include "treemapwidget.h"
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QMouseEvent>
#include <QToolTip>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <cmath>
#include "libs/utils/utils.h"

TreemapWidget::TreemapWidget(QWidget *parent) 
    : QWidget(parent)
    , m_hoveredNode(nullptr)
    , m_zoomLevel(1.0)
    , m_isPanning(false)
{
    setMouseTracking(true);
}

TreemapWidget::~TreemapWidget() {
}

void TreemapWidget::setTreemap(std::unique_ptr<TreemapNode> root) {
    m_root = std::move(root);
    m_hoveredNode = nullptr;
    m_zoomLevel = 1.0;
    m_panOffset = QPoint(0, 0);
    update();
}

void TreemapWidget::clear() {
    m_root.reset();
    m_hoveredNode = nullptr;
    update();
}

void TreemapWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_root) {
        // Scale the treemap to fit the widget
        if (!m_root->children.empty()) {
            // Update bounds to fit the current widget size with zoom and pan
            updateZoomAndPan();
            drawNode(painter, m_root.get());
        } else {
            // Draw a message when there's no data
            painter.setPen(QPen(Qt::black));
            painter.drawText(rect(), Qt::AlignCenter, "No files found or scanning...");
        }
    } else {
        // Draw a welcome message
        painter.setPen(QPen(Qt::black));
        painter.drawText(rect(), Qt::AlignCenter, "Scanning directory...\nPlease wait...");
    }
}

void TreemapWidget::drawNode(QPainter& painter, const TreemapNode* node) {
    if (!node) return;

    // Set brush and pen based on node properties
    QColor fillColor = Qt::lightGray;
    if (node->isDirectory) {
        fillColor = Qt::darkGray;
    } else {
        QString extension = QString::fromStdString(FileUtils::get_file_extension(node->fileEntry.fullPath));
        if (extension == ".txt") {
            fillColor = Qt::blue;
        } else if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
            fillColor = Qt::red;
        } else if (extension == ".mp3" || extension == ".wav") {
            fillColor = Qt::green;
        } else {
            fillColor = Qt::lightGray;
        }
    }
    
    // Highlight hovered node
    if (node == m_hoveredNode) {
        fillColor = fillColor.lighter(150);
    }
    
    // Only draw if the rectangle is visible
    QRectF widgetRect = nodeRectToWidgetRect(node->bounds);
    if (widgetRect.width() > 1 && widgetRect.height() > 1) {
        painter.setBrush(QBrush(fillColor));
        painter.setPen(QPen(Qt::black, 1));
        
        // Draw rectangle
        painter.drawRect(widgetRect);
        
        // Draw text (file name or directory name)
        if (widgetRect.width() > 50 && widgetRect.height() > 20) { // Only draw if enough space
            painter.setPen(QPen(Qt::white));
            QString name = QString::fromStdString(node->name);
            if (name.isEmpty()) {
                name = QFileInfo(QString::fromStdString(node->fileEntry.fullPath)).fileName();
            }
            painter.drawText(widgetRect.adjusted(5, 5, -5, -5),
                           Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, 
                           name);
        }
    }

    // Recursively draw children
    for (const auto& child : node->children) {
        drawNode(painter, child.get());
    }
}

void TreemapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_root) {
            QPoint pos = event->pos();
            m_hoveredNode = hitTest(m_root.get(), pos);
            update();
            
            if (m_hoveredNode) {
                // Show tooltip with file information
                QString info = QString("Name: %1\nSize: %2\nPath: %3")
                    .arg(QString::fromStdString(m_hoveredNode->name))
                    .arg(formatFileSize(m_hoveredNode->totalSize))
                    .arg(QString::fromStdString(m_hoveredNode->fileEntry.fullPath));
                QToolTip::showText(event->globalPosition().toPoint(), info, this);
            }
        }
    } else if (event->button() == Qt::RightButton) {
        // Start panning
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    
    QWidget::mousePressEvent(event);
}

void TreemapWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_root) {
        QPoint pos = event->pos();
        TreemapNode* node = hitTest(m_root.get(), pos);
        if (node) {
            // Open file or directory in system handler
            QString path = QString::fromStdString(node->fileEntry.fullPath);
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TreemapWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        // Pan the view
        QPoint delta = event->pos() - m_lastMousePos;
        m_panOffset += delta;
        m_lastMousePos = event->pos();
        update();
    } else if (m_root) {
        // Update hover
        QPoint pos = event->pos();
        TreemapNode* newHovered = hitTest(m_root.get(), pos);
        if (newHovered != m_hoveredNode) {
            m_hoveredNode = newHovered;
            update();
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void TreemapWidget::resizeEvent(QResizeEvent *event) {
    updateZoomAndPan();
    QWidget::resizeEvent(event);
}

TreemapNode* TreemapWidget::hitTest(const TreemapNode* node, const QPoint& pos) const {
    if (!node) return nullptr;
    
    QRectF widgetRect = nodeRectToWidgetRect(node->bounds);
    if (widgetRect.contains(pos)) {
        // Check children first (depth-first)
        for (const auto& child : node->children) {
            TreemapNode* result = hitTest(child.get(), pos);
            if (result) return result;
        }
        // If no child was hit, this node was hit
        return const_cast<TreemapNode*>(node);
    }
    return nullptr;
}

void TreemapWidget::updateZoomAndPan() {
    if (m_root) {
        // Update the root bounds to fit the widget with current zoom and pan
        double scaledWidth = width() * m_zoomLevel;
        double scaledHeight = height() * m_zoomLevel;
        m_root->bounds = Rect(-m_panOffset.x(), -m_panOffset.y(), scaledWidth, scaledHeight);
    }
}

QRectF TreemapWidget::nodeRectToWidgetRect(const Rect& nodeRect) const {
    // Convert node coordinates to widget coordinates considering zoom and pan
    double invZoom = 1.0 / m_zoomLevel;
    return QRectF(
        (nodeRect.x + m_panOffset.x()) * invZoom,
        (nodeRect.y + m_panOffset.y()) * invZoom,
        nodeRect.width * invZoom,
        nodeRect.height * invZoom
    );
}

QString TreemapWidget::formatFileSize(uint64_t size) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double sizeDouble = static_cast<double>(size);
    
    while (sizeDouble >= 1024.0 && unitIndex < 4) {
        sizeDouble /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(sizeDouble, 0, 'f', 2).arg(units[unitIndex]);
}
