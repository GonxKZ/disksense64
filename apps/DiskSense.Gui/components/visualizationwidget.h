#ifndef COMPONENTS_VISUALIZATIONWIDGET_H
#define COMPONENTS_VISUALIZATIONWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <memory>
#include "core/gfx/treemap.h"
#include "core/gfx/3dtreemap.h"
#include "core/gfx/charts.h"

class TreemapWidget2D;
class TreemapWidget3D;
class ChartWidget;

class VisualizationWidget : public QWidget {
    Q_OBJECT

public:
    enum VisualizationType {
        TwoDTreemap,
        ThreeDTreemap,
        PieChart,
        BarChart,
        LineChart
    };

public:
    explicit VisualizationWidget(QWidget *parent = nullptr);
    ~VisualizationWidget() override;

    // Data management
    void setData(const QList<QFileInfo>& files);
    void clearData();
    
    // Visualization control
    VisualizationType currentVisualization() const;
    void setVisualizationType(VisualizationType type);
    
    // Animation
    bool isAnimating() const;
    void startAnimation();
    void stopAnimation();
    
    // Export
    bool saveAsImage(const QString& fileName, const QSize& size = QSize(800, 600));

public slots:
    void refresh();
    void zoomIn();
    void zoomOut();
    void resetView();
    void toggleAnimation(bool enabled);

signals:
    void visualizationChanged(VisualizationType type);
    void nodeSelected(const QString& filePath);
    void nodeDoubleClicked(const QString& filePath);

private slots:
    void onVisualizationTypeChanged(int index);
    void onColorSchemeChanged(int index);
    void onSizeMetricChanged(int index);
    void updateAnimation();

private:
    void setupUI();
    void setupConnections();
    void updateVisualization();
    std::unique_ptr<TreemapNode> createTreemap(const QList<QFileInfo>& files);
    
    // UI components
    QComboBox* m_typeCombo;
    QComboBox* m_colorSchemeCombo;
    QComboBox* m_sizeMetricCombo;
    QCheckBox* m_animationCheck;
    QPushButton* m_refreshButton;
    QPushButton* m_zoomInButton;
    QPushButton* m_zoomOutButton;
    QPushButton* m_resetButton;
    
    QTabWidget* m_visualizationTabs;
    
    TreemapWidget2D* m_treemap2D;
    TreemapWidget3D* m_treemap3D;
    ChartWidget* m_pieChart;
    ChartWidget* m_barChart;
    ChartWidget* m_lineChart;
    
    // Data
    QList<QFileInfo> m_files;
    
    // Animation
    QTimer* m_animationTimer;
    bool m_isAnimating;
    
    // Current settings
    VisualizationType m_currentType;
};

// 2D Treemap widget
class TreemapWidget2D : public QGraphicsView {
    Q_OBJECT

public:
    explicit TreemapWidget2D(QWidget *parent = nullptr);
    ~TreemapWidget2D() override;

    void setTreemap(std::unique_ptr<TreemapNode> treemap);
    void clear();
    
    // Interaction
    void zoomIn();
    void zoomOut();
    void resetView();
    
    // Settings
    void setColorScheme(int scheme);
    void setSizeMetric(int metric);

signals:
    void nodeSelected(const QString& filePath);
    void nodeDoubleClicked(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void drawTreemap(QPainter* painter, const TreemapNode* node, const QRectF& bounds);
    QRectF nodeRectToViewRect(const Rect& nodeRect) const;
    TreemapNode* hitTest(const TreemapNode* node, const QPointF& pos) const;
    
    std::unique_ptr<TreemapNode> m_root;
    QGraphicsScene* m_scene;
    int m_colorScheme;
    int m_sizeMetric;
    double m_zoomLevel;
    QPointF m_panOffset;
};

// 3D Treemap widget
class TreemapWidget3D : public QWidget {
    Q_OBJECT

public:
    explicit TreemapWidget3D(QWidget *parent = nullptr);
    ~TreemapWidget3D() override;

    void setTreemap(std::unique_ptr<Treemap3DNode> treemap);
    void clear();
    
    // Interaction
    void zoomIn();
    void zoomOut();
    void resetView();
    
    // Animation
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const;

signals:
    void nodeSelected(const QString& filePath);
    void nodeDoubleClicked(const QString& filePath);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void initializeRenderer();
    void updateAnimation();
    
    std::unique_ptr<Treemap3DNode> m_root;
    std::unique_ptr<Treemap3DRenderer> m_renderer;
    std::unique_ptr<VisualizationCamera> m_camera;
    
    QPoint m_lastMousePos;
    bool m_mousePressed;
    bool m_isAnimating;
    QTimer* m_animationTimer;
};

// Chart widget
class ChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChartWidget(ChartFactory::ChartType chartType, QWidget *parent = nullptr);
    ~ChartWidget() override;

    void setChartData(const ChartData& data);
    void clear();
    
    // Chart settings
    void setChartType(ChartFactory::ChartType type);
    ChartFactory::ChartType chartType() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void renderChart(QPainter* painter);
    void updateChart();
    
    std::unique_ptr<Chart> m_chart;
    ChartFactory::ChartType m_chartType;
    int m_hoveredIndex;
};

#endif // COMPONENTS_VISUALIZATIONWIDGET_H