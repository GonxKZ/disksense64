#include "libs/slack/slack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

// Create test files with slack space
static int create_test_files(const char* test_dir) {
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    // Create files with different sizes to generate slack space
    const char* filenames[] = {
        "small_file.txt",
        "medium_file.txt",
        "large_file.txt",
        NULL
    };
    
    const size_t sizes[] = {
        100,   // Small file
        2000,  // Medium file
        10000  // Large file
    };
    
    // Cluster size for typical filesystems
    const size_t cluster_size = 4096;
    
    for (int i = 0; filenames[i] != NULL; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", test_dir, filenames[i]);
        
        // Calculate file size that will create slack space
        size_t file_size = sizes[i];
        size_t allocated_size = ((file_size + cluster_size - 1) / cluster_size) * cluster_size;
        size_t slack_size = allocated_size - file_size;
        
        printf("Creating file %s: size=%lu, allocated=%lu, slack=%lu\n",
               filenames[i], (unsigned long)file_size, 
               (unsigned long)allocated_size, (unsigned long)slack_size);
        
        // Create file with specific size
        int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            // Write file content
            char* content = (char*)malloc(file_size);
            if (content) {
                // Fill with pattern
                for (size_t j = 0; j < file_size; j++) {
                    content[j] = 'A' + (j % 26);
                }
                write(fd, content, file_size);
                free(content);
            }
            close(fd);
        }
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

int test_filesystem_info() {
    printf("Testing filesystem information retrieval...\n");
    
    filesystem_info_t info;
    int ret = slack_get_filesystem_info(".", &info);
    
    if (ret != 0) {
        printf("Failed to get filesystem information\n");
        return 1;
    }
    
    printf("Filesystem information:\n");
    printf("  Type: %s\n", info.filesystem_type ? info.filesystem_type : "unknown");
    printf("  Block size: %lu\n", (unsigned long)info.block_size);
    printf("  Cluster size: %lu\n", (unsigned long)info.cluster_size);
    printf("  Total clusters: %lu\n", (unsigned long)info.total_clusters);
    printf("  Free clusters: %lu\n", (unsigned long)info.free_clusters);
    printf("  Used clusters: %lu\n", (unsigned long)info.used_clusters);
    
    filesystem_info_free(&info);
    
    printf("Filesystem information test passed!\n");
    return 0;
}

int test_slack_analysis() {
    printf("Testing slack space analysis...\n");
    
    const char* test_dir = "/tmp/slack_test_dir";
    
    // Create test environment
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test environment\n");
        return 1;
    }
    
    // Analyze slack space
    slack_analysis_result_t result;
    int ret = slack_analyze_directory(test_dir, &result);
    
    if (ret != 0) {
        printf("Failed to analyze slack space\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("Found %lu files with slack space:\n", (unsigned long)result.file_count);
    
    for (size_t i = 0; i < result.file_count; i++) {
        slack_file_result_t* file = &result.files[i];
        printf("  File: %s\n", file->file_path ? file->file_path : "unknown");
        printf("    File size: %lu bytes\n", (unsigned long)file->file_size);
        printf("    Allocated size: %lu bytes\n", (unsigned long)file->allocated_size);
        printf("    Slack size: %lu bytes\n", (unsigned long)file->slack_size);
        printf("    Slack data size: %lu bytes\n", (unsigned long)file->slack_data_size);
    }
    
    // Test searching in slack space
    printf("\nSearching for keywords in slack space...\n");
    int matches = slack_search_slack(&result, "test", 0); // Case insensitive
    printf("Found %d matches for 'test'\n", matches);
    
    // Clean up
    slack_analysis_result_free(&result);
    cleanup_test_environment(test_dir);
    
    printf("Slack space analysis test passed!\n");
    return 0;
}

int test_unallocated_analysis() {
    printf("Testing unallocated space analysis...\n");
    
    // Analyze unallocated space (simulated)
    unallocated_analysis_result_t result;
    int ret = slack_analyze_unallocated("/dev/sda1", &result); // Using a common device path for testing
    
    if (ret != 0) {
        printf("Failed to analyze unallocated space\n");
        return 1;
    }
    
    printf("Found %lu unallocated clusters:\n", (unsigned long)result.cluster_count);
    
    for (size_t i = 0; i < result.cluster_count && i < 3; i++) { // Show first 3
        unallocated_cluster_result_t* cluster = &result.clusters[i];
        printf("  Cluster %lu:\n", (unsigned long)cluster->cluster_number);
        printf("    Offset: %lu\n", (unsigned long)cluster->cluster_offset);
        printf("    Size: %lu bytes\n", (unsigned long)cluster->cluster_size);
        printf("    Data size: %lu bytes\n", (unsigned long)cluster->cluster_data_size);
    }
    
    if (result.cluster_count > 3) {
        printf("  ... and %lu more clusters\n", (unsigned long)(result.cluster_count - 3));
    }
    
    // Test searching in unallocated space
    printf("\nSearching for keywords in unallocated space...\n");
    int matches = slack_search_unallocated(&result, "password", 0); // Case insensitive
    printf("Found %d matches for 'password'\n", matches);
    
    // Clean up
    unallocated_analysis_result_free(&result);
    
    printf("Unallocated space analysis test passed!\n");
    return 0;
}

int main() {
    srand(time(NULL)); // Initialize random seed
    
    printf("Running slack space and unallocated space analysis tests...\n");
    
    int result1 = test_filesystem_info();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_slack_analysis();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_unallocated_analysis();
    if (result3 != 0) {
        return result3;
    }
    
    printf("All slack space and unallocated space analysis tests passed!\n");
    return 0;
}