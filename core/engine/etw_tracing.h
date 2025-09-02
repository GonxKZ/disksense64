#ifndef CORE_ENGINE_ETW_TRACING_H
#define CORE_ENGINE_ETW_TRACING_H

#include <windows.h>
#include <evntprov.h>
#include <stdint.h>
#include <string>
#include <chrono>
#include <memory>

// ETW Provider GUID for DiskSense64
// {12345678-1234-1234-1234-123456789ABC}
static const GUID DISKSENSE64_PROVIDER_GUID = {
    0x12345678, 0x1234, 0x1234, 
    {0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc}
};

// Event levels
enum class TraceLevel {
    Critical = 1,
    Error = 2,
    Warning = 3,
    Informational = 4,
    Verbose = 5
};

// Event opcodes
enum class TraceOpcode {
    Info = 0,
    Start = 1,
    Stop = 2,
    DataCollectionStart = 3,
    DataCollectionStop = 4,
    Extension = 5,
    Reply = 6,
    Resume = 7,
    Suspend = 8,
    Send = 9,
    Receive = 10
};

// ETW tracing provider
class EtwTracingProvider {
public:
    // Event categories
    enum class Category {
        IO = 0x0001,           // I/O operations
        Hashing = 0x0002,      // Hash computations
        Deduplication = 0x0004, // Deduplication operations
        UI = 0x0008,           // User interface events
        Index = 0x0010,        // Index operations
        FileSystem = 0x0020,   // File system operations
        Memory = 0x0040,       // Memory allocations
        Threading = 0x0080,    // Thread operations
        Network = 0x0100,      // Network operations
        Security = 0x0200,     // Security events
        Performance = 0x0400,  // Performance measurements
        Debug = 0x0800,        // Debug events
        All = 0xFFFF           // All categories
    };
    
    // File I/O events
    struct IoEvent {
        uint64_t file_id_low;
        uint64_t file_id_high;
        uint64_t volume_id;
        uint64_t offset;
        uint32_t size;
        uint32_t duration_ms;
        uint32_t bytes_transferred;
        uint32_t error_code;
        const wchar_t* file_path;
        
        IoEvent() 
            : file_id_low(0), file_id_high(0), volume_id(0), offset(0),
              size(0), duration_ms(0), bytes_transferred(0), error_code(0),
              file_path(nullptr) {}
    };
    
    // Hash computation events
    struct HashEvent {
        uint64_t file_id_low;
        uint64_t file_id_high;
        uint64_t volume_id;
        uint32_t algorithm; // 0=BLAKE3, 1=SHA256, 2=pHash, 3=minHash
        uint32_t data_size;
        uint32_t duration_ms;
        uint32_t hash_rate_mbps;
        const wchar_t* file_path;
        
        HashEvent()
            : file_id_low(0), file_id_high(0), volume_id(0),
              algorithm(0), data_size(0), duration_ms(0),
              hash_rate_mbps(0), file_path(nullptr) {}
    };
    
    // Deduplication events
    struct DedupeEvent {
        uint64_t group_id;
        uint32_t file_count;
        uint64_t total_size;
        uint64_t potential_savings;
        uint32_t duration_ms;
        uint32_t duplicate_count;
        const wchar_t* operation; // "scan", "dedupe", "cleanup"
        
        DedupeEvent()
            : group_id(0), file_count(0), total_size(0),
              potential_savings(0), duration_ms(0),
              duplicate_count(0), operation(nullptr) {}
    };
    
    // UI events
    struct UiEvent {
        const wchar_t* action; // "click", "hover", "drag", "resize"
        const wchar_t* element; // Element name/type
        uint32_t x, y; // Coordinates for mouse events
        uint32_t duration_ms; // Duration for timed actions
        
        UiEvent()
            : action(nullptr), element(nullptr),
              x(0), y(0), duration_ms(0) {}
    };
    
    // Index events
    struct IndexEvent {
        const wchar_t* operation; // "insert", "update", "delete", "query"
        uint64_t key_volume_id;
        uint64_t key_file_id_low;
        uint64_t key_file_id_high;
        uint32_t duration_ms;
        uint32_t result_count;
        const wchar_t* table_name;
        
        IndexEvent()
            : operation(nullptr), key_volume_id(0),
              key_file_id_low(0), key_file_id_high(0),
              duration_ms(0), result_count(0),
              table_name(nullptr) {}
    };
    
    // Performance counter events
    struct PerformanceEvent {
        const wchar_t* metric_name;
        double value;
        const wchar_t* unit;
        uint64_t timestamp;
        
        PerformanceEvent()
            : metric_name(nullptr), value(0.0),
              unit(nullptr), timestamp(0) {}
    };
    
private:
    REGHANDLE m_reg_handle;
    bool m_registered;
    uint64_t m_keywords;
    TraceLevel m_level;
    
public:
    EtwTracingProvider();
    ~EtwTracingProvider();
    
    // Registration and configuration
    bool register_provider(const wchar_t* provider_name = L"DiskSense64");
    void unregister_provider();
    void set_trace_level(TraceLevel level) { m_level = level; }
    void set_keywords(uint64_t keywords) { m_keywords = keywords; }
    bool is_registered() const { return m_registered; }
    
    // Check if tracing is enabled for specific level and keywords
    bool is_enabled(TraceLevel level, uint64_t keywords) const;
    
    // Write events
    void write_io_event(const IoEvent& event);
    void write_hash_event(const HashEvent& event);
    void write_dedupe_event(const DedupeEvent& event);
    void write_ui_event(const UiEvent& event);
    void write_index_event(const IndexEvent& event);
    void write_performance_event(const PerformanceEvent& event);
    
    // Generic event writer
    template<typename... Args>
    void write_event(uint32_t event_id, TraceLevel level, uint64_t keywords,
                     const wchar_t* message, Args... args);
    
    // Performance tracing helpers
    class ScopedTimer {
    private:
        EtwTracingProvider* m_provider;
        uint32_t m_event_id;
        TraceLevel m_level;
        uint64_t m_keywords;
        std::chrono::high_resolution_clock::time_point m_start_time;
        
    public:
        ScopedTimer(EtwTracingProvider* provider, uint32_t event_id,
                   TraceLevel level, uint64_t keywords);
        ~ScopedTimer();
        
        void stop();
    };
    
private:
    // Helper functions
    static ULONG get_etw_level(TraceLevel level);
    static USHORT get_etw_opcode(TraceOpcode opcode);
    void write_event_internal(uint32_t event_id, TraceLevel level, uint64_t keywords,
                             const EVENT_DATA_DESCRIPTOR* data_descriptors, uint32_t descriptor_count);
};

// High-resolution timer for performance measurements
class HighResolutionTimer {
private:
    std::chrono::high_resolution_clock::time_point m_start_time;
    std::chrono::high_resolution_clock::time_point m_end_time;
    bool m_stopped;
    
public:
    HighResolutionTimer();
    ~HighResolutionTimer() = default;
    
    void start();
    void stop();
    void reset();
    
    // Get elapsed time in different units
    double get_elapsed_nanoseconds() const;
    double get_elapsed_microseconds() const;
    double get_elapsed_milliseconds() const;
    double get_elapsed_seconds() const;
    
    // Get current timestamp
    static uint64_t get_timestamp();
    
    // Calculate throughput
    double calculate_throughput_mbps(uint64_t bytes_processed) const;
    double calculate_ops_per_second(uint64_t operations_completed) const;
};

// Performance counter collection
class PerformanceCounters {
public:
    struct CounterSnapshot {
        uint64_t timestamp;
        uint64_t total_bytes_read;
        uint64_t total_bytes_written;
        uint64_t total_operations;
        uint64_t total_errors;
        uint64_t active_threads;
        uint64_t memory_usage_bytes;
        double cpu_usage_percent;
        double io_wait_time_ms;
        
        CounterSnapshot()
            : timestamp(0), total_bytes_read(0), total_bytes_written(0),
              total_operations(0), total_errors(0), active_threads(0),
              memory_usage_bytes(0), cpu_usage_percent(0.0),
              io_wait_time_ms(0.0) {}
    };
    
private:
    CounterSnapshot m_last_snapshot;
    std::chrono::high_resolution_clock::time_point m_last_update;
    mutable std::mutex m_mutex;
    
public:
    PerformanceCounters();
    ~PerformanceCounters() = default;
    
    // Take a snapshot of current performance counters
    CounterSnapshot take_snapshot() const;
    
    // Calculate deltas since last snapshot
    struct CounterDeltas {
        double elapsed_seconds;
        double read_throughput_mbps;
        double write_throughput_mbps;
        double operations_per_second;
        double error_rate_per_second;
        double thread_count_change;
        double memory_usage_change_mb;
        double cpu_usage_change_percent;
        double io_wait_time_change_ms;
        
        CounterDeltas()
            : elapsed_seconds(0.0), read_throughput_mbps(0.0),
              write_throughput_mbps(0.0), operations_per_second(0.0),
              error_rate_per_second(0.0), thread_count_change(0.0),
              memory_usage_change_mb(0.0), cpu_usage_change_percent(0.0),
              io_wait_time_change_ms(0.0) {}
    };
    
    CounterDeltas calculate_deltas(const CounterSnapshot& current) const;
    
    // Get system performance metrics
    static double get_system_cpu_usage();
    static uint64_t get_process_memory_usage();
    static uint64_t get_available_memory();
    static double get_disk_io_utilization();
    
    // Performance alert system
    struct PerformanceAlert {
        enum class Type {
            HighCpuUsage,
            LowMemory,
            SlowIo,
            HighErrorRate,
            ThreadStarvation
        };
        
        Type type;
        double threshold;
        double current_value;
        std::wstring message;
        uint64_t timestamp;
        
        PerformanceAlert(Type t, double thresh, double current, const std::wstring& msg)
            : type(t), threshold(thresh), current_value(current),
              message(msg), timestamp(HighResolutionTimer::get_timestamp()) {}
    };
    
    std::vector<PerformanceAlert> check_performance_alerts(const CounterSnapshot& snapshot) const;
    
private:
    // Helper functions
    static uint64_t get_process_id();
    static double calculate_percentage_change(double old_value, double new_value);
};

// Histogram for latency tracking
template<size_t BucketCount = 100>
class LatencyHistogram {
public:
    struct HistogramData {
        std::array<uint64_t, BucketCount> buckets;
        uint64_t total_count;
        double min_value;
        double max_value;
        double sum;
        
        HistogramData()
            : buckets{}, total_count(0), min_value(0.0),
              max_value(0.0), sum(0.0) {}
    };
    
private:
    HistogramData m_data;
    double m_bucket_width;
    mutable std::mutex m_mutex;
    
public:
    LatencyHistogram(double max_expected_value = 1000.0); // Max expected latency in ms
    ~LatencyHistogram() = default;
    
    // Record a latency value
    void record(double value);
    
    // Get histogram data
    HistogramData get_data() const;
    
    // Calculate percentiles
    struct Percentiles {
        double p50, p90, p95, p99, p999;
        
        Percentiles()
            : p50(0.0), p90(0.0), p95(0.0), p99(0.0), p999(0.0) {}
    };
    
    Percentiles calculate_percentiles() const;
    
    // Clear histogram
    void clear();
    
    // Merge with another histogram
    void merge(const HistogramData& other);
    
private:
    size_t get_bucket_index(double value) const;
};

// Trace logger with file output fallback
class TraceLogger {
public:
    enum class OutputMode {
        ETW_ONLY,      // Only ETW tracing
        FILE_ONLY,     // Only file output
        BOTH           // Both ETW and file output
    };
    
    struct LogEntry {
        uint64_t timestamp;
        TraceLevel level;
        std::wstring category;
        std::wstring message;
        std::wstring module;
        uint32_t line_number;
        
        LogEntry()
            : timestamp(0), level(TraceLevel::Informational),
              line_number(0) {}
    };
    
private:
    OutputMode m_output_mode;
    std::wstring m_log_file_path;
    std::unique_ptr<EtwTracingProvider> m_etw_provider;
    mutable std::mutex m_file_mutex;
    mutable std::mutex m_log_mutex;
    
public:
    TraceLogger(OutputMode mode = OutputMode::BOTH);
    ~TraceLogger();
    
    // Configuration
    void set_output_mode(OutputMode mode) { m_output_mode = mode; }
    void set_log_file_path(const std::wstring& path) { m_log_file_path = path; }
    bool initialize_etw(const wchar_t* provider_name = L"DiskSense64");
    
    // Logging methods
    void log_critical(const std::wstring& category, const std::wstring& message,
                     const wchar_t* module = nullptr, uint32_t line = 0);
    void log_error(const std::wstring& category, const std::wstring& message,
                  const wchar_t* module = nullptr, uint32_t line = 0);
    void log_warning(const std::wstring& category, const std::wstring& message,
                    const wchar_t* module = nullptr, uint32_t line = 0);
    void log_info(const std::wstring& category, const std::wstring& message,
                 const wchar_t* module = nullptr, uint32_t line = 0);
    void log_verbose(const std::wstring& category, const std::wstring& message,
                    const wchar_t* module = nullptr, uint32_t line = 0);
    
    // Performance logging
    void log_performance(const std::wstring& metric_name, double value,
                        const std::wstring& unit = L"ms");
    
    // Batch logging
    void log_batch(const std::vector<LogEntry>& entries);
    
    // Log file management
    bool rotate_log_file();
    bool compress_old_logs();
    std::vector<LogEntry> read_recent_entries(size_t count = 100) const;
    
private:
    void write_to_file(const LogEntry& entry);
    std::wstring format_log_entry(const LogEntry& entry) const;
    std::wstring get_timestamp_string(uint64_t timestamp) const;
};

// Macros for easy tracing
#define TRACE_CRITICAL(category, message) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_critical(category, message, _CRT_WIDE(__FILE__), __LINE__); \
        } \
    } while(0)

#define TRACE_ERROR(category, message) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_error(category, message, _CRT_WIDE(__FILE__), __LINE__); \
        } \
    } while(0)

#define TRACE_WARNING(category, message) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_warning(category, message, _CRT_WIDE(__FILE__), __LINE__); \
        } \
    } while(0)

#define TRACE_INFO(category, message) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_info(category, message, _CRT_WIDE(__FILE__), __LINE__); \
        } \
    } while(0)

#define TRACE_VERBOSE(category, message) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_verbose(category, message, _CRT_WIDE(__FILE__), __LINE__); \
        } \
    } while(0)

#define TRACE_PERFORMANCE(metric, value, unit) \
    do { \
        if (g_trace_logger) { \
            g_trace_logger->log_performance(metric, value, unit); \
        } \
    } while(0)

#define TRACE_SCOPE_TIMER(provider, event_id, level, keywords) \
    EtwTracingProvider::ScopedTimer UNIQUE_NAME(trace_timer_)(provider, event_id, level, keywords)

// Global trace logger instance
extern std::unique_ptr<TraceLogger> g_trace_logger;

#endif // CORE_ENGINE_ETW_TRACING_H