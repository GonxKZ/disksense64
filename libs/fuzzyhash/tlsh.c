#include "tlsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// TLSH constants
#define TLSH_CHECKSUM_LEN 1
#define TLSH_BUCKET_COUNT 128
#define TLSH_QRATIO_COUNT 16
#define TLSH_CODE_SIZE 32

// TLSH hash structure
typedef struct {
    uint8_t checksum[TLSH_CHECKSUM_LEN];
    uint8_t l_value;
    uint8_t q_ratios[TLSH_QRATIO_COUNT];
    uint8_t buckets[TLSH_BUCKET_COUNT];
} tlsh_hash_t;

// TLSH implementation
static uint8_t log_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

// Process data for TLSH hash
static int tlsh_process_data(const uint8_t* data, size_t len, tlsh_hash_t* result) {
    // Initialize result
    memset(result, 0, sizeof(tlsh_hash_t));
    
    if (len < 50 || len > 4294967295UL) {
        // TLSH requires at least 50 bytes and has a maximum size
        return -1;
    }
    
    // Simple bucket counting (simplified TLSH implementation)
    uint32_t buckets[TLSH_BUCKET_COUNT] = {0};
    
    // Process data in 512-byte chunks
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        // Simple bucket assignment based on byte value
        uint8_t bucket_idx = byte % TLSH_BUCKET_COUNT;
        buckets[bucket_idx]++;
    }
    
    // Convert counts to TLSH bucket values (simplified)
    for (int i = 0; i < TLSH_BUCKET_COUNT; i++) {
        if (buckets[i] > 255) {
            result->buckets[i] = 255;
        } else {
            result->buckets[i] = (uint8_t)buckets[i];
        }
    }
    
    // Set L value based on data length (simplified)
    if (len <= 256) {
        result->l_value = 0;
    } else if (len <= 1024) {
        result->l_value = 1;
    } else if (len <= 4096) {
        result->l_value = 2;
    } else if (len <= 16384) {
        result->l_value = 3;
    } else if (len <= 65536) {
        result->l_value = 4;
    } else if (len <= 262144) {
        result->l_value = 5;
    } else if (len <= 1048576) {
        result->l_value = 6;
    } else {
        result->l_value = 7;
    }
    
    // Set checksum (simplified)
    result->checksum[0] = (uint8_t)(len % 256);
    
    // Set Q ratios (simplified)
    for (int i = 0; i < TLSH_QRATIO_COUNT; i++) {
        result->q_ratios[i] = (uint8_t)(i % 4);
    }
    
    return 0;
}

// Format TLSH hash as string (simplified hex representation)
static char* format_tlsh_hash(const tlsh_hash_t* hash) {
    // TLSH hashes are typically 70 hex characters + null terminator
    char* result = (char*)malloc(71);
    if (!result) {
        return NULL;
    }
    
    // Format simplified TLSH hash
    int pos = 0;
    
    // Checksum
    pos += sprintf(result + pos, "%02x", hash->checksum[0]);
    
    // L value
    pos += sprintf(result + pos, "%01x", hash->l_value);
    
    // Q ratios (simplified)
    for (int i = 0; i < 4 && pos < 60; i++) {
        pos += sprintf(result + pos, "%01x", hash->q_ratios[i]);
    }
    
    // Buckets (simplified - first 8 buckets)
    for (int i = 0; i < 8 && pos < 70; i++) {
        pos += sprintf(result + pos, "%02x", hash->buckets[i]);
    }
    
    // Null terminate
    result[pos] = '\0';
    
    return result;
}

int tlsh_hash_file(const char* path, char** hash) {
    if (!path || !hash) {
        return -1;
    }
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return -1;
    }
    
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return -1;
    }
    
    // Compute hash
    tlsh_hash_t tlsh_result;
    int ret = tlsh_process_data(buffer, bytes_read, &tlsh_result);
    free(buffer);
    
    if (ret != 0) {
        return -1;
    }
    
    // Format result
    *hash = format_tlsh_hash(&tlsh_result);
    return (*hash) ? 0 : -1;
}

int tlsh_hash_data(const void* data, size_t size, char** hash) {
    if (!data || !hash || size == 0) {
        return -1;
    }
    
    // Compute hash
    tlsh_hash_t tlsh_result;
    int ret = tlsh_process_data((const uint8_t*)data, size, &tlsh_result);
    
    if (ret != 0) {
        return -1;
    }
    
    // Format result
    *hash = format_tlsh_hash(&tlsh_result);
    return (*hash) ? 0 : -1;
}

// Simplified comparison function
int tlsh_compare(const char* hash1, const char* hash2) {
    if (!hash1 || !hash2) {
        return 0;
    }
    
    // For now, return a simple string comparison score
    // In a full implementation, this would parse the hashes and compute
    // a proper distance metric
    if (strcmp(hash1, hash2) == 0) {
        return 100; // Identical
    }
    
    // Simple similarity check based on length and common characters
    size_t len1 = strlen(hash1);
    size_t len2 = strlen(hash2);
    
    if (len1 != len2) {
        return 0; // Different lengths
    }
    
    // Count matching characters
    int matches = 0;
    for (size_t i = 0; i < len1; i++) {
        if (hash1[i] == hash2[i]) {
            matches++;
        }
    }
    
    // Return percentage similarity
    return (matches * 100) / len1;
}