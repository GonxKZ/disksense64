#include "memory.h"
#include "dumps.h"
#include "processes.h"
#include "network.h"
#include "analysis.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void memory_options_init(memory_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(memory_options_t));
        options->analyze_processes = 1;
        options->analyze_network = 1;
        options->analyze_regions = 1;
        options->detect_malware = 1;
        options->detect_rootkits = 1;
        options->detect_code_injection = 1;
        options->extract_strings = 1;
        options->max_processes = 1000;
        options->max_connections = 5000;
        options->max_regions = 10000;
    }
}

void memory_options_free(memory_options_t* options) {
    if (options) {
        if (options->exclude_processes) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_processes[i]);
            }
            free(options->exclude_processes);
        }
        memset(options, 0, sizeof(memory_options_t));
    }
}

void memory_analysis_result_init(memory_analysis_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(memory_analysis_result_t));
    }
}

int memory_analysis_result_add_process(memory_analysis_result_t* result, const memory_process_t* process) {
    if (!result || !process) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->process_count >= result->process_capacity) {
        size_t new_capacity = (result->process_capacity == 0) ? 16 : result->process_capacity * 2;
        memory_process_t* new_processes = (memory_process_t*)realloc(result->processes, new_capacity * sizeof(memory_process_t));
        if (!new_processes) {
            return -1;
        }
        result->processes = new_processes;
        result->process_capacity = new_capacity;
    }
    
    // Copy process data
    memory_process_t* new_process = &result->processes[result->process_count];
    memory_process_init(new_process);
    new_process->process_id = process->process_id;
    new_process->parent_process_id = process->parent_process_id;
    new_process->process_name = strdup_safe(process->process_name);
    new_process->base_address = process->base_address;
    new_process->image_size = process->image_size;
    new_process->thread_count = process->thread_count;
    new_process->virtual_size = process->virtual_size;
    new_process->working_set_size = process->working_set_size;
    new_process->is_suspicious = process->is_suspicious;
    new_process->threat_type = strdup_safe(process->threat_type);
    new_process->confidence = process->confidence;
    
    if ((!new_process->process_name || !new_process->threat_type) && 
        (process->process_name || process->threat_type)) {
        // Clean up on failure
        memory_process_free(new_process);
        return -1;
    }
    
    result->process_count++;
    
    if (process->is_suspicious) {
        result->has_suspicious_processes = 1;
    }
    
    return 0;
}

int memory_analysis_result_add_connection(memory_analysis_result_t* result, const memory_connection_t* connection) {
    if (!result || !connection) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->connection_count >= result->connection_capacity) {
        size_t new_capacity = (result->connection_capacity == 0) ? 16 : result->connection_capacity * 2;
        memory_connection_t* new_connections = (memory_connection_t*)realloc(result->connections, new_capacity * sizeof(memory_connection_t));
        if (!new_connections) {
            return -1;
        }
        result->connections = new_connections;
        result->connection_capacity = new_capacity;
    }
    
    // Copy connection data
    memory_connection_t* new_connection = &result->connections[result->connection_count];
    memory_connection_init(new_connection);
    new_connection->process_id = connection->process_id;
    new_connection->local_ip = connection->local_ip;
    new_connection->remote_ip = connection->remote_ip;
    new_connection->local_port = connection->local_port;
    new_connection->remote_port = connection->remote_port;
    new_connection->protocol = connection->protocol;
    new_connection->state = connection->state;
    new_connection->is_suspicious = connection->is_suspicious;
    new_connection->threat_type = strdup_safe(connection->threat_type);
    new_connection->confidence = connection->confidence;
    
    if (new_connection->threat_type == NULL && connection->threat_type != NULL) {
        // Clean up on failure
        memory_connection_free(new_connection);
        return -1;
    }
    
    result->connection_count++;
    
    if (connection->is_suspicious) {
        result->has_suspicious_connections = 1;
    }
    
    return 0;
}

int memory_analysis_result_add_region(memory_analysis_result_t* result, const memory_region_t* region) {
    if (!result || !region) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->region_count >= result->region_capacity) {
        size_t new_capacity = (result->region_capacity == 0) ? 16 : result->region_capacity * 2;
        memory_region_t* new_regions = (memory_region_t*)realloc(result->regions, new_capacity * sizeof(memory_region_t));
        if (!new_regions) {
            return -1;
        }
        result->regions = new_regions;
        result->region_capacity = new_capacity;
    }
    
    // Copy region data
    memory_region_t* new_region = &result->regions[result->region_count];
    memory_region_init(new_region);
    new_region->start_address = region->start_address;
    new_region->end_address = region->end_address;
    new_region->size = region->size;
    new_region->protection = region->protection;
    new_region->type = region->type;
    new_region->is_suspicious = region->is_suspicious;
    new_region->description = strdup_safe(region->description);
    
    if (new_region->description == NULL && region->description != NULL) {
        // Clean up on failure
        memory_region_free(new_region);
        return -1;
    }
    
    result->region_count++;
    
    return 0;
}

void memory_analysis_result_free(memory_analysis_result_t* result) {
    if (result) {
        free(result->dump_path);
        
        if (result->processes) {
            for (size_t i = 0; i < result->process_count; i++) {
                memory_process_free(&result->processes[i]);
            }
            free(result->processes);
        }
        
        if (result->connections) {
            for (size_t i = 0; i < result->connection_count; i++) {
                memory_connection_free(&result->connections[i]);
            }
            free(result->connections);
        }
        
        if (result->regions) {
            for (size_t i = 0; i < result->region_count; i++) {
                memory_region_free(&result->regions[i]);
            }
            free(result->regions);
        }
        
        memset(result, 0, sizeof(memory_analysis_result_t));
    }
}

void memory_process_init(memory_process_t* process) {
    if (process) {
        memset(process, 0, sizeof(memory_process_t));
    }
}

void memory_process_free(memory_process_t* process) {
    if (process) {
        free(process->process_name);
        free(process->threat_type);
        memset(process, 0, sizeof(memory_process_t));
    }
}

void memory_connection_init(memory_connection_t* connection) {
    if (connection) {
        memset(connection, 0, sizeof(memory_connection_t));
    }
}

void memory_connection_free(memory_connection_t* connection) {
    if (connection) {
        free(connection->threat_type);
        memset(connection, 0, sizeof(memory_connection_t));
    }
}

void memory_region_init(memory_region_t* region) {
    if (region) {
        memset(region, 0, sizeof(memory_region_t));
    }
}

void memory_region_free(memory_region_t* region) {
    if (region) {
        free(region->description);
        memset(region, 0, sizeof(memory_region_t));
    }
}

void memory_string_init(memory_string_t* string) {
    if (string) {
        memset(string, 0, sizeof(memory_string_t));
    }
}

void memory_string_free(memory_string_t* string) {
    if (string) {
        free(string->string_value);
        free(string->context);
        memset(string, 0, sizeof(memory_string_t));
    }
}

memory_dump_format_t memory_detect_format(const char* dump_path) {
    if (!dump_path) {
        return MEMORY_DUMP_UNKNOWN;
    }
    
    // Check file extension
    const char* ext = strrchr(dump_path, '.');
    if (ext) {
        if (strcasecmp(ext, ".raw") == 0) {
            return MEMORY_DUMP_RAW;
        } else if (strcasecmp(ext, ".dmp") == 0) {
            return MEMORY_DUMP_CRASHDUMP;
        } else if (strcasecmp(ext, ".lime") == 0) {
            return MEMORY_DUMP_LIME;
        } else if (strcasecmp(ext, ".avml") == 0) {
            return MEMORY_DUMP_AVML;
        } else if (strcasecmp(ext, ".vmss") == 0) {
            return MEMORY_DUMP_VMSS;
        } else if (strcasecmp(ext, ".vmsn") == 0) {
            return MEMORY_DUMP_VMSN;
        }
    }
    
    // Try to read file header to determine format
    FILE* file = fopen(dump_path, "rb");
    if (!file) {
        return MEMORY_DUMP_UNKNOWN;
    }
    
    uint8_t header[16];
    size_t bytes_read = fread(header, 1, sizeof(header), file);
    fclose(file);
    
    if (bytes_read < 4) {
        return MEMORY_DUMP_UNKNOWN;
    }
    
    // Check for known signatures
    if (header[0] == 0x50 && header[1] == 0x41 && header[2] == 0x47 && header[3] == 0x45) {
        // LIME signature "PAGE"
        return MEMORY_DUMP_LIME;
    }
    
    // Default to raw
    return MEMORY_DUMP_RAW;
}

int memory_analyze_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result) {
    if (!dump_path || !result) {
        return -1;
    }
    
    memory_analysis_result_init(result);
    
    // Set dump path and format
    result->dump_path = strdup_safe(dump_path);
    result->format = memory_detect_format(dump_path);
    
    // Get file size
    FILE* file = fopen(dump_path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        result->total_size = ftell(file);
        result->analyzed_size = result->total_size;
        fclose(file);
    }
    
    printf("Analyzing memory dump: %s\n", dump_path);
    printf("Format: %s\n", memory_get_format_name(result->format));
    printf("Size: %lu bytes\n", (unsigned long)result->total_size);
    
    // Extract processes if requested
    if (!options || options->analyze_processes) {
        memory_process_t* processes;
        size_t process_count;
        
        int ret = memory_extract_processes(dump_path, options, &processes, &process_count);
        if (ret == 0) {
            printf("Extracted %lu processes\n", (unsigned long)process_count);
            
            // Add processes to result
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
        
        int ret = memory_extract_connections(dump_path, options, &connections, &connection_count);
        if (ret == 0) {
            printf("Extracted %lu connections\n", (unsigned long)connection_count);
            
            // Add connections to result
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
        
        int ret = memory_extract_regions(dump_path, options, &regions, &region_count);
        if (ret == 0) {
            printf("Extracted %lu memory regions\n", (unsigned long)region_count);
            
            // Add regions to result
            for (size_t i = 0; i < region_count; i++) {
                memory_analysis_result_add_region(result, &regions[i]);
                memory_region_free(&regions[i]);
            }
            
            free(regions);
        }
    }
    
    // Detect suspicious activities
    if (!options || options->detect_malware) {
        size_t suspicious_processes;
        memory_detect_suspicious_processes(result->processes, result->process_count, &suspicious_processes);
        
        size_t suspicious_connections;
        memory_detect_suspicious_connections(result->connections, result->connection_count, &suspicious_connections);
        
        printf("Found %lu suspicious processes and %lu suspicious connections\n", 
               (unsigned long)suspicious_processes, (unsigned long)suspicious_connections);
    }
    
    // Detect code injection
    if (!options || options->detect_code_injection) {
        size_t injected_count;
        memory_detect_code_injection(dump_path, result->processes, result->process_count, &injected_count);
        if (injected_count > 0) {
            result->has_injected_code = 1;
            printf("Detected code injection in %lu processes\n", (unsigned long)injected_count);
        }
    }
    
    // Detect rootkit activity
    if (!options || options->detect_rootkits) {
        int is_rootkit_detected;
        memory_detect_rootkit_activity(dump_path, result, &is_rootkit_detected);
        if (is_rootkit_detected) {
            result->has_rootkit_activity = 1;
            printf("Detected potential rootkit activity\n");
        }
    }
    
    return 0;
}

int memory_extract_processes(const char* dump_path, const memory_options_t* options, 
                           memory_process_t** processes, size_t* count) {
    if (!dump_path || !processes || !count) {
        return -1;
    }
    
    *processes = NULL;
    *count = 0;
    
    // In a real implementation, this would parse the memory dump to extract processes
    // For now, we'll create some dummy processes
    
    size_t max_processes = options ? options->max_processes : 1000;
    if (max_processes > 10) max_processes = 10; // Limit for testing
    
    *processes = (memory_process_t*)malloc(max_processes * sizeof(memory_process_t));
    if (!*processes) {
        return -1;
    }
    
    // Create dummy processes
    const char* process_names[] = {
        "System", "smss.exe", "csrss.exe", "wininit.exe", "winlogon.exe",
        "services.exe", "lsass.exe", "svchost.exe", "explorer.exe", "chrome.exe"
    };
    
    for (size_t i = 0; i < max_processes; i++) {
        memory_process_init(&(*processes)[i]);
        (*processes)[i].process_id = 4 + i * 100;
        (*processes)[i].parent_process_id = (i == 0) ? 0 : 4 + (i-1) * 100;
        (*processes)[i].process_name = strdup_safe(process_names[i % 10]);
        (*processes)[i].base_address = 0x400000 + i * 0x100000;
        (*processes)[i].image_size = 0x10000 + i * 0x1000;
        (*processes)[i].thread_count = 5 + i % 10;
        (*processes)[i].virtual_size = 0x100000 + i * 0x10000;
        (*processes)[i].working_set_size = 0x50000 + i * 0x5000;
        
        // Mark some processes as suspicious
        if (i == 5 || i == 8) {
            (*processes)[i].is_suspicious = 1;
            (*processes)[i].threat_type = strdup_safe("Suspicious process");
            (*processes)[i].confidence = 0.8;
        }
    }
    
    *count = max_processes;
    return 0;
}

int memory_extract_connections(const char* dump_path, const memory_options_t* options,
                             memory_connection_t** connections, size_t* count) {
    if (!dump_path || !connections || !count) {
        return -1;
    }
    
    *connections = NULL;
    *count = 0;
    
    // In a real implementation, this would parse the memory dump to extract network connections
    // For now, we'll create some dummy connections
    
    size_t max_connections = options ? options->max_connections : 5000;
    if (max_connections > 20) max_connections = 20; // Limit for testing
    
    *connections = (memory_connection_t*)malloc(max_connections * sizeof(memory_connection_t));
    if (!*connections) {
        return -1;
    }
    
    // Create dummy connections
    for (size_t i = 0; i < max_connections; i++) {
        memory_connection_init(&(*connections)[i]);
        (*connections)[i].process_id = 1000 + i % 10 * 100;
        (*connections)[i].local_ip = 0xC0A80101 + i; // 192.168.1.1 + i
        (*connections)[i].remote_ip = 0x08080808 + i; // 8.8.8.8 + i
        (*connections)[i].local_port = 1024 + i;
        (*connections)[i].remote_port = 80 + i % 100;
        (*connections)[i].protocol = (i % 3 == 0) ? 6 : 17; // TCP or UDP
        (*connections)[i].state = 1; // Established
        
        // Mark some connections as suspicious
        if (i == 3 || i == 7 || i == 15) {
            (*connections)[i].is_suspicious = 1;
            (*connections)[i].threat_type = strdup_safe("Suspicious connection");
            (*connections)[i].confidence = 0.9;
        }
    }
    
    *count = max_connections;
    return 0;
}

int memory_extract_regions(const char* dump_path, const memory_options_t* options,
                         memory_region_t** regions, size_t* count) {
    if (!dump_path || !regions || !count) {
        return -1;
    }
    
    *regions = NULL;
    *count = 0;
    
    // In a real implementation, this would parse the memory dump to extract memory regions
    // For now, we'll create some dummy regions
    
    size_t max_regions = options ? options->max_regions : 10000;
    if (max_regions > 15) max_regions = 15; // Limit for testing
    
    *regions = (memory_region_t*)malloc(max_regions * sizeof(memory_region_t));
    if (!*regions) {
        return -1;
    }
    
    // Create dummy regions
    for (size_t i = 0; i < max_regions; i++) {
        memory_region_init(&(*regions)[i]);
        (*regions)[i].start_address = 0x100000 + i * 0x10000;
        (*regions)[i].end_address = (*regions)[i].start_address + 0x10000;
        (*regions)[i].size = 0x10000;
        (*regions)[i].protection = (i % 4); // Read/Write/Execute flags
        (*regions)[i].type = (i % 3); // Private/Shared/Image
        (*regions)[i].description = strdup_safe("Memory region");
    }
    
    *count = max_regions;
    return 0;
}

int memory_extract_strings(const char* dump_path, size_t min_length,
                         memory_string_t** strings, size_t* count) {
    if (!dump_path || !strings || !count) {
        return -1;
    }
    
    *strings = NULL;
    *count = 0;
    
    // In a real implementation, this would extract strings from the memory dump
    // For now, we'll create some dummy strings
    
    size_t max_strings = 50;
    *strings = (memory_string_t*)malloc(max_strings * sizeof(memory_string_t));
    if (!*strings) {
        return -1;
    }
    
    // Create dummy strings
    const char* sample_strings[] = {
        "Hello World", "C:\\Windows\\System32\\", "http://example.com",
        "malware.exe", "cmd.exe", "C:\\Users\\Admin\\", "password123",
        "kernel32.dll", "user32.dll", "ntdll.dll"
    };
    
    for (size_t i = 0; i < max_strings && i < 10; i++) {
        memory_string_init(&(*strings)[i]);
        (*strings)[i].string_value = strdup_safe(sample_strings[i]);
        (*strings)[i].address = 0x100000 + i * 0x1000;
        (*strings)[i].length = strlen(sample_strings[i]);
        (*strings)[i].context = strdup_safe("Process memory");
        
        // Mark some strings as suspicious
        if (i == 3 || i == 6) {
            (*strings)[i].is_suspicious = 1;
        }
    }
    
    *count = (max_strings < 10) ? max_strings : 10;
    return 0;
}

int memory_detect_suspicious_processes(const memory_process_t* processes, size_t count, size_t* suspicious_count) {
    if (!processes || !suspicious_count) {
        return -1;
    }
    
    *suspicious_count = 0;
    
    // Count suspicious processes
    for (size_t i = 0; i < count; i++) {
        if (processes[i].is_suspicious) {
            (*suspicious_count)++;
        }
    }
    
    return 0;
}

int memory_detect_suspicious_connections(const memory_connection_t* connections, size_t count, size_t* suspicious_count) {
    if (!connections || !suspicious_count) {
        return -1;
    }
    
    *suspicious_count = 0;
    
    // Count suspicious connections
    for (size_t i = 0; i < count; i++) {
        if (connections[i].is_suspicious) {
            (*suspicious_count)++;
        }
    }
    
    return 0;
}

int memory_detect_code_injection(const char* dump_path, const memory_process_t* processes, size_t count, size_t* injected_count) {
    if (!dump_path || !processes || !injected_count) {
        return -1;
    }
    
    *injected_count = 0;
    
    // In a real implementation, this would analyze process memory for injection indicators
    // For now, we'll simulate detection based on process names
    
    for (size_t i = 0; i < count; i++) {
        if (processes[i].process_name) {
            // Check for suspicious process names that might indicate injection
            if (strstr(processes[i].process_name, "inject") != NULL ||
                strstr(processes[i].process_name, "malware") != NULL) {
                (*injected_count)++;
            }
        }
    }
    
    return 0;
}

int memory_detect_rootkit_activity(const char* dump_path, const memory_analysis_result_t* result, int* is_rootkit_detected) {
    if (!dump_path || !result || !is_rootkit_detected) {
        return -1;
    }
    
    *is_rootkit_detected = 0;
    
    // In a real implementation, this would look for rootkit indicators
    // For now, we'll check for suspicious combinations
    
    // If we have many suspicious processes and connections, flag as potential rootkit
    if (result->has_suspicious_processes && result->has_suspicious_connections) {
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
        
        if (suspicious_process_count > 3 && suspicious_connection_count > 5) {
            *is_rootkit_detected = 1;
        }
    }
    
    return 0;
}

const char* memory_get_format_name(memory_dump_format_t format) {
    switch (format) {
        case MEMORY_DUMP_RAW:
            return "Raw";
        case MEMORY_DUMP_CRASHDUMP:
            return "Crash Dump";
        case MEMORY_DUMP_LIME:
            return "LiME";
        case MEMORY_DUMP_AVML:
            return "AVML";
        case MEMORY_DUMP_VMSS:
            return "VMware Snapshot";
        case MEMORY_DUMP_VMSN:
            return "VMware Memory";
        default:
            return "Unknown";
    }
}

const char* memory_get_format_description(memory_dump_format_t format) {
    switch (format) {
        case MEMORY_DUMP_RAW:
            return "Raw memory dump";
        case MEMORY_DUMP_CRASHDUMP:
            return "Windows crash dump";
        case MEMORY_DUMP_LIME:
            return "LiME (Linux Memory Extractor) format";
        case MEMORY_DUMP_AVML:
            return "Azure Virtual Machine Memory";
        case MEMORY_DUMP_VMSS:
            return "VMware snapshot file";
        case MEMORY_DUMP_VMSN:
            return "VMware memory file";
        default:
            return "Unknown memory dump format";
    }
}

int memory_ip_to_string(uint32_t ip, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 16) {
        return -1;
    }
    
    struct in_addr addr;
    addr.s_addr = ip;
    const char* ip_str = inet_ntoa(addr);
    if (ip_str) {
        strncpy(buffer, ip_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return 0;
    }
    
    return -1;
}