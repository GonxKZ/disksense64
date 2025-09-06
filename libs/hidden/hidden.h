#ifndef LIBS_HIDDEN_HIDDEN_H
#define LIBS_HIDDEN_HIDDEN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hidden file detection result
typedef struct {
    char* path;
    int is_suspicious;
    int is_hidden;
    int has_suspicious_name;
    int has_suspicious_permissions;
    char* reason;
} hidden_file_result_t;

// Rootkit detection result
typedef struct {
    char* name;
    char* description;
    int detected;
    char* detection_method;
} rootkit_result_t;

// Hidden files detection result
typedef struct {
    hidden_file_result_t* files;
    size_t count;
    size_t capacity;
} hidden_detection_result_t;

// Rootkits detection result
typedef struct {
    rootkit_result_t* rootkits;
    size_t count;
    size_t capacity;
} rootkit_detection_result_t;

// Initialize hidden detection result
void hidden_detection_result_init(hidden_detection_result_t* result);

// Add a hidden file result to the detection result
int hidden_detection_result_add_file(hidden_detection_result_t* result, const hidden_file_result_t* file);

// Free hidden detection result
void hidden_detection_result_free(hidden_detection_result_t* result);

// Initialize rootkit detection result
void rootkit_detection_result_init(rootkit_detection_result_t* result);

// Add a rootkit result to the detection result
int rootkit_detection_result_add_rootkit(rootkit_detection_result_t* result, const rootkit_result_t* rootkit);

// Free rootkit detection result
void rootkit_detection_result_free(rootkit_detection_result_t* result);

// Detect hidden files in a directory
// path: path to the directory
// result: output detection result (must be freed with hidden_detection_result_free)
// Returns 0 on success, non-zero on error
int hidden_detect_files(const char* path, hidden_detection_result_t* result);

// Detect rootkits on the system
// result: output detection result (must be freed with rootkit_detection_result_free)
// Returns 0 on success, non-zero on error
int hidden_detect_rootkits(rootkit_detection_result_t* result);

// Check if a file name is suspicious
// filename: name of the file to check
// Returns 1 if suspicious, 0 if not, negative on error
int hidden_is_suspicious_filename(const char* filename);

// Check if file permissions are suspicious
// path: path to the file
// Returns 1 if suspicious, 0 if not, negative on error
int hidden_has_suspicious_permissions(const char* path);

#ifdef __cplusplus
}
#endif

#endif // LIBS_HIDDEN_HIDDEN_H