#include "blake3.h"
#include <string.h>
#include <assert.h>

// IV constants
static const uint32_t IV[8] = {
    0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
    0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

// Permutation
static const uint8_t SIGMA[10][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}
};

// Rotation constants
static const uint8_t R1[16] = {16, 12, 8, 7, 16, 12, 8, 7, 16, 12, 8, 7, 16, 12, 8, 7};
static const uint8_t R2[16] = {16, 12, 8, 7, 16, 12, 8, 7, 16, 12, 8, 7, 16, 12, 8, 7};

// G function
#define G(a, b, c, d, x, y, r1, r2) \
    do { \
        a += b + x; \
        d ^= a; \
        d = (d >> r1) | (d << (32 - r1)); \
        c += d; \
        b ^= c; \
        b = (b >> r2) | (b << (32 - r2)); \
        a += b + y; \
        d ^= a; \
        d = (d >> r1) | (d << (32 - r1)); \
        c += d; \
        b ^= c; \
        b = (b >> r2) | (b << (32 - r2)); \
    } while (0)

// Round function
static void blake3_round(uint32_t* v, const uint32_t* m, const uint8_t* sigma) {
    for (int i = 0; i < 16; i += 2) {
        G(v[sigma[i]], v[sigma[i + 1]], v[sigma[i] + 8], v[sigma[i + 1] + 8],
          m[sigma[i]], m[sigma[i + 1]], R1[i], R2[i]);
    }
}

// Compression function
static void blake3_compress(BLAKE3_HASH_STATE* state, const uint8_t block[BLAKE3_BLOCK_LEN], uint8_t flags) {
    uint32_t v[16];
    uint32_t m[16];
    
    // Convert block to words
    for (int i = 0; i < 16; i++) {
        m[i] = (block[i * 4] << 0) |
               (block[i * 4 + 1] << 8) |
               (block[i * 4 + 2] << 16) |
               (block[i * 4 + 3] << 24);
    }
    
    // Initialize state
    for (int i = 0; i < 8; i++) {
        v[i] = state->chaining[i];
    }
    for (int i = 0; i < 4; i++) {
        v[i + 8] = IV[i];
    }
    v[12] = (uint32_t)(state->count & 0xFFFFFFFF);
    v[13] = (uint32_t)(state->count >> 32);
    v[14] = 0; // counter high (not used in this simplified version)
    v[15] = flags;
    
    // 7 rounds
    for (int r = 0; r < 7; r++) {
        blake3_round(v, m, SIGMA[r]);
    }
    
    // XOR the chaining values
    for (int i = 0; i < 8; i++) {
        state->chaining[i] ^= v[i] ^ v[i + 8];
    }
}

void blake3_hash_init(BLAKE3_HASH_STATE* state) {
    for (int i = 0; i < 8; i++) {
        state->chaining[i] = IV[i];
    }
    state->count = 0;
    state->buf_len = 0;
    state->flags = 0;
}

void blake3_hash_init_keyed(BLAKE3_HASH_STATE* state, const uint8_t key[BLAKE3_KEY_LEN]) {
    for (int i = 0; i < 8; i++) {
        state->chaining[i] = (key[i * 4] << 0) |
                            (key[i * 4 + 1] << 8) |
                            (key[i * 4 + 2] << 16) |
                            (key[i * 4 + 3] << 24);
    }
    state->count = 0;
    state->buf_len = 0;
    state->flags = 0;
}

void blake3_hash_update(BLAKE3_HASH_STATE* state, const void* input, size_t input_len) {
    const uint8_t* data = (const uint8_t*)input;
    size_t remaining = input_len;
    
    // Process any buffered data
    if (state->buf_len > 0) {
        size_t take = BLAKE3_BLOCK_LEN - state->buf_len;
        if (take > remaining) {
            take = remaining;
        }
        memcpy(state->buf + state->buf_len, data, take);
        state->buf_len += take;
        data += take;
        remaining -= take;
        
        if (state->buf_len == BLAKE3_BLOCK_LEN) {
            blake3_compress(state, state->buf, 0);
            state->count++;
            state->buf_len = 0;
        }
    }
    
    // Process full blocks
    while (remaining >= BLAKE3_BLOCK_LEN) {
        blake3_compress(state, data, 0);
        state->count++;
        data += BLAKE3_BLOCK_LEN;
        remaining -= BLAKE3_BLOCK_LEN;
    }
    
    // Buffer any remaining data
    if (remaining > 0) {
        memcpy(state->buf, data, remaining);
        state->buf_len = remaining;
    }
}

void blake3_hash_finalize(const BLAKE3_HASH_STATE* state, uint8_t* out, size_t out_len) {
    // Create a copy of the state to avoid modifying the original
    BLAKE3_HASH_STATE final_state = *state;
    
    // Pad the buffer if necessary
    if (final_state.buf_len > 0) {
        memset(final_state.buf + final_state.buf_len, 0, BLAKE3_BLOCK_LEN - final_state.buf_len);
        blake3_compress(&final_state, final_state.buf, 0x80); // FLAG_END = 0x80
    } else if (final_state.count == 0) {
        // Handle empty input
        memset(final_state.buf, 0, BLAKE3_BLOCK_LEN);
        blake3_compress(&final_state, final_state.buf, 0x80); // FLAG_END = 0x80
    }
    
    // Output the hash
    size_t out_pos = 0;
    while (out_pos < out_len) {
        // For simplicity, we just output the chaining values
        // In a full implementation, we would do more complex output chaining
        for (int i = 0; i < 8 && out_pos < out_len; i++) {
            uint32_t word = final_state.chaining[i];
            for (int j = 0; j < 4 && out_pos < out_len; j++) {
                out[out_pos++] = (uint8_t)(word >> (j * 8));
            }
        }
        
        // If we need more output, we would do additional compression rounds
        // This is a simplified implementation that only outputs the first 32 bytes
        if (out_len > 32) {
            memset(out + 32, 0, out_len - 32);
            break;
        }
    }
}

void blake3_hash_finalize_xof(const BLAKE3_HASH_STATE* state, uint8_t* out, size_t out_len) {
    // For this simplified implementation, we just call the regular finalize
    blake3_hash_finalize(state, out, out_len);
}