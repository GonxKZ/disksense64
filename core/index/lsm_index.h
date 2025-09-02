#ifndef CORE_INDEX_LSM_INDEX_H
#define CORE_INDEX_LSM_INDEX_H

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include "core/model/model.h"

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Forward declarations
class SSTable;
struct MemTable;

// Platform-independent file handle type
#ifdef _WIN32
typedef HANDLE file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = INVALID_HANDLE_VALUE;
#else
typedef int file_handle_t;
const file_handle_t INVALID_FILE_HANDLE = -1;
#endif

// LSM Index implementation
class LsmIndex {
public:
    explicit LsmIndex(const std::wstring& indexPath, size_t memtableSize = 64 * 1024 * 1024); // 64MB default
    ~LsmIndex();
    
    // Delete copy constructor and assignment operator
    LsmIndex(const LsmIndex&) = delete;
    LsmIndex& operator=(const LsmIndex&) = delete;
    
    // Move constructor and assignment
    LsmIndex(LsmIndex&& other) noexcept;
    LsmIndex& operator=(LsmIndex&& other) noexcept;
    
    // Insert or update a file entry
    void put(const FileEntry& entry);
    
    // Remove a file entry
    void remove(VolumeId volumeId, FileId fileId);
    
    // Get a file entry
    std::optional<FileEntry> get(VolumeId volumeId, FileId fileId) const;
    
    // Range query by volume
    std::vector<FileEntry> getByVolume(VolumeId volumeId) const;
    
    // Range query by size
    std::vector<FileEntry> getBySize(uint64_t size) const;
    
    // Flush memtable to disk
    void flush();
    
    // Compact SSTables
    void compact();
    
    // Start background compaction
    void startCompaction();
    
    // Stop background compaction
    void stopCompaction();
    
private:
    std::wstring m_indexPath;
    size_t m_memtableSize;
    
    // In-memory table
    std::unique_ptr<MemTable> m_memTable;
    
    // Immutable memtable being flushed
    std::unique_ptr<MemTable> m_immMemTable;
    
    // SSTables sorted by level
    std::vector<std::vector<std::unique_ptr<SSTable>>> m_sstables;
    
    // Memory-mapped files for SSTables
    mutable std::map<std::wstring, file_handle_t> m_mappedFiles;
    mutable std::map<std::wstring, void*> m_mappedViews;
    
    // Synchronization
    mutable std::mutex m_mutex;
    mutable std::mutex m_flushMutex;
    std::atomic<bool> m_compactionRunning;
    std::unique_ptr<std::thread> m_compactionThread;
    
    // File operations
    file_handle_t createMappedFile(const std::wstring& fileName, size_t size);
    void* mapViewOfFile(file_handle_t hFile, size_t size);
    void unmapViewOfFile(const std::wstring& fileName);
    
    // Helper functions
    std::wstring getMemTableFileName() const;
    std::wstring getSSTableFileName(int level, int number) const;
    void mergeSSTables(int level);
};

// MemTable structure (in-memory sorted table)
struct MemTable {
    struct Entry {
        VolumeId volumeId;
        FileId fileId;
        FileEntry fileEntry;
        bool deleted;
        
        Entry(VolumeId v, FileId f, const FileEntry& entry, bool del = false)
            : volumeId(v), fileId(f), fileEntry(entry), deleted(del) {}
        
        // Comparison operator for sorting
        bool operator<(const Entry& other) const {
            if (volumeId != other.volumeId) {
                return volumeId < other.volumeId;
            }
            return fileId < other.fileId;
        }
    };
    
    std::vector<Entry> entries;
    size_t size;
    
    MemTable() : size(0) {}
    
    void put(const FileEntry& entry) {
        // In a real implementation, we would maintain a sorted structure
        // For simplicity, we're using a vector and will sort on flush
        entries.emplace_back(entry.volumeId, entry.fileId, entry);
        size += sizeof(Entry) + entry.sizeLogical; // Approximate size
    }
    
    void remove(VolumeId volumeId, FileId fileId) {
        entries.emplace_back(volumeId, fileId, FileEntry(), true);
        size += sizeof(Entry);
    }
    
    std::optional<FileEntry> get(VolumeId volumeId, FileId fileId) const {
        // Linear search for simplicity (in reality would use binary search)
        for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
            if (it->volumeId == volumeId && it->fileId == fileId) {
                if (it->deleted) {
                    return std::nullopt;
                }
                return it->fileEntry;
            }
        }
        return std::nullopt;
    }
};

// SSTable structure (immutable sorted table on disk)
class SSTable {
public:
    struct Header {
        uint32_t magic;
        uint32_t version;
        uint64_t entryCount;
        uint64_t minVolumeId;
        uint64_t maxVolumeId;
        uint64_t minFileId;
        uint64_t maxFileId;
        
        Header() : magic(0x494E4458), version(1), entryCount(0),
                   minVolumeId(0), maxVolumeId(0), minFileId(0), maxFileId(0) {}
    };
    
    struct IndexEntry {
        VolumeId volumeId;
        FileId fileId;
        uint64_t offset;
        uint32_t size;
        bool deleted;
        
        IndexEntry() : volumeId(0), fileId(0), offset(0), size(0), deleted(false) {}
    };
    
private:
    std::wstring m_filePath;
    file_handle_t m_hFile;
    void* m_mappedView;
    size_t m_fileSize;
    
    Header m_header;
    std::vector<IndexEntry> m_index;
    
public:
    SSTable(const std::wstring& filePath);
    ~SSTable();
    
    bool load();
    bool save(const MemTable& memTable);
    
    std::optional<FileEntry> get(VolumeId volumeId, FileId fileId) const;
    std::vector<FileEntry> getByVolume(VolumeId volumeId) const;
    std::vector<FileEntry> getBySize(uint64_t size) const;
    
    const Header& getHeader() const { return m_header; }
    const std::vector<IndexEntry>& getIndex() const { return m_index; }
    
    VolumeId getMinVolumeId() const { return m_header.minVolumeId; }
    VolumeId getMaxVolumeId() const { return m_header.maxVolumeId; }
    FileId getMinFileId() const { return m_header.minFileId; }
    FileId getMaxFileId() const { return m_header.maxFileId; }
};

#endif // CORE_INDEX_LSM_INDEX_H

#endif // CORE_INDEX_LSM_INDEX_H