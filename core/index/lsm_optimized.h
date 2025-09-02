#ifndef CORE_INDEX_LSM_OPTIMIZED_H
#define CORE_INDEX_LSM_OPTIMIZED_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

// Optimized LSM (Log-Structured Merge) Tree index
class LsmIndexOptimized {
public:
    // File entry structure
    struct FileEntry {
        uint64_t volume_id;
        uint64_t file_id_low;
        uint64_t file_id_high;
        std::wstring file_path;
        uint64_t logical_size;
        uint64_t on_disk_size;
        uint64_t creation_time;
        uint64_t last_write_time;
        uint64_t last_access_time;
        uint32_t attributes;
        
        // Signatures
        std::vector<uint8_t> head_tail_signature; // 32KB head+tail hash
        std::vector<uint8_t> full_hash;          // BLAKE3/SHA-256 full hash
        std::vector<uint8_t> perceptual_hash;     // pHash for images/audio
        std::vector<uint8_t> audio_fingerprint;   // Audio fingerprint
        
        // Chunk information for CDC
        struct ChunkInfo {
            uint64_t offset;
            uint64_t size;
            std::vector<uint8_t> hash; // Chunk hash
            
            ChunkInfo(uint64_t off, uint64_t sz) : offset(off), size(sz) {}
        };
        
        std::vector<ChunkInfo> chunks;
        std::vector<uint8_t> min_hash_signature; // Min-hash of chunk set
        
        FileEntry() 
            : volume_id(0), file_id_low(0), file_id_high(0),
              logical_size(0), on_disk_size(0),
              creation_time(0), last_write_time(0), last_access_time(0),
              attributes(0) {}
    };
    
    // SSTable (Sorted String Table) structure
    class SSTable {
    public:
        struct Header {
            uint32_t magic;
            uint32_t version;
            uint64_t entry_count;
            uint64_t min_volume_id;
            uint64_t max_volume_id;
            uint64_t min_file_id_low;
            uint64_t max_file_id_low;
            uint64_t timestamp; // Creation timestamp
            
            Header() 
                : magic(0x494E4458), version(1), entry_count(0),
                  min_volume_id(0), max_volume_id(0),
                  min_file_id_low(0), max_file_id_low(0),
                  timestamp(0) {}
        };
        
        struct IndexEntry {
            uint64_t volume_id;
            uint64_t file_id_low;
            uint64_t file_id_high;
            uint64_t offset;
            uint32_t size;
            
            IndexEntry() 
                : volume_id(0), file_id_low(0), file_id_high(0),
                  offset(0), size(0) {}
        };
        
    private:
        std::wstring m_file_path;
        Header m_header;
        std::vector<IndexEntry> m_index;
        mutable std::mutex m_mutex;
        
    public:
        SSTable(const std::wstring& file_path);
        ~SSTable();
        
        // Load SSTable from file
        bool load();
        
        // Save SSTable to file
        bool save(const std::vector<FileEntry>& entries);
        
        // Get file entry
        std::unique_ptr<FileEntry> get(uint64_t volume_id, 
                                      uint64_t file_id_low,
                                      uint64_t file_id_high) const;
        
        // Range queries
        std::vector<FileEntry> get_by_volume(uint64_t volume_id) const;
        std::vector<FileEntry> get_by_size_range(uint64_t min_size, uint64_t max_size) const;
        
        // Accessors
        const Header& get_header() const { return m_header; }
        const std::vector<IndexEntry>& get_index() const { return m_index; }
        uint64_t get_min_volume_id() const { return m_header.min_volume_id; }
        uint64_t get_max_volume_id() const { return m_header.max_volume_id; }
    };
    
    // MemTable (in-memory sorted table)
    class MemTable {
    public:
        struct Entry {
            uint64_t volume_id;
            uint64_t file_id_low;
            uint64_t file_id_high;
            FileEntry file_entry;
            bool deleted;
            uint64_t timestamp;
            
            Entry(uint64_t vol_id, uint64_t fid_low, uint64_t fid_high,
                  const FileEntry& entry, bool del = false)
                : volume_id(vol_id), file_id_low(fid_low), file_id_high(fid_high),
                  file_entry(entry), deleted(del), timestamp(0) {}
            
            // Comparison operator for sorting
            bool operator<(const Entry& other) const {
                if (volume_id != other.volume_id) {
                    return volume_id < other.volume_id;
                }
                if (file_id_low != other.file_id_low) {
                    return file_id_low < other.file_id_low;
                }
                return file_id_high < other.file_id_high;
            }
        };
        
    private:
        std::vector<Entry> m_entries;
        size_t m_size_bytes;
        mutable std::mutex m_mutex;
        
    public:
        MemTable();
        ~MemTable() = default;
        
        // Add file entry
        void put(const FileEntry& entry);
        
        // Mark file as deleted
        void remove(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high);
        
        // Get file entry
        std::unique_ptr<FileEntry> get(uint64_t volume_id, 
                                      uint64_t file_id_low,
                                      uint64_t file_id_high) const;
        
        // Flush to SSTable
        bool flush_to_sstable(const std::wstring& file_path) const;
        
        // Get size in bytes
        size_t get_size_bytes() const { return m_size_bytes; }
        
        // Check if empty
        bool empty() const { return m_entries.empty(); }
        
        // Get entry count
        size_t size() const { return m_entries.size(); }
        
        // Clear table
        void clear();
    };
    
    // Bloom filter for probabilistic membership testing
    class BloomFilter {
    private:
        std::vector<uint64_t> m_bits;
        size_t m_bit_count;
        size_t m_hash_count;
        std::atomic<size_t> m_element_count;
        
    public:
        BloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
        ~BloomFilter() = default;
        
        // Add element to filter
        void add(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high);
        
        // Check if element might be in set
        bool might_contain(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high) const;
        
        // Get estimated false positive rate
        double get_false_positive_rate() const;
        
        // Get element count
        size_t get_element_count() const { return m_element_count.load(); }
        
        // Serialize to buffer
        std::vector<uint8_t> serialize() const;
        
        // Deserialize from buffer
        bool deserialize(const std::vector<uint8_t>& data);
        
    private:
        // Hash functions
        static uint64_t hash1(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high);
        static uint64_t hash2(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high);
    };
    
private:
    std::wstring m_index_path;
    size_t m_memtable_size_limit;
    
    // Current memtable
    std::unique_ptr<MemTable> m_memtable;
    
    // Immutable memtable being flushed
    std::unique_ptr<MemTable> m_immutable_memtable;
    
    // SSTables organized by level (0 = newest, higher = older/merged)
    std::vector<std::vector<std::unique_ptr<SSTable>>> m_sstables;
    
    // Bloom filters for each SSTable level
    std::vector<std::unique_ptr<BloomFilter>> m_bloom_filters;
    
    // Memory-mapped file handles and views
    mutable std::map<std::wstring, HANDLE> m_mapped_handles;
    mutable std::map<std::wstring, void*> m_mapped_views;
    
    // Compaction state
    std::atomic<bool> m_compaction_running;
    std::unique_ptr<std::thread> m_compaction_thread;
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> total_reads;
        std::atomic<uint64_t> total_writes;
        std::atomic<uint64_t> total_compactions;
        std::atomic<uint64_t> bloom_filter_hits;
        std::atomic<uint64_t> bloom_filter_misses;
        std::atomic<uint64_t> sstable_reads;
        
        Stats() 
            : total_reads(0), total_writes(0), total_compactions(0),
              bloom_filter_hits(0), bloom_filter_misses(0), sstable_reads(0) {}
    };
    
    Stats m_stats;
    
    // Synchronization
    mutable std::mutex m_mutex;
    mutable std::mutex m_flush_mutex;
    
public:
    LsmIndexOptimized(const std::wstring& index_path, size_t memtable_size_limit = 64 * 1024 * 1024); // 64MB
    ~LsmIndexOptimized();
    
    // Put file entry
    void put(const FileEntry& entry);
    
    // Remove file entry
    void remove(uint64_t volume_id, uint64_t file_id_low, uint64_t file_id_high);
    
    // Get file entry
    std::unique_ptr<FileEntry> get(uint64_t volume_id, 
                                  uint64_t file_id_low,
                                  uint64_t file_id_high) const;
    
    // Range queries
    std::vector<FileEntry> get_by_volume(uint64_t volume_id) const;
    std::vector<FileEntry> get_by_size_range(uint64_t min_size, uint64_t max_size) const;
    std::vector<FileEntry> get_similar_files(const std::vector<uint8_t>& perceptual_hash,
                                            size_t max_results = 100) const;
    
    // Flush memtable to disk
    void flush();
    
    // Trigger manual compaction
    void compact();
    
    // Start background compaction
    void start_compaction();
    
    // Stop background compaction
    void stop_compaction();
    
    // Get index statistics
    struct IndexStats {
        uint64_t total_entries;
        uint64_t total_sstables;
        uint64_t memtable_size;
        uint64_t total_reads;
        uint64_t total_writes;
        double bloom_filter_hit_rate;
        double average_sstable_read_time_ms;
        
        IndexStats() 
            : total_entries(0), total_sstables(0), memtable_size(0),
              total_reads(0), total_writes(0), 
              bloom_filter_hit_rate(0.0), average_sstable_read_time_ms(0.0) {}
    };
    
    IndexStats get_stats() const;
    
    // Clear index
    void clear();
    
    // Check index health
    bool is_healthy() const;
    
private:
    // Generate SSTable filename
    std::wstring generate_sstable_filename(int level, int number) const;
    
    // Merge SSTables at specific level
    void merge_sstables(int level);
    
    // Create memory-mapped file
    HANDLE create_mapped_file(const std::wstring& filename, size_t size);
    
    // Map view of file
    void* map_view_of_file(HANDLE handle, size_t size);
    
    // Unmap view of file
    void unmap_view_of_file(const std::wstring& filename);
    
    // Compact worker thread
    void compaction_worker();
    
    // Check if compaction is needed
    bool is_compaction_needed() const;
    
    // Update bloom filter for SSTable
    void update_bloom_filter(const SSTable& sstable, int level);
};

#endif // CORE_INDEX_LSM_OPTIMIZED_H