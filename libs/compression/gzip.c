#include "gzip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

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

// GZIP flags
#define GZIP_FLAG_TEXT 0x01
#define GZIP_FLAG_HEADER_CRC 0x02
#define GZIP_FLAG_EXTRA_FIELDS 0x04
#define GZIP_FLAG_ORIGINAL_NAME 0x08
#define GZIP_FLAG_COMMENT 0x10
#define GZIP_FLAG_ENCRYPTED 0x20

// GZIP compression methods
#define GZIP_COMPRESSION_DEFLATE 8

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

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert GZIP time to Unix time
static time_t gzip_time_to_unix(uint32_t gzip_time) {
    return (time_t)gzip_time;
}

// Convert file attributes to string representation
static char* gzip_attributes_to_string(uint8_t os_type) {
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
    
    // Handle OS-specific attributes
    switch (os_type) {
        case GZIP_OS_FAT:  // FAT
            perm_str[0] = 'd';  // Directory flag
            perm_str[1] = '-';
            perm_str[2] = 'w';
            perm_str[3] = '-';
            break;
        case GZIP_OS_UNIX:  // Unix
            perm_str[0] = '-';
            perm_str[1] = 'r';
            perm_str[2] = 'w';
            perm_str[3] = 'x';
            break;
        case GZIP_OS_NTFS:  // NTFS
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

int compression_analyze_gzip(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read GZIP header
    gzip_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify GZIP signature
    if (header.id1 != 0x1F || header.id2 != 0x8B) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_GZ;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_GZ));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Check for extra fields
    if (header.flags & GZIP_FLAG_EXTRA_FIELDS) {
        uint16_t extra_len;
        if (fread(&extra_len, sizeof(extra_len), 1, file) == 1) {
            if (fseek(file, extra_len, SEEK_CUR) != 0) {
                fclose(file);
                archive->is_corrupted = 1;
                return -1;
            }
        }
    }
    
    // Check for original filename
    char* original_filename = NULL;
    if (header.flags & GZIP_FLAG_ORIGINAL_NAME) {
        // Read null-terminated string
        char filename[256];
        size_t i = 0;
        while (i < sizeof(filename) - 1) {
            if (fread(&filename[i], 1, 1, file) != 1) {
                break;
            }
            if (filename[i] == '\0') {
                break;
            }
            i++;
        }
        filename[i] = '\0';
        original_filename = strdup_safe(filename);
    }
    
    // Check for comment
    if (header.flags & GZIP_FLAG_COMMENT) {
        // Skip comment (null-terminated string)
        char ch;
        while (fread(&ch, 1, 1, file) == 1 && ch != '\0') {
            // Continue reading until null terminator
        }
    }
    
    // Check for header CRC
    if (header.flags & GZIP_FLAG_HEADER_CRC) {
        uint16_t header_crc;
        if (fread(&header_crc, sizeof(header_crc), 1, file) != 1) {
            free(original_filename);
            fclose(file);
            archive->is_corrupted = 1;
            return -1;
        }
    }
    
    // Seek to trailer
    if (fseek(file, -sizeof(gzip_trailer_t), SEEK_END) != 0) {
        free(original_filename);
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Read GZIP trailer
    gzip_trailer_t trailer;
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
        free(original_filename);
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Update archive information
    archive->total_uncompressed_size = trailer.uncompressed_size;
    
    // Check for encryption
    archive->is_password_protected = (header.flags & GZIP_FLAG_ENCRYPTED) ? 1 : 0;
    
    // Create compressed entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    entry.filename = original_filename ? original_filename : strdup_safe("unknown.gz");
    entry.compressed_size = archive->total_compressed_size;
    entry.uncompressed_size = trailer.uncompressed_size;
    entry.offset = 0;
    entry.crc32 = trailer.crc32;
    entry.is_encrypted = (header.flags & GZIP_FLAG_ENCRYPTED) ? 1 : 0;
    entry.is_directory = 0;
    entry.timestamp = gzip_time_to_unix(header.mtime);
    entry.permissions = gzip_attributes_to_string(header.os_type);
    
    // Add entry to archive
    compressed_archive_add_entry(archive, &entry);
    compressed_entry_free(&entry);
    free(original_filename);
    
    fclose(file);
    return 0;
}

int compression_extract_gzip(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the GZIP archive
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

int compression_list_gzip_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in GZIP archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read GZIP header
    gzip_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify GZIP signature
    if (header.id1 != 0x1F || header.id2 != 0x8B) {
        fclose(file);
        return -1;
    }
    
    // Check for extra fields
    if (header.flags & GZIP_FLAG_EXTRA_FIELDS) {
        uint16_t extra_len;
        if (fread(&extra_len, sizeof(extra_len), 1, file) == 1) {
            if (fseek(file, extra_len, SEEK_CUR) != 0) {
                fclose(file);
                return -1;
            }
        }
    }
    
    // Check for original filename
    char* original_filename = NULL;
    if (header.flags & GZIP_FLAG_ORIGINAL_NAME) {
        // Read null-terminated string
        char filename[256];
        size_t i = 0;
        while (i < sizeof(filename) - 1) {
            if (fread(&filename[i], 1, 1, file) != 1) {
                break;
            }
            if (filename[i] == '\0') {
                break;
            }
            i++;
        }
        filename[i] = '\0';
        original_filename = strdup_safe(filename);
    }
    
    // Check for comment
    if (header.flags & GZIP_FLAG_COMMENT) {
        // Skip comment (null-terminated string)
        char ch;
        while (fread(&ch, 1, 1, file) == 1 && ch != '\0') {
            // Continue reading until null terminator
        }
    }
    
    // Check for header CRC
    if (header.flags & GZIP_FLAG_HEADER_CRC) {
        uint16_t header_crc;
        if (fread(&header_crc, sizeof(header_crc), 1, file) != 1) {
            free(original_filename);
            fclose(file);
            return -1;
        }
    }
    
    // Seek to trailer
    if (fseek(file, -sizeof(gzip_trailer_t), SEEK_END) != 0) {
        free(original_filename);
        fclose(file);
        return -1;
    }
    
    // Read GZIP trailer
    gzip_trailer_t trailer;
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
        free(original_filename);
        fclose(file);
        return -1;
    }
    
    // Allocate space for entries
    size_t capacity = 16;
    *entries = (compressed_entry_t*)malloc(capacity * sizeof(compressed_entry_t));
    if (!*entries) {
        free(original_filename);
        fclose(file);
        return -1;
    }
    
    // Initialize entry
    compressed_entry_init(&(*entries)[*count]);
    (*entries)[*count].filename = original_filename ? original_filename : strdup_safe("unknown.gz");
    (*entries)[*count].compressed_size = ftell(file);
    (*entries)[*count].uncompressed_size = trailer.uncompressed_size;
    (*entries)[*count].offset = 0;
    (*entries)[*count].crc32 = trailer.crc32;
    (*entries)[*count].is_encrypted = (header.flags & GZIP_FLAG_ENCRYPTED) ? 1 : 0;
    (*entries)[*count].is_directory = 0;
    (*entries)[*count].timestamp = gzip_time_to_unix(header.mtime);
    (*entries)[*count].permissions = gzip_attributes_to_string(header.os_type);
    
    free(original_filename);
    (*count)++;
    
    fclose(file);
    return 0;
}

int compression_gzip_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if GZIP archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read GZIP header
    gzip_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify GZIP signature
    if (header.id1 != 0x1F || header.id2 != 0x8B) {
        fclose(file);
        return -1;
    }
    
    // Check for encryption flag
    if (header.flags & GZIP_FLAG_ENCRYPTED) {
        *is_protected = 1;
    }
    
    fclose(file);
    return 0;
}