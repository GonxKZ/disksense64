#ifndef LIBS_COMPRESSION_LZMA_H
#define LIBS_COMPRESSION_LZMA_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// LZMA file format constants
#define LZMA_SIGNATURE_SIZE 5
#define LZMA_HEADER_SIZE 13
#define LZMA_PROPERTIES_SIZE 5
#define LZMA_DICTIONARY_SIZE 4
#define LZMA_UNCOMPRESSED_SIZE 8
#define LZMA_TRAILER_SIZE 20

// LZMA compression methods
#define LZMA_COMPRESSION_LZMA 0x01
#define LZMA_COMPRESSION_LZMA2 0x21

// LZMA filter IDs
#define LZMA_FILTER_DELTA 0x03
#define LZMA_FILTER_X86 0x04
#define LZMA_FILTER_POWERPC 0x05
#define LZMA_FILTER_IA64 0x06
#define LZMA_FILTER_ARM 0x07
#define LZMA_FILTER_ARMTHUMB 0x08
#define LZMA_FILTER_SPARC 0x09

// LZMA file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t properties[LZMA_PROPERTIES_SIZE];  // LZMA properties
    uint64_t dictionary_size;                  // Dictionary size
    uint64_t uncompressed_size;                // Uncompressed size
} lzma_header_t;

typedef struct {
    uint8_t lc;                                // Literal context bits
    uint8_t lp;                                // Literal position bits
    uint8_t pb;                                // Position bits
    uint32_t dictionary_size;                  // Dictionary size
} lzma_properties_t;

typedef struct {
    uint64_t compressed_size;                  // Compressed size
    uint64_t uncompressed_size;                // Uncompressed size
    uint32_t crc32;                            // CRC32 of uncompressed data
} lzma_trailer_t;
#pragma pack(pop)

// Analyze LZMA file
int compression_analyze_lzma(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from LZMA archive
int compression_extract_lzma(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in LZMA archive
int compression_list_lzma_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if LZMA archive is password protected
int compression_lzma_is_password_protected(const char* archive_path, int* is_protected);

// Convert LZMA properties to string representation
char* lzma_properties_to_string(const lzma_properties_t* props);

// Convert file attributes to string representation
char* lzma_attributes_to_string(uint32_t attributes);

// Convert DOS time to Unix time
time_t lzma_time_to_unix(uint16_t dos_date, uint16_t dos_time);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_LZMA_H