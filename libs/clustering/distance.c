#include "distance.h"
#include <math.h>
#include <float.h>
#include <string.h>

double clustering_euclidean_distance(const double* a, const double* b, size_t dim) {
    if (!a || !b) {
        return DBL_MAX;
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < dim; i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return sqrt(sum);
}

double clustering_manhattan_distance(const double* a, const double* b, size_t dim) {
    if (!a || !b) {
        return DBL_MAX;
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < dim; i++) {
        sum += fabs(a[i] - b[i]);
    }
    
    return sum;
}

double clustering_cosine_similarity(const double* a, const double* b, size_t dim) {
    if (!a || !b) {
        return 0.0;
    }
    
    double dot_product = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    
    for (size_t i = 0; i < dim; i++) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a == 0.0 || norm_b == 0.0) {
        return 0.0;
    }
    
    return dot_product / (sqrt(norm_a) * sqrt(norm_b));
}

double clustering_calculate_similarity(const file_feature_t* a, const file_feature_t* b) {
    if (!a || !b) {
        return 0.0;
    }
    
    // If the same file, return 1.0
    if (a == b || (a->file_path && b->file_path && strcmp(a->file_path, b->file_path) == 0)) {
        return 1.0;
    }
    
    double similarity = 0.0;
    int components = 0;
    
    // Compare file sizes (normalized)
    if (a->file_size > 0 && b->file_size > 0) {
        double size_diff = fabs((double)a->file_size - (double)b->file_size) / 
                          ((double)a->file_size + (double)b->file_size);
        similarity += (1.0 - size_diff);
        components++;
    }
    
    // Compare file types
    if (a->file_type && b->file_type) {
        if (strcmp(a->file_type, b->file_type) == 0) {
            similarity += 1.0;
        } else {
            similarity += 0.1; // Small similarity for different types
        }
        components++;
    }
    
    // Compare fuzzy hashes
    if (a->fuzzy_hash > 0 && b->fuzzy_hash > 0) {
        // Simple comparison based on hash values
        double hash_diff = fabs((double)a->fuzzy_hash - (double)b->fuzzy_hash) / 
                          ((double)a->fuzzy_hash + (double)b->fuzzy_hash);
        similarity += (1.0 - hash_diff);
        components++;
    }
    
    // Compare timestamps
    if (a->modification_time > 0 && b->modification_time > 0) {
        double time_diff = fabs((double)a->modification_time - (double)b->modification_time) / 
                          (fabs((double)a->modification_time) + fabs((double)b->modification_time) + 1.0);
        similarity += (1.0 - time_diff);
        components++;
    }
    
    // Compare feature vectors using cosine similarity
    if (a->features && b->features && a->feature_count > 0 && b->feature_count > 0) {
        size_t min_dim = (a->feature_count < b->feature_count) ? a->feature_count : b->feature_count;
        double feature_similarity = clustering_cosine_similarity(a->features, b->features, min_dim);
        similarity += feature_similarity;
        components++;
    }
    
    // Return average similarity
    return (components > 0) ? (similarity / components) : 0.0;
}

double clustering_calculate_distance(const file_feature_t* a, const file_feature_t* b) {
    if (!a || !b) {
        return DBL_MAX;
    }
    
    // Distance is 1 - similarity
    return 1.0 - clustering_calculate_similarity(a, b);
}