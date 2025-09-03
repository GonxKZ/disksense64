#include "qtreemap.h"
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QMessageBox>
#include "libs/utils/utils.h"

QTreemap::QTreemap(QWidget *parent) : QWidget(parent) {
}

QTreemap::~QTreemap() {
}

void QTreemap::setTreemap(std::unique_ptr<TreemapNode> root) {
    m_root = std::move(root);
    update(); // Request a repaint
}

void QTreemap::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_root) {
        // Scale the treemap to fit the widget
        if (!m_root->children.empty()) {
            // Update bounds to fit the current widget size
            m_root->bounds = Rect(0, 0, width(), height());
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

void QTreemap::drawNode(QPainter& painter, const TreemapNode* node) {
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
    
    // Only draw if the rectangle is visible
    if (node->bounds.width > 1 && node->bounds.height > 1) {
        painter.setBrush(QBrush(fillColor));
        painter.setPen(QPen(Qt::black, 1));
        
        // Draw rectangle
        QRect rect(node->bounds.x, node->bounds.y, node->bounds.width, node->bounds.height);
        painter.drawRect(rect);
        
        // Draw text (file name or directory name)
        if (node->bounds.width > 50 && node->bounds.height > 20) { // Only draw if enough space
            painter.setPen(QPen(Qt::white));
            QString name = QString::fromStdString(node->fileEntry.fullPath);
            // Extract just the filename, not the full path
            QFileInfo fileInfo(name);
            name = fileInfo.fileName();
            if (name.isEmpty()) {
                name = QString::fromStdString(node->fileEntry.fullPath);
            }
            painter.drawText(rect.adjusted(5, 5, -5, -5),
                           Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, 
                           name);
        }
    }

    // Recursively draw children
    for (const auto& child : node->children) {
        drawNode(painter, child.get());
    }
}