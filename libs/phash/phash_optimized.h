#ifndef LIBS_PHASH_PHASH_OPTIMIZED_H
#define LIBS_PHASH_PHASH_OPTIMIZED_H

#include <cstdint>
#include <cstddef>
#include <memory>

// Optimized perceptual hash for images
class PHashOptimized {
public:
    static constexpr size_t HASH_SIZE = 64; // 64-bit hash
    static constexpr size_t DCT_SIZE = 32;  // 32x32 DCT
    static constexpr size_t LOW_FREQ_SIZE = 8; // 8x8 low frequency components
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
public:
    enum class SimdLevel {
        NONE,      // Portable C implementation
        SSE2,      // SSE2 instructions
        AVX2,      // AVX2 instructions
        AVX512     // AVX-512 instructions
    };
    
    PHashOptimized();
    ~PHashOptimized();
    
    // Compute pHash for image data (grayscale)
    bool compute_phash(const uint8_t* image_data, 
                       size_t width, size_t height,
                       uint64_t& hash_out);
    
    // Compute pHash for image file
    bool compute_phash_file(const char* image_path, uint64_t& hash_out);
    
    // Compute pHash with custom parameters
    struct Parameters {
        size_t dct_size;           // DCT size (16, 32, 64)
        size_t low_freq_size;      // Low frequency components to use
        bool normalize_brightness; // Normalize brightness before computing DCT
        bool normalize_contrast;   // Normalize contrast before computing DCT
        
        Parameters()
            : dct_size(DCT_SIZE), low_freq_size(LOW_FREQ_SIZE),
              normalize_brightness(true), normalize_contrast(true) {}
    };
    
    bool compute_phash_with_params(const uint8_t* image_data,
                                   size_t width, size_t height,
                                   const Parameters& params,
                                   uint64_t& hash_out);
    
    // Batch computation for multiple images
    struct ImageBatch {
        const uint8_t* image_data;
        size_t width;
        size_t height;
        uint64_t* hash_out; // Output hash
    };
    
    size_t compute_phash_batch(const ImageBatch* images, size_t count);
    
    // Compare two hashes (Hamming distance)
    static int compare_hashes(uint64_t hash1, uint64_t hash2);
    
    // Check similarity (threshold in Hamming distance)
    static bool is_similar(uint64_t hash1, uint64_t hash2, int threshold = 5);
    
    // Get detected SIMD level
    SimdLevel get_simd_level() const;
    
private:
    // Detect available SIMD instructions
    static SimdLevel detect_simd();
    
    // Resize image to target size
    static bool resize_image(const uint8_t* src_data, size_t src_width, size_t src_height,
                           uint8_t* dst_data, size_t dst_width, size_t dst_height);
    
    // Convert to grayscale if needed
    static bool convert_to_grayscale(const uint8_t* src_data, size_t width, size_t height,
                                   int channels, uint8_t* gray_data);
    
    // Normalize image brightness and contrast
    static void normalize_image(uint8_t* image_data, size_t width, size_t height);
    
    // Compute 2D DCT
    static bool compute_dct(const uint8_t* image_data, size_t width, size_t height,
                          double* dct_coefficients);
    
    // Extract low frequency components and compute hash
    static bool extract_hash_from_dct(const double* dct_coefficients, 
                                     size_t dct_size,
                                     size_t low_freq_size,
                                     uint64_t& hash_out);
};

// Locality Sensitive Hashing for perceptual similarity
class LSHIndex {
public:
    struct Candidate {
        size_t image_id;
        uint64_t hash;
        double similarity_score;
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
public:
    // LSH parameters
    struct Parameters {
        size_t num_hash_tables;    // Number of hash tables
        size_t num_hash_functions; // Number of hash functions per table
        size_t bucket_size;        // Maximum items per bucket
        int similarity_threshold;  // Hamming distance threshold
        
        Parameters()
            : num_hash_tables(16), num_hash_functions(8),
              bucket_size(100), similarity_threshold(5) {}
    };
    
    LSHIndex(const Parameters& params = Parameters());
    ~LSHIndex();
    
    // Add image hash to index
    bool add_hash(size_t image_id, uint64_t hash);
    
    // Add multiple hashes
    bool add_hashes(const size_t* image_ids, const uint64_t* hashes, size_t count);
    
    // Query similar images
    size_t query_similar(uint64_t query_hash, Candidate* candidates, size_t max_candidates);
    
    // Query similar images with custom threshold
    size_t query_similar_with_threshold(uint64_t query_hash, int threshold,
                                      Candidate* candidates, size_t max_candidates);
    
    // Remove image from index
    bool remove_hash(size_t image_id);
    
    // Clear index
    void clear();
    
    // Get index statistics
    struct Statistics {
        size_t total_images;
        size_t total_buckets;
        size_t average_bucket_size;
        double load_factor;
    };
    
    Statistics get_statistics() const;
    
    // Save index to file
    bool save_to_file(const char* filepath) const;
    
    // Load index from file
    bool load_from_file(const char* filepath);
};

#endif // LIBS_PHASH_PHASH_OPTIMIZED_H