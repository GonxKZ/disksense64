#include "logs.h"
#include "parsers.h"
#include "filters.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

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

// Helper function to convert string to lowercase
static void strtolower(char* str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower(str[i]);
    }
}

void log_result_init(log_result_t* result) {
    if (result) {
        result->entries = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int log_result_add_entry(log_result_t* result, const log_entry_t* entry) {
    if (!result || !entry) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        log_entry_t* new_entries = (log_entry_t*)realloc(result->entries, new_capacity * sizeof(log_entry_t));
        if (!new_entries) {
            return -1;
        }
        result->entries = new_entries;
        result->capacity = new_capacity;
    }
    
    // Copy entry data
    log_entry_t* new_entry = &result->entries[result->count];
    new_entry->timestamp = entry->timestamp;
    new_entry->source = strdup_safe(entry->source);
    new_entry->level = strdup_safe(entry->level);
    new_entry->message = strdup_safe(entry->message);
    new_entry->host = strdup_safe(entry->host);
    new_entry->process = strdup_safe(entry->process);
    new_entry->pid = entry->pid;
    
    if ((!new_entry->source || !new_entry->level || !new_entry->message || 
         !new_entry->host || !new_entry->process) && 
        (entry->source || entry->level || entry->message || 
         entry->host || entry->process)) {
        // Clean up on failure
        free(new_entry->source);
        free(new_entry->level);
        free(new_entry->message);
        free(new_entry->host);
        free(new_entry->process);
        return -1;
    }
    
    result->count++;
    return 0;
}

void log_result_free(log_result_t* result) {
    if (result) {
        if (result->entries) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->entries[i].source);
                free(result->entries[i].level);
                free(result->entries[i].message);
                free(result->entries[i].host);
                free(result->entries[i].process);
            }
            free(result->entries);
        }
        result->entries = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

void log_filter_init(log_filter_t* filter) {
    if (filter) {
        memset(filter, 0, sizeof(log_filter_t));
        filter->include_errors = 1;
        filter->include_warnings = 1;
        filter->include_info = 1;
        filter->include_debug = 1;
    }
}

void log_filter_free(log_filter_t* filter) {
    if (filter) {
        free(filter->source_filter);
        free(filter->level_filter);
        free(filter->host_filter);
        free(filter->process_filter);
        free(filter->keyword_filter);
        memset(filter, 0, sizeof(log_filter_t));
    }
}

void log_statistics_init(log_statistics_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(log_statistics_t));
        stats->first_timestamp = -1;
        stats->last_timestamp = -1;
    }
}

void log_statistics_free(log_statistics_t* stats) {
    if (stats) {
        if (stats->top_sources) {
            for (size_t i = 0; i < stats->source_count; i++) {
                free(stats->top_sources[i]);
            }
            free(stats->top_sources);
        }
        memset(stats, 0, sizeof(log_statistics_t));
    }
}

int log_parse_file(const char* path, log_format_t format, log_result_t* result) {
    if (!path || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Open file
    FILE* file = fopen(path, "r");
    if (!file) {
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return -1;
    }
    
    // Allocate buffer
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return -1;
    }
    
    // Null terminate
    buffer[bytes_read] = '\0';
    
    // Parse data
    int ret = log_parse_data(buffer, bytes_read, format, result);
    free(buffer);
    
    return ret;
}

int log_parse_data(const void* data, size_t size, log_format_t format, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Convert data to string
    char* str_data = (char*)memdup(data, size);
    if (!str_data) {
        return -1;
    }
    str_data[size] = '\0';
    
    // Parse based on format
    int ret = -1;
    switch (format) {
        case LOG_FORMAT_SYSLOG:
            ret = log_parse_syslog(str_data, result);
            break;
        case LOG_FORMAT_APACHE:
            ret = log_parse_apache(str_data, result);
            break;
        case LOG_FORMAT_NGINX:
            ret = log_parse_nginx(str_data, result);
            break;
        case LOG_FORMAT_JSON:
            ret = log_parse_json(str_data, result);
            break;
        case LOG_FORMAT_CSV:
            ret = log_parse_csv(str_data, result);
            break;
        default:
            ret = log_parse_custom(str_data, result);
            break;
    }
    
    free(str_data);
    return ret;
}

int log_filter_entries(const log_result_t* input, const log_filter_t* filter, log_result_t* output) {
    if (!input || !filter || !output) {
        return -1;
    }
    
    log_result_init(output);
    
    // Apply filter to each entry
    for (size_t i = 0; i < input->count; i++) {
        const log_entry_t* entry = &input->entries[i];
        int include = 1;
        
        // Time filter
        if (filter->start_time > 0 && entry->timestamp < filter->start_time) {
            include = 0;
        }
        if (filter->end_time > 0 && entry->timestamp > filter->end_time) {
            include = 0;
        }
        
        // Source filter
        if (include && filter->source_filter && entry->source) {
            char* entry_source = strdup_safe(entry->source);
            if (entry_source) {
                strtolower(entry_source);
                char* filter_source = strdup_safe(filter->source_filter);
                if (filter_source) {
                    strtolower(filter_source);
                    if (strstr(entry_source, filter_source) == NULL) {
                        include = 0;
                    }
                    free(filter_source);
                }
                free(entry_source);
            }
        }
        
        // Level filter
        if (include && filter->level_filter && entry->level) {
            char* entry_level = strdup_safe(entry->level);
            if (entry_level) {
                strtolower(entry_level);
                char* filter_level = strdup_safe(filter->level_filter);
                if (filter_level) {
                    strtolower(filter_level);
                    if (strstr(entry_level, filter_level) == NULL) {
                        include = 0;
                    }
                    free(filter_level);
                }
                free(entry_level);
            }
        }
        
        // Host filter
        if (include && filter->host_filter && entry->host) {
            char* entry_host = strdup_safe(entry->host);
            if (entry_host) {
                strtolower(entry_host);
                char* filter_host = strdup_safe(filter->host_filter);
                if (filter_host) {
                    strtolower(filter_host);
                    if (strstr(entry_host, filter_host) == NULL) {
                        include = 0;
                    }
                    free(filter_host);
                }
                free(entry_host);
            }
        }
        
        // Process filter
        if (include && filter->process_filter && entry->process) {
            char* entry_process = strdup_safe(entry->process);
            if (entry_process) {
                strtolower(entry_process);
                char* filter_process = strdup_safe(filter->process_filter);
                if (filter_process) {
                    strtolower(filter_process);
                    if (strstr(entry_process, filter_process) == NULL) {
                        include = 0;
                    }
                    free(filter_process);
                }
                free(entry_process);
            }
        }
        
        // Keyword filter
        if (include && filter->keyword_filter && entry->message) {
            char* entry_message = strdup_safe(entry->message);
            if (entry_message) {
                strtolower(entry_message);
                char* filter_keyword = strdup_safe(filter->keyword_filter);
                if (filter_keyword) {
                    strtolower(filter_keyword);
                    if (strstr(entry_message, filter_keyword) == NULL) {
                        include = 0;
                    }
                    free(filter_keyword);
                }
                free(entry_message);
            }
        }
        
        // Level type filters
        if (include && entry->level) {
            char* level = strdup_safe(entry->level);
            if (level) {
                strtolower(level);
                
                if (strstr(level, "error") || strstr(level, "fatal")) {
                    include = filter->include_errors;
                } else if (strstr(level, "warn")) {
                    include = filter->include_warnings;
                } else if (strstr(level, "info")) {
                    include = filter->include_info;
                } else if (strstr(level, "debug")) {
                    include = filter->include_debug;
                }
                
                free(level);
            }
        }
        
        // Add to output if included
        if (include) {
            log_result_add_entry(output, entry);
        }
    }
    
    return 0;
}

int log_get_statistics(const log_result_t* result, log_statistics_t* stats) {
    if (!result || !stats) {
        return -1;
    }
    
    log_statistics_init(stats);
    
    stats->total_entries = result->count;
    
    // Process each entry
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        // Update timestamps
        if (stats->first_timestamp == -1 || entry->timestamp < stats->first_timestamp) {
            stats->first_timestamp = entry->timestamp;
        }
        if (stats->last_timestamp == -1 || entry->timestamp > stats->last_timestamp) {
            stats->last_timestamp = entry->timestamp;
        }
        
        // Count by level
        if (entry->level) {
            char* level = strdup_safe(entry->level);
            if (level) {
                strtolower(level);
                
                if (strstr(level, "error") || strstr(level, "fatal")) {
                    stats->error_count++;
                } else if (strstr(level, "warn")) {
                    stats->warning_count++;
                } else if (strstr(level, "info")) {
                    stats->info_count++;
                } else if (strstr(level, "debug")) {
                    stats->debug_count++;
                }
                
                free(level);
            }
        }
        
        // Track sources (simplified - in a real implementation, this would be more sophisticated)
        if (entry->source && stats->source_count < 10) { // Limit to 10 sources for simplicity
            int found = 0;
            for (size_t j = 0; j < stats->source_count; j++) {
                if (strcmp(stats->top_sources[j], entry->source) == 0) {
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                char** new_sources = (char**)realloc(stats->top_sources, (stats->source_count + 1) * sizeof(char*));
                if (new_sources) {
                    stats->top_sources = new_sources;
                    stats->top_sources[stats->source_count] = strdup_safe(entry->source);
                    if (stats->top_sources[stats->source_count]) {
                        stats->source_count++;
                    }
                }
            }
        }
    }
    
    return 0;
}

int log_search_entries(const log_result_t* result, const char* keyword, int case_sensitive) {
    if (!result || !keyword) {
        return -1;
    }
    
    int total_matches = 0;
    
    // Search each entry's message
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        if (!entry->message) {
            continue;
        }
        
        // Search for keyword in message
        size_t keyword_len = strlen(keyword);
        size_t message_len = strlen(entry->message);
        
        if (keyword_len > message_len) {
            continue;
        }
        
        int entry_matches = 0;
        for (size_t j = 0; j <= message_len - keyword_len; j++) {
            int match = 1;
            
            for (size_t k = 0; k < keyword_len; k++) {
                if (case_sensitive) {
                    if (entry->message[j + k] != keyword[k]) {
                        match = 0;
                        break;
                    }
                } else {
                    if (tolower(entry->message[j + k]) != tolower(keyword[k])) {
                        match = 0;
                        break;
                    }
                }
            }
            
            if (match) {
                entry_matches++;
                total_matches++;
                printf("Found keyword '%s' in log entry at %s: %s\n",
                       keyword, ctime(&entry->timestamp), entry->message);
            }
        }
        
        if (entry_matches > 0) {
            printf("Entry timestamp %s: %d matches\n", ctime(&entry->timestamp), entry_matches);
        }
    }
    
    return total_matches;
}

int log_export_entries(const log_result_t* result, const char* path, log_format_t format) {
    if (!result || !path) {
        return -1;
    }
    
    // Open output file
    FILE* file = fopen(path, "w");
    if (!file) {
        return -1;
    }
    
    // Export based on format
    int ret = 0;
    switch (format) {
        case LOG_FORMAT_SYSLOG:
            ret = log_export_syslog(result, file);
            break;
        case LOG_FORMAT_APACHE:
            ret = log_export_apache(result, file);
            break;
        case LOG_FORMAT_NGINX:
            ret = log_export_nginx(result, file);
            break;
        case LOG_FORMAT_JSON:
            ret = log_export_json(result, file);
            break;
        case LOG_FORMAT_CSV:
            ret = log_export_csv(result, file);
            break;
        default:
            ret = log_export_custom(result, file);
            break;
    }
    
    fclose(file);
    return ret;
}

log_format_t log_detect_format(const char* path) {
    if (!path) {
        return -1;
    }
    
    // Check file extension first
    const char* ext = strrchr(path, '.');
    if (ext) {
        if (strcasecmp(ext, ".log") == 0) {
            // Check content to determine specific log format
        } else if (strcasecmp(ext, ".json") == 0) {
            return LOG_FORMAT_JSON;
        } else if (strcasecmp(ext, ".csv") == 0) {
            return LOG_FORMAT_CSV;
        }
    }
    
    // Try to read file header to determine format
    FILE* file = fopen(path, "r");
    if (!file) {
        return -1;
    }
    
    char header[1024];
    size_t bytes_read = fread(header, 1, sizeof(header) - 1, file);
    fclose(file);
    
    if (bytes_read == 0) {
        return -1;
    }
    
    header[bytes_read] = '\0';
    
    // Check for syslog format (e.g., "Jan  1 00:00:00 hostname process:")
    if (bytes_read > 20) {
        // Simple heuristic for syslog
        if ((header[3] == ' ' || header[3] == '\t') && 
            (header[6] == ' ' || header[6] == '\t') &&
            header[9] == ':' && header[12] == ':') {
            return LOG_FORMAT_SYSLOG;
        }
    }
    
    // Check for Apache format (contains common log format pattern)
    if (strstr(header, "GET ") || strstr(header, "POST ") || strstr(header, "HTTP/")) {
        return LOG_FORMAT_APACHE;
    }
    
    // Check for JSON format
    if (header[0] == '{' || header[0] == '[') {
        return LOG_FORMAT_JSON;
    }
    
    // Default to custom format
    return LOG_FORMAT_CUSTOM;
}

const char* log_get_format_description(log_format_t format) {
    switch (format) {
        case LOG_FORMAT_SYSLOG:
            return "Syslog format";
        case LOG_FORMAT_APACHE:
            return "Apache Common Log Format";
        case LOG_FORMAT_NGINX:
            return "Nginx Log Format";
        case LOG_FORMAT_JSON:
            return "JSON Log Format";
        case LOG_FORMAT_CSV:
            return "CSV Log Format";
        case LOG_FORMAT_CUSTOM:
            return "Custom Log Format";
        default:
            return "Unknown format";
    }
}