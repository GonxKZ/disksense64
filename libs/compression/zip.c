#include "zip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <zlib.h>

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

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert DOS time to Unix time
static time_t dos_time_to_unix(uint16_t dos_date, uint16_t dos_time) {
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(tm_time));
    
    tm_time.tm_year = ((dos_date >> 9) & 0x7F) + 80;  // Year since 1900
    tm_time.tm_mon = ((dos_date >> 5) & 0x0F) - 1;    // Month (0-11)
    tm_time.tm_mday = dos_date & 0x1F;                // Day (1-31)
    tm_time.tm_hour = (dos_time >> 11) & 0x1F;        // Hour (0-23)
    tm_time.tm_min = (dos_time >> 5) & 0x3F;          // Minute (0-59)
    tm_time.tm_sec = (dos_time & 0x1F) * 2;           // Second (0-59, multiplied by 2)
    
    return mktime(&tm_time);
}

// Convert file attributes to string representation
static char* attributes_to_string(uint32_t attributes) {
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

// Find end of central directory record
static int find_end_of_central_directory(FILE* file, zip_end_of_central_directory_t* eocd) {
    if (!file || !eocd) {
        return -1;
    }
    
    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        return -1;
    }
    
    long file_size = ftell(file);
    if (file_size < 0) {
        return -1;
    }
    
    // Search backwards for EOCD signature
    long search_start = (file_size - 65536 > 0) ? file_size - 65536 : 0;
    
    if (fseek(file, search_start, SEEK_SET) != 0) {
        return -1;
    }
    
    // Read in chunks
    char buffer[1024];
    long pos = search_start;
    
    while (pos < file_size) {
        size_t bytes_to_read = (file_size - pos > (long)sizeof(buffer)) ? sizeof(buffer) : (file_size - pos);
        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
        
        if (bytes_read == 0) {
            break;
        }
        
        // Search for EOCD signature in buffer
        for (size_t i = 0; i <= bytes_read - 4; i++) {
            if (buffer[i] == 0x50 && buffer[i+1] == 0x4B && buffer[i+2] == 0x05 && buffer[i+3] == 0x06) {
                // Found EOCD signature
                if (fseek(file, pos + i, SEEK_SET) == 0) {
                    if (fread(eocd, sizeof(zip_end_of_central_directory_t), 1, file) == 1) {
                        if (eocd->signature == 0x06054B50) {
                            return 0;
                        }
                    }
                }
                break;
            }
        }
        
        pos += bytes_read;
        
        // Adjust position to overlap previous buffer for signature detection
        if (pos < file_size && bytes_read >= 4) {
            pos -= 3; // Overlap by 3 bytes
            if (fseek(file, pos, SEEK_SET) != 0) {
                break;
            }
        }
    }
    
    return -1;
}

int compression_analyze_zip(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZIP signature
    uint32_t signature;
    if (fread(&signature, sizeof(signature), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZIP signature
    if (signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE) {
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_ZIP;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_ZIP));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Find end of central directory
    zip_end_of_central_directory_t eocd;
    if (find_end_of_central_directory(file, &eocd) != 0) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Check for password protection
    archive->is_password_protected = (eocd.disk_number != 0) ? 1 : 0;
    
    // Seek to central directory
    if (fseek(file, eocd.central_dir_offset, SEEK_SET) != 0) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Process central directory entries
    for (int i = 0; i < eocd.num_entries_total; i++) {
        // Read central directory entry header
        zip_central_directory_entry_t cd_header;
        if (fread(&cd_header, sizeof(cd_header), 1, file) != 1) {
            break;
        }
        
        // Verify signature
        if (cd_header.signature != ZIP_CENTRAL_DIRECTORY_ENTRY_SIGNATURE) {
            archive->is_corrupted = 1;
            break;
        }
        
        // Read filename
        char* filename = (char*)malloc(cd_header.filename_length + 1);
        if (!filename) {
            break;
        }
        
        if (fread(filename, cd_header.filename_length, 1, file) != 1) {
            free(filename);
            break;
        }
        filename[cd_header.filename_length] = '\0';
        
        // Skip extra field and file comment
        if (fseek(file, cd_header.extra_field_length + cd_header.file_comment_length, SEEK_CUR) != 0) {
            free(filename);
            break;
        }
        
        // Create compressed entry
        compressed_entry_t entry;
        compressed_entry_init(&entry);
        entry.filename = filename;
        entry.compressed_size = cd_header.compressed_size;
        entry.uncompressed_size = cd_header.uncompressed_size;
        entry.offset = cd_header.relative_offset_of_local_header;
        entry.crc32 = cd_header.crc32;
        entry.is_encrypted = (cd_header.flags & ZIP_FLAG_ENCRYPTED) ? 1 : 0;
        entry.is_directory = (cd_header.external_file_attributes & 0x10) ? 1 : 0;
        entry.timestamp = dos_time_to_unix(cd_header.last_mod_date, cd_header.last_mod_time);
        entry.permissions = attributes_to_string(cd_header.external_file_attributes);
        
        // Add entry to archive
        compressed_archive_add_entry(archive, &entry);
        compressed_entry_free(&entry);
        free(filename);
        
        // Update archive totals
        archive->total_compressed_size += cd_header.compressed_size;
        archive->total_uncompressed_size += cd_header.uncompressed_size;
        
        // Check password protection
        if (cd_header.flags & ZIP_FLAG_ENCRYPTED) {
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

int compression_extract_zip(const char* archive_path,
                          const char* entry_name,
                          const char* output_path,
                          const char* password) {
    if (!archive_path || !entry_name || !output_path) {
        return -1;
    }

    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }

    zip_end_of_central_directory_t eocd;
    if (find_end_of_central_directory(file, &eocd) != 0) {
        fclose(file);
        return -1;
    }

    if (fseek(file, eocd.central_dir_offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    zip_central_directory_entry_t cd_header;
    long entry_offset = -1;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint16_t compression_method = 0;
    uint16_t flags = 0;

    for (int i = 0; i < eocd.num_entries_total; i++) {
        if (fread(&cd_header, sizeof(cd_header), 1, file) != 1) {
            fclose(file);
            return -1;
        }

        if (cd_header.signature != ZIP_CENTRAL_DIRECTORY_ENTRY_SIGNATURE) {
            fclose(file);
            return -1;
        }

        char* filename = (char*)malloc(cd_header.filename_length + 1);
        if (!filename) {
            fclose(file);
            return -1;
        }

        if (fread(filename, cd_header.filename_length, 1, file) != 1) {
            free(filename);
            fclose(file);
            return -1;
        }
        filename[cd_header.filename_length] = '\0';

        if (strcmp(filename, entry_name) == 0) {
            entry_offset = cd_header.relative_offset_of_local_header;
            compressed_size = cd_header.compressed_size;
            uncompressed_size = cd_header.uncompressed_size;
            compression_method = cd_header.compression_method;
            flags = cd_header.flags;
            free(filename);
            break;
        }

        free(filename);
        fseek(file, cd_header.extra_field_length + cd_header.file_comment_length, SEEK_CUR);
    }

    if (entry_offset == -1) {
        fclose(file);
        return -1; // Entry not found
    }

    if (fseek(file, entry_offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    zip_local_file_header_t lfh;
    if (fread(&lfh, sizeof(lfh), 1, file) != 1) {
        fclose(file);
        return -1;
    }

    if (lfh.signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE) {
        fclose(file);
        return -1;
    }

    fseek(file, lfh.filename_length + lfh.extra_field_length, SEEK_CUR);

    if (compression_method != ZIP_COMPRESSION_DEFLATE && compression_method != ZIP_COMPRESSION_STORE) {
        // Unsupported compression method
        fclose(file);
        return -1;
    }
    
    if (flags & ZIP_FLAG_ENCRYPTED) {
        // Password protected files not supported in this implementation
        fclose(file);
        return -1;
    }

    FILE* out_file = fopen(output_path, "wb");
    if (!out_file) {
        fclose(file);
        return -1;
    }

    if (compression_method == ZIP_COMPRESSION_STORE) {
        char* buffer = (char*)malloc(compressed_size);
        if (!buffer) {
            fclose(file);
            fclose(out_file);
            return -1;
        }
        if (fread(buffer, compressed_size, 1, file) != 1) {
            free(buffer);
            fclose(file);
            fclose(out_file);
            return -1;
        }
        if (fwrite(buffer, uncompressed_size, 1, out_file) != 1) {
            free(buffer);
            fclose(file);
            fclose(out_file);
            return -1;
        }
        free(buffer);
    } else { // DEFLATE
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
            fclose(file);
            fclose(out_file);
            return -1;
        }

        unsigned char in[4096];
        unsigned char out[4096];
        unsigned long total_read = 0;

        do {
            unsigned long read_size = (compressed_size - total_read < sizeof(in)) ? (compressed_size - total_read) : sizeof(in);
            strm.avail_in = fread(in, 1, read_size, file);
            total_read += strm.avail_in;

            if (ferror(file)) {
                (void)inflateEnd(&strm);
                fclose(file);
                fclose(out_file);
                return -1;
            }
            if (strm.avail_in == 0)
                break;
            strm.next_in = in;

            do {
                strm.avail_out = sizeof(out);
                strm.next_out = out;
                int ret = inflate(&strm, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR) {
                    (void)inflateEnd(&strm);
                    fclose(file);
                    fclose(out_file);
                    return -1;
                }
                switch (ret) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    fclose(file);
                    fclose(out_file);
                    return -1;
                }
                unsigned have = sizeof(out) - strm.avail_out;
                if (fwrite(out, 1, have, out_file) != have || ferror(out_file)) {
                    (void)inflateEnd(&strm);
                    fclose(file);
                    fclose(out_file);
                    return -1;
                }
            } while (strm.avail_out == 0);
        } while (total_read < compressed_size);

        (void)inflateEnd(&strm);
    }

    fclose(file);
    fclose(out_file);

    return 0;
}

int compression_list_zip_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in ZIP archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZIP signature
    uint32_t signature;
    if (fread(&signature, sizeof(signature), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZIP signature
    if (signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE) {
        fclose(file);
        return -1;
    }
    
    // Find end of central directory
    zip_end_of_central_directory_t eocd;
    if (find_end_of_central_directory(file, &eocd) != 0) {
        fclose(file);
        return -1;
    }
    
    // Seek to central directory
    if (fseek(file, eocd.central_dir_offset, SEEK_SET) != 0) {
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
    
    // Process central directory entries
    for (int i = 0; i < eocd.num_entries_total; i++) {
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
        
        // Read central directory entry header
        zip_central_directory_entry_t cd_header;
        if (fread(&cd_header, sizeof(cd_header), 1, file) != 1) {
            break;
        }
        
        // Verify signature
        if (cd_header.signature != ZIP_CENTRAL_DIRECTORY_ENTRY_SIGNATURE) {
            break;
        }
        
        // Read filename
        char* filename = (char*)malloc(cd_header.filename_length + 1);
        if (!filename) {
            break;
        }
        
        if (fread(filename, cd_header.filename_length, 1, file) != 1) {
            free(filename);
            break;
        }
        filename[cd_header.filename_length] = '\0';
        
        // Skip extra field and file comment
        if (fseek(file, cd_header.extra_field_length + cd_header.file_comment_length, SEEK_CUR) != 0) {
            free(filename);
            break;
        }
        
        // Initialize entry
        compressed_entry_init(&(*entries)[*count]);
        (*entries)[*count].filename = filename;
        (*entries)[*count].compressed_size = cd_header.compressed_size;
        (*entries)[*count].uncompressed_size = cd_header.uncompressed_size;
        (*entries)[*count].offset = cd_header.relative_offset_of_local_header;
        (*entries)[*count].crc32 = cd_header.crc32;
        (*entries)[*count].is_encrypted = (cd_header.flags & ZIP_FLAG_ENCRYPTED) ? 1 : 0;
        (*entries)[*count].is_directory = (cd_header.external_file_attributes & 0x10) ? 1 : 0;
        (*entries)[*count].timestamp = dos_time_to_unix(cd_header.last_mod_date, cd_header.last_mod_time);
        (*entries)[*count].permissions = attributes_to_string(cd_header.external_file_attributes);
        
        free(filename);
        (*count)++;
    }
    
    fclose(file);
    return 0;
}

int compression_zip_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if ZIP archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read ZIP signature
    uint32_t signature;
    if (fread(&signature, sizeof(signature), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify ZIP signature
    if (signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE) {
        fclose(file);
        return -1;
    }
    
    // Find end of central directory
    zip_end_of_central_directory_t eocd;
    if (find_end_of_central_directory(file, &eocd) != 0) {
        fclose(file);
        return -1;
    }
    
    // Check for password protection
    if (eocd.disk_number != 0) {
        *is_protected = 1;
    }
    
    // Seek to central directory
    if (fseek(file, eocd.central_dir_offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    
    // Check first entry for encryption flag
    if (eocd.num_entries_total > 0) {
        zip_central_directory_entry_t cd_header;
        if (fread(&cd_header, sizeof(cd_header), 1, file) == 1) {
            if (cd_header.signature == ZIP_CENTRAL_DIRECTORY_ENTRY_SIGNATURE) {
                *is_protected = (cd_header.flags & ZIP_FLAG_ENCRYPTED) ? 1 : 0;
            }
        }
    }
    
    fclose(file);
    return 0;
}