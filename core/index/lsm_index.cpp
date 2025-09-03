#include "lsm_index.h"
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
#include "libs/utils/utils.h"

// LSMIndex implementation
LSMIndex::LSMIndex(const std::string& indexPath, size_t memtableSize)
    : m_impl(std::make_unique<LSMIndexImpl>(indexPath, memtableSize)) {
}

LSMIndex::~LSMIndex() = default;

LSMIndex::LSMIndex(LSMIndex&& other) noexcept = default;
LSMIndex& LSMIndex::operator=(LSMIndex&& other) noexcept = default;

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
    // TODO: Implement proper range query
    return std::vector<FileEntry>();
}

void LSMIndex::flush() {
    m_impl->flush();
}

void LSMIndex::compact() {
    m_impl->compact();
}

void LSMIndex::startCompaction() {
    m_impl->startCompaction();
}

void LSMIndex::stopCompaction() {
    m_impl->stopCompaction();
}