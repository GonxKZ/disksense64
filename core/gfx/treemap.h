#ifndef CORE_GFX_TREEMAP_H
#define CORE_GFX_TREEMAP_H

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <limits>
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
    std::string name;
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
    static double worstAspectRatio(const std::vector<TreemapNode*>& row, double totalArea, double width);
    static void layoutRow(std::vector<TreemapNode*>& row, const Rect& bounds, bool horizontal);
};

#endif // CORE_GFX_TREEMAP_H
