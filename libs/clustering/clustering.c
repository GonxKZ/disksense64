#include "clustering.h"
#include "algorithms.h"
#include "distance.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include "../fuzzyhash/fuzzy_hash.h"

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

void clustering_result_init(clustering_result_t* result) {
    if (result) {
        result->clusters = NULL;
        result->cluster_count = 0;
        result->capacity = 0;
        result->total_files = 0;
    }
}

int clustering_result_add_cluster(clustering_result_t* result, const cluster_t* cluster) {
    if (!result || !cluster) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->cluster_count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        cluster_t* new_clusters = (cluster_t*)realloc(result->clusters, new_capacity * sizeof(cluster_t));
        if (!new_clusters) {
            return -1;
        }
        result->clusters = new_clusters;
        result->capacity = new_capacity;
    }
    
    // Copy cluster data
    cluster_t* new_cluster = &result->clusters[result->cluster_count];
    new_cluster->files = NULL;
    new_cluster->file_count = 0;
    new_cluster->capacity = 0;
    new_cluster->cluster_id = cluster->cluster_id;
    memcpy(new_cluster->centroid, cluster->centroid, sizeof(cluster->centroid));
    new_cluster->radius = cluster->radius;
    
    // Copy files
    for (size_t i = 0; i < cluster->file_count; i++) {
        cluster_add_file(new_cluster, &cluster->files[i]);
    }
    
    result->cluster_count++;
    result->total_files += cluster->file_count;
    
    return 0;
}

void clustering_result_free(clustering_result_t* result) {
    if (result) {
        if (result->clusters) {
            for (size_t i = 0; i < result->cluster_count; i++) {
                cluster_free(&result->clusters[i]);
            }
            free(result->clusters);
        }
        result->clusters = NULL;
        result->cluster_count = 0;
        result->capacity = 0;
        result->total_files = 0;
    }
}

void cluster_init(cluster_t* cluster) {
    if (cluster) {
        cluster->files = NULL;
        cluster->file_count = 0;
        cluster->capacity = 0;
        cluster->cluster_id = 0;
        memset(cluster->centroid, 0, sizeof(cluster->centroid));
        cluster->radius = 0.0;
    }
}

int cluster_add_file(cluster_t* cluster, const file_feature_t* file) {
    if (!cluster || !file) {
        return -1;
    }
    
    // Reallocate if needed
    if (cluster->file_count >= cluster->capacity) {
        size_t new_capacity = (cluster->capacity == 0) ? 16 : cluster->capacity * 2;
        file_feature_t* new_files = (file_feature_t*)realloc(cluster->files, new_capacity * sizeof(file_feature_t));
        if (!new_files) {
            return -1;
        }
        cluster->files = new_files;
        cluster->capacity = new_capacity;
    }
    
    // Copy file data
    file_feature_t* new_file = &cluster->files[cluster->file_count];
    new_file->file_path = strdup_safe(file->file_path);
    new_file->file_hash = file->file_hash;
    new_file->fuzzy_hash = file->fuzzy_hash;
    new_file->file_size = file->file_size;
    new_file->file_type = strdup_safe(file->file_type);
    new_file->creation_time = file->creation_time;
    new_file->modification_time = file->modification_time;
    new_file->feature_count = file->feature_count;
    
    if (file->feature_count > 0) {
        new_file->features = (double*)memdup(file->features, file->feature_count * sizeof(double));
    } else {
        new_file->features = NULL;
    }
    
    if ((!new_file->file_path || !new_file->file_type || 
         (file->feature_count > 0 && !new_file->features)) && 
        (file->file_path || file->file_type || 
         (file->feature_count > 0 && file->features))) {
        // Clean up on failure
        free(new_file->file_path);
        free(new_file->file_type);
        free(new_file->features);
        return -1;
    }
    
    cluster->file_count++;
    return 0;
}

void cluster_free(cluster_t* cluster) {
    if (cluster) {
        if (cluster->files) {
            for (size_t i = 0; i < cluster->file_count; i++) {
                file_feature_free(&cluster->files[i]);
            }
            free(cluster->files);
        }
        cluster->files = NULL;
        cluster->file_count = 0;
        cluster->capacity = 0;
    }
}

void clustering_options_init(clustering_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(clustering_options_t));
        options->algorithm = CLUSTERING_KMEANS;
        options->max_clusters = 10;
        options->tolerance = 0.1;
        options->use_fuzzy_hash = 1;
        options->use_file_size = 1;
        options->use_file_type = 1;
        options->use_timestamps = 1;
    }
}

void clustering_options_free(clustering_options_t* options) {
    if (options) {
        if (options->exclude_patterns) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_patterns[i]);
            }
            free(options->exclude_patterns);
        }
        memset(options, 0, sizeof(clustering_options_t));
    }
}

void file_feature_init(file_feature_t* feature) {
    if (feature) {
        memset(feature, 0, sizeof(file_feature_t));
    }
}

void file_feature_free(file_feature_t* feature) {
    if (feature) {
        free(feature->file_path);
        free(feature->file_type);
        free(feature->features);
        memset(feature, 0, sizeof(file_feature_t));
    }
}

int clustering_extract_features(const char* file_path, file_feature_t* feature) {
    if (!file_path || !feature) {
        return -1;
    }
    
    file_feature_init(feature);
    
    // Get file statistics
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }
    
    // Fill basic features
    feature->file_path = strdup_safe(file_path);
    feature->file_size = st.st_size;
    feature->creation_time = st.st_ctime;
    feature->modification_time = st.st_mtime;
    
    // Determine file type from extension
    const char* ext = strrchr(file_path, '.');
    if (ext) {
        feature->file_type = strdup_safe(ext + 1);
    } else {
        feature->file_type = strdup_safe("unknown");
    }
    
    // Compute simple hash (simplified)
    feature->file_hash = st.st_size ^ st.st_mtime;
    
    // Compute fuzzy hash
    fuzzy_hash_result_t fuzzy_result;
    if (fuzzy_hash_file(file_path, FUZZY_HASH_SSDEEP, &fuzzy_result) == 0) {
        // For simplicity, we'll use the hash length as the fuzzy hash value
        feature->fuzzy_hash = fuzzy_result.hash_length;
        fuzzy_hash_free(&fuzzy_result);
    } else {
        feature->fuzzy_hash = 0;
    }
    
    // Allocate space for additional features
    feature->feature_count = 8;
    feature->features = (double*)malloc(feature->feature_count * sizeof(double));
    if (feature->features) {
        // Fill with normalized features
        feature->features[0] = (double)feature->file_size / (1024.0 * 1024.0); // Size in MB
        feature->features[1] = (double)feature->file_hash / 1000000.0; // Normalized hash
        feature->features[2] = (double)feature->fuzzy_hash / 1000.0; // Normalized fuzzy hash
        feature->features[3] = (double)feature->creation_time / (24.0 * 3600.0 * 365.0); // Years since 1970
        feature->features[4] = (double)feature->modification_time / (24.0 * 3600.0 * 365.0); // Years since 1970
        feature->features[5] = (double)strlen(feature->file_type); // Length of file type
        feature->features[6] = 0.0; // Placeholder
        feature->features[7] = 0.0; // Placeholder
    }
    
    return 0;
}

int clustering_extract_directory_features(const char* directory_path, 
                                        file_feature_t** features, 
                                        size_t* count,
                                        const clustering_options_t* options) {
    if (!directory_path || !features || !count) {
        return -1;
    }
    
    *features = NULL;
    *count = 0;
    
    // Open directory
    DIR* dir = opendir(directory_path);
    if (!dir) {
        return -1;
    }
    
    // Count files first
    size_t file_count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check exclude patterns
        int excluded = 0;
        if (options && options->exclude_patterns && options->exclude_count > 0) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                if (strstr(entry->d_name, options->exclude_patterns[i]) != NULL) {
                    excluded = 1;
                    break;
                }
            }
        }
        
        if (excluded) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            file_count++;
        }
        
        free(full_path);
    }
    
    // Allocate features array
    *features = (file_feature_t*)malloc(file_count * sizeof(file_feature_t));
    if (!*features) {
        closedir(dir);
        return -1;
    }
    
    // Extract features for each file
    rewinddir(dir);
    size_t index = 0;
    
    while ((entry = readdir(dir)) != NULL && index < file_count) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check exclude patterns
        int excluded = 0;
        if (options && options->exclude_patterns && options->exclude_count > 0) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                if (strstr(entry->d_name, options->exclude_patterns[i]) != NULL) {
                    excluded = 1;
                    break;
                }
            }
        }
        
        if (excluded) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // Extract features
            if (clustering_extract_features(full_path, &(*features)[index]) == 0) {
                index++;
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    *count = index;
    
    return 0;
}

int clustering_perform_clustering(const file_feature_t* features, 
                                size_t count,
                                const clustering_options_t* options,
                                clustering_result_t* result) {
    if (!features || count == 0 || !result) {
        return -1;
    }
    
    clustering_result_init(result);
    
    // Select algorithm based on options
    switch (options ? options->algorithm : CLUSTERING_KMEANS) {
        case CLUSTERING_KMEANS:
            return clustering_kmeans(features, count, options, result);
        case CLUSTERING_HIERARCHICAL:
            return clustering_hierarchical(features, count, options, result);
        case CLUSTERING_DBSCAN:
            return clustering_dbscan(features, count, options, result);
        case CLUSTERING_AFFINITY_PROPAGATION:
            return clustering_affinity_propagation(features, count, options, result);
        default:
            return clustering_kmeans(features, count, options, result);
    }
}

int clustering_get_cluster_statistics(const cluster_t* cluster, double* avg_similarity) {
    if (!cluster || !avg_similarity) {
        return -1;
    }
    
    if (cluster->file_count < 2) {
        *avg_similarity = 1.0; // Single file is 100% similar to itself
        return 0;
    }
    
    // Calculate average similarity between all pairs of files in the cluster
    double total_similarity = 0.0;
    size_t pair_count = 0;
    
    for (size_t i = 0; i < cluster->file_count; i++) {
        for (size_t j = i + 1; j < cluster->file_count; j++) {
            // Calculate similarity between files i and j
            double similarity = clustering_calculate_similarity(&cluster->files[i], &cluster->files[j]);
            total_similarity += similarity;
            pair_count++;
        }
    }
    
    *avg_similarity = (pair_count > 0) ? (total_similarity / pair_count) : 0.0;
    return 0;
}

int clustering_find_similar_files(const file_feature_t* features,
                                size_t count,
                                const file_feature_t* target_file,
                                file_feature_t** similar_files,
                                size_t* similar_count,
                                double threshold) {
    if (!features || count == 0 || !target_file || !similar_files || !similar_count) {
        return -1;
    }
    
    *similar_files = NULL;
    *similar_count = 0;
    
    // Allocate initial space for similar files
    size_t capacity = 16;
    *similar_files = (file_feature_t*)malloc(capacity * sizeof(file_feature_t));
    if (!*similar_files) {
        return -1;
    }
    
    // Find files with similarity above threshold
    for (size_t i = 0; i < count; i++) {
        double similarity = clustering_calculate_similarity(target_file, &features[i]);
        
        if (similarity >= threshold) {
            // Reallocate if needed
            if (*similar_count >= capacity) {
                capacity *= 2;
                file_feature_t* new_files = (file_feature_t*)realloc(*similar_files, capacity * sizeof(file_feature_t));
                if (!new_files) {
                    // Clean up and return error
                    for (size_t j = 0; j < *similar_count; j++) {
                        file_feature_free(&(*similar_files)[j]);
                    }
                    free(*similar_files);
                    *similar_files = NULL;
                    *similar_count = 0;
                    return -1;
                }
                *similar_files = new_files;
            }
            
            // Copy the similar file
            file_feature_init(&(*similar_files)[*similar_count]);
            (*similar_files)[*similar_count].file_path = strdup_safe(features[i].file_path);
            (*similar_files)[*similar_count].file_hash = features[i].file_hash;
            (*similar_files)[*similar_count].fuzzy_hash = features[i].fuzzy_hash;
            (*similar_files)[*similar_count].file_size = features[i].file_size;
            (*similar_files)[*similar_count].file_type = strdup_safe(features[i].file_type);
            (*similar_files)[*similar_count].creation_time = features[i].creation_time;
            (*similar_files)[*similar_count].modification_time = features[i].modification_time;
            (*similar_files)[*similar_count].feature_count = features[i].feature_count;
            
            if (features[i].feature_count > 0) {
                (*similar_files)[*similar_count].features = (double*)memdup(features[i].features, 
                                                                           features[i].feature_count * sizeof(double));
            } else {
                (*similar_files)[*similar_count].features = NULL;
            }
            
            (*similar_count)++;
        }
    }
    
    return 0;
}

int clustering_add_exclude_pattern(clustering_options_t* options, const char* pattern) {
    if (!options || !pattern) {
        return -1;
    }
    
    // Reallocate exclude patterns array
    char** new_patterns = (char**)realloc(options->exclude_patterns, (options->exclude_count + 1) * sizeof(char*));
    if (!new_patterns) {
        return -1;
    }
    
    options->exclude_patterns = new_patterns;
    options->exclude_patterns[options->exclude_count] = strdup_safe(pattern);
    
    if (!options->exclude_patterns[options->exclude_count]) {
        return -1;
    }
    
    options->exclude_count++;
    return 0;
}

void clustering_clear_exclude_patterns(clustering_options_t* options) {
    if (options) {
        if (options->exclude_patterns) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_patterns[i]);
            }
            free(options->exclude_patterns);
            options->exclude_patterns = NULL;
            options->exclude_count = 0;
        }
    }
}

const char* clustering_get_algorithm_name(clustering_algorithm_t algorithm) {
    switch (algorithm) {
        case CLUSTERING_KMEANS:
            return "K-Means";
        case CLUSTERING_HIERARCHICAL:
            return "Hierarchical";
        case CLUSTERING_DBSCAN:
            return "DBSCAN";
        case CLUSTERING_AFFINITY_PROPAGATION:
            return "Affinity Propagation";
        default:
            return "Unknown";
    }
}