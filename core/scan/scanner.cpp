#include "scanner.h"
#include <iostream>
#include <algorithm>
#include <stack>
#include "core/model/model.h"
#include "libs/chash/sha256.h"
#include "libs/chash/blake3.h"
#include "libs/utils/utils.h"

Scanner::Scanner() : m_cancelled(false), m_scanning(false) {
}

Scanner::~Scanner() {
    cancel();
}

void Scanner::scanVolume(const std::wstring& volumePath, 
                        const ScanOptions& options,
                        std::function<void(const ScanEvent&)> callback) {
    if (m_scanning.exchange(true)) {
        return; // Already scanning
    }
    
    m_cancelled = false;
    
    // Convert wide string to platform-specific string
    std::string path = FileUtils::to_platform_path(StringUtils::to_utf8_string(volumePath));
    
    // Scan the directory
    scanDirectory(path, options, callback);
    
    m_scanning = false;
}

void Scanner::cancel() {
    m_cancelled = true;
}

bool Scanner::isScanning() const {
    return m_scanning;
}

void Scanner::scanDirectory(const std::string& path,
                           const ScanOptions& options,
                           std::function<void(const ScanEvent&)> callback) {
    if (m_cancelled) return;
    
    // Get directory contents
    auto files = FileUtils::list_directory(path);
    
    for (const auto& file_info : files) {
        if (m_cancelled) break;
        
        // Skip . and ..
        if (file_info.name == "." || file_info.name == "..") {
            continue;
        }
        
        std::string full_path = FileUtils::join_paths(path, file_info.name);
        
        // Check if path should be excluded
        if (isExcludedPath(full_path, options)) {
            continue;
        }
        
        if (file_info.is_directory) {
            // Recursively scan subdirectory
            scanDirectory(full_path, options, callback);
        } else {
            // Process file
            processFile(full_path, file_info, options, callback);
        }
    }
}

void Scanner::processFile(const std::string& path,
                         const file_info_t& file_info,
                         const ScanOptions& options,
                         std::function<void(const ScanEvent&)> callback) {
    FileEntry entry;
    
    // File identification
    entry.volumeId = 1; // Simplified for cross-platform
    entry.fileId = std::hash<std::string>{}(path);
    entry.pathId = std::hash<std::string>{}(path);
    
    // File size
    entry.sizeLogical = file_info.size;
    entry.sizeOnDisk = (file_info.size + 4095) & ~4095; // Assume 4KB clusters
    
    // File attributes
    entry.attributes.readOnly = false;
    entry.attributes.hidden = false;
    entry.attributes.system = false;
    entry.attributes.directory = false;
    entry.attributes.archive = true;
    entry.attributes.temporary = false;
    entry.attributes.sparse = false;
    entry.attributes.reparsePoint = false;
    entry.attributes.compressed = false;
    entry.attributes.encrypted = false;
    entry.attributes.offline = false;
    entry.attributes.notContentIndexed = false;
    entry.attributes.virtualFile = false;
    
#ifdef _WIN32
    entry.attributes.readOnly = (file_info.attributes & 0x00000001) != 0;
    entry.attributes.hidden = (file_info.attributes & 0x00000002) != 0;
    entry.attributes.system = (file_info.attributes & 0x00000004) != 0;
    entry.attributes.directory = (file_info.attributes & 0x00000010) != 0;
    entry.attributes.archive = (file_info.attributes & 0x00000020) != 0;
    entry.attributes.temporary = (file_info.attributes & 0x00000100) != 0;
    entry.attributes.sparse = (file_info.attributes & 0x00000200) != 0;
    entry.attributes.reparsePoint = (file_info.attributes & 0x00000400) != 0;
    entry.attributes.compressed = (file_info.attributes & 0x00000800) != 0;
    entry.attributes.encrypted = (file_info.attributes & 0x00004000) != 0;
#endif
    
    // Timestamps
    entry.timestamps.creationTime = file_info.creation_time;
    entry.timestamps.lastWriteTime = file_info.last_modified_time;
    entry.timestamps.lastAccessTime = file_info.last_access_time;
    entry.timestamps.changeTime = file_info.last_modified_time; // Simplified
    
    // Compute signatures if requested and file is large enough
    if (file_info.size > 0) {
        if (options.computeHeadTail) {
            entry.headTail16 = computeHeadTailSignature(path);
        }
        
        if (options.computeFullHash) {
            entry.sha256 = computeFullHash(path);
        }
    }
    
    // Create scan event
    ScanEvent event(ScanEventType::FileAdded, entry);
    callback(event);
}

std::vector<uint8_t> Scanner::computeHeadTailSignature(const std::string& path) {
    file_handle_t handle = FileUtils::open_file(path, true);
    if (!FileUtils::is_valid_handle(handle)) {
        return {};
    }
    
    uint64_t fileSize = FileUtils::get_file_size(handle);
    if (fileSize == 0) {
        FileUtils::close_file(handle);
        return {};
    }
    
    const size_t chunkSize = 16 * 1024; // 16KB
    std::vector<uint8_t> buffer(chunkSize);
    std::vector<uint8_t> headTailData;
    
    // Read head
    if (FileUtils::read_file_data(handle, buffer.data(), 
                                  std::min(chunkSize, static_cast<size_t>(fileSize)), 0)) {
        size_t bytesRead = std::min(chunkSize, static_cast<size_t>(fileSize));
        headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + bytesRead);
    }
    
    // If file is larger than 32KB, read tail
    if (fileSize > chunkSize * 2) {
        uint64_t offset = fileSize - chunkSize;
        if (FileUtils::read_file_data(handle, buffer.data(), chunkSize, offset)) {
            headTailData.insert(headTailData.end(), buffer.begin(), buffer.end());
        }
    }
    
    FileUtils::close_file(handle);
    
    // Compute hash of head+tail data
    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);
    blake3_hash_update(&blake3, headTailData.data(), headTailData.size());
    
    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);
    
    return hash;
}

std::vector<uint8_t> Scanner::computeFullHash(const std::string& path) {
    file_handle_t handle = FileUtils::open_file(path, true);
    if (!FileUtils::is_valid_handle(handle)) {
        return {};
    }
    
    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);
    
    const size_t bufferSize = 64 * 1024; // 64KB buffer
    std::vector<uint8_t> buffer(bufferSize);
    uint64_t offset = 0;
    
    uint64_t fileSize = FileUtils::get_file_size(handle);
    while (offset < fileSize && !m_cancelled) {
        size_t bytesToRead = static_cast<size_t>(std::min(static_cast<uint64_t>(bufferSize), fileSize - offset));
        
        if (FileUtils::read_file_data(handle, buffer.data(), bytesToRead, offset)) {
            blake3_hash_update(&blake3, buffer.data(), bytesToRead);
            offset += bytesToRead;
        } else {
            break;
        }
    }
    
    FileUtils::close_file(handle);
    
    if (m_cancelled) {
        return {};
    }
    
    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);
    
    return hash;
}

bool Scanner::isExcludedPath(const std::string& path, const ScanOptions& options) {
    for (const auto& excludePath : options.excludePaths) {
        // Convert to platform-specific path for comparison
        std::string platformExcludePath = FileUtils::to_platform_path(StringUtils::to_utf8_string(excludePath));
        if (path.find(platformExcludePath) == 0) {
            return true;
        }
    }
    return false;
}