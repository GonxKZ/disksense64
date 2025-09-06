#include "libs/hidden/hidden.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <cerrno>

// Create test files with various characteristics
static int create_test_environment(const char* test_dir) {
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    // Create normal file
    char normal_file[1024];
    snprintf(normal_file, sizeof(normal_file), "%s/normal_file.txt", test_dir);
    int fd = open(normal_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "Normal file content", 19);
        close(fd);
    }
    
    // Create hidden file
    char hidden_file[1024];
    snprintf(hidden_file, sizeof(hidden_file), "%s/.hidden_file.txt", test_dir);
    fd = open(hidden_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "Hidden file content", 19);
        close(fd);
    }
    
    // Create suspiciously named file
    char suspicious_file[1024];
    snprintf(suspicious_file, sizeof(suspicious_file), "%s/..file.txt", test_dir);
    fd = open(suspicious_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "Suspicious file content", 23);
        close(fd);
    }
    
    // Create backup file
    char backup_file[1024];
    snprintf(backup_file, sizeof(backup_file), "%s/file.bak", test_dir);
    fd = open(backup_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "Backup file content", 19);
        close(fd);
    }
    
    // Create world-writable file (suspicious permissions)
    char writable_file[1024];
    snprintf(writable_file, sizeof(writable_file), "%s/writable_file.txt", test_dir);
    fd = open(writable_file, O_CREAT | O_WRONLY, 0666);
    if (fd >= 0) {
        write(fd, "World-writable file content", 27);
        close(fd);
    }
    
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* test_dir) {
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

int test_hidden_filename_detection() {
    printf("Testing hidden filename detection...\n");
    
    // Test various filenames
    const char* test_cases[][2] = {
        {".hidden", "1"},           // Hidden file
        {"normal.txt", "0"},        // Normal file
        {"..hidden", "1"},          // Suspicious (directory traversal)
        {"file~", "1"},             // Suspicious (backup)
        {"file.tmp", "1"},          // Suspicious (temporary)
        {"file.bak", "1"},          // Suspicious (backup)
        {"file.swp", "1"},          // Suspicious (vim swap)
        {"123.456", "1"},           // Suspicious (dots and numbers)
        {"a", "0"},                 // Normal
        {"very_long_filename_that_exceeds_the_typical_limit_and_might_be_suspicious.txt", "1"}, // Suspicious (too long)
        {NULL, NULL}
    };
    
    for (int i = 0; test_cases[i][0] != NULL; i++) {
        int expected = atoi(test_cases[i][1]);
        int result = hidden_is_suspicious_filename(test_cases[i][0]);
        
        if (result != expected) {
            printf("Test failed for '%s': expected %d, got %d\n", 
                   test_cases[i][0], expected, result);
            return 1;
        }
    }
    
    printf("Hidden filename detection test passed!\n");
    return 0;
}

int test_hidden_file_detection() {
    printf("Testing hidden file detection...\n");
    
    const char* test_dir = "/tmp/hidden_test_dir";
    
    // Create test environment
    if (create_test_environment(test_dir) != 0) {
        printf("Failed to create test environment\n");
        return 1;
    }
    
    // Detect hidden files
    hidden_detection_result_t result;
    int ret = hidden_detect_files(test_dir, &result);
    
    if (ret != 0) {
        printf("Failed to detect hidden files\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("Found %lu suspicious files:\n", (unsigned long)result.count);
    
    for (size_t i = 0; i < result.count; i++) {
        hidden_file_result_t* file = &result.files[i];
        printf("  File: %s\n", file->path ? file->path : "unknown");
        printf("    Is hidden: %s\n", file->is_hidden ? "yes" : "no");
        printf("    Has suspicious name: %s\n", file->has_suspicious_name ? "yes" : "no");
        printf("    Has suspicious permissions: %s\n", file->has_suspicious_permissions ? "yes" : "no");
        printf("    Reason: %s\n", file->reason ? file->reason : "none");
    }
    
    // Clean up
    hidden_detection_result_free(&result);
    cleanup_test_environment(test_dir);
    
    printf("Hidden file detection test passed!\n");
    return 0;
}

int test_rootkit_detection() {
    printf("Testing rootkit detection...\n");
    
    // Detect rootkits
    rootkit_detection_result_t result;
    int ret = hidden_detect_rootkits(&result);
    
    if (ret != 0) {
        printf("Failed to detect rootkits\n");
        return 1;
    }
    
    printf("Known rootkits in database: %lu\n", (unsigned long)result.count);
    
    // Show first 5 rootkits
    for (size_t i = 0; i < result.count && i < 5; i++) {
        rootkit_result_t* rk = &result.rootkits[i];
        printf("  Rootkit: %s\n", rk->name ? rk->name : "unknown");
        printf("    Description: %s\n", rk->description ? rk->description : "none");
        printf("    Detection method: %s\n", rk->detection_method ? rk->detection_method : "none");
        printf("    Detected: %s\n", rk->detected ? "yes" : "no");
    }
    
    if (result.count > 5) {
        printf("  ... and %lu more rootkits\n", (unsigned long)(result.count - 5));
    }
    
    // Clean up
    rootkit_detection_result_free(&result);
    
    printf("Rootkit detection test passed!\n");
    return 0;
}

int main() {
    printf("Running hidden files and rootkits detection tests...\n");
    
    int result1 = test_hidden_filename_detection();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_hidden_file_detection();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_rootkit_detection();
    if (result3 != 0) {
        return result3;
    }
    
    printf("All hidden files and rootkits detection tests passed!\n");
    return 0;
}
