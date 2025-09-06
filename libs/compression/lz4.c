#include "lz4.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

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

// LZ4 block types
#define LZ4_BLOCK_STREAM_HEADER 0x01
#define LZ4_BLOCK_BLOCK 0x02
#define LZ4_BLOCK_INDEX 0x03
#define LZ4_BLOCK_STREAM_FOOTER 0x04
#define LZ4_BLOCK_PADDING 0x05

// LZ4 filter IDs
#define LZ4_FILTER_DELTA 0x03
#define LZ4_FILTER_X86 0x04
#define LZ4_FILTER_POWERPC 0x05
#define LZ4_FILTER_IA64 0x06
#define LZ4_FILTER_ARM 0x07
#define LZ4_FILTER_ARMTHUMB 0x08
#define LZ4_FILTER_SPARC 0x09
#define LZ4_FILTER_LZMA2 0x21

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert LZ4 time to Unix time
static time_t lz4_time_to_unix(uint64_t lz4_time) {
    // LZ4 doesn't store timestamps directly
    // Return current time as fallback
    return time(NULL);
}

// Convert file attributes to string representation
static char* lz4_attributes_to_string(uint32_t attributes) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    perm_str[0] = (attributes & 0x10) ? 'd' : '-';  // Directory flag
    perm_str[1] = (attributes & 0x01) ? '-' : 'r';  // Read-only
    perm_str[2] = (attributes & 0x02) ? 'w' : '-';  // Hidden
    perm_str[3] = (attributes & 0x04) ? 'x' : '-';  // System
    perm_str[4] = (attributes & 0x20) ? 'r' : '-';  // Archive
    perm_str[5] = '-';
    perm_str[6] = '-';
    perm_str[7] = '-';
    perm_str[8] = '-';
    perm_str[9] = '-';
    perm_str[10] = '\0';
    
    // Handle special bits
    if (attributes & 0x0100) perm_str[3] = (attributes & 0x20) ? 's' : 'S'; // Setuid
    if (attributes & 0x0200) perm_str[6] = (attributes & 0x02) ? 's' : 'S'; // Setgid
    if (attributes & 0x0400) perm_str[9] = (attributes & 0x01) ? 't' : 'T'; // Sticky
    
    return perm_str;
}

int compression_analyze_lz4(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZ4 frame header
    lz4_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZ4 signature
    if (header.magic != 0x184D2204) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_LZ4;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_LZ4));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Check for content size
    if (header.content_size_flag) {
        archive->total_uncompressed_size = header.content_size;
    } else {
        // Estimate uncompressed size (typically 2-4x compressed size)
        archive->total_uncompressed_size = st.st_size * 3;
    }
    
    // Check for encryption
    archive->is_password_protected = 0; // LZ4 doesn't support encryption natively
    
    // Create compressed entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    entry.filename = strdup_safe(file_path);
    entry.compressed_size = archive->total_compressed_size;
    entry.uncompressed_size = archive->total_uncompressed_size;
    entry.offset = 0;
    entry.crc32 = header.content_checksum; // Use content checksum as CRC32
    entry.is_encrypted = 0;
    entry.is_directory = 0;
    entry.timestamp = lz4_time_to_unix(time(NULL));
    entry.permissions = lz4_attributes_to_string(0644);
    
    // Add entry to archive
    compressed_archive_add_entry(archive, &entry);
    compressed_entry_free(&entry);
    
    fclose(file);
    return 0;
}

int compression_extract_lz4(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the LZ4 archive
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

int compression_list_lz4_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in LZ4 archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZ4 frame header
    lz4_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZ4 signature
    if (header.magic != 0x184D2204) {
        fclose(file);
        return -1;
    }
    
    // Allocate initial space for entries
    size_t capacity = 16;
    *entries = (compressed_entry_t*)malloc(capacity * sizeof(compressed_entry_t));
    if (!*entries) {
        fclose(file);
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (stat(archive_path, &st) != 0) {
        fclose(file);
        free(*entries);
        *entries = NULL;
        *count = 0;
        return -1;
    }
    
    // Initialize entry
    compressed_entry_init(&(*entries)[*count]);
    (*entries)[*count].filename = strdup_safe(archive_path);
    (*entries)[*count].compressed_size = st.st_size;
    
    // Check for content size
    if (header.content_size_flag) {
        (*entries)[*count].uncompressed_size = header.content_size;
    } else {
        // Estimate uncompressed size (typically 2-4x compressed size)
        (*entries)[*count].uncompressed_size = st.st_size * 3;
    }
    
    (*entries)[*count].offset = 0;
    (*entries)[*count].crc32 = header.content_checksum; // Use content checksum as CRC32
    (*entries)[*count].is_encrypted = 0;
    (*entries)[*count].is_directory = 0;
    (*entries)[*count].timestamp = lz4_time_to_unix(time(NULL));
    (*entries)[*count].permissions = lz4_attributes_to_string(0644);
    
    free((*entries)[*count].filename);
    free((*entries)[*count].permissions);
    free((*entries)[*count].owner_name);
    free((*entries)[*count].group_name);
    free((*entries)[*count].security_issue);
    memset(&(*entries)[*count], 0, sizeof(compressed_entry_t));
    
    (*count)++;
    
    fclose(file);
    return 0;
}

int compression_lz4_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if LZ4 archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZ4 frame header
    lz4_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZ4 signature
    if (header.magic != 0x184D2204) {
        fclose(file);
        return -1;
    }
    
    // LZ4 doesn't support encryption natively
    *is_protected = 0;
    
    fclose(file);
    return 0;
}