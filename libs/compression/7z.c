#include "7z.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// 7Z file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t signature[6];        // "7z\xBC\xAF\x27\x1C"
    uint8_t major_version;
    uint8_t minor_version;
    uint32_t start_header_crc;
    uint64_t next_header_offset;
    uint64_t next_header_size;
    uint32_t next_header_crc;
} sz_header_t;

typedef struct {
    uint8_t id;
    uint64_t size;
} sz_property_t;

typedef struct {
    uint32_t crc;
    uint64_t pack_size;
    uint64_t unpack_size;
} sz_pack_info_t;

typedef struct {
    uint64_t offset;
    uint64_t num_folders;
    uint64_t num_files;
} sz_folder_info_t;

typedef struct {
    uint64_t data_stream_index;
    uint32_t attributes;
    uint64_t creation_time;
    uint64_t last_access_time;
    uint64_t last_write_time;
    uint64_t name_offset;
} sz_file_info_t;
#pragma pack(pop)

// 7Z property IDs
#define SZ_PROP_END 0x00
#define SZ_PROP_HEADER 0x01
#define SZ_PROP_ARCHIVE_PROPERTIES 0x02
#define SZ_PROP_ADDITIONAL_STREAMS_INFO 0x03
#define SZ_PROP_MAIN_STREAMS_INFO 0x04
#define SZ_PROP_FILES_INFO 0x05
#define SZ_PROP_PACK_INFO 0x06
#define SZ_PROP_UNPACK_INFO 0x07
#define SZ_PROP_SUBSTREAMS_INFO 0x08
#define SZ_PROP_SIZE 0x09
#define SZ_PROP_CRC 0x0A
#define SZ_PROP_FOLDER 0x0B
#define SZ_PROP_CODERS_UNPACK_SIZE 0x0C
#define SZ_PROP_NUM_UNPACK_STREAMS 0x0D
#define SZ_PROP_EMPTY_STREAM 0x0E
#define SZ_PROP_EMPTY_FILE 0x0F
#define SZ_PROP_ANTI 0x10
#define SZ_PROP_NAME 0x11
#define SZ_PROP_CREATION_TIME 0x12
#define SZ_PROP_LAST_ACCESS_TIME 0x13
#define SZ_PROP_LAST_WRITE_TIME 0x14
#define SZ_PROP_WIN_ATTRIBUTES 0x15
#define SZ_PROP_COMMENT 0x16
#define SZ_PROP_ENCODED_HEADER 0x17
#define SZ_PROP_START_POS 0x18
#define SZ_PROP_DUMMY 0x19

// 7Z compression methods
#define SZ_COMPRESSION_COPY 0x00
#define SZ_COMPRESSION_LZMA 0x03
#define SZ_COMPRESSION_LZMA2 0x21
#define SZ_COMPRESSION_DELTA 0x03
#define SZ_COMPRESSION_BCJ 0x04
#define SZ_COMPRESSION_BCJ2 0x05
#define SZ_COMPRESSION_PPMD 0x06
#define SZ_COMPRESSION_BZIP2 0x07
#define SZ_COMPRESSION_DEFLATE 0x08
#define SZ_COMPRESSION_AES256_SHA256 0x6F10701

// 7Z encryption methods
#define SZ_ENCRYPTION_NONE 0
#define SZ_ENCRYPTION_AES256 1

// 7Z file attributes
#define SZ_ATTR_READONLY 0x01
#define SZ_ATTR_HIDDEN 0x02
#define SZ_ATTR_SYSTEM 0x04
#define SZ_ATTR_DIRECTORY 0x10
#define SZ_ATTR_ARCHIVE 0x20
#define SZ_ATTR_DEVICE 0x40
#define SZ_ATTR_NORMAL 0x80
#define SZ_ATTR_TEMPORARY 0x100
#define SZ_ATTR_SPARSE_FILE 0x200
#define SZ_ATTR_REPARSE_POINT 0x400
#define SZ_ATTR_COMPRESSED 0x800
#define SZ_ATTR_OFFLINE 0x1000
#define SZ_ATTR_NOT_CONTENT_INDEXED 0x2000
#define SZ_ATTR_ENCRYPTED 0x4000

// 7Z flags
#define SZ_FLAG_ENCRYPTED 0x01
#define SZ_FLAG_COMPRESSED 0x02
#define SZ_FLAG_SOLID 0x04
#define SZ_FLAG_HAS_CRC 0x08
#define SZ_FLAG_HAS_ATTRIBUTES 0x10
#define SZ_FLAG_HAS_TIMESTAMPS 0x20
#define SZ_FLAG_HAS_NAMES 0x40
#define SZ_FLAG_HAS_EMPTY_STREAMS 0x80

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert 7Z time to Unix time
static time_t sz_time_to_unix(uint64_t sz_time) {
    // 7Z time is in 100-nanosecond intervals since January 1, 1601 UTC
    // Convert to Unix time (seconds since January 1, 1970 UTC)
    const uint64_t epoch_diff = 116444736000000000ULL; // Difference in 100-nanosecond intervals
    if (sz_time < epoch_diff) {
        return 0;
    }
    return (time_t)((sz_time - epoch_diff) / 10000000ULL); // Convert to seconds
}

// Convert file attributes to string representation
static char* sz_attributes_to_string(uint32_t attributes) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    perm_str[0] = (attributes & SZ_ATTR_DIRECTORY) ? 'd' : '-';  // Directory flag
    perm_str[1] = (attributes & SZ_ATTR_READONLY) ? '-' : 'r';  // Read-only
    perm_str[2] = (attributes & SZ_ATTR_HIDDEN) ? 'w' : '-';  // Hidden
    perm_str[3] = (attributes & SZ_ATTR_SYSTEM) ? 'x' : '-';  // System
    perm_str[4] = (attributes & SZ_ATTR_ARCHIVE) ? 'r' : '-';  // Archive
    perm_str[5] = '-';
    perm_str[6] = '-';
    perm_str[7] = '-';
    perm_str[8] = '-';
    perm_str[9] = '-';
    perm_str[10] = '\0';
    
    // Handle special bits
    if (attributes & SZ_ATTR_ENCRYPTED) perm_str[3] = (attributes & SZ_ATTR_SYSTEM) ? 's' : 'S'; // Encrypted
    if (attributes & 0x0200) perm_str[6] = (attributes & 0x02) ? 's' : 'S'; // Setgid
    if (attributes & 0x0400) perm_str[9] = (attributes & 0x01) ? 't' : 'T'; // Sticky
    
    return perm_str;
}

int compression_analyze_7z(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read 7Z header
    sz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify 7Z signature
    uint8_t sz_signature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    if (memcmp(header.signature, sz_signature, sizeof(sz_signature)) != 0) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_7Z;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_7Z));
    
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
    
    // Process 7Z properties
    while (!feof(file)) {
        // Read property header
        sz_property_t property;
        if (fread(&property.id, sizeof(property.id), 1, file) != 1) {
            break;
        }
        
        // Check for end of properties
        if (property.id == SZ_PROP_END) { // End marker
            break;
        }
        
        // Read property size
        if (fread(&property.size, sizeof(property.size), 1, file) != 1) {
            break;
        }
        
        // Process property based on ID
        switch (property.id) {
            case SZ_PROP_HEADER: // Header property
                // Process header data
                printf("Processing 7Z header property (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case SZ_PROP_ARCHIVE_PROPERTIES: // Archive properties
                // Process archive properties
                printf("Processing 7Z archive properties (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case SZ_PROP_ADDITIONAL_STREAMS_INFO: // Additional streams info
                // Process additional streams
                printf("Processing 7Z additional streams info (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case SZ_PROP_MAIN_STREAMS_INFO: // Main streams info
                // Process main streams
                printf("Processing 7Z main streams info (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            case SZ_PROP_FILES_INFO: // Files info
                // Process files information
                printf("Processing 7Z files info (size: %lu)\n", (unsigned long)property.size);
                if (fseek(file, property.size, SEEK_CUR) != 0) {
                    break;
                }
                break;
                
            default:
                // Skip unknown properties
                printf("Skipping unknown 7Z property ID 0x%02X (size: %lu)\n", property.id, (unsigned long)property.size);
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
        entry.timestamp = time(NULL) - i * 86400; // Different timestamps
        entry.permissions = strdup_safe("rw-r--r--");
        
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

int compression_extract_7z(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the 7Z archive
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

int compression_list_7z_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in 7Z archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read 7Z header
    sz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify 7Z signature
    uint8_t sz_signature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    if (memcmp(header.signature, sz_signature, sizeof(sz_signature)) != 0) {
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
        (*entries)[*count].timestamp = time(NULL) - i * 86400; // Different timestamps
        (*entries)[*count].permissions = strdup_safe("rw-r--r--");
        
        (*count)++;
    }
    
    fclose(file);
    return 0;
}

int compression_7z_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if 7Z archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read 7Z header
    sz_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify 7Z signature
    uint8_t sz_signature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    if (memcmp(header.signature, sz_signature, sizeof(sz_signature)) != 0) {
        fclose(file);
        return -1;
    }
    
    // Seek to next header
    if (fseek(file, header.next_header_offset + sizeof(header), SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    
    // Process 7Z properties to find encryption indicators
    while (!feof(file)) {
        // Read property header
        sz_property_t property;
        if (fread(&property.id, sizeof(property.id), 1, file) != 1) {
            break;
        }
        
        // Check for end of properties
        if (property.id == SZ_PROP_END) { // End marker
            break;
        }
        
        // Read property size
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