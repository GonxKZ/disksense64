#include "visualizationwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QTimerEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include "core/analysis/analysisutils.h"

// VisualizationWidget implementation
VisualizationWidget::VisualizationWidget(QWidget *parent)
    : QWidget(parent)
    , m_typeCombo(nullptr)
    , m_colorSchemeCombo(nullptr)
    , m_sizeMetricCombo(nullptr)
    , m_animationCheck(nullptr)
    , m_refreshButton(nullptr)
    , m_zoomInButton(nullptr)
    , m_zoomOutButton(nullptr)
    , m_resetButton(nullptr)
    , m_visualizationTabs(nullptr)
    , m_treemap2D(nullptr)
    , m_treemap3D(nullptr)
    , m_pieChart(nullptr)
    , m_barChart(nullptr)
    , m_lineChart(nullptr)
    , m_animationTimer(nullptr)
    , m_isAnimating(false)
    , m_currentType(TwoDTreemap)
{
    setupUI();
    setupConnections();
    
    // Set up animation timer
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &VisualizationWidget::updateAnimation);
    
    // Set initial visualization
    setVisualizationType(TwoDTreemap);
}

VisualizationWidget::~VisualizationWidget() {
    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
}

void VisualizationWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Controls
    QGroupBox* controlsGroup = new QGroupBox("Visualization Controls");
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsGroup);
    
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem("2D Treemap");
    m_typeCombo->addItem("3D Treemap");
    m_typeCombo->addItem("Pie Chart");
    m_typeCombo->addItem("Bar Chart");
    m_typeCombo->addItem("Line Chart");
    
    m_colorSchemeCombo = new QComboBox();
    m_colorSchemeCombo->addItem("Default");
    m_colorSchemeCombo->addItem("Rainbow");
    m_colorSchemeCombo->addItem("Heatmap");
    m_colorSchemeCombo->addItem("Monochrome");
    
    m_sizeMetricCombo = new QComboBox();
    m_sizeMetricCombo->addItem("File Size");
    m_sizeMetricCombo->addItem("File Count");
    m_sizeMetricCombo->addItem("Modification Date");
    
    m_animationCheck = new QCheckBox("Animate");
    
    m_refreshButton = new QPushButton("Refresh");
    m_zoomInButton = new QPushButton("Zoom In");
    m_zoomOutButton = new QPushButton("Zoom Out");
    m_resetButton = new QPushButton("Reset View");
    
    controlsLayout->addWidget(new QLabel("Type:"));
    controlsLayout->addWidget(m_typeCombo);
    controlsLayout->addWidget(new QLabel("Color:"));
    controlsLayout->addWidget(m_colorSchemeCombo);
    controlsLayout->addWidget(new QLabel("Size:"));
    controlsLayout->addWidget(m_sizeMetricCombo);
    controlsLayout->addWidget(m_animationCheck);
    controlsLayout->addWidget(m_refreshButton);
    controlsLayout->addWidget(m_zoomInButton);
    controlsLayout->addWidget(m_zoomOutButton);
    controlsLayout->addWidget(m_resetButton);
    controlsLayout->addStretch();
    
    // Visualization tabs
    m_visualizationTabs = new QTabWidget();
    
    m_treemap2D = new TreemapWidget2D();
    m_treemap3D = new TreemapWidget3D();
    m_pieChart = new ChartWidget(ChartFactory::Pie);
    m_barChart = new ChartWidget(ChartFactory::Bar);
    m_lineChart = new ChartWidget(ChartFactory::Line);
    
    m_visualizationTabs->addTab(m_treemap2D, "2D Treemap");
    m_visualizationTabs->addTab(m_treemap3D, "3D Treemap");
    m_visualizationTabs->addTab(m_pieChart, "Pie Chart");
    m_visualizationTabs->addTab(m_barChart, "Bar Chart");
    m_visualizationTabs->addTab(m_lineChart, "Line Chart");
    
    // Add to main layout
    mainLayout->addWidget(controlsGroup);
    mainLayout->addWidget(m_visualizationTabs);
    
    // Set initial state
    m_zoomInButton->setEnabled(false);
    m_zoomOutButton->setEnabled(false);
}

void VisualizationWidget::setupConnections() {
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VisualizationWidget::onVisualizationTypeChanged);
    connect(m_colorSchemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VisualizationWidget::onColorSchemeChanged);
    connect(m_sizeMetricCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VisualizationWidget::onSizeMetricChanged);
    connect(m_animationCheck, &QCheckBox::toggled,
            this, &VisualizationWidget::toggleAnimation);
    
    connect(m_refreshButton, &QPushButton::clicked, this, &VisualizationWidget::refresh);
    connect(m_zoomInButton, &QPushButton::clicked, this, &VisualizationWidget::zoomIn);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &VisualizationWidget::zoomOut);
    connect(m_resetButton, &QPushButton::clicked, this, &VisualizationWidget::resetView);
    
    // Connect visualization signals
    connect(m_treemap2D, &TreemapWidget2D::nodeSelected,
            this, &VisualizationWidget::nodeSelected);
    connect(m_treemap2D, &TreemapWidget2D::nodeDoubleClicked,
            this, &VisualizationWidget::nodeDoubleClicked);
    connect(m_treemap3D, &TreemapWidget3D::nodeSelected,
            this, &VisualizationWidget::nodeSelected);
    connect(m_treemap3D, &TreemapWidget3D::nodeDoubleClicked,
            this, &VisualizationWidget::nodeDoubleClicked);
}

void VisualizationWidget::setData(const QList<QFileInfo>& files) {
    m_files = files;
    updateVisualization();
}

void VisualizationWidget::clearData() {
    m_files.clear();
    m_treemap2D->clear();
    m_treemap3D->clear();
    m_pieChart->clear();
    m_barChart->clear();
    m_lineChart->clear();
}

VisualizationWidget::VisualizationType VisualizationWidget::currentVisualization() const {
    return m_currentType;
}

void VisualizationWidget::setVisualizationType(VisualizationType type) {
    m_currentType = type;
    m_typeCombo->setCurrentIndex(static_cast<int>(type));
    m_visualizationTabs->setCurrentIndex(static_cast<int>(type));
    emit visualizationChanged(type);
}

bool VisualizationWidget::isAnimating() const {
    return m_isAnimating;
}

void VisualizationWidget::startAnimation() {
    if (!m_isAnimating) {
        m_isAnimating = true;
        m_animationTimer->start(16); // ~60 FPS
        m_treemap3D->startAnimation();
    }
}

void VisualizationWidget::stopAnimation() {
    if (m_isAnimating) {
        m_isAnimating = false;
        m_animationTimer->stop();
        m_treemap3D->stopAnimation();
    }
}

bool VisualizationWidget::saveAsImage(const QString& fileName, const QSize& size) {
    Q_UNUSED(fileName);
    Q_UNUSED(size);
    // In a real implementation, this would render the current visualization to an image
    return true;
}

void VisualizationWidget::refresh() {
    updateVisualization();
}

void VisualizationWidget::zoomIn() {
    switch (m_currentType) {
        case TwoDTreemap:
            m_treemap2D->zoomIn();
            break;
        case ThreeDTreemap:
            m_treemap3D->zoomIn();
            break;
        default:
            // Other visualizations don't support zoom
            break;
    }
}

void VisualizationWidget::zoomOut() {
    switch (m_currentType) {
        case TwoDTreemap:
            m_treemap2D->zoomOut();
            break;
        case ThreeDTreemap:
            m_treemap3D->zoomOut();
            break;
        default:
            // Other visualizations don't support zoom
            break;
    }
}

void VisualizationWidget::resetView() {
    switch (m_currentType) {
        case TwoDTreemap:
            m_treemap2D->resetView();
            break;
        case ThreeDTreemap:
            m_treemap3D->resetView();
            break;
        default:
            // Other visualizations don't support view reset
            break;
    }
}

void VisualizationWidget::toggleAnimation(bool enabled) {
    if (enabled) {
        startAnimation();
    } else {
        stopAnimation();
    }
}

void VisualizationWidget::onVisualizationTypeChanged(int index) {
    setVisualizationType(static_cast<VisualizationType>(index));
}

void VisualizationWidget::onColorSchemeChanged(int index) {
    Q_UNUSED(index);
    // Update color scheme for all visualizations
    m_treemap2D->setColorScheme(index);
    // Other visualizations would also update their color schemes
}

void VisualizationWidget::onSizeMetricChanged(int index) {
    Q_UNUSED(index);
    // Update size metric for all visualizations
    m_treemap2D->setSizeMetric(index);
    // Other visualizations would also update their size metrics
}

void VisualizationWidget::updateAnimation() {
    // Update any animation-related state
    if (m_currentType == ThreeDTreemap) {
        // The 3D widget handles its own animation updates
    }
}

void VisualizationWidget::updateVisualization() {
    if (m_files.isEmpty()) {
        return;
    }
    
    // Create treemap data
    std::unique_ptr<TreemapNode> treemap2D = createTreemap(m_files);
    if (treemap2D) {
        m_treemap2D->setTreemap(std::move(treemap2D));
    }
    
    // Create chart data using analysis utilities
    AnalysisUtils analyzer;
    auto fileTypeDistribution = analyzer.analyzeFileTypeDistribution(m_files);
    
    ChartData pieData;
    for (const auto& dist : fileTypeDistribution) {
        pieData.addDataPoint(dist.fileType, static_cast<double>(dist.totalSize));
    }
    m_pieChart->setChartData(pieData);
    
    ChartData barData;
    for (const auto& dist : fileTypeDistribution) {
        barData.addDataPoint(dist.fileType, static_cast<double>(dist.count));
    }
    m_barChart->setChartData(barData);
    
    // For line chart, we'll use file age distribution
    auto fileAgeDistribution = analyzer.analyzeFileAgeDistribution(m_files);
    ChartData lineData;
    for (const auto& dist : fileAgeDistribution) {
        lineData.addDataPoint(QString("%1 days").arg(dist.daysOld), static_cast<double>(dist.totalSize));
    }
    m_lineChart->setChartData(lineData);
}

std::unique_ptr<TreemapNode> VisualizationWidget::createTreemap(const QList<QFileInfo>& files) {
    // Convert files to the format expected by TreemapLayout
    std::vector<FileEntry> fileEntries;
    for (const QFileInfo& fileInfo : files) {
        if (fileInfo.isFile()) {
            FileEntry entry;
            entry.path = fileInfo.absoluteFilePath().toStdString();
            entry.size = fileInfo.size();
            entry.modified = fileInfo.lastModified().toSecsSinceEpoch();
            fileEntries.push_back(entry);
        }
    }
    
    if (fileEntries.empty()) {
        return nullptr;
    }
    
    // Create bounds for the treemap
    Rect bounds(0, 0, 800, 600);
    return TreemapLayout::createTreemap(fileEntries, bounds);
}

// TreemapWidget2D implementation
TreemapWidget2D::TreemapWidget2D(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_colorScheme(0)
    , m_sizeMetric(0)
    , m_zoomLevel(1.0)
    , m_panOffset(0, 0)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

TreemapWidget2D::~TreemapWidget2D() {
}

void TreemapWidget2D::setTreemap(std::unique_ptr<TreemapNode> treemap) {
    m_root = std::move(treemap);
    update();
}

void TreemapWidget2D::clear() {
    m_root.reset();
    m_scene->clear();
}

void TreemapWidget2D::zoomIn() {
    scale(1.2, 1.2);
    m_zoomLevel *= 1.2;
}

void TreemapWidget2D::zoomOut() {
    scale(1/1.2, 1/1.2);
    m_zoomLevel /= 1.2;
}

void TreemapWidget2D::resetView() {
    resetMatrix();
    m_zoomLevel = 1.0;
    m_panOffset = QPointF(0, 0);
}

void TreemapWidget2D::setColorScheme(int scheme) {
    m_colorScheme = scheme;
    update();
}

void TreemapWidget2D::setSizeMetric(int metric) {
    m_sizeMetric = metric;
    update();
}

void TreemapWidget2D::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    // Fit the scene to the view
    if (m_root) {
        QRectF sceneRect(0, 0, m_root->rect.width, m_root->rect.height);
        m_scene->setSceneRect(sceneRect);
        fitInView(sceneRect, Qt::KeepAspectRatio);
    }
}

void TreemapWidget2D::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_root) {
        QPointF scenePos = mapToScene(event->pos());
        TreemapNode* node = hitTest(m_root.get(), scenePos);
        if (node) {
            emit nodeSelected(QString::fromStdString(node->path));
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void TreemapWidget2D::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_root) {
        QPointF scenePos = mapToScene(event->pos());
        TreemapNode* node = hitTest(m_root.get(), scenePos);
        if (node) {
            emit nodeDoubleClicked(QString::fromStdString(node->path));
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void TreemapWidget2D::wheelEvent(QWheelEvent *event) {
    // Zoom in/out with mouse wheel
    double scaleFactor = pow(1.2, event->angleDelta().y() / 120.0);
    scale(scaleFactor, scaleFactor);
    m_zoomLevel *= scaleFactor;
    event->accept();
}

void TreemapWidget2D::drawTreemap(QPainter* painter, const TreemapNode* node, const QRectF& bounds) {
    if (!node) {
        return;
    }
    
    // Draw this node
    QColor color = QColor::fromHsv((node->size % 360), 200, 220);
    painter->setBrush(color);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRect(bounds);
    
    // Draw label if there's enough space
    if (bounds.width() > 20 && bounds.height() > 20) {
        QString label = QString::fromStdString(node->name);
        if (bounds.width() > 50 && bounds.height() > 20) {
            label += QString("\n%1").arg(node->size);
        }
        
        painter->setPen(Qt::black);
        painter->drawText(bounds, Qt::AlignCenter | Qt::TextWordWrap, label);
    }
    
    // Draw children
    for (const auto& child : node->children) {
        if (child) {
            // Calculate child bounds
            QRectF childBounds(
                bounds.x() + (child->rect.x - node->rect.x) * bounds.width() / node->rect.width,
                bounds.y() + (child->rect.y - node->rect.y) * bounds.height() / node->rect.height,
                child->rect.width * bounds.width() / node->rect.width,
                child->rect.height * bounds.height() / node->rect.height
            );
            
            drawTreemap(painter, child.get(), childBounds);
        }
    }
}

QRectF TreemapWidget2D::nodeRectToViewRect(const Rect& nodeRect) const {
    return QRectF(nodeRect.x, nodeRect.y, nodeRect.width, nodeRect.height);
}

TreemapNode* TreemapWidget2D::hitTest(const TreemapNode* node, const QPointF& pos) const {
    if (!node) {
        return nullptr;
    }
    
    QRectF nodeRect(node->rect.x, node->rect.y, node->rect.width, node->rect.height);
    if (!nodeRect.contains(pos)) {
        return nullptr;
    }
    
    // Check children first (depth-first)
    for (const auto& child : node->children) {
        TreemapNode* hit = hitTest(child.get(), pos);
        if (hit) {
            return hit;
        }
    }
    
    // If no children hit, this node is the hit
    return const_cast<TreemapNode*>(node);
}

// TreemapWidget3D implementation
TreemapWidget3D::TreemapWidget3D(QWidget *parent)
    : QWidget(parent)
    , m_renderer(std::make_unique<Treemap3DRenderer>())
    , m_camera(std::make_unique<VisualizationCamera>())
    , m_mousePressed(false)
    , m_isAnimating(false)
    , m_animationTimer(new QTimer(this))
{
    setMinimumSize(400, 300);
    
    // Set up animation timer
    connect(m_animationTimer, &QTimer::timeout, this, &TreemapWidget3D::updateAnimation);
    
    // Initialize renderer
    initializeRenderer();
}

TreemapWidget3D::~TreemapWidget3D() {
    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
}

void TreemapWidget3D::setTreemap(std::unique_ptr<Treemap3DNode> treemap) {
    m_root = std::move(treemap);
    update();
}

void TreemapWidget3D::clear() {
    m_root.reset();
    update();
}

void TreemapWidget3D::zoomIn() {
    if (m_camera) {
        m_camera->zoom(0.5f);
        update();
    }
}

void TreemapWidget3D::zoomOut() {
    if (m_camera) {
        m_camera->zoom(-0.5f);
        update();
    }
}

void TreemapWidget3D::resetView() {
    if (m_camera) {
        m_camera->reset();
        update();
    }
}

void TreemapWidget3D::startAnimation() {
    if (!m_isAnimating) {
        m_isAnimating = true;
        m_animationTimer->start(16); // ~60 FPS
    }
}

void TreemapWidget3D::stopAnimation() {
    if (m_isAnimating) {
        m_isAnimating = false;
        m_animationTimer->stop();
    }
}

bool TreemapWidget3D::isAnimating() const {
    return m_isAnimating;
}

void TreemapWidget3D::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // In a real implementation, this would use OpenGL to render the 3D treemap
    // For now, we'll just draw a placeholder
    painter.fillRect(rect(), Qt::lightGray);
    painter.setPen(Qt::darkGray);
    painter.drawText(rect(), Qt::AlignCenter, "3D Treemap Visualization\n(OpenGL rendering would appear here)");
    
    if (m_root) {
        // Render the treemap if we have data
        // m_renderer->render(static_cast<float>(width()) / height());
    }
}

void TreemapWidget3D::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // Update the renderer with the new size
}

void TreemapWidget3D::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        m_mousePressed = true;
    }
    QWidget::mousePressEvent(event);
}

void TreemapWidget3D::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void TreemapWidget3D::mouseMoveEvent(QMouseEvent *event) {
    if (m_mousePressed && m_camera) {
        QPoint delta = event->pos() - m_lastMousePos;
        
        // Rotate camera based on mouse movement
        if (m_camera) {
            m_camera->rotate(delta.x() * 0.5f, delta.y() * 0.5f);
            update();
        }
        
        m_lastMousePos = event->pos();
    }
    QWidget::mouseMoveEvent(event);
}

void TreemapWidget3D::wheelEvent(QWheelEvent *event) {
    if (m_camera) {
        m_camera->zoom(event->angleDelta().y() * 0.01f);
        update();
    }
    event->accept();
}

void TreemapWidget3D::initializeRenderer() {
    // Initialize OpenGL context and resources
    // In a real implementation, this would set up shaders, buffers, etc.
}

void TreemapWidget3D::updateAnimation() {
    // Update animation state
    if (m_isAnimating) {
        m_renderer->update(1.0f / 60.0f); // Assuming 60 FPS
        update();
    }
}

// ChartWidget implementation
ChartWidget::ChartWidget(ChartFactory::ChartType chartType, QWidget *parent)
    : QWidget(parent)
    , m_chart(ChartFactory::createChart(chartType, this))
    , m_chartType(chartType)
    , m_hoveredIndex(-1)
{
    setMinimumSize(400, 300);
}

ChartWidget::~ChartWidget() {
}

void ChartWidget::setChartData(const ChartData& data) {
    if (m_chart) {
        m_chart->setData(data);
        update();
    }
}

void ChartWidget::clear() {
    if (m_chart) {
        ChartData emptyData;
        m_chart->setData(emptyData);
        update();
    }
}

void ChartWidget::setChartType(ChartFactory::ChartType type) {
    if (m_chartType != type) {
        m_chartType = type;
        m_chart = ChartFactory::createChart(type, this);
        update();
    }
}

ChartFactory::ChartType ChartWidget::chartType() const {
    return m_chartType;
}

void ChartWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    if (m_chart) {
        renderChart(&painter);
    } else {
        // Draw placeholder
        painter.fillRect(rect(), Qt::lightGray);
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, 
                        QString("%1 Chart\n(Data would appear here)")
                        .arg(m_chartType == ChartFactory::Pie ? "Pie" :
                             m_chartType == ChartFactory::Bar ? "Bar" : "Line"));
    }
}

void ChartWidget::mousePressEvent(QMouseEvent *event) {
    if (m_chart && event->button() == Qt::LeftButton) {
        int dataIndex;
        if (m_chart->hitTest(event->pos(), dataIndex)) {
            emit qobject_cast<VisualizationWidget*>(parent())->nodeSelected(
                QString("Data point %1").arg(dataIndex));
        }
    }
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_chart) {
        int dataIndex;
        if (m_chart->hitTest(event->pos(), dataIndex)) {
            if (dataIndex != m_hoveredIndex) {
                m_hoveredIndex = dataIndex;
                update();
            }
        } else if (m_hoveredIndex != -1) {
            m_hoveredIndex = -1;
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ChartWidget::leaveEvent(QEvent *event) {
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void ChartWidget::renderChart(QPainter* painter) {
    // Set up chart bounds
    QRectF bounds = rect().adjusted(10, 10, -10, -10);
    m_chart->setBounds(bounds);
    
    // In a real implementation, this would use the chart's render method
    // For now, we'll just draw a placeholder based on the chart type
    
    painter->fillRect(bounds, Qt::white);
    painter->setPen(Qt::black);
    painter->drawRect(bounds);
    
    QString chartName;
    switch (m_chartType) {
        case ChartFactory::Pie:
            chartName = "Pie Chart";
            break;
        case ChartFactory::Bar:
            chartName = "Bar Chart";
            break;
        case ChartFactory::Line:
            chartName = "Line Chart";
            break;
    }
    
    painter->drawText(bounds, Qt::AlignCenter, chartName + "\n(Chart rendering would appear here)");
}

void ChartWidget::updateChart() {
    if (m_chart) {
        update();
    }
}