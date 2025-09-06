#ifndef LIBS_LOGS_LOGS_H
#define LIBS_LOGS_LOGS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log entry structure
typedef struct {
    time_t timestamp;
    char* source;
    char* level;
    char* message;
    char* host;
    char* process;
    int pid;
} log_entry_t;

// Log analysis result
typedef struct {
    log_entry_t* entries;
    size_t count;
    size_t capacity;
} log_result_t;

// Log filter criteria
typedef struct {
    time_t start_time;
    time_t end_time;
    char* source_filter;
    char* level_filter;
    char* host_filter;
    char* process_filter;
    char* keyword_filter;
    int include_errors;      // Include ERROR/FATAL logs
    int include_warnings;    // Include WARNING logs
    int include_info;        // Include INFO logs
    int include_debug;       // Include DEBUG logs
} log_filter_t;

// Log statistics
typedef struct {
    size_t total_entries;
    size_t error_count;
    size_t warning_count;
    size_t info_count;
    size_t debug_count;
    time_t first_timestamp;
    time_t last_timestamp;
    char** top_sources;
    size_t source_count;
} log_statistics_t;

// Supported log formats
typedef enum {
    LOG_FORMAT_SYSLOG = 0,
    LOG_FORMAT_APACHE = 1,
    LOG_FORMAT_NGINX = 2,
    LOG_FORMAT_JSON = 3,
    LOG_FORMAT_CSV = 4,
    LOG_FORMAT_CUSTOM = 5
} log_format_t;

// Initialize log result
void log_result_init(log_result_t* result);

// Add a log entry to the result
int log_result_add_entry(log_result_t* result, const log_entry_t* entry);

// Free log result
void log_result_free(log_result_t* result);

// Initialize log filter with default values
void log_filter_init(log_filter_t* filter);

// Free log filter
void log_filter_free(log_filter_t* filter);

// Initialize log statistics
void log_statistics_init(log_statistics_t* stats);

// Free log statistics
void log_statistics_free(log_statistics_t* stats);

// Parse log file
// path: path to the log file
// format: log format
// result: output log result (must be freed with log_result_free)
// Returns 0 on success, non-zero on error
int log_parse_file(const char* path, log_format_t format, log_result_t* result);

// Parse log data from memory
// data: pointer to log data
// size: size of log data in bytes
// format: log format
// result: output log result (must be freed with log_result_free)
// Returns 0 on success, non-zero on error
int log_parse_data(const void* data, size_t size, log_format_t format, log_result_t* result);

// Filter log entries
// input: input log result
// filter: filter criteria
// output: output filtered log result (must be freed with log_result_free)
// Returns 0 on success, non-zero on error
int log_filter_entries(const log_result_t* input, const log_filter_t* filter, log_result_t* output);

// Get log statistics
// result: log result to analyze
// stats: output log statistics (must be freed with log_statistics_free)
// Returns 0 on success, non-zero on error
int log_get_statistics(const log_result_t* result, log_statistics_t* stats);

// Search for keywords in log entries
// result: log result to search
// keyword: keyword to search for
// case_sensitive: 1 for case sensitive, 0 for case insensitive
// Returns number of matches found, negative on error
int log_search_entries(const log_result_t* result, const char* keyword, int case_sensitive);

// Export log entries to file
// result: log result to export
// path: output file path
// format: export format
// Returns 0 on success, non-zero on error
int log_export_entries(const log_result_t* result, const char* path, log_format_t format);

// Detect log format
// path: path to the log file
// Returns detected format, or -1 on error
log_format_t log_detect_format(const char* path);

// Get format description
// format: log format
// Returns a description of the format
const char* log_get_format_description(log_format_t format);

#ifdef __cplusplus
}
#endif

#endif // LIBS_LOGS_LOGS_H