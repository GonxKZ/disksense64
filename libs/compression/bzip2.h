#ifndef LIBS_COMPRESSION_BZIP2_H
#define LIBS_COMPRESSION_BZIP2_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// BZIP2 file format constants
#define BZIP2_SIGNATURE_SIZE 3
#define BZIP2_HEADER_SIZE 4
#define BZIP2_BLOCK_HEADER_SIZE 12
#define BZIP2_TRAILER_SIZE 8

// BZIP2 compression methods
#define BZIP2_COMPRESSION_BZIP2 0x627A68  // "bzh"

// BZIP2 block size codes
#define BZIP2_BLOCK_SIZE_1 '1'  // 100K
#define BZIP2_BLOCK_SIZE_2 '2'  // 200K
#define BZIP2_BLOCK_SIZE_3 '3'  // 300K
#define BZIP2_BLOCK_SIZE_4 '4'  // 400K
#define BZIP2_BLOCK_SIZE_5 '5'  // 500K
#define BZIP2_BLOCK_SIZE_6 '6'  // 600K
#define BZIP2_BLOCK_SIZE_7 '7'  // 700K
#define BZIP2_BLOCK_SIZE_8 '8'  // 800K
#define BZIP2_BLOCK_SIZE_9 '9'  // 900K

// BZIP2 file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[3];        // "BZh"
    uint8_t block_size;          // Block size ('1'-'9')
} bzip2_header_t;

typedef struct {
    uint32_t crc32;              // CRC32 of compressed data
    uint32_t compressed_size;    // Size of compressed data
    uint32_t uncompressed_size;  // Size of uncompressed data
} bzip2_block_header_t;

typedef struct {
    uint32_t crc32;              // CRC32 of uncompressed data
    uint8_t eos_marker[6];       // End of stream marker (0x177245385090)
} bzip2_trailer_t;
#pragma pack(pop)

// Analyze BZIP2 file
int compression_analyze_bzip2(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from BZIP2 archive
int compression_extract_bzip2(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in BZIP2 archive
int compression_list_bzip2_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if BZIP2 archive is password protected
int compression_bzip2_is_password_protected(const char* archive_path, int* is_protected);

// Convert BZIP2 block size to bytes
uint32_t bzip2_block_size_to_bytes(uint8_t block_size);

// Convert BZIP2 time to Unix time
time_t bzip2_time_to_unix(uint32_t bzip2_time);

// Convert file attributes to string representation
char* bzip2_attributes_to_string(uint8_t block_size);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_BZIP2_H