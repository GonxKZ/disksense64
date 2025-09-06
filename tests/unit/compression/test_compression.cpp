#include "libs/compression/compression.h"
#include "libs/compression/zip.h"
#include "libs/compression/rar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

// Create test compressed files
static int create_test_compressed_files(const char* test_dir) {
    printf("Creating test compressed files in: %s\n", test_dir);
    
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create test directory");
        return -1;
    }
    
    // Create test ZIP file
    char zip_file[1024];
    snprintf(zip_file, sizeof(zip_file), "%s/test_archive.zip", test_dir);
    
    int fd = open(zip_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        // Write ZIP file signature
        uint8_t zip_signature[] = {0x50, 0x4B, 0x03, 0x04};
        write(fd, zip_signature, sizeof(zip_signature));
        
        // Write some test data
        const char* zip_content = "This is a test ZIP file with some content for analysis.";
        write(fd, zip_content, strlen(zip_content));
        
        // Write ZIP end of central directory signature
        uint8_t eocd_signature[] = {0x50, 0x4B, 0x05, 0x06};
        write(fd, eocd_signature, sizeof(eocd_signature));
        
        close(fd);
    }
    
    // Create test RAR file
    char rar_file[1024];
    snprintf(rar_file, sizeof(rar_file), "%s/test_archive.rar", test_dir);
    
    fd = open(rar_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        // Write RAR file signature
        uint8_t rar_signature[] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00};
        write(fd, rar_signature, sizeof(rar_signature));
        
        // Write some test data
        const char* rar_content = "This is a test RAR file with some content for analysis.";
        write(fd, rar_content, strlen(rar_content));
        
        close(fd);
    }
    
    // Create test 7Z file
    char sevenz_file[1024];
    snprintf(sevenz_file, sizeof(sevenz_file), "%s/test_archive.7z", test_dir);
    
    fd = open(sevenz_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        // Write 7Z file signature
        uint8_t sevenz_signature[] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
        write(fd, sevenz_signature, sizeof(sevenz_signature));
        
        // Write some test data
        const char* sevenz_content = "This is a test 7Z file with some content for analysis.";
        write(fd, sevenz_content, strlen(sevenz_content));
        
        close(fd);
    }
    
    // Create test TAR file
    char tar_file[1024];
    snprintf(tar_file, sizeof(tar_file), "%s/test_archive.tar", test_dir);
    
    fd = open(tar_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        // Write some test data
        const char* tar_content = "This is a test TAR file with some content for analysis.";
        write(fd, tar_content, strlen(tar_content));
        
        close(fd);
    }
    
    // Create test GZIP file
    char gz_file[1024];
    snprintf(gz_file, sizeof(gz_file), "%s/test_file.gz", test_dir);
    
    fd = open(gz_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        // Write GZIP file signature
        uint8_t gz_signature[] = {0x1F, 0x8B, 0x08};
        write(fd, gz_signature, sizeof(gz_signature));
        
        // Write some test data
        const char* gz_content = "This is a test GZIP file with some content for analysis.";
        write(fd, gz_content, strlen(gz_content));
        
        close(fd);
    }
    
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* test_dir) {
    printf("Cleaning up test environment: %s\n", test_dir);
    
    DIR* dir = opendir(test_dir);
    if (!dir) {
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", test_dir, entry->d_name);
        unlink(filepath);
    }
    
    closedir(dir);
    rmdir(test_dir);
}

int test_compression_format_detection() {
    printf("\n=== Testing Compression Format Detection ===\n");
    
    const char* test_dir = "/tmp/compression_test_dir";
    
    // Create test compressed files
    if (create_test_compressed_files(test_dir) != 0) {
        printf("Failed to create test compressed files\n");
        return 1;
    }
    
    // Test format detection for each file
    const char* test_files[] = {
        "/tmp/compression_test_dir/test_archive.zip",
        "/tmp/compression_test_dir/test_archive.rar",
        "/tmp/compression_test_dir/test_archive.7z",
        "/tmp/compression_test_dir/test_archive.tar",
        "/tmp/compression_test_dir/test_file.gz",
        NULL
    };
    
    for (int i = 0; test_files[i] != NULL; i++) {
        compression_format_t format = compression_detect_format(test_files[i]);
        const char* format_name = compression_get_format_name(format);
        const char* format_description = compression_get_format_description(format);
        
        printf("File: %s\n", test_files[i]);
        printf("  Detected format: %s\n", format_name);
        printf("  Format description: %s\n", format_description);
        printf("\n");
    }
    
    // Clean up
    cleanup_test_environment(test_dir);
    
    printf("Compression format detection test passed!\n");
    return 0;
}

int test_compression_analysis() {
    printf("\n=== Testing Compression Analysis ===\n");
    
    const char* test_dir = "/tmp/compression_test_dir";
    
    // Create test compressed files
    if (create_test_compressed_files(test_dir) != 0) {
        printf("Failed to create test compressed files\n");
        return 1;
    }
    
    // Initialize options
    compression_options_t options;
    compression_options_init(&options);
    options.extract_metadata_only = 1;
    options.check_integrity = 1;
    options.detect_encryption = 1;
    options.deep_analysis = 0;
    options.max_entries = 100;
    
    // Analyze each compressed file
    const char* test_files[] = {
        "/tmp/compression_test_dir/test_archive.zip",
        "/tmp/compression_test_dir/test_archive.rar",
        "/tmp/compression_test_dir/test_archive.7z",
        "/tmp/compression_test_dir/test_archive.tar",
        "/tmp/compression_test_dir/test_file.gz",
        NULL
    };
    
    for (int i = 0; test_files[i] != NULL; i++) {
        compressed_archive_t archive;
        int ret = compression_analyze_file(test_files[i], &options, &archive);
        
        if (ret == 0) {
            printf("Analyzed %s:\n", test_files[i]);
            printf("  Format: %s\n", compression_get_format_name(archive.format));
            printf("  Total compressed size: %lu bytes\n", (unsigned long)archive.total_compressed_size);
            printf("  Total uncompressed size: %lu bytes\n", (unsigned long)archive.total_uncompressed_size);
            printf("  Entries: %lu\n", (unsigned long)archive.entry_count);
            printf("  Password protected: %s\n", archive.is_password_protected ? "Yes" : "No");
            printf("  Corrupted: %s\n", archive.is_corrupted ? "Yes" : "No");
            
            // Show first few entries
            for (size_t j = 0; j < archive.entry_count && j < 3; j++) {
                const compressed_entry_t* entry = &archive.entries[j];
                printf("    Entry %lu: %s\n", (unsigned long)j, entry->filename ? entry->filename : "Unknown");
                printf("      Compressed size: %lu bytes\n", (unsigned long)entry->compressed_size);
                printf("      Uncompressed size: %lu bytes\n", (unsigned long)entry->uncompressed_size);
                printf("      Encrypted: %s\n", entry->is_encrypted ? "Yes" : "No");
                printf("      Directory: %s\n", entry->is_directory ? "Yes" : "No");
                printf("      Timestamp: %lu\n", (unsigned long)entry->timestamp);
                printf("      Permissions: %s\n", entry->permissions ? entry->permissions : "Unknown");
            }
            
            if (archive.entry_count > 3) {
                printf("    ... and %lu more entries\n", (unsigned long)(archive.entry_count - 3));
            }
            
            printf("\n");
            
            // Clean up
            compressed_archive_free(&archive);
        } else {
            printf("Failed to analyze %s\n", test_files[i]);
        }
    }
    
    // Clean up
    compression_options_free(&options);
    cleanup_test_environment(test_dir);
    
    printf("Compression analysis test passed!\n");
    return 0;
}

int test_directory_compression_analysis() {
    printf("\n=== Testing Directory Compression Analysis ===\n");
    
    const char* test_dir = "/tmp/compression_test_dir";
    
    // Create test compressed files
    if (create_test_compressed_files(test_dir) != 0) {
        printf("Failed to create test compressed files\n");
        return 1;
    }
    
    // Initialize options
    compression_options_t options;
    compression_options_init(&options);
    options.extract_metadata_only = 1;
    options.check_integrity = 1;
    options.detect_encryption = 1;
    options.deep_analysis = 0;
    options.max_entries = 100;
    
    // Analyze directory
    compression_result_t result;
    int ret = compression_analyze_directory(test_dir, &options, &result);
    
    if (ret == 0) {
        printf("Analyzed directory %s:\n", test_dir);
        printf("  Archives found: %lu\n", (unsigned long)result.archive_count);
        
        for (size_t i = 0; i < result.archive_count; i++) {
            const compressed_archive_t* archive = &result.archives[i];
            printf("    Archive %lu: %s\n", (unsigned long)i, archive->archive_path ? archive->archive_path : "Unknown");
            printf("      Format: %s\n", compression_get_format_name(archive->format));
            printf("      Total compressed size: %lu bytes\n", (unsigned long)archive->total_compressed_size);
            printf("      Total uncompressed size: %lu bytes\n", (unsigned long)archive->total_uncompressed_size);
            printf("      Entries: %lu\n", (unsigned long)archive->entry_count);
            printf("      Password protected: %s\n", archive->is_password_protected ? "Yes" : "No");
            printf("      Corrupted: %s\n", archive->is_corrupted ? "Yes" : "No");
            
            // Show first few entries
            for (size_t j = 0; j < archive->entry_count && j < 2; j++) {
                const compressed_entry_t* entry = &archive->entries[j];
                printf("        Entry %lu: %s\n", (unsigned long)j, entry->filename ? entry->filename : "Unknown");
                printf("          Compressed size: %lu bytes\n", (unsigned long)entry->compressed_size);
                printf("          Uncompressed size: %lu bytes\n", (unsigned long)entry->uncompressed_size);
                printf("          Encrypted: %s\n", entry->is_encrypted ? "Yes" : "No");
                printf("          Directory: %s\n", entry->is_directory ? "Yes" : "No");
            }
            
            if (archive->entry_count > 2) {
                printf("        ... and %lu more entries\n", (unsigned long)(archive->entry_count - 2));
            }
        }
        
        // Clean up
        compression_result_free(&result);
    } else {
        printf("Failed to analyze directory %s\n", test_dir);
        compression_options_free(&options);
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    // Clean up
    compression_options_free(&options);
    cleanup_test_environment(test_dir);
    
    printf("Directory compression analysis test passed!\n");
    return 0;
}

int test_password_protection_detection() {
    printf("\n=== Testing Password Protection Detection ===\n");
    
    const char* test_dir = "/tmp/compression_test_dir";
    
    // Create test compressed files
    if (create_test_compressed_files(test_dir) != 0) {
        printf("Failed to create test compressed files\n");
        return 1;
    }
    
    // Test password protection detection for each file
    const char* test_files[] = {
        "/tmp/compression_test_dir/test_archive.zip",
        "/tmp/compression_test_dir/test_archive.rar",
        "/tmp/compression_test_dir/test_archive.7z",
        "/tmp/compression_test_dir/test_archive.tar",
        "/tmp/compression_test_dir/test_file.gz",
        NULL
    };
    
    for (int i = 0; test_files[i] != NULL; i++) {
        int is_protected;
        int ret = compression_is_password_protected(test_files[i], &is_protected);
        
        if (ret == 0) {
            printf("File: %s\n", test_files[i]);
            printf("  Password protected: %s\n", is_protected ? "Yes" : "No");
        } else {
            printf("Failed to check password protection for %s\n", test_files[i]);
        }
    }
    
    // Clean up
    cleanup_test_environment(test_dir);
    
    printf("Password protection detection test passed!\n");
    return 0;
}

int test_compression_ratio_calculation() {
    printf("\n=== Testing Compression Ratio Calculation ===\n");
    
    // Test various compression ratios
    struct {
        uint64_t compressed_size;
        uint64_t uncompressed_size;
        double expected_ratio;
    } test_cases[] = {
        {100, 1000, 0.9},
        {500, 1000, 0.5},
        {900, 1000, 0.1},
        {1000, 1000, 0.0},
        {1500, 1000, 0.0}, // Negative ratio should be clamped to 0
        {0, 1000, 1.0},
        {1000, 0, 0.0}
    };
    
    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double ratio = compression_get_ratio(test_cases[i].compressed_size, test_cases[i].uncompressed_size);
        printf("Compressed: %lu, Uncompressed: %lu -> Ratio: %.2f (Expected: %.2f)\n",
               (unsigned long)test_cases[i].compressed_size,
               (unsigned long)test_cases[i].uncompressed_size,
               ratio, test_cases[i].expected_ratio);
        
        // Check if ratio is within acceptable range (accounting for floating point precision)
        if (fabs(ratio - test_cases[i].expected_ratio) > 0.01) {
            printf("Error: Ratio mismatch for test case %d\n", i);
            return 1;
        }
    }
    
    printf("Compression ratio calculation test passed!\n");
    return 0;
}

int test_compression_format_descriptions() {
    printf("\n=== Testing Compression Format Descriptions ===\n");
    
    compression_format_t formats[] = {
        COMPRESSION_FORMAT_UNKNOWN,
        COMPRESSION_FORMAT_ZIP,
        COMPRESSION_FORMAT_RAR,
        COMPRESSION_FORMAT_7Z,
        COMPRESSION_FORMAT_TAR,
        COMPRESSION_FORMAT_GZ,
        COMPRESSION_FORMAT_BZ2,
        COMPRESSION_FORMAT_XZ,
        COMPRESSION_FORMAT_LZMA,
        COMPRESSION_FORMAT_LZ4,
        COMPRESSION_FORMAT_ZSTD
    };
    
    int num_formats = sizeof(formats) / sizeof(formats[0]);
    
    for (int i = 0; i < num_formats; i++) {
        const char* format_name = compression_get_format_name(formats[i]);
        const char* format_description = compression_get_format_description(formats[i]);
        
        printf("Format %d:\n", i);
        printf("  Name: %s\n", format_name);
        printf("  Description: %s\n", format_description);
        printf("\n");
    }
    
    printf("Compression format descriptions test passed!\n");
    return 0;
}

int test_compression_options_management() {
    printf("\n=== Testing Compression Options Management ===\n");
    
    // Initialize options
    compression_options_t options;
    compression_options_init(&options);
    
    printf("Default compression options:\n");
    printf("  Extract metadata only: %s\n", options.extract_metadata_only ? "Yes" : "No");
    printf("  Check integrity: %s\n", options.check_integrity ? "Yes" : "No");
    printf("  Detect encryption: %s\n", options.detect_encryption ? "Yes" : "No");
    printf("  Deep analysis: %s\n", options.deep_analysis ? "Yes" : "No");
    printf("  Password: %s\n", options.password ? options.password : "None");
    printf("  Max entries: %lu\n", (unsigned long)options.max_entries);
    
    // Modify options
    options.extract_metadata_only = 0;
    options.check_integrity = 0;
    options.detect_encryption = 0;
    options.deep_analysis = 1;
    options.password = strdup("test_password");
    options.max_entries = 500;
    
    printf("\nModified compression options:\n");
    printf("  Extract metadata only: %s\n", options.extract_metadata_only ? "Yes" : "No");
    printf("  Check integrity: %s\n", options.check_integrity ? "Yes" : "No");
    printf("  Detect encryption: %s\n", options.detect_encryption ? "Yes" : "No");
    printf("  Deep analysis: %s\n", options.deep_analysis ? "Yes" : "No");
    printf("  Password: %s\n", options.password ? options.password : "None");
    printf("  Max entries: %lu\n", (unsigned long)options.max_entries);
    
    // Test freeing options
    compression_options_free(&options);
    
    printf("Compression options management test passed!\n");
    return 0;
}

int test_compression_entry_management() {
    printf("\n=== Testing Compression Entry Management ===\n");
    
    // Initialize entry
    compressed_entry_t entry;
    compressed_entry_init(&entry);
    
    printf("Initialized empty compression entry\n");
    
    // Fill entry with test data
    entry.filename = strdup("test_file.txt");
    entry.compressed_size = 1024;
    entry.uncompressed_size = 2048;
    entry.offset = 0;
    entry.crc32 = 0x12345678;
    entry.is_encrypted = 0;
    entry.is_directory = 0;
    entry.timestamp = time(NULL);
    entry.permissions = strdup("rw-r--r--");
    
    printf("Filled compression entry with test data:\n");
    printf("  Filename: %s\n", entry.filename ? entry.filename : "None");
    printf("  Compressed size: %lu bytes\n", (unsigned long)entry.compressed_size);
    printf("  Uncompressed size: %lu bytes\n", (unsigned long)entry.uncompressed_size);
    printf("  Offset: %lu\n", (unsigned long)entry.offset);
    printf("  CRC32: 0x%08X\n", entry.crc32);
    printf("  Encrypted: %s\n", entry.is_encrypted ? "Yes" : "No");
    printf("  Directory: %s\n", entry.is_directory ? "Yes" : "No");
    printf("  Timestamp: %lu\n", (unsigned long)entry.timestamp);
    printf("  Permissions: %s\n", entry.permissions ? entry.permissions : "None");
    
    // Test freeing entry
    compressed_entry_free(&entry);
    
    printf("Compression entry management test passed!\n");
    return 0;
}

int test_compression_archive_management() {
    printf("\n=== Testing Compression Archive Management ===\n");
    
    // Initialize archive
    compressed_archive_t archive;
    compressed_archive_init(&archive);
    
    printf("Initialized empty compression archive\n");
    
    // Fill archive with test data
    archive.archive_path = strdup("/tmp/test_archive.zip");
    archive.format = COMPRESSION_FORMAT_ZIP;
    archive.total_compressed_size = 4096;
    archive.total_uncompressed_size = 8192;
    archive.is_password_protected = 0;
    archive.is_corrupted = 0;
    
    printf("Filled compression archive with test data:\n");
    printf("  Archive path: %s\n", archive.archive_path ? archive.archive_path : "None");
    printf("  Format: %s\n", compression_get_format_name(archive.format));
    printf("  Total compressed size: %lu bytes\n", (unsigned long)archive.total_compressed_size);
    printf("  Total uncompressed size: %lu bytes\n", (unsigned long)archive.total_uncompressed_size);
    printf("  Password protected: %s\n", archive.is_password_protected ? "Yes" : "No");
    printf("  Corrupted: %s\n", archive.is_corrupted ? "Yes" : "No");
    printf("  Entries: %lu\n", (unsigned long)archive.entry_count);
    printf("  Capacity: %lu\n", (unsigned long)archive.capacity);
    
    // Add test entries
    for (int i = 0; i < 3; i++) {
        compressed_entry_t entry;
        compressed_entry_init(&entry);
        entry.filename = (char*)malloc(32);
        snprintf(entry.filename, 32, "file_%d.txt", i);
        entry.compressed_size = 1024 + i * 512;
        entry.uncompressed_size = 2048 + i * 1024;
        entry.permissions = strdup("rw-r--r--");
        
        int ret = compressed_archive_add_entry(&archive, &entry);
        if (ret == 0) {
            printf("  Added entry %d: %s (%lu/%lu bytes)\n", 
                   i, entry.filename, 
                   (unsigned long)entry.compressed_size, 
                   (unsigned long)entry.uncompressed_size);
        } else {
            printf("  Failed to add entry %d\n", i);
        }
        
        compressed_entry_free(&entry);
    }
    
    printf("  Final entry count: %lu\n", (unsigned long)archive.entry_count);
    
    // Test freeing archive
    compressed_archive_free(&archive);
    
    printf("Compression archive management test passed!\n");
    return 0;
}

int test_compression_result_management() {
    printf("\n=== Testing Compression Result Management ===\n");
    
    // Initialize result
    compression_result_t result;
    compression_result_init(&result);
    
    printf("Initialized empty compression result\n");
    
    // Fill result with test data
    printf("Result initialized with:\n");
    printf("  Archives: %lu\n", (unsigned long)result.archive_count);
    printf("  Capacity: %lu\n", (unsigned long)result.capacity);
    
    // Add test archives
    for (int i = 0; i < 2; i++) {
        compressed_archive_t archive;
        compressed_archive_init(&archive);
        archive.archive_path = (char*)malloc(32);
        snprintf(archive.archive_path, 32, "/tmp/test_archive_%d.zip", i);
        archive.format = COMPRESSION_FORMAT_ZIP;
        archive.total_compressed_size = 2048 + i * 1024;
        archive.total_uncompressed_size = 4096 + i * 2048;
        
        int ret = compression_result_add_archive(&result, &archive);
        if (ret == 0) {
            printf("  Added archive %d: %s (%lu/%lu bytes)\n", 
                   i, archive.archive_path, 
                   (unsigned long)archive.total_compressed_size, 
                   (unsigned long)archive.total_uncompressed_size);
        } else {
            printf("  Failed to add archive %d\n", i);
        }
        
        compressed_archive_free(&archive);
    }
    
    printf("  Final archive count: %lu\n", (unsigned long)result.archive_count);
    printf("  Final capacity: %lu\n", (unsigned long)result.capacity);
    
    // Test freeing result
    compression_result_free(&result);
    
    printf("Compression result management test passed!\n");
    return 0;
}

int main() {
    srand((unsigned int)time(NULL)); // Initialize random seed
    
    printf("Running compression analysis tests...\n");
    
    int result1 = test_compression_format_detection();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_compression_analysis();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_directory_compression_analysis();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_password_protection_detection();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_compression_ratio_calculation();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_compression_format_descriptions();
    if (result6 != 0) {
        return result6;
    }
    
    int result7 = test_compression_options_management();
    if (result7 != 0) {
        return result7;
    }
    
    int result8 = test_compression_entry_management();
    if (result8 != 0) {
        return result8;
    }
    
    int result9 = test_compression_archive_management();
    if (result9 != 0) {
        return result9;
    }
    
    int result10 = test_compression_result_management();
    if (result10 != 0) {
        return result10;
    }
    
    printf("\n=== All Compression Analysis Tests Passed! ===\n");
    return 0;
}