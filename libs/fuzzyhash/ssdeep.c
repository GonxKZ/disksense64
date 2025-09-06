#include "ssdeep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// SSDeep constants
#define SSDEEP_BLOCKSIZE_MIN 3
#define SSDEEP_BLOCKSIZE_MAX 4096
#define SSDEEP_SPAMSUM_LENGTH 64
#define SSDEEP_CHUNK_SIZE 64

// SSDeep hash structure
typedef struct {
    uint32_t blocksize;
    char hash1[SSDEEP_SPAMSUM_LENGTH + 1];
    char hash2[SSDEEP_SPAMSUM_LENGTH + 1];
} ssdeep_hash_t;

// Rolling hash context
typedef struct {
    uint32_t window[7];
    uint32_t h1, h2, h3;
    uint32_t n;
} rolling_state_t;

// Base64-like encoding for SSDeep
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Rolling hash functions
static void roll_init(rolling_state_t* state) {
    memset(state, 0, sizeof(rolling_state_t));
}

static uint32_t roll_hash(rolling_state_t* state, uint8_t c) {
    state->h1 = (state->h1 << 4) | (state->h1 >> 28);
    state->h1 += c;
    state->h2 += c;
    state->h2 += state->h1;
    state->h3 <<= 1;
    state->h3 += c;
    
    state->window[state->n % 7] = c;
    state->n++;
    
    return state->h1 + state->h2 + state->h3;
}

// Sum hash function
static uint32_t sum_hash(uint32_t blocksize, const uint8_t* data, size_t len) {
    uint32_t hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash = (hash * 256 + data[i]) % blocksize;
    }
    return hash;
}

// Get base64 character
static char get_b64_char(uint32_t value) {
    return b64[value % 64];
}

// Process data for SSDeep hash
static int ssdeep_process_data(const uint8_t* data, size_t len, ssdeep_hash_t* result) {
    rolling_state_t roll_state;
    roll_init(&roll_state);
    
    // Initialize result
    memset(result, 0, sizeof(ssdeep_hash_t));
    
    // Find optimal block size
    uint32_t blocksize = SSDEEP_BLOCKSIZE_MIN;
    while (blocksize * SSDEEP_SPAMSUM_LENGTH < len) {
        blocksize *= 2;
    }
    
    // Cap blocksize to maximum
    if (blocksize > SSDEEP_BLOCKSIZE_MAX) {
        blocksize = SSDEEP_BLOCKSIZE_MAX;
    }
    
    result->blocksize = blocksize;
    
    // Process data in chunks
    uint32_t sum1 = 0, sum2 = 0;
    size_t hash1_len = 0, hash2_len = 0;
    
    for (size_t i = 0; i < len && hash1_len < SSDEEP_SPAMSUM_LENGTH; i++) {
        uint32_t rh = roll_hash(&roll_state, data[i]);
        
        // Check for block boundary in first hash
        if ((rh % blocksize) == (blocksize - 1) && hash1_len < SSDEEP_SPAMSUM_LENGTH - 1) {
            result->hash1[hash1_len++] = get_b64_char(sum1);
            sum1 = 0;
        }
        
        // Check for block boundary in second hash
        if ((rh % (blocksize * 2)) == (blocksize * 2 - 1) && hash2_len < SSDEEP_SPAMSUM_LENGTH - 1) {
            result->hash2[hash2_len++] = get_b64_char(sum2);
            sum2 = 0;
        }
        
        // Update sums
        sum1 = (sum1 * 256 + data[i]) % 0xFFFFFFFF;
        sum2 = (sum2 * 256 + data[i]) % 0xFFFFFFFF;
    }
    
    // Add final characters if needed
    if (hash1_len < SSDEEP_SPAMSUM_LENGTH && sum1 != 0) {
        result->hash1[hash1_len++] = get_b64_char(sum1);
    }
    
    if (hash2_len < SSDEEP_SPAMSUM_LENGTH && sum2 != 0) {
        result->hash2[hash2_len++] = get_b64_char(sum2);
    }
    
    // Null terminate
    result->hash1[hash1_len] = '\0';
    result->hash2[hash2_len] = '\0';
    
    return 0;
}

// Format SSDeep hash as string
static char* format_ssdeep_hash(const ssdeep_hash_t* hash) {
    size_t len = snprintf(NULL, 0, "%u:%s:%s", hash->blocksize, hash->hash1, hash->hash2);
    char* result = (char*)malloc(len + 1);
    if (result) {
        snprintf(result, len + 1, "%u:%s:%s", hash->blocksize, hash->hash1, hash->hash2);
    }
    return result;
}

int ssdeep_hash_file(const char* path, char** hash) {
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
    ssdeep_hash_t ssdeep_result;
    int ret = ssdeep_process_data(buffer, bytes_read, &ssdeep_result);
    free(buffer);
    
    if (ret != 0) {
        return -1;
    }
    
    // Format result
    *hash = format_ssdeep_hash(&ssdeep_result);
    return (*hash) ? 0 : -1;
}

int ssdeep_hash_data(const void* data, size_t size, char** hash) {
    if (!data || !hash || size == 0) {
        return -1;
    }
    
    // Compute hash
    ssdeep_hash_t ssdeep_result;
    int ret = ssdeep_process_data((const uint8_t*)data, size, &ssdeep_result);
    
    if (ret != 0) {
        return -1;
    }
    
    // Format result
    *hash = format_ssdeep_hash(&ssdeep_result);
    return (*hash) ? 0 : -1;
}

// Simplified comparison function
int ssdeep_compare(const char* hash1, const char* hash2) {
    if (!hash1 || !hash2) {
        return 0;
    }
    
    // For now, return a simple string comparison score
    // In a full implementation, this would parse the hashes and compute
    // a proper similarity score based on the SSDeep algorithm
    if (strcmp(hash1, hash2) == 0) {
        return 100;
    }
    
    // Simple heuristic: if they start with the same blocksize, they might be similar
    unsigned int blocksize1, blocksize2;
    if (sscanf(hash1, "%u:", &blocksize1) == 1 && 
        sscanf(hash2, "%u:", &blocksize2) == 1) {
        if (blocksize1 == blocksize2) {
            return 50; // Moderate similarity
        }
    }
    
    return 0; // No similarity
}