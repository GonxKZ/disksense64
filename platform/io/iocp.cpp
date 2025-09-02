#include "iocp.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

// IOCompletionPort implementation
IOCompletionPort::IOCompletionPort() 
    : m_port(INVALID_HANDLE_VALUE), m_running(false) {
}

IOCompletionPort::~IOCompletionPort() {
    close();
}

bool IOCompletionPort::create() {
    close();
    
    m_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    return is_valid();
}

bool IOCompletionPort::associate_handle(HANDLE file_handle, ULONG_PTR completion_key) {
    if (!is_valid() || file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    return CreateIoCompletionPort(file_handle, m_port, completion_key, 0) == m_port;
}

bool IOCompletionPort::post_completion(DWORD bytes_transferred, ULONG_PTR completion_key, LPOVERLAPPED overlapped) {
    if (!is_valid()) {
        return false;
    }
    
    return PostQueuedCompletionStatus(m_port, bytes_transferred, completion_key, overlapped);
}

bool IOCompletionPort::get_completion(DWORD* bytes_transferred, PULONG_PTR completion_key, 
                                     LPOVERLAPPED* overlapped, DWORD timeout_ms) {
    if (!is_valid() || !bytes_transferred || !completion_key || !overlapped) {
        return false;
    }
    
    return GetQueuedCompletionStatus(m_port, bytes_transferred, completion_key, overlapped, timeout_ms);
}

void IOCompletionPort::close() {
    if (is_valid()) {
        CloseHandle(m_port);
        m_port = INVALID_HANDLE_VALUE;
    }
    m_running = false;
}

// IoScheduler implementation
IoScheduler::IoScheduler() 
    : m_running(false), m_current_concurrent_ops(0),
      m_total_bytes_read(0), m_total_operations(0), m_failed_operations(0),
      m_latency_index(0) {
    m_iocp = std::make_unique<IOCompletionPort>();
    m_latency_samples.resize(LATENCY_BUFFER_SIZE, 0.0);
}

IoScheduler::~IoScheduler() {
    // Stop worker threads
    m_running = false;
    
    // Wake up any waiting threads
    if (m_iocp && m_iocp->is_valid()) {
        m_iocp->post_completion(0, 0, nullptr);
    }
    
    // Wait for worker threads to finish
    for (auto& thread : m_worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

bool IoScheduler::initialize(const wchar_t* volume_path) {
    if (!volume_path) {
        return false;
    }
    
    // Copy volume path
    wcscpy_s(m_volume_path, volume_path);
    
    // Create IO completion port
    if (!m_iocp->create()) {
        return false;
    }
    
    // Try to load cached profile
    load_cached_profile();
    
    // Set initial concurrent operations
    m_current_concurrent_ops = m_profile.optimal_concurrent_ops;
    
    // Start worker threads
    m_running = true;
    size_t thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 4;
    
    for (size_t i = 0; i < thread_count; ++i) {
        m_worker_threads.emplace_back(&IoScheduler::worker_thread, this);
    }
    
    return true;
}

IoScheduler::CalibrationResult IoScheduler::autocalibrate() {
    CalibrationResult result;
    
    std::cout << "Starting IO autocalibration for volume: " << m_volume_path << std::endl;
    
    // Test parameters
    std::vector<size_t> test_concurrent_ops = {1, 2, 4, 8, 16, 32, 64};
    std::vector<size_t> test_buffer_sizes = {
        256 * 1024,     // 256 KB
        512 * 1024,     // 512 KB
        1024 * 1024,    // 1 MB
        2 * 1024 * 1024, // 2 MB
        4 * 1024 * 1024  // 4 MB
    };
    
    double best_throughput = 0.0;
    size_t best_n = 4;
    size_t best_buffer = 1024 * 1024;
    
    // Warm-up phase
    std::cout << "Warm-up phase..." << std::endl;
    measure_performance(4, 1024 * 1024);
    
    // Test different combinations
    for (size_t n : test_concurrent_ops) {
        for (size_t buffer_size : test_buffer_sizes) {
            auto [throughput, latency_p95] = measure_performance(n, buffer_size);
            
            std::cout << "N=" << n << ", Buffer=" << buffer_size/1024 << "KB -> "
                      << throughput << " MB/s, P95=" << latency_p95 << "ms" << std::endl;
            
            // Choose best combination that meets latency requirements
            if (throughput > best_throughput && latency_p95 <= m_profile.p95_latency_ms) {
                best_throughput = throughput;
                best_n = n;
                best_buffer = buffer_size;
            }
        }
    }
    
    result.best_n_concurrent = best_n;
    result.best_buffer_size = best_buffer;
    result.throughput_mbps = best_throughput;
    result.calibration_successful = true;
    
    // Update profile
    m_profile.optimal_concurrent_ops = best_n;
    m_profile.optimal_buffer_size = best_buffer;
    m_profile.max_throughput_mbps = best_throughput;
    
    // Get final latency measurements
    auto [_, latency_p95] = measure_performance(best_n, best_buffer);
    result.p95_latency_ms = latency_p95;
    
    std::cout << "Autocalibration complete. Optimal: N=" << best_n 
              << ", Buffer=" << best_buffer/1024 << "KB, Throughput=" 
              << best_throughput << " MB/s" << std::endl;
    
    return result;
}

bool IoScheduler::load_cached_profile() {
    // In a real implementation, this would load from a cache file
    // For now, we'll use default values
    m_profile.optimal_concurrent_ops = 4;
    m_profile.optimal_buffer_size = 1024 * 1024; // 1MB
    m_profile.p95_latency_ms = 25.0;
    
    return true;
}

bool IoScheduler::save_profile() {
    // In a real implementation, this would save to a cache file
    return true;
}

bool IoScheduler::submit_read(HANDLE file_handle, void* buffer, size_t size, uint64_t offset) {
    if (!m_iocp || !m_iocp->is_valid() || !file_handle || !buffer) {
        return false;
    }
    
    // Associate file handle if not already done
    if (!m_iocp->associate_handle(file_handle, reinterpret_cast<ULONG_PTR>(file_handle))) {
        return false;
    }
    
    // Create OVERLAPPED structure
    auto* request = new IoRequest();
    request->file_offset = offset;
    request->buffer_size = size;
    request->buffer = buffer;
    request->start_time = std::chrono::high_resolution_clock::now();
    
    request->overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
    request->overlapped.OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFF);
    
    // Submit async read
    BOOL result = ReadFile(file_handle, buffer, static_cast<DWORD>(size), 
                          nullptr, &request->overlapped);
    
    if (!result && GetLastError() != ERROR_IO_PENDING) {
        delete request;
        return false;
    }
    
    m_total_operations.fetch_add(1);
    return true;
}

void IoScheduler::process_completions() {
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    LPOVERLAPPED overlapped;
    
    while (m_running) {
        if (m_iocp->get_completion(&bytes_transferred, &completion_key, &overlapped, 1000)) {
            if (overlapped) {
                // Calculate latency
                auto* request = reinterpret_cast<IoRequest*>(overlapped);
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - request->start_time);
                double latency_ms = duration.count() / 1000.0;
                
                record_latency(latency_ms);
                m_total_bytes_read.fetch_add(bytes_transferred);
                
                delete request;
            }
        }
    }
}

IoScheduler::Stats IoScheduler::get_stats() const {
    Stats stats;
    stats.total_bytes_read = m_total_bytes_read.load();
    stats.total_operations = m_total_operations.load();
    stats.failed_operations = m_failed_operations.load();
    stats.current_concurrent_ops = m_current_concurrent_ops.load();
    
    // Calculate current throughput (last 5 seconds approx)
    static uint64_t last_bytes = 0;
    static auto last_time = std::chrono::steady_clock::now();
    static double last_throughput = 0.0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
    
    if (elapsed_ms > 1000) { // Update every second
        uint64_t current_bytes = m_total_bytes_read.load();
        uint64_t bytes_delta = current_bytes - last_bytes;
        
        last_throughput = (bytes_delta / (1024.0 * 1024.0)) / (elapsed_ms / 1000.0);
        last_bytes = current_bytes;
        last_time = now;
    }
    
    stats.current_throughput_mbps = last_throughput;
    
    // Calculate percentiles
    auto percentiles = calculate_percentiles();
    stats.p50_latency_ms = percentiles.p50;
    stats.p95_latency_ms = percentiles.p95;
    stats.p99_latency_ms = percentiles.p99;
    
    return stats;
}

void IoScheduler::worker_thread() {
    process_completions();
}

std::pair<double, double> IoScheduler::measure_performance(size_t concurrent_ops, size_t buffer_size) {
    // This is a simplified measurement
    // In a real implementation, we would actually perform I/O operations
    
    // Simulate I/O operations
    size_t num_ops = 1000;
    std::vector<double> latencies;
    latencies.reserve(num_ops);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t total_bytes = 0;
    
    // Simulate I/O operations with some variance
    for (size_t i = 0; i < num_ops; ++i) {
        // Simulate varying I/O times
        double base_latency = 1.0 + (rand() % 20); // 1-20ms base
        double variance = (rand() % 10) - 5; // -5 to +5ms
        double latency = base_latency + variance;
        
        if (latency < 0.1) latency = 0.1; // Minimum latency
        
        latencies.push_back(latency);
        total_bytes += buffer_size;
        
        // Simulate some I/O delay
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(latency * 1000)));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Calculate throughput
    double elapsed_seconds = duration.count() / 1000.0;
    double throughput_mbps = (total_bytes / (1024.0 * 1024.0)) / elapsed_seconds;
    
    // Calculate P95 latency
    std::sort(latencies.begin(), latencies.end());
    size_t p95_index = static_cast<size_t>(latencies.size() * 0.95);
    double p95_latency = latencies[p95_index];
    
    return {throughput_mbps, p95_latency};
}

void IoScheduler::record_latency(double latency_ms) {
    size_t index = m_latency_index.fetch_add(1) % LATENCY_BUFFER_SIZE;
    m_latency_samples[index] = latency_ms;
}

IoScheduler::Percentiles IoScheduler::calculate_percentiles() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    
    // Copy current samples
    std::vector<double> samples;
    size_t current_index = m_latency_index.load() % LATENCY_BUFFER_SIZE;
    size_t valid_samples = std::min(m_latency_index.load(), LATENCY_BUFFER_SIZE);
    
    samples.reserve(valid_samples);
    
    for (size_t i = 0; i < valid_samples; ++i) {
        size_t index = (current_index + i) % LATENCY_BUFFER_SIZE;
        if (m_latency_samples[index] > 0) {
            samples.push_back(m_latency_samples[index]);
        }
    }
    
    if (samples.empty()) {
        return {0.0, 0.0, 0.0};
    }
    
    std::sort(samples.begin(), samples.end());
    
    Percentiles percentiles;
    size_t size = samples.size();
    
    percentiles.p50 = samples[size / 2];
    percentiles.p95 = samples[(size * 95) / 100];
    percentiles.p99 = samples[(size * 99) / 100];
    
    return percentiles;
}