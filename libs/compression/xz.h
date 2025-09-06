#ifndef LIBS_COMPRESSION_XZ_H
#define LIBS_COMPRESSION_XZ_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// XZ file format constants
#define XZ_SIGNATURE_SIZE 6
#define XZ_HEADER_SIZE 12
#define XZ_FOOTER_SIZE 12
#define XZ_BLOCK_HEADER_SIZE 5
#define XZ_INDEX_RECORD_SIZE 8

// XZ block types
#define XZ_BLOCK_STREAM_HEADER 0x01
#define XZ_BLOCK_BLOCK 0x02
#define XZ_BLOCK_INDEX 0x03
#define XZ_BLOCK_STREAM_FOOTER 0x04
#define XZ_BLOCK_PADDING 0x05

// XZ filter IDs
#define XZ_FILTER_DELTA 0x03
#define XZ_FILTER_X86 0x04
#define XZ_FILTER_POWERPC 0x05
#define XZ_FILTER_IA64 0x06
#define XZ_FILTER_ARM 0x07
#define XZ_FILTER_ARMTHUMB 0x08
#define XZ_FILTER_SPARC 0x09
#define XZ_FILTER_LZMA2 0x21

// XZ file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[6];        // "7zXZ" followed by null and checksum
    uint8_t stream_header[12];   // Stream header
} xz_header_t;

typedef struct {
    uint8_t id;
    uint64_t size;
} xz_block_header_t;

typedef struct {
    uint32_t crc32;              // CRC32 of uncompressed data
    uint64_t uncompressed_size;  // Size of uncompressed data
} xz_index_record_t;

typedef struct {
    uint32_t crc32;              // CRC32 of index
    uint8_t footer_magic[2];     // Footer magic (0x59, 0x5A)
    uint32_t backward_size;      // Backward size
    uint8_t stream_flags[2];     // Stream flags
} xz_footer_t;
#pragma pack(pop)

// Analyze XZ file
int compression_analyze_xz(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from XZ archive
int compression_extract_xz(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in XZ archive
int compression_list_xz_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if XZ archive is password protected
int compression_xz_is_password_protected(const char* archive_path, int* is_protected);

// Convert XZ time to Unix time
time_t xz_time_to_unix(uint64_t xz_time);

// Convert file attributes to string representation
char* xz_attributes_to_string(uint32_t attributes);

// Read variable-length integer from XZ stream
int xz_read_vli(FILE* file, uint64_t* value);

// Write variable-length integer to XZ stream
int xz_write_vli(FILE* file, uint64_t value);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_XZ_H