#ifndef LIBS_COMPRESSION_COMPRESSION_H
#define LIBS_COMPRESSION_COMPRESSION_H

#include <stdint.h>
#include <stddef.h>
#include <archive.h>
#include <archive_entry.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compressed file entry
typedef struct {
    char* filename;
    uint64_t compressed_size;
    uint64_t uncompressed_size;
    uint64_t offset;
    uint32_t crc32;
    int is_encrypted;
    int is_directory;
    uint64_t timestamp;
    char* permissions;
} compressed_entry_t;

// Compressed archive information
typedef struct {
    char* archive_path;
    char* format_name;
    uint64_t total_compressed_size;
    uint64_t total_uncompressed_size;
    compressed_entry_t* entries;
    size_t entry_count;
    size_t capacity;
    int is_password_protected;
    int is_corrupted;
} compressed_archive_t;

// Compression analysis options
typedef struct {
    int extract_metadata_only;     // Only extract metadata, don't decompress
    int check_integrity;           // Check archive integrity
    int detect_encryption;         // Detect encryption
    int deep_analysis;             // Perform deep analysis
    char* password;                // Password for encrypted archives
    size_t max_entries;            // Maximum number of entries to analyze
} compression_options_t;

// Compression analysis result
typedef struct {
    compressed_archive_t* archives;
    size_t archive_count;
    size_t capacity;
} compression_result_t;

// Initialize compressed archive
void compressed_archive_init(compressed_archive_t* archive);

// Add an entry to the compressed archive
int compressed_archive_add_entry(compressed_archive_t* archive, const compressed_entry_t* entry);

// Free compressed archive
void compressed_archive_free(compressed_archive_t* archive);

// Initialize compression result
void compression_result_init(compression_result_t* result);

// Add an archive to the result
int compression_result_add_archive(compression_result_t* result, const compressed_archive_t* archive);

// Free compression result
void compression_result_free(compression_result_t* result);

// Initialize compression options with default values
void compression_options_init(compression_options_t* options);

// Free compression options
void compression_options_free(compression_options_t* options);

// Initialize compressed entry
void compressed_entry_init(compressed_entry_t* entry);

// Free compressed entry
void compressed_entry_free(compressed_entry_t* entry);

// Analyze compressed file
int compression_analyze_file(const char* file_path, 
                           const compression_options_t* options,
                           compressed_archive_t* archive);

// Extract file from compressed archive
int compression_extract_file(const char* archive_path,
                           const char* entry_name,
                           const char* output_path,
                           const char* password);

// List entries in compressed archive
int compression_list_entries(const char* archive_path,
                           compressed_entry_t** entries,
                           size_t* count,
                           const char* password);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_COMPRESSION_H