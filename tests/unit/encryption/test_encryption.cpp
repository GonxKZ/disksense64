#include "libs/encryption/encryption.h"
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void create_test_file(const char* path, const char* content, size_t size) {
    FILE* f = fopen(path, "wb");
    assert(f);
    fwrite(content, 1, size, f);
    fclose(f);
}

void test_encryption_analysis() {
    printf("--- Testing Encryption Analysis ---\n");

    // Create test files
    create_test_file("test_plaintext.txt", "This is a test file.", 20);

    unsigned char random_data[1024];
    for (int i = 0; i < 1024; ++i) random_data[i] = rand() % 256;
    create_test_file("test_random.bin", (char*)random_data, 1024);

    create_test_file("test_pgp.pgp", "-----BEGIN PGP MESSAGE-----", 27);
    create_test_file("test_tdef.tdef", "TDEF", 4);

    // Test plaintext file
    encryption_result_t result_plaintext;
    encryption_analyze_file("test_plaintext.txt", NULL, &result_plaintext);
    printf("Analyzing plaintext file...\n");
    assert(!result_plaintext.is_encrypted);
    assert(result_plaintext.entropy < 0.5);
    encryption_result_free(&result_plaintext);

    // Test random file
    encryption_result_t result_random;
    encryption_analyze_file("test_random.bin", NULL, &result_random);
    printf("Analyzing random file...\n");
    assert(result_random.is_encrypted);
    assert(result_random.entropy > 0.9);
    encryption_result_free(&result_random);

    // Test PGP file
    encryption_result_t result_pgp;
    encryption_analyze_file("test_pgp.pgp", NULL, &result_pgp);
    printf("Analyzing PGP file...\n");
    assert(result_pgp.is_encrypted);
    assert(strcmp(result_pgp.encryption_algorithm, "PGP (ASCII-Armored)") == 0);
    encryption_result_free(&result_pgp);

    // Test TDEF file
    encryption_result_t result_tdef;
    encryption_analyze_file("test_tdef.tdef", NULL, &result_tdef);
    printf("Analyzing TDEF file...\n");
    assert(result_tdef.is_encrypted);
    assert(strcmp(result_tdef.encryption_algorithm, "Telegram Desktop Encrypted File") == 0);
    encryption_result_free(&result_tdef);

    printf("Encryption analysis tests passed!\n");

    // Cleanup
    unlink("test_plaintext.txt");
    unlink("test_random.bin");
    unlink("test_pgp.pgp");
    unlink("test_tdef.tdef");
}

int main() {
    test_encryption_analysis();
    return 0;
}
