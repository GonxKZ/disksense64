#ifndef CORE_GFX_UI_OPTIMIZED_H
#define CORE_GFX_UI_OPTIMIZED_H

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dcomp.h>
#include <wincodec.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

// Forward declarations
struct TreemapNode;
class UIManager;

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
        bool is_node_visible(const TreeNode* node, const D2D1_RECT_F& viewport) const;
        
        // Hit testing
        TreeNode* hit_test_node(TreeNode* node, float x, float y) const;
    };
    
    // List view element with virtualization
    class ListViewElement : public UIElement {
    public:
        struct ListItem {
            std::wstring text;
            std::wstring subtext;
            D2D1_COLOR_F color;
            void* user_data;
            
            ListItem(const std::wstring& t, const std::wstring& st = L"")
                : text(t), subtext(st), color(D2D1::ColorF(D2D1::ColorF::Black)), user_data(nullptr) {}
        };
        
    private:
        std::vector<ListItem> m_items;
        std::vector<size_t> m_selected_indices;
        size_t m_top_index;
        size_t m_visible_count;
        float m_item_height;
        bool m_multi_select;
        
        // Rendering resources
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_main_text_format;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_sub_text_format;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_item_brush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_selected_brush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_text_brush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_subtext_brush;
        
    public:
        ListViewElement(const D2D1_RECT_F& bounds);
        ~ListViewElement() override = default;
        
        // Item management
        void add_item(const ListItem& item);
        void add_items(const std::vector<ListItem>& items);
        void clear_items();
        void remove_item(size_t index);
        
        // Selection
        void select_item(size_t index, bool extend_selection = false);
        void deselect_item(size_t index);
        void select_all();
        void deselect_all();
        const std::vector<size_t>& get_selected_indices() const { return m_selected_indices; }
        
        // Scrolling
        void scroll_to_top();
        void scroll_to_bottom();
        void scroll_by_lines(int lines);
        void scroll_to_item(size_t index);
        
        // Rendering
        void render(ID2D1DeviceContext* context) override;
        
        // Input handling
        void on_mouse_move(float x, float y) override;
        void on_mouse_down(float x, float y, int button) override;
        void on_mouse_up(float x, float y, int button) override;
        void on_key_down(WPARAM key) override;
        
    private:
        void update_visible_range();
        void render_item(ID2D1DeviceContext* context, size_t index, const D2D1_RECT_F& bounds);
        size_t hit_test_item(float y) const;
    };
    
private:
    HWND m_hwnd;
    bool m_initialized;
    
    // Direct2D resources
    Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2d_factory;
    Microsoft::WRL::ComPtr<ID2D1Device> m_d2d_device;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2d_context;
    Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgi_device;
    Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgi_adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgi_factory;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_target_bitmap;
    
    // DirectWrite resources
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dw_factory;
    
    // DirectComposition resources
    Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcomp_device;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_dcomp_target;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_dcomp_visual;
    
    // UI elements
    std::vector<std::unique_ptr<UIElement>> m_elements;
    std::unique_ptr<TreemapElement> m_treemap;
    std::unique_ptr<ListViewElement> m_list_view;
    
    // Frame timing
    std::atomic<double> m_frame_time_ms;
    std::atomic<uint64_t> m_frame_count;
    std::chrono::high_resolution_clock::time_point m_last_frame_time;
    
    // Threading
    std::atomic<bool> m_rendering_active;
    std::unique_ptr<std::thread> m_render_thread;
    mutable std::mutex m_render_mutex;
    
public:
    UIOptimized();
    ~UIOptimized();
    
    // Initialization
    bool initialize(HWND hwnd);
    void shutdown();
    
    // Window management
    void resize(UINT width, UINT height);
    void show_window();
    void hide_window();
    
    // Element management
    template<typename T, typename... Args>
    T* create_element(Args&&... args) {
        auto element = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = element.get();
        m_elements.push_back(std::move(element));
        return ptr;
    }
    
    // Rendering
    void begin_frame();
    void end_frame();
    void render();
    
    // Input handling
    void on_mouse_move(float x, float y);
    void on_mouse_down(float x, float y, int button);
    void on_mouse_up(float x, float y, int button);
    void on_mouse_wheel(float delta, float x, float y);
    void on_key_down(WPARAM key);
    void on_key_up(WPARAM key);
    
    // Frame timing
    double get_current_frame_time() const { return m_frame_time_ms.load(); }
    double get_average_frame_time() const;
    uint64_t get_frame_count() const { return m_frame_count.load(); }
    
    // Performance monitoring
    struct PerformanceStats {
        double current_fps;
        double average_frame_time_ms;
        double cpu_usage_percent;
        size_t memory_usage_mb;
        uint64_t total_frames_rendered;
        
        PerformanceStats() 
            : current_fps(0.0), average_frame_time_ms(0.0),
              cpu_usage_percent(0.0), memory_usage_mb(0), total_frames_rendered(0) {}
    };
    
    PerformanceStats get_performance_stats() const;
    
    // Animation support
    class Animation {
    public:
        enum class Type {
            LINEAR,
            EASE_IN,
            EASE_OUT,
            EASE_IN_OUT,
            BOUNCE,
            ELASTIC
        };
        
        struct Keyframe {
            float value;
            float time; // 0.0 to 1.0
            Type easing;
            
            Keyframe(float v, float t, Type e = Type::LINEAR)
                : value(v), time(t), easing(e) {}
        };
        
    private:
        std::vector<Keyframe> m_keyframes;
        float m_current_value;
        float m_start_time;
        float m_duration;
        bool m_playing;
        std::function<void(float)> m_callback;
        
    public:
        Animation();
        ~Animation() = default;
        
        void add_keyframe(const Keyframe& keyframe);
        void set_duration(float duration_seconds);
        void play();
        void stop();
        void pause();
        void update(float delta_time);
        float get_current_value() const { return m_current_value; }
        void set_callback(const std::function<void(float)>& callback) { m_callback = callback; }
        
    private:
        float interpolate(float t, Type easing) const;
    };
    
private:
    // Private initialization methods
    bool initialize_d2d();
    bool initialize_dwrite();
    bool initialize_dcomp();
    bool create_render_target();
    
    // Render thread
    void render_worker();
    
    // Performance monitoring
    void update_performance_counters();
};

#endif // CORE_GFX_UI_OPTIMIZED_H