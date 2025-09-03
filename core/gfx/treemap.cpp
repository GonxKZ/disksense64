#include "treemap.h"
#include <algorithm>
#include <numeric>

// Helper function to calculate aspect ratio
static double aspectRatio(double x, double y) {
    if (x == 0 || y == 0) return 0.0;
    return std::max(x / y, y / x);
}

// TreemapLayout implementation
std::unique_ptr<TreemapNode> TreemapLayout::createTreemap(const std::vector<FileEntry>& files,
                                                         const Rect& bounds) {
    auto root = std::make_unique<TreemapNode>();
    root->bounds = bounds;
    root->isDirectory = true;
    root->totalSize = 0;

    // Create child nodes for each file
    for (const auto& file : files) {
        auto child = std::make_unique<TreemapNode>();
        child->fileEntry = file;
        child->name = file.fullPath;
        child->isDirectory = file.attributes.directory;
        child->totalSize = file.sizeLogical;
        root->children.push_back(std::move(child));
        root->totalSize += file.sizeLogical;
    }

    // Sort children by size (descending)
    std::sort(root->children.begin(), root->children.end(),
              [](const std::unique_ptr<TreemapNode>& a, const std::unique_ptr<TreemapNode>& b) {
                  return a->totalSize > b->totalSize;
              });

    // Apply squarified layout
    squarifyLayout(*root, bounds);

    return root;
}

void TreemapLayout::squarifyLayout(TreemapNode& node, const Rect& bounds) {
    if (node.children.empty()) return;

    // Set the bounds for the current node
    node.bounds = bounds;

    // Filter out children with zero size to avoid division by zero
    std::vector<TreemapNode*> validChildren;
    for (const auto& child : node.children) {
        if (child->totalSize > 0) {
            validChildren.push_back(child.get());
        }
    }

    if (validChildren.empty()) return;

    // Calculate total size of valid children
    uint64_t totalChildrenSize = 0;
    for (const auto& child : validChildren) {
        totalChildrenSize += child->totalSize;
    }

    if (totalChildrenSize == 0) return; // Avoid division by zero if all children have zero size

    // Normalize sizes to the bounds area
    double scale = static_cast<double>(bounds.width * bounds.height) / totalChildrenSize;

    // Squarify algorithm
    std::vector<TreemapNode*> currentRow;
    double currentX = bounds.x;
    double currentY = bounds.y;
    double remainingWidth = bounds.width;
    double remainingHeight = bounds.height;

    for (size_t i = 0; i < validChildren.size(); ++i) {
        TreemapNode* child = validChildren[i];
        currentRow.push_back(child);

        // Calculate areas for current row
        double currentArea = 0;
        for (const auto& r : currentRow) {
            currentArea += r->totalSize * scale;
        }

        double worst = worstAspectRatio(currentRow, currentArea,
                                        (remainingWidth < remainingHeight) ? remainingWidth : remainingHeight);

        if (i + 1 < validChildren.size()) {
            // Check if adding the next child would improve the aspect ratio
            TreemapNode* nextChild = validChildren[i + 1];
            currentRow.push_back(nextChild);
            double nextArea = 0;
            for (const auto& r : currentRow) {
                nextArea += r->totalSize * scale;
            }
            double nextWorst = worstAspectRatio(currentRow, nextArea,
                                                (remainingWidth < remainingHeight) ? remainingWidth : remainingHeight);
            currentRow.pop_back(); // Remove the peeked child

            if (nextWorst < worst) {
                // Adding the next child improves the aspect ratio, continue adding
                continue;
            }
        }

        // Layout the current row
        if (remainingWidth < remainingHeight) {
            layoutRow(currentRow, Rect(currentX, currentY, remainingWidth, currentArea / remainingWidth), false);
            currentY += currentArea / remainingWidth;
            remainingHeight -= currentArea / remainingWidth;
        } else {
            layoutRow(currentRow, Rect(currentX, currentY, currentArea / remainingHeight, remainingHeight), true);
            currentX += currentArea / remainingHeight;
            remainingWidth -= currentArea / remainingHeight;
        }

        // Recursively layout children
        for (const auto& r : currentRow) {
            squarifyLayout(*r, r->bounds);
        }

        currentRow.clear();
    }
}

TreemapNode* TreemapLayout::hitTest(TreemapNode& node, float x, float y) {
    if (node.bounds.contains(x, y)) {
        for (const auto& child : node.children) {
            TreemapNode* hit = hitTest(*child, x, y);
            if (hit) return hit;
        }
        return &node;
    }
    return nullptr;
}

std::vector<TreemapNode*> TreemapLayout::getPathToNode(TreemapNode& root, TreemapNode* target) {
    std::vector<TreemapNode*> path;
    // Implement path finding (e.g., recursive search)
    return path;
}

double TreemapLayout::worstAspectRatio(const std::vector<TreemapNode*>& row, double totalArea, double width) {
    if (row.empty() || width == 0 || totalArea == 0) return 0.0;

    double minVal = std::numeric_limits<double>::max();
    double maxVal = 0.0;
    
    for (const auto& node : row) {
        double normalizedSize = node->totalSize / totalArea;
        if (normalizedSize < minVal) minVal = normalizedSize;
        if (normalizedSize > maxVal) maxVal = normalizedSize;
    }

    return std::max(width * maxVal, 1.0 / (width * minVal));
}

void TreemapLayout::layoutRow(std::vector<TreemapNode*>& row, const Rect& bounds, bool horizontal) {
    if (row.empty()) return;
    
    double totalRowSize = 0;
    for (const auto& node : row) {
        totalRowSize += node->totalSize;
    }

    if (totalRowSize == 0) return;

    double currentOffset = 0;
    for (const auto& node : row) {
        double nodeSizeRatio = static_cast<double>(node->totalSize) / totalRowSize;
        if (horizontal) {
            node->bounds = Rect(bounds.x + currentOffset, bounds.y, bounds.width * nodeSizeRatio, bounds.height);
            currentOffset += bounds.width * nodeSizeRatio;
        } else {
            node->bounds = Rect(bounds.x, bounds.y + currentOffset, bounds.width, bounds.height * nodeSizeRatio);
            currentOffset += bounds.height * nodeSizeRatio;
        }
    }
}
