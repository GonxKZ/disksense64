#include "treemap.h"
#include <d2d1.h>
#include <dwrite.h>
#include <algorithm>
#include <cmath>
#include <stack>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// TreemapLayout implementation
std::unique_ptr<TreemapNode> TreemapLayout::createTreemap(const std::vector<FileEntry>& files, 
                                                         const Rect& bounds) {
    // Create root node
    auto root = std::make_unique<TreemapNode>();
    root->bounds = bounds;
    root->isDirectory = true;
    
    // Group files by directory
    std::map<std::wstring, std::vector<FileEntry>> directoryMap;
    for (const auto& file : files) {
        // Extract directory path (simplified)
        std::wstring dirPath = L"Root"; // In reality, we would extract the actual directory
        directoryMap[dirPath].push_back(file);
    }
    
    // Create child nodes for each directory
    for (const auto& [dirPath, dirFiles] : directoryMap) {
        auto dirNode = std::make_unique<TreemapNode>();
        dirNode->isDirectory = true;
        
        // Calculate total size
        uint64_t totalSize = 0;
        for (const auto& file : dirFiles) {
            totalSize += file.sizeLogical;
        }
        dirNode->totalSize = totalSize;
        
        // Create leaf nodes for files
        for (const auto& file : dirFiles) {
            auto fileNode = std::make_unique<TreemapNode>();
            fileNode->fileEntry = file;
            fileNode->totalSize = file.sizeLogical;
            fileNode->isDirectory = false;
            dirNode->children.push_back(std::move(fileNode));
        }
        
        root->children.push_back(std::move(dirNode));
    }
    
    // Apply layout
    squarifyLayout(*root, bounds);
    
    return root;
}

void TreemapLayout::squarifyLayout(TreemapNode& node, const Rect& bounds) {
    node.bounds = bounds;
    
    // If this is a leaf node, nothing to layout
    if (node.isLeaf()) {
        return;
    }
    
    // Sort children by size (descending)
    std::sort(node.children.begin(), node.children.end(),
              [](const std::unique_ptr<TreemapNode>& a, const std::unique_ptr<TreemapNode>& b) {
                  return a->totalSize > b->totalSize;
              });
    
    // Apply squarified layout algorithm
    std::vector<TreemapNode*> row;
    double rowSize = 0;
    double totalSize = static_cast<double>(node.totalSize);
    
    Rect currentBounds = bounds;
    
    for (auto& child : node.children) {
        double childSize = static_cast<double>(child->totalSize);
        double ratio = childSize / totalSize;
        double area = ratio * (bounds.width * bounds.height);
        double side = sqrt(area);
        
        row.push_back(child.get());
        rowSize += childSize;
        
        // Check if adding this child worsens the aspect ratio
        if (row.size() > 1) {
            // Simplified approach: layout when we have enough items
            if (row.size() >= 5 || &child == &node.children.back()) {
                // Layout the current row
                bool horizontal = currentBounds.width > currentBounds.height;
                layoutRow(row, currentBounds, horizontal);
                
                // Update bounds for next row
                if (horizontal) {
                    float usedHeight = 0;
                    for (const auto& item : row) {
                        usedHeight = std::max(usedHeight, item->bounds.height);
                    }
                    currentBounds.y += usedHeight;
                    currentBounds.height -= usedHeight;
                } else {
                    float usedWidth = 0;
                    for (const auto& item : row) {
                        usedWidth = std::max(usedWidth, item->bounds.width);
                    }
                    currentBounds.x += usedWidth;
                    currentBounds.width -= usedWidth;
                }
                
                // Clear row for next iteration
                row.clear();
                rowSize = 0;
            }
        } else {
            // First item in row
            row.push_back(child.get());
            rowSize += childSize;
        }
    }
    
    // Layout any remaining items
    if (!row.empty()) {
        bool horizontal = currentBounds.width > currentBounds.height;
        layoutRow(row, currentBounds, horizontal);
    }
}

double TreemapLayout::worstAspectRatio(const std::vector<double>& row, double width) {
    if (row.empty()) return 1.0;
    
    double sum = 0;
    double minVal = row[0];
    double maxVal = row[0];
    
    for (double val : row) {
        sum += val;
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    
    if (sum == 0) return 1.0;
    
    double widthSq = width * width;
    double sumSq = sum * sum;
    
    double ratio1 = (widthSq * maxVal) / sumSq;
    double ratio2 = sumSq / (widthSq * minVal);
    
    return std::max(ratio1, ratio2);
}

void TreemapLayout::layoutRow(std::vector<TreemapNode*>& row, const Rect& bounds, bool horizontal) {
    if (row.empty()) return;
    
    double totalSize = 0;
    for (const auto& node : row) {
        totalSize += node->totalSize;
    }
    
    if (totalSize == 0) return;
    
    if (horizontal) {
        // Layout horizontally
        float x = bounds.x;
        float height = static_cast<float>((bounds.width * totalSize) / (bounds.width * bounds.height) * bounds.height);
        
        for (auto& node : row) {
            float width = static_cast<float>((node->totalSize / totalSize) * bounds.width);
            node->bounds = Rect(x, bounds.y, width, height);
            x += width;
            
            // Recursively layout children
            if (!node->isLeaf()) {
                squarifyLayout(*node, node->bounds);
            }
        }
    } else {
        // Layout vertically
        float y = bounds.y;
        float width = static_cast<float>((bounds.height * totalSize) / (bounds.width * bounds.height) * bounds.width);
        
        for (auto& node : row) {
            float height = static_cast<float>((node->totalSize / totalSize) * bounds.height);
            node->bounds = Rect(bounds.x, y, width, height);
            y += height;
            
            // Recursively layout children
            if (!node->isLeaf()) {
                squarifyLayout(*node, node->bounds);
            }
        }
    }
}

TreemapNode* TreemapLayout::hitTest(TreemapNode& node, float x, float y) {
    // Check if point is within this node
    if (!node.bounds.contains(x, y)) {
        return nullptr;
    }
    
    // Check children first (depth-first)
    for (auto& child : node.children) {
        TreemapNode* hit = hitTest(*child, x, y);
        if (hit) {
            return hit;
        }
    }
    
    // If no children hit, this node is the hit
    return &node;
}

std::vector<TreemapNode*> TreemapLayout::getPathToNode(TreemapNode& root, TreemapNode* target) {
    std::vector<TreemapNode*> path;
    
    if (!target) {
        return path;
    }
    
    // Use BFS to find path
    std::stack<std::pair<TreemapNode*, std::vector<TreemapNode*>>> stack;
    std::vector<TreemapNode*> rootPath = {&root};
    stack.push({&root, rootPath});
    
    while (!stack.empty()) {
        auto [current, currentPath] = stack.top();
        stack.pop();
        
        if (current == target) {
            return currentPath;
        }
        
        for (auto& child : current->children) {
            auto childPath = currentPath;
            childPath.push_back(child.get());
            stack.push({child.get(), childPath});
        }
    }
    
    return path;
}

// TreemapRenderer implementation
TreemapRenderer::TreemapRenderer() 
    : m_hwnd(nullptr), m_pD2DFactory(nullptr), m_pRenderTarget(nullptr), 
      m_pBrush(nullptr), m_pDWriteFactory(nullptr), m_pTextFormat(nullptr) {
}

TreemapRenderer::~TreemapRenderer() {
    cleanup();
}

bool TreemapRenderer::initialize(HWND hwnd) {
    m_hwnd = hwnd;
    
    // Create D2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) {
        return false;
    }
    
    // Create DirectWrite factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    if (FAILED(hr)) {
        return false;
    }
    
    // Create text format
    hr = m_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-US",
        &m_pTextFormat
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create render target
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    
    hr = m_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &m_pRenderTarget
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create brush
    hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pBrush);
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

void TreemapRenderer::render(TreemapNode& root, const Rect& viewport) {
    if (!m_pRenderTarget) {
        return;
    }
    
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    
    // Render the treemap
    renderNode(root, viewport);
    
    m_pRenderTarget->EndDraw();
}

void TreemapRenderer::resize(UINT width, UINT height) {
    if (m_pRenderTarget) {
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void TreemapRenderer::cleanup() {
    if (m_pBrush) {
        m_pBrush->Release();
        m_pBrush = nullptr;
    }
    
    if (m_pTextFormat) {
        m_pTextFormat->Release();
        m_pTextFormat = nullptr;
    }
    
    if (m_pDWriteFactory) {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }
    
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }
    
    if (m_pD2DFactory) {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
}

Rect TreemapRenderer::screenToTreemap(const Rect& screenRect, const Rect& viewport, const Rect& treemapBounds) {
    // Convert screen coordinates to treemap coordinates
    float scaleX = treemapBounds.width / viewport.width;
    float scaleY = treemapBounds.height / viewport.height;
    
    return Rect(
        (screenRect.x - viewport.x) * scaleX + treemapBounds.x,
        (screenRect.y - viewport.y) * scaleY + treemapBounds.y,
        screenRect.width * scaleX,
        screenRect.height * scaleY
    );
}

D2D1_COLOR_F TreemapRenderer::getColorForNode(const TreemapNode& node) {
    if (node.isDirectory) {
        return D2D1::ColorF(D2D1::ColorF::Blue);
    }
    
    // Color based on file extension
    return getColorForExtension(L""); // Simplified
}

D2D1_COLOR_F TreemapRenderer::getColorForExtension(const std::wstring& extension) {
    // Simple color mapping based on extension
    if (extension == L".exe" || extension == L".dll") {
        return D2D1::ColorF(D2D1::ColorF::Red);
    } else if (extension == L".jpg" || extension == L".png" || extension == L".gif") {
        return D2D1::ColorF(D2D1::ColorF::Green);
    } else if (extension == L".mp3" || extension == L".wav" || extension == L".flac") {
        return D2D1::ColorF(D2D1::ColorF::Yellow);
    } else if (extension == L".txt" || extension == L".doc" || extension == L".pdf") {
        return D2D1::ColorF(D2D1::ColorF::Purple);
    } else {
        return D2D1::ColorF(D2D1::ColorF::Gray);
    }
}

D2D1_COLOR_F TreemapRenderer::getColorForSize(uint64_t size) {
    // Color based on file size
    if (size < 1024) { // < 1KB
        return D2D1::ColorF(D2D1::ColorF::LightBlue);
    } else if (size < 1024 * 1024) { // < 1MB
        return D2D1::ColorF(D2D1::ColorF::Blue);
    } else if (size < 10 * 1024 * 1024) { // < 10MB
        return D2D1::ColorF(D2D1::ColorF::Green);
    } else if (size < 100 * 1024 * 1024) { // < 100MB
        return D2D1::ColorF(D2D1::ColorF::Yellow);
    } else if (size < 1024 * 1024 * 1024) { // < 1GB
        return D2D1::ColorF(D2D1::ColorF::Orange);
    } else { // >= 1GB
        return D2D1::ColorF(D2D1::ColorF::Red);
    }
}

void TreemapRenderer::renderNode(TreemapNode& node, const Rect& viewport) {
    if (!m_pRenderTarget || !m_pBrush) {
        return;
    }
    
    // Check if node is visible in viewport
    if (node.bounds.x + node.bounds.width < viewport.x || 
        node.bounds.x > viewport.x + viewport.width ||
        node.bounds.y + node.bounds.height < viewport.y || 
        node.bounds.y > viewport.y + viewport.height) {
        return;
    }
    
    // Set color based on node properties
    D2D1_COLOR_F color = getColorForNode(node);
    m_pBrush->SetColor(color);
    
    // Draw rectangle
    D2D1_RECT_F rect = D2D1::RectF(
        node.bounds.x, node.bounds.y,
        node.bounds.x + node.bounds.width,
        node.bounds.y + node.bounds.height
    );
    
    m_pRenderTarget->FillRectangle(&rect, m_pBrush);
    
    // Draw border
    m_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    m_pRenderTarget->DrawRectangle(&rect, m_pBrush, 1.0f);
    
    // Render text if node is large enough
    if (node.bounds.width > 20 && node.bounds.height > 20) {
        std::wstring text = L"Node"; // Simplified
        renderText(text, node.bounds);
    }
    
    // Render children
    for (auto& child : node.children) {
        renderNode(*child, viewport);
    }
}

void TreemapRenderer::renderText(const std::wstring& text, const Rect& bounds) {
    if (!m_pRenderTarget || !m_pBrush || !m_pTextFormat) {
        return;
    }
    
    // Create layout rect
    D2D1_RECT_F layoutRect = D2D1::RectF(
        bounds.x, bounds.y,
        bounds.x + bounds.width,
        bounds.y + bounds.height
    );
    
    // Set text color
    m_pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    
    // Draw text
    m_pRenderTarget->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_pTextFormat,
        layoutRect,
        m_pBrush
    );
}

void TreemapRenderer::renderBorder(const Rect& bounds, const D2D1_COLOR_F& color, float strokeWidth) {
    if (!m_pRenderTarget || !m_pBrush) {
        return;
    }
    
    D2D1_RECT_F rect = D2D1::RectF(
        bounds.x, bounds.y,
        bounds.x + bounds.width,
        bounds.y + bounds.height
    );
    
    m_pBrush->SetColor(color);
    m_pRenderTarget->DrawRectangle(&rect, m_pBrush, strokeWidth);
}