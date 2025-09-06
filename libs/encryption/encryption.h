#ifndef LIBS_ENCRYPTION_ENCRYPTION_H
#define LIBS_ENCRYPTION_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Encryption detection result
typedef struct {
    char* file_path;
    int is_encrypted;
    int is_compressed;
    double entropy;
    char* cipher_type;
    char* encryption_algorithm;
    int key_length;
    char* mode_of_operation;
    int is_password_protected;
    double confidence;
} encryption_result_t;

// Cipher information
typedef struct {
    char* name;
    char* description;
    int block_size;
    int key_sizes[8];  // Supported key sizes in bits
    int key_size_count;
    char* modes[8];    // Supported modes of operation
    int mode_count;
} cipher_info_t;

// Encryption analysis options
typedef struct {
    int check_entropy;         // Check entropy for randomness
    int detect_ciphers;        // Detect specific ciphers
    int detect_compression;    // Detect compression
    int check_headers;         // Check file headers
    int deep_analysis;         // Perform deep analysis
    double entropy_threshold;  // Entropy threshold for detection (0.0-1.0)
    size_t sample_size;        // Sample size for analysis in bytes
    char* password;            // Password for testing decryption
} encryption_options_t;

// Initialize encryption result
void encryption_result_init(encryption_result_t* result);

// Free encryption result
void encryption_result_free(encryption_result_t* result);

// Initialize encryption options with default values
void encryption_options_init(encryption_options_t* options);

// Free encryption options
void encryption_options_free(encryption_options_t* options);

// Initialize cipher info
void cipher_info_init(cipher_info_t* info);

// Free cipher info
void cipher_info_free(cipher_info_t* info);

// Analyze file for encryption
// file_path: path to the file
// options: analysis options
// result: output analysis result (must be freed with encryption_result_free)
// Returns 0 on success, non-zero on error
int encryption_analyze_file(const char* file_path,
                          const encryption_options_t* options,
                          encryption_result_t* result);

// Analyze data in memory for encryption
// data: pointer to data
// size: size of data in bytes
// options: analysis options
// result: output analysis result (must be freed with encryption_result_free)
// Returns 0 on success, non-zero on error
int encryption_analyze_data(const void* data,
                          size_t size,
                          const encryption_options_t* options,
                          encryption_result_t* result);

// Analyze all files in a directory for encryption
// directory_path: path to the directory
// options: analysis options
// results: output array of analysis results (must be freed)
// count: output number of results
// Returns 0 on success, non-zero on error
int encryption_analyze_directory(const char* directory_path,
                               const encryption_options_t* options,
                               encryption_result_t** results,
                               size_t* count);

// Calculate entropy of data
// data: pointer to data
// size: size of data in bytes
// Returns entropy value (0.0-1.0, where 1.0 is maximum randomness)
double encryption_calculate_entropy(const uint8_t* data, size_t size);

// Detect cipher type from data
// data: pointer to data
// size: size of data in bytes
// cipher_name: output cipher name (must be freed)
// confidence: output confidence level (0.0-1.0)
// Returns 0 on success, non-zero on error
int encryption_detect_cipher(const uint8_t* data, size_t size, char** cipher_name, double* confidence);

// Detect encryption algorithm from data
// data: pointer to data
// size: size of data in bytes
// algorithm_name: output algorithm name (must be freed)
// confidence: output confidence level (0.0-1.0)
// Returns 0 on success, non-zero on error
int encryption_detect_algorithm(const uint8_t* data, size_t size, char** algorithm_name, double* confidence);

// Check if data is compressed
// data: pointer to data
// size: size of data in bytes
// is_compressed: output flag indicating if data is compressed
// Returns 0 on success, non-zero on error
int encryption_is_compressed(const uint8_t* data, size_t size, int* is_compressed);

// Get information about supported ciphers
// ciphers: output array of cipher information (must be freed with cipher_info_free for each element)
// count: output number of ciphers
// Returns 0 on success, non-zero on error
int encryption_get_cipher_info(cipher_info_t** ciphers, size_t* count);

// Get cipher description
// cipher_name: name of the cipher
// Returns description of the cipher, or NULL if not found
const char* encryption_get_cipher_description(const char* cipher_name);

// Get entropy description
// entropy: entropy value (0.0-1.0)
// Returns description of the entropy level
const char* encryption_get_entropy_description(double entropy);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ENCRYPTION_ENCRYPTION_H