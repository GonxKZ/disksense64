#ifndef CORE_GFX_UI_OPTIMIZED_H
#define CORE_GFX_UI_OPTIMIZED_H

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dcomp.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <wrl/client.h>

// Forward declarations
struct TreemapNode;

// High-performance UI manager with Direct2D/DirectComposition
class UIOptimized {
public:
    // Frame timing target
    static constexpr double TARGET_FRAME_TIME_60FPS = 16.666; // ms
    static constexpr double TARGET_FRAME_TIME_120FPS = 8.333; // ms
    
    // UI element types
    enum class ElementType {
        WINDOW,
        PANEL,
        BUTTON,
        LABEL,
        LIST_VIEW,
        TREEMAP,
        PROGRESS_BAR,
        SCROLLBAR
    };
    
    // Base UI element
    class UIElement {
    protected:
        ElementType m_type;
        D2D1_RECT_F m_bounds;
        bool m_visible;
        bool m_enabled;
        std::wstring m_tooltip;
        
    public:
        UIElement(ElementType type, const D2D1_RECT_F& bounds)
            : m_type(type), m_bounds(bounds), m_visible(true), m_enabled(true) {}
        
        virtual ~UIElement() = default;
        
        // Get/set bounds
        const D2D1_RECT_F& get_bounds() const { return m_bounds; }
        void set_bounds(const D2D1_RECT_F& bounds) { m_bounds = bounds; }
        
        // Visibility
        bool is_visible() const { return m_visible; }
        void set_visible(bool visible) { m_visible = visible; }
        
        // Enabled state
        bool is_enabled() const { return m_enabled; }
        void set_enabled(bool enabled) { m_enabled = enabled; }
        
        // Tooltip
        const std::wstring& get_tooltip() const { return m_tooltip; }
        void set_tooltip(const std::wstring& tooltip) { m_tooltip = tooltip; }
        
        // Virtual methods for derived classes
        virtual void render(ID2D1DeviceContext* context) = 0;
        virtual bool hit_test(float x, float y) const;
        virtual void on_mouse_move(float x, float y) {}
        virtual void on_mouse_down(float x, float y, int button) {}
        virtual void on_mouse_up(float x, float y, int button) {}
        virtual void on_key_down(WPARAM key) {}
        virtual void on_key_up(WPARAM key) {}
    };
    
    // Treemap visualization element
    class TreemapElement : public UIElement {
    public:
        struct TreeNode {
            std::wstring name;
            uint64_t size;
            D2D1_COLOR_F color;
            D2D1_RECT_F bounds;
            std::vector<std::unique_ptr<TreeNode>> children;
            bool is_leaf;
            bool is_expanded;
            
            TreeNode(const std::wstring& n, uint64_t s)
                : name(n), size(s), color(D2D1::ColorF(D2D1::ColorF::Blue)),
                  is_leaf(true), is_expanded(false) {}
        };
        
        // Layout algorithms
        enum class LayoutAlgorithm {
            SQUARIFIED,  // Minimize aspect ratios
            STRIP,       // Strip layout
            SPIRAL       // Spiral layout
        };
        
        // Interaction modes
        enum class InteractionMode {
            NAVIGATE,    // Pan and zoom
            SELECT,      // Select nodes
            DRAG         // Drag nodes
        };
        
    private:
        std::unique_ptr<TreeNode> m_root;
        LayoutAlgorithm m_layout_algorithm;
        InteractionMode m_interaction_mode;
        D2D1_POINT_2F m_view_offset;
        float m_zoom_level;
        std::vector<D2D1_RECT_F> m_view_history;
        size_t m_history_index;
        
        // Rendering resources
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_text_format;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_node_brush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_border_brush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_text_brush;
        
    public:
        TreemapElement(const D2D1_RECT_F& bounds);
        ~TreemapElement() override = default;
        
        // Set root node
        void set_root(std::unique_ptr<TreeNode> root);
        
        // Layout management
        void set_layout_algorithm(LayoutAlgorithm algorithm) { m_layout_algorithm = algorithm; }
        LayoutAlgorithm get_layout_algorithm() const { return m_layout_algorithm; }
        
        // View management
        void set_view(const D2D1_POINT_2F& offset, float zoom);
        void zoom_to_fit();
        void navigate_back();
        void navigate_forward();
        
        // Interaction
        void set_interaction_mode(InteractionMode mode) { m_interaction_mode = mode; }
        InteractionMode get_interaction_mode() const { return m_interaction_mode; }
        
        // Rendering
        void render(ID2D1DeviceContext* context) override;
        
        // Input handling
        void on_mouse_move(float x, float y) override;
        void on_mouse_down(float x, float y, int button) override;
        void on_mouse_up(float x, float y, int button) override;
        void on_mouse_wheel(float delta, float x, float y);
        
    private:
        // Layout algorithms
        void layout_squarified(TreeNode* node, const D2D1_RECT_F& bounds);
        void layout_strip(TreeNode* node, const D2D1_RECT_F& bounds);
        void layout_spiral(TreeNode* node, const D2D1_RECT_F& bounds);
        
        // Helper functions
        void render_node(ID2D1DeviceContext* context, const TreeNode* node, const D2D1_RECT_F& bounds);
        void render_text(ID2D1DeviceContext* context, const std::wstring& text, const D2D1_RECT_F& bounds);
        D2D1_COLOR_F get_color_for_node(const TreeNode* node) const;
        D2D1_COLOR_F get_color_for_extension(const std::wstring& extension);
        D2D1_COLOR_F get_color_for_size(uint64_t size);
        
        // Hit testing
        TreeNode* hit_test_node(TreeNode* node, float x, float y) const;
    };
};

#endif // CORE_GFX_UI_OPTIMIZED_H