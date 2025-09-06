#include "hidden.h"
#include "rootkits.h"
#include "../metadata/metadata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Helper function to check if string contains only digits
static int is_all_digits(const char* str) {
    if (!str || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

void hidden_detection_result_init(hidden_detection_result_t* result) {
    if (result) {
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int hidden_detection_result_add_file(hidden_detection_result_t* result, const hidden_file_result_t* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        hidden_file_result_t* new_files = (hidden_file_result_t*)realloc(result->files, new_capacity * sizeof(hidden_file_result_t));
        if (!new_files) {
            return -1;
        }
        result->files = new_files;
        result->capacity = new_capacity;
    }
    
    // Copy file data
    hidden_file_result_t* new_file = &result->files[result->count];
    new_file->path = strdup_safe(file->path);
    new_file->is_suspicious = file->is_suspicious;
    new_file->is_hidden = file->is_hidden;
    new_file->has_suspicious_name = file->has_suspicious_name;
    new_file->has_suspicious_permissions = file->has_suspicious_permissions;
    new_file->reason = strdup_safe(file->reason);
    
    if ((!new_file->path || !new_file->reason) && 
        (file->path || file->reason)) {
        // Clean up on failure
        free(new_file->path);
        free(new_file->reason);
        return -1;
    }
    
    result->count++;
    return 0;
}

void hidden_detection_result_free(hidden_detection_result_t* result) {
    if (result) {
        if (result->files) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->files[i].path);
                free(result->files[i].reason);
            }
            free(result->files);
        }
        result->files = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

void rootkit_detection_result_init(rootkit_detection_result_t* result) {
    if (result) {
        result->rootkits = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int rootkit_detection_result_add_rootkit(rootkit_detection_result_t* result, const rootkit_result_t* rootkit) {
    if (!result || !rootkit) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        rootkit_result_t* new_rootkits = (rootkit_result_t*)realloc(result->rootkits, new_capacity * sizeof(rootkit_result_t));
        if (!new_rootkits) {
            return -1;
        }
        result->rootkits = new_rootkits;
        result->capacity = new_capacity;
    }
    
    // Copy rootkit data
    rootkit_result_t* new_rootkit = &result->rootkits[result->count];
    new_rootkit->name = strdup_safe(rootkit->name);
    new_rootkit->description = strdup_safe(rootkit->description);
    new_rootkit->detected = rootkit->detected;
    new_rootkit->detection_method = strdup_safe(rootkit->detection_method);
    
    if ((!new_rootkit->name || !new_rootkit->description || !new_rootkit->detection_method) && 
        (rootkit->name || rootkit->description || rootkit->detection_method)) {
        // Clean up on failure
        free(new_rootkit->name);
        free(new_rootkit->description);
        free(new_rootkit->detection_method);
        return -1;
    }
    
    result->count++;
    return 0;
}

void rootkit_detection_result_free(rootkit_detection_result_t* result) {
    if (result) {
        if (result->rootkits) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->rootkits[i].name);
                free(result->rootkits[i].description);
                free(result->rootkits[i].detection_method);
            }
            free(result->rootkits);
        }
        result->rootkits = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int hidden_is_suspicious_filename(const char* filename) {
    if (!filename) {
        return -1;
    }
    
    // Check for suspicious patterns
    const char* suspicious_patterns[] = {
        "..",           // Directory traversal
        "~",            // Backup files
        ".tmp",         // Temporary files
        ".temp",        // Temporary files
        ".bak",         // Backup files
        ".old",         // Old files
        ".orig",        // Original files
        ".swp",         // Vim swap files
        ".swo",         // Vim swap files
        ".swn",         // Vim swap files
        ".lock",        // Lock files
        ".pid",         // Process ID files
        ".log",         // Log files (in system directories)
        NULL
    };
    
    for (int i = 0; suspicious_patterns[i] != NULL; i++) {
        if (strstr(filename, suspicious_patterns[i]) != NULL) {
            return 1;
        }
    }
    
    // Check for files with only dots or dots and numbers
    int only_dots_and_digits = 1;
    for (int i = 0; filename[i] != '\0'; i++) {
        if (filename[i] != '.' && !isdigit(filename[i])) {
            only_dots_and_digits = 0;
            break;
        }
    }
    
    if (only_dots_and_digits) {
        return 1;
    }
    
    // Check for very long filenames
    if (strlen(filename) > 255) {
        return 1;
    }
    
    return 0;
}

int hidden_has_suspicious_permissions(const char* path) {
    if (!path) {
        return -1;
    }
    
    metadata_info_t info;
    if (metadata_get_file_info(path, &info) != 0) {
        return -1;
    }
    
    int result = metadata_has_suspicious_permissions(&info);
    metadata_free_info(&info);
    
    return result;
}

// Check if a file is suspicious
static int is_suspicious_file(const char* path, const char* filename, hidden_file_result_t* result) {
    if (!path || !filename || !result) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(hidden_file_result_t));
    result->path = strdup_safe(path);
    
    // Check if file is hidden (starts with dot on Unix)
    result->is_hidden = (filename[0] == '.') ? 1 : 0;
    
    // Check for suspicious filename
    result->has_suspicious_name = hidden_is_suspicious_filename(filename);
    
    // Check for suspicious permissions
    result->has_suspicious_permissions = hidden_has_suspicious_permissions(path);
    
    // Determine if file is suspicious
    result->is_suspicious = result->is_hidden || result->has_suspicious_name || result->has_suspicious_permissions;
    
    // Set reason
    if (result->is_suspicious) {
        if (result->is_hidden) {
            result->reason = strdup_safe("Hidden file");
        } else if (result->has_suspicious_name) {
            result->reason = strdup_safe("Suspicious filename");
        } else if (result->has_suspicious_permissions) {
            result->reason = strdup_safe("Suspicious permissions");
        } else {
            result->reason = strdup_safe("Unknown reason");
        }
    } else {
        result->reason = strdup_safe("No suspicious characteristics");
    }
    
    return result->is_suspicious;
}

int hidden_detect_files(const char* path, hidden_detection_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    hidden_detection_result_init(result);
    
    // Open directory
    DIR* dir = opendir(path);
    if (!dir) {
        return -1;
    }
    
    // Process directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", path, entry->d_name);
        
        // Check if file is suspicious
        hidden_file_result_t file_result;
        if (is_suspicious_file(full_path, entry->d_name, &file_result) >= 0) {
            // Add to results if suspicious or if we want to include all files
            if (file_result.is_suspicious) {
                hidden_detection_result_add_file(result, &file_result);
            }
        }
        
        // Clean up
        free(file_result.path);
        free(file_result.reason);
        free(full_path);
    }
    
    closedir(dir);
    
    return 0;
}

int hidden_detect_rootkits(rootkit_detection_result_t* result) {
    if (!result) {
        return -1;
    }
    
    rootkit_detection_result_init(result);
    
    // Get known rootkits
    size_t rootkit_count;
    const rootkit_info_t* rootkits = hidden_get_known_rootkits(&rootkit_count);
    
    if (!rootkits || rootkit_count == 0) {
        return -1;
    }
    
    // Check for each rootkit
    for (size_t i = 0; i < rootkit_count; i++) {
        const rootkit_info_t* rk = &rootkits[i];
        
        // Create rootkit result
        rootkit_result_t rootkit_result;
        rootkit_result.name = strdup_safe(rk->name);
        rootkit_result.description = strdup_safe(rk->description);
        rootkit_result.detection_method = strdup_safe(rk->detection_method);
        
        // Check if rootkit is detected (simplified implementation)
        // In a real implementation, this would check for specific files, processes, etc.
        rootkit_result.detected = 0; // For now, assume not detected
        
        // Add to results
        rootkit_detection_result_add_rootkit(result, &rootkit_result);
        
        // Clean up
        free(rootkit_result.name);
        free(rootkit_result.description);
        free(rootkit_result.detection_method);
    }
    
    return 0;
}