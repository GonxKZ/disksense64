#include "../../libs/chash/sha256.h"
#include <stdio.h>
#include <string.h>

// Test vectors from NIST
static const char* test_input1 = "abc";
static const char* expected_hash1 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

static const char* test_input2 = "";
static const char* expected_hash2 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

static const char* test_input3 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const char* expected_hash3 = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

int test_sha256_basic() {
    SHA256_CTX ctx;
    uint8_t hash[SHA256_BLOCK_SIZE];
    char hex_hash[SHA256_BLOCK_SIZE * 2 + 1];
    
    // Test 1: "abc"
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)test_input1, strlen(test_input1));
    sha256_final(&ctx, hash);
    
    // Convert to hex string
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(hex_hash + i * 2, "%02x", hash[i]);
    }
    hex_hash[SHA256_BLOCK_SIZE * 2] = '\0';
    
    if (strcmp(hex_hash, expected_hash1) != 0) {
        printf("Test 1 failed: expected %s, got %s\n", expected_hash1, hex_hash);
        return 1;
    }
    
    // Test 2: ""
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)test_input2, strlen(test_input2));
    sha256_final(&ctx, hash);
    
    // Convert to hex string
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(hex_hash + i * 2, "%02x", hash[i]);
    }
    hex_hash[SHA256_BLOCK_SIZE * 2] = '\0';
    
    if (strcmp(hex_hash, expected_hash2) != 0) {
        printf("Test 2 failed: expected %s, got %s\n", expected_hash2, hex_hash);
        return 1;
    }
    
    // Test 3: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)test_input3, strlen(test_input3));
    sha256_final(&ctx, hash);
    
    // Convert to hex string
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(hex_hash + i * 2, "%02x", hash[i]);
    }
    hex_hash[SHA256_BLOCK_SIZE * 2] = '\0';
    
    if (strcmp(hex_hash, expected_hash3) != 0) {
        printf("Test 3 failed: expected %s, got %s\n", expected_hash3, hex_hash);
        return 1;
    }
    
    printf("All SHA256 tests passed!\n");
    return 0;
}

int main() {
    return test_sha256_basic();
}