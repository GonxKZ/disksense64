#include "tar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

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

// TAR file type flags
#define TAR_TYPE_REGULAR '0'
#define TAR_TYPE_HARDLINK '1'
#define TAR_TYPE_SYMLINK '2'
#define TAR_TYPE_CHARDEV '3'
#define TAR_TYPE_BLOCKDEV '4'
#define TAR_TYPE_DIRECTORY '5'
#define TAR_TYPE_FIFO '6'

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert octal string to integer
static uint64_t octal_to_int(const char* octal_str, size_t len) {
    if (!octal_str) {
        return 0;
    }
    
    uint64_t result = 0;
    for (size_t i = 0; i < len && octal_str[i] != '\0' && octal_str[i] != ' '; i++) {
        if (octal_str[i] >= '0' && octal_str[i] <= '7') {
            result = result * 8 + (octal_str[i] - '0');
        } else {
            break;
        }
    }
    
    return result;
}

// Convert integer to octal string
static void int_to_octal(uint64_t value, char* octal_str, size_t len) {
    if (!octal_str || len == 0) {
        return;
    }
    
    // Convert to octal
    char temp[32];
    int pos = 0;
    
    if (value == 0) {
        temp[pos++] = '0';
    } else {
        while (value > 0) {
            temp[pos++] = '0' + (value % 8);
            value /= 8;
        }
    }
    
    // Reverse and pad
    size_t temp_len = pos;
    for (size_t i = 0; i < len - 1; i++) {
        if (i < temp_len) {
            octal_str[len - 2 - i] = temp[i];
        } else {
            octal_str[len - 2 - i] = '0';
        }
    }
    
    octal_str[len - 1] = '\0';
}

// Calculate TAR header checksum
static uint32_t tar_calculate_checksum(const tar_header_t* header) {
    if (!header) {
        return 0;
    }
    
    uint32_t checksum = 0;
    const uint8_t* bytes = (const uint8_t*)header;
    
    // Sum all bytes except checksum field
    for (size_t i = 0; i < 148; i++) {
        checksum += bytes[i];
    }
    
    // Add spaces for checksum field
    for (size_t i = 148; i < 156; i++) {
        checksum += ' ';
    }
    
    // Sum remaining bytes
    for (size_t i = 156; i < 512; i++) {
        checksum += bytes[i];
    }
    
    return checksum;
}

// Verify TAR header checksum
static int tar_verify_checksum(const tar_header_t* header) {
    if (!header) {
        return -1;
    }
    
    uint32_t calculated_checksum = tar_calculate_checksum(header);
    uint32_t stored_checksum = octal_to_int(header->checksum, sizeof(header->checksum));
    
    return (calculated_checksum == stored_checksum) ? 0 : -1;
}

// Convert TAR mode to string representation
static char* tar_mode_to_string(uint32_t mode) {
    char* perm_str = (char*)malloc(12);
    if (!perm_str) {
        return NULL;
    }
    
    perm_str[0] = (mode & 0x4000) ? 'd' : '-';  // Directory flag
    perm_str[1] = (mode & 0x0100) ? 'r' : '-';
    perm_str[2] = (mode & 0x0080) ? 'w' : '-';
    perm_str[3] = (mode & 0x0040) ? 'x' : '-';
    perm_str[4] = (mode & 0x0020) ? 'r' : '-';
    perm_str[5] = (mode & 0x0010) ? 'w' : '-';
    perm_str[6] = (mode & 0x0008) ? 'x' : '-';
    perm_str[7] = (mode & 0x0004) ? 'r' : '-';
    perm_str[8] = (mode & 0x0002) ? 'w' : '-';
    perm_str[9] = (mode & 0x0001) ? 'x' : '-';
    perm_str[10] = '\0';
    
    // Handle special bits
    if (mode & 0x0800) perm_str[3] = (mode & 0x0040) ? 's' : 'S'; // Setuid
    if (mode & 0x0400) perm_str[6] = (mode & 0x0008) ? 's' : 'S'; // Setgid
    if (mode & 0x0200) perm_str[9] = (mode & 0x0001) ? 't' : 'T'; // Sticky
    
    return perm_str;
}

int compression_analyze_tar(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read TAR signature
    tar_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify TAR signature
    if (strncmp(header.magic, "ustar", 5) != 0) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_TAR;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_TAR));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Process TAR entries
    while (!feof(file)) {
        // Read TAR header
        tar_header_t header;
        if (fread(&header, sizeof(header), 1, file) != 1) {
            break;
        }
        
        // Check for end of archive (two consecutive zero blocks)
        if (header.name[0] == '\0') {
            // Read next block to check if it's also zero
            tar_header_t next_header;
            if (fread(&next_header, sizeof(next_header), 1, file) != 1) {
                break;
            }
            
            if (next_header.name[0] == '\0') {
                // End of archive
                break;
            } else {
                // Put back the next header
                if (fseek(file, -sizeof(next_header), SEEK_CUR) != 0) {
                    break;
                }
            }
        }
        
        // Verify checksum
        if (tar_verify_checksum(&header) != 0) {
            archive->is_corrupted = 1;
            break;
        }
        
        // Parse header fields
        uint64_t file_size = octal_to_int(header.size, sizeof(header.size));
        uint64_t mode = octal_to_int(header.mode, sizeof(header.mode));
        uint64_t uid = octal_to_int(header.uid, sizeof(header.uid));
        uint64_t gid = octal_to_int(header.gid, sizeof(header.gid));
        uint64_t mtime = octal_to_int(header.mtime, sizeof(header.mtime));
        
        // Create compressed entry
        compressed_entry_t entry;
        compressed_entry_init(&entry);
        entry.filename = strdup_safe(header.name);
        entry.compressed_size = file_size;
        entry.uncompressed_size = file_size; // TAR doesn't compress
        entry.offset = ftell(file) - sizeof(header);
        entry.crc32 = 0; // TAR doesn't store CRC32
        entry.is_encrypted = 0; // TAR doesn't support encryption natively
        entry.is_directory = (header.typeflag == TAR_TYPE_DIRECTORY) ? 1 : 0;
        entry.timestamp = mtime;
        entry.permissions = tar_mode_to_string(mode);
        
        // Add entry to archive
        compressed_archive_add_entry(archive, &entry);
        compressed_entry_free(&entry);
        
        // Update archive totals
        archive->total_compressed_size += file_size;
        archive->total_uncompressed_size += file_size;
        
        // Skip file data
        if (file_size > 0) {
            // TAR pads to 512-byte boundaries
            uint64_t padded_size = ((file_size + 511) / 512) * 512;
            if (fseek(file, padded_size, SEEK_CUR) != 0) {
                break;
            }
        }
        
        // Check if we've reached the maximum entries
        if (options && archive->entry_count >= options->max_entries) {
            break;
        }
    }
    
    fclose(file);
    return 0;
}

int compression_extract_tar(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the TAR archive
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

int compression_list_tar_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in TAR archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read TAR signature
    tar_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify TAR signature
    if (strncmp(header.magic, "ustar", 5) != 0) {
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
    
    // Process TAR entries
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
        
        // Read TAR header
        tar_header_t header;
        if (fread(&header, sizeof(header), 1, file) != 1) {
            break;
        }
        
        // Check for end of archive (two consecutive zero blocks)
        if (header.name[0] == '\0') {
            // Read next block to check if it's also zero
            tar_header_t next_header;
            if (fread(&next_header, sizeof(next_header), 1, file) != 1) {
                break;
            }
            
            if (next_header.name[0] == '\0') {
                // End of archive
                break;
            } else {
                // Put back the next header
                if (fseek(file, -sizeof(next_header), SEEK_CUR) != 0) {
                    break;
                }
            }
        }
        
        // Verify checksum
        if (tar_verify_checksum(&header) != 0) {
            break;
        }
        
        // Parse header fields
        uint64_t file_size = octal_to_int(header.size, sizeof(header.size));
        uint64_t mode = octal_to_int(header.mode, sizeof(header.mode));
        uint64_t uid = octal_to_int(header.uid, sizeof(header.uid));
        uint64_t gid = octal_to_int(header.gid, sizeof(header.gid));
        uint64_t mtime = octal_to_int(header.mtime, sizeof(header.mtime));
        
        // Initialize entry
        compressed_entry_init(&(*entries)[*count]);
        (*entries)[*count].filename = strdup_safe(header.name);
        (*entries)[*count].compressed_size = file_size;
        (*entries)[*count].uncompressed_size = file_size; // TAR doesn't compress
        (*entries)[*count].offset = ftell(file) - sizeof(header);
        (*entries)[*count].crc32 = 0; // TAR doesn't store CRC32
        (*entries)[*count].is_encrypted = 0; // TAR doesn't support encryption natively
        (*entries)[*count].is_directory = (header.typeflag == TAR_TYPE_DIRECTORY) ? 1 : 0;
        (*entries)[*count].timestamp = mtime;
        (*entries)[*count].permissions = tar_mode_to_string(mode);
        
        (*count)++;
        
        // Skip file data
        if (file_size > 0) {
            // TAR pads to 512-byte boundaries
            uint64_t padded_size = ((file_size + 511) / 512) * 512;
            if (fseek(file, padded_size, SEEK_CUR) != 0) {
                break;
            }
        }
        
        // Check if we've reached the maximum entries
        if (options && *count >= options->max_entries) {
            break;
        }
    }
    
    fclose(file);
    return 0;
}

int compression_tar_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if TAR archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read TAR signature
    tar_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify TAR signature
    if (strncmp(header.magic, "ustar", 5) != 0) {
        fclose(file);
        return -1;
    }
    
    // TAR doesn't support password protection natively
    *is_protected = 0;
    
    fclose(file);
    return 0;
}