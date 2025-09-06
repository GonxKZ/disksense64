#include "libs/carving/carving.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// Create test data with embedded files
static uint8_t* create_test_data(size_t* size) {
    // Create a buffer with some "raw disk" data containing embedded files
    *size = 1024 * 1024; // 1MB
    uint8_t* data = (uint8_t*)calloc(1, *size);
    if (!data) {
        return NULL;
    }
    
    // Add some JPEG data at offset 1000
    uint8_t jpeg_header[] = {0xFF, 0xD8, 0xFF};
    uint8_t jpeg_footer[] = {0xFF, 0xD9};
    
    memcpy(data + 1000, jpeg_header, sizeof(jpeg_header));
    // Add some JPEG-like data in between
    memset(data + 1003, 0x55, 5000);
    memcpy(data + 6003, jpeg_footer, sizeof(jpeg_footer));
    
    // Add some PNG data at offset 10000
    uint8_t png_header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    uint8_t png_footer[] = {0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};
    
    memcpy(data + 10000, png_header, sizeof(png_header));
    // Add some PNG-like data in between
    memset(data + 10008, 0xAA, 8000);
    memcpy(data + 18008, png_footer, sizeof(png_footer));
    
    // Add some PDF data at offset 20000
    uint8_t pdf_header[] = {0x25, 0x50, 0x44, 0x46}; // %PDF
    uint8_t pdf_footer[] = {0x25, 0x25, 0x45, 0x4F, 0x46}; // %%EOF
    
    memcpy(data + 20000, pdf_header, sizeof(pdf_header));
    // Add some PDF-like data in between
    memset(data + 20004, 0xCC, 12000);
    memcpy(data + 32004, pdf_footer, sizeof(pdf_footer));
    
    return data;
}

// Create a test file with the test data
static int create_test_file(const char* path, const uint8_t* data, size_t size) {
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    ssize_t written = write(fd, data, size);
    close(fd);
    
    return (written == (ssize_t)size) ? 0 : -1;
}

int test_carving_data() {
    printf("Testing file carving from data...\n");
    
    // Create test data
    size_t data_size;
    uint8_t* test_data = create_test_data(&data_size);
    if (!test_data) {
        printf("Failed to create test data\n");
        return 1;
    }
    
    // Carve files from data
    carving_result_t result;
    int ret = carving_carve_data(test_data, data_size, &result);
    
    if (ret != 0) {
        printf("Failed to carve files from data\n");
        free(test_data);
        return 1;
    }
    
    printf("Found %lu carved files:\n", (unsigned long)result.count);
    
    for (size_t i = 0; i < result.count; i++) {
        carved_file_t* file = &result.files[i];
        printf("  File %lu: offset=%lu, size=%lu, extension=%s\n",
               (unsigned long)i, (unsigned long)file->offset, 
               (unsigned long)file->size, file->extension ? file->extension : "unknown");
    }
    
    // Test saving files
    const char* output_dir = "/tmp/carving_test_output";
    ret = carving_save_files(&result, output_dir);
    if (ret != 0) {
        printf("Failed to save carved files\n");
        carving_result_free(&result);
        free(test_data);
        return 1;
    }
    
    printf("Saved carved files to %s\n", output_dir);
    
    // Clean up
    carving_result_free(&result);
    free(test_data);
    
    printf("File carving from data test passed!\n");
    return 0;
}

int test_carving_file() {
    printf("Testing file carving from file...\n");
    
    const char* test_file = "/tmp/carving_test_file.dat";
    
    // Create test data
    size_t data_size;
    uint8_t* test_data = create_test_data(&data_size);
    if (!test_data) {
        printf("Failed to create test data\n");
        return 1;
    }
    
    // Create test file
    if (create_test_file(test_file, test_data, data_size) != 0) {
        printf("Failed to create test file\n");
        free(test_data);
        return 1;
    }
    
    // Carve files from file
    carving_result_t result;
    int ret = carving_carve_file(test_file, &result);
    
    if (ret != 0) {
        printf("Failed to carve files from file\n");
        free(test_data);
        unlink(test_file);
        return 1;
    }
    
    printf("Found %lu carved files:\n", (unsigned long)result.count);
    
    for (size_t i = 0; i < result.count; i++) {
        carved_file_t* file = &result.files[i];
        printf("  File %lu: offset=%lu, size=%lu, extension=%s\n",
               (unsigned long)i, (unsigned long)file->offset, 
               (unsigned long)file->size, file->extension ? file->extension : "unknown");
    }
    
    // Clean up
    carving_result_free(&result);
    free(test_data);
    unlink(test_file);
    
    printf("File carving from file test passed!\n");
    return 0;
}

int test_signatures() {
    printf("Testing file signatures...\n");
    
    size_t count;
    const file_signature_t* signatures = carving_get_signatures(&count);
    
    if (!signatures || count == 0) {
        printf("Failed to get file signatures\n");
        return 1;
    }
    
    printf("Found %lu file signatures:\n", (unsigned long)count);
    
    for (size_t i = 0; i < count && i < 10; i++) { // Show first 10
        const file_signature_t* sig = &signatures[i];
        printf("  Signature %lu: %s (%lu header bytes, %lu footer bytes)\n",
               (unsigned long)i, sig->extension ? sig->extension : "unknown",
               (unsigned long)sig->header_signature_length,
               (unsigned long)sig->footer_signature_length);
    }
    
    if (count > 10) {
        printf("  ... and %lu more signatures\n", (unsigned long)(count - 10));
    }
    
    printf("File signatures test passed!\n");
    return 0;
}

int main() {
    printf("Running file carving tests...\n");
    
    int result1 = test_signatures();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_carving_data();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_carving_file();
    if (result3 != 0) {
        return result3;
    }
    
    printf("All file carving tests passed!\n");
    return 0;
}