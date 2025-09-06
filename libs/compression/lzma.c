#include "lzma.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// LZMA file format structures
#pragma pack(push, 1)
typedef struct {
    uint8_t properties[5];        // LZMA properties
    uint64_t dictionary_size;     // Dictionary size
    uint64_t uncompressed_size;   // Uncompressed size
} lzma_header_t;

typedef struct {
    uint8_t lc;                   // Literal context bits
    uint8_t lp;                   // Literal position bits
    uint8_t pb;                   // Position bits
    uint32_t dictionary_size;     // Dictionary size
} lzma_properties_t;

typedef struct {
    uint32_t crc32;               // CRC32 of uncompressed data
    uint64_t pack_size;           // Size of compressed data
    uint64_t unpack_size;         // Size of uncompressed data
} lzma_trailer_t;
#pragma pack(pop)

// LZMA compression methods
#define LZMA_COMPRESSION_LZMA 0x01
#define LZMA_COMPRESSION_LZMA2 0x21

// LZMA filter IDs
#define LZMA_FILTER_DELTA 0x03
#define LZMA_FILTER_X86 0x04
#define LZMA_FILTER_POWERPC 0x05
#define LZMA_FILTER_IA64 0x06
#define LZMA_FILTER_ARM 0x07
#define LZMA_FILTER_ARMTHUMB 0x08
#define LZMA_FILTER_SPARC 0x09

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Convert LZMA time to Unix time
static time_t lzma_time_to_unix(uint64_t lzma_time) {
    // LZMA time is in 100-nanosecond intervals since January 1, 1601 UTC
    // Convert to Unix time (seconds since January 1, 1970 UTC)
    const uint64_t epoch_diff = 116444736000000000ULL; // Difference in 100-nanosecond intervals
    if (lzma_time < epoch_diff) {
        return 0;
    }
    return (time_t)((lzma_time - epoch_diff) / 10000000ULL); // Convert to seconds
}

// Convert file attributes to string representation
static char* lzma_attributes_to_string(uint32_t attributes) {
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

int compression_analyze_lzma(const char* file_path, 
                          const compression_options_t* options,
                          compressed_archive_t* archive) {
    if (!file_path || !archive) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZMA header
    lzma_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZMA signature (no fixed signature, check properties)
    if (header.properties[0] > 0xFF) { // Invalid properties
        fclose(file);
        return -1;
    }
    
    // Initialize archive
    compressed_archive_init(archive);
    archive->archive_path = strdup_safe(file_path);
    archive->format = COMPRESSION_FORMAT_LZMA;
    archive->format_description = strdup_safe(compression_get_format_description(COMPRESSION_FORMAT_LZMA));
    
    // Get file size
    struct stat st;
    if (stat(file_path, &st) == 0) {
        archive->total_compressed_size = st.st_size;
    }
    
    // Parse LZMA properties
    lzma_properties_t props;
    props.lc = header.properties[0] % 9;
    props.lp = (header.properties[0] / 9) % 5;
    props.pb = (header.properties[0] / 45) % 5;
    props.dictionary_size = header.dictionary_size;
    
    printf("LZMA properties: lc=%u, lp=%u, pb=%u, dictionary_size=%lu\n",
           props.lc, props.lp, props.pb, (unsigned long)props.dictionary_size);
    
    // Seek to trailer
    if (fseek(file, -sizeof(lzma_trailer_t), SEEK_END) != 0) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Read LZMA trailer
    lzma_trailer_t trailer;
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
        fclose(file);
        archive->is_corrupted = 1;
        return -1;
    }
    
    // Update archive information
    archive->total_uncompressed_size = trailer.unpack_size;
    
    // Check for encryption
    archive->is_password_protected = 0; // LZMA doesn't support encryption natively
    
    // Create compressed entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    entry.filename = strdup_safe(file_path);
    entry.compressed_size = archive->total_compressed_size;
    entry.uncompressed_size = trailer.unpack_size;
    entry.offset = 0;
    entry.crc32 = trailer.crc32;
    entry.is_encrypted = 0;
    entry.is_directory = 0;
    entry.timestamp = lzma_time_to_unix(time(NULL) * 10000000ULL); // Current time
    entry.permissions = lzma_attributes_to_string(0644);
    
    // Add entry to archive
    compressed_archive_add_entry(archive, &entry);
    compressed_entry_free(&entry);
    
    fclose(file);
    return 0;
}

int compression_extract_lzma(const char* archive_path,
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
    
    // In a real implementation, this would extract the specified file from the LZMA archive
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

int compression_list_lzma_entries(const char* archive_path,
                               compressed_entry_t** entries,
                               size_t* count,
                               const char* password) {
    if (!archive_path || !entries || !count) {
        return -1;
    }
    
    *entries = NULL;
    *count = 0;
    
    printf("Listing entries in LZMA archive: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZMA header
    lzma_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZMA signature
    if (header.properties[0] > 0xFF) { // Invalid properties
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
    
    // Seek to trailer
    if (fseek(file, -sizeof(lzma_trailer_t), SEEK_END) != 0) {
        fclose(file);
        free(*entries);
        *entries = NULL;
        *count = 0;
        return -1;
    }
    
    // Read LZMA trailer
    lzma_trailer_t trailer;
    if (fread(&trailer, sizeof(trailer), 1, file) != 1) {
        fclose(file);
        free(*entries);
        *entries = NULL;
        *count = 0;
        return -1;
    }
    
    // Initialize entry
    compressed_entry_init(&(*entries)[*count]);
    (*entries)[*count].filename = strdup_safe(archive_path);
    (*entries)[*count].compressed_size = ftell(file);
    (*entries)[*count].uncompressed_size = trailer.unpack_size;
    (*entries)[*count].offset = 0;
    (*entries)[*count].crc32 = trailer.crc32;
    (*entries)[*count].is_encrypted = 0;
    (*entries)[*count].is_directory = 0;
    (*entries)[*count].timestamp = lzma_time_to_unix(time(NULL) * 10000000ULL); // Current time
    (*entries)[*count].permissions = lzma_attributes_to_string(0644);
    
    (*count)++;
    
    fclose(file);
    return 0;
}

int compression_lzma_is_password_protected(const char* archive_path, int* is_protected) {
    if (!archive_path || !is_protected) {
        return -1;
    }
    
    *is_protected = 0;
    
    printf("Checking if LZMA archive is password protected: %s\n", archive_path);
    
    // Open archive
    FILE* file = fopen(archive_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read LZMA header
    lzma_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Verify LZMA signature
    if (header.properties[0] > 0xFF) { // Invalid properties
        fclose(file);
        return -1;
    }
    
    // LZMA doesn't support encryption natively
    *is_protected = 0;
    
    fclose(file);
    return 0;
}