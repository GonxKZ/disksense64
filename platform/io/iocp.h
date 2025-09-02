#ifndef PLATFORM_IO_IO_COMPLETION_PORT_H
#define PLATFORM_IO_IO_COMPLETION_PORT_H

#include <windows.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

// Forward declarations
struct IoRequest;
class IoScheduler;

// IO Completion Port wrapper
class IOCompletionPort {
private:
    HANDLE m_port;
    std::atomic<bool> m_running;
    
public:
    IOCompletionPort();
    ~IOCompletionPort();
    
    // Create completion port
    bool create();
    
    // Associate file handle with completion port
    bool associate_handle(HANDLE file_handle, ULONG_PTR completion_key = 0);
    
    // Post completion status
    bool post_completion(DWORD bytes_transferred, ULONG_PTR completion_key, LPOVERLAPPED overlapped);
    
    // Get completion status
    bool get_completion(DWORD* bytes_transferred, PULONG_PTR completion_key, 
                       LPOVERLAPPED* overlapped, DWORD timeout_ms = INFINITE);
    
    // Close completion port
    void close();
    
    // Check if port is valid
    bool is_valid() const { return m_port != INVALID_HANDLE_VALUE && m_port != nullptr; }
};

// IO Scheduler with autocalibration
class IoScheduler {
public:
    struct PerformanceProfile {
        size_t optimal_concurrent_ops;
        size_t optimal_buffer_size;
        double max_throughput_mbps;
        double p95_latency_ms;
        DWORD device_type; // FILE_DEVICE_DISK, etc.
        
        PerformanceProfile() 
            : optimal_concurrent_ops(4), optimal_buffer_size(1024*1024),
              max_throughput_mbps(0.0), p95_latency_ms(25.0), device_type(0) {}
    };
    
    struct CalibrationResult {
        size_t best_n_concurrent;
        size_t best_buffer_size;
        double throughput_mbps;
        double p95_latency_ms;
        bool calibration_successful;
        
        CalibrationResult() 
            : best_n_concurrent(0), best_buffer_size(0),
              throughput_mbps(0.0), p95_latency_ms(0.0), 
              calibration_successful(false) {}
    };
    
private:
    std::unique_ptr<IOCompletionPort> m_iocp;
    std::vector<std::thread> m_worker_threads;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_current_concurrent_ops;
    
    // Performance metrics
    std::atomic<uint64_t> m_total_bytes_read;
    std::atomic<uint64_t> m_total_operations;
    std::atomic<uint64_t> m_failed_operations;
    
    // Latency tracking (simple circular buffer for percentiles)
    static constexpr size_t LATENCY_BUFFER_SIZE = 10000;
    std::vector<double> m_latency_samples;
    std::atomic<size_t> m_latency_index;
    mutable std::mutex m_metrics_mutex;
    
    // Device information
    wchar_t m_volume_path[MAX_PATH];
    PerformanceProfile m_profile;
    
public:
    IoScheduler();
    ~IoScheduler();
    
    // Initialize scheduler for specific volume
    bool initialize(const wchar_t* volume_path);
    
    // Autocalibrate I/O parameters
    CalibrationResult autocalibrate();
    
    // Load profile from cache
    bool load_cached_profile();
    
    // Save profile to cache
    bool save_profile();
    
    // Submit read operation
    bool submit_read(HANDLE file_handle, void* buffer, size_t size, uint64_t offset);
    
    // Process completions
    void process_completions();
    
    // Get performance statistics
    struct Stats {
        uint64_t total_bytes_read;
        uint64_t total_operations;
        uint64_t failed_operations;
        double current_throughput_mbps;
        double p50_latency_ms;
        double p95_latency_ms;
        double p99_latency_ms;
        size_t current_concurrent_ops;
    };
    
    Stats get_stats() const;
    
    // Get performance profile
    const PerformanceProfile& get_profile() const { return m_profile; }
    
    // Set performance limits
    void set_latency_limit(double p95_limit_ms) { m_profile.p95_latency_ms = p95_limit_ms; }
    
private:
    // Worker thread function
    void worker_thread();
    
    // Measure performance for specific parameters
    std::pair<double, double> measure_performance(size_t concurrent_ops, size_t buffer_size);
    
    // Update latency sample
    void record_latency(double latency_ms);
    
    // Calculate percentiles from latency samples
    struct Percentiles {
        double p50, p95, p99;
    };
    
    Percentiles calculate_percentiles() const;
};

// IO Request structure
struct IoRequest {
    OVERLAPPED overlapped;
    uint64_t file_offset;
    size_t buffer_size;
    void* buffer;
    std::chrono::high_resolution_clock::time_point start_time;
    
    IoRequest() : file_offset(0), buffer_size(0), buffer(nullptr) {
        ZeroMemory(&overlapped, sizeof(overlapped));
    }
};

#endif // PLATFORM_IO_IO_COMPLETION_PORT_H