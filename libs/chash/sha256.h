#ifndef LIBS_CHASH_SHA256_H
#define LIBS_CHASH_SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_BLOCK_SIZE 32

// Ensure proper C linkage
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CHASH_SHA256_H
