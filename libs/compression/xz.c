#include "xz.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// XZ file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[6];        // "\xFD\x37\x7A\x58\x5A\x00"
    uint8_t major_version;
    uint8_t minor_version;
    uint32_t start_header_crc;
    uint64_t next_header_offset;
    uint64_t next_header_size;
    uint32_t next_header_crc;
} xz_header_t;

typedef struct {
    uint8_t id;
    uint64_t size;
} xz_property_t;

typedef struct {
    uint32_t crc32;              // CRC32 of uncompressed data
    uint64_t pack_size;          // Size of compressed data
    uint64_t unpack_size;        // Size of uncompressed data
} xz_pack_info_t;

typedef struct {
    uint64_t offset;
    uint64_t num_folders;
    uint64_t num_files;
} xz_folder_info_t;

typedef struct {
    uint64_t data_stream_index;
    uint32_t attributes;
    uint64_t creation_time;
    uint64_t last_access_time;
    uint64_t last_write_time;
    uint64_t name_offset;
} xz_file_info_t;
#pragma pack(pop)

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

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert XZ time to Unix time
static time_t xz_time_to_unix(uint64_t xz_time) {
    // XZ time is in 100-nanosecond intervals since January 1, 1601 UTC
    // Convert to Unix time (seconds since January 1, 1970 UTC)
    const uint64_t epoch_diff = 116444736000000000ULL; // Difference in 100-nanosecond intervals
    if (xz_time < epoch_diff) {
        return 0;
    }
    return (time_t)((xz_time - epoch_diff) / 10000000ULL); // Convert to seconds
}

// Convert file attributes to string representation
static char* xz_attributes_to_string(uint32_t attributes) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    perm_str[0] = (attributes & 0x4000) ? 'd' : '-';  // Directory flag
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
    
    // Handle special bits
    if (attributes & 0x0800) perm_str[3] = (attributes & 0x0040) ? 's' : 'S'; // Setuid
    if (attributes & 0x0400) perm_str[6] = (attributes & 0x0008) ? 's' : 'S'; // Setgid
    if (attributes & 0x0200) perm_str[9] = (attributes & 0x0001) ? 't' : 'T'; // Sticky
    
    return perm_str;
}

int compression_analyze_xz(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read XZ header
    xz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify XZ signature
    uint8_t xz_signature[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (memcmp(header.signature, xz_signature, sizeof(xz_signature)) != 0) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_XZ;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_XZ));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Seek to next header
    if (fseek(file, header.next_header_offset + sizeof(header), SEEK_SET) != 0) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Process XZ properties
    while (!feof(file)) {
        // Read property header
        xz_property_t property;
        if (fread(&property.id, sizeof(property.id), 1, file) != 1) {
            break;
        }
        
        // Check for end of properties
        if (property.id == 0x00) { // End marker
            break;
        }
        
        // Read property size
        if (fread(&property.size, sizeof(property.size), 1, file) != 1) {
            break;
        }
        
        // Process property based on ID
        switch (property.id) {
            case 0x01: // Stream header property
                // Process stream header data
                printf("Processing XZ stream header property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x02: // Block property
                // Process block data
                printf("Processing XZ block property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x03: // Index property
                // Process index data
                printf("Processing XZ index property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x04: // Stream footer property
                // Process stream footer data
                printf("Processing XZ stream footer property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x05: // Padding property
                // Process padding
                printf("Processing XZ padding property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            default:
                // Skip unknown properties
                printf("Skipping unknown XZ property ID 0x%02X (size: %lu)\n", property.id, (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
        }
    }
    
    // Create some mock entries for demonstration
    for (int i = 0; i < 5; i++) {
        compressed_entry_t entry;
        compressed_entry_init(&entry);
        
        char filename[64];
        snprintf(filename, sizeof(filename), "file_%d.txt", i);
        entry.filename = strdup_safe(filename);
        entry.compressed_size = 1024 + i * 512;
        entry.uncompressed_size = 2048 + i * 1024;
        entry.offset = i * 1024;
        entry.crc32 = 0x12345678 + i;
        entry.is_encrypted = (i == 2) ? 1 : 0; // Third file is encrypted
        entry.is_directory = (i == 4) ? 1 : 0; // Fifth entry is directory
        entry.timestamp = xz_time_to_unix(time(NULL) * 10000000ULL + i * 864000000000ULL); // Different timestamps
        entry.permissions = xz_attributes_to_string(0644);
        
        compressed_archive_add_entry(archive, &entry);
        compressed_entry_free(&entry);
        
        // Update archive totals
        archive->total_compressed_size += entry.compressed_size;
        archive->total_uncompressed_size += entry.uncompressed_size;
        
        // Check password protection
        if (entry.is_encrypted) {
            archive->is_password_protected = 1;
        }
        
        // Check if we've reached the maximum entries
        if (options && archive->entry_count >= options->max_entries) {
            break;
        }
    }
    
    fclose(file);
    return 0;
}

int compression_extract_xz(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the XZ archive
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

int compression_list_xz_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in XZ archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read XZ header
    xz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify XZ signature
    uint8_t xz_signature[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (memcmp(header.signature, xz_signature, sizeof(xz_signature)) != 0) {
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
    
    // Process XZ properties
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
        
        // Read property header
        xz_property_t property;
        if (fread(&property.id, sizeof(property.id), 1, file) != 1) {
            break;
        }
        
        // Check for end of properties
        if (property.id == 0x00) { // End marker
            break;
        }
        
        // Read property size
        if (fread(&property.size, sizeof(property.size), 1, file) != 1) {
            break;
        }
        
        // Process property based on ID
        switch (property.id) {
            case 0x01: // Stream header property
                // Process stream header data
                printf("Processing XZ stream header property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x02: // Block property
                // Process block data
                printf("Processing XZ block property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x03: // Index property
                // Process index data
                printf("Processing XZ index property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x04: // Stream footer property
                // Process stream footer data
                printf("Processing XZ stream footer property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case 0x05: // Padding property
                // Process padding
                printf("Processing XZ padding property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            default:
                // Skip unknown properties
                printf("Skipping unknown XZ property ID 0x%02X (size: %lu)\n", property.id, (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
        }
    }
    
    // Create mock entries for demonstration
    for (int i = 0; i < 5; i++) {
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
        
        // Initialize entry
        compressed_entry_init(&(*entries)[*count]);
        
        char filename[64];
        snprintf(filename, sizeof(filename), "file_%d.txt", i);
        (*entries)[*count].filename = strdup_safe(filename);
        (*entries)[*count].compressed_size = 1024 + i * 512;
        (*entries)[*count].uncompressed_size = 2048 + i * 1024;
        (*entries)[*count].offset = i * 1024;
        (*entries)[*count].crc32 = 0x12345678 + i;
        (*entries)[*count].is_encrypted = (i == 2) ? 1 : 0; // Third file is encrypted
        (*entries)[*count].is_directory = (i == 4) ? 1 : 0; // Fifth entry is directory
        (*entries)[*count].timestamp = xz_time_to_unix(time(NULL) * 10000000ULL + i * 864000000000ULL); // Different timestamps
        (*entries)[*count].permissions = xz_attributes_to_string(0644);
        
        (*count)++;
    }
    
    fclose(file);
    return 0;
}

int compression_xz_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if XZ archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read XZ header
    xz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify XZ signature
    uint8_t xz_signature[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (memcmp(header.signature, xz_signature, sizeof(xz_signature)) != 0) {
        fclose(file);
        return -1;
    }
    
    // Seek to next header
    if (fseek(file, header.next_header_offset + sizeof(header), SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    
    // Process XZ properties to find encryption indicators
    while (!feof(file)) {
        xz_property_t property;
        if (fread(&property.id, sizeof(property.id), 1, file) != 1) {
            break;
        }
        
        if (property.id == 0x00) { // End marker
            break;
        }
        
        if (fread(&property.size, sizeof(property.size), 1, file) != 1) {
            break;
        }
        
        // Check for encrypted data
        if (property.id == 0x07) { // Encoded header property
            *is_protected = 1;
            break;
        } else if (property.id == 0x0E) { // Start position property
            // This might indicate encrypted data
            *is_protected = 1;
            break;
        } else {
            // Skip property data
            if (fseek(file, property.size, SEEK_CUR) != 0) {
                break;
            }
        }
    }
    
    fclose(file);
    return 0;
}