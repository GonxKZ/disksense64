#ifndef LIBS_FUZZYHASH_FUZZY_HASH_H
#define LIBS_FUZZYHASH_FUZZY_HASH_H

// Ensure we define _GNU_SOURCE before any system headers
#ifdef __cplusplus
// For C++ files, we need to define _GNU_SOURCE before including any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fuzzy hash types
typedef enum {
    FUZZY_HASH_SSDEEP = 0,
    FUZZY_HASH_TLSH   = 1
} fuzzy_hash_type_t;

// Fuzzy hash result structure
typedef struct {
    fuzzy_hash_type_t type;
    char* hash_value;
    size_t hash_length;
} fuzzy_hash_result_t;

// Compute fuzzy hash for a file
// path: path to the file
// type: type of fuzzy hash to compute
// result: output fuzzy hash result (must be freed with fuzzy_hash_free)
// Returns 0 on success, non-zero on error
int fuzzy_hash_file(const char* path, fuzzy_hash_type_t type, fuzzy_hash_result_t* result);

// Compute fuzzy hash for data in memory
// data: pointer to data
// size: size of data in bytes
// type: type of fuzzy hash to compute
// result: output fuzzy hash result (must be freed with fuzzy_hash_free)
// Returns 0 on success, non-zero on error
int fuzzy_hash_data(const void* data, size_t size, fuzzy_hash_type_t type, fuzzy_hash_result_t* result);

// Free fuzzy hash result
void fuzzy_hash_free(fuzzy_hash_result_t* result);

// Compare two fuzzy hashes
// hash1: first hash
// hash2: second hash
// score: output similarity score (0-100, higher means more similar)
// Returns 0 on success, non-zero on error
int fuzzy_hash_compare(const fuzzy_hash_result_t* hash1, const fuzzy_hash_result_t* hash2, int* score);

// Check if two hashes are similar within a threshold
// hash1: first hash
// hash2: second hash
// threshold: similarity threshold (0-100)
// Returns 1 if similar, 0 if not similar, negative on error
int fuzzy_hash_is_similar(const fuzzy_hash_result_t* hash1, const fuzzy_hash_result_t* hash2, int threshold);

#ifdef __cplusplus
}
#endif

#endif // LIBS_FUZZYHASH_FUZZY_HASH_H