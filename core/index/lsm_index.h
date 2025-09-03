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

// Forward declarations
class LSMIndexImpl;

// LSM Index
class LSMIndex {
public:
    explicit LSMIndex(const std::string& indexPath, size_t memtableSize = 64 * 1024 * 1024); // 64MB default
    ~LSMIndex();

    // Delete copy constructor and assignment operator
    LSMIndex(const LSMIndex&) = delete;
    LSMIndex& operator=(const LSMIndex&) = delete;

    // Move constructor and assignment
    LSMIndex(LSMIndex&& other) noexcept;
    LSMIndex& operator=(LSMIndex&& other) noexcept;

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
    std::unique_ptr<LSMIndexImpl> m_impl;
};

#endif // CORE_INDEX_LSM_INDEX_H