#include "ui_optimized.h"
#include <d2d1helper.h>
#include <dwrite.h>
#include <dcomp.h>
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cassert>

// UIElement implementation
bool UIOptimized::UIElement::hit_test(float x, float y) const {
    return x >= m_bounds.left && x <= m_bounds.right &&
           y >= m_bounds.top && y <= m_bounds.bottom;
}

// TreemapElement implementation
UIOptimized::TreemapElement::TreemapElement(const D2D1_RECT_F& bounds)
    : UIElement(ElementType::TREEMAP, bounds),
      m_layout_algorithm(LayoutAlgorithm::SQUARIFIED),
      m_interaction_mode(InteractionMode::NAVIGATE),
      m_view_offset(D2D1::Point2F(0, 0)),
      m_zoom_level(1.0f),
      m_history_index(0) {
}

void UIOptimized::TreemapElement::set_root(std::unique_ptr<TreeNode> root) {
    m_root = std::move(root);
    if (m_root) {
        layout_squarified(m_root.get(), m_bounds);
    }
}

void UIOptimized::TreemapElement::set_view(const D2D1_POINT_2F& offset, float zoom) {
    m_view_offset = offset;
    m_zoom_level = zoom;
    
    // Add to history
    if (m_history_index < m_view_history.size()) {
        m_view_history.resize(m_history_index);
    }
    m_view_history.push_back(D2D1::RectF(offset.x, offset.y, offset.x, offset.y));
    m_history_index = m_view_history.size();
}

void UIOptimized::TreemapElement::zoom_to_fit() {
    if (m_root) {
        m_view_offset = D2D1::Point2F(0, 0);
        m_zoom_level = 1.0f;
    }
}

void UIOptimized::TreemapElement::navigate_back() {
    if (m_history_index > 1) {
        m_history_index--;
        const auto& rect = m_view_history[m_history_index - 1];
        m_view_offset = D2D1::Point2F(rect.left, rect.top);
        m_zoom_level = 1.0f; // Simplified
    }
}

void UIOptimized::TreemapElement::navigate_forward() {
    if (m_history_index < m_view_history.size()) {
        m_history_index++;
        const auto& rect = m_view_history[m_history_index - 1];
        m_view_offset = D2D1::Point2F(rect.left, rect.top);
        m_zoom_level = 1.0f; // Simplified
    }
}

void UIOptimized::TreemapElement::render(ID2D1DeviceContext* context) {
    if (!context || !m_visible || !m_root) {
        return;
    }
    
    // Apply transformations
    D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();
    transform = D2D1::Matrix3x2F::Translation(m_view_offset.x, m_view_offset.y) * 
                D2D1::Matrix3x2F::Scale(m_zoom_level, m_zoom_level, 
                                       D2D1::Point2F(m_bounds.left, m_bounds.top));
    
    context->SetTransform(transform);
    
    // Render the treemap
    render_node(context, m_root.get(), m_bounds);
}

void UIOptimized::TreemapElement::on_mouse_move(float x, float y) {
    UIElement::on_mouse_move(x, y);
    
    switch (m_interaction_mode) {
        case InteractionMode::NAVIGATE:
            // Implement panning logic
            break;
        case InteractionMode::SELECT:
            // Implement selection highlighting
            break;
        case InteractionMode::DRAG:
            // Implement dragging logic
            break;
    }
}

void UIOptimized::TreemapElement::on_mouse_down(float x, float y, int button) {
    UIElement::on_mouse_down(x, y, button);
    
    if (button == VK_LBUTTON) {
        if (m_root) {
            TreeNode* clicked_node = hit_test_node(m_root.get(), x, y);
            if (clicked_node) {
                // Handle node click
                // Could expand/collapse or select the node
            }
        }
    }
}

void UIOptimized::TreemapElement::on_mouse_up(float x, float y, int button) {
    UIElement::on_mouse_up(x, y, button);
}

void UIOptimized::TreemapElement::on_mouse_wheel(float delta, float x, float y) {
    // Implement zooming
    float zoom_factor = delta > 0 ? 1.1f : 0.9f;
    m_zoom_level *= zoom_factor;
    
    // Clamp zoom level
    m_zoom_level = std::max(0.1f, std::min(m_zoom_level, 10.0f));
}

void UIOptimized::TreemapElement::layout_squarified(TreeNode* node, const D2D1_RECT_F& bounds) {
    if (!node) return;
    
    node->bounds = bounds;
    
    if (node->is_leaf || node->children.empty()) {
        return;
    }
    
    // Squarified treemap algorithm
    std::vector<TreeNode*> sorted_children = node->children;
    std::sort(sorted_children.begin(), sorted_children.end(),
              [](const std::unique_ptr<TreeNode>& a, const std::unique_ptr<TreeNode>& b) {
                  return a->size > b->size;
              });
    
    // Normalize sizes
    uint64_t total_size = 0;
    for (const auto& child : sorted_children) {
        total_size += child->size;
    }
    
    if (total_size == 0) return;
    
    float bounds_width = bounds.right - bounds.left;
    float bounds_height = bounds.bottom - bounds.top;
    float bounds_area = bounds_width * bounds_height;
    
    // Assign normalized areas
    for (auto& child : sorted_children) {
        float ratio = static_cast<float>(child->size) / static_cast<float>(total_size);
        child->bounds = D2D1::RectF(0, 0, bounds_width * ratio, bounds_height * ratio);
    }
    
    // Layout children recursively
    float current_x = bounds.left;
    float current_y = bounds.top;
    float row_height = 0;
    
    for (size_t i = 0; i < sorted_children.size(); ++i) {
        TreeNode* child = sorted_children[i].get();
        
        // Calculate dimensions
        float width = std::sqrt(static_cast<float>(child->size) / static_cast<float>(total_size) * bounds_area);
        float height = static_cast<float>(child->size) / static_cast<float>(total_size) * bounds_area / width;
        
        // Position child
        D2D1_RECT_F child_bounds = D2D1::RectF(current_x, current_y, 
                                              current_x + width, current_y + height);
        layout_squarified(child, child_bounds);
        
        // Update position for next child
        current_x += width;
        row_height = std::max(row_height, height);
        
        // Check if we need to start a new row
        if (current_x > bounds.right) {
            current_x = bounds.left;
            current_y += row_height;
            row_height = 0;
        }
    }
}

void UIOptimized::TreemapElement::layout_strip(TreeNode* node, const D2D1_RECT_F& bounds) {
    // Strip layout algorithm implementation
    if (!node || node->children.empty()) return;
    
    node->bounds = bounds;
    
    float bounds_width = bounds.right - bounds.left;
    float bounds_height = bounds.bottom - bounds.top;
    float total_size = 0;
    
    // Calculate total size
    for (const auto& child : node->children) {
        total_size += child->size;
    }
    
    if (total_size == 0) return;
    
    // Horizontal strips
    float current_y = bounds.top;
    for (auto& child : node->children) {
        float height_ratio = static_cast<float>(child->size) / static_cast<float>(total_size);
        float height = bounds_height * height_ratio;
        
        D2D1_RECT_F child_bounds = D2D1::RectF(bounds.left, current_y,
                                             bounds.right, current_y + height);
        layout_strip(child.get(), child_bounds);
        
        current_y += height;
    }
}

void UIOptimized::TreemapElement::layout_spiral(TreeNode* node, const D2D1_RECT_F& bounds) {
    // Spiral layout algorithm implementation
    if (!node || node->children.empty()) return;
    
    node->bounds = bounds;
    
    // Simplified spiral layout - arrange children in spiral pattern
    float center_x = (bounds.left + bounds.right) / 2.0f;
    float center_y = (bounds.top + bounds.bottom) / 2.0f;
    
    float angle = 0.0f;
    float radius = 10.0f;
    float radius_increment = 5.0f;
    
    for (auto& child : node->children) {
        float x = center_x + radius * std::cos(angle);
        float y = center_y + radius * std::sin(angle);
        
        // Size based on child's importance
        float size = std::sqrt(static_cast<float>(child->size)) * 2.0f;
        
        D2D1_RECT_F child_bounds = D2D1::RectF(x - size/2, y - size/2, x + size/2, y + size/2);
        layout_spiral(child.get(), child_bounds);
        
        angle += 0.5f;
        radius += radius_increment;
    }
}

void UIOptimized::TreemapElement::render_node(ID2D1DeviceContext* context, 
                                             const TreeNode* node, 
                                             const D2D1_RECT_F& bounds) {
    if (!context || !node) return;
    
    // Clip to bounds
    context->PushAxisAlignedClip(bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    
    // Fill node background
    D2D1_COLOR_F color = get_color_for_node(node);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    context->CreateSolidColorBrush(color, &brush);
    
    if (brush) {
        context->FillRectangle(bounds, brush.Get());
    }
    
    // Draw border
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> border_brush;
    context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 0.3f), &border_brush);
    
    if (border_brush) {
        context->DrawRectangle(bounds, border_brush.Get(), 1.0f);
    }
    
    // Render text for leaf nodes or when zoomed in enough
    if (node->is_leaf || (bounds.right - bounds.left > 50 && bounds.bottom - bounds.top > 20)) {
        render_text(context, node->name, bounds);
    }
    
    // Render children if expanded
    if (node->is_expanded && !node->children.empty()) {
        for (const auto& child : node->children) {
            render_node(context, child.get(), child->bounds);
        }
    }
    
    context->PopAxisAlignedClip();
}

void UIOptimized::TreemapElement::render_text(ID2D1DeviceContext* context, 
                                              const std::wstring& text, 
                                              const D2D1_RECT_F& bounds) {
    if (!context || text.empty()) return;
    
    // Create text layout
    Microsoft::WRL::ComPtr<IDWriteTextLayout> text_layout;
    
    // Simplified text rendering
    std::string narrow_text(text.begin(), text.end());
    
    // Draw text (simplified)
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> text_brush;
    context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &text_brush);
    
    if (text_brush) {
        D2D1_RECT_F text_rect = bounds;
        text_rect.left += 2;
        text_rect.top += 2;
        text_rect.right -= 2;
        text_rect.bottom -= 2;
        
        // In a real implementation, we would use DirectWrite to layout text
        context->DrawTextA(narrow_text.c_str(), static_cast<UINT32>(narrow_text.length()),
                          nullptr, text_rect, text_brush.Get());
    }
}

D2D1_COLOR_F UIOptimized::TreemapElement::get_color_for_node(const TreeNode* node) const {
    if (!node) {
        return D2D1::ColorF(D2D1::ColorF::Gray);
    }
    
    // Generate color based on node properties
    // This is a simplified implementation
    if (node->is_leaf) {
        // Leaf nodes get color based on size
        float intensity = std::min(1.0f, static_cast<float>(node->size) / 1000000.0f);
        return D2D1::ColorF(intensity, 0.5f, 1.0f - intensity);
    } else {
        // Parent nodes get different colors
        return D2D1::ColorF(0.2f, 0.6f, 0.8f);
    }
}

bool UIOptimized::TreemapElement::is_node_visible(const TreeNode* node, 
                                                 const D2D1_RECT_F& viewport) const {
    if (!node) return false;
    
    const D2D1_RECT_F& bounds = node->bounds;
    return !(bounds.right < viewport.left || bounds.left > viewport.right ||
             bounds.bottom < viewport.top || bounds.top > viewport.bottom);
}

UIOptimized::TreemapElement::TreeNode* 
UIOptimized::TreemapElement::hit_test_node(TreeNode* node, float x, float y) const {
    if (!node) return nullptr;
    
    const D2D1_RECT_F& bounds = node->bounds;
    if (x < bounds.left || x > bounds.right || y < bounds.top || y > bounds.bottom) {
        return nullptr;
    }
    
    // Check children first (depth-first)
    if (!node->is_leaf && !node->children.empty()) {
        for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
            TreeNode* result = hit_test_node(it->get(), x, y);
            if (result) {
                return result;
            }
        }
    }
    
    // If no children hit, this node is the hit
    return node;
}

D2D1_COLOR_F UIOptimized::TreemapElement::get_color_for_extension(const std::wstring& extension) {
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

D2D1_COLOR_F UIOptimized::TreemapElement::get_color_for_size(uint64_t size) {
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