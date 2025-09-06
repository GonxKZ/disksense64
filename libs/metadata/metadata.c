#include "metadata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#endif

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Helper function to get owner name from UID
static char* get_owner_name(uid_t uid) {
#ifndef _WIN32
    struct passwd* pwd = getpwuid(uid);
    if (pwd) {
        return strdup_safe(pwd->pw_name);
    }
#endif
    return NULL;
}

// Helper function to get group name from GID
static char* get_group_name(gid_t gid) {
#ifndef _WIN32
    struct group* grp = getgrgid(gid);
    if (grp) {
        return strdup_safe(grp->gr_name);
    }
#endif
    return NULL;
}

int metadata_get_file_info(const char* path, metadata_info_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(metadata_info_t));
    
#ifdef _WIN32
    // Windows implementation
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attrs)) {
        return -1;
    }
    
    // Basic info
    result->name = strdup_safe(path); // Simplified
    result->full_path = strdup_safe(path);
    result->size = ((uint64_t)attrs.nFileSizeHigh << 32) | attrs.nFileSizeLow;
    result->is_directory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    result->is_symlink = 0; // Windows symlinks require special handling
    result->is_hidden = (attrs.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? 1 : 0;
    
    // Timestamps (convert Windows FILETIME to time_t)
    // This is a simplified conversion
    result->creation_time = 0; // Would need actual conversion
    result->last_write_time = 0; // Would need actual conversion
    result->last_access_time = 0; // Would need actual conversion
    result->change_time = 0; // Would need actual conversion
    
    // Permissions and ownership
    result->permissions = attrs.dwFileAttributes;
    result->owner_id = 0;
    result->group_id = 0;
    result->owner_name = NULL;
    result->group_name = NULL;
    
    // Attributes
    result->attributes = attrs.dwFileAttributes;
    result->extended_attributes = NULL;
    result->extended_attributes_size = 0;
#else
    // Unix/Linux implementation
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    
    // Basic info
    result->name = strdup_safe(path); // Simplified
    result->full_path = strdup_safe(path);
    result->size = st.st_size;
    result->is_directory = S_ISDIR(st.st_mode) ? 1 : 0;
    result->is_symlink = S_ISLNK(st.st_mode) ? 1 : 0;
    result->is_hidden = (path[0] == '.') ? 1 : 0; // Unix hidden files start with .
    
    // Timestamps
    result->creation_time = st.st_ctime;
    result->last_write_time = st.st_mtime;
    result->last_access_time = st.st_atime;
    result->change_time = st.st_ctime; // Change time same as creation on many systems
    
    // Permissions and ownership
    result->permissions = st.st_mode;
    result->owner_id = st.st_uid;
    result->group_id = st.st_gid;
    result->owner_name = get_owner_name(st.st_uid);
    result->group_name = get_group_name(st.st_gid);
    
    // Attributes
    result->attributes = 0; // Not used on Unix
    result->extended_attributes = NULL;
    result->extended_attributes_size = 0;
#endif
    
    return 0;
}

int metadata_get_directory_info(const char* path, metadata_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    // Initialize result
    result->metadata = NULL;
    result->count = 0;
    
#ifdef _WIN32
    // Windows implementation
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    // Count files first
    size_t count = 0;
    char** file_paths = NULL;
    size_t file_paths_capacity = 0;
    
    do {
        if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
            // Reallocate if needed
            if (count >= file_paths_capacity) {
                file_paths_capacity = (file_paths_capacity == 0) ? 16 : file_paths_capacity * 2;
                file_paths = (char**)realloc(file_paths, file_paths_capacity * sizeof(char*));
            }
            
            // Build full path
            size_t path_len = strlen(path);
            size_t name_len = strlen(find_data.cFileName);
            file_paths[count] = (char*)malloc(path_len + name_len + 2);
            if (file_paths[count]) {
                sprintf(file_paths[count], "%s\\%s", path, find_data.cFileName);
                count++;
            }
        }
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
    
    // Allocate metadata array
    result->metadata = (metadata_info_t*)malloc(count * sizeof(metadata_info_t));
    if (!result->metadata) {
        // Free allocated paths
        for (size_t i = 0; i < count; i++) {
            free(file_paths[i]);
        }
        free(file_paths);
        return -1;
    }
    
    // Fill metadata
    result->count = 0;
    for (size_t i = 0; i < count; i++) {
        if (metadata_get_file_info(file_paths[i], &result->metadata[result->count]) == 0) {
            result->count++;
        }
        free(file_paths[i]);
    }
    
    free(file_paths);
    
#else
    // Unix/Linux implementation
    DIR* dir = opendir(path);
    if (!dir) {
        return -1;
    }
    
    // Count entries first
    size_t count = 0;
    struct dirent* entry;
    
    // Rewind and count
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    
    // Allocate metadata array
    result->metadata = (metadata_info_t*)malloc(count * sizeof(metadata_info_t));
    if (!result->metadata) {
        closedir(dir);
        return -1;
    }
    
    // Fill metadata
    result->count = 0;
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Build full path
            size_t path_len = strlen(path);
            size_t name_len = strlen(entry->d_name);
            char* full_path = (char*)malloc(path_len + name_len + 2);
            if (full_path) {
                sprintf(full_path, "%s/%s", path, entry->d_name);
                
                if (metadata_get_file_info(full_path, &result->metadata[result->count]) == 0) {
                    result->count++;
                }
                
                free(full_path);
            }
        }
    }
    
    closedir(dir);
#endif
    
    return 0;
}

void metadata_free_info(metadata_info_t* info) {
    if (info) {
        free(info->name);
        free(info->full_path);
        free(info->owner_name);
        free(info->group_name);
        free(info->extended_attributes);
        memset(info, 0, sizeof(metadata_info_t));
    }
}

void metadata_free_result(metadata_result_t* result) {
    if (result) {
        if (result->metadata) {
            for (size_t i = 0; i < result->count; i++) {
                metadata_free_info(&result->metadata[i]);
            }
            free(result->metadata);
        }
        result->metadata = NULL;
        result->count = 0;
    }
}

int metadata_has_suspicious_permissions(const metadata_info_t* info) {
    if (!info) {
        return -1;
    }
    
#ifndef _WIN32
    // Check for world-writable files (excluding directories)
    if (!info->is_directory && (info->permissions & S_IWOTH)) {
        return 1;
    }
    
    // Check for setuid/setgid bits on regular files
    if (!info->is_directory && (info->permissions & (S_ISUID | S_ISGID))) {
        return 1;
    }
#endif
    
    return 0;
}

int metadata_is_hidden(const metadata_info_t* info) {
    if (!info) {
        return -1;
    }
    
    return info->is_hidden;
}

int metadata_get_permission_string(const metadata_info_t* info, char* buffer, size_t buffer_size) {
    if (!info || !buffer || buffer_size < 11) {
        return -1;
    }
    
#ifndef _WIN32
    // Unix permission string
    char perms[11];
    perms[0] = S_ISDIR(info->permissions) ? 'd' : '-';
    perms[1] = (info->permissions & S_IRUSR) ? 'r' : '-';
    perms[2] = (info->permissions & S_IWUSR) ? 'w' : '-';
    perms[3] = (info->permissions & S_IXUSR) ? 'x' : '-';
    perms[4] = (info->permissions & S_IRGRP) ? 'r' : '-';
    perms[5] = (info->permissions & S_IWGRP) ? 'w' : '-';
    perms[6] = (info->permissions & S_IXGRP) ? 'x' : '-';
    perms[7] = (info->permissions & S_IROTH) ? 'r' : '-';
    perms[8] = (info->permissions & S_IWOTH) ? 'w' : '-';
    perms[9] = (info->permissions & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
    
    // Handle special bits
    if (info->permissions & S_ISUID) perms[3] = (info->permissions & S_IXUSR) ? 's' : 'S';
    if (info->permissions & S_ISGID) perms[6] = (info->permissions & S_IXGRP) ? 's' : 'S';
    if (info->permissions & S_ISVTX) perms[9] = (info->permissions & S_IXOTH) ? 't' : 'T';
    
    strncpy(buffer, perms, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return 0;
#else
    // Windows permission string (simplified)
    strncpy(buffer, "----------", buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return 0;
#endif
}

const char* metadata_get_file_type_string(const metadata_info_t* info) {
    if (!info) {
        return "unknown";
    }
    
    if (info->is_directory) {
        return "directory";
    } else if (info->is_symlink) {
        return "symbolic link";
    } else {
        return "regular file";
    }
}