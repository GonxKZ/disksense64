#ifndef LIBS_COMPRESSION_TAR_H
#define LIBS_COMPRESSION_TAR_H

#include "compression.h"

#ifdef __cplusplus
extern "C" {
#endif

// TAR file format constants
#define TAR_BLOCK_SIZE 512
#define TAR_SIGNATURE_SIZE 512
#define TAR_HEADER_SIZE 512
#define TAR_MAGIC "ustar"
#define TAR_VERSION "00"

// TAR file type flags
#define TAR_TYPE_REGULAR '0'
#define TAR_TYPE_HARDLINK '1'
#define TAR_TYPE_SYMLINK '2'
#define TAR_TYPE_CHARDEV '3'
#define TAR_TYPE_BLOCKDEV '4'
#define TAR_TYPE_DIRECTORY '5'
#define TAR_TYPE_FIFO '6'

// TAR file format structures
#pragma pack(push, 1)
typedef struct {
    char name[100];              // File name
    char mode[8];                // File mode
    char uid[8];                 // User ID
    char gid[8];                 // Group ID
    char size[12];               // File size in bytes
    char mtime[12];              // Modification time
    char checksum[8];            // Checksum for header
    char typeflag;               // Type of file
    char linkname[100];          // Name of linked file
    char magic[6];               // Magic signature
    char version[2];             // TAR version
    char uname[32];              // User name
    char gname[32];              // Group name
    char devmajor[8];            // Device major number
    char devminor[8];            // Device minor number
    char prefix[155];            // Prefix for long file names
    char padding[12];            // Padding to make header 512 bytes
} tar_header_t;
#pragma pack(pop)

// TAR compression methods
#define TAR_COMPRESSION_NONE 0
#define TAR_COMPRESSION_GZIP 1
#define TAR_COMPRESSION_BZIP2 2
#define TAR_COMPRESSION_XZ 3
#define TAR_COMPRESSION_LZMA 4
#define TAR_COMPRESSION_LZ4 5
#define TAR_COMPRESSION_ZSTD 6

// TAR encryption methods
#define TAR_ENCRYPTION_NONE 0
#define TAR_ENCRYPTION_GPG 1

// TAR file attributes
#define TAR_ATTR_READONLY 0x01
#define TAR_ATTR_HIDDEN 0x02
#define TAR_ATTR_SYSTEM 0x04
#define TAR_ATTR_DIRECTORY 0x10
#define TAR_ATTR_ARCHIVE 0x20
#define TAR_ATTR_DEVICE 0x40
#define TAR_ATTR_NORMAL 0x80
#define TAR_ATTR_TEMPORARY 0x100
#define TAR_ATTR_SPARSE_FILE 0x200
#define TAR_ATTR_REPARSE_POINT 0x400
#define TAR_ATTR_COMPRESSED 0x800
#define TAR_ATTR_OFFLINE 0x1000
#define TAR_ATTR_NOT_CONTENT_INDEXED 0x2000
#define TAR_ATTR_ENCRYPTED 0x4000

// TAR flags
#define TAR_FLAG_ENCRYPTED 0x01
#define TAR_FLAG_COMPRESSED 0x02
#define TAR_FLAG_SOLID 0x04
#define TAR_FLAG_HAS_CRC 0x08
#define TAR_FLAG_HAS_ATTRIBUTES 0x10
#define TAR_FLAG_HAS_TIMESTAMPS 0x20
#define TAR_FLAG_HAS_NAMES 0x40
#define TAR_FLAG_HAS_EMPTY_STREAMS 0x80

// Analyze TAR file
int compression_analyze_tar(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive);

// Extract file from TAR archive
int compression_extract_tar(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password);

// List entries in TAR archive
int compression_list_tar_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password);

// Check if TAR archive is password protected
int compression_tar_is_password_protected(const char* archive_path, int* is_protected);

// Find end of central directory record
static int find_end_of_central_directory(FILE* file, tar_end_of_central_directory_t* eocd);

// Convert TAR time to Unix time
static time_t tar_time_to_unix(uint32_t dos_time);

// Convert file attributes to string representation
static char* tar_attributes_to_string(uint32_t attributes);

// Parse octal string to integer
static uint64_t octal_to_int(const char* octal_str, size_t len);

// Parse integer to octal string
static void int_to_octal(uint64_t value, char* octal_str, size_t len);

// Calculate TAR header checksum
static uint32_t tar_calculate_checksum(const tar_header_t* header);

// Verify TAR header checksum
static int tar_verify_checksum(const tar_header_t* header);

#ifdef __cplusplus
}
#endif

#endif // LIBS_COMPRESSION_TAR_H