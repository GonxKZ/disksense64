#ifndef LIBS_FUZZYHASH_SSDEEP_H
#define LIBS_FUZZYHASH_SSDEEP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute SSDeep hash for a file
// path: path to the file
// hash: output hash string (must be freed by caller)
// Returns 0 on success, non-zero on error
int ssdeep_hash_file(const char* path, char** hash);

// Compute SSDeep hash for data in memory
// data: pointer to data
// size: size of data in bytes
// hash: output hash string (must be freed by caller)
// Returns 0 on success, non-zero on error
int ssdeep_hash_data(const void* data, size_t size, char** hash);

// Compare two SSDeep hashes
// hash1: first hash
// hash2: second hash
// Returns similarity score (0-100, higher means more similar)
int ssdeep_compare(const char* hash1, const char* hash2);

#ifdef __cplusplus
}
#endif

#endif // LIBS_FUZZYHASH_SSDEEP_H