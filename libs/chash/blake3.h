#ifndef LIBS_CHASH_BLAKE3_H
#define LIBS_CHASH_BLAKE3_H

#include <stdint.h>
#include <stddef.h>

#define BLAKE3_KEY_LEN 32
#define BLAKE3_OUT_LEN 32
#define BLAKE3_BLOCK_LEN 64
#define BLAKE3_CHUNK_LEN 1024

// Ensure proper C linkage
#ifdef __cplusplus
extern "C" {
#endif

// BLAKE3 state structure
typedef struct {
    uint32_t chaining[8];
    uint32_t iv[4];
    uint64_t count;
    uint8_t buf[BLAKE3_BLOCK_LEN];
    size_t buf_len;
    uint8_t flags;
} BLAKE3_HASH_STATE;

// Keyed hashing
typedef struct {
    uint32_t key[8];
} blake3_keyed_hasher;

// Public API
void blake3_hash_init(BLAKE3_HASH_STATE* state);
void blake3_hash_update(BLAKE3_HASH_STATE* state, const void* input, size_t input_len);
void blake3_hash_finalize(const BLAKE3_HASH_STATE* state, uint8_t* out, size_t out_len);

// Keyed hashing
void blake3_hash_init_keyed(BLAKE3_HASH_STATE* state, const uint8_t key[BLAKE3_KEY_LEN]);

// Extended output
void blake3_hash_finalize_xof(const BLAKE3_HASH_STATE* state, uint8_t* out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CHASH_BLAKE3_H