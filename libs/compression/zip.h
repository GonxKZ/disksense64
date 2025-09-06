#ifndef LIBS_COMPRESSION_ZIP_H
#define LIBS_COMPRESSION_ZIP_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// ZIP file format constants
#define ZIP_LOCAL_FILE_HEADER_SIGNATURE 0x04034B50
#define ZIP_CENTRAL_DIRECTORY_ENTRY_SIGNATURE 0x02014B50
#define ZIP_END_OF_CENTRAL_DIRECTORY_SIGNATURE 0x06054B50

// ZIP compression methods
#define ZIP_COMPRESSION_STORE 0
#define ZIP_COMPRESSION_DEFLATE 8
#define ZIP_COMPRESSION_BZIP2 12
#define ZIP_COMPRESSION_LZMA 14
#define ZIP_COMPRESSION_AES 99

// ZIP encryption methods
#define ZIP_ENCRYPTION_NONE 0
#define ZIP_ENCRYPTION_TRADITIONAL 1
#define ZIP_ENCRYPTION_AES128 2
#define ZIP_ENCRYPTION_AES192 3
#define ZIP_ENCRYPTION_AES256 4

// ZIP file attributes
#define ZIP_ATTR_READONLY 0x01
#define ZIP_ATTR_HIDDEN 0x02
#define ZIP_ATTR_SYSTEM 0x04
#define ZIP_ATTR_DIRECTORY 0x10
#define ZIP_ATTR_ARCHIVE 0x20

// ZIP flags
#define ZIP_FLAG_ENCRYPTED 0x01
#define ZIP_FLAG_COMPRESSION_OPTION_1 0x02
#define ZIP_FLAG_COMPRESSION_OPTION_2 0x04
#define ZIP_FLAG_DATA_DESCRIPTOR 0x08
#define ZIP_FLAG_ENHANCED_DEFLATE 0x10
#define ZIP_FLAG_PATCHED_DATA 0x20
#define ZIP_FLAG_STRONG_ENCRYPTION 0x40
#define ZIP_FLAG_LANGUAGE_ENCODING 0x800
#define ZIP_FLAG_MASK_HEADER_VALUES 0x2000

// ZIP file format structures
#pragma pack(push, 1)
typedef struct {
    uint32_t signature;          // 0x04034B50
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
} zip_local_file_header_t;

typedef struct {
    uint32_t signature;          // 0x02014B50
    uint16_t version_made_by;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
    uint16_t file_comment_length;
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t relative_offset_of_local_header;
} zip_central_directory_entry_t;

typedef struct {
    uint32_t signature;          // 0x06054B50
    uint16_t disk_number;
    uint16_t central_dir_disk_number;
    uint16_t num_entries_this_disk;
    uint16_t num_entries_total;
    uint32_t central_dir_size;
    uint32_t central_dir_offset;
    uint16_t comment_length;
} zip_end_of_central_directory_t;
#pragma pack(pop)

// Analyze ZIP file
int compression_analyze_zip(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from ZIP archive
int compression_extract_zip(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in ZIP archive
int compression_list_zip_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if ZIP archive is password protected
int compression_zip_is_password_protected(const char* archive_path, int* is_protected);

// Find end of central directory record
static int find_end_of_central_directory(FILE* file, zip_end_of_central_directory_t* eocd);

// Convert DOS time to Unix time
static time_t dos_time_to_unix(uint16_t dos_date, uint16_t dos_time);

// Convert file attributes to string representation
static char* attributes_to_string(uint32_t attributes);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_ZIP_H