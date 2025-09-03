#include <stdio.h>
#include <string.h>
#include "libs/chash/blake3.h"

int main() {
    BLAKE3_HASH_STATE state;
    uint8_t hash[32];
    const char* input = "Hello, World!";
    
    blake3_hash_init(&state);
    blake3_hash_update(&state, input, strlen(input));
    blake3_hash_finalize(&state, hash, sizeof(hash));
    
    printf("BLAKE3 hash of '%s': ", input);
    for (int i = 0; i < 32; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
    
    return 0;
}