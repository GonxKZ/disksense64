#ifndef LIBS_COMPRESSION_ZSTD_H
#define LIBS_COMPRESSION_ZSTD_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// ZSTD file format constants
#define ZSTD_SIGNATURE 0xFD2FB528
#define ZSTD_SIGNATURE_SIZE 4
#define ZSTD_FRAME_HEADER_SIZE 16
#define ZSTD_BLOCK_HEADER_SIZE 3
#define ZSTD_FRAME_FOOTER_SIZE 4

// ZSTD compression methods
#define ZSTD_COMPRESSION_ZSTD 0x01
#define ZSTD_COMPRESSION_ZSTDHC 0x02

// ZSTD frame descriptor flags
#define ZSTD_FLAG_SINGLE_SEGMENT 0x20
#define ZSTD_FLAG_CONTENT_CHECKSUM 0x04
#define ZSTD_FLAG_DICTIONARY_ID 0x01
#define ZSTD_FLAG_WINDOW_DESCRIPTOR 0x80
#define ZSTD_FLAG_CONTENT_SIZE 0x40

// ZSTD block types
#define ZSTD_BLOCK_TYPE_RAW 0x00
#define ZSTD_BLOCK_TYPE_RLE 0x01
#define ZSTD_BLOCK_TYPE_COMPRESSED 0x02
#define ZSTD_BLOCK_TYPE_RESERVED 0x03

// ZSTD file format structures
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;              // 0xFD2FB528
    uint8_t frame_header_descriptor;
    uint8_t window_descriptor;
    uint8_t dictionary_id[4];
    uint8_t frame_content_size[8];
} zstd_frame_header_t;

typedef struct {
    uint32_t block_header;       // Block header (3 bytes + 1 byte padding)
    uint8_t block_data[1];       // Variable-size block data
} zstd_block_t;

typedef struct {
    uint32_t checksum;           // Content checksum
} zstd_frame_footer_t;
#pragma pack(pop)

// Analyze ZSTD file
int compression_analyze_zstd(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from ZSTD archive
int compression_extract_zstd(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in ZSTD archive
int compression_list_zstd_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if ZSTD archive is password protected
int compression_zstd_is_password_protected(const char* archive_path, int* is_protected);

// Convert ZSTD time to Unix time
time_t zstd_time_to_unix(uint64_t zstd_time);

// Convert file attributes to string representation
char* zstd_attributes_to_string(uint32_t attributes);

// Convert DOS time to Unix time
time_t dos_time_to_unix(uint16_t dos_date, uint16_t dos_time);

// Convert file attributes to string representation
char* attributes_to_string(uint32_t attributes);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_ZSTD_H