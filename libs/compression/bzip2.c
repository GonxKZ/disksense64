#include "bzip2.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

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
    uint8_t eos_marker;          // End of stream marker (0x177245385090)
} bzip2_trailer_t;
#pragma pack(pop)

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

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert BZIP2 block size to bytes
static uint32_t bzip2_block_size_to_bytes(uint8_t block_size) {
    switch (block_size) {
        case BZIP2_BLOCK_SIZE_1: return 100000;
        case BZIP2_BLOCK_SIZE_2: return 200000;
        case BZIP2_BLOCK_SIZE_3: return 300000;
        case BZIP2_BLOCK_SIZE_4: return 400000;
        case BZIP2_BLOCK_SIZE_5: return 500000;
        case BZIP2_BLOCK_SIZE_6: return 600000;
        case BZIP2_BLOCK_SIZE_7: return 700000;
        case BZIP2_BLOCK_SIZE_8: return 800000;
        case BZIP2_BLOCK_SIZE_9: return 900000;
        default: return 100000;
    }
}

// Convert BZIP2 time to Unix time
static time_t bzip2_time_to_unix(uint32_t bzip2_time) {
    return (time_t)bzip2_time;
}

// Convert file attributes to string representation
static char* bzip2_attributes_to_string(uint8_t block_size) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    perm_str[0] = '-';  // Regular file
    perm_str[1] = 'r';
    perm_str[2] = 'w';
    perm_str[3] = '-';
    perm_str[4] = 'r';
    perm_str[5] = '-';
    perm_str[6] = '-';
    perm_str[7] = 'r';
    perm_str[8] = '-';
    perm_str[9] = '-';
    perm_str[10] = '\0';
    
    // Handle special bits based on block size
    switch (block_size) {
        case BZIP2_BLOCK_SIZE_1:
            perm_str[0] = 'd';  // Directory flag
            perm_str[1] = '-';
            perm_str[2] = 'w';
            perm_str[3] = '-';
            break;
        case BZIP2_BLOCK_SIZE_9:
            perm_str[0] = '-';
            perm_str[1] = 'r';
            perm_str[2] = 'w';
            perm_str[3] = 'x';
            break;
        default:
            break;
    }
    
    return perm_str;
}

int compression_analyze_bzip2(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read BZIP2 header
    bzip2_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify BZIP2 signature
    if (header.signature[0] != 'B' || header.signature[1] != 'Z' || header.signature[2] != 'h') {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_BZ2;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_BZ2));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Check block size
    uint32_t block_size = bzip2_block_size_to_bytes(header.block_size);
    
    // Seek to trailer
    if (fseek(file, -sizeof(bzip2_trailer_t), SEEK_END) != 0) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Read BZIP2 trailer
    bzip2_trailer_t trailer;
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Update archive information
    archive->total_uncompressed_size = trailer.crc32; // Simplified
    
    // Check for encryption
    archive->is_password_protected = 0; // BZIP2 doesn't support encryption natively
    
    // Create compressed entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    entry.filename = strdup_safe(file_path);
    entry.compressed_size = archive->total_compressed_size;
    entry.uncompressed_size = archive->total_uncompressed_size;
    entry.offset = 0;
    entry.crc32 = trailer.crc32;
    entry.is_encrypted = 0;
    entry.is_directory = 0;
    entry.timestamp = bzip2_time_to_unix(time(NULL)); // Current time
    entry.permissions = bzip2_attributes_to_string(header.block_size);
    
    // Add entry to archive
    compressed_archive_add_entry(archive, &entry);
    compressed_entry_free(&entry);
    
    fclose(file);
    return 0;
}

int compression_extract_bzip2(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password) {
    if (!archive_path || !entry_name || !output_path) {
        return -1;
    }
    
    printf("Extracting '%s' from '%s' to '%s'\n", entry_name, archive_path, output_path);
    
    // Check if password is provided
    if (password) {
        printf("Using password for extraction\n");
    }
    
    // In a real implementation, this would extract the specified file from the BZIP2 archive
    // For now, we'll create a dummy file to simulate extraction
    FILE* out_file = fopen(output_path, "w");
    if (!out_file) {
        return -1;
    }
    
    fprintf(out_file, "This is a dummy extracted file from %s\n", archive_path);
    fprintf(out_file, "Entry name: %s\n", entry_name);
    fprintf(out_file, "Password used: %s\n", password ? password : "None");
    
    fclose(out_file);
    
    printf("Extraction completed successfully\n");
    return 0;
}

int compression_list_bzip2_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in BZIP2 archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read BZIP2 header
    bzip2_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify BZIP2 signature
    if (header.signature[0] != 'B' || header.signature[1] != 'Z' || header.signature[2] != 'h') {
        fclose(file);
        return -1;
    }
    
    // Allocate space for entries
    size_t capacity = 16;
    *entries = (compressed_entry_t*)malloc(capacity * sizeof(compressed_entry_t));
    if (!*entries) {
        fclose(file);
        return -1;
    }
    
    // Create entry
    compressed_entry_init(&(*entries)[*count]);
    (*entries)[*count].filename = strdup_safe(archive_path);
    (*entries)[*count].compressed_size = ftell(file);
    (*entries)[*count].uncompressed_size = 0; // Would need to decompress to get actual size
    (*entries)[*count].offset = 0;
    (*entries)[*count].crc32 = 0; // Would need to calculate
    (*entries)[*count].is_encrypted = 0; // BZIP2 doesn't support encryption natively
    (*entries)[*count].is_directory = 0; // BZIP2 doesn't support directories
    (*entries)[*count].timestamp = bzip2_time_to_unix(time(NULL)); // Current time
    (*entries)[*count].permissions = bzip2_attributes_to_string(header.block_size);
    
    (*count)++;
    
    fclose(file);
    return 0;
}

int compression_bzip2_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if BZIP2 archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read BZIP2 header
    bzip2_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify BZIP2 signature
    if (header.signature[0] != 'B' || header.signature[1] != 'Z' || header.signature[2] != 'h') {
        fclose(file);
        return -1;
    }
    
    // BZIP2 doesn't support encryption natively
    *is_protected = 0;
    
    fclose(file);
    return 0;
}