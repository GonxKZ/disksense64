#include "fuzzy_hash.h"
#include "ssdeep.h"
#include "tlsh.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
static int fuzzy_hash_file_ssdeep(const char* path, fuzzy_hash_result_t* result);
static int fuzzy_hash_file_tlsh(const char* path, fuzzy_hash_result_t* result);
static int fuzzy_hash_data_ssdeep(const void* data, size_t size, fuzzy_hash_result_t* result);
static int fuzzy_hash_data_tlsh(const void* data, size_t size, fuzzy_hash_result_t* result);

int fuzzy_hash_file(const char* path, fuzzy_hash_type_t type, fuzzy_hash_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    // Initialize result
    result->type = type;
    result->hash_value = NULL;
    result->hash_length = 0;
    
    switch (type) {
        case FUZZY_HASH_SSDEEP:
            return fuzzy_hash_file_ssdeep(path, result);
        case FUZZY_HASH_TLSH:
            return fuzzy_hash_file_tlsh(path, result);
        default:
            return -1;
    }
}

int fuzzy_hash_data(const void* data, size_t size, fuzzy_hash_type_t type, fuzzy_hash_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    // Initialize result
    result->type = type;
    result->hash_value = NULL;
    result->hash_length = 0;
    
    switch (type) {
        case FUZZY_HASH_SSDEEP:
            return fuzzy_hash_data_ssdeep(data, size, result);
        case FUZZY_HASH_TLSH:
            return fuzzy_hash_data_tlsh(data, size, result);
        default:
            return -1;
    }
}

void fuzzy_hash_free(fuzzy_hash_result_t* result) {
    if (result && result->hash_value) {
        free(result->hash_value);
        result->hash_value = NULL;
        result->hash_length = 0;
    }
}

int fuzzy_hash_compare(const fuzzy_hash_result_t* hash1, const fuzzy_hash_result_t* hash2, int* score) {
    if (!hash1 || !hash2 || !score) {
        return -1;
    }
    
    // Both hashes must be of the same type
    if (hash1->type != hash2->type) {
        return -1;
    }
    
    switch (hash1->type) {
        case FUZZY_HASH_SSDEEP:
            // SSDeep comparison returns a score from 0-100
            *score = ssdeep_compare(hash1->hash_value, hash2->hash_value);
            return 0;
        case FUZZY_HASH_TLSH:
            // TLSH comparison returns a distance, convert to similarity score
            *score = tlsh_compare(hash1->hash_value, hash2->hash_value);
            return 0;
        default:
            return -1;
    }
}

int fuzzy_hash_is_similar(const fuzzy_hash_result_t* hash1, const fuzzy_hash_result_t* hash2, int threshold) {
    if (!hash1 || !hash2 || threshold < 0 || threshold > 100) {
        return -1;
    }
    
    // Both hashes must be of the same type
    if (hash1->type != hash2->type) {
        return -1;
    }
    
    int score = 0;
    int result = fuzzy_hash_compare(hash1, hash2, &score);
    if (result != 0) {
        return result;
    }
    
    return (score >= threshold) ? 1 : 0;
}

// SSDeep implementation
static int fuzzy_hash_file_ssdeep(const char* path, fuzzy_hash_result_t* result) {
    char* hash = NULL;
    int ret = ssdeep_hash_file(path, &hash);
    
    if (ret == 0 && hash) {
        result->hash_value = hash;
        result->hash_length = strlen(hash);
        return 0;
    }
    
    return -1;
}

static int fuzzy_hash_data_ssdeep(const void* data, size_t size, fuzzy_hash_result_t* result) {
    char* hash = NULL;
    int ret = ssdeep_hash_data(data, size, &hash);
    
    if (ret == 0 && hash) {
        result->hash_value = hash;
        result->hash_length = strlen(hash);
        return 0;
    }
    
    return -1;
}

// TLSH implementation
static int fuzzy_hash_file_tlsh(const char* path, fuzzy_hash_result_t* result) {
    char* hash = NULL;
    int ret = tlsh_hash_file(path, &hash);
    
    if (ret == 0 && hash) {
        result->hash_value = hash;
        result->hash_length = strlen(hash);
        return 0;
    }
    
    return -1;
}

static int fuzzy_hash_data_tlsh(const void* data, size_t size, fuzzy_hash_result_t* result) {
    char* hash = NULL;
    int ret = tlsh_hash_data(data, size, &hash);
    
    if (ret == 0 && hash) {
        result->hash_value = hash;
        result->hash_length = strlen(hash);
        return 0;
    }
    
    return -1;
}