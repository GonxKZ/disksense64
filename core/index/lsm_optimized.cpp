#include "lsm_optimized.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>

// SSTable implementation
LsmIndexOptimized::SSTable::SSTable(const std::wstring& file_path)
    : m_file_path(file_path) {
}

LsmIndexOptimized::SSTable::~SSTable() {
}

bool LsmIndexOptimized::SSTable::load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Open file for reading
    HANDLE file_handle = CreateFileW(
        m_file_path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Get file size
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size)) {
        CloseHandle(file_handle);
        return false;
    }
    
    // Create file mapping
    HANDLE map_handle = CreateFileMappingW(
        file_handle,
        nullptr,
        PAGE_READONLY,
        file_size.HighPart,
        file_size.LowPart,
        nullptr
    );
    
    if (!map_handle) {
        CloseHandle(file_handle);
        return false;
    }
    
    // Map view of file
    void* mapped_view = MapViewOfFile(
        map_handle,
        FILE_MAP_READ,
        0, 0,
        static_cast<SIZE_T>(file_size.QuadPart)
    );
    
    if (!mapped_view) {
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        return false;
    }
    
    // Read header
    const char* data_ptr = static_cast<const char*>(mapped_view);
    const Header* header = reinterpret_cast<const Header*>(data_ptr);
    
    // Validate header
    if (header->magic != 0x494E4458) { // "INDX"
        UnmapViewOfFile(mapped_view);
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        return false;
    }
    
    m_header = *header;
    data_ptr += sizeof(Header);
    
    // Read index entries
    m_index.clear();
    m_index.reserve(static_cast<size_t>(m_header.entry_count));
    
    const IndexEntry* index_entries = reinterpret_cast<const IndexEntry*>(data_ptr);
    for (uint64_t i = 0; i < m_header.entry_count; ++i) {
        m_index.push_back(index_entries[i]);
    }
    
    // Keep handles for later access
    // In a real implementation, we would need to manage these carefully
    UnmapViewOfFile(mapped_view);
    CloseHandle(map_handle);
    CloseHandle(file_handle);
    
    return true;
}

bool LsmIndexOptimized::SSTable::save(const std::vector<FileEntry>& entries) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (entries.empty()) {
        return false;
    }
    
    // Sort entries by volume_id, file_id_low, file_id_high
    std::vector<FileEntry> sorted_entries = entries;
    std::sort(sorted_entries.begin(), sorted_entries.end(),
              [](const FileEntry& a, const FileEntry& b) {
                  if (a.volume_id != b.volume_id) {
                      return a.volume_id < b.volume_id;
                  }
                  if (a.file_id_low != b.file_id_low) {
                      return a.file_id_low < b.file_id_low;
                  }
                  return a.file_id_high < b.file_id_high;
              });
    
    // Build header
    m_header.magic = 0x494E4458; // "INDX"
    m_header.version = 1;
    m_header.entry_count = sorted_entries.size();
    m_header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    
    // Calculate min/max IDs
    m_header.min_volume_id = sorted_entries.front().volume_id;
    m_header.max_volume_id = sorted_entries.back().volume_id;
    m_header.min_file_id_low = sorted_entries.front().file_id_low;
    m_header.max_file_id_low = sorted_entries.back().file_id_low;
    
    // Calculate file size
    size_t header_size = sizeof(Header);
    size_t index_size = sorted_entries.size() * sizeof(IndexEntry);
    size_t data_size = 0;
    
    // Calculate data size (simplified - in real implementation would serialize entries)
    for (const auto& entry : sorted_entries) {
        data_size += sizeof(FileEntry) + entry.file_path.size() * sizeof(wchar_t);
        // Add signature sizes
        data_size += entry.head_tail_signature.size();
        data_size += entry.full_hash.size();
        data_size += entry.perceptual_hash.size();
        data_size += entry.audio_fingerprint.size();
        // Add chunk info
        data_size += entry.chunks.size() * (sizeof(FileEntry::ChunkInfo) + 32); // 32 for hash
        data_size += entry.min_hash_signature.size();
    }
    
    size_t total_size = header_size + index_size + data_size;
    
    // Create file
    HANDLE file_handle = CreateFileW(
        m_file_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Set file size
    LARGE_INTEGER li_size;
    li_size.QuadPart = total_size;
    if (!SetFilePointerEx(file_handle, li_size, nullptr, FILE_BEGIN) || 
        !SetEndOfFile(file_handle)) {
        CloseHandle(file_handle);
        return false;
    }
    
    // Create file mapping
    HANDLE map_handle = CreateFileMappingW(
        file_handle,
        nullptr,
        PAGE_READWRITE,
        li_size.HighPart,
        li_size.LowPart,
        nullptr
    );
    
    if (!map_handle) {
        CloseHandle(file_handle);
        return false;
    }
    
    // Map view of file
    void* mapped_view = MapViewOfFile(
        map_handle,
        FILE_MAP_WRITE,
        0, 0,
        static_cast<SIZE_T>(total_size)
    );
    
    if (!mapped_view) {
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        return false;
    }
    
    // Write data
    char* write_ptr = static_cast<char*>(mapped_view);
    
    // Write header
    *reinterpret_cast<Header*>(write_ptr) = m_header;
    write_ptr += sizeof(Header);
    
    // Build and write index
    std::vector<IndexEntry> index_entries;
    index_entries.reserve(sorted_entries.size());
    
    size_t data_offset = header_size + index_size;
    for (const auto& entry : sorted_entries) {
        IndexEntry index_entry;
        index_entry.volume_id = entry.volume_id;
        index_entry.file_id_low = entry.file_id_low;
        index_entry.file_id_high = entry.file_id_high;
        index_entry.offset = data_offset;
        
        // Calculate serialized size (simplified)
        size_t entry_size = sizeof(FileEntry) + 
                           entry.file_path.size() * sizeof(wchar_t) +
                           entry.head_tail_signature.size() +
                           entry.full_hash.size() +
                           entry.perceptual_hash.size() +
                           entry.audio_fingerprint.size() +
                           entry.chunks.size() * (sizeof(FileEntry::ChunkInfo) + 32) +
                           entry.min_hash_signature.size();
        
        index_entry.size = static_cast<uint32_t>(entry_size);
        index_entries.push_back(index_entry);
        
        data_offset += entry_size;
    }
    
    // Write index entries
    memcpy(write_ptr, index_entries.data(), index_entries.size() * sizeof(IndexEntry));
    write_ptr += index_entries.size() * sizeof(IndexEntry);
    
    // In a real implementation, we would serialize the entries here
    // For now, we'll just zero-fill the data section
    memset(write_ptr, 0, data_size);
    
    // Unmap and close
    UnmapViewOfFile(mapped_view);
    CloseHandle(map_handle);
    CloseHandle(file_handle);
    
    // Update in-memory index
    m_index = std::move(index_entries);
    
    return true;
}

std::unique_ptr<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::SSTable::get(uint64_t volume_id, 
                              uint64_t file_id_low,
                              uint64_t file_id_high) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Binary search in index
    IndexEntry search_key;
    search_key.volume_id = volume_id;
    search_key.file_id_low = file_id_low;
    search_key.file_id_high = file_id_high;
    
    auto it = std::lower_bound(m_index.begin(), m_index.end(), search_key,
        [](const IndexEntry& a, const IndexEntry& b) {
            if (a.volume_id != b.volume_id) {
                return a.volume_id < b.volume_id;
            }
            if (a.file_id_low != b.file_id_low) {
                return a.file_id_low < b.file_id_low;
            }
            return a.file_id_high < b.file_id_high;
        });
    
    if (it != m_index.end() && 
        it->volume_id == volume_id && 
        it->file_id_low == file_id_low && 
        it->file_id_high == file_id_high) {
        
        // In a real implementation, we would read the actual entry from disk
        // For now, return a dummy entry
        auto entry = std::make_unique<FileEntry>();
        entry->volume_id = volume_id;
        entry->file_id_low = file_id_low;
        entry->file_id_high = file_id_high;
        return entry;
    }
    
    return nullptr;
}

std::vector<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::SSTable::get_by_volume(uint64_t volume_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Find range of entries for this volume
    auto lower = std::lower_bound(m_index.begin(), m_index.end(), volume_id,
        [](const IndexEntry& entry, uint64_t vol_id) {
            return entry.volume_id < vol_id;
        });
    
    auto upper = std::upper_bound(m_index.begin(), m_index.end(), volume_id,
        [](uint64_t vol_id, const IndexEntry& entry) {
            return vol_id < entry.volume_id;
        });
    
    // Collect entries
    for (auto it = lower; it != upper; ++it) {
        // In a real implementation, we would read the actual entries
        FileEntry entry;
        entry.volume_id = it->volume_id;
        entry.file_id_low = it->file_id_low;
        entry.file_id_high = it->file_id_high;
        results.push_back(entry);
    }
    
    return results;
}

std::vector<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::SSTable::get_by_size_range(uint64_t min_size, uint64_t max_size) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // In a real implementation, we would need size information in the index
    // For now, return empty results
    return results;
}

// MemTable implementation
LsmIndexOptimized::MemTable::MemTable()
    : m_size_bytes(0) {
}

void LsmIndexOptimized::MemTable::put(const FileEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Entry mem_entry(entry.volume_id, entry.file_id_low, entry.file_id_high, entry);
    
    // Check if entry already exists
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), mem_entry);
    if (it != m_entries.end() && 
        it->volume_id == entry.volume_id &&
        it->file_id_low == entry.file_id_low &&
        it->file_id_high == entry.file_id_high) {
        
        // Update existing entry
        m_size_bytes -= sizeof(Entry) + it->file_entry.logical_size;
        *it = mem_entry;
    } else {
        // Insert new entry
        m_entries.insert(it, mem_entry);
    }
    
    m_size_bytes += sizeof(Entry) + entry.logical_size;
}

void LsmIndexOptimized::MemTable::remove(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Entry search_entry(volume_id, file_id_low, file_id_high, FileEntry(), true);
    
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), search_entry);
    if (it != m_entries.end() && 
        it->volume_id == volume_id &&
        it->file_id_low == file_id_low &&
        it->file_id_high == file_id_high) {
        
        m_size_bytes -= sizeof(Entry) + it->file_entry.logical_size;
        m_entries.erase(it);
    }
}

std::unique_ptr<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::MemTable::get(uint64_t volume_id, 
                                uint64_t file_id_low,
                                uint64_t file_id_high) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Entry search_entry(volume_id, file_id_low, file_id_high, FileEntry());
    
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), search_entry);
    if (it != m_entries.end() && 
        it->volume_id == volume_id &&
        it->file_id_low == file_id_low &&
        it->file_id_high == file_id_high &&
        !it->deleted) {
        
        return std::make_unique<FileEntry>(it->file_entry);
    }
    
    return nullptr;
}

bool LsmIndexOptimized::MemTable::flush_to_sstable(const std::wstring& file_path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_entries.empty()) {
        return false;
    }
    
    // Extract file entries
    std::vector<FileEntry> entries;
    entries.reserve(m_entries.size());
    
    for (const auto& mem_entry : m_entries) {
        if (!mem_entry.deleted) {
            entries.push_back(mem_entry.file_entry);
        }
    }
    
    if (entries.empty()) {
        return false;
    }
    
    // Create SSTable
    SSTable sstable(file_path);
    return sstable.save(entries);
}

void LsmIndexOptimized::MemTable::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
    m_size_bytes = 0;
}

// BloomFilter implementation
LsmIndexOptimized::BloomFilter::BloomFilter(size_t expected_elements, double false_positive_rate)
    : m_element_count(0) {
    
    // Calculate optimal parameters
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
        false_positive_rate = 0.01; // 1%
    }
    
    // Number of bits: m = -n*ln(p) / (ln(2)^2)
    double ln2_squared = std::log(2.0) * std::log(2.0);
    m_bit_count = static_cast<size_t>(-static_cast<double>(expected_elements) * 
                                     std::log(false_positive_rate) / ln2_squared);
    
    // Round up to nearest multiple of 64 for efficient operations
    m_bit_count = ((m_bit_count + 63) / 64) * 64;
    
    // Number of hash functions: k = (m/n) * ln(2)
    m_hash_count = static_cast<size_t>(static_cast<double>(m_bit_count) / 
                                      static_cast<double>(expected_elements) * std::log(2.0));
    
    // Ensure reasonable bounds
    if (m_hash_count < 1) m_hash_count = 1;
    if (m_hash_count > 20) m_hash_count = 20;
    
    // Allocate bit array
    m_bits.resize((m_bit_count + 63) / 64, 0);
}

void LsmIndexOptimized::BloomFilter::add(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) {
    uint64_t hash1_val = hash1(volume_id, file_id_low, file_id_high);
    uint64_t hash2_val = hash2(volume_id, file_id_low, file_id_high);
    
    for (size_t i = 0; i < m_hash_count; ++i) {
        uint64_t combined_hash = hash1_val + i * hash2_val;
        uint64_t bit_index = combined_hash % m_bit_count;
        uint64_t word_index = bit_index / 64;
        uint64_t bit_position = bit_index % 64;
        
        m_bits[word_index] |= (1ULL << bit_position);
    }
    
    m_element_count.fetch_add(1);
}

bool LsmIndexOptimized::BloomFilter::might_contain(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) const {
    uint64_t hash1_val = hash1(volume_id, file_id_low, file_id_high);
    uint64_t hash2_val = hash2(volume_id, file_id_low, file_id_high);
    
    for (size_t i = 0; i < m_hash_count; ++i) {
        uint64_t combined_hash = hash1_val + i * hash2_val;
        uint64_t bit_index = combined_hash % m_bit_count;
        uint64_t word_index = bit_index / 64;
        uint64_t bit_position = bit_index % 64;
        
        if ((m_bits[word_index] & (1ULL << bit_position)) == 0) {
            return false; // Definitely not in set
        }
    }
    
    return true; // Might be in set (false positive possible)
}

double LsmIndexOptimized::BloomFilter::get_false_positive_rate() const {
    size_t n = m_element_count.load();
    if (n == 0 || m_bit_count == 0) {
        return 0.0;
    }
    
    // p = (1 - e^(-kn/m))^k
    double exp_val = std::exp(-static_cast<double>(m_hash_count * n) / static_cast<double>(m_bit_count));
    return std::pow(1.0 - exp_val, static_cast<double>(m_hash_count));
}

std::vector<uint8_t> LsmIndexOptimized::BloomFilter::serialize() const {
    std::vector<uint8_t> data;
    data.resize(sizeof(m_bit_count) + sizeof(m_hash_count) + sizeof(size_t) + 
                m_bits.size() * sizeof(uint64_t));
    
    size_t offset = 0;
    
    // Serialize parameters
    memcpy(data.data() + offset, &m_bit_count, sizeof(m_bit_count));
    offset += sizeof(m_bit_count);
    
    memcpy(data.data() + offset, &m_hash_count, sizeof(m_hash_count));
    offset += sizeof(m_hash_count);
    
    size_t element_count = m_element_count.load();
    memcpy(data.data() + offset, &element_count, sizeof(element_count));
    offset += sizeof(element_count);
    
    // Serialize bit array
    memcpy(data.data() + offset, m_bits.data(), m_bits.size() * sizeof(uint64_t));
    
    return data;
}

bool LsmIndexOptimized::BloomFilter::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(m_bit_count) + sizeof(m_hash_count) + sizeof(size_t)) {
        return false;
    }
    
    size_t offset = 0;
    
    // Deserialize parameters
    memcpy(&m_bit_count, data.data() + offset, sizeof(m_bit_count));
    offset += sizeof(m_bit_count);
    
    memcpy(&m_hash_count, data.data() + offset, sizeof(m_hash_count));
    offset += sizeof(m_hash_count);
    
    size_t element_count;
    memcpy(&element_count, data.data() + offset, sizeof(element_count));
    offset += sizeof(element_count);
    
    m_element_count.store(element_count);
    
    // Deserialize bit array
    size_t bit_array_size = (m_bit_count + 63) / 64;
    if (data.size() < offset + bit_array_size * sizeof(uint64_t)) {
        return false;
    }
    
    m_bits.resize(bit_array_size);
    memcpy(m_bits.data(), data.data() + offset, bit_array_size * sizeof(uint64_t));
    
    return true;
}

uint64_t LsmIndexOptimized::BloomFilter::hash1(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) {
    // Simple hash combination
    uint64_t hash = volume_id;
    hash = hash * 31 + file_id_low;
    hash = hash * 31 + file_id_high;
    hash = hash * 0x9E3779B185EBCA87ULL;
    return hash;
}

uint64_t LsmIndexOptimized::BloomFilter::hash2(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) {
    // Different hash combination
    uint64_t hash = file_id_high;
    hash = hash * 37 + file_id_low;
    hash = hash * 37 + volume_id;
    hash = hash * 0xC2B2AE3D27D4EB4FULL;
    return hash;
}

// LsmIndexOptimized implementation
LsmIndexOptimized::LsmIndexOptimized(const std::wstring& index_path, size_t memtable_size_limit)
    : m_index_path(index_path), m_memtable_size_limit(memtable_size_limit),
      m_compaction_running(false) {
    
    // Create directory if it doesn't exist
    CreateDirectoryW(index_path.c_str(), nullptr);
    
    // Initialize memtable
    m_memtable = std::make_unique<MemTable>();
    
    // Initialize SSTable levels
    m_sstables.resize(5); // 5 levels initially
    
    // Initialize bloom filters
    for (size_t i = 0; i < m_sstables.size(); ++i) {
        m_bloom_filters.push_back(std::make_unique<BloomFilter>(1000000)); // 1M elements expected
    }
}

LsmIndexOptimized::~LsmIndexOptimized() {
    stop_compaction();
    flush();
}

void LsmIndexOptimized::put(const FileEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Add to memtable
    m_memtable->put(entry);
    m_stats.total_writes.fetch_add(1);
    
    // Check if flush is needed
    if (m_memtable->get_size_bytes() >= m_memtable_size_limit) {
        flush();
    }
}

void LsmIndexOptimized::remove(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Mark as deleted in memtable
    m_memtable->remove(volume_id, file_id_low, file_id_high);
    m_stats.total_writes.fetch_add(1);
    
    // Check if flush is needed
    if (m_memtable->get_size_bytes() >= m_memtable_size_limit) {
        flush();
    }
}

std::unique_ptr<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::get(uint64_t volume_id, 
                       uint64_t file_id_low,
                       uint64_t file_id_high) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check bloom filter first (optimize for negative cases)
    if (m_bloom_filters[0] && 
        !m_bloom_filters[0]->might_contain(volume_id, file_id_low, file_id_high)) {
        m_stats.bloom_filter_hits.fetch_add(1);
        return nullptr; // Definitely not in index
    } else {
        m_stats.bloom_filter_misses.fetch_add(1);
    }
    
    // Check memtable first (most recent data)
    auto result = m_memtable->get(volume_id, file_id_low, file_id_high);
    if (result) {
        m_stats.total_reads.fetch_add(1);
        return result;
    }
    
    // Check immutable memtable
    if (m_immutable_memtable) {
        result = m_immutable_memtable->get(volume_id, file_id_low, file_id_high);
        if (result) {
            m_stats.total_reads.fetch_add(1);
            return result;
        }
    }
    
    // Check SSTables (newest to oldest)
    for (int level = static_cast<int>(m_sstables.size()) - 1; level >= 0; --level) {
        for (auto it = m_sstables[level].rbegin(); it != m_sstables[level].rend(); ++it) {
            // Check level bloom filter
            if (level < static_cast<int>(m_bloom_filters.size()) && m_bloom_filters[level] &&
                !m_bloom_filters[level]->might_contain(volume_id, file_id_low, file_id_high)) {
                m_stats.bloom_filter_hits.fetch_add(1);
                continue; // Skip this SSTable
            } else {
                m_stats.bloom_filter_misses.fetch_add(1);
            }
            
            result = (*it)->get(volume_id, file_id_low, file_id_high);
            if (result) {
                m_stats.total_reads.fetch_add(1);
                m_stats.sstable_reads.fetch_add(1);
                return result;
            }
        }
    }
    
    m_stats.total_reads.fetch_add(1);
    return nullptr;
}

std::vector<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::get_by_volume(uint64_t volume_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Collect from all sources
    // This is a simplified implementation
    
    return results;
}

std::vector<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::get_by_size_range(uint64_t min_size, uint64_t max_size) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Collect from all sources
    // This is a simplified implementation
    
    return results;
}

std::vector<LsmIndexOptimized::FileEntry> 
LsmIndexOptimized::get_similar_files(const std::vector<uint8_t>& perceptual_hash,
                                    size_t max_results) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileEntry> results;
    
    // Find similar files using perceptual hash
    // This is a simplified implementation
    
    return results;
}

void LsmIndexOptimized::flush() {
    std::lock_guard<std::mutex> flush_lock(m_flush_mutex);
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // If memtable is empty, nothing to flush
    if (m_memtable->empty()) {
        return;
    }
    
    // Move memtable to immutable
    m_immutable_memtable = std::move(m_memtable);
    m_memtable = std::make_unique<MemTable>();
    
    // Generate SSTable filename
    std::wstring sstable_file = generate_sstable_filename(0, 0);
    
    // Flush to SSTable
    if (m_immutable_memtable->flush_to_sstable(sstable_file)) {
        // Add to level 0
        auto sstable = std::make_unique<SSTable>(sstable_file);
        if (sstable->load()) {
            m_sstables[0].push_back(std::move(sstable));
            
            // Update bloom filter
            update_bloom_filter(*m_sstables[0].back(), 0);
        }
    }
    
    // Clear immutable memtable
    m_immutable_memtable.reset();
}

void LsmIndexOptimized::compact() {
    // Trigger manual compaction
    for (size_t level = 0; level < m_sstables.size() - 1; ++level) {
        if (is_compaction_needed()) {
            merge_sstables(static_cast<int>(level));
        }
    }
}

void LsmIndexOptimized::start_compaction() {
    if (m_compaction_running.exchange(true)) {
        return; // Already running
    }
    
    m_compaction_thread = std::make_unique<std::thread>(&LsmIndexOptimized::compaction_worker, this);
}

void LsmIndexOptimized::stop_compaction() {
    if (!m_compaction_running.exchange(false)) {
        return; // Not running
    }
    
    if (m_compaction_thread && m_compaction_thread->joinable()) {
        m_compaction_thread->join();
    }
    m_compaction_thread.reset();
}

LsmIndexOptimized::IndexStats LsmIndexOptimized::get_stats() const {
    IndexStats stats;
    
    stats.total_reads = m_stats.total_reads.load();
    stats.total_writes = m_stats.total_writes.load();
    stats.total_sstables = 0;
    
    for (const auto& level : m_sstables) {
        stats.total_sstables += level.size();
    }
    
    stats.memtable_size = m_memtable ? m_memtable->get_size_bytes() : 0;
    
    uint64_t total_hits = m_stats.bloom_filter_hits.load();
    uint64_t total_misses = m_stats.bloom_filter_misses.load();
    if (total_hits + total_misses > 0) {
        stats.bloom_filter_hit_rate = static_cast<double>(total_hits) / 
                                      static_cast<double>(total_hits + total_misses);
    }
    
    return stats;
}

void LsmIndexOptimized::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_memtable->clear();
    m_immutable_memtable.reset();
    
    for (auto& level : m_sstables) {
        level.clear();
    }
    
    for (auto& filter : m_bloom_filters) {
        if (filter) {
            // Reset bloom filter with same parameters
            size_t current_bit_count = filter->serialize().size() * 8; // Approximate
            filter = std::make_unique<BloomFilter>(1000000); // Reset with same capacity
        }
    }
}

bool LsmIndexOptimized::is_healthy() const {
    // Check if index is healthy
    return true; // Simplified implementation
}

std::wstring LsmIndexOptimized::generate_sstable_filename(int level, int number) const {
    wchar_t filename[256];
    swprintf_s(filename, L"%s\\sstable_%d_%d.dat", 
               m_index_path.c_str(), level, number);
    return std::wstring(filename);
}

void LsmIndexOptimized::merge_sstables(int level) {
    if (level < 0 || level >= static_cast<int>(m_sstables.size()) - 1) {
        return;
    }
    
    // Merge SSTables from current level to next level
    // This is a simplified implementation
    
    m_stats.total_compactions.fetch_add(1);
}

void LsmIndexOptimized::compaction_worker() {
    while (m_compaction_running) {
        if (is_compaction_needed()) {
            compact();
        }
        
        // Sleep for a while before checking again
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

bool LsmIndexOptimized::is_compaction_needed() const {
    // Check if compaction is needed
    // For example, if level 0 has too many SSTables
    
    return m_sstables[0].size() >= 4; // Arbitrary threshold
}

void LsmIndexOptimized::update_bloom_filter(const SSTable& sstable, int level) {
    if (level < 0 || level >= static_cast<int>(m_bloom_filters.size())) {
        return;
    }
    
    // Update bloom filter with entries from SSTable
    // This is a simplified implementation
}