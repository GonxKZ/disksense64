#ifndef LIBS_COMPRESSION_GZIP_H
#define LIBS_COMPRESSION_GZIP_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// GZIP file format constants
#define GZIP_SIGNATURE_SIZE 10
#define GZIP_HEADER_SIZE 10
#define GZIP_TRAILER_SIZE 8
#define GZIP_MAGIC1 0x1F
#define GZIP_MAGIC2 0x8B

// GZIP compression methods
#define GZIP_COMPRESSION_DEFLATE 8

// GZIP flags
#define GZIP_FLAG_TEXT 0x01
#define GZIP_FLAG_HEADER_CRC 0x02
#define GZIP_FLAG_EXTRA_FIELDS 0x04
#define GZIP_FLAG_ORIGINAL_NAME 0x08
#define GZIP_FLAG_COMMENT 0x10
#define GZIP_FLAG_ENCRYPTED 0x20

// GZIP operating system types
#define GZIP_OS_FAT 0
#define GZIP_OS_AMIGA 1
#define GZIP_OS_VMS 2
#define GZIP_OS_UNIX 3
#define GZIP_OS_VM_CMS 4
#define GZIP_OS_ATARI 5
#define GZIP_OS_HPFS 6
#define GZIP_OS_MACINTOSH 7
#define GZIP_OS_Z_SYSTEM 8
#define GZIP_OS_CP_M 9
#define GZIP_OS_TOPS_20 10
#define GZIP_OS_NTFS 11
#define GZIP_OS_QDOS 12
#define GZIP_OS_ACORN_RISCOS 13
#define GZIP_OS_UNKNOWN 255

// GZIP file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t id1;                 // 0x1F
    uint8_t id2;                 // 0x8B
    uint8_t compression_method;  // 8 = deflate
    uint8_t flags;               // FLG
    uint32_t mtime;              // Modification time
    uint8_t extra_flags;         // XFL
    uint8_t os_type;             // Operating system type
} gzip_header_t;

typedef struct {
    uint32_t crc32;              // CRC32 of uncompressed data
    uint32_t uncompressed_size;  // Size of uncompressed data
} gzip_trailer_t;
#pragma pack(pop)

// Analyze GZIP file
int compression_analyze_gzip(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from GZIP archive
int compression_extract_gzip(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in GZIP archive
int compression_list_gzip_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if GZIP archive is password protected
int compression_gzip_is_password_protected(const char* archive_path, int* is_protected);

// Find end of central directory record
static int find_end_of_central_directory(FILE* file, gzip_end_of_central_directory_t* eocd);

// Convert GZIP time to Unix time
static time_t gzip_time_to_unix(uint32_t gzip_time);

// Convert file attributes to string representation
static char* gzip_attributes_to_string(uint8_t os_type);

// Parse extra fields
static int gzip_parse_extra_fields(FILE* file, const gzip_header_t* header);

// Parse original filename
static int gzip_parse_original_name(FILE* file, const gzip_header_t* header, char** filename);

// Parse comment
static int gzip_parse_comment(FILE* file, const gzip_header_t* header, char** comment);

// Parse header CRC
static int gzip_parse_header_crc(FILE* file, const gzip_header_t* header);

// Parse trailer
static int gzip_parse_trailer(FILE* file, gzip_trailer_t* trailer);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_GZIP_H