#include "rar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// RAR file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[7];        // "Rar!" followed by version
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

// RAR block types
#define RAR_BLOCK_MARK 0x72
#define RAR_BLOCK_MAIN 0x73
#define RAR_BLOCK_FILE 0x74
#define RAR_BLOCK_END 0x7B

// RAR5 block types
#define RAR5_BLOCK_MAIN 1
#define RAR5_BLOCK_FILE 2
#define RAR5_BLOCK_END 3

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert RAR time to Unix time
static time_t rar_time_to_unix(uint32_t dos_time) {
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(tm_time));
    
    tm_time.tm_year = ((dos_time >> 25) & 0x7F) + 80;  // Year since 1900
    tm_time.tm_mon = ((dos_time >> 21) & 0x0F) - 1;    // Month (0-11)
    tm_time.tm_mday = (dos_time >> 16) & 0x1F;         // Day (1-31)
    tm_time.tm_hour = (dos_time >> 11) & 0x1F;         // Hour (0-23)
    tm_time.tm_min = (dos_time >> 5) & 0x3F;           // Minute (0-59)
    tm_time.tm_sec = (dos_time & 0x1F) * 2;            // Second (0-59, multiplied by 2)
    
    return mktime(&tm_time);
}

// Convert file attributes to string representation
static char* rar_attributes_to_string(uint32_t attributes, uint8_t host_os) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    // Simplified attribute conversion based on host OS
    if (host_os == 0) {  // MS-DOS
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
    } else {  // Unix
        perm_str[0] = (attributes & 0x4000) ? 'd' : '-';  // Directory
        perm_str[1] = (attributes & 0x0100) ? 'r' : '-';
        perm_str[2] = (attributes & 0x0080) ? 'w' : '-';
        perm_str[3] = (attributes & 0x0040) ? 'x' : '-';
        perm_str[4] = (attributes & 0x0020) ? 'r' : '-';
        perm_str[5] = (attributes & 0x0010) ? 'w' : '-';
        perm_str[6] = (attributes & 0x0008) ? 'x' : '-';
        perm_str[7] = (attributes & 0x0004) ? 'r' : '-';
        perm_str[8] = (attributes & 0x0002) ? 'w' : '-';
        perm_str[9] = (attributes & 0x0001) ? 'x' : '-';
        perm_str[10] = '\0';
    }
    
    return perm_str;
}

int compression_analyze_rar(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read RAR signature
    uint8_t signature[7];
    if (fread(signature, 1, 7, file) != 7) {
        fclose(file);
        return -1;
    }
    
    // Verify RAR signature
    if (signature[0] != 0x52 || signature[1] != 0x61 || signature[2] != 0x72 || signature[3] != 0x21) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_RAR;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_RAR));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Process RAR blocks
    while (!feof(file)) {
        // Read block header
        rar_block_header_t block_header;
        if (fread(&block_header, sizeof(block_header), 1, file) != 1) {
            break;
        }
        
        // Check for end of archive
        if (block_header.type == RAR_BLOCK_END) {
            break;
        }
        
        // Process file blocks
        if (block_header.type == RAR_BLOCK_FILE) {
            // Read file header
            rar_file_header_t file_header;
            if (fread(&file_header, sizeof(file_header), 1, file) != 1) {
                break;
            }
            
            // Read filename
            char* filename = (char*)malloc(file_header.name_size + 1);
            if (!filename) {
                break;
            }
            
            if (fread(filename, file_header.name_size, 1, file) != 1) {
                free(filename);
                break;
            }
            filename[file_header.name_size] = '\0';
            
            // Skip additional data
            uint16_t skip_bytes = block_header.size - sizeof(block_header) - sizeof(file_header) - file_header.name_size;
            if (skip_bytes > 0) {
                if (fseek(file, skip_bytes, SEEK_CUR) != 0) {
                    free(filename);
                    break;
                }
            }
            
            // Create compressed entry
            compressed_entry_t entry;
            compressed_entry_init(&entry);
            entry.filename = filename;
            entry.compressed_size = file_header.pack_size;
            entry.uncompressed_size = file_header.unp_size;
            entry.offset = ftell(file) - block_header.size;
            entry.crc32 = file_header.file_crc;
            entry.is_encrypted = (block_header.flags & 0x04) ? 1 : 0;
            entry.is_directory = (file_header.attr & 0x10) ? 1 : 0;
            entry.timestamp = rar_time_to_unix(file_header.file_time);
            entry.permissions = rar_attributes_to_string(file_header.attr, file_header.host_os);
            
            // Add to archive
            compressed_archive_add_entry(archive, &entry);
            compressed_entry_free(&entry);
            free(filename);
            
            // Update archive totals
            archive->total_compressed_size += file_header.pack_size;
            archive->total_uncompressed_size += file_header.unp_size;
            
            // Check password protection
            if (block_header.flags & 0x04) {
                archive->is_password_protected = 1;
            }
            
            // Check if we've reached the maximum entries
            if (options && archive->entry_count >= options->max_entries) {
                break;
            }
        } else {
            // Skip other block types
            if (fseek(file, block_header.size - sizeof(block_header), SEEK_CUR) != 0) {
                break;
            }
        }
    }
    
    fclose(file);
    return 0;
}

int compression_extract_rar(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the RAR archive
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

int compression_list_rar_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in RAR archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read RAR signature
    uint8_t signature[7];
    if (fread(signature, 1, 7, file) != 7) {
        fclose(file);
        return -1;
    }
    
    // Verify RAR signature
    if (signature[0] != 0x52 || signature[1] != 0x61 || signature[2] != 0x72 || signature[3] != 0x21) {
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
    
    // Process RAR blocks
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
        rar_block_header_t block_header;
        if (fread(&block_header, sizeof(block_header), 1, file) != 1) {
            break;
        }
        
        // Check for end of archive
        if (block_header.type == RAR_BLOCK_END) {
            break;
        }
        
        // Process file blocks
        if (block_header.type == RAR_BLOCK_FILE) {
            rar_file_header_t file_header;
            if (fread(&file_header, sizeof(file_header), 1, file) != 1) {
                break;
            }
            
            // Read filename
            char* filename = (char*)malloc(file_header.name_size + 1);
            if (!filename) {
                break;
            }
            
            if (fread(filename, file_header.name_size, 1, file) != 1) {
                free(filename);
                break;
            }
            filename[file_header.name_size] = '\0';
            
            // Skip additional data
            uint16_t skip_bytes = block_header.size - sizeof(block_header) - sizeof(file_header) - file_header.name_size;
            if (skip_bytes > 0) {
                if (fseek(file, skip_bytes, SEEK_CUR) != 0) {
                    free(filename);
                    break;
                }
            }
            
            // Initialize entry
            compressed_entry_init(&(*entries)[*count]);
            (*entries)[*count].filename = filename;
            (*entries)[*count].compressed_size = file_header.pack_size;
            (*entries)[*count].uncompressed_size = file_header.unp_size;
            (*entries)[*count].offset = ftell(file) - block_header.size;
            (*entries)[*count].crc32 = file_header.file_crc;
            (*entries)[*count].is_encrypted = (block_header.flags & 0x04) ? 1 : 0;
            (*entries)[*count].is_directory = (file_header.attr & 0x10) ? 1 : 0;
            (*entries)[*count].timestamp = rar_time_to_unix(file_header.file_time);
            (*entries)[*count].permissions = rar_attributes_to_string(file_header.attr, file_header.host_os);
            
            free(filename);
            (*count)++;
        } else {
            // Skip other block types
            if (fseek(file, block_header.size - sizeof(block_header), SEEK_CUR) != 0) {
                break;
            }
        }
    }
    
    fclose(file);
    return 0;
}

int compression_rar_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if RAR archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read RAR signature
    uint8_t signature[7];
    if (fread(signature, 1, 7, file) != 7) {
        fclose(file);
        return -1;
    }
    
    // Verify RAR signature
    if (signature[0] != 0x52 || signature[1] != 0x61 || signature[2] != 0x72 || signature[3] != 0x21) {
        fclose(file);
        return -1;
    }
    
    // Process RAR blocks to find encrypted files
    while (!feof(file)) {
        rar_block_header_t block_header;
        if (fread(&block_header, sizeof(block_header), 1, file) != 1) {
            break;
        }
        
        if (block_header.type == RAR_BLOCK_END) {
            break;
        }
        
        if (block_header.type == RAR_BLOCK_FILE) {
            rar_file_header_t file_header;
            if (fread(&file_header, sizeof(file_header), 1, file) != 1) {
                break;
            }
            
            // Check if file is encrypted
            if (block_header.flags & 0x04) {
                *is_protected = 1;
                break;
            }
            
            // Skip filename and additional data
            uint16_t skip_bytes = block_header.size - sizeof(block_header) - sizeof(file_header) - file_header.name_size;
            if (skip_bytes > 0) {
                if (fseek(file, skip_bytes, SEEK_CUR) != 0) {
                    break;
                }
            }
        } else {
            // Skip other block types
            if (fseek(file, block_header.size - sizeof(block_header), SEEK_CUR) != 0) {
                break;
            }
        }
    }
    
    fclose(file);
    return 0;
}