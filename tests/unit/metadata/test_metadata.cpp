#include "libs/metadata/metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// Create a test file
static int create_test_file(const char* path) {
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        return -1;
    }
    
    const char* test_content = "This is a test file for metadata analysis.";
    ssize_t written = write(fd, test_content, strlen(test_content));
    close(fd);
    
    return (written == (ssize_t)strlen(test_content)) ? 0 : -1;
}

// Create a test directory
static int create_test_directory(const char* path) {
    return mkdir(path, 0755);
}

int test_metadata_file_info() {
    const char* test_file = "/tmp/test_metadata_file.txt";
    
    // Create test file
    if (create_test_file(test_file) != 0) {
        printf("Failed to create test file\n");
        return 1;
    }
    
    // Test metadata extraction
    metadata_info_t info;
    int ret = metadata_get_file_info(test_file, &info);
    
    if (ret != 0) {
        printf("Failed to get file metadata\n");
        unlink(test_file);
        return 1;
    }
    
    printf("File metadata:\n");
    printf("  Name: %s\n", info.name ? info.name : "N/A");
    printf("  Full path: %s\n", info.full_path ? info.full_path : "N/A");
    printf("  Size: %lu bytes\n", (unsigned long)info.size);
    printf("  Is directory: %s\n", info.is_directory ? "yes" : "no");
    printf("  Is symlink: %s\n", info.is_symlink ? "yes" : "no");
    printf("  Is hidden: %s\n", info.is_hidden ? "yes" : "no");
    printf("  Owner ID: %u\n", info.owner_id);
    printf("  Group ID: %u\n", info.group_id);
    printf("  Owner name: %s\n", info.owner_name ? info.owner_name : "N/A");
    printf("  Group name: %s\n", info.group_name ? info.group_name : "N/A");
    
    // Test permission string
    char perm_str[12];
    if (metadata_get_permission_string(&info, perm_str, sizeof(perm_str)) == 0) {
        printf("  Permissions: %s\n", perm_str);
    }
    
    printf("  File type: %s\n", metadata_get_file_type_string(&info));
    
    // Test suspicious permissions
    int suspicious = metadata_has_suspicious_permissions(&info);
    printf("  Suspicious permissions: %s\n", suspicious > 0 ? "yes" : "no");
    
    // Clean up
    metadata_free_info(&info);
    unlink(test_file);
    
    printf("File metadata test passed!\n");
    return 0;
}

int test_metadata_directory_info() {
    const char* test_dir = "/tmp/test_metadata_dir";
    const char* test_file1 = "/tmp/test_metadata_dir/file1.txt";
    const char* test_file2 = "/tmp/test_metadata_dir/file2.txt";
    
    // Create test directory
    if (create_test_directory(test_dir) != 0) {
        printf("Failed to create test directory\n");
        return 1;
    }
    
    // Create test files
    if (create_test_file(test_file1) != 0 || create_test_file(test_file2) != 0) {
        printf("Failed to create test files\n");
        rmdir(test_dir);
        return 1;
    }
    
    // Test directory metadata extraction
    metadata_result_t result;
    int ret = metadata_get_directory_info(test_dir, &result);
    
    if (ret != 0) {
        printf("Failed to get directory metadata\n");
        unlink(test_file1);
        unlink(test_file2);
        rmdir(test_dir);
        return 1;
    }
    
    printf("Directory contains %lu items:\n", (unsigned long)result.count);
    
    for (size_t i = 0; i < result.count; i++) {
        metadata_info_t* info = &result.metadata[i];
        printf("  Item %lu: %s (%s)\n", 
               (unsigned long)i, 
               info->name ? info->name : "N/A",
               metadata_get_file_type_string(info));
    }
    
    // Clean up
    metadata_free_result(&result);
    unlink(test_file1);
    unlink(test_file2);
    rmdir(test_dir);
    
    printf("Directory metadata test passed!\n");
    return 0;
}

int main() {
    printf("Running metadata analysis tests...\n");
    
    int result1 = test_metadata_file_info();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_metadata_directory_info();
    if (result2 != 0) {
        return result2;
    }
    
    printf("All metadata analysis tests passed!\n");
    return 0;
}