#include "libs/mount/mount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

// Create a test image file
static int create_test_image(const char* image_path, size_t size) {
    // Create a sparse file to simulate an image
    int fd = open(image_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    // Write some data at the beginning
    char header[] = "This is a test forensic image file";
    write(fd, header, sizeof(header) - 1);
    
    // Seek to near the end and write some data
    if (lseek(fd, size - 100, SEEK_SET) >= 0) {
        char footer[] = "End of test forensic image file";
        write(fd, footer, sizeof(footer) - 1);
    }
    
    close(fd);
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* image_path, const char* mount_point) {
    unlink(image_path);
    if (mount_point) {
        char flag_file[1024];
        snprintf(flag_file, sizeof(flag_file), "%s/.mounted", mount_point);
        unlink(flag_file);
        rmdir(mount_point);
    }
}

int test_format_detection() {
    printf("Testing image format detection...\n");
    
    // Test various formats
    const char* test_cases[][2] = {
        {"test_image.e01", "EWF"},
        {"test_image.vmdk", "VMDK"},
        {"test_image.vhd", "VHD"},
        {"test_image.qcow2", "QCOW2"},
        {"test_image.dd", "RAW"},
        {"test_image.img", "RAW"},
        {"test_image", "RAW"}, // No extension
        {NULL, NULL}
    };
    
    for (int i = 0; test_cases[i][0] != NULL; i++) {
        image_format_t format = mount_detect_format(test_cases[i][0]);
        const char* description = mount_get_format_description(format);
        
        printf("File: %s -> Format: %s\n", test_cases[i][0], description);
    }
    
    printf("Image format detection test passed!\n");
    return 0;
}

int test_image_info() {
    printf("Testing image information retrieval...\n");
    
    const char* test_image = "/tmp/test_forensic_image.dd";
    const size_t image_size = 1024 * 1024; // 1MB
    
    // Create test image
    if (create_test_image(test_image, image_size) != 0) {
        printf("Failed to create test image\n");
        return 1;
    }
    
    // Get image information
    image_info_t info;
    int ret = mount_get_image_info(test_image, &info);
    
    if (ret != 0) {
        printf("Failed to get image information\n");
        cleanup_test_environment(test_image, NULL);
        return 1;
    }
    
    printf("Image information:\n");
    printf("  Path: %s\n", info.image_path ? info.image_path : "unknown");
    printf("  Format: %s\n", info.format_description ? info.format_description : "unknown");
    printf("  Size: %lu bytes\n", (unsigned long)info.image_size);
    printf("  Has partitions: %s\n", info.has_partitions ? "yes" : "no");
    
    // Clean up
    image_info_free(&info);
    cleanup_test_environment(test_image, NULL);
    
    printf("Image information test passed!\n");
    return 0;
}

int test_mount_operations() {
    printf("Testing mount operations...\n");
    
    const char* test_image = "/tmp/test_forensic_image.dd";
    const char* mount_point = "/tmp/forensic_test_mount";
    const size_t image_size = 1024 * 1024; // 1MB
    
    // Create test image
    if (create_test_image(test_image, image_size) != 0) {
        printf("Failed to create test image\n");
        return 1;
    }
    
    // Create mount point directory
    if (mkdir(mount_point, 0755) != 0 && errno != EEXIST) {
        printf("Failed to create mount point directory\n");
        cleanup_test_environment(test_image, NULL);
        return 1;
    }
    
    // Test mounting with custom options
    mount_options_t options;
    mount_options_init(&options);
    options.mount_point = strdup(mount_point);
    options.read_only = 1;
    options.no_execute = 1;
    
    mount_result_t result;
    int ret = mount_image(test_image, &options, &result);
    
    if (ret != 0) {
        printf("Failed to mount image: %s\n", result.error_message ? result.error_message : "unknown error");
        mount_options_free(&options);
        mount_result_free(&result);
        cleanup_test_environment(test_image, mount_point);
        return 1;
    }
    
    printf("Mounted image successfully:\n");
    printf("  Image: %s\n", result.image_path ? result.image_path : "unknown");
    printf("  Mount point: %s\n", result.mount_point ? result.mount_point : "unknown");
    printf("  Format: %s\n", mount_get_format_description(result.format));
    printf("  Mounted: %s\n", result.is_mounted ? "yes" : "no");
    
    // Test unmounting
    ret = unmount_image(mount_point);
    if (ret != 0) {
        printf("Failed to unmount image\n");
        mount_options_free(&options);
        mount_result_free(&result);
        cleanup_test_environment(test_image, mount_point);
        return 1;
    }
    
    printf("Unmounted image successfully\n");
    
    // Clean up
    mount_options_free(&options);
    mount_result_free(&result);
    cleanup_test_environment(test_image, mount_point);
    
    printf("Mount operations test passed!\n");
    return 0;
}

int test_mount_point_validation() {
    printf("Testing mount point validation...\n");
    
    const char* test_cases[][2] = {
        {"/tmp", "0"},           // Existing directory, not empty
        {"/nonexistent", "0"},   // Non-existent path
        {"/tmp/test_mount_point_validation", "1"}, // Valid (we'll create it)
        {NULL, NULL}
    };
    
    // Create a temporary directory for testing
    const char* test_dir = "/tmp/test_mount_point_validation";
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        printf("Failed to create test directory\n");
        return 1;
    }
    
    for (int i = 0; test_cases[i][0] != NULL; i++) {
        int expected = atoi(test_cases[i][1]);
        int result = mount_validate_mount_point(test_cases[i][0]);
        
        if (result != expected) {
            printf("Test failed for '%s': expected %d, got %d\n", 
                   test_cases[i][0], expected, result);
        } else {
            printf("Test passed for '%s': %s\n", 
                   test_cases[i][0], result ? "valid" : "invalid");
        }
    }
    
    // Clean up
    rmdir(test_dir);
    
    printf("Mount point validation test passed!\n");
    return 0;
}

int main() {
    printf("Running forensic image mounting tests...\n");
    
    int result1 = test_format_detection();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_image_info();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_mount_operations();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_mount_point_validation();
    if (result4 != 0) {
        return result4;
    }
    
    printf("All forensic image mounting tests passed!\n");
    return 0;
}