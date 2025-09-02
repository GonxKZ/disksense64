#include "ui_optimized.h"
#include <windowsx.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <wrl/client.h>

using namespace Microsoft::WRL;

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
    ComPtr<ID2D1SolidColorBrush> brush;
    context->CreateSolidColorBrush(color, &brush);
    
    if (brush) {
        context->FillRectangle(bounds, brush.Get());
    }
    
    // Draw border
    ComPtr<ID2D1SolidColorBrush> border_brush;
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
    ComPtr<IDWriteTextLayout> text_layout;
    
    // Simplified text rendering
    std::string narrow_text(text.begin(), text.end());
    
    // Draw text (simplified)
    ComPtr<ID2D1SolidColorBrush> text_brush;
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
    if (!node->children.empty()) {
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

// ListViewElement implementation
UIOptimized::ListViewElement::ListViewElement(const D2D1_RECT_F& bounds)
    : UIElement(ElementType::LIST_VIEW, bounds),
      m_top_index(0), m_visible_count(0), m_item_height(20.0f),
      m_multi_select(false) {
}

void UIOptimized::ListViewElement::add_item(const ListItem& item) {
    m_items.push_back(item);
    update_visible_range();
}

void UIOptimized::ListViewElement::add_items(const std::vector<ListItem>& items) {
    m_items.insert(m_items.end(), items.begin(), items.end());
    update_visible_range();
}

void UIOptimized::ListViewElement::clear_items() {
    m_items.clear();
    m_selected_indices.clear();
    m_top_index = 0;
}

void UIOptimized::ListViewElement::remove_item(size_t index) {
    if (index < m_items.size()) {
        m_items.erase(m_items.begin() + index);
        
        // Update selections
        std::vector<size_t> new_selections;
        for (size_t sel_index : m_selected_indices) {
            if (sel_index < index) {
                new_selections.push_back(sel_index);
            } else if (sel_index > index) {
                new_selections.push_back(sel_index - 1);
            }
        }
        m_selected_indices = std::move(new_selections);
        
        update_visible_range();
    }
}

void UIOptimized::ListViewElement::select_item(size_t index, bool extend_selection) {
    if (index >= m_items.size()) {
        return;
    }
    
    if (!extend_selection) {
        m_selected_indices.clear();
    }
    
    // Check if already selected
    auto it = std::find(m_selected_indices.begin(), m_selected_indices.end(), index);
    if (it == m_selected_indices.end()) {
        m_selected_indices.push_back(index);
    }
}

void UIOptimized::ListViewElement::deselect_item(size_t index) {
    auto it = std::find(m_selected_indices.begin(), m_selected_indices.end(), index);
    if (it != m_selected_indices.end()) {
        m_selected_indices.erase(it);
    }
}

void UIOptimized::ListViewElement::select_all() {
    m_selected_indices.clear();
    m_selected_indices.reserve(m_items.size());
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_selected_indices.push_back(i);
    }
}

void UIOptimized::ListViewElement::deselect_all() {
    m_selected_indices.clear();
}

void UIOptimized::ListViewElement::scroll_to_top() {
    m_top_index = 0;
    update_visible_range();
}

void UIOptimized::ListViewElement::scroll_to_bottom() {
    if (!m_items.empty()) {
        m_top_index = m_items.size() - 1;
        update_visible_range();
    }
}

void UIOptimized::ListViewElement::scroll_by_lines(int lines) {
    if (lines > 0) {
        m_top_index = std::min(m_top_index + static_cast<size_t>(lines), m_items.size() - 1);
    } else {
        m_top_index = m_top_index >= static_cast<size_t>(-lines) ? 
                     m_top_index + lines : 0;
    }
    update_visible_range();
}

void UIOptimized::ListViewElement::scroll_to_item(size_t index) {
    if (index < m_items.size()) {
        m_top_index = index;
        update_visible_range();
    }
}

void UIOptimized::ListViewElement::render(ID2D1DeviceContext* context) {
    if (!context || !m_visible || m_items.empty()) {
        return;
    }
    
    // Clip to bounds
    context->PushAxisAlignedClip(m_bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    
    // Render background
    ComPtr<ID2D1SolidColorBrush> bg_brush;
    context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &bg_brush);
    if (bg_brush) {
        context->FillRectangle(m_bounds, bg_brush.Get());
    }
    
    // Render items
    update_visible_range();
    for (size_t i = 0; i < m_visible_count && (m_top_index + i) < m_items.size(); ++i) {
        size_t item_index = m_top_index + i;
        float item_top = m_bounds.top + static_cast<float>(i) * m_item_height;
        D2D1_RECT_F item_bounds = D2D1::RectF(m_bounds.left, item_top,
                                             m_bounds.right, item_top + m_item_height);
        
        render_item(context, item_index, item_bounds);
    }
    
    context->PopAxisAlignedClip();
}

void UIOptimized::ListViewElement::on_mouse_move(float x, float y) {
    UIElement::on_mouse_move(x, y);
    // Handle hover effects
}

void UIOptimized::ListViewElement::on_mouse_down(float x, float y, int button) {
    UIElement::on_mouse_down(x, y, button);
    
    if (button == VK_LBUTTON) {
        size_t item_index = hit_test_item(y);
        if (item_index < m_items.size()) {
            select_item(item_index, GetAsyncKeyState(VK_CONTROL) < 0);
        }
    }
}

void UIOptimized::ListViewElement::on_mouse_up(float x, float y, int button) {
    UIElement::on_mouse_up(x, y, button);
}

void UIOptimized::ListViewElement::on_key_down(WPARAM key) {
    UIElement::on_key_down(key);
    
    switch (key) {
        case VK_UP:
            if (!m_selected_indices.empty()) {
                size_t first_selected = *std::min_element(m_selected_indices.begin(), 
                                                         m_selected_indices.end());
                if (first_selected > 0) {
                    select_item(first_selected - 1, false);
                    scroll_to_item(first_selected - 1);
                }
            }
            break;
        case VK_DOWN:
            if (!m_selected_indices.empty()) {
                size_t last_selected = *std::max_element(m_selected_indices.begin(), 
                                                       m_selected_indices.end());
                if (last_selected + 1 < m_items.size()) {
                    select_item(last_selected + 1, false);
                    scroll_to_item(last_selected + 1);
                }
            }
            break;
        case VK_SPACE:
            if (!m_selected_indices.empty()) {
                // Toggle selection or perform action
            }
            break;
    }
}

void UIOptimized::ListViewElement::update_visible_range() {
    float visible_height = m_bounds.bottom - m_bounds.top;
    m_visible_count = static_cast<size_t>(std::ceil(visible_height / m_item_height));
    m_visible_count = std::min(m_visible_count, m_items.size() - m_top_index);
}

void UIOptimized::ListViewElement::render_item(ID2D1DeviceContext* context, 
                                               size_t index, 
                                               const D2D1_RECT_F& bounds) {
    if (!context || index >= m_items.size()) return;
    
    const ListItem& item = m_items[index];
    
    // Check if selected
    bool is_selected = std::find(m_selected_indices.begin(), m_selected_indices.end(), index) 
                      != m_selected_indices.end();
    
    // Create brushes
    ComPtr<ID2D1SolidColorBrush> item_brush, text_brush;
    if (is_selected) {
        context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &item_brush);
    } else {
        context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &item_brush);
    }
    
    context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &text_brush);
    
    // Render background
    if (item_brush) {
        context->FillRectangle(bounds, item_brush.Get());
    }
    
    // Render text
    if (text_brush) {
        D2D1_RECT_F text_bounds = bounds;
        text_bounds.left += 5; // Left padding
        text_bounds.right -= 5; // Right padding
        
        // Render main text
        std::string narrow_text(item.text.begin(), item.text.end());
        context->DrawTextA(narrow_text.c_str(), static_cast<UINT32>(narrow_text.length()),
                          nullptr, text_bounds, text_brush.Get());
        
        // Render subtext if present
        if (!item.subtext.empty()) {
            D2D1_RECT_F subtext_bounds = text_bounds;
            subtext_bounds.top += m_item_height * 0.5f;
            
            std::string narrow_subtext(item.subtext.begin(), item.subtext.end());
            context->DrawTextA(narrow_subtext.c_str(), static_cast<UINT32>(narrow_subtext.length()),
                              nullptr, subtext_bounds, text_brush.Get());
        }
    }
}

size_t UIOptimized::ListViewElement::hit_test_item(float y) const {
    float relative_y = y - m_bounds.top;
    size_t item_index = static_cast<size_t>(relative_y / m_item_height);
    return m_top_index + item_index;
}

// UIOptimized implementation
UIOptimized::UIOptimized()
    : m_hwnd(nullptr), m_initialized(false),
      m_frame_time_ms(0.0), m_frame_count(0),
      m_rendering_active(false) {
    m_last_frame_time = std::chrono::high_resolution_clock::now();
}

UIOptimized::~UIOptimized() {
    shutdown();
}

bool UIOptimized::initialize(HWND hwnd) {
    if (m_initialized) {
        return true;
    }
    
    m_hwnd = hwnd;
    
    // Initialize Direct2D
    if (!initialize_d2d()) {
        return false;
    }
    
    // Initialize DirectWrite
    if (!initialize_dwrite()) {
        return false;
    }
    
    // Initialize DirectComposition
    if (!initialize_dcomp()) {
        return false;
    }
    
    // Create render target
    if (!create_render_target()) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void UIOptimized::shutdown() {
    m_rendering_active = false;
    
    if (m_render_thread && m_render_thread->joinable()) {
        m_render_thread->join();
    }
    
    m_render_thread.reset();
    
    // Release resources
    m_dcomp_visual.Reset();
    m_dcomp_target.Reset();
    m_dcomp_device.Reset();
    
    m_target_bitmap.Reset();
    
    m_d2d_context.Reset();
    m_d2d_device.Reset();
    m_d2d_factory.Reset();
    
    m_dw_factory.Reset();
    
    m_initialized = false;
}

void UIOptimized::resize(UINT width, UINT height) {
    if (!m_initialized) {
        return;
    }
    
    // Resize render target
    if (m_target_bitmap) {
        D2D1_SIZE_U size = D2D1::SizeU(width, height);
        m_target_bitmap->Resize(size);
    }
    
    // Update composition target
    if (m_dcomp_target) {
        m_dcomp_target->SetRoot(m_dcomp_visual.Get());
    }
}

void UIOptimized::show_window() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void UIOptimized::hide_window() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void UIOptimized::begin_frame() {
    if (!m_initialized) {
        return;
    }
    
    m_last_frame_time = std::chrono::high_resolution_clock::now();
    
    // Begin drawing
    if (m_d2d_context) {
        m_d2d_context->BeginDraw();
        m_d2d_context->Clear(D2D1::ColorF(D2D1::ColorF::White));
    }
}

void UIOptimized::end_frame() {
    if (!m_initialized || !m_d2d_context) {
        return;
    }
    
    // End drawing
    HRESULT hr = m_d2d_context->EndDraw();
    
    // Present
    if (SUCCEEDED(hr) && m_target_bitmap) {
        // In a real implementation, we would present to the window
    }
    
    // Update frame timing
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_frame_time);
    m_frame_time_ms = duration.count() / 1000.0;
    m_frame_count.fetch_add(1);
}

void UIOptimized::render() {
    if (!m_initialized) {
        return;
    }
    
    begin_frame();
    
    // Render all elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible()) {
            element->render(m_d2d_context.Get());
        }
    }
    
    end_frame();
}

void UIOptimized::on_mouse_move(float x, float y) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible() && element->hit_test(x, y)) {
            element->on_mouse_move(x, y);
        }
    }
}

void UIOptimized::on_mouse_down(float x, float y, int button) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible() && element->hit_test(x, y)) {
            element->on_mouse_down(x, y, button);
        }
    }
}

void UIOptimized::on_mouse_up(float x, float y, int button) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible() && element->hit_test(x, y)) {
            element->on_mouse_up(x, y, button);
        }
    }
}

void UIOptimized::on_mouse_wheel(float delta, float x, float y) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible() && element->hit_test(x, y)) {
            // Check if element has mouse wheel handler
            // This is a simplified approach
        }
    }
}

void UIOptimized::on_key_down(WPARAM key) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible()) {
            element->on_key_down(key);
        }
    }
}

void UIOptimized::on_key_up(WPARAM key) {
    // Forward to elements
    for (const auto& element : m_elements) {
        if (element && element->is_visible()) {
            element->on_key_up(key);
        }
    }
}

double UIOptimized::get_average_frame_time() const {
    // Return moving average of frame times
    return m_frame_time_ms.load();
}

UIOptimized::PerformanceStats UIOptimized::get_performance_stats() const {
    PerformanceStats stats;
    
    stats.current_fps = 1000.0 / m_frame_time_ms.load();
    stats.average_frame_time_ms = m_frame_time_ms.load();
    stats.total_frames_rendered = m_frame_count.load();
    stats.cpu_usage_percent = 0.0; // Would need to query system
    stats.memory_usage_mb = 0; // Would need to query process
    
    return stats;
}

// Private initialization methods
bool UIOptimized::initialize_d2d() {
    // Create D2D factory
    D2D1_FACTORY_OPTIONS options = {};
#ifndef NDEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, 
                                   __uuidof(ID2D1Factory1), 
                                   &options, 
                                   reinterpret_cast<void**>(m_d2d_factory.GetAddressOf()));
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Get DXGI device
    hr = m_d2d_factory.As(&m_dxgi_device);
    if (FAILED(hr)) {
        return false;
    }
    
    // Create D2D device
    hr = m_d2d_factory->CreateDevice(m_dxgi_device.Get(), m_d2d_device.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Create device context
    hr = m_d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, 
                                           m_d2d_context.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool UIOptimized::initialize_dwrite() {
    // Create DirectWrite factory
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                    __uuidof(IDWriteFactory),
                                    reinterpret_cast<IUnknown**>(m_dw_factory.GetAddressOf()));
    
    return SUCCEEDED(hr);
}

bool UIOptimized::initialize_dcomp() {
    // Create DirectComposition device
    HRESULT hr = DCompositionCreateDevice(m_dxgi_device.Get(),
                                         __uuidof(IDCompositionDevice),
                                         reinterpret_cast<void**>(m_dcomp_device.GetAddressOf()));
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create composition target
    hr = m_dcomp_device->CreateTargetForHwnd(m_hwnd, TRUE, m_dcomp_target.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Create root visual
    hr = m_dcomp_device->CreateVisual(m_dcomp_visual.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }
    
    // Set root visual
    hr = m_dcomp_target->SetRoot(m_dcomp_visual.Get());
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool UIOptimized::create_render_target() {
    if (!m_hwnd || !m_d2d_context) {
        return false;
    }
    
    // Get client rect
    RECT client_rect;
    GetClientRect(m_hwnd, &client_rect);
    
    D2D1_SIZE_U size = D2D1::SizeU(client_rect.right - client_rect.left,
                                   client_rect.bottom - client_rect.top);
    
    // Create bitmap properties
    D2D1_BITMAP_PROPERTIES1 bitmap_properties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    
    // Create bitmap
    HRESULT hr = m_d2d_context->CreateBitmap(size, nullptr, 0, &bitmap_properties, 
                                            m_target_bitmap.GetAddressOf());
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Set as target
    m_d2d_context->SetTarget(m_target_bitmap.Get());
    
    return true;
}

void UIOptimized::render_worker() {
    while (m_rendering_active) {
        render();
        
        // Maintain target frame rate
        double frame_time = m_frame_time_ms.load();
        double target_frame_time = TARGET_FRAME_TIME_60FPS;
        
        if (frame_time < target_frame_time) {
            double sleep_time = target_frame_time - frame_time;
            std::this_thread::sleep_for(std::chrono::microseconds(
                static_cast<long long>(sleep_time * 1000)));
        }
    }
}

void UIOptimized::update_performance_counters() {
    // Update performance counters periodically
    static auto last_update = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
    
    if (duration.count() >= 1) {
        // Update FPS counter, memory usage, etc.
        last_update = now;
    }
}

// Animation implementation
UIOptimized::Animation::Animation()
    : m_current_value(0.0f), m_start_time(0.0f), m_duration(1.0f),
      m_playing(false) {
}

void UIOptimized::Animation::add_keyframe(const Keyframe& keyframe) {
    m_keyframes.push_back(keyframe);
    std::sort(m_keyframes.begin(), m_keyframes.end(),
              [](const Keyframe& a, const Keyframe& b) {
                  return a.time < b.time;
              });
}

void UIOptimized::Animation::set_duration(float duration_seconds) {
    m_duration = duration_seconds;
}

void UIOptimized::Animation::play() {
    m_start_time = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count()) / 1000.0f;
    m_playing = true;
}

void UIOptimized::Animation::stop() {
    m_playing = false;
    m_current_value = 0.0f;
}

void UIOptimized::Animation::pause() {
    m_playing = false;
}

void UIOptimized::Animation::update(float delta_time) {
    if (!m_playing) {
        return;
    }
    
    float current_time = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    float elapsed = current_time - m_start_time;
    float progress = std::min(1.0f, elapsed / m_duration);
    
    if (progress >= 1.0f) {
        m_playing = false;
    }
    
    // Interpolate value based on keyframes
    if (m_keyframes.empty()) {
        m_current_value = 0.0f;
    } else if (m_keyframes.size() == 1) {
        m_current_value = m_keyframes[0].value;
    } else {
        // Find keyframes to interpolate between
        size_t keyframe_index = 0;
        for (size_t i = 0; i < m_keyframes.size() - 1; ++i) {
            if (progress >= m_keyframes[i].time && progress <= m_keyframes[i + 1].time) {
                keyframe_index = i;
                break;
            }
        }
        
        const Keyframe& key1 = m_keyframes[keyframe_index];
        const Keyframe& key2 = m_keyframes[std::min(keyframe_index + 1, m_keyframes.size() - 1)];
        
        if (key1.time == key2.time) {
            m_current_value = key1.value;
        } else {
            float t = (progress - key1.time) / (key2.time - key1.time);
            t = interpolate(t, key1.easing);
            m_current_value = key1.value + t * (key2.value - key1.value);
        }
    }
    
    // Call callback if set
    if (m_callback) {
        m_callback(m_current_value);
    }
}

float UIOptimized::Animation::interpolate(float t, Type easing) const {
    switch (easing) {
        case Type::LINEAR:
            return t;
        case Type::EASE_IN:
            return t * t * t;
        case Type::EASE_OUT:
            return 1.0f - std::pow(1.0f - t, 3.0f);
        case Type::EASE_IN_OUT:
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        case Type::BOUNCE:
            // Simplified bounce
            return t < 0.3636f ? 7.5625f * t * t : 
                   t < 0.7272f ? 7.5625f * (t - 0.5454f) * (t - 0.5454f) + 0.75f :
                   t < 0.9090f ? 7.5625f * (t - 0.8181f) * (t - 0.8181f) + 0.9375f :
                   7.5625f * (t - 0.9545f) * (t - 0.9545f) + 0.984375f;
        case Type::ELASTIC:
            // Simplified elastic
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * 3.14159f / 3.0f)) + 1.0f;
        default:
            return t;
    }
}