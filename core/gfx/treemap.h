#ifndef CORE_GFX_TREEMAP_H
#define CORE_GFX_TREEMAP_H

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "core/model/model.h"

// Rectangle structure
struct Rect {
    float x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

// Treemap node
struct TreemapNode {
    FileEntry fileEntry;
    Rect bounds;
    std::vector<std::unique_ptr<TreemapNode>> children;
    bool isDirectory;
    uint64_t totalSize; // For directories, includes children
    
    TreemapNode() : isDirectory(false), totalSize(0) {}
    
    bool isLeaf() const { return children.empty(); }
};

// Treemap layout algorithm
class TreemapLayout {
public:
    // Create treemap from file entries
    static std::unique_ptr<TreemapNode> createTreemap(const std::vector<FileEntry>& files, 
                                                     const Rect& bounds);
    
    // Layout using squarified algorithm
    static void squarifyLayout(TreemapNode& node, const Rect& bounds);
    
    // Find node at position
    static TreemapNode* hitTest(TreemapNode& node, float x, float y);
    
    // Get path from root to node
    static std::vector<TreemapNode*> getPathToNode(TreemapNode& root, TreemapNode* target);
    
private:
    // Helper functions for squarified layout
    static double worstAspectRatio(const std::vector<double>& row, double width);
    static void layoutRow(std::vector<TreemapNode*>& row, const Rect& bounds, bool horizontal);
};

// Treemap renderer
class TreemapRenderer {
public:
    TreemapRenderer();
    ~TreemapRenderer();
    
    // Initialize Direct2D resources
    bool initialize(HWND hwnd);
    
    // Render the treemap
    void render(TreemapNode& root, const Rect& viewport);
    
    // Resize the render target
    void resize(UINT width, UINT height);
    
    // Cleanup resources
    void cleanup();
    
    // Convert screen coordinates to treemap coordinates
    Rect screenToTreemap(const Rect& screenRect, const Rect& viewport, const Rect& treemapBounds);
    
private:
    HWND m_hwnd;
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pBrush;
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pTextFormat;
    
    // Color mapping functions
    D2D1_COLOR_F getColorForNode(const TreemapNode& node);
    D2D1_COLOR_F getColorForExtension(const std::wstring& extension);
    D2D1_COLOR_F getColorForSize(uint64_t size);
    
    // Rendering helpers
    void renderNode(TreemapNode& node, const Rect& viewport);
    void renderText(const std::wstring& text, const Rect& bounds);
    void renderBorder(const Rect& bounds, const D2D1_COLOR_F& color, float strokeWidth);
};

#endif // CORE_GFX_TREEMAP_H