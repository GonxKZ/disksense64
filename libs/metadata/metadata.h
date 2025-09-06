#ifndef LIBS_METADATA_METADATA_H
#define LIBS_METADATA_METADATA_H

// Ensure we define _GNU_SOURCE before any system headers
#ifdef __cplusplus
// For C++ files, we need to define _GNU_SOURCE before including any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Extended file information structure
typedef struct {
    char* name;
    char* full_path;
    uint64_t size;
    int is_directory;
    int is_symlink;
    int is_hidden;
    
    // Timestamps
    time_t creation_time;
    time_t last_write_time;
    time_t last_access_time;
    time_t change_time;
    
    // Permissions and ownership
    uint32_t permissions;
    uint32_t owner_id;
    uint32_t group_id;
    char* owner_name;
    char* group_name;
    
    // File attributes
    uint32_t attributes;
    
    // Extended attributes (platform-specific)
    char* extended_attributes;
    size_t extended_attributes_size;
} metadata_info_t;

// Metadata analysis result
typedef struct {
    metadata_info_t* metadata;
    size_t count;
} metadata_result_t;

// Get metadata for a file
// path: path to the file
// result: output metadata information (must be freed with metadata_free_info)
// Returns 0 on success, non-zero on error
int metadata_get_file_info(const char* path, metadata_info_t* result);

// Get metadata for all files in a directory
// path: path to the directory
// result: output metadata result (must be freed with metadata_free_result)
// Returns 0 on success, non-zero on error
int metadata_get_directory_info(const char* path, metadata_result_t* result);

// Free metadata information
void metadata_free_info(metadata_info_t* info);

// Free metadata result
void metadata_free_result(metadata_result_t* result);

// Check if file has suspicious permissions (e.g., world-writable)
int metadata_has_suspicious_permissions(const metadata_info_t* info);

// Check if file is hidden
int metadata_is_hidden(const metadata_info_t* info);

// Get permission string (e.g., "rwxr-xr-x")
int metadata_get_permission_string(const metadata_info_t* info, char* buffer, size_t buffer_size);

// Get file type string (e.g., "regular file", "directory", "symlink")
const char* metadata_get_file_type_string(const metadata_info_t* info);

#ifdef __cplusplus
}
#endif

#endif // LIBS_METADATA_METADATA_H