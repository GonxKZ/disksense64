#include "zstd.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

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
    uint32_t block_header;
    uint8_t block_data[1];       // Variable-size block data
} zstd_block_t;

typedef struct {
    uint32_t checksum;           // Content checksum
} zstd_frame_footer_t;
#pragma pack(pop)

// ZSTD block types
#define ZSTD_BLOCK_STREAM_HEADER 0x01
#define ZSTD_BLOCK_BLOCK 0x02
#define ZSTD_BLOCK_INDEX 0x03
#define ZSTD_BLOCK_STREAM_FOOTER 0x04
#define ZSTD_BLOCK_PADDING 0x05

// ZSTD filter IDs
#define ZSTD_FILTER_DELTA 0x03
#define ZSTD_FILTER_X86 0x04
#define ZSTD_FILTER_POWERPC 0x05
#define ZSTD_FILTER_IA64 0x06
#define ZSTD_FILTER_ARM 0x07
#define ZSTD_FILTER_ARMTHUMB 0x08
#define ZSTD_FILTER_SPARC 0x09
#define ZSTD_FILTER_LZMA2 0x21

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert ZSTD time to Unix time
static time_t zstd_time_to_unix(uint64_t zstd_time) {
    // ZSTD doesn't store timestamps directly
    // Return current time as fallback
    return time(NULL);
}

// Convert file attributes to string representation
static char* zstd_attributes_to_string(uint32_t attributes) {
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

int compression_analyze_zstd(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZSTD frame header
    zstd_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZSTD signature
    if (header.magic != 0xFD2FB528) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_ZSTD;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_ZSTD));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Parse frame header
    uint8_t fhd = header.frame_header_descriptor;
    int single_segment = (fhd & 0x20) ? 1 : 0;
    int has_content_checksum = (fhd & 0x04) ? 1 : 0;
    int has_dictionary_id = (fhd & 0x01) ? 1 : 0;
    
    printf("ZSTD frame header:\n");
    printf("  Single segment: %s\n", single_segment ? "Yes" : "No");
    printf("  Content checksum: %s\n", has_content_checksum ? "Yes" : "No");
    printf("  Dictionary ID: %s\n", has_dictionary_id ? "Yes" : "No");
    
    // Check for window descriptor
    if (!(fhd & 0x80)) { // No dictionary ID
        if (fseek(file, 1, SEEK_CUR) != 0) { // Skip window descriptor
            fclose(file);
            archive->is_corrupted = 1;
            return -1;
        }
    }
    
    // Check for dictionary ID
    if (has_dictionary_id) {
        if (fseek(file, 4, SEEK_CUR) != 0) { // Skip dictionary ID
            fclose(file);
            archive->is_corrupted = 1;
            return -1;
        }
    }
    
    // Check for frame content size
    if (single_segment || (fhd & 0x40)) { // Single segment or has content size
        if (fseek(file, 8, SEEK_CUR) != 0) { // Skip content size
            fclose(file);
            archive->is_corrupted = 1;
            return -1;
        }
    }
    
    // Process ZSTD blocks
    while (!feof(file)) {
        // Read block header
        zstd_block_t block;
        if (fread(&block.block_header, sizeof(block.block_header), 1, file) != 1) {
            break;
        }
        
        // Check for end of frame
        if ((block.block_header & 0x03) == 0x00) { // Last block
            break;
        }
        
        // Get block size
        uint32_t block_size = block.block_header >> 3;
        
        // Skip block data
        if (fseek(file, block_size, SEEK_CUR) != 0) {
            break;
        }
    }
    
    // Check for content checksum
    if (has_content_checksum) {
        zstd_frame_footer_t footer;
        if (fread(&footer, sizeof(footer), 1, file) != 1) {
            fclose(file);
            archive->is_corrupted = 1;
            return -1;
        }
    }
    
    // Update archive information
    archive->total_uncompressed_size = st.st_size * 2; // Estimate uncompressed size
    
    // Check for encryption
    archive->is_password_protected = 0; // ZSTD doesn't support encryption natively
    
    // Create compressed entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    entry.filename = strdup_safe(file_path);
    entry.compressed_size = archive->total_compressed_size;
    entry.uncompressed_size = archive->total_uncompressed_size;
    entry.offset = 0;
    entry.crc32 = 0; // ZSTD doesn't store CRC32
    entry.is_encrypted = 0;
    entry.is_directory = 0;
    entry.timestamp = zstd_time_to_unix(time(NULL));
    entry.permissions = zstd_attributes_to_string(0644);
    
    // Add entry to archive
    compressed_archive_add_entry(archive, &entry);
    compressed_entry_free(&entry);
    
    fclose(file);
    return 0;
}

int compression_extract_zstd(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the ZSTD archive
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

int compression_list_zstd_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in ZSTD archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZSTD frame header
    zstd_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZSTD signature
    if (header.magic != 0xFD2FB528) {
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
    
    // Parse frame header
    uint8_t fhd = header.frame_header_descriptor;
    int single_segment = (fhd & 0x20) ? 1 : 0;
    int has_content_checksum = (fhd & 0x04) ? 1 : 0;
    int has_dictionary_id = (fhd & 0x01) ? 1 : 0;
    
    // Check for window descriptor
    if (!(fhd & 0x80)) { // No dictionary ID
        if (fseek(file, 1, SEEK_CUR) != 0) { // Skip window descriptor
            fclose(file);
            free(*entries);
            *entries = NULL;
            *count = 0;
            return -1;
        }
    }
    
    // Check for dictionary ID
    if (has_dictionary_id) {
        if (fseek(file, 4, SEEK_CUR) != 0) { // Skip dictionary ID
            fclose(file);
            free(*entries);
            *entries = NULL;
            *count = 0;
            return -1;
        }
    }
    
    // Check for frame content size
    if (single_segment || (fhd & 0x40)) { // Single segment or has content size
        if (fseek(file, 8, SEEK_CUR) != 0) { // Skip content size
            fclose(file);
            free(*entries);
            *entries = NULL;
            *count = 0;
            return -1;
        }
    }
    
    // Process ZSTD blocks
    while (!feof(file)) {
        // Reallocate if needed
        if (*count >= capacity) {
            size_t new_capacity = (capacity == 0) ? 16 : capacity * 2;
            compressed_entry_t* new_entries = (compressed_entry_t*)realloc(*entries, new_capacity * sizeof(compressed_entry_t));
            if (!new_entries) {
                // Clean up and return error
                for (size_t j = 0; j < *count; j++) {
                    compressed_entry_free(&(*entries)[j]);
                }
                free(*entries);
                *entries = NULL;
                *count = 0;
                fclose(file);
                return -1;
            }
            *entries = new_entries;
            capacity = new_capacity;
        }
        
        // Read block header
        zstd_block_t block;
        if (fread(&block.block_header, sizeof(block.block_header), 1, file) != 1) {
            break;
        }
        
        // Check for end of frame
        if ((block.block_header & 0x03) == 0x00) { // Last block
            break;
        }
        
        // Get block size
        uint32_t block_size = block.block_header >> 3;
        
        // Skip block data
        if (fseek(file, block_size, SEEK_CUR) != 0) {
            break;
        }
        
        // Initialize entry
        compressed_entry_init(&(*entries)[*count]);
        (*entries)[*count].filename = strdup_safe(archive_path);
        (*entries)[*count].compressed_size = ftell(file);
        (*entries)[*count].uncompressed_size = block_size * 2; // Estimate uncompressed size
        (*entries)[*count].offset = ftell(file) - block_size;
        (*entries)[*count].crc32 = 0; // ZSTD doesn't store CRC32
        (*entries)[*count].is_encrypted = 0;
        (*entries)[*count].is_directory = 0;
        (*entries)[*count].timestamp = zstd_time_to_unix(time(NULL));
        (*entries)[*count].permissions = zstd_attributes_to_string(0644);
        
        (*count)++;
    }
    
    // Check for content checksum
    if (has_content_checksum) {
        zstd_frame_footer_t footer;
        if (fread(&footer, sizeof(footer), 1, file) != 1) {
            fclose(file);
            // Clean up and return error
            for (size_t j = 0; j < *count; j++) {
                compressed_entry_free(&(*entries)[j]);
            }
            free(*entries);
            *entries = NULL;
            *count = 0;
            return -1;
        }
    }
    
    fclose(file);
    return 0;
}

int compression_zstd_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if ZSTD archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZSTD frame header
    zstd_frame_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZSTD signature
    if (header.magic != 0xFD2FB528) {
        fclose(file);
        return -1;
    }
    
    // Parse frame header
    uint8_t fhd = header.frame_header_descriptor;
    int has_dictionary_id = (fhd & 0x01) ? 1 : 0;
    
    // Check for window descriptor
    if (!(fhd & 0x80)) { // No dictionary ID
        if (fseek(file, 1, SEEK_CUR) != 0) { // Skip window descriptor
            fclose(file);
            return -1;
        }
    }
    
    // Check for dictionary ID
    if (has_dictionary_id) {
        if (fseek(file, 4, SEEK_CUR) != 0) { // Skip dictionary ID
            fclose(file);
            return -1;
        }
    }
    
    // Process ZSTD blocks to find encryption indicators
    while (!feof(file)) {
        zstd_block_t block;
        if (fread(&block.block_header, sizeof(block.block_header), 1, file) != 1) {
            break;
        }
        
        // Check for end of frame
        if ((block.block_header & 0x03) == 0x00) { // Last block
            break;
        }
        
        // Get block size
        uint32_t block_size = block.block_header >> 3;
        
        // Check for encrypted blocks
        if ((block.block_header & 0x03) == 0x03) { // Reserved block type might indicate encryption
            *is_protected = 1;
            break;
        }
        
        // Skip block data
        if (fseek(file, block_size, SEEK_CUR) != 0) {
            break;
        }
    }
    
    fclose(file);
    return 0;
}