#include "scanner.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include "libs/chash/sha256.h"
#include "libs/chash/blake3.h"
#include "libs/utils/utils.h"
#ifdef _WIN32
#include "win_mft.h"
#endif
#include <sys/stat.h>

Scanner::Scanner()
    : m_cancelled(false), m_scanning(false) {
}

Scanner::~Scanner() {
    cancel();
}

void Scanner::scanVolume(const std::string& volumePath,
                        const ScanOptions& options,
                        std::function<void(const ScanEvent&)> callback) {
    if (m_scanning.exchange(true)) {
        return; // Already scanning
    }

    m_cancelled = false;

    // Windows fast path: MFT enumerator when requested
#ifdef _WIN32
    if (options.useMftReader) {
        if (!winfs::enumerate_mft(volumePath, callback)) {
            // fallback to recursive
            scanDirectory(volumePath, options, callback);
        }
    } else {
        scanDirectory(volumePath, options, callback);
    }
#else
    scanDirectory(volumePath, options, callback);
#endif

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
            if (isExcludedPath(full_path, options)) {
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
    if (!matchesExtension(path, options)) {
        return;
    }

    FileEntry entry;
    file_info_t info;

    if (!FileUtils::get_file_info(path, info)) {
        return;
    }

    // File identification
    entry.volumeId = 1; // Simplified for cross-platform
    entry.fileId = info.last_access_time; // Placeholder
    entry.pathId = std::hash<std::string>{}(path);
    entry.fullPath = path;

    // File size
    entry.sizeLogical = info.size;
    entry.sizeOnDisk = (info.size + 4095) & ~4095; // Assume 4KB clusters

    // File attributes
#ifdef _WIN32
    entry.attributes.readOnly = (info.attributes & FILE_ATTRIBUTE_READONLY) != 0;
    entry.attributes.hidden = (info.attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    entry.attributes.system = (info.attributes & FILE_ATTRIBUTE_SYSTEM) != 0;
    entry.attributes.directory = (info.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    entry.attributes.archive = (info.attributes & FILE_ATTRIBUTE_ARCHIVE) != 0;
    entry.attributes.temporary = (info.attributes & FILE_ATTRIBUTE_TEMPORARY) != 0;
    entry.attributes.sparse = (info.attributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0;
    entry.attributes.reparsePoint = (info.attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    entry.attributes.compressed = (info.attributes & FILE_ATTRIBUTE_COMPRESSED) != 0;
    entry.attributes.encrypted = (info.attributes & FILE_ATTRIBUTE_ENCRYPTED) != 0;
    entry.attributes.offline = (info.attributes & FILE_ATTRIBUTE_OFFLINE) != 0;
    entry.attributes.notContentIndexed = (info.attributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0;
    entry.attributes.virtualFile = (info.attributes & FILE_ATTRIBUTE_VIRTUAL) != 0;
#else
    entry.attributes.readOnly = (info.permissions & S_IWUSR) == 0;
    entry.attributes.hidden = info.name[0] == '.';
    entry.attributes.system = false;
    entry.attributes.directory = S_ISDIR(info.permissions);
    entry.attributes.archive = false;
    entry.attributes.temporary = false;
    entry.attributes.sparse = false;
    entry.attributes.reparsePoint = false;
    entry.attributes.compressed = false;
    entry.attributes.encrypted = false;
    entry.attributes.offline = false;
    entry.attributes.notContentIndexed = false;
    entry.attributes.virtualFile = false;
#endif

    // Timestamps
    entry.timestamps.creationTime = info.creation_time;
    entry.timestamps.lastWriteTime = info.last_modified_time;
    entry.timestamps.lastAccessTime = info.last_access_time;
    entry.timestamps.changeTime = info.last_modified_time;

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
    file_handle_t hFile = FileUtils::open_file(path, true);
    if (!FileUtils::is_valid_handle(hFile)) {
        return {};
    }

    uint64_t fileSize = FileUtils::get_file_size(hFile);
    if (fileSize == 0) {
        FileUtils::close_file(hFile);
        return {};
    }

    const size_t chunkSize = 16 * 1024; // 16KB
    std::vector<uint8_t> buffer(chunkSize * 2); // Head + Tail
    std::vector<uint8_t> headTailData;

    // Read head
    if (FileUtils::read_file_data(hFile, buffer.data(), chunkSize)) {
        headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + chunkSize);
    }

    // If file is larger than 32KB, read tail
    if (fileSize > chunkSize * 2) {
        if (FileUtils::read_file_data(hFile, buffer.data(), chunkSize, fileSize - chunkSize)) {
            headTailData.insert(headTailData.end(), buffer.begin(), buffer.begin() + chunkSize);
        }
    }

    // Compute hash of head+tail data
    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);
    blake3_hash_update(&blake3, headTailData.data(), headTailData.size());

    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);

    FileUtils::close_file(hFile);

    return hash;
}

std::vector<uint8_t> Scanner::computeFullHash(const std::string& path) {
    file_handle_t hFile = FileUtils::open_file(path, true);
    if (!FileUtils::is_valid_handle(hFile)) {
        return {};
    }

    BLAKE3_HASH_STATE blake3;
    blake3_hash_init(&blake3);

    const size_t bufferSize = 64 * 1024; // 64KB buffer
    std::vector<uint8_t> buffer(bufferSize);

    uint64_t offset = 0;
    while (FileUtils::read_file_data(hFile, buffer.data(), bufferSize, offset)) {
        if (m_cancelled) {
            FileUtils::close_file(hFile);
            return {};
        }
        blake3_hash_update(&blake3, buffer.data(), bufferSize);
        offset += bufferSize;
    }

    FileUtils::close_file(hFile);

    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hash_finalize(&blake3, hash.data(), BLAKE3_OUT_LEN);

    return hash;
}

FileId Scanner::extractFileId(const std::string& path) {
    file_info_t info;
    if (FileUtils::get_file_info(path, info)) {
#ifdef _WIN32
        return info.last_access_time; // Placeholder
#else
        // On Linux, combine inode and device ID for a more unique identifier
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            // Combine device ID and inode number to create a unique file ID
            return (static_cast<FileId>(st.st_dev) << 32) | (st.st_ino & 0xFFFFFFFF);
        }
#endif
    }
    // Fallback: use a hash of the path
    return std::hash<std::string>{}(path);
}

bool Scanner::isExcludedPath(const std::string& path, const ScanOptions& options) {
    for (const auto& excludePath : options.excludePaths) {
        if (path.find(excludePath) == 0) {
            return true;
        }
    }
    return false;
}

bool Scanner::matchesExtension(const std::string& path, const ScanOptions& options) {
    if (options.includeExtensions.empty()) {
        return true; // Include all extensions
    }

    // Get file extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false; // No extension
    }

    std::string extension = path.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Check include list
    for (const auto& includeExt : options.includeExtensions) {
        std::string lowerInclude = includeExt;
        std::transform(lowerInclude.begin(), lowerInclude.end(), lowerInclude.begin(), ::tolower);
        if (extension == lowerInclude) {
            return true;
        }
    }

    return false;
}
