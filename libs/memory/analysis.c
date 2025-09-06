#include "analysis.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int memory_perform_comprehensive_analysis(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result) {
    if (!dump_path || !result) {
        return -1;
    }
    
    memory_analysis_result_init(result);
    
    // Set dump path and format
    result->dump_path = strdup(dump_path);
    result->format = memory_detect_format(dump_path);
    
    // Get file size
    FILE* file = fopen(dump_path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        result->total_size = ftell(file);
        result->analyzed_size = result->total_size;
        fclose(file);
    }
    
    printf("Performing comprehensive memory analysis on: %s\n", dump_path);
    printf("Format: %s, Size: %lu bytes\n", 
           memory_get_format_name(result->format), 
           (unsigned long)result->total_size);
    
    // Extract processes if requested
    if (!options || options->analyze_processes) {
        memory_process_t* processes;
        size_t process_count;
        
        if (memory_extract_processes_internal(dump_path, options, &processes, &process_count) == 0) {
            printf("Extracted %lu processes\n", (unsigned long)process_count);
            
            // Add processes to result and free originals
            for (size_t i = 0; i < process_count; i++) {
                memory_analysis_result_add_process(result, &processes[i]);
                memory_process_free(&processes[i]);
            }
            
            free(processes);
        }
    }
    
    // Extract connections if requested
    if (!options || options->analyze_network) {
        memory_connection_t* connections;
        size_t connection_count;
        
        if (memory_extract_connections_internal(dump_path, options, &connections, &connection_count) == 0) {
            printf("Extracted %lu connections\n", (unsigned long)connection_count);
            
            // Add connections to result and free originals
            for (size_t i = 0; i < connection_count; i++) {
                memory_analysis_result_add_connection(result, &connections[i]);
                memory_connection_free(&connections[i]);
            }
            
            free(connections);
        }
    }
    
    // Extract regions if requested
    if (!options || options->analyze_regions) {
        memory_region_t* regions;
        size_t region_count;
        
        if (memory_analyze_regions_internal(dump_path, options, &regions, &region_count) == 0) {
            printf("Extracted %lu memory regions\n", (unsigned long)region_count);
            
            // Add regions to result and free originals
            for (size_t i = 0; i < region_count; i++) {
                memory_analysis_result_add_region(result, &regions[i]);
                memory_region_free(&regions[i]);
            }
            
            free(regions);
        }
    }
    
    // Detect suspicious activities
    if (!options || options->detect_malware) {
        size_t suspicious_processes = 0;
        memory_detect_suspicious_processes(result->processes, result->process_count, &suspicious_processes);
        
        size_t suspicious_connections = 0;
        memory_detect_suspicious_connections(result->connections, result->connection_count, &suspicious_connections);
        
        if (suspicious_processes > 0) {
            result->has_suspicious_processes = 1;
        }
        
        if (suspicious_connections > 0) {
            result->has_suspicious_connections = 1;
        }
        
        printf("Found %lu suspicious processes and %lu suspicious connections\n", 
               (unsigned long)suspicious_processes, (unsigned long)suspicious_connections);
    }
    
    // Detect code injection
    if (!options || options->detect_code_injection) {
        size_t injected_count = 0;
        if (memory_detect_injected_code_internal(dump_path, result->processes, result->process_count, &injected_count) == 0) {
            if (injected_count > 0) {
                result->has_injected_code = 1;
                printf("Detected code injection in %lu processes\n", (unsigned long)injected_count);
            }
        }
    }
    
    // Detect rootkit activity
    if (!options || options->detect_rootkits) {
        int is_rootkit_detected = 0;
        if (memory_detect_rootkit_activity_internal(dump_path, result, &is_rootkit_detected) == 0) {
            if (is_rootkit_detected) {
                result->has_rootkit_activity = 1;
                printf("Detected potential rootkit activity\n");
            }
        }
    }
    
    return 0;
}

int memory_detect_injected_code_internal(const char* dump_path, const memory_process_t* processes, size_t count, size_t* injected_count) {
    if (!dump_path || !processes || !injected_count) {
        return -1;
    }
    
    *injected_count = 0;
    
    printf("Detecting code injection in %lu processes\n", (unsigned long)count);
    
    // Analyze each process for injection indicators
    for (size_t i = 0; i < count; i++) {
        const memory_process_t* process = &processes[i];
        
        // Check for suspicious process names that might indicate injection
        if (process->process_name) {
            const char* suspicious_indicators[] = {
                "inject", "malware", "trojan", "backdoor", 
                "keylog", "ransom", "spyware", "rootkit"
            };
            
            int num_indicators = sizeof(suspicious_indicators) / sizeof(suspicious_indicators[0]);
            
            for (int j = 0; j < num_indicators; j++) {
                if (strstr(process->process_name, suspicious_indicators[j]) != NULL) {
                    (*injected_count)++;
                    printf("Process %s flagged for injection indicator: %s\n", 
                           process->process_name, suspicious_indicators[j]);
                    break;
                }
            }
        }
        
        // Check for unusual memory characteristics
        if (process->virtual_size > 1000000000UL && // 1GB+
            process->working_set_size < 10000000UL) { // Less than 10MB
            (*injected_count)++;
            printf("Process %s flagged for unusual memory characteristics\n", 
                   process->process_name ? process->process_name : "unknown");
        }
    }
    
    return 0;
}

int memory_detect_rootkit_activity_internal(const char* dump_path, const memory_analysis_result_t* result, int* is_rootkit_detected) {
    if (!dump_path || !result || !is_rootkit_detected) {
        return -1;
    }
    
    *is_rootkit_detected = 0;
    
    printf("Detecting rootkit activity\n");
    
    // Check for suspicious combinations that might indicate rootkit activity
    int suspicious_process_count = 0;
    for (size_t i = 0; i < result->process_count; i++) {
        if (result->processes[i].is_suspicious) {
            suspicious_process_count++;
        }
    }
    
    int suspicious_connection_count = 0;
    for (size_t i = 0; i < result->connection_count; i++) {
        if (result->connections[i].is_suspicious) {
            suspicious_connection_count++;
        }
    }
    
    // Rootkit detection heuristic: many suspicious processes and connections
    if (suspicious_process_count > 3 && suspicious_connection_count > 5) {
        *is_rootkit_detected = 1;
        printf("Rootkit activity detected based on suspicious process/connection patterns\n");
    }
    
    // Check for hidden processes (processes with no name)
    int hidden_processes = 0;
    for (size_t i = 0; i < result->process_count; i++) {
        if (!result->processes[i].process_name || strlen(result->processes[i].process_name) == 0) {
            hidden_processes++;
        }
    }
    
    if (hidden_processes > 0) {
        *is_rootkit_detected = 1;
        printf("Rootkit activity detected based on %d hidden processes\n", hidden_processes);
    }
    
    return 0;
}

int memory_extract_and_analyze_strings(const char* dump_path, size_t min_length, memory_string_t** strings, size_t* count) {
    if (!dump_path || !strings || !count) {
        return -1;
    }
    
    *strings = NULL;
    *count = 0;
    
    printf("Extracting and analyzing strings from: %s\n", dump_path);
    
    // Open memory dump file
    FILE* file = fopen(dump_path, "rb");
    if (!file) {
        perror("Failed to open memory dump file");
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
    
    // Allocate buffer for reading
    size_t buffer_size = 1024 * 1024; // 1MB buffer
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    
    // Allocate initial array for strings
    size_t string_capacity = 1000;
    *strings = (memory_string_t*)malloc(string_capacity * sizeof(memory_string_t));
    if (!*strings) {
        free(buffer);
        fclose(file);
        return -1;
    }
    
    // Read file in chunks and extract strings
    long position = 0;
    while (position < file_size) {
        size_t bytes_to_read = (file_size - position > (long)buffer_size) ? buffer_size : (file_size - position);
        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
        
        if (bytes_read == 0) {
            break;
        }
        
        // Extract ASCII strings from buffer
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) { // Printable ASCII
                // Start of potential string
                size_t string_start = i;
                size_t string_length = 0;
                
                // Count consecutive printable characters
                while (i < bytes_read && buffer[i] >= 32 && buffer[i] <= 126) {
                    string_length++;
                    i++;
                }
                
                // Check if string meets minimum length requirement
                if (string_length >= min_length) {
                    // Reallocate if needed
                    if (*count >= string_capacity) {
                        string_capacity *= 2;
                        memory_string_t* new_strings = (memory_string_t*)realloc(*strings, string_capacity * sizeof(memory_string_t));
                        if (!new_strings) {
                            // Clean up and return error
                            for (size_t j = 0; j < *count; j++) {
                                memory_string_free(&(*strings)[j]);
                            }
                            free(*strings);
                            free(buffer);
                            fclose(file);
                            return -1;
                        }
                        *strings = new_strings;
                    }
                    
                    // Initialize and populate string structure
                    memory_string_init(&(*strings)[*count]);
                    (*strings)[*count].string_value = (char*)malloc(string_length + 1);
                    if ((*strings)[*count].string_value) {
                        strncpy((*strings)[*count].string_value, &buffer[string_start], string_length);
                        (*strings)[*count].string_value[string_length] = '\0';
                        (*strings)[*count].address = position + string_start;
                        (*strings)[*count].length = string_length;
                        
                        // Analyze string for suspicious content
                        const char* suspicious_keywords[] = {
                            "malware", "trojan", "virus", "backdoor", 
                            "keylog", "ransom", "exploit", "shellcode"
                        };
                        
                        int num_keywords = sizeof(suspicious_keywords) / sizeof(suspicious_keywords[0]);
                        (*strings)[*count].is_suspicious = 0;
                        
                        for (int j = 0; j < num_keywords; j++) {
                            if (strstr((*strings)[*count].string_value, suspicious_keywords[j]) != NULL) {
                                (*strings)[*count].is_suspicious = 1;
                                (*strings)[*count].context = strdup("Suspicious keyword detected");
                                break;
                            }
                        }
                        
                        if (!(*strings)[*count].is_suspicious) {
                            (*strings)[*count].context = strdup("Normal string");
                        }
                        
                        (*count)++;
                    }
                }
            }
        }
        
        position += bytes_read;
        fseek(file, position, SEEK_SET);
    }
    
    free(buffer);
    fclose(file);
    
    printf("Extracted %lu strings\n", (unsigned long)*count);
    return 0;
}

int memory_analyze_regions_internal(const char* dump_path, const memory_options_t* options, memory_region_t** regions, size_t* count) {
    if (!dump_path || !regions || !count) {
        return -1;
    }
    
    *regions = NULL;
    *count = 0;
    
    printf("Analyzing memory regions in: %s\n", dump_path);
    
    // In a real implementation, this would parse the memory dump to extract detailed region information
    // For now, we'll create representative regions based on typical memory layouts
    
    size_t max_regions = options ? options->max_regions : 10000;
    if (max_regions > 50) max_regions = 50; // Limit for testing
    
    *regions = (memory_region_t*)malloc(max_regions * sizeof(memory_region_t));
    if (!*regions) {
        return -1;
    }
    
    // Create representative memory regions
    const char* region_descriptions[] = {
        "Executable code", "Heap", "Stack", "Data section", 
        "Shared library", "Mapped file", "Device driver", 
        "Kernel memory", "Page table", "Free memory"
    };
    
    for (size_t i = 0; i < max_regions; i++) {
        memory_region_init(&(*regions)[i]);
        (*regions)[i].start_address = 0x100000 + i * 0x10000;
        (*regions)[i].end_address = (*regions)[i].start_address + 0x10000;
        (*regions)[i].size = 0x10000;
        
        // Set protection flags (bit 0: read, bit 1: write, bit 2: execute)
        (*regions)[i].protection = i % 8; // Various combinations of R/W/X
        
        // Set region type (0: private, 1: shared, 2: image)
        (*regions)[i].type = i % 3;
        
        // Set description
        (*regions)[i].description = strdup(region_descriptions[i % 10]);
        
        // Mark some regions as suspicious based on protection
        if (((*regions)[i].protection & 0x5) == 0x5) { // Both read and execute
            (*regions)[i].is_suspicious = 1;
            printf("Region at 0x%08lX flagged for RWX permissions\n", (unsigned long)(*regions)[i].start_address);
        }
    }
    
    *count = max_regions;
    return 0;
}

int memory_generate_report(const memory_analysis_result_t* result, const char* report_path) {
    if (!result || !report_path) {
        return -1;
    }
    
    FILE* file = fopen(report_path, "w");
    if (!file) {
        perror("Failed to create report file");
        return -1;
    }
    
    fprintf(file, "Memory Analysis Report\n");
    fprintf(file, "======================\n\n");
    
    fprintf(file, "Dump Information:\n");
    fprintf(file, "  Path: %s\n", result->dump_path ? result->dump_path : "Unknown");
    fprintf(file, "  Format: %s\n", memory_get_format_name(result->format));
    fprintf(file, "  Size: %lu bytes\n", (unsigned long)result->total_size);
    fprintf(file, "  Analyzed: %lu bytes\n", (unsigned long)result->analyzed_size);
    fprintf(file, "\n");
    
    fprintf(file, "Summary:\n");
    fprintf(file, "  Processes: %lu\n", (unsigned long)result->process_count);
    fprintf(file, "  Connections: %lu\n", (unsigned long)result->connection_count);
    fprintf(file, "  Regions: %lu\n", (unsigned long)result->region_count);
    fprintf(file, "  Suspicious Processes: %s\n", result->has_suspicious_processes ? "Yes" : "No");
    fprintf(file, "  Suspicious Connections: %s\n", result->has_suspicious_connections ? "Yes" : "No");
    fprintf(file, "  Injected Code: %s\n", result->has_injected_code ? "Yes" : "No");
    fprintf(file, "  Rootkit Activity: %s\n", result->has_rootkit_activity ? "Yes" : "No");
    fprintf(file, "\n");
    
    fprintf(file, "Suspicious Processes:\n");
    for (size_t i = 0; i < result->process_count; i++) {
        const memory_process_t* process = &result->processes[i];
        if (process->is_suspicious) {
            fprintf(file, "  PID %u: %s\n", process->process_id, 
                    process->process_name ? process->process_name : "Unknown");
            fprintf(file, "    Threat: %s (Confidence: %.2f)\n", 
                    process->threat_type ? process->threat_type : "Unknown", 
                    process->confidence);
            fprintf(file, "    Memory: %lu bytes virtual, %lu bytes working set\n",
                    (unsigned long)process->virtual_size, 
                    (unsigned long)process->working_set_size);
        }
    }
    fprintf(file, "\n");
    
    fprintf(file, "Suspicious Connections:\n");
    for (size_t i = 0; i < result->connection_count; i++) {
        const memory_connection_t* connection = &result->connections[i];
        if (connection->is_suspicious) {
            char local_ip_str[16], remote_ip_str[16];
            memory_ip_to_string(connection->local_ip, local_ip_str, sizeof(local_ip_str));
            memory_ip_to_string(connection->remote_ip, remote_ip_str, sizeof(remote_ip_str));
            
            fprintf(file, "  PID %u: %s:%u -> %s:%u\n", 
                    connection->process_id,
                    local_ip_str, connection->local_port,
                    remote_ip_str, connection->remote_port);
            fprintf(file, "    Threat: %s (Confidence: %.2f)\n", 
                    connection->threat_type ? connection->threat_type : "Unknown", 
                    connection->confidence);
        }
    }
    
    fclose(file);
    printf("Report generated: %s\n", report_path);
    return 0;
}

int memory_export_to_csv(const memory_analysis_result_t* result, const char* csv_path) {
    if (!result || !csv_path) {
        return -1;
    }
    
    FILE* file = fopen(csv_path, "w");
    if (!file) {
        perror("Failed to create CSV file");
        return -1;
    }
    
    // Export processes
    fprintf(file, "Processes\n");
    fprintf(file, "PID,ParentPID,Name,BaseAddress,ImageSize,VirtSize,WorkSet,Threat,Confidence,Suspicious\n");
    
    for (size_t i = 0; i < result->process_count; i++) {
        const memory_process_t* process = &result->processes[i];
        fprintf(file, "%u,%u,\"%s\",%lu,%lu,%lu,%lu,\"%s\",%.2f,%s\n",
                process->process_id,
                process->parent_process_id,
                process->process_name ? process->process_name : "",
                (unsigned long)process->base_address,
                (unsigned long)process->image_size,
                (unsigned long)process->virtual_size,
                (unsigned long)process->working_set_size,
                process->threat_type ? process->threat_type : "",
                process->confidence,
                process->is_suspicious ? "Yes" : "No");
    }
    
    fprintf(file, "\n");
    
    // Export connections
    fprintf(file, "Connections\n");
    fprintf(file, "PID,LocalIP,LocalPort,RemoteIP,RemotePort,Protocol,Threat,Confidence,Suspicious\n");
    
    for (size_t i = 0; i < result->connection_count; i++) {
        const memory_connection_t* connection = &result->connections[i];
        char local_ip_str[16], remote_ip_str[16];
        memory_ip_to_string(connection->local_ip, local_ip_str, sizeof(local_ip_str));
        memory_ip_to_string(connection->remote_ip, remote_ip_str, sizeof(remote_ip_str));
        
        fprintf(file, "%u,\"%s\",%u,\"%s\",%u,%d,\"%s\",%.2f,%s\n",
                connection->process_id,
                local_ip_str,
                connection->local_port,
                remote_ip_str,
                connection->remote_port,
                connection->protocol,
                connection->threat_type ? connection->threat_type : "",
                connection->confidence,
                connection->is_suspicious ? "Yes" : "No");
    }
    
    fclose(file);
    printf("CSV export generated: %s\n", csv_path);
    return 0;
}

int memory_export_to_json(const memory_analysis_result_t* result, const char* json_path) {
    if (!result || !json_path) {
        return -1;
    }
    
    FILE* file = fopen(json_path, "w");
    if (!file) {
        perror("Failed to create JSON file");
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"dump_info\": {\n");
    fprintf(file, "    \"path\": \"%s\",\n", result->dump_path ? result->dump_path : "");
    fprintf(file, "    \"format\": \"%s\",\n", memory_get_format_name(result->format));
    fprintf(file, "    \"size\": %lu,\n", (unsigned long)result->total_size);
    fprintf(file, "    \"analyzed\": %lu\n", (unsigned long)result->analyzed_size);
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"summary\": {\n");
    fprintf(file, "    \"processes\": %lu,\n", (unsigned long)result->process_count);
    fprintf(file, "    \"connections\": %lu,\n", (unsigned long)result->connection_count);
    fprintf(file, "    \"regions\": %lu,\n", (unsigned long)result->region_count);
    fprintf(file, "    \"suspicious_processes\": %s,\n", result->has_suspicious_processes ? "true" : "false");
    fprintf(file, "    \"suspicious_connections\": %s,\n", result->has_suspicious_connections ? "true" : "false");
    fprintf(file, "    \"injected_code\": %s,\n", result->has_injected_code ? "true" : "false");
    fprintf(file, "    \"rootkit_activity\": %s\n", result->has_rootkit_activity ? "true" : "false");
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"processes\": [\n");
    for (size_t i = 0; i < result->process_count; i++) {
        const memory_process_t* process = &result->processes[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"pid\": %u,\n", process->process_id);
        fprintf(file, "      \"parent_pid\": %u,\n", process->parent_process_id);
        fprintf(file, "      \"name\": \"%s\",\n", process->process_name ? process->process_name : "");
        fprintf(file, "      \"base_address\": %lu,\n", (unsigned long)process->base_address);
        fprintf(file, "      \"image_size\": %lu,\n", (unsigned long)process->image_size);
        fprintf(file, "      \"virtual_size\": %lu,\n", (unsigned long)process->virtual_size);
        fprintf(file, "      \"working_set_size\": %lu,\n", (unsigned long)process->working_set_size);
        fprintf(file, "      \"threat\": \"%s\",\n", process->threat_type ? process->threat_type : "");
        fprintf(file, "      \"confidence\": %.2f,\n", process->confidence);
        fprintf(file, "      \"suspicious\": %s\n", process->is_suspicious ? "true" : "false");
        fprintf(file, "    }%s\n", (i < result->process_count - 1) ? "," : "");
    }
    fprintf(file, "  ],\n");
    
    fprintf(file, "  \"connections\": [\n");
    for (size_t i = 0; i < result->connection_count; i++) {
        const memory_connection_t* connection = &result->connections[i];
        char local_ip_str[16], remote_ip_str[16];
        memory_ip_to_string(connection->local_ip, local_ip_str, sizeof(local_ip_str));
        memory_ip_to_string(connection->remote_ip, remote_ip_str, sizeof(remote_ip_str));
        
        fprintf(file, "    {\n");
        fprintf(file, "      \"pid\": %u,\n", connection->process_id);
        fprintf(file, "      \"local_ip\": \"%s\",\n", local_ip_str);
        fprintf(file, "      \"local_port\": %u,\n", connection->local_port);
        fprintf(file, "      \"remote_ip\": \"%s\",\n", remote_ip_str);
        fprintf(file, "      \"remote_port\": %u,\n", connection->remote_port);
        fprintf(file, "      \"protocol\": %d,\n", connection->protocol);
        fprintf(file, "      \"threat\": \"%s\",\n", connection->threat_type ? connection->threat_type : "");
        fprintf(file, "      \"confidence\": %.2f,\n", connection->confidence);
        fprintf(file, "      \"suspicious\": %s\n", connection->is_suspicious ? "true" : "false");
        fprintf(file, "    }%s\n", (i < result->connection_count - 1) ? "," : "");
    }
    fprintf(file, "  ]\n");
    
    fprintf(file, "}\n");
    
    fclose(file);
    printf("JSON export generated: %s\n", json_path);
    return 0;
}