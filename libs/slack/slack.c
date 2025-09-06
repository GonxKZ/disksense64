#include "slack.h"
#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <sys/statvfs.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Helper function to duplicate memory
static void* memdup(const void* src, size_t size) {
    if (!src || size == 0) return NULL;
    void* dst = malloc(size);
    if (dst) {
        memcpy(dst, src, size);
    }
    return dst;
}

void slack_analysis_result_init(slack_analysis_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(slack_analysis_result_t));
    }
}

int slack_analysis_result_add_file(slack_analysis_result_t* result, const slack_file_result_t* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->file_count >= result->file_capacity) {
        size_t new_capacity = (result->file_capacity == 0) ? 16 : result->file_capacity * 2;
        slack_file_result_t* new_files = (slack_file_result_t*)realloc(result->files, new_capacity * sizeof(slack_file_result_t));
        if (!new_files) {
            return -1;
        }
        result->files = new_files;
        result->file_capacity = new_capacity;
    }
    
    // Copy file data
    slack_file_result_t* new_file = &result->files[result->file_count];
    new_file->file_path = strdup_safe(file->file_path);
    new_file->file_size = file->file_size;
    new_file->allocated_size = file->allocated_size;
    new_file->slack_size = file->slack_size;
    new_file->slack_data = (uint8_t*)memdup(file->slack_data, file->slack_data_size);
    new_file->slack_data_size = file->slack_data_size;
    
    if ((!new_file->file_path || !new_file->slack_data) && 
        (file->file_path || (file->slack_data && file->slack_data_size > 0))) {
        // Clean up on failure
        free(new_file->file_path);
        free(new_file->slack_data);
        return -1;
    }
    
    result->file_count++;
    return 0;
}

void slack_analysis_result_free(slack_analysis_result_t* result) {
    if (result) {
        if (result->files) {
            for (size_t i = 0; i < result->file_count; i++) {
                free(result->files[i].file_path);
                free(result->files[i].slack_data);
            }
            free(result->files);
        }
        result->files = NULL;
        result->file_count = 0;
        result->file_capacity = 0;
    }
}

void unallocated_analysis_result_init(unallocated_analysis_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(unallocated_analysis_result_t));
    }
}

int unallocated_analysis_result_add_cluster(unallocated_analysis_result_t* result, const unallocated_cluster_result_t* cluster) {
    if (!result || !cluster) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->cluster_count >= result->cluster_capacity) {
        size_t new_capacity = (result->cluster_capacity == 0) ? 16 : result->cluster_capacity * 2;
        unallocated_cluster_result_t* new_clusters = (unallocated_cluster_result_t*)realloc(result->clusters, new_capacity * sizeof(unallocated_cluster_result_t));
        if (!new_clusters) {
            return -1;
        }
        result->clusters = new_clusters;
        result->cluster_capacity = new_capacity;
    }
    
    // Copy cluster data
    unallocated_cluster_result_t* new_cluster = &result->clusters[result->cluster_count];
    new_cluster->cluster_number = cluster->cluster_number;
    new_cluster->cluster_offset = cluster->cluster_offset;
    new_cluster->cluster_size = cluster->cluster_size;
    new_cluster->cluster_data = (uint8_t*)memdup(cluster->cluster_data, cluster->cluster_data_size);
    new_cluster->cluster_data_size = cluster->cluster_data_size;
    
    if (new_cluster->cluster_data == NULL && cluster->cluster_data_size > 0) {
        return -1;
    }
    
    result->cluster_count++;
    return 0;
}

void unallocated_analysis_result_free(unallocated_analysis_result_t* result) {
    if (result) {
        if (result->clusters) {
            for (size_t i = 0; i < result->cluster_count; i++) {
                free(result->clusters[i].cluster_data);
            }
            free(result->clusters);
        }
        result->clusters = NULL;
        result->cluster_count = 0;
        result->cluster_capacity = 0;
    }
}

void filesystem_info_free(filesystem_info_t* info) {
    if (info) {
        free(info->filesystem_type);
        memset(info, 0, sizeof(filesystem_info_t));
    }
}

int slack_get_filesystem_info(const char* path, filesystem_info_t* info) {
    if (!path || !info) {
        return -1;
    }
    
    // Initialize result
    memset(info, 0, sizeof(filesystem_info_t));
    
    // Get filesystem statistics
    struct statvfs stat_buf;
    if (statvfs(path, &stat_buf) != 0) {
        return -1;
    }
    
    // Fill in filesystem information
    info->filesystem_type = strdup_safe("ext4"); // Simplified - would detect real type in full implementation
    info->block_size = stat_buf.f_bsize;
    info->cluster_size = stat_buf.f_bsize; // Simplified - assuming block size equals cluster size
    info->total_clusters = stat_buf.f_blocks;
    info->free_clusters = stat_buf.f_bavail;
    info->used_clusters = stat_buf.f_blocks - stat_buf.f_bfree;
    
    return 0;
}

// Calculate slack space for a file
static int calculate_slack_space(const char* file_path, slack_file_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }
    
    // Get file statistics
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(slack_file_result_t));
    result->file_path = strdup_safe(file_path);
    result->file_size = st.st_size;
    
    // Calculate allocated size (rounded up to nearest block/cluster)
    // Simplified implementation - in a real forensic tool, this would be more precise
    filesystem_info_t fs_info;
    if (slack_get_filesystem_info(file_path, &fs_info) == 0) {
        result->allocated_size = ((st.st_size + fs_info.cluster_size - 1) / fs_info.cluster_size) * fs_info.cluster_size;
        result->slack_size = result->allocated_size - st.st_size;
        filesystem_info_free(&fs_info);
    } else {
        // Fallback to default cluster size
        const uint64_t default_cluster_size = 4096;
        result->allocated_size = ((st.st_size + default_cluster_size - 1) / default_cluster_size) * default_cluster_size;
        result->slack_size = result->allocated_size - st.st_size;
    }
    
    // Read slack space data if there is any
    if (result->slack_size > 0) {
        // Limit slack data to a reasonable size for analysis
        size_t read_size = (result->slack_size > 4096) ? 4096 : result->slack_size;
        result->slack_data = (uint8_t*)malloc(read_size);
        if (result->slack_data) {
            FILE* file = fopen(file_path, "rb");
            if (file) {
                // Seek to end of actual file data
                if (fseek(file, st.st_size, SEEK_SET) == 0) {
                    result->slack_data_size = fread(result->slack_data, 1, read_size, file);
                }
                fclose(file);
            }
        }
    }
    
    return 0;
}

int slack_analyze_directory(const char* path, slack_analysis_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    slack_analysis_result_init(result);
    
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
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // Calculate slack space for this file
            slack_file_result_t file_result;
            if (calculate_slack_space(full_path, &file_result) == 0) {
                // Add to results if there's slack space
                if (file_result.slack_size > 0) {
                    slack_analysis_result_add_file(result, &file_result);
                    printf("File %s has %lu bytes of slack space\n", 
                           file_result.file_path, (unsigned long)file_result.slack_size);
                }
                
                // Clean up
                free(file_result.file_path);
                free(file_result.slack_data);
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    return 0;
}

int slack_analyze_unallocated(const char* device_path, unallocated_analysis_result_t* result) {
    if (!device_path || !result) {
        return -1;
    }
    
    unallocated_analysis_result_init(result);
    
    // In a real implementation, this would read unallocated clusters from the device
    // For this example, we'll simulate finding some unallocated clusters
    
    printf("Analyzing unallocated space on %s...\n", device_path);
    
    // Simulate finding 5 unallocated clusters
    for (int i = 0; i < 5; i++) {
        unallocated_cluster_result_t cluster_result;
        cluster_result.cluster_number = 1000 + i;
        cluster_result.cluster_offset = (1000 + i) * 4096;
        cluster_result.cluster_size = 4096;
        cluster_result.cluster_data = (uint8_t*)malloc(4096);
        cluster_result.cluster_data_size = 4096;
        
        if (cluster_result.cluster_data) {
            // Fill with random data for simulation
            for (int j = 0; j < 4096; j++) {
                cluster_result.cluster_data[j] = (uint8_t)(rand() % 256);
            }
            
            // Add to results
            unallocated_analysis_result_add_cluster(result, &cluster_result);
            free(cluster_result.cluster_data);
        }
    }
    
    printf("Found %lu unallocated clusters\n", (unsigned long)result->cluster_count);
    
    return 0;
}

int slack_search_slack(const slack_analysis_result_t* result, const char* keyword, int case_sensitive) {
    if (!result || !keyword) {
        return -1;
    }
    
    int total_matches = 0;
    
    // Search each file's slack space
    for (size_t i = 0; i < result->file_count; i++) {
        const slack_file_result_t* file = &result->files[i];
        
        if (!file->slack_data || file->slack_data_size == 0) {
            continue;
        }
        
        // Search for keyword in slack data
        size_t keyword_len = strlen(keyword);
        if (keyword_len > file->slack_data_size) {
            continue;
        }
        
        int file_matches = 0;
        for (size_t j = 0; j <= file->slack_data_size - keyword_len; j++) {
            int match = 1;
            
            for (size_t k = 0; k < keyword_len; k++) {
                if (case_sensitive) {
                    if (file->slack_data[j + k] != keyword[k]) {
                        match = 0;
                        break;
                    }
                } else {
                    if (tolower(file->slack_data[j + k]) != tolower(keyword[k])) {
                        match = 0;
                        break;
                    }
                }
            }
            
            if (match) {
                file_matches++;
                total_matches++;
                printf("Found keyword '%s' in slack space of file %s at offset %lu\n",
                       keyword, file->file_path, (unsigned long)j);
            }
        }
        
        if (file_matches > 0) {
            printf("File %s: %d matches\n", file->file_path, file_matches);
        }
    }
    
    return total_matches;
}

int slack_search_unallocated(const unallocated_analysis_result_t* result, const char* keyword, int case_sensitive) {
    if (!result || !keyword) {
        return -1;
    }
    
    int total_matches = 0;
    
    // Search each unallocated cluster
    for (size_t i = 0; i < result->cluster_count; i++) {
        const unallocated_cluster_result_t* cluster = &result->clusters[i];
        
        if (!cluster->cluster_data || cluster->cluster_data_size == 0) {
            continue;
        }
        
        // Search for keyword in cluster data
        size_t keyword_len = strlen(keyword);
        if (keyword_len > cluster->cluster_data_size) {
            continue;
        }
        
        int cluster_matches = 0;
        for (size_t j = 0; j <= cluster->cluster_data_size - keyword_len; j++) {
            int match = 1;
            
            for (size_t k = 0; k < keyword_len; k++) {
                if (case_sensitive) {
                    if (cluster->cluster_data[j + k] != keyword[k]) {
                        match = 0;
                        break;
                    }
                } else {
                    if (tolower(cluster->cluster_data[j + k]) != tolower(keyword[k])) {
                        match = 0;
                        break;
                    }
                }
            }
            
            if (match) {
                cluster_matches++;
                total_matches++;
                printf("Found keyword '%s' in unallocated cluster %lu at offset %lu\n",
                       keyword, (unsigned long)cluster->cluster_number, (unsigned long)j);
            }
        }
        
        if (cluster_matches > 0) {
            printf("Cluster %lu: %d matches\n", (unsigned long)cluster->cluster_number, cluster_matches);
        }
    }
    
    return total_matches;
}