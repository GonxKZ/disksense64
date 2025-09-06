#ifndef LIBS_COMPRESSION_LZ4_H
#define LIBS_COMPRESSION_LZ4_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// LZ4 file format constants
#define LZ4_SIGNATURE 0x184D2204
#define LZ4_SIGNATURE_SIZE 4
#define LZ4_FRAME_HEADER_SIZE 16
#define LZ4_BLOCK_HEADER_SIZE 4
#define LZ4_FRAME_FOOTER_SIZE 8

// LZ4 compression methods
#define LZ4_COMPRESSION_LZ4 0x01
#define LZ4_COMPRESSION_LZ4HC 0x02

// LZ4 frame descriptor flags
#define LZ4_FLAG_BLOCK_INDEPENDENCE 0x20
#define LZ4_FLAG_BLOCK_CHECKSUM 0x10
#define LZ4_FLAG_CONTENT_SIZE 0x08
#define LZ4_FLAG_CONTENT_CHECKSUM 0x04
#define LZ4_FLAG_DICTIONARY_ID 0x01

// LZ4 file format structures
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;              // 0x184D2204
    uint8_t version;             // Version number
    uint8_t block_independence;  // Block independence flag
    uint8_t block_checksum;      // Block checksum flag
    uint8_t content_size_flag;   // Content size flag
    uint8_t content_checksum;    // Content checksum flag
    uint8_t reserved;            // Reserved bits
    uint64_t content_size;       // Content size
    uint32_t header_checksum;    // Header checksum
} lz4_frame_header_t;

typedef struct {
    uint32_t block_size;         // Block size
    uint8_t block_data[1];       // Variable-size block data
} lz4_block_t;

typedef struct {
    uint32_t end_mark;           // 0x00000000
    uint32_t content_checksum;   // Content checksum
} lz4_frame_footer_t;
#pragma pack(pop)

// Analyze LZ4 file
int compression_analyze_lz4(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from LZ4 archive
int compression_extract_lz4(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in LZ4 archive
int compression_list_lz4_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if LZ4 archive is password protected
int compression_lz4_is_password_protected(const char* archive_path, int* is_protected);

// Convert LZ4 time to Unix time
time_t lz4_time_to_unix(uint64_t lz4_time);

// Convert file attributes to string representation
char* lz4_attributes_to_string(uint32_t attributes);

// Convert DOS time to Unix time
time_t dos_time_to_unix(uint16_t dos_date, uint16_t dos_time);

// Convert file attributes to string representation
char* attributes_to_string(uint32_t attributes);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_LZ4_H