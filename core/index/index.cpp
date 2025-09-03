#include "index.h"
#include "lsm_index_impl.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "libs/chash/sha256.h"
#include "libs/chash/blake3.h"

// Index implementation
Index::Index() {
}

Index::~Index() {
}

// LSMIndex implementation
LSMIndex::LSMIndex(const std::string& indexPath, size_t memtableSize)
    : m_impl(std::make_unique<LSMIndexImpl>(indexPath, memtableSize)) {
}

LSMIndex::~LSMIndex() {
}

void LSMIndex::put(const FileEntry& entry) {
    m_impl->put(entry);
}

void LSMIndex::remove(VolumeId volumeId, FileId fileId) {
    m_impl->remove(volumeId, fileId);
}

std::optional<FileEntry> LSMIndex::get(VolumeId volumeId, FileId fileId) const {
    return m_impl->get(volumeId, fileId);
}

std::vector<FileEntry> LSMIndex::getByVolume(VolumeId volumeId) const {
    return m_impl->getByVolume(volumeId);
}

std::vector<FileEntry> LSMIndex::getBySize(uint64_t size) const {
    return std::vector<FileEntry>();
}

std::vector<FileEntry> LSMIndex::getByPath(const std::string& path) const {
    return std::vector<FileEntry>();
}

std::vector<FileEntry> LSMIndex::getByExtension(const std::string& extension) const {
    return std::vector<FileEntry>();
}

std::vector<FileEntry> LSMIndex::getByDateRange(uint64_t startDate, uint64_t endDate) const {
    return std::vector<FileEntry>();
}

void LSMIndex::flush() {
    m_impl->flush();
}

void LSMIndex::compact() {
    m_impl->compact();
}

IndexStats LSMIndex::getStats() const {
    return IndexStats();
}

void LSMIndex::clear() {
}

void LSMIndex::startCompaction() {
    m_impl->startCompaction();
}

void LSMIndex::stopCompaction() {
    m_impl->stopCompaction();
}

// InMemoryIndex implementation
InMemoryIndex::InMemoryIndex()
    : m_queryCount(0), m_totalQueryTime(0) {
}

InMemoryIndex::~InMemoryIndex() {
}

void InMemoryIndex::put(const FileEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto key = std::make_pair(entry.volumeId, entry.fileId);
    m_entries[key] = IndexEntry(entry, false);
}

void InMemoryIndex::remove(VolumeId volumeId, FileId fileId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto key = std::make_pair(volumeId, fileId);
    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        it->second.deleted = true;
    }
}

std::optional<FileEntry> InMemoryIndex::get(VolumeId volumeId, FileId fileId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    auto key = std::make_pair(volumeId, fileId);
    auto it = m_entries.find(key);
    if (it != m_entries.end() && !it->second.deleted) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        m_queryCount.fetch_add(1);
        m_totalQueryTime.fetch_add(duration.count());
        return it->second.fileEntry;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());
    return std::nullopt;
}

std::vector<FileEntry> InMemoryIndex::getByVolume(VolumeId volumeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<FileEntry> results;
    for (const auto& [key, entry] : m_entries) {
        if (key.first == volumeId && !entry.deleted) {
            results.push_back(entry.fileEntry);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());

    return results;
}

std::vector<FileEntry> InMemoryIndex::getBySize(uint64_t size) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<FileEntry> results;
    for (const auto& [key, entry] : m_entries) {
        if (entry.fileEntry.sizeLogical == size && !entry.deleted) {
            results.push_back(entry.fileEntry);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());

    return results;
}

std::vector<FileEntry> InMemoryIndex::getByPath(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<FileEntry> results;
    for (const auto& [key, entry] : m_entries) {
        if (entry.fileEntry.pathId == std::hash<std::string>{}(path) && !entry.deleted) {
            results.push_back(entry.fileEntry);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());

    return results;
}

std::vector<FileEntry> InMemoryIndex::getByExtension(const std::string& extension) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<FileEntry> results;
    for (const auto& [key, entry] : m_entries) {
        if (!entry.deleted) {
            std::string path = std::to_string(entry.fileEntry.pathId); // Simplified
            size_t dot_pos = path.find_last_of('.');
            if (dot_pos != std::string::npos) {
                std::string file_ext = path.substr(dot_pos);
                if (file_ext == extension) {
                    results.push_back(entry.fileEntry);
                }
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());

    return results;
}

std::vector<FileEntry> InMemoryIndex::getByDateRange(uint64_t startDate, uint64_t endDate) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<FileEntry> results;
    for (const auto& [key, entry] : m_entries) {
        if (!entry.deleted) {
            uint64_t file_time = entry.fileEntry.timestamps.lastWriteTime;
            if (file_time >= startDate && file_time <= endDate) {
                results.push_back(entry.fileEntry);
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    m_queryCount.fetch_add(1);
    m_totalQueryTime.fetch_add(duration.count());

    return results;
}

void InMemoryIndex::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
}

void InMemoryIndex::compact() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->second.deleted) {
            it = m_entries.erase(it);
        } else {
            ++it;
        }
    }
}

IndexStats InMemoryIndex::getStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    IndexStats stats;

    stats.totalEntries = m_entries.size();
    stats.totalSize = 0;
    stats.memtableSize = 0;
    stats.sstableCount = 0;

    uint64_t queryCount = m_queryCount.load();
    uint64_t totalTime = m_totalQueryTime.load();

    if (queryCount > 0) {
        stats.averageQueryTimeMs = static_cast<double>(totalTime) / static_cast<double>(queryCount) / 1000.0;
    } else {
        stats.averageQueryTimeMs = 0.0;
    }

    stats.bloomFilterHitRate = 0.0;

    return stats;
}

void InMemoryIndex::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
    m_queryCount.store(0);
    m_totalQueryTime.store(0);
}
