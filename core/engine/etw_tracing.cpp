#include "etw_tracing.h"
#include <windows.h>
#include <evntrace.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <mutex>
#include <thread>

// Global trace logger instance
std::unique_ptr<TraceLogger> g_trace_logger;

// EtwTracingProvider implementation
EtwTracingProvider::EtwTracingProvider()
    : m_reg_handle(0), m_registered(false), m_keywords(0), m_level(TraceLevel::Informational) {
}

EtwTracingProvider::~EtwTracingProvider() {
    unregister_provider();
}

bool EtwTracingProvider::register_provider(const wchar_t* provider_name) {
    if (m_registered) {
        return true;
    }
    
    // Register the provider with ETW
    ULONG status = EventRegister(&DISKSENSE64_PROVIDER_GUID, nullptr, nullptr, &m_reg_handle);
    m_registered = (status == ERROR_SUCCESS);
    
    if (!m_registered) {
        // Log error
        std::wcerr << L"Failed to register ETW provider: " << status << std::endl;
        return false;
    }
    
    return true;
}

void EtwTracingProvider::unregister_provider() {
    if (m_registered && m_reg_handle != 0) {
        EventUnregister(m_reg_handle);
        m_reg_handle = 0;
        m_registered = false;
    }
}

bool EtwTracingProvider::is_enabled(TraceLevel level, uint64_t keywords) const {
    if (!m_registered) {
        return false;
    }
    
    return EventEnabled(m_reg_handle, get_etw_level(level), 0) != FALSE;
}

void EtwTracingProvider::write_io_event(const IoEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::IO))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[10];
    
    // File ID low
    EventDataDescCreate(&data_descriptors[0], 
                       &event.file_id_low, 
                       sizeof(event.file_id_low));
    
    // File ID high
    EventDataDescCreate(&data_descriptors[1], 
                       &event.file_id_high, 
                       sizeof(event.file_id_high));
    
    // Volume ID
    EventDataDescCreate(&data_descriptors[2], 
                       &event.volume_id, 
                       sizeof(event.volume_id));
    
    // Offset
    EventDataDescCreate(&data_descriptors[3], 
                       &event.offset, 
                       sizeof(event.offset));
    
    // Size
    EventDataDescCreate(&data_descriptors[4], 
                       &event.size, 
                       sizeof(event.size));
    
    // Duration
    EventDataDescCreate(&data_descriptors[5], 
                       &event.duration_ms, 
                       sizeof(event.duration_ms));
    
    // Bytes transferred
    EventDataDescCreate(&data_descriptors[6], 
                       &event.bytes_transferred, 
                       sizeof(event.bytes_transferred));
    
    // Error code
    EventDataDescCreate(&data_descriptors[7], 
                       &event.error_code, 
                       sizeof(event.error_code));
    
    // File path (if provided)
    size_t file_path_length = event.file_path ? (wcslen(event.file_path) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[8], 
                       event.file_path, 
                       static_cast<ULONG>(file_path_length));
    
    // Null terminator for file path
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[9], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(1, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::IO),
                         data_descriptors, 10);
}

void EtwTracingProvider::write_hash_event(const HashEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::Hashing))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[9];
    
    // File ID low
    EventDataDescCreate(&data_descriptors[0], 
                       &event.file_id_low, 
                       sizeof(event.file_id_low));
    
    // File ID high
    EventDataDescCreate(&data_descriptors[1], 
                       &event.file_id_high, 
                       sizeof(event.file_id_high));
    
    // Volume ID
    EventDataDescCreate(&data_descriptors[2], 
                       &event.volume_id, 
                       sizeof(event.volume_id));
    
    // Algorithm
    EventDataDescCreate(&data_descriptors[3], 
                       &event.algorithm, 
                       sizeof(event.algorithm));
    
    // Data size
    EventDataDescCreate(&data_descriptors[4], 
                       &event.data_size, 
                       sizeof(event.data_size));
    
    // Duration
    EventDataDescCreate(&data_descriptors[5], 
                       &event.duration_ms, 
                       sizeof(event.duration_ms));
    
    // Hash rate
    EventDataDescCreate(&data_descriptors[6], 
                       &event.hash_rate_mbps, 
                       sizeof(event.hash_rate_mbps));
    
    // File path
    size_t file_path_length = event.file_path ? (wcslen(event.file_path) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[7], 
                       event.file_path, 
                       static_cast<ULONG>(file_path_length));
    
    // Null terminator
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[8], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(2, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::Hashing),
                         data_descriptors, 9);
}

void EtwTracingProvider::write_dedupe_event(const DedupeEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::Deduplication))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[8];
    
    // Group ID
    EventDataDescCreate(&data_descriptors[0], 
                       &event.group_id, 
                       sizeof(event.group_id));
    
    // File count
    EventDataDescCreate(&data_descriptors[1], 
                       &event.file_count, 
                       sizeof(event.file_count));
    
    // Total size
    EventDataDescCreate(&data_descriptors[2], 
                       &event.total_size, 
                       sizeof(event.total_size));
    
    // Potential savings
    EventDataDescCreate(&data_descriptors[3], 
                       &event.potential_savings, 
                       sizeof(event.potential_savings));
    
    // Duration
    EventDataDescCreate(&data_descriptors[4], 
                       &event.duration_ms, 
                       sizeof(event.duration_ms));
    
    // Duplicate count
    EventDataDescCreate(&data_descriptors[5], 
                       &event.duplicate_count, 
                       sizeof(event.duplicate_count));
    
    // Operation
    size_t operation_length = event.operation ? (wcslen(event.operation) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[6], 
                       event.operation, 
                       static_cast<ULONG>(operation_length));
    
    // Null terminator
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[7], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(3, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::Deduplication),
                         data_descriptors, 8);
}

void EtwTracingProvider::write_ui_event(const UiEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::UI))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[6];
    
    // Action
    size_t action_length = event.action ? (wcslen(event.action) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[0], 
                       event.action, 
                       static_cast<ULONG>(action_length));
    
    // Element
    size_t element_length = event.element ? (wcslen(event.element) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[1], 
                       event.element, 
                       static_cast<ULONG>(element_length));
    
    // X coordinate
    EventDataDescCreate(&data_descriptors[2], 
                       &event.x, 
                       sizeof(event.x));
    
    // Y coordinate
    EventDataDescCreate(&data_descriptors[3], 
                       &event.y, 
                       sizeof(event.y));
    
    // Duration
    EventDataDescCreate(&data_descriptors[4], 
                       &event.duration_ms, 
                       sizeof(event.duration_ms));
    
    // Null terminator
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[5], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(4, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::UI),
                         data_descriptors, 6);
}

void EtwTracingProvider::write_index_event(const IndexEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::Index))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[8];
    
    // Operation
    size_t operation_length = event.operation ? (wcslen(event.operation) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[0], 
                       event.operation, 
                       static_cast<ULONG>(operation_length));
    
    // Key volume ID
    EventDataDescCreate(&data_descriptors[1], 
                       &event.key_volume_id, 
                       sizeof(event.key_volume_id));
    
    // Key file ID low
    EventDataDescCreate(&data_descriptors[2], 
                       &event.key_file_id_low, 
                       sizeof(event.key_file_id_low));
    
    // Key file ID high
    EventDataDescCreate(&data_descriptors[3], 
                       &event.key_file_id_high, 
                       sizeof(event.key_file_id_high));
    
    // Duration
    EventDataDescCreate(&data_descriptors[4], 
                       &event.duration_ms, 
                       sizeof(event.duration_ms));
    
    // Result count
    EventDataDescCreate(&data_descriptors[5], 
                       &event.result_count, 
                       sizeof(event.result_count));
    
    // Table name
    size_t table_name_length = event.table_name ? (wcslen(event.table_name) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[6], 
                       event.table_name, 
                       static_cast<ULONG>(table_name_length));
    
    // Null terminator
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[7], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(5, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::Index),
                         data_descriptors, 8);
}

void EtwTracingProvider::write_performance_event(const PerformanceEvent& event) {
    if (!is_enabled(TraceLevel::Informational, static_cast<uint64_t>(Category::Performance))) {
        return;
    }
    
    // Prepare event data descriptors
    EVENT_DATA_DESCRIPTOR data_descriptors[5];
    
    // Metric name
    size_t metric_name_length = event.metric_name ? (wcslen(event.metric_name) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[0], 
                       event.metric_name, 
                       static_cast<ULONG>(metric_name_length));
    
    // Value
    EventDataDescCreate(&data_descriptors[1], 
                       &event.value, 
                       sizeof(event.value));
    
    // Unit
    size_t unit_length = event.unit ? (wcslen(event.unit) + 1) * sizeof(wchar_t) : 0;
    EventDataDescCreate(&data_descriptors[2], 
                       event.unit, 
                       static_cast<ULONG>(unit_length));
    
    // Timestamp
    EventDataDescCreate(&data_descriptors[3], 
                       &event.timestamp, 
                       sizeof(event.timestamp));
    
    // Null terminator
    static const wchar_t null_terminator = L'\0';
    EventDataDescCreate(&data_descriptors[4], 
                       &null_terminator, 
                       sizeof(null_terminator));
    
    write_event_internal(6, TraceLevel::Informational, 
                         static_cast<uint64_t>(Category::Performance),
                         data_descriptors, 5);
}

template<typename... Args>
void EtwTracingProvider::write_event(uint32_t event_id, TraceLevel level, uint64_t keywords,
                                     const wchar_t* message, Args... args) {
    if (!is_enabled(level, keywords)) {
        return;
    }
    
    // Format the message with arguments
    std::wstring formatted_message = format_message(message, args...);
    
    // Prepare event data descriptor
    EVENT_DATA_DESCRIPTOR data_descriptor;
    EventDataDescCreate(&data_descriptor, 
                       formatted_message.c_str(), 
                       static_cast<ULONG>((formatted_message.length() + 1) * sizeof(wchar_t)));
    
    write_event_internal(event_id, level, keywords, &data_descriptor, 1);
}

void EtwTracingProvider::write_event_internal(uint32_t event_id, TraceLevel level, uint64_t keywords,
                                             const EVENT_DATA_DESCRIPTOR* data_descriptors, uint32_t descriptor_count) {
    if (!m_registered) {
        return;
    }
    
    EVENT_DESCRIPTOR event_descriptor;
    event_descriptor.Id = event_id;
    event_descriptor.Version = 1;
    event_descriptor.Channel = 0;
    event_descriptor.Level = get_etw_level(level);
    event_descriptor.Opcode = 0; // Info
    event_descriptor.Task = 0;
    event_descriptor.Keyword = keywords;
    
    EventWrite(m_reg_handle, &event_descriptor, descriptor_count, data_descriptors);
}

ULONG EtwTracingProvider::get_etw_level(TraceLevel level) {
    switch (level) {
        case TraceLevel::Critical: return 1;
        case TraceLevel::Error: return 2;
        case TraceLevel::Warning: return 3;
        case TraceLevel::Informational: return 4;
        case TraceLevel::Verbose: return 5;
        default: return 4;
    }
}

USHORT EtwTracingProvider::get_etw_opcode(TraceOpcode opcode) {
    return static_cast<USHORT>(opcode);
}

template<typename... Args>
std::wstring EtwTracingProvider::format_message(const wchar_t* format, Args... args) {
    // This is a simplified implementation
    // In a real implementation, we would use proper formatting
    std::wostringstream oss;
    oss << format;
    // Add arguments (simplified)
    return oss.str();
}

// EtwTracingProvider::ScopedTimer implementation
EtwTracingProvider::ScopedTimer::ScopedTimer(EtwTracingProvider* provider, uint32_t event_id,
                                            TraceLevel level, uint64_t keywords)
    : m_provider(provider), m_event_id(event_id), m_level(level), m_keywords(keywords) {
    m_start_time = std::chrono::high_resolution_clock::now();
}

EtwTracingProvider::ScopedTimer::~ScopedTimer() {
    stop();
}

void EtwTracingProvider::ScopedTimer::stop() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_start_time);
    
    // In a real implementation, we would write a performance event
    // with the measured duration
}

// HighResolutionTimer implementation
HighResolutionTimer::HighResolutionTimer()
    : m_stopped(false) {
    start();
}

void HighResolutionTimer::start() {
    m_start_time = std::chrono::high_resolution_clock::now();
    m_stopped = false;
}

void HighResolutionTimer::stop() {
    if (!m_stopped) {
        m_end_time = std::chrono::high_resolution_clock::now();
        m_stopped = true;
    }
}

void HighResolutionTimer::reset() {
    start();
    m_stopped = false;
}

double HighResolutionTimer::get_elapsed_nanoseconds() const {
    auto end_time = m_stopped ? m_end_time : std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - m_start_time);
    return static_cast<double>(duration.count());
}

double HighResolutionTimer::get_elapsed_microseconds() const {
    return get_elapsed_nanoseconds() / 1000.0;
}

double HighResolutionTimer::get_elapsed_milliseconds() const {
    return get_elapsed_nanoseconds() / 1000000.0;
}

double HighResolutionTimer::get_elapsed_seconds() const {
    return get_elapsed_nanoseconds() / 1000000000.0;
}

uint64_t HighResolutionTimer::get_timestamp() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

double HighResolutionTimer::calculate_throughput_mbps(uint64_t bytes_processed) const {
    double elapsed_seconds = get_elapsed_seconds();
    if (elapsed_seconds > 0) {
        return (static_cast<double>(bytes_processed) / (1024.0 * 1024.0)) / elapsed_seconds;
    }
    return 0.0;
}

double HighResolutionTimer::calculate_ops_per_second(uint64_t operations_completed) const {
    double elapsed_seconds = get_elapsed_seconds();
    if (elapsed_seconds > 0) {
        return static_cast<double>(operations_completed) / elapsed_seconds;
    }
    return 0.0;
}

// PerformanceCounters implementation
PerformanceCounters::PerformanceCounters() {
    m_last_snapshot = take_snapshot();
    m_last_update = std::chrono::high_resolution_clock::now();
}

PerformanceCounters::CounterSnapshot PerformanceCounters::take_snapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    CounterSnapshot snapshot;
    snapshot.timestamp = HighResolutionTimer::get_timestamp();
    
    // These would be populated with actual system metrics
    // For now, we'll use dummy values
    snapshot.total_bytes_read = 0;
    snapshot.total_bytes_written = 0;
    snapshot.total_operations = 0;
    snapshot.total_errors = 0;
    snapshot.active_threads = std::thread::hardware_concurrency();
    snapshot.memory_usage_bytes = 0;
    snapshot.cpu_usage_percent = 0.0;
    snapshot.io_wait_time_ms = 0.0;
    
    return snapshot;
}

PerformanceCounters::CounterDeltas 
PerformanceCounters::calculate_deltas(const CounterSnapshot& current) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    CounterDeltas deltas;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_update);
    deltas.elapsed_seconds = elapsed.count() / 1000.0;
    
    if (deltas.elapsed_seconds > 0) {
        // Calculate rates
        deltas.read_throughput_mbps = 
            (static_cast<double>(current.total_bytes_read - m_last_snapshot.total_bytes_read) / 
             (1024.0 * 1024.0)) / deltas.elapsed_seconds;
            
        deltas.write_throughput_mbps = 
            (static_cast<double>(current.total_bytes_written - m_last_snapshot.total_bytes_written) / 
             (1024.0 * 1024.0)) / deltas.elapsed_seconds;
            
        deltas.operations_per_second = 
            static_cast<double>(current.total_operations - m_last_snapshot.total_operations) / 
            deltas.elapsed_seconds;
            
        deltas.error_rate_per_second = 
            static_cast<double>(current.total_errors - m_last_snapshot.total_errors) / 
            deltas.elapsed_seconds;
    }
    
    // Calculate changes
    deltas.thread_count_change = 
        static_cast<double>(current.active_threads) - static_cast<double>(m_last_snapshot.active_threads);
        
    deltas.memory_usage_change_mb = 
        (static_cast<double>(current.memory_usage_bytes) - static_cast<double>(m_last_snapshot.memory_usage_bytes)) / 
        (1024.0 * 1024.0);
        
    deltas.cpu_usage_change_percent = 
        current.cpu_usage_percent - m_last_snapshot.cpu_usage_percent;
        
    deltas.io_wait_time_change_ms = 
        current.io_wait_time_ms - m_last_snapshot.io_wait_time_ms;
    
    return deltas;
}

double PerformanceCounters::get_system_cpu_usage() {
    // In a real implementation, this would query system performance counters
    return 0.0;
}

uint64_t PerformanceCounters::get_process_memory_usage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

uint64_t PerformanceCounters::get_available_memory() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
}

double PerformanceCounters::get_disk_io_utilization() {
    // In a real implementation, this would query disk performance counters
    return 0.0;
}

std::vector<PerformanceCounters::PerformanceAlert> 
PerformanceCounters::check_performance_alerts(const CounterSnapshot& snapshot) const {
    std::vector<PerformanceAlert> alerts;
    
    // Check CPU usage
    if (snapshot.cpu_usage_percent > 90.0) {
        alerts.emplace_back(PerformanceAlert::Type::HighCpuUsage, 90.0, snapshot.cpu_usage_percent,
                           L"High CPU usage detected"));
    }
    
    // Check memory usage
    uint64_t available_memory = get_available_memory();
    if (available_memory < 100 * 1024 * 1024) { // Less than 100MB available
        alerts.emplace_back(PerformanceAlert::Type::LowMemory, 100.0, 
                           static_cast<double>(available_memory / (1024 * 1024)),
                           L"Low available memory"));
    }
    
    // Check error rate
    // This would be calculated based on error rate per second
    
    return alerts;
}

uint64_t PerformanceCounters::get_process_id() {
    return GetCurrentProcessId();
}

double PerformanceCounters::calculate_percentage_change(double old_value, double new_value) {
    if (old_value == 0.0) {
        return new_value > 0.0 ? 100.0 : 0.0;
    }
    return ((new_value - old_value) / old_value) * 100.0;
}

// LatencyHistogram implementation
template<size_t BucketCount>
LatencyHistogram<BucketCount>::LatencyHistogram(double max_expected_value)
    : m_bucket_width(max_expected_value / static_cast<double>(BucketCount - 1)) {
    clear();
}

template<size_t BucketCount>
void LatencyHistogram<BucketCount>::record(double value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_data.total_count == 0) {
        m_data.min_value = value;
        m_data.max_value = value;
    } else {
        m_data.min_value = std::min(m_data.min_value, value);
        m_data.max_value = std::max(m_data.max_value, value);
    }
    
    m_data.sum += value;
    m_data.total_count++;
    
    size_t bucket_index = get_bucket_index(value);
    if (bucket_index < BucketCount) {
        m_data.buckets[bucket_index]++;
    }
}

template<size_t BucketCount>
typename LatencyHistogram<BucketCount>::HistogramData 
LatencyHistogram<BucketCount>::get_data() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data;
}

template<size_t BucketCount>
typename LatencyHistogram<BucketCount>::Percentiles 
LatencyHistogram<BucketCount>::calculate_percentiles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_data.total_count == 0) {
        return Percentiles();
    }
    
    Percentiles percentiles;
    
    // Calculate positions for each percentile
    uint64_t p50_pos = static_cast<uint64_t>(m_data.total_count * 0.5);
    uint64_t p90_pos = static_cast<uint64_t>(m_data.total_count * 0.9);
    uint64_t p95_pos = static_cast<uint64_t>(m_data.total_count * 0.95);
    uint64_t p99_pos = static_cast<uint64_t>(m_data.total_count * 0.99);
    uint64_t p999_pos = static_cast<uint64_t>(m_data.total_count * 0.999);
    
    // Find values at each position
    uint64_t cumulative_count = 0;
    double* targets[] = {&percentiles.p50, &percentiles.p90, &percentiles.p95, 
                        &percentiles.p99, &percentiles.p999};
    uint64_t positions[] = {p50_pos, p90_pos, p95_pos, p99_pos, p999_pos};
    
    for (size_t i = 0; i < BucketCount && cumulative_count < positions[4]; ++i) {
        cumulative_count += m_data.buckets[i];
        
        for (size_t j = 0; j < 5; ++j) {
            if (cumulative_count >= positions[j] && *targets[j] == 0.0) {
                *targets[j] = static_cast<double>(i) * m_bucket_width;
            }
        }
    }
    
    return percentiles;
}

template<size_t BucketCount>
void LatencyHistogram<BucketCount>::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    memset(&m_data, 0, sizeof(m_data));
}

template<size_t BucketCount>
void LatencyHistogram<BucketCount>::merge(const HistogramData& other) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_data.total_count += other.total_count;
    m_data.sum += other.sum;
    
    if (other.total_count > 0) {
        if (m_data.total_count == other.total_count) {
            // First merge
            m_data.min_value = other.min_value;
            m_data.max_value = other.max_value;
        } else {
            m_data.min_value = std::min(m_data.min_value, other.min_value);
            m_data.max_value = std::max(m_data.max_value, other.max_value);
        }
    }
    
    for (size_t i = 0; i < BucketCount; ++i) {
        m_data.buckets[i] += other.buckets[i];
    }
}

template<size_t BucketCount>
size_t LatencyHistogram<BucketCount>::get_bucket_index(double value) const {
    if (value < 0.0) return 0;
    size_t index = static_cast<size_t>(value / m_bucket_width);
    return std::min(index, BucketCount - 1);
}

// Explicit template instantiation
template class LatencyHistogram<100>;

// TraceLogger implementation
TraceLogger::TraceLogger(OutputMode mode)
    : m_output_mode(mode) {
    // Initialize with default log file path
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    m_log_file_path = std::wstring(temp_path) + L"DiskSense64.log";
}

TraceLogger::~TraceLogger() {
    // Ensure all logs are flushed
}

bool TraceLogger::initialize_etw(const wchar_t* provider_name) {
    if (m_output_mode == OutputMode::FILE_ONLY) {
        return true; // No ETW needed
    }
    
    m_etw_provider = std::make_unique<EtwTracingProvider>();
    return m_etw_provider->register_provider(provider_name);
}

void TraceLogger::log_critical(const std::wstring& category, const std::wstring& message,
                              const wchar_t* module, uint32_t line) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Critical;
    entry.category = category;
    entry.message = message;
    entry.module = module ? module : L"";
    entry.line_number = line;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_error(const std::wstring& category, const std::wstring& message,
                           const wchar_t* module, uint32_t line) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Error;
    entry.category = category;
    entry.message = message;
    entry.module = module ? module : L"";
    entry.line_number = line;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_warning(const std::wstring& category, const std::wstring& message,
                             const wchar_t* module, uint32_t line) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Warning;
    entry.category = category;
    entry.message = message;
    entry.module = module ? module : L"";
    entry.line_number = line;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_info(const std::wstring& category, const std::wstring& message,
                          const wchar_t* module, uint32_t line) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Informational;
    entry.category = category;
    entry.message = message;
    entry.module = module ? module : L"";
    entry.line_number = line;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_verbose(const std::wstring& category, const std::wstring& message,
                             const wchar_t* module, uint32_t line) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Verbose;
    entry.category = category;
    entry.message = message;
    entry.module = module ? module : L"";
    entry.line_number = line;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_performance(const std::wstring& metric_name, double value,
                                 const std::wstring& unit) {
    LogEntry entry;
    entry.timestamp = HighResolutionTimer::get_timestamp();
    entry.level = TraceLevel::Informational;
    entry.category = L"PERFORMANCE";
    entry.message = metric_name + L" = " + std::to_wstring(value) + L" " + unit;
    entry.module = L"";
    entry.line_number = 0;
    
    if (m_output_mode == OutputMode::ETW_ONLY || m_output_mode == OutputMode::BOTH) {
        if (m_etw_provider) {
            // Write to ETW
        }
    }
    
    if (m_output_mode == OutputMode::FILE_ONLY || m_output_mode == OutputMode::BOTH) {
        write_to_file(entry);
    }
}

void TraceLogger::log_batch(const std::vector<LogEntry>& entries) {
    for (const auto& entry : entries) {
        switch (entry.level) {
            case TraceLevel::Critical:
                log_critical(entry.category, entry.message, entry.module.c_str(), entry.line_number);
                break;
            case TraceLevel::Error:
                log_error(entry.category, entry.message, entry.module.c_str(), entry.line_number);
                break;
            case TraceLevel::Warning:
                log_warning(entry.category, entry.message, entry.module.c_str(), entry.line_number);
                break;
            case TraceLevel::Informational:
                log_info(entry.category, entry.message, entry.module.c_str(), entry.line_number);
                break;
            case TraceLevel::Verbose:
                log_verbose(entry.category, entry.message, entry.module.c_str(), entry.line_number);
                break;
        }
    }
}

bool TraceLogger::rotate_log_file() {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    
    // Check if file exists and get its size
    WIN32_FILE_ATTRIBUTE_DATA file_attr;
    if (GetFileAttributesExW(m_log_file_path.c_str(), GetFileExInfoStandard, &file_attr)) {
        ULARGE_INTEGER file_size;
        file_size.LowPart = file_attr.nFileSizeLow;
        file_size.HighPart = file_attr.nFileSizeHigh;
        
        // Rotate if file is larger than 10MB
        if (file_size.QuadPart > 10 * 1024 * 1024) {
            std::wstring backup_path = m_log_file_path + L".old";
            
            // Delete old backup if it exists
            DeleteFileW(backup_path.c_str());
            
            // Rename current log to backup
            if (MoveFileW(m_log_file_path.c_str(), backup_path.c_str())) {
                // Create new log file
                return true;
            }
        }
    }
    
    return false;
}

bool TraceLogger::compress_old_logs() {
    // In a real implementation, this would compress old log files
    return true;
}

std::vector<TraceLogger::LogEntry> TraceLogger::read_recent_entries(size_t count) const {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    std::vector<LogEntry> entries;
    
    // In a real implementation, this would read the last 'count' entries from the log file
    return entries;
}

void TraceLogger::write_to_file(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_file_mutex);
    
    std::wofstream log_file(m_log_file_path, std::ios_base::app);
    if (log_file.is_open()) {
        log_file << format_log_entry(entry) << std::endl;
        log_file.close();
    }
}

std::wstring TraceLogger::format_log_entry(const LogEntry& entry) const {
    std::wostringstream oss;
    
    // Timestamp
    oss << get_timestamp_string(entry.timestamp) << L" ";
    
    // Level
    switch (entry.level) {
        case TraceLevel::Critical: oss << L"[CRITICAL] "; break;
        case TraceLevel::Error: oss << L"[ERROR] "; break;
        case TraceLevel::Warning: oss << L"[WARNING] "; break;
        case TraceLevel::Informational: oss << L"[INFO] "; break;
        case TraceLevel::Verbose: oss << L"[VERBOSE] "; break;
    }
    
    // Category
    oss << L"[" << entry.category << L"] ";
    
    // Module and line (if available)
    if (!entry.module.empty()) {
        size_t last_slash = entry.module.find_last_of(L"\\/");
        std::wstring filename = (last_slash != std::wstring::npos) ? 
                               entry.module.substr(last_slash + 1) : entry.module;
        oss << L"(" << filename << L":" << entry.line_number << L") ";
    }
    
    // Message
    oss << entry.message;
    
    return oss.str();
}

std::wstring TraceLogger::get_timestamp_string(uint64_t timestamp) const {
    // Convert timestamp to readable format
    time_t time_value = static_cast<time_t>(timestamp / 1000000000);
    tm time_info;
    localtime_s(&time_info, &time_value);
    
    wchar_t buffer[32];
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d", 
                time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
                time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    
    return std::wstring(buffer);
}

// Create a global trace logger instance
namespace {
    struct TraceLoggerInitializer {
        TraceLoggerInitializer() {
            g_trace_logger = std::make_unique<TraceLogger>();
            g_trace_logger->initialize_etw();
        }
        
        ~TraceLoggerInitializer() {
            g_trace_logger.reset();
        }
    };
    
    TraceLoggerInitializer g_initializer;
}