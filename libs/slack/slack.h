#ifndef LIBS_SLACK_SLACK_H
#define LIBS_SLACK_SLACK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Slack space analysis result for a file
typedef struct {
    char* file_path;
    uint64_t file_size;
    uint64_t allocated_size;
    uint64_t slack_size;
    uint8_t* slack_data;
    size_t slack_data_size;
} slack_file_result_t;

// Unallocated space analysis result
typedef struct {
    uint64_t cluster_number;
    uint64_t cluster_offset;
    uint64_t cluster_size;
    uint8_t* cluster_data;
    size_t cluster_data_size;
} unallocated_cluster_result_t;

// Slack space analysis result
typedef struct {
    slack_file_result_t* files;
    size_t file_count;
    size_t file_capacity;
} slack_analysis_result_t;

// Unallocated space analysis result
typedef struct {
    unallocated_cluster_result_t* clusters;
    size_t cluster_count;
    size_t cluster_capacity;
} unallocated_analysis_result_t;

// Filesystem information
typedef struct {
    char* filesystem_type;
    uint64_t block_size;
    uint64_t cluster_size;
    uint64_t total_clusters;
    uint64_t free_clusters;
    uint64_t used_clusters;
} filesystem_info_t;

// Initialize slack analysis result
void slack_analysis_result_init(slack_analysis_result_t* result);

// Add a slack file result to the analysis result
int slack_analysis_result_add_file(slack_analysis_result_t* result, const slack_file_result_t* file);

// Free slack analysis result
void slack_analysis_result_free(slack_analysis_result_t* result);

// Initialize unallocated analysis result
void unallocated_analysis_result_init(unallocated_analysis_result_t* result);

// Add an unallocated cluster result to the analysis result
int unallocated_analysis_result_add_cluster(unallocated_analysis_result_t* result, const unallocated_cluster_result_t* cluster);

// Free unallocated analysis result
void unallocated_analysis_result_free(unallocated_analysis_result_t* result);

// Get filesystem information
// path: path to a file or directory on the filesystem
// info: output filesystem information (must be freed with filesystem_info_free)
// Returns 0 on success, non-zero on error
int slack_get_filesystem_info(const char* path, filesystem_info_t* info);

// Free filesystem information
void filesystem_info_free(filesystem_info_t* info);

// Analyze slack space for files in a directory
// path: path to the directory
// result: output analysis result (must be freed with slack_analysis_result_free)
// Returns 0 on success, non-zero on error
int slack_analyze_directory(const char* path, slack_analysis_result_t* result);

// Analyze unallocated space on a filesystem
// device_path: path to the device or filesystem
// result: output analysis result (must be freed with unallocated_analysis_result_free)
// Returns 0 on success, non-zero on error
int slack_analyze_unallocated(const char* device_path, unallocated_analysis_result_t* result);

// Search for keywords in slack space
// result: slack analysis result to search
// keyword: keyword to search for
// case_sensitive: 1 for case sensitive, 0 for case insensitive
// Returns number of matches found, negative on error
int slack_search_slack(const slack_analysis_result_t* result, const char* keyword, int case_sensitive);

// Search for keywords in unallocated space
// result: unallocated analysis result to search
// keyword: keyword to search for
// case_sensitive: 1 for case sensitive, 0 for case insensitive
// Returns number of matches found, negative on error
int slack_search_unallocated(const unallocated_analysis_result_t* result, const char* keyword, int case_sensitive);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SLACK_SLACK_H