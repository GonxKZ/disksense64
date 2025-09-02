#ifndef CORE_ENGINE_IOCP_H
#define CORE_ENGINE_IOCP_H

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

// Forward declarations
struct IoContext;

// IO Completion Port class (simplified for cross-compilation)
class IOCompletionPort {
public:
    explicit IOCompletionPort(size_t concurrency = 0);
    ~IOCompletionPort();
    
    // Delete copy constructor and assignment operator
    IOCompletionPort(const IOCompletionPort&) = delete;
    IOCompletionPort& operator=(const IOCompletionPort&) = delete;
    
    // Move constructor and assignment
    IOCompletionPort(IOCompletionPort&& other) noexcept;
    IOCompletionPort& operator=(IOCompletionPort&& other) noexcept;
    
    // Stop processing completions
    void stop();
    
private:
    std::atomic<bool> m_running;
};

// I/O scheduler for adaptive concurrency control (simplified)
class IOScheduler {
public:
    struct Stats {
        uint64_t totalOperations;
        uint64_t completedOperations;
        uint64_t failedOperations;
        double avgLatencyMs;
        double p50LatencyMs;
        double p95LatencyMs;
        size_t currentConcurrency;
        size_t maxConcurrency;
    };
    
    IOScheduler(size_t initialConcurrency = 4, size_t maxConcurrency = 64);
    ~IOScheduler();
    
    // Start the scheduler
    void start();
    
    // Stop the scheduler
    void stop();
    
    // Get current statistics
    Stats getStats() const;
    
private:
    std::unique_ptr<IOCompletionPort> m_ioPort;
    std::vector<std::thread> m_workerThreads;
    std::atomic<bool> m_running;
    mutable std::mutex m_statsMutex;
    Stats m_stats;
    
    size_t m_currentConcurrency;
    size_t m_maxConcurrency;
    size_t m_minConcurrency;
};

#endif // CORE_ENGINE_IOCP_H