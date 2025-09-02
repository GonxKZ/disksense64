#include "lsm_index.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

LsmIndex::LsmIndex(const std::wstring& indexPath, size_t memtableSize)
    : m_indexPath(indexPath), m_memtableSize(memtableSize),
      m_compactionRunning(false) {
    // Convert wide string to platform string
    std::string path = FileUtils::to_platform_path(StringUtils::to_utf8_string(indexPath));
    
    // Create directory if it doesn't exist
#ifdef _WIN32
    CreateDirectoryW(indexPath.c_str(), nullptr);
#else
    // Simple directory creation for Linux (in a real implementation, we would handle nested directories)
    mkdir(path.c_str(), 0755);
#endif
    
    // Initialize memtable
    m_memTable = std::make_unique<MemTable>();
    
    // Load existing SSTables
    // In a real implementation, we would scan the directory for existing SSTables
    // and load their metadata
}

LsmIndex::~LsmIndex() {
    stopCompaction();
    flush();
}

LsmIndex::LsmIndex(LsmIndex&& other) noexcept
    : m_indexPath(std::move(other.m_indexPath)),
      m_memtableSize(other.m_memtableSize),
      m_memTable(std::move(other.m_memTable)),
      m_immMemTable(std::move(other.m_immMemTable)),
      m_sstables(std::move(other.m_sstables)),
      m_mappedFiles(std::move(other.m_mappedFiles)),
      m_mappedViews(std::move(other.m_mappedViews)),
      m_compactionRunning(other.m_compactionRunning.load()) {
}

LsmIndex& LsmIndex::operator=(LsmIndex&& other) noexcept {
    if (this != &other) {
        stopCompaction();
        
        m_indexPath = std::move(other.m_indexPath);
        m_memtableSize = other.m_memtableSize;
        m_memTable = std::move(other.m_memTable);
        m_immMemTable = std::move(other.m_immMemTable);
        m_sstables = std::move(other.m_sstables);
        m_mappedFiles = std::move(other.m_mappedFiles);
        m_mappedViews = std::move(other.m_mappedViews);
        m_compactionRunning = other.m_compactionRunning.load();
    }
    return *this;
}

void LsmIndex::put(const FileEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Add to memtable
    m_memTable->put(entry);
    
    // Check if we need to flush
    if (m_memTable->size >= m_memtableSize) {
        flush();
    }
}

void LsmIndex::remove(VolumeId volumeId, FileId fileId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Mark as deleted in memtable
    m_memTable->remove(volumeId, fileId);
    
    // Check if we need to flush
    if (m_memTable->size >= m_memtableSize) {
        flush();
    }
}

std::optional<FileEntry> LsmIndex::get(VolumeId volumeId, FileId fileId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check memtable first
    auto result = m_memTable->get(volumeId, fileId);
    if (result.has_value()) {
        return result;
    }
    
    // Check immutable memtable
    if (m_immMemTable) {
        result = m_immMemTable->get(volumeId, fileId);
        if (result.has_value()) {
            return result;
        }
    }
    
    // Check SSTables in reverse order (newest first)
    for (int level = static_cast<int>(m_sstables.size()) - 1; level >= 0; --level) {
        for (auto it = m_sstables[level].rbegin(); it != m_sstables[level].rend(); ++it) {
            result = (*it)->get(volumeId, fileId);
            if (result.has_value()) {
                return result;
            }
        }
    }
    
    return std::nullopt;
}

std::vector<FileEntry> LsmIndex::getByVolume(VolumeId volumeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Collect from all sources
    // This is a simplified implementation
    
    return results;
}

std::vector<FileEntry> LsmIndex::getBySize(uint64_t size) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Collect from all sources
    // This is a simplified implementation
    
    return results;
}

void LsmIndex::flush() {
    std::lock_guard<std::mutex> flushLock(m_flushMutex);
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // If there's nothing to flush, return
    if (m_memTable->entries.empty()) {
        return;
    }
    
    // Move memtable to immutable memtable
    m_immMemTable = std::move(m_memTable);
    m_memTable = std::make_unique<MemTable>();
    
    // Sort the entries
    std::sort(m_immMemTable->entries.begin(), m_immMemTable->entries.end());
    
    // Create SSTable from immutable memtable
    auto sstable = std::make_unique<SSTable>(getSSTableFileName(0, 0));
    // In a real implementation, we would save the memtable to disk
    
    // Add to level 0
    if (m_sstables.size() <= 0) {
        m_sstables.resize(1);
    }
    m_sstables[0].push_back(std::move(sstable));
    
    // Clear immutable memtable
    m_immMemTable.reset();
}

void LsmIndex::compact() {
    // In a real implementation, this would merge SSTables across levels
    // to reduce read amplification
}

void LsmIndex::startCompaction() {
    if (m_compactionRunning.exchange(true)) {
        return; // Already running
    }
    
    m_compactionThread = std::make_unique<std::thread>([this]() {
        while (m_compactionRunning) {
            compact();
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
}

void LsmIndex::stopCompaction() {
    if (!m_compactionRunning.exchange(false)) {
        return; // Not running
    }
    
    if (m_compactionThread && m_compactionThread->joinable()) {
        m_compactionThread->join();
    }
    m_compactionThread.reset();
}

file_handle_t LsmIndex::createMappedFile(const std::wstring& fileName, size_t size) {
    std::string utf8FileName = StringUtils::to_utf8_string(fileName);
    
#ifdef _WIN32
    HANDLE hFile = CreateFileW(
        fileName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }
    
    // Set file size
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = size;
    if (!SetFilePointerEx(hFile, fileSize, nullptr, FILE_BEGIN) || !SetEndOfFile(hFile)) {
        CloseHandle(hFile);
        return INVALID_HANDLE_VALUE;
    }
    
    // Create file mapping
    HANDLE hMapping = CreateFileMappingW(
        hFile,
        nullptr,
        PAGE_READWRITE,
        fileSize.HighPart,
        fileSize.LowPart,
        nullptr
    );
    
    if (!hMapping) {
        CloseHandle(hFile);
        return INVALID_HANDLE_VALUE;
    }
    
    m_mappedFiles[fileName] = hMapping;
    CloseHandle(hFile); // We can close the file handle, keep the mapping
    
    return hMapping;
#else
    int fd = open(utf8FileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return -1;
    }
    
    // Set file size
    if (ftruncate(fd, size) == -1) {
        close(fd);
        return -1;
    }
    
    m_mappedFiles[fileName] = fd;
    return fd;
#endif
}

void* LsmIndex::mapViewOfFile(file_handle_t hFile, size_t size) {
#ifdef _WIN32
    void* pView = MapViewOfFile(hFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size);
    return pView;
#else
    void* pView = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, hFile, 0);
    if (pView == MAP_FAILED) {
        return nullptr;
    }
    return pView;
#endif
}

void LsmIndex::unmapViewOfFile(const std::wstring& fileName) {
    auto viewIt = m_mappedViews.find(fileName);
    if (viewIt != m_mappedViews.end()) {
#ifdef _WIN32
        UnmapViewOfFile(viewIt->second);
#else
        // In Linux, we need to know the size to unmap, which we don't have here
        // In a real implementation, we would store the size as well
        // For now, we'll just remove the entry
#endif
        m_mappedViews.erase(viewIt);
    }
    
    auto fileIt = m_mappedFiles.find(fileName);
    if (fileIt != m_mappedFiles.end()) {
#ifdef _WIN32
        CloseHandle(fileIt->second);
#else
        close(fileIt->second);
#endif
        m_mappedFiles.erase(fileIt);
    }
}

std::wstring LsmIndex::getMemTableFileName() const {
    return m_indexPath + std::wstring(L"\\memtable.dat");
}

std::wstring LsmIndex::getSSTableFileName(int level, int number) const {
    wchar_t fileName[256];
#ifdef _WIN32
    swprintf_s(fileName, L"\\sstable_%d_%d.dat", level, number);
#else
    swprintf(fileName, 256, L"/sstable_%d_%d.dat", level, number);
#endif
    return m_indexPath + std::wstring(fileName);
}

void LsmIndex::mergeSSTables(int level) {
    // In a real implementation, this would merge overlapping SSTables
    // in a level to reduce the number of files
}

// SSTable implementation
SSTable::SSTable(const std::wstring& filePath)
    : m_filePath(filePath), m_hFile(INVALID_FILE_HANDLE),
      m_mappedView(nullptr), m_fileSize(0) {
}

SSTable::~SSTable() {
    if (m_mappedView) {
#ifdef _WIN32
        UnmapViewOfFile(m_mappedView);
#else
        // In Linux, we need to know the size to unmap
        // In a real implementation, we would store the size
        // For now, we'll just do a simple unmap with a large size
        munmap(m_mappedView, m_fileSize);
#endif
    }
    if (FileUtils::is_valid_handle(m_hFile)) {
#ifdef _WIN32
        CloseHandle(m_hFile);
#else
        close(m_hFile);
#endif
    }
}

bool SSTable::load() {
    std::string utf8Path = StringUtils::to_utf8_string(m_filePath);
    
#ifdef _WIN32
    // Open existing file
    m_hFile = CreateFileW(
        m_filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (m_hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(m_hFile, &fileSize)) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    m_fileSize = static_cast<size_t>(fileSize.QuadPart);
    
    // Create file mapping
    HANDLE hMapping = CreateFileMappingW(
        m_hFile,
        nullptr,
        PAGE_READONLY,
        fileSize.HighPart,
        fileSize.LowPart,
        nullptr
    );
    
    if (!hMapping) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    // Map view of file
    m_mappedView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping); // We can close the mapping handle, keep the view
    
    if (!m_mappedView) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
#else
    // Open existing file
    m_hFile = open(utf8Path.c_str(), O_RDONLY);
    if (m_hFile == -1) {
        return false;
    }
    
    // Get file size
    struct stat st;
    if (fstat(m_hFile, &st) == -1) {
        close(m_hFile);
        m_hFile = -1;
        return false;
    }
    m_fileSize = static_cast<size_t>(st.st_size);
    
    // Map view of file
    m_mappedView = mmap(nullptr, m_fileSize, PROT_READ, MAP_PRIVATE, m_hFile, 0);
    if (m_mappedView == MAP_FAILED) {
        close(m_hFile);
        m_hFile = -1;
        return false;
    }
#endif
    
    // Read header
    if (m_fileSize < sizeof(Header)) {
#ifdef _WIN32
        UnmapViewOfFile(m_mappedView);
        m_mappedView = nullptr;
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
#else
        munmap(m_mappedView, m_fileSize);
        m_mappedView = nullptr;
        close(m_hFile);
        m_hFile = -1;
#endif
        return false;
    }
    
    m_header = *static_cast<const Header*>(m_mappedView);
    
    // Validate header
    if (m_header.magic != 0x494E4458) { // "INDX"
#ifdef _WIN32
        UnmapViewOfFile(m_mappedView);
        m_mappedView = nullptr;
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
#else
        munmap(m_mappedView, m_fileSize);
        m_mappedView = nullptr;
        close(m_hFile);
        m_hFile = -1;
#endif
        return false;
    }
    
    // Read index
    const char* pData = static_cast<const char*>(m_mappedView);
    const IndexEntry* pIndex = reinterpret_cast<const IndexEntry*>(pData + sizeof(Header));
    
    m_index.reserve(static_cast<size_t>(m_header.entryCount));
    for (uint64_t i = 0; i < m_header.entryCount; ++i) {
        m_index.push_back(pIndex[i]);
    }
    
    return true;
}

bool SSTable::save(const MemTable& memTable) {
    // Remove deleted entries and sort
    std::vector<MemTable::Entry> entries;
    entries.reserve(memTable.entries.size());
    
    for (const auto& entry : memTable.entries) {
        if (!entry.deleted) {
            entries.push_back(entry);
        }
    }
    
    std::sort(entries.begin(), entries.end());
    
    // Calculate file size
    size_t headerSize = sizeof(Header);
    size_t indexSize = entries.size() * sizeof(IndexEntry);
    size_t dataSize = 0;
    
    // Calculate data size
    for (const auto& entry : entries) {
        dataSize += sizeof(FileEntry); // Simplified - in reality would serialize the entry
    }
    
    size_t fileSize = headerSize + indexSize + dataSize;
    std::string utf8Path = StringUtils::to_utf8_string(m_filePath);
    
#ifdef _WIN32
    // Create file
    m_hFile = CreateFileW(
        m_filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // No sharing
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (m_hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Set file size
    LARGE_INTEGER liFileSize;
    liFileSize.QuadPart = fileSize;
    if (!SetFilePointerEx(m_hFile, liFileSize, nullptr, FILE_BEGIN) || !SetEndOfFile(m_hFile)) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    // Create file mapping
    HANDLE hMapping = CreateFileMappingW(
        m_hFile,
        nullptr,
        PAGE_READWRITE,
        liFileSize.HighPart,
        liFileSize.LowPart,
        nullptr
    );
    
    if (!hMapping) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    // Map view of file
    m_mappedView = MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hMapping); // We can close the mapping handle, keep the view
    
    if (!m_mappedView) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        return false;
    }
#else
    // Create file
    m_hFile = open(utf8Path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (m_hFile == -1) {
        return false;
    }
    
    // Set file size
    if (ftruncate(m_hFile, fileSize) == -1) {
        close(m_hFile);
        m_hFile = -1;
        return false;
    }
    
    // Map view of file
    m_mappedView = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_hFile, 0);
    if (m_mappedView == MAP_FAILED) {
        close(m_hFile);
        m_hFile = -1;
        return false;
    }
#endif
    
    // Write header
    Header* pHeader = static_cast<Header*>(m_mappedView);
    pHeader->magic = 0x494E4458; // "INDX"
    pHeader->version = 1;
    pHeader->entryCount = entries.size();
    
    if (!entries.empty()) {
        pHeader->minVolumeId = entries.front().volumeId;
        pHeader->maxVolumeId = entries.back().volumeId;
        pHeader->minFileId = entries.front().fileId;
        pHeader->maxFileId = entries.back().fileId;
    }
    
    m_header = *pHeader;
    
    // Write index
    IndexEntry* pIndex = reinterpret_cast<IndexEntry*>(static_cast<char*>(m_mappedView) + sizeof(Header));
    uint64_t offset = sizeof(Header) + indexSize;
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        pIndex[i].volumeId = entry.volumeId;
        pIndex[i].fileId = entry.fileId;
        pIndex[i].offset = offset;
        pIndex[i].size = sizeof(FileEntry); // Simplified
        pIndex[i].deleted = false;
        
        offset += pIndex[i].size;
    }
    
    // Write data
    char* pData = static_cast<char*>(m_mappedView) + sizeof(Header) + indexSize;
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        // In a real implementation, we would serialize the FileEntry
        // For now, we'll just copy the structure
        *reinterpret_cast<FileEntry*>(pData) = entry.fileEntry;
        pData += sizeof(FileEntry);
    }
    
    // Update index
    m_index.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) {
        m_index.push_back(pIndex[i]);
    }
    
    return true;
}

std::optional<FileEntry> SSTable::get(VolumeId volumeId, FileId fileId) const {
    if (!m_mappedView) {
        return std::nullopt;
    }
    
    // Binary search in index
    IndexEntry searchKey;
    searchKey.volumeId = volumeId;
    searchKey.fileId = fileId;
    
    auto it = std::lower_bound(m_index.begin(), m_index.end(), searchKey,
        [](const IndexEntry& a, const IndexEntry& b) {
            if (a.volumeId != b.volumeId) {
                return a.volumeId < b.volumeId;
            }
            return a.fileId < b.fileId;
        });
    
    if (it != m_index.end() && it->volumeId == volumeId && it->fileId == fileId) {
        if (it->deleted) {
            return std::nullopt;
        }
        
        // Read data from mapped view
        const char* pData = static_cast<const char*>(m_mappedView);
        const FileEntry* pEntry = reinterpret_cast<const FileEntry*>(pData + it->offset);
        return *pEntry;
    }
    
    return std::nullopt;
}

std::vector<FileEntry> SSTable::getByVolume(VolumeId volumeId) const {
    std::vector<FileEntry> results;
    
    if (!m_mappedView) {
        return results;
    }
    
    // Find range of entries for this volume
    auto lower = std::lower_bound(m_index.begin(), m_index.end(), volumeId,
        [](const IndexEntry& entry, VolumeId volId) {
            return entry.volumeId < volId;
        });
    
    auto upper = std::upper_bound(m_index.begin(), m_index.end(), volumeId,
        [](VolumeId volId, const IndexEntry& entry) {
            return volId < entry.volumeId;
        });
    
    // Collect entries
    for (auto it = lower; it != upper; ++it) {
        if (!it->deleted) {
            const char* pData = static_cast<const char*>(m_mappedView);
            const FileEntry* pEntry = reinterpret_cast<const FileEntry*>(pData + it->offset);
            results.push_back(*pEntry);
        }
    }
    
    return results;
}

std::vector<FileEntry> SSTable::getBySize(uint64_t size) const {
    std::vector<FileEntry> results;
    
    if (!m_mappedView) {
        return results;
    }
    
    // Linear search for entries with matching size
    // In a real implementation, we would have a size index
    for (const auto& entry : m_index) {
        if (!entry.deleted) {
            const char* pData = static_cast<const char*>(m_mappedView);
            const FileEntry* pEntry = reinterpret_cast<const FileEntry*>(pData + entry.offset);
            if (pEntry->sizeLogical == size) {
                results.push_back(*pEntry);
            }
        }
    }
    
    return results;
}