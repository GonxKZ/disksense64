#include "parsers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

// Parse a single syslog line
static int parse_syslog_line(const char* line, log_entry_t* entry) {
    if (!line || !entry) {
        return -1;
    }
    
    // Initialize entry
    memset(entry, 0, sizeof(log_entry_t));
    
    // Example syslog format: "Jan  1 00:00:00 hostname process[pid]: message"
    // This is a simplified parser
    
    // Parse timestamp (first 15 characters)
    char timestamp_str[16];
    strncpy(timestamp_str, line, 15);
    timestamp_str[15] = '\0';
    
    // For simplicity, we'll just set current time
    entry->timestamp = time(NULL);
    
    // Parse hostname (after timestamp)
    const char* hostname_start = line + 16;
    const char* hostname_end = strchr(hostname_start, ' ');
    if (hostname_end) {
        size_t hostname_len = hostname_end - hostname_start;
        entry->host = (char*)malloc(hostname_len + 1);
        if (entry->host) {
            strncpy(entry->host, hostname_start, hostname_len);
            entry->host[hostname_len] = '\0';
        }
        
        // Parse process name and PID
        const char* process_start = hostname_end + 1;
        const char* process_end = strchr(process_start, ':');
        if (process_end) {
            size_t process_len = process_end - process_start;
            entry->process = (char*)malloc(process_len + 1);
            if (entry->process) {
                strncpy(entry->process, process_start, process_len);
                entry->process[process_len] = '\0';
                
                // Try to extract PID from process[pid] format
                char* pid_start = strchr(entry->process, '[');
                if (pid_start) {
                    char* pid_end = strchr(pid_start, ']');
                    if (pid_end) {
                        *pid_end = '\0';
                        entry->pid = atoi(pid_start + 1);
                        *pid_start = '\0'; // Truncate process name
                    }
                }
            }
            
            // Parse message
            const char* message_start = process_end + 2; // Skip ": "
            entry->message = strdup(message_start);
        }
    }
    
    // Set defaults
    entry->source = strdup("syslog");
    entry->level = strdup("info");
    
    return 0;
}

int log_parse_syslog(const char* data, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Parse line by line
    const char* line_start = data;
    const char* line_end = strchr(line_start, '\n');
    
    while (line_start && *line_start) {
        size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
        
        // Skip empty lines
        if (line_len > 0) {
            // Create a copy of the line
            char* line = (char*)malloc(line_len + 1);
            if (line) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // Parse the line
                log_entry_t entry;
                if (parse_syslog_line(line, &entry) == 0) {
                    log_result_add_entry(result, &entry);
                }
                
                // Clean up entry
                free(entry.source);
                free(entry.level);
                free(entry.message);
                free(entry.host);
                free(entry.process);
                
                free(line);
            }
        }
        
        // Move to next line
        line_start = line_end ? line_end + 1 : NULL;
        line_end = line_start ? strchr(line_start, '\n') : NULL;
    }
    
    return 0;
}

int log_parse_apache(const char* data, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Parse Apache Common Log Format:
    // 127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
    
    const char* line_start = data;
    const char* line_end = strchr(line_start, '\n');
    
    while (line_start && *line_start) {
        size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
        
        // Skip empty lines
        if (line_len > 0) {
            // Create a copy of the line
            char* line = (char*)malloc(line_len + 1);
            if (line) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // Parse the line
                log_entry_t entry;
                memset(&entry, 0, sizeof(log_entry_t));
                entry.timestamp = time(NULL); // Simplified
                entry.source = strdup("apache");
                entry.level = strdup("info");
                
                // Extract IP address (first field)
                char* ip_end = strchr(line, ' ');
                if (ip_end) {
                    *ip_end = '\0';
                    entry.host = strdup(line);
                    *ip_end = ' ';
                    
                    // Extract request (between quotes)
                    char* request_start = strchr(line, '"');
                    if (request_start) {
                        char* request_end = strchr(request_start + 1, '"');
                        if (request_end) {
                            *request_end = '\0';
                            entry.message = strdup(request_start + 1);
                            *request_end = '"';
                        }
                    }
                }
                
                entry.process = strdup("apache");
                
                log_result_add_entry(result, &entry);
                
                // Clean up entry
                free(entry.source);
                free(entry.level);
                free(entry.message);
                free(entry.host);
                free(entry.process);
                
                free(line);
            }
        }
        
        // Move to next line
        line_start = line_end ? line_end + 1 : NULL;
        line_end = line_start ? strchr(line_start, '\n') : NULL;
    }
    
    return 0;
}

int log_parse_nginx(const char* data, log_result_t* result) {
    // For simplicity, we'll use the same parser as Apache
    return log_parse_apache(data, result);
}

int log_parse_json(const char* data, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Simplified JSON parser - in a real implementation, this would use a proper JSON library
    
    // Parse line by line (assuming one JSON object per line)
    const char* line_start = data;
    const char* line_end = strchr(line_start, '\n');
    
    while (line_start && *line_start) {
        size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
        
        // Skip empty lines
        if (line_len > 0) {
            // Create a copy of the line
            char* line = (char*)malloc(line_len + 1);
            if (line) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // Parse the line as JSON (simplified)
                log_entry_t entry;
                memset(&entry, 0, sizeof(log_entry_t));
                entry.timestamp = time(NULL); // Simplified
                entry.source = strdup("json");
                entry.level = strdup("info");
                entry.message = strdup(line);
                entry.host = strdup("localhost");
                entry.process = strdup("json_parser");
                
                log_result_add_entry(result, &entry);
                
                // Clean up entry
                free(entry.source);
                free(entry.level);
                free(entry.message);
                free(entry.host);
                free(entry.process);
                
                free(line);
            }
        }
        
        // Move to next line
        line_start = line_end ? line_end + 1 : NULL;
        line_end = line_start ? strchr(line_start, '\n') : NULL;
    }
    
    return 0;
}

int log_parse_csv(const char* data, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Parse CSV format (assuming standard format with header)
    const char* line_start = data;
    const char* line_end = strchr(line_start, '\n');
    int first_line = 1; // Skip header
    
    while (line_start && *line_start) {
        size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
        
        // Skip empty lines
        if (line_len > 0 && !first_line) {
            // Create a copy of the line
            char* line = (char*)malloc(line_len + 1);
            if (line) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // Parse the line as CSV (simplified)
                log_entry_t entry;
                memset(&entry, 0, sizeof(log_entry_t));
                entry.timestamp = time(NULL); // Simplified
                entry.source = strdup("csv");
                entry.level = strdup("info");
                entry.message = strdup(line);
                entry.host = strdup("localhost");
                entry.process = strdup("csv_parser");
                
                log_result_add_entry(result, &entry);
                
                // Clean up entry
                free(entry.source);
                free(entry.level);
                free(entry.message);
                free(entry.host);
                free(entry.process);
                
                free(line);
            }
        }
        
        first_line = 0;
        
        // Move to next line
        line_start = line_end ? line_end + 1 : NULL;
        line_end = line_start ? strchr(line_start, '\n') : NULL;
    }
    
    return 0;
}

int log_parse_custom(const char* data, log_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    log_result_init(result);
    
    // Parse custom format (treat each line as a separate message)
    const char* line_start = data;
    const char* line_end = strchr(line_start, '\n');
    
    while (line_start && *line_start) {
        size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
        
        // Skip empty lines
        if (line_len > 0) {
            // Create a copy of the line
            char* line = (char*)malloc(line_len + 1);
            if (line) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // Parse the line
                log_entry_t entry;
                memset(&entry, 0, sizeof(log_entry_t));
                entry.timestamp = time(NULL); // Simplified
                entry.source = strdup("custom");
                entry.level = strdup("info");
                entry.message = strdup(line);
                entry.host = strdup("localhost");
                entry.process = strdup("custom_parser");
                
                log_result_add_entry(result, &entry);
                
                // Clean up entry
                free(entry.source);
                free(entry.level);
                free(entry.message);
                free(entry.host);
                free(entry.process);
                
                free(line);
            }
        }
        
        // Move to next line
        line_start = line_end ? line_end + 1 : NULL;
        line_end = line_start ? strchr(line_start, '\n') : NULL;
    }
    
    return 0;
}

int log_export_syslog(const log_result_t* result, FILE* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Export in syslog format
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        // Format: "Jan  1 00:00:00 hostname process[pid]: message"
        char* time_str = ctime(&entry->timestamp);
        if (time_str) {
            // Remove newline from ctime
            char* nl = strchr(time_str, '\n');
            if (nl) *nl = '\0';
            
            fprintf(file, "%s %s %s[%d]: %s\n",
                    time_str + 4, // Skip day of week
                    entry->host ? entry->host : "localhost",
                    entry->process ? entry->process : "unknown",
                    entry->pid,
                    entry->message ? entry->message : "");
        }
    }
    
    return 0;
}

int log_export_apache(const log_result_t* result, FILE* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Export in Apache Common Log Format
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        // Format: "127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326"
        char* time_str = ctime(&entry->timestamp);
        if (time_str) {
            // Remove newline from ctime
            char* nl = strchr(time_str, '\n');
            if (nl) *nl = '\0';
            
            fprintf(file, "%s - - [%s] \"%s\" 200 1024\n",
                    entry->host ? entry->host : "127.0.0.1",
                    time_str,
                    entry->message ? entry->message : "");
        }
    }
    
    return 0;
}

int log_export_nginx(const log_result_t* result, FILE* file) {
    // For simplicity, we'll use the same exporter as Apache
    return log_export_apache(result, file);
}

int log_export_json(const log_result_t* result, FILE* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Export in JSON format
    fprintf(file, "[\n");
    
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        fprintf(file, "  {\n");
        fprintf(file, "    \"timestamp\": %ld,\n", entry->timestamp);
        fprintf(file, "    \"source\": \"%s\",\n", entry->source ? entry->source : "");
        fprintf(file, "    \"level\": \"%s\",\n", entry->level ? entry->level : "");
        fprintf(file, "    \"message\": \"%s\",\n", entry->message ? entry->message : "");
        fprintf(file, "    \"host\": \"%s\",\n", entry->host ? entry->host : "");
        fprintf(file, "    \"process\": \"%s\",\n", entry->process ? entry->process : "");
        fprintf(file, "    \"pid\": %d\n", entry->pid);
        fprintf(file, "  }%s\n", (i < result->count - 1) ? "," : "");
    }
    
    fprintf(file, "]\n");
    
    return 0;
}

int log_export_csv(const log_result_t* result, FILE* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Export in CSV format
    fprintf(file, "timestamp,source,level,message,host,process,pid\n");
    
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        fprintf(file, "%ld,%s,%s,\"%s\",%s,%s,%d\n",
                entry->timestamp,
                entry->source ? entry->source : "",
                entry->level ? entry->level : "",
                entry->message ? entry->message : "",
                entry->host ? entry->host : "",
                entry->process ? entry->process : "",
                entry->pid);
    }
    
    return 0;
}

int log_export_custom(const log_result_t* result, FILE* file) {
    if (!result || !file) {
        return -1;
    }
    
    // Export in custom format (one message per line)
    for (size_t i = 0; i < result->count; i++) {
        const log_entry_t* entry = &result->entries[i];
        
        fprintf(file, "%s\n", entry->message ? entry->message : "");
    }
    
    return 0;
}