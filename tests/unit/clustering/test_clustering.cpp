#include "libs/clustering/clustering.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <cerrno>

// Create test files with different characteristics
static int create_test_files(const char* test_dir) {
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    // Create text files
    for (int i = 0; i < 5; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/text_file_%d.txt", test_dir, i);
        int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            char content[256];
            snprintf(content, sizeof(content), "This is text file %d with some content.", i);
            write(fd, content, strlen(content));
            close(fd);
        }
    }
    
    // Create image files
    for (int i = 0; i < 3; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/image_file_%d.jpg", test_dir, i);
        int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            char content[256];
            snprintf(content, sizeof(content), "This simulates JPEG image data %d.", i);
            write(fd, content, strlen(content));
            close(fd);
        }
    }
    
    // Create PDF files
    for (int i = 0; i < 2; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/document_%d.pdf", test_dir, i);
        int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            char content[256];
            snprintf(content, sizeof(content), "This simulates PDF document %d.", i);
            write(fd, content, strlen(content));
            close(fd);
        }
    }
    
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* test_dir) {
    DIR* dir = opendir(test_dir);
    if (!dir) {
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", test_dir, entry->d_name);
        unlink(filepath);
    }
    
    closedir(dir);
    rmdir(test_dir);
}

int test_feature_extraction() {
    printf("Testing file feature extraction...\n");
    
    const char* test_dir = "/tmp/clustering_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Extract features from directory
    file_feature_t* features;
    size_t count;
    clustering_options_t options;
    clustering_options_init(&options);
    
    int ret = clustering_extract_directory_features(test_dir, &features, &count, &options);
    
    if (ret != 0) {
        printf("Failed to extract directory features\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("Extracted features from %lu files:\n", (unsigned long)count);
    
    for (size_t i = 0; i < count && i < 5; i++) { // Show first 5
        file_feature_t* feature = &features[i];
        printf("  File %lu: %s\n", (unsigned long)i, feature->file_path ? feature->file_path : "unknown");
        printf("    Size: %lu bytes\n", (unsigned long)feature->file_size);
        printf("    Type: %s\n", feature->file_type ? feature->file_type : "unknown");
        printf("    Hash: %lu\n", (unsigned long)feature->file_hash);
        printf("    Fuzzy hash: %lu\n", (unsigned long)feature->fuzzy_hash);
        printf("    Creation time: %lu\n", (unsigned long)feature->creation_time);
        printf("    Modification time: %lu\n", (unsigned long)feature->modification_time);
        printf("    Features: %lu\n", (unsigned long)feature->feature_count);
    }
    
    if (count > 5) {
        printf("  ... and %lu more files\n", (unsigned long)(count - 5));
    }
    
    // Clean up
    for (size_t i = 0; i < count; i++) {
        file_feature_free(&features[i]);
    }
    free(features);
    cleanup_test_environment(test_dir);
    
    printf("File feature extraction test passed!\n");
    return 0;
}

int test_kmeans_clustering() {
    printf("Testing K-Means clustering...\n");
    
    const char* test_dir = "/tmp/clustering_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Extract features
    file_feature_t* features;
    size_t count;
    clustering_options_t options;
    clustering_options_init(&options);
    
    int ret = clustering_extract_directory_features(test_dir, &features, &count, &options);
    
    if (ret != 0) {
        printf("Failed to extract directory features\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    // Perform K-Means clustering
    clustering_result_t result;
    options.algorithm = CLUSTERING_KMEANS;
    options.max_clusters = 3;
    
    ret = clustering_perform_clustering(features, count, &options, &result);
    
    if (ret != 0) {
        printf("Failed to perform K-Means clustering\n");
        for (size_t i = 0; i < count; i++) {
            file_feature_free(&features[i]);
        }
        free(features);
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("K-Means clustering result (%lu clusters):\n", (unsigned long)result.cluster_count);
    
    for (size_t i = 0; i < result.cluster_count; i++) {
        cluster_t* cluster = &result.clusters[i];
        printf("  Cluster %lu (ID: %lu): %lu files\n", 
               (unsigned long)i, (unsigned long)cluster->cluster_id, (unsigned long)cluster->file_count);
        
        for (size_t j = 0; j < cluster->file_count && j < 3; j++) {
            file_feature_t* file = &cluster->files[j];
            printf("    File %lu: %s (%s)\n", 
                   (unsigned long)j, 
                   file->file_path ? strrchr(file->file_path, '/') + 1 : "unknown",
                   file->file_type ? file->file_type : "unknown");
        }
        
        if (cluster->file_count > 3) {
            printf("    ... and %lu more files\n", (unsigned long)(cluster->file_count - 3));
        }
        
        // Get cluster statistics
        double avg_similarity;
        if (clustering_get_cluster_statistics(cluster, &avg_similarity) == 0) {
            printf("    Average similarity: %.2f\n", avg_similarity);
        }
    }
    
    // Clean up
    clustering_result_free(&result);
    for (size_t i = 0; i < count; i++) {
        file_feature_free(&features[i]);
    }
    free(features);
    cleanup_test_environment(test_dir);
    
    printf("K-Means clustering test passed!\n");
    return 0;
}

int test_hierarchical_clustering() {
    printf("Testing Hierarchical clustering...\n");
    
    const char* test_dir = "/tmp/clustering_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Extract features
    file_feature_t* features;
    size_t count;
    clustering_options_t options;
    clustering_options_init(&options);
    
    int ret = clustering_extract_directory_features(test_dir, &features, &count, &options);
    
    if (ret != 0) {
        printf("Failed to extract directory features\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    // Perform Hierarchical clustering
    clustering_result_t result;
    options.algorithm = CLUSTERING_HIERARCHICAL;
    options.max_clusters = 3;
    
    ret = clustering_perform_clustering(features, count, &options, &result);
    
    if (ret != 0) {
        printf("Failed to perform Hierarchical clustering\n");
        for (size_t i = 0; i < count; i++) {
            file_feature_free(&features[i]);
        }
        free(features);
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("Hierarchical clustering result (%lu clusters):\n", (unsigned long)result.cluster_count);
    
    for (size_t i = 0; i < result.cluster_count; i++) {
        cluster_t* cluster = &result.clusters[i];
        printf("  Cluster %lu (ID: %lu): %lu files\n", 
               (unsigned long)i, (unsigned long)cluster->cluster_id, (unsigned long)cluster->file_count);
    }
    
    // Clean up
    clustering_result_free(&result);
    for (size_t i = 0; i < count; i++) {
        file_feature_free(&features[i]);
    }
    free(features);
    cleanup_test_environment(test_dir);
    
    printf("Hierarchical clustering test passed!\n");
    return 0;
}

int test_dbscan_clustering() {
    printf("Testing DBSCAN clustering...\n");
    
    const char* test_dir = "/tmp/clustering_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Extract features
    file_feature_t* features;
    size_t count;
    clustering_options_t options;
    clustering_options_init(&options);
    
    int ret = clustering_extract_directory_features(test_dir, &features, &count, &options);
    
    if (ret != 0) {
        printf("Failed to extract directory features\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    // Perform DBSCAN clustering
    clustering_result_t result;
    options.algorithm = CLUSTERING_DBSCAN;
    options.tolerance = 0.2;
    
    ret = clustering_perform_clustering(features, count, &options, &result);
    
    if (ret != 0) {
        printf("Failed to perform DBSCAN clustering\n");
        for (size_t i = 0; i < count; i++) {
            file_feature_free(&features[i]);
        }
        free(features);
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    printf("DBSCAN clustering result (%lu clusters):\n", (unsigned long)result.cluster_count);
    
    for (size_t i = 0; i < result.cluster_count; i++) {
        cluster_t* cluster = &result.clusters[i];
        printf("  Cluster %lu (ID: %lu): %lu files\n", 
               (unsigned long)i, (unsigned long)cluster->cluster_id, (unsigned long)cluster->file_count);
    }
    
    // Clean up
    clustering_result_free(&result);
    for (size_t i = 0; i < count; i++) {
        file_feature_free(&features[i]);
    }
    free(features);
    cleanup_test_environment(test_dir);
    
    printf("DBSCAN clustering test passed!\n");
    return 0;
}

int test_similarity_search() {
    printf("Testing similarity search...\n");
    
    const char* test_dir = "/tmp/clustering_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Extract features
    file_feature_t* features;
    size_t count;
    clustering_options_t options;
    clustering_options_init(&options);
    
    int ret = clustering_extract_directory_features(test_dir, &features, &count, &options);
    
    if (ret != 0) {
        printf("Failed to extract directory features\n");
        cleanup_test_environment(test_dir);
        return 1;
    }
    
    // Find similar files to the first file
    if (count > 0) {
        file_feature_t* similar_files;
        size_t similar_count;
        
        ret = clustering_find_similar_files(features, count, &features[0], &similar_files, &similar_count, 0.3);
        
        if (ret != 0) {
            printf("Failed to find similar files\n");
            for (size_t i = 0; i < count; i++) {
                file_feature_free(&features[i]);
            }
            free(features);
            cleanup_test_environment(test_dir);
            return 1;
        }
        
        printf("Files similar to %s (threshold 0.3):\n", 
               features[0].file_path ? strrchr(features[0].file_path, '/') + 1 : "unknown");
        
        for (size_t i = 0; i < similar_count; i++) {
            printf("  Similar file %lu: %s (%s)\n", 
                   (unsigned long)i,
                   similar_files[i].file_path ? strrchr(similar_files[i].file_path, '/') + 1 : "unknown",
                   similar_files[i].file_type ? similar_files[i].file_type : "unknown");
        }
        
        // Clean up similar files
        for (size_t i = 0; i < similar_count; i++) {
            file_feature_free(&similar_files[i]);
        }
        free(similar_files);
    }
    
    // Clean up
    for (size_t i = 0; i < count; i++) {
        file_feature_free(&features[i]);
    }
    free(features);
    cleanup_test_environment(test_dir);
    
    printf("Similarity search test passed!\n");
    return 0;
}

int test_algorithm_names() {
    printf("Testing algorithm name retrieval...\n");
    
    clustering_algorithm_t algorithms[] = {
        CLUSTERING_KMEANS,
        CLUSTERING_HIERARCHICAL,
        CLUSTERING_DBSCAN,
        CLUSTERING_AFFINITY_PROPAGATION
    };
    
    const char* expected_names[] = {
        "K-Means",
        "Hierarchical",
        "DBSCAN",
        "Affinity Propagation"
    };
    
    for (int i = 0; i < 4; i++) {
        const char* name = clustering_get_algorithm_name(algorithms[i]);
        printf("Algorithm %d: %s\n", i, name);
        
        if (strcmp(name, expected_names[i]) != 0) {
            printf("Error: Expected '%s', got '%s'\n", expected_names[i], name);
            return 1;
        }
    }
    
    // Test unknown algorithm
    const char* unknown_name = clustering_get_algorithm_name((clustering_algorithm_t)999);
    printf("Unknown algorithm: %s\n", unknown_name);
    
    printf("Algorithm name retrieval test passed!\n");
    return 0;
}

int main() {
    srand(time(NULL)); // Initialize random seed
    
    printf("Running file clustering tests...\n");
    
    int result1 = test_feature_extraction();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_kmeans_clustering();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_hierarchical_clustering();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_dbscan_clustering();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_similarity_search();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_algorithm_names();
    if (result6 != 0) {
        return result6;
    }
    
    printf("All file clustering tests passed!\n");
    return 0;
}
