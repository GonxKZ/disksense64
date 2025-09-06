#include "libs/logs/logs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Create test log data
static int create_test_logs(const char* log_path) {
    FILE* file = fopen(log_path, "w");
    if (!file) {
        return -1;
    }
    
    // Write test syslog data
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M:%S", tm_info);
    
    fprintf(file, "%s localhost systemd[1]: Started Session 1 of user testuser.\n", time_str);
    fprintf(file, "%s localhost sshd[1234]: Accepted password for testuser from 192.168.1.100 port 22 ssh2\n", time_str);
    fprintf(file, "%s localhost sshd[1235]: Connection closed by 192.168.1.100 port 22 [preauth]\n", time_str);
    fprintf(file, "%s localhost kernel: [    0.000000] Linux version 5.4.0-generic\n", time_str);
    fprintf(file, "%s localhost apache2: [error] [client 192.168.1.101] PHP Notice:  Undefined index\n", time_str);
    fprintf(file, "%s localhost apache2: [notice] Apache/2.4.41 (Ubuntu) configured\n", time_str);
    
    fclose(file);
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* log_path) {
    unlink(log_path);
}

int test_log_parsing() {
    printf("Testing log parsing...\n");
    
    const char* test_log = "/tmp/test_syslog.log";
    
    // Create test log data
    if (create_test_logs(test_log) != 0) {
        printf("Failed to create test log data\n");
        return 1;
    }
    
    // Parse log file
    log_result_t result;
    int ret = log_parse_file(test_log, LOG_FORMAT_SYSLOG, &result);
    
    if (ret != 0) {
        printf("Failed to parse log file\n");
        cleanup_test_environment(test_log);
        return 1;
    }
    
    printf("Parsed %lu log entries:\n", (unsigned long)result.count);
    
    for (size_t i = 0; i < result.count; i++) {
        log_entry_t* entry = &result.entries[i];
        printf("  Entry %lu:\n", (unsigned long)i);
        printf("    Timestamp: %s", ctime(&entry->timestamp));
        printf("    Source: %s\n", entry->source ? entry->source : "unknown");
        printf("    Level: %s\n", entry->level ? entry->level : "unknown");
        printf("    Host: %s\n", entry->host ? entry->host : "unknown");
        printf("    Process: %s\n", entry->process ? entry->process : "unknown");
        printf("    PID: %d\n", entry->pid);
        printf("    Message: %s\n", entry->message ? entry->message : "unknown");
    }
    
    // Clean up
    log_result_free(&result);
    cleanup_test_environment(test_log);
    
    printf("Log parsing test passed!\n");
    return 0;
}

int test_log_filtering() {
    printf("Testing log filtering...\n");
    
    const char* test_log = "/tmp/test_syslog.log";
    
    // Create test log data
    if (create_test_logs(test_log) != 0) {
        printf("Failed to create test log data\n");
        return 1;
    }
    
    // Parse log file
    log_result_t parsed_result;
    int ret = log_parse_file(test_log, LOG_FORMAT_SYSLOG, &parsed_result);
    
    if (ret != 0) {
        printf("Failed to parse log file\n");
        cleanup_test_environment(test_log);
        return 1;
    }
    
    // Apply filter
    log_filter_t filter;
    log_filter_init(&filter);
    filter.process_filter = strdup("apache2");
    filter.include_errors = 1;
    filter.include_warnings = 1;
    filter.include_info = 1;
    filter.include_debug = 1;
    
    log_result_t filtered_result;
    ret = log_filter_entries(&parsed_result, &filter, &filtered_result);
    
    if (ret != 0) {
        printf("Failed to filter log entries\n");
        log_filter_free(&filter);
        log_result_free(&parsed_result);
        cleanup_test_environment(test_log);
        return 1;
    }
    
    printf("Filtered %lu log entries (process=apache2):\n", (unsigned long)filtered_result.count);
    
    for (size_t i = 0; i < filtered_result.count; i++) {
        log_entry_t* entry = &filtered_result.entries[i];
        printf("  Entry %lu: %s\n", (unsigned long)i, entry->message ? entry->message : "unknown");
    }
    
    // Clean up
    log_filter_free(&filter);
    log_result_free(&parsed_result);
    log_result_free(&filtered_result);
    cleanup_test_environment(test_log);
    
    printf("Log filtering test passed!\n");
    return 0;
}

int test_log_statistics() {
    printf("Testing log statistics...\n");
    
    const char* test_log = "/tmp/test_syslog.log";
    
    // Create test log data
    if (create_test_logs(test_log) != 0) {
        printf("Failed to create test log data\n");
        return 1;
    }
    
    // Parse log file
    log_result_t result;
    int ret = log_parse_file(test_log, LOG_FORMAT_SYSLOG, &result);
    
    if (ret != 0) {
        printf("Failed to parse log file\n");
        cleanup_test_environment(test_log);
        return 1;
    }
    
    // Get statistics
    log_statistics_t stats;
    ret = log_get_statistics(&result, &stats);
    
    if (ret != 0) {
        printf("Failed to get log statistics\n");
        log_result_free(&result);
        cleanup_test_environment(test_log);
        return 1;
    }
    
    printf("Log statistics:\n");
    printf("  Total entries: %lu\n", (unsigned long)stats.total_entries);
    printf("  Error count: %lu\n", (unsigned long)stats.error_count);
    printf("  Warning count: %lu\n", (unsigned long)stats.warning_count);
    printf("  Info count: %lu\n", (unsigned long)stats.info_count);
    printf("  Debug count: %lu\n", (unsigned long)stats.debug_count);
    printf("  First timestamp: %s", ctime(&stats.first_timestamp));
    printf("  Last timestamp: %s", ctime(&stats.last_timestamp));
    printf("  Top sources (%lu):\n", (unsigned long)stats.source_count);
    
    for (size_t i = 0; i < stats.source_count; i++) {
        printf("    %s\n", stats.top_sources[i] ? stats.top_sources[i] : "unknown");
    }
    
    // Clean up
    log_statistics_free(&stats);
    log_result_free(&result);
    cleanup_test_environment(test_log);
    
    printf("Log statistics test passed!\n");
    return 0;
}

int test_log_search() {
    printf("Testing log search...\n");
    
    const char* test_log = "/tmp/test_syslog.log";
    
    // Create test log data
    if (create_test_logs(test_log) != 0) {
        printf("Failed to create test log data\n");
        return 1;
    }
    
    // Parse log file
    log_result_t result;
    int ret = log_parse_file(test_log, LOG_FORMAT_SYSLOG, &result);
    
    if (ret != 0) {
        printf("Failed to parse log file\n");
        cleanup_test_environment(test_log);
        return 1;
    }
    
    // Search for keywords
    printf("Searching for 'apache' (case insensitive)...\n");
    int matches = log_search_entries(&result, "apache", 0);
    printf("Found %d matches for 'apache'\n", matches);
    
    printf("Searching for 'sshd' (case insensitive)...\n");
    matches = log_search_entries(&result, "sshd", 0);
    printf("Found %d matches for 'sshd'\n", matches);
    
    // Clean up
    log_result_free(&result);
    cleanup_test_environment(test_log);
    
    printf("Log search test passed!\n");
    return 0;
}

int test_log_export() {
    printf("Testing log export...\n");
    
    const char* test_log = "/tmp/test_syslog.log";
    const char* export_log = "/tmp/exported_logs.json";
    
    // Create test log data
    if (create_test_logs(test_log) != 0) {
        printf("Failed to create test log data\n");
        return 1;
    }
    
    // Parse log file
    log_result_t result;
    int ret = log_parse_file(test_log, LOG_FORMAT_SYSLOG, &result);
    
    if (ret != 0) {
        printf("Failed to parse log file\n");
        cleanup_test_environment(test_log);
        return 1;
    }
    
    // Export to JSON
    ret = log_export_entries(&result, export_log, LOG_FORMAT_JSON);
    
    if (ret != 0) {
        printf("Failed to export log entries\n");
        log_result_free(&result);
        cleanup_test_environment(test_log);
        return 1;
    }
    
    printf("Exported log entries to %s\n", export_log);
    
    // Clean up
    log_result_free(&result);
    cleanup_test_environment(test_log);
    unlink(export_log);
    
    printf("Log export test passed!\n");
    return 0;
}

int test_format_detection() {
    printf("Testing log format detection...\n");
    
    const char* test_cases[][2] = {
        {"/var/log/syslog", "syslog"},
        {"/var/log/apache2/access.log", "apache"},
        {"/var/log/nginx/access.log", "nginx"},
        {"/tmp/test.json", "json"},
        {"/tmp/test.csv", "csv"},
        {NULL, NULL}
    };
    
    for (int i = 0; test_cases[i][0] != NULL; i++) {
        log_format_t format = log_detect_format(test_cases[i][0]);
        const char* description = log_get_format_description(format);
        
        printf("File: %s -> Format: %s\n", test_cases[i][0], description);
    }
    
    printf("Log format detection test passed!\n");
    return 0;
}

int main() {
    printf("Running system logs analysis tests...\n");
    
    int result1 = test_log_parsing();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_log_filtering();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_log_statistics();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_log_search();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_log_export();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_format_detection();
    if (result6 != 0) {
        return result6;
    }
    
    printf("All system logs analysis tests passed!\n");
    return 0;
}