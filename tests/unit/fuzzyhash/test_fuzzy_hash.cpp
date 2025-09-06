#include "libs/fuzzyhash/fuzzy_hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test data
static const char* test_data1 = "This is a test file for fuzzy hashing. It contains some sample text that we can use to test our implementation.";
static const char* test_data2 = "This is a test file for fuzzy hashing. It contains some sample text that we can use to test our implementation with minor changes.";
static const char* test_data3 = "Completely different content that should not be similar to the other test data.";

int test_fuzzy_hash_ssdeep_basic() {
    fuzzy_hash_result_t hash1, hash2, hash3;
    
    // Test hashing data
    int ret1 = fuzzy_hash_data(test_data1, strlen(test_data1), FUZZY_HASH_SSDEEP, &hash1);
    int ret2 = fuzzy_hash_data(test_data2, strlen(test_data2), FUZZY_HASH_SSDEEP, &hash2);
    int ret3 = fuzzy_hash_data(test_data3, strlen(test_data3), FUZZY_HASH_SSDEEP, &hash3);
    
    if (ret1 != 0 || ret2 != 0 || ret3 != 0) {
        printf("SSDeep hash computation failed\n");
        return 1;
    }
    
    // Test comparison
    int score12, score13;
    int comp12 = fuzzy_hash_compare(&hash1, &hash2, &score12);
    int comp13 = fuzzy_hash_compare(&hash1, &hash3, &score13);
    
    if (comp12 != 0 || comp13 != 0) {
        printf("SSDeep hash comparison failed\n");
        fuzzy_hash_free(&hash1);
        fuzzy_hash_free(&hash2);
        fuzzy_hash_free(&hash3);
        return 1;
    }
    
    printf("SSDeep similarity scores: %d (similar texts), %d (different texts)\n", score12, score13);
    
    // Test similarity check
    int similar12 = fuzzy_hash_is_similar(&hash1, &hash2, 30);
    int similar13 = fuzzy_hash_is_similar(&hash1, &hash3, 30);
    
    if (similar12 < 0 || similar13 < 0) {
        printf("SSDeep similarity check failed\n");
        fuzzy_hash_free(&hash1);
        fuzzy_hash_free(&hash2);
        fuzzy_hash_free(&hash3);
        return 1;
    }
    
    printf("SSDeep similarity check: %s (similar texts), %s (different texts)\n", 
           similar12 ? "similar" : "not similar", 
           similar13 ? "similar" : "not similar");
    
    // Clean up
    fuzzy_hash_free(&hash1);
    fuzzy_hash_free(&hash2);
    fuzzy_hash_free(&hash3);
    
    printf("SSDeep basic tests passed!\n");
    return 0;
}

int test_fuzzy_hash_tlsh_basic() {
    fuzzy_hash_result_t hash1, hash2, hash3;
    
    // Test hashing data
    int ret1 = fuzzy_hash_data(test_data1, strlen(test_data1), FUZZY_HASH_TLSH, &hash1);
    int ret2 = fuzzy_hash_data(test_data2, strlen(test_data2), FUZZY_HASH_TLSH, &hash2);
    int ret3 = fuzzy_hash_data(test_data3, strlen(test_data3), FUZZY_HASH_TLSH, &hash3);
    
    if (ret1 != 0 || ret2 != 0 || ret3 != 0) {
        printf("TLSH hash computation failed\n");
        return 1;
    }
    
    // Test comparison
    int score12, score13;
    int comp12 = fuzzy_hash_compare(&hash1, &hash2, &score12);
    int comp13 = fuzzy_hash_compare(&hash1, &hash3, &score13);
    
    if (comp12 != 0 || comp13 != 0) {
        printf("TLSH hash comparison failed\n");
        fuzzy_hash_free(&hash1);
        fuzzy_hash_free(&hash2);
        fuzzy_hash_free(&hash3);
        return 1;
    }
    
    printf("TLSH similarity scores: %d (similar texts), %d (different texts)\n", score12, score13);
    
    // Test similarity check
    int similar12 = fuzzy_hash_is_similar(&hash1, &hash2, 30);
    int similar13 = fuzzy_hash_is_similar(&hash1, &hash3, 30);
    
    if (similar12 < 0 || similar13 < 0) {
        printf("TLSH similarity check failed\n");
        fuzzy_hash_free(&hash1);
        fuzzy_hash_free(&hash2);
        fuzzy_hash_free(&hash3);
        return 1;
    }
    
    printf("TLSH similarity check: %s (similar texts), %s (different texts)\n", 
           similar12 ? "similar" : "not similar", 
           similar13 ? "similar" : "not similar");
    
    // Clean up
    fuzzy_hash_free(&hash1);
    fuzzy_hash_free(&hash2);
    fuzzy_hash_free(&hash3);
    
    printf("TLSH basic tests passed!\n");
    return 0;
}

int main() {
    printf("Running fuzzy hashing tests...\n");
    
    int result1 = test_fuzzy_hash_ssdeep_basic();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_fuzzy_hash_tlsh_basic();
    if (result2 != 0) {
        return result2;
    }
    
    printf("All fuzzy hashing tests passed!\n");
    return 0;
}