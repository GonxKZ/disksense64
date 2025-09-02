#include "scanner.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include "libs/chash/sha256.h"
#include "libs/chash/blake3.h"
#include "libs/utils/utils.h"

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

Scanner::Scanner() 
    : m_cancelled(false), m_scanning(false) {
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
    
    try {
        // Check if path exists and is accessible
        if (!std::filesystem::exists(path)) {
            return;
        }
        
        // Iterate through directory entries
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (m_cancelled) break;
            
            // Skip . and ..
            if (entry.path().filename() == "." || entry.path().filename() == "..") {
                continue;
            }
            
            std::string full_path = entry.path().string();
            
            // Check if path should be excluded
            if (isExcludedPath(StringUtils::to_wide_string(full_path), options)) {
                continue;
            }
            
            if (entry.is_directory()) {
                // Recursively scan subdirectory
                scanDirectory(full_path, options, callback);
            } else if (entry.is_regular_file()) {
                // Process file
                uint64_t file_size = entry.file_size();
                processFile(full_path, file_size, options, callback);
            }
        }
    } catch (const std::exception& e) {
        // Silently ignore inaccessible directories
        std::cerr << "Warning: Could not access directory " << path << ": " << e.what() << std::endl;
    }
}

void Scanner::processFile(const std::string& path,
                         uint64_t fileSize,
                         const ScanOptions& options,
                         std::function<void(const ScanEvent&)> callback) {
    // Apply size filters
    if (options.minFileSize > 0 && fileSize < options.minFileSize) {
        return;
    }
    
    if (options.maxFileSize > 0 && fileSize > options.maxFileSize) {
        return;
    }
    
    // Apply extension filters
    if (!matchesExtension(StringUtils::to_wide_string(path), options)) {
        return;
    }
    
    FileEntry entry;
    
    // File identification
    entry.volumeId = 1; // Simplified for cross-platform
    entry.fileId = std::hash<std::string>{}(path);
    entry.pathId = std::hash<std::string>{}(path);
    
    // File size
    entry.sizeLogical = fileSize;
    entry.sizeOnDisk = (fileSize + 4095) & ~4095; // Assume 4KB clusters
    
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
    // Get Windows-specific file attributes
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        entry.attributes.readOnly = (attrs & FILE_ATTRIBUTE_READONLY) != 0;
        entry.attributes.hidden = (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
        entry.attributes.system = (attrs & FILE_ATTRIBUTE_SYSTEM) != 0;
        entry.attributes.directory = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry.attributes.archive = (attrs & FILE_ATTRIBUTE_ARCHIVE) != 0;
        entry.attributes.temporary = (attrs & FILE_ATTRIBUTE_TEMPORARY) != 0;
        entry.attributes.sparse = (attrs & FILE_ATTRIBUTE_SPARSE_FILE) != 0;
        entry.attributes.reparsePoint = (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        entry.attributes.compressed = (attrs & FILE_ATTRIBUTE_COMPRESSED) != 0;
        entry.attributes.encrypted = (attrs & FILE_ATTRIBUTE_ENCRYPTED) != 0;
        entry.attributes.offline = (attrs & FILE_ATTRIBUTE_OFFLINE) != 0;
        entry.attributes.notContentIndexed = (attrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0;
        entry.attributes.virtualFile = (attrs & FILE_ATTRIBUTE_VIRTUAL) != 0;
    }
#endif
    
    // Timestamps
    auto ftime = std::filesystem::last_write_time(path);
    auto systime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    
    entry.timestamps.creationTime = std::chrono::duration_cast<std::chrono::seconds>(
        systime.time_since_epoch()).count();
    entry.timestamps.lastWriteTime = entry.timestamps.creationTime;
    entry.timestamps.lastAccessTime = entry.timestamps.creationTime;
    entry.timestamps.changeTime = entry.timestamps.creationTime;
    
    // Compute signatures if requested
    if (options.computeHeadTail && fileSize > 0) {
        entry.headTail16 = computeHeadTailSignature(path);
    }
    
    if (options.computeFullHash && fileSize > 0) {
        entry.sha256 = computeFullHash(path);
    }
    
    // Extract file ID
    entry.fileId = extractFileId(path);
    
    // Create scan event
    ScanEvent event(ScanEventType::FileAdded, entry);
    callback(event);
}

std::vector<uint8_t> Scanner::computeHeadTailSignature(const std::string& path) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return {};
    }
#else
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return {};
    }
#endif
    
    uint64_t fileSize = 0;
#ifdef _WIN32
    LARGE_INTEGER liFileSize;
    if (GetFileSizeEx(hFile, &liFileSize)) {
        fileSize = static_cast<uint64_t>(liFileSize.QuadPart);
    }
#else
    struct stat st;
    if (fstat(fd, &st) == 0) {
        fileSize = static_cast<uint64_t>(st.st_size);
    }
#endif
    
    if (fileSize == 0) {
#ifdef _WIN32
        CloseHandle(hFile);
#else
        close(fd);
#endif
        return {};
    }
    
    const size_t chunkSize = 16 * 1024; // 16KB
    std::vector<uint8_t> buffer(chunkSize * 2); // Head + Tail
    std::vector<uint8_t> headTailData;
    
    // Read head
#ifdef _WIN32
    DWORD bytesRead = 0;
    if (ReadFile(hFile, buffer.data(), static_cast<DWORD>(chunkSize), &bytesRead, nullptr)) {
        headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + bytesRead);
    }
#else
    ssize_t bytesRead = read(fd, buffer.data(), chunkSize);
    if (bytesRead > 0) {
        headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + bytesRead);
    }
#endif
    
    // If file is larger than 32KB, read tail
    if (fileSize > chunkSize * 2) {
#ifdef _WIN32
        LARGE_INTEGER offset;
        offset.QuadPart = fileSize - chunkSize;
        if (SetFilePointerEx(hFile, offset, nullptr, FILE_BEGIN)) {
            if (ReadFile(hFile, buffer.data(), static_cast<DWORD>(chunkSize), &bytesRead, nullptr)) {
                headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + bytesRead);
            }
        }
#else
        off_t offset = fileSize - chunkSize;
        if (lseek(fd, offset, SEEK_SET) != -1) {
            bytesRead = read(fd, buffer.data(), chunkSize);
            if (bytesRead > 0) {
                headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + bytesRead);
            }
        }
#endif
    }
    
    // Compute hash of head+tail data
    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);
    blake3_hash_update(&blake3, headTailData.data(), headTailData.size());
    
    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);
    
#ifdef _WIN32
    CloseHandle(hFile);
#else
    close(fd);
#endif
    
    return hash;
}

std::vector<uint8_t> Scanner::computeFullHash(const std::string& path) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return {};
    }
#else
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return {};
    }
#endif
    
    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);
    
    const size_t bufferSize = 64 * 1024; // 64KB buffer
    std::vector<uint8_t> buffer(bufferSize);
    
#ifdef _WIN32
    DWORD bytesRead;
    while (ReadFile(hFile, buffer.data(), static_cast<DWORD>(bufferSize), &bytesRead, nullptr) && 
           bytesRead > 0) {
        if (m_cancelled) {
            CloseHandle(hFile);
            return {};
        }
        blake3_hash_update(&blake3, buffer.data(), bytesRead);
    }
    CloseHandle(hFile);
#else
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer.data(), bufferSize)) > 0) {
        if (m_cancelled) {
            close(fd);
            return {};
        }
        blake3_hash_update(&blake3, buffer.data(), bytesRead);
    }
    close(fd);
#endif
    
    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);
    
    return hash;
}

FileId Scanner::extractFileId(const std::string& path) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(
        path.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    // Get file ID from handle
    FILE_ID_INFO fileIdInfo = {0};
    if (GetFileInformationByHandleEx(hFile, FileIdInfo, &fileIdInfo, sizeof(fileIdInfo))) {
        CloseHandle(hFile);
        // Use the 64-bit file ID when possible
        // For simplicity, we're using the low part, but a real implementation
        // would use the full 128-bit identifier
        return fileIdInfo.FileId.LowPart;
    }
    
    // Fallback to basic file info
    BY_HANDLE_FILE_INFORMATION fileInfo = {0};
    if (GetFileInformationByHandle(hFile, &fileInfo)) {
        CloseHandle(hFile);
        return (static_cast<FileId>(fileInfo.nFileIndexHigh) << 32) | fileInfo.nFileIndexLow;
    }
    
    CloseHandle(hFile);
    return 0;
#else
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return static_cast<FileId>(st.st_ino);
    }
    return 0;
#endif
}

bool Scanner::isExcludedPath(const std::wstring& path, const ScanOptions& options) {
    for (const auto& excludePath : options.excludePaths) {
        if (path.find(excludePath) == 0) {
            return true;
        }
    }
    return false;
}

bool Scanner::matchesExtension(const std::wstring& path, const ScanOptions& options) {
    if (options.includeExtensions.empty()) {
        return true; // Include all extensions
    }
    
    // Get file extension
    size_t dotPos = path.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        return false; // No extension
    }
    
    std::wstring extension = path.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Check include list
    for (const auto& includeExt : options.includeExtensions) {
        std::wstring lowerInclude = includeExt;
        std::transform(lowerInclude.begin(), lowerInclude.end(), lowerInclude.begin(), ::tolower);
        if (extension == lowerInclude) {
            return true;
        }
    }
    
    return false;
}