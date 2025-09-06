// Ensure we define _GNU_SOURCE before any system headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ml_internal.h"

int ml_extract_metadata_features(const char* file_path, double* features, size_t* feature_count) {
    if (!file_path || !features || !feature_count) {
        return -1;
    }
    
    // Extract metadata
    metadata_info_t info;
    if (metadata_get_file_info(file_path, &info) != 0) {
        return -1;
    }
    
    // Extract up to 10 metadata features
    size_t index = 0;
    
    // File size (log normalized)
    if (index < *feature_count) {
        features[index++] = log(info.size + 1) / 20.0;
    }
    
    // Timestamps (normalized to years since 1970)
    if (index < *feature_count) {
        features[index++] = (double)info.creation_time / (365.0 * 24.0 * 3600.0 * 50.0);
    }
    
    if (index < *feature_count) {
        features[index++] = (double)info.last_write_time / (365.0 * 24.0 * 3600.0 * 50.0);
    }
    
    if (index < *feature_count) {
        features[index++] = (double)info.last_access_time / (365.0 * 24.0 * 3600.0 * 50.0);
    }
    
    // Permissions (normalized)
    if (index < *feature_count) {
        features[index++] = (double)info.permissions / 0x1FF;
    }
    
    // File type flags
    if (index < *feature_count) {
        features[index++] = info.is_directory ? 1.0 : 0.0;
    }
    
    if (index < *feature_count) {
        features[index++] = info.is_symlink ? 1.0 : 0.0;
    }
    
    if (index < *feature_count) {
        features[index++] = info.is_hidden ? 1.0 : 0.0;
    }
    
    // Owner and group IDs (normalized)
    if (index < *feature_count) {
        features[index++] = (double)info.owner_id / 1000000.0;
    }
    
    if (index < *feature_count) {
        features[index++] = (double)info.group_id / 1000000.0;
    }
    
    // Update feature count
    *feature_count = index;
    
    // Clean up
    metadata_free_info(&info);
    
    return 0;
}

int ml_extract_hash_features(const char* file_path, double* features, size_t* feature_count) {
    if (!file_path || !features || !feature_count) {
        return -1;
    }
    
    // Extract up to 5 hash-based features
    size_t index = 0;
    
    // For now, we'll use dummy hash features
    // In a real implementation, this would compute actual hashes
    
    for (int i = 0; i < 5 && index < *feature_count; i++) {
        features[index++] = (double)rand() / RAND_MAX;
    }
    
    // Update feature count
    *feature_count = index;
    
    return 0;
}

int ml_extract_content_features(const char* file_path, double* features, size_t* feature_count) {
    if (!file_path || !features || !feature_count) {
        return -1;
    }
    
    // Extract up to 20 content-based features
    size_t index = 0;
    
    // For now, we'll use dummy content features
    // In a real implementation, this would analyze file content
    
    for (int i = 0; i < 20 && index < *feature_count; i++) {
        features[index++] = (double)rand() / RAND_MAX;
    }
    
    // Update feature count
    *feature_count = index;
    
    return 0;
}

int ml_extract_fuzzy_features(const char* file_path, double* features, size_t* feature_count) {
    if (!file_path || !features || !feature_count) {
        return -1;
    }
    
    // Extract up to 3 fuzzy hash features
    size_t index = 0;
    
    // Compute SSDeep hash
    fuzzy_hash_result_t ssdeep_result;
    if (fuzzy_hash_file(file_path, FUZZY_HASH_SSDEEP, &ssdeep_result) == 0) {
        if (index < *feature_count) {
            // Use hash length as feature
            features[index++] = (double)ssdeep_result.hash_length / 1000.0;
        }
        fuzzy_hash_free(&ssdeep_result);
    }
    
    // Compute TLSH hash
    fuzzy_hash_result_t tlsh_result;
    if (fuzzy_hash_file(file_path, FUZZY_HASH_TLSH, &tlsh_result) == 0) {
        if (index < *feature_count) {
            // Use hash length as feature
            features[index++] = (double)tlsh_result.hash_length / 1000.0;
        }
        fuzzy_hash_free(&tlsh_result);
    }
    
    // Add a third fuzzy feature
    if (index < *feature_count) {
        features[index++] = (double)rand() / RAND_MAX;
    }
    
    // Update feature count
    *feature_count = index;
    
    return 0;
}

int ml_normalize_features(double* features, size_t feature_count) {
    if (!features) {
        return -1;
    }
    
    // Simple min-max normalization to [0,1] range
    // In a real implementation, this would be more sophisticated
    
    for (size_t i = 0; i < feature_count; i++) {
        // Clamp values to [0,1] range
        if (features[i] < 0.0) {
            features[i] = 0.0;
        } else if (features[i] > 1.0) {
            features[i] = 1.0;
        }
    }
    
    return 0;
}