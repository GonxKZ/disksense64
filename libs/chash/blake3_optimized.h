#ifndef LIBS_CHASH_BLAKE3_OPTIMIZED_H
#define LIBS_CHASH_BLAKE3_OPTIMIZED_H

#include <cstdint>
#include <cstddef>
#include <memory>

// Optimized BLAKE3 implementation with SIMD detection
class Blake3Optimized {
public:
    static constexpr size_t HASH_SIZE = 32;
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t CHUNK_SIZE = 1024;
    
    enum class SimdLevel {
        NONE,      // Portable C implementation
        SSE41,     // SSE4.1 instructions
        AVX2,      // AVX2 instructions
        AVX512     // AVX-512 instructions
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    SimdLevel m_simd_level;
    
public:
    Blake3Optimized();
    ~Blake3Optimized();
    
    // Get detected SIMD level
    SimdLevel get_simd_level() const { return m_simd_level; }
    
    // Initialize hash state
    void init();
    
    // Update hash with data
    void update(const void* data, size_t len);
    
    // Finalize hash
    void finalize(uint8_t* out, size_t out_len = HASH_SIZE);
    
    // One-shot hash
    static void hash(const void* data, size_t len, uint8_t* out, size_t out_len = HASH_SIZE);
    
    // Batch hashing for multiple inputs
    struct BatchInput {
        const void* data;
        size_t len;
        uint8_t* out;  // Output buffer (HASH_SIZE bytes)
    };
    
    static void hash_batch(const BatchInput* inputs, size_t count);
    
private:
    // Detect available SIMD instructions
    static SimdLevel detect_simd();
};

// Content Defined Chunking (CDC) with rolling hash
class ContentDefinedChunker {
public:
    struct Chunk {
        const uint8_t* data;
        size_t size;
        uint64_t hash;  // Rolling hash for quick comparison
    };
    
    // CDC parameters
    struct Parameters {
        uint32_t window_size;      // Rolling window size
        uint32_t mask_bits;        // Normalized cut mask bits
        size_t min_chunk_size;     // Minimum chunk size
        size_t max_chunk_size;     // Maximum chunk size
        size_t target_chunk_size; // Target chunk size
        
        Parameters()
            : window_size(48), mask_bits(0x00031FFF),  // 18-bit mask for ~256KB chunks
              min_chunk_size(128 * 1024),              // 128KB minimum
              max_chunk_size(4 * 1024 * 1024),         // 4MB maximum
              target_chunk_size(256 * 1024) {}          // 256KB target
    };
    
private:
    Parameters m_params;
    uint32_t m_rolling_hash;
    size_t m_current_chunk_size;
    size_t m_window_pos;
    
public:
    ContentDefinedChunker(const Parameters& params = Parameters());
    ~ContentDefinedChunker() = default;
    
    // Process data and emit chunks
    template<typename ChunkCallback>
    void process_data(const uint8_t* data, size_t len, ChunkCallback callback) {
        size_t pos = 0;
        while (pos < len) {
            uint8_t byte = data[pos];
            process_byte(byte, pos, data, len, callback);
            pos++;
        }
    }
    
    // Reset chunker state
    void reset();
    
    // Get current parameters
    const Parameters& get_parameters() const { return m_params; }
    
    // Set new parameters
    void set_parameters(const Parameters& params) { m_params = params; }
    
private:
    template<typename ChunkCallback>
    void process_byte(uint8_t byte, size_t pos, const uint8_t* data, size_t len, 
                     ChunkCallback callback);
    
    // Update rolling hash with new byte
    void update_rolling_hash(uint8_t byte);
    
    // Check if we should cut a chunk at current position
    bool should_cut_chunk() const;
};

// MinHash for chunk sets
class ChunkSetMinHash {
public:
    static constexpr size_t SIGNATURE_SIZE = 128; // 128-bit signature
    
private:
    uint64_t m_signature[SIGNATURE_SIZE / 64]; // 2 64-bit values
    
public:
    ChunkSetMinHash();
    ~ChunkSetMinHash() = default;
    
    // Add chunk hash to signature
    void add_chunk_hash(uint64_t chunk_hash);
    
    // Add multiple chunk hashes
    void add_chunk_hashes(const uint64_t* hashes, size_t count);
    
    // Get final signature
    void get_signature(uint8_t* out, size_t out_len = SIGNATURE_SIZE / 8) const;
    
    // Calculate similarity between two signatures (0.0 to 1.0)
    static double calculate_similarity(const ChunkSetMinHash& a, const ChunkSetMinHash& b);
    
    // Reset signature
    void reset();
};

#endif // LIBS_CHASH_BLAKE3_OPTIMIZED_H