#ifndef LIBS_YARA_YARA_H
#define LIBS_YARA_YARA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// YARA match result
typedef struct {
    char* rule_name;
    char* rule_namespace;
    char* matched_string;
    size_t offset;
    size_t length;
    int severity; // 1-10 scale
} yara_match_t;

// YARA scan result
typedef struct {
    yara_match_t* matches;
    size_t count;
    size_t capacity;
} yara_result_t;

// YARA rule information
typedef struct {
    char* name;
    char* namespace;
    char* description;
    char* author;
    char* version;
    int severity;
} yara_rule_info_t;

// YARA scanner options
typedef struct {
    int timeout_seconds;      // Scan timeout in seconds
    int follow_symlinks;      // Follow symbolic links
    int scan_compressed;      // Scan compressed files
    int scan_archives;        // Scan archive files
    char** exclude_patterns;  // File patterns to exclude
    size_t exclude_count;     // Number of exclude patterns
} yara_options_t;

// Initialize YARA result
void yara_result_init(yara_result_t* result);

// Add a YARA match to the result
int yara_result_add_match(yara_result_t* result, const yara_match_t* match);

// Free YARA result
void yara_result_free(yara_result_t* result);

// Initialize YARA options with default values
void yara_options_init(yara_options_t* options);

// Free YARA options
void yara_options_free(yara_options_t* options);

// Initialize YARA rule info
void yara_rule_info_init(yara_rule_info_t* info);

// Free YARA rule info
void yara_rule_info_free(yara_rule_info_t* info);

// Load YARA rules from file
// rules_path: path to the YARA rules file
// Returns 0 on success, non-zero on error
int yara_load_rules(const char* rules_path);

// Load YARA rules from memory
// data: pointer to YARA rules data
// size: size of YARA rules data in bytes
// Returns 0 on success, non-zero on error
int yara_load_rules_from_memory(const void* data, size_t size);

// Scan file for malware using YARA rules
// file_path: path to the file to scan
// options: scanner options
// result: output scan result (must be freed with yara_result_free)
// Returns 0 on success, non-zero on error
int yara_scan_file(const char* file_path, const yara_options_t* options, yara_result_t* result);

// Scan data in memory for malware using YARA rules
// data: pointer to data to scan
// size: size of data in bytes
// options: scanner options
// result: output scan result (must be freed with yara_result_free)
// Returns 0 on success, non-zero on error
int yara_scan_data(const void* data, size_t size, const yara_options_t* options, yara_result_t* result);

// Scan directory for malware using YARA rules
// directory_path: path to the directory to scan
// options: scanner options
// result: output scan result (must be freed with yara_result_free)
// Returns 0 on success, non-zero on error
int yara_scan_directory(const char* directory_path, const yara_options_t* options, yara_result_t* result);

// Get information about loaded YARA rules
// rules: array of rule information (must be freed with yara_rule_info_free for each element)
// count: output number of rules
// Returns 0 on success, non-zero on error
int yara_get_rule_info(yara_rule_info_t** rules, size_t* count);

// Add exclude pattern
// options: YARA options
// pattern: pattern to exclude
// Returns 0 on success, non-zero on error
int yara_add_exclude_pattern(yara_options_t* options, const char* pattern);

// Clear all exclude patterns
// options: YARA options
void yara_clear_exclude_patterns(yara_options_t* options);

// Get match severity description
// severity: severity level (1-10)
// Returns a description of the severity level
const char* yara_get_severity_description(int severity);

#ifdef __cplusplus
}
#endif

#endif // LIBS_YARA_YARA_H