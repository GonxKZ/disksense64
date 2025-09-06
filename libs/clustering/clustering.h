#ifndef LIBS_CLUSTERING_CLUSTERING_H
#define LIBS_CLUSTERING_CLUSTERING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// File feature structure
typedef struct {
    char* file_path;
    uint64_t file_hash;        // Simple hash for quick comparison
    uint64_t fuzzy_hash;       // Fuzzy hash for similarity detection
    size_t file_size;
    char* file_type;
    uint64_t creation_time;
    uint64_t modification_time;
    double* features;          // Additional numerical features
    size_t feature_count;
} file_feature_t;

// Cluster structure
typedef struct {
    file_feature_t* files;
    size_t file_count;
    size_t capacity;
    uint64_t cluster_id;
    double centroid[8];        // Centroid coordinates (simplified)
    double radius;             // Cluster radius
} cluster_t;

// Clustering result
typedef struct {
    cluster_t* clusters;
    size_t cluster_count;
    size_t capacity;
    size_t total_files;
} clustering_result_t;

// Clustering options
typedef struct {
    int algorithm;             // Clustering algorithm to use
    size_t max_clusters;       // Maximum number of clusters
    double tolerance;          // Clustering tolerance
    int use_fuzzy_hash;        // Use fuzzy hashing for similarity
    int use_file_size;         // Use file size for similarity
    int use_file_type;         // Use file type for similarity
    int use_timestamps;        // Use timestamps for similarity
    char** exclude_patterns;   // File patterns to exclude
    size_t exclude_count;      // Number of exclude patterns
} clustering_options_t;

// Clustering algorithms
typedef enum {
    CLUSTERING_KMEANS = 0,
    CLUSTERING_HIERARCHICAL = 1,
    CLUSTERING_DBSCAN = 2,
    CLUSTERING_AFFINITY_PROPAGATION = 3
} clustering_algorithm_t;

// Initialize clustering result
void clustering_result_init(clustering_result_t* result);

// Add a cluster to the result
int clustering_result_add_cluster(clustering_result_t* result, const cluster_t* cluster);

// Free clustering result
void clustering_result_free(clustering_result_t* result);

// Initialize cluster
void cluster_init(cluster_t* cluster);

// Add a file to the cluster
int cluster_add_file(cluster_t* cluster, const file_feature_t* file);

// Free cluster
void cluster_free(cluster_t* cluster);

// Initialize clustering options with default values
void clustering_options_init(clustering_options_t* options);

// Free clustering options
void clustering_options_free(clustering_options_t* options);

// Initialize file feature
void file_feature_init(file_feature_t* feature);

// Free file feature
void file_feature_free(file_feature_t* feature);

// Extract features from a file
// file_path: path to the file
// feature: output file feature (must be freed with file_feature_free)
// Returns 0 on success, non-zero on error
int clustering_extract_features(const char* file_path, file_feature_t* feature);

// Extract features from all files in a directory
// directory_path: path to the directory
// features: output array of file features (must be freed)
// count: output number of features
// options: clustering options
// Returns 0 on success, non-zero on error
int clustering_extract_directory_features(const char* directory_path, 
                                        file_feature_t** features, 
                                        size_t* count,
                                        const clustering_options_t* options);

// Perform clustering on file features
// features: array of file features
// count: number of features
// options: clustering options
// result: output clustering result (must be freed with clustering_result_free)
// Returns 0 on success, non-zero on error
int clustering_perform_clustering(const file_feature_t* features, 
                                size_t count,
                                const clustering_options_t* options,
                                clustering_result_t* result);

// Get cluster statistics
// cluster: cluster to analyze
// avg_similarity: output average similarity within cluster
// Returns 0 on success, non-zero on error
int clustering_get_cluster_statistics(const cluster_t* cluster, double* avg_similarity);

// Find similar files to a given file
// features: array of file features
// count: number of features
// target_file: file to find similarities for
// similar_files: output array of similar files (must be freed)
// similar_count: output number of similar files
// threshold: similarity threshold (0.0-1.0)
// Returns 0 on success, non-zero on error
int clustering_find_similar_files(const file_feature_t* features,
                                size_t count,
                                const file_feature_t* target_file,
                                file_feature_t** similar_files,
                                size_t* similar_count,
                                double threshold);

// Add exclude pattern
// options: clustering options
// pattern: pattern to exclude
// Returns 0 on success, non-zero on error
int clustering_add_exclude_pattern(clustering_options_t* options, const char* pattern);

// Clear all exclude patterns
// options: clustering options
void clustering_clear_exclude_patterns(clustering_options_t* options);

// Get algorithm name
// algorithm: clustering algorithm
// Returns name of the algorithm
const char* clustering_get_algorithm_name(clustering_algorithm_t algorithm);

#ifdef __cplusplus
}
#endif

#endif // LIBS_CLUSTERING_CLUSTERING_H