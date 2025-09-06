#include "mount.h"
#include "image_formats.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <sys/mount.h>
#include <dirent.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Helper function to check if file exists
static int file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

void mount_options_init(mount_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(mount_options_t));
        options->read_only = 1;
        options->no_execute = 1;
        options->sector_size = strdup_safe("512");
    }
}

void mount_options_free(mount_options_t* options) {
    if (options) {
        free(options->mount_point);
        free(options->offset);
        free(options->sector_size);
        memset(options, 0, sizeof(mount_options_t));
    }
}

void mount_result_init(mount_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(mount_result_t));
    }
}

void mount_result_free(mount_result_t* result) {
    if (result) {
        free(result->image_path);
        free(result->mount_point);
        free(result->error_message);
        memset(result, 0, sizeof(mount_result_t));
    }
}

void image_info_init(image_info_t* info) {
    if (info) {
        memset(info, 0, sizeof(image_info_t));
    }
}

void image_info_free(image_info_t* info) {
    if (info) {
        free(info->image_path);
        free(info->format_description);
        memset(info, 0, sizeof(image_info_t));
    }
}

image_format_t mount_detect_format(const char* image_path) {
    if (!image_path) {
        return IMAGE_FORMAT_UNKNOWN;
    }
    
    // Check if file exists
    if (!file_exists(image_path)) {
        return IMAGE_FORMAT_UNKNOWN;
    }
    
    // Get file extension
    const char* ext = strrchr(image_path, '.');
    if (!ext) {
        ext = ""; // No extension
    }
    
    // Check extension
    if (strcasecmp(ext, ".e01") == 0 || strcasecmp(ext, ".ex01") == 0) {
        return IMAGE_FORMAT_EWF;
    } else if (strcasecmp(ext, ".vmdk") == 0) {
        return IMAGE_FORMAT_VMDK;
    } else if (strcasecmp(ext, ".vhd") == 0 || strcasecmp(ext, ".vhd.xz") == 0) {
        return IMAGE_FORMAT_VHD;
    } else if (strcasecmp(ext, ".qcow2") == 0 || strcasecmp(ext, ".qcow") == 0) {
        return IMAGE_FORMAT_QCOW2;
    } else if (strcasecmp(ext, ".dd") == 0 || strcasecmp(ext, ".img") == 0 || ext[0] == '\0') {
        // Raw images often have no extension or .dd/.img
        return IMAGE_FORMAT_RAW;
    }
    
    // Try to read file header to determine format
    int fd = open(image_path, O_RDONLY);
    if (fd < 0) {
        return IMAGE_FORMAT_UNKNOWN;
    }
    
    char header[16];
    ssize_t bytes_read = read(fd, header, sizeof(header));
    close(fd);
    
    if (bytes_read < 8) {
        return IMAGE_FORMAT_UNKNOWN;
    }
    
    // Check for EWF signature
    if (memcmp(header, "EVF\x09\x0d\x0a\xff\x00", 8) == 0) {
        return IMAGE_FORMAT_EWF;
    }
    
    // Check for QCOW2 signature
    if (memcmp(header, "QFI\xfb", 4) == 0) {
        return IMAGE_FORMAT_QCOW2;
    }
    
    // Default to raw
    return IMAGE_FORMAT_RAW;
}

const char* mount_get_format_description(image_format_t format) {
    switch (format) {
        case IMAGE_FORMAT_RAW:
            return "Raw disk image";
        case IMAGE_FORMAT_EWF:
            return "Encase EWF (E01/Ex01)";
        case IMAGE_FORMAT_VMDK:
            return "VMware Virtual Disk";
        case IMAGE_FORMAT_VHD:
            return "Virtual Hard Disk";
        case IMAGE_FORMAT_QCOW2:
            return "QEMU Copy-On-Write v2";
        case IMAGE_FORMAT_DD:
            return "Raw disk dump";
        default:
            return "Unknown format";
    }
}

int mount_get_image_info(const char* image_path, image_info_t* info) {
    if (!image_path || !info) {
        return -1;
    }
    
    image_info_init(info);
    
    // Check if file exists
    if (!file_exists(image_path)) {
        return -1;
    }
    
    // Get file statistics
    struct stat st;
    if (stat(image_path, &st) != 0) {
        return -1;
    }
    
    // Fill in image information
    info->image_path = strdup_safe(image_path);
    info->format = mount_detect_format(image_path);
    info->image_size = st.st_size;
    info->has_partitions = 0; // Would need to analyze the image to determine this
    info->format_description = strdup_safe(mount_get_format_description(info->format));
    
    return 0;
}

int mount_image(const char* image_path, const mount_options_t* options, mount_result_t* result) {
    if (!image_path || !result) {
        return -1;
    }
    
    mount_result_init(result);
    
    // Check if image file exists
    if (!file_exists(image_path)) {
        result->error_message = strdup_safe("Image file does not exist");
        return -1;
    }
    
    // Detect image format
    image_format_t format = mount_detect_format(image_path);
    if (format == IMAGE_FORMAT_UNKNOWN) {
        result->error_message = strdup_safe("Unknown image format");
        return -1;
    }
    
    // Validate mount point
    char* mount_point = NULL;
    if (options && options->mount_point) {
        if (mount_validate_mount_point(options->mount_point) <= 0) {
            result->error_message = strdup_safe("Invalid mount point");
            return -1;
        }
        mount_point = strdup_safe(options->mount_point);
    } else {
        // Generate a temporary mount point
        char temp_path[] = "/tmp/forensic_mount_XXXXXX";
        if (mkdtemp(temp_path) == NULL) {
            result->error_message = strdup_safe("Failed to create temporary mount point");
            return -1;
        }
        mount_point = strdup_safe(temp_path);
    }
    
    // Store result information
    result->image_path = strdup_safe(image_path);
    result->mount_point = mount_point;
    result->format = format;
    
    // In a real implementation, this would actually mount the image
    // For this example, we'll simulate a successful mount
    printf("Mounting %s (%s) to %s\n", 
           image_path, mount_get_format_description(format), mount_point);
    
    // Create a flag file to indicate the image is mounted
    char flag_file[1024];
    snprintf(flag_file, sizeof(flag_file), "%s/.mounted", mount_point);
    FILE* flag = fopen(flag_file, "w");
    if (flag) {
        fprintf(flag, "Mounted image: %s\nFormat: %s\n", 
                image_path, mount_get_format_description(format));
        fclose(flag);
    }
    
    result->is_mounted = 1;
    return 0;
}

int unmount_image(const char* mount_point) {
    if (!mount_point) {
        return -1;
    }
    
    // Check if mount point exists
    if (!file_exists(mount_point)) {
        return -1;
    }
    
    // In a real implementation, this would actually unmount the image
    // For this example, we'll just remove the flag file
    char flag_file[1024];
    snprintf(flag_file, sizeof(flag_file), "%s/.mounted", mount_point);
    unlink(flag_file);
    
    printf("Unmounted image from %s\n", mount_point);
    
    return 0;
}

int mount_list_images(mount_result_t** results, size_t* count) {
    if (!results || !count) {
        return -1;
    }
    
    // In a real implementation, this would scan for mounted forensic images
    // For this example, we'll return an empty list
    *results = NULL;
    *count = 0;
    
    printf("Listing mounted forensic images...\n");
    
    return 0;
}

int mount_validate_mount_point(const char* mount_point) {
    if (!mount_point) {
        return -1;
    }
    
    // Check if path is absolute
    if (mount_point[0] != '/') {
        return 0; // Not absolute
    }
    
    // Check if path exists
    struct stat st;
    if (stat(mount_point, &st) != 0) {
        return 0; // Does not exist
    }
    
    // Check if it's a directory
    if (!S_ISDIR(st.st_mode)) {
        return 0; // Not a directory
    }
    
    // Check if directory is empty
    DIR* dir = opendir(mount_point);
    if (!dir) {
        return 0; // Cannot open directory
    }
    
    int is_empty = 1;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            is_empty = 0;
            break;
        }
    }
    
    closedir(dir);
    
    if (!is_empty) {
        return 0; // Directory is not empty
    }
    
    return 1; // Valid mount point
}