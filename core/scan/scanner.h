#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <atomic>
#include "core/model/model.h"
#include "libs/utils/utils.h"

// Forward declarations
struct file_info_t;

// Scan event types
enum class ScanEventType {
    FileAdded,
    FileUpdated,
    FileRemoved
};

// Scan event structure
struct ScanEvent {
    ScanEventType type;
    FileEntry fileEntry;
    
    ScanEvent(ScanEventType t, const FileEntry& entry) : type(t), fileEntry(entry) {}
};

// Scan options
struct ScanOptions {
    bool useMftReader = false;        // Use MFT reader for faster scanning (requires admin)
    bool followReparsePoints = false; // Follow junctions and symlinks
    bool computeHeadTail = true;      // Compute head/tail signatures
    bool computeFullHash = false;     // Compute full file hash (expensive)
    std::vector<std::wstring> excludePaths; // Paths to exclude from scanning
};

// Scanner interface
class Scanner {
public:
    Scanner();
    ~Scanner();
    
    // Scan a volume path and return events
    void scanVolume(const std::wstring& volumePath, 
                   const ScanOptions& options,
                   std::function<void(const ScanEvent&)> callback);
    
    // Cancel ongoing scan
    void cancel();
    
    // Check if scan is in progress
    bool isScanning() const;
    
private:
    std::atomic<bool> m_cancelled;
    std::atomic<bool> m_scanning;
    
    // Scan a directory recursively
    void scanDirectory(const std::string& path,
                      const ScanOptions& options,
                      std::function<void(const ScanEvent&)> callback);
    
    // Process a single file
    void processFile(const std::string& path,
                    const file_info_t& file_info,
                    const ScanOptions& options,
                    std::function<void(const ScanEvent&)> callback);
    
    // Compute head/tail signature
    std::vector<uint8_t> computeHeadTailSignature(const std::string& path);
    
    // Compute full file hash
    std::vector<uint8_t> computeFullHash(const std::string& path);
    
    // Check if path is excluded
    bool isExcludedPath(const std::string& path, const ScanOptions& options);
};