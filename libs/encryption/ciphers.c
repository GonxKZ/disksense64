#include "ciphers.h"
#include <stdlib.h>
#include <string.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

int encryption_get_cipher_info_internal(cipher_info_t** ciphers, size_t* count) {
    if (!ciphers || !count) {
        return -1;
    }
    
    // Define supported ciphers
    static const int cipher_count = 8;
    static cipher_info_t cipher_data[] = {
        {
            "AES", 
            "Advanced Encryption Standard",
            16,
            {128, 192, 256},
            3,
            {"ECB", "CBC", "CFB", "OFB", "CTR", "GCM"},
            6
        },
        {
            "DES",
            "Data Encryption Standard (deprecated)",
            8,
            {56},
            1,
            {"ECB", "CBC", "CFB", "OFB"},
            4
        },
        {
            "3DES",
            "Triple DES",
            8,
            {112, 168},
            2,
            {"ECB", "CBC", "CFB", "OFB"},
            4
        },
        {
            "Blowfish",
            "Symmetric block cipher",
            8,
            {32, 64, 128, 256, 448},
            5,
            {"ECB", "CBC", "CFB", "OFB"},
            4
        },
        {
            "Twofish",
            "Successor to Blowfish",
            16,
            {128, 192, 256},
            3,
            {"ECB", "CBC", "CFB", "OFB", "CTR"},
            5
        },
        {
            "RC4",
            "Stream cipher",
            1,
            {40, 56, 64, 128, 256},
            5,
            {"Stream"},
            1
        },
        {
            "ChaCha20",
            "Stream cipher",
            1,
            {256},
            1,
            {"Stream"},
            1
        },
        {
            "Salsa20",
            "Stream cipher",
            1,
            {128, 256},
            2,
            {"Stream"},
            1
        }
    };
    
    // Allocate and copy cipher information
    *ciphers = (cipher_info_t*)malloc(cipher_count * sizeof(cipher_info_t));
    if (!*ciphers) {
        return -1;
    }
    
    for (int i = 0; i < cipher_count; i++) {
        cipher_info_init(&(*ciphers)[i]);
        (*ciphers)[i].name = strdup_safe(cipher_data[i].name);
        (*ciphers)[i].description = strdup_safe(cipher_data[i].description);
        (*ciphers)[i].block_size = cipher_data[i].block_size;
        (*ciphers)[i].key_size_count = cipher_data[i].key_size_count;
        (*ciphers)[i].mode_count = cipher_data[i].mode_count;
        
        // Copy key sizes
        for (int j = 0; j < cipher_data[i].key_size_count && j < 8; j++) {
            (*ciphers)[i].key_sizes[j] = cipher_data[i].key_sizes[j];
        }
        
        // Copy modes
        for (int j = 0; j < cipher_data[i].mode_count && j < 8; j++) {
            (*ciphers)[i].modes[j] = strdup_safe(cipher_data[i].modes[j]);
        }
    }
    
    *count = cipher_count;
    return 0;
}

// Public interface function
int encryption_get_cipher_info(cipher_info_t** ciphers, size_t* count) {
    return encryption_get_cipher_info_internal(ciphers, count);
}