#ifndef LIBS_COMPRESSION_RAR_H
#define LIBS_COMPRESSION_RAR_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// RAR file format constants
#define RAR_SIGNATURE_SIZE 7
#define RAR_MARK_HEADER_SIZE 7
#define RAR_MAIN_HEADER_SIZE 13
#define RAR_FILE_HEADER_SIZE 25
#define RAR_END_HEADER_SIZE 13

// RAR block types
#define RAR_BLOCK_MARK 0x72
#define RAR_BLOCK_MAIN 0x73
#define RAR_BLOCK_FILE 0x74
#define RAR_BLOCK_END 0x7B

// RAR compression methods
#define RAR_COMPRESSION_STORE 0x30
#define RAR_COMPRESSION_FASTEST 0x31
#define RAR_COMPRESSION_FAST 0x32
#define RAR_COMPRESSION_NORMAL 0x33
#define RAR_COMPRESSION_GOOD 0x34
#define RAR_COMPRESSION_BEST 0x35

// RAR encryption methods
#define RAR_ENCRYPTION_NONE 0
#define RAR_ENCRYPTION_RAR3 1
#define RAR_ENCRYPTION_RAR5 2

// RAR file attributes
#define RAR_ATTR_READONLY 0x01
#define RAR_ATTR_HIDDEN 0x02
#define RAR_ATTR_SYSTEM 0x04
#define RAR_ATTR_DIRECTORY 0x10
#define RAR_ATTR_ARCHIVE 0x20

// RAR flags
#define RAR_FLAG_PASSWORD 0x04
#define RAR_FLAG_COMMENT 0x08
#define RAR_FLAG_SOLID 0x10
#define RAR_FLAG_PACKED 0x20
#define RAR_FLAG_LOCKED 0x80

// RAR file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[RAR_SIGNATURE_SIZE];  // "Rar!" followed by version
    uint16_t crc;
    uint8_t type;
    uint16_t flags;
    uint16_t size;
    uint32_t add_size;
} rar_block_header_t;

typedef struct {
    uint16_t version;
    uint16_t flags;
    uint32_t pack_size;
    uint32_t unp_size;
    uint8_t host_os;
    uint32_t file_crc;
    uint32_t file_time;
    uint8_t unp_ver;
    uint8_t method;
    uint16_t name_size;
    uint32_t attr;
} rar_file_header_t;

typedef struct {
    uint32_t crc;
    uint16_t type;
    uint16_t flags;
    uint16_t size;
    uint32_t reserved1;
    uint32_t reserved2;
} rar5_block_header_t;

typedef struct {
    uint32_t pack_size;
    uint32_t unp_size;
    uint8_t checksum[4];
    uint8_t flags;
    uint8_t compression_info;
    uint8_t host_os;
    uint32_t file_crc;
    uint32_t file_time;
    uint8_t unp_ver;
    uint8_t method;
    uint16_t name_size;
    uint32_t attr;
} rar5_file_header_t;
#pragma pack(pop)

// Analyze RAR file
int compression_analyze_rar(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from RAR archive
int compression_extract_rar(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in RAR archive
int compression_list_rar_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if RAR archive is password protected
int compression_rar_is_password_protected(const char* archive_path, int* is_protected);

// Find end of central directory record
static int find_end_of_central_directory(FILE* file, rar_end_of_central_directory_t* eocd);

// Convert RAR time to Unix time
static time_t rar_time_to_unix(uint32_t dos_time);

// Convert file attributes to string representation
static char* rar_attributes_to_string(uint32_t attributes, uint8_t host_os);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_RAR_H