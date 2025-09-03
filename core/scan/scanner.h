#ifndef CORE_SCAN_SCANNER_H
#define CORE_SCAN_SCANNER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <thread>
#include "core/model/model.h"

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
    std::vector<std::string> excludePaths; // Paths to exclude from scanning
    uint64_t minFileSize = 0;         // Minimum file size to scan
    uint64_t maxFileSize = 0;         // Maximum file size to scan (0 = unlimited)
    std::vector<std::string> includeExtensions; // File extensions to include (* = all)
    std::vector<std::string> excludeExtensions; // File extensions to exclude
};

// Scanner interface
class Scanner {
public:
    Scanner();
    ~Scanner();

    // Scan a volume path and return events
    void scanVolume(const std::string& volumePath,
                   const ScanOptions& options,
                   std::function<void(const ScanEvent&)> callback);

    // Cancel ongoing scan
    void cancel();

    // Check if scan is in progress
    bool isScanning() const;

private:
    std::atomic<bool> m_cancelled;
    std::atomic<bool> m_scanning;

    // Process a directory recursively
    void scanDirectory(const std::string& path,
                      const ScanOptions& options,
                      std::function<void(const ScanEvent&)> callback);

    // Process a single file
    void processFile(const std::string& path,
                    uint64_t fileSize,
                    const ScanOptions& options,
                    std::function<void(const ScanEvent&)> callback);

    // Compute head/tail signature
    std::vector<uint8_t> computeHeadTailSignature(const std::string& path);

    // Compute full file hash
    std::vector<uint8_t> computeFullHash(const std::string& path);

    // Extract file ID from handle
    FileId extractFileId(const std::string& path);

    // Check if path should be excluded
    bool isExcludedPath(const std::string& path, const ScanOptions& options);

    // Check file extension filters
    bool matchesExtension(const std::string& path, const ScanOptions& options);
};

#endif // CORE_SCAN_SCANNER_H
