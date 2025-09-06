#ifndef LIBS_MOUNT_MOUNT_H
#define LIBS_MOUNT_MOUNT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Supported image formats
typedef enum {
    IMAGE_FORMAT_RAW = 0,
    IMAGE_FORMAT_EWF = 1,   // E01, Ex01 (Encase)
    IMAGE_FORMAT_VMDK = 2,  // VMware
    IMAGE_FORMAT_VHD = 3,   // Virtual Hard Disk
    IMAGE_FORMAT_QCOW2 = 4, // QEMU Copy-On-Write
    IMAGE_FORMAT_DD = 5,    // Raw disk dump
    IMAGE_FORMAT_UNKNOWN = -1
} image_format_t;

// Mount options
typedef struct {
    int read_only;          // Mount read-only (default: 1)
    int no_execute;         // Prevent execution of files (default: 1)
    int hide_meta_files;    // Hide metadata files (default: 0)
    char* mount_point;      // Mount point (if NULL, auto-generate)
    char* offset;           // Offset in bytes (for partition images)
    char* sector_size;      // Sector size (default: 512)
} mount_options_t;

// Mount result
typedef struct {
    char* image_path;
    char* mount_point;
    image_format_t format;
    int is_mounted;
    char* error_message;
} mount_result_t;

// Image information
typedef struct {
    char* image_path;
    image_format_t format;
    uint64_t image_size;
    int has_partitions;
    char* format_description;
} image_info_t;

// Initialize mount options with default values
void mount_options_init(mount_options_t* options);

// Free mount options
void mount_options_free(mount_options_t* options);

// Initialize mount result
void mount_result_init(mount_result_t* result);

// Free mount result
void mount_result_free(mount_result_t* result);

// Initialize image info
void image_info_init(image_info_t* info);

// Free image info
void image_info_free(image_info_t* info);

// Detect image format
// image_path: path to the image file
// Returns the detected format, or IMAGE_FORMAT_UNKNOWN on error
image_format_t mount_detect_format(const char* image_path);

// Get format description
// format: image format
// Returns a description of the format
const char* mount_get_format_description(image_format_t format);

// Get image information
// image_path: path to the image file
// info: output image information (must be freed with image_info_free)
// Returns 0 on success, non-zero on error
int mount_get_image_info(const char* image_path, image_info_t* info);

// Mount forensic image
// image_path: path to the image file
// options: mount options
// result: output mount result (must be freed with mount_result_free)
// Returns 0 on success, non-zero on error
int mount_image(const char* image_path, const mount_options_t* options, mount_result_t* result);

// Unmount forensic image
// mount_point: mount point to unmount
// Returns 0 on success, non-zero on error
int unmount_image(const char* mount_point);

// List mounted forensic images
// results: array of mount results (must be freed with mount_result_free for each element)
// count: output number of mounted images
// Returns 0 on success, non-zero on error
int mount_list_images(mount_result_t** results, size_t* count);

// Validate mount point
// mount_point: mount point to validate
// Returns 1 if valid, 0 if invalid, negative on error
int mount_validate_mount_point(const char* mount_point);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MOUNT_MOUNT_H