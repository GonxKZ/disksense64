#ifndef LIBS_MEMORY_MEMORY_H
#define LIBS_MEMORY_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory dump formats
typedef enum {
    MEMORY_DUMP_UNKNOWN = 0,
    MEMORY_DUMP_RAW = 1,
    MEMORY_DUMP_CRASHDUMP = 2,
    MEMORY_DUMP_LIME = 3,
    MEMORY_DUMP_AVML = 4,
    MEMORY_DUMP_VMSS = 5,
    MEMORY_DUMP_VMSN = 6
} memory_dump_format_t;

// Process information from memory dump
typedef struct {
    uint32_t process_id;
    uint32_t parent_process_id;
    char* process_name;
    uint64_t base_address;
    uint64_t image_size;
    uint32_t thread_count;
    uint64_t virtual_size;
    uint64_t working_set_size;
    int is_suspicious;
    char* threat_type;
    double confidence;
} memory_process_t;

// Network connection from memory dump
typedef struct {
    uint32_t process_id;
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    int protocol;  // 6=TCP, 17=UDP
    int state;     // TCP state
    int is_suspicious;
    char* threat_type;
    double confidence;
} memory_connection_t;

// Memory region information
typedef struct {
    uint64_t start_address;
    uint64_t end_address;
    uint64_t size;
    int protection;  // Read/Write/Execute flags
    int type;        // Private/Shared/Image
    int is_suspicious;
    char* description;
} memory_region_t;

// Memory analysis result
typedef struct {
    char* dump_path;
    memory_dump_format_t format;
    uint64_t total_size;
    uint64_t analyzed_size;
    memory_process_t* processes;
    size_t process_count;
    size_t process_capacity;
    memory_connection_t* connections;
    size_t connection_count;
    size_t connection_capacity;
    memory_region_t* regions;
    size_t region_count;
    size_t region_capacity;
    int has_suspicious_processes;
    int has_suspicious_connections;
    int has_injected_code;
    int has_rootkit_activity;
} memory_analysis_result_t;

// Memory analysis options
typedef struct {
    int analyze_processes;        // Analyze processes
    int analyze_network;          // Analyze network connections
    int analyze_regions;          // Analyze memory regions
    int detect_malware;           // Detect malware indicators
    int detect_rootkits;          // Detect rootkit activity
    int detect_code_injection;    // Detect code injection
    int extract_strings;          // Extract strings from memory
    size_t max_processes;         // Maximum number of processes to analyze
    size_t max_connections;       // Maximum number of connections to analyze
    size_t max_regions;           // Maximum number of regions to analyze
    char** exclude_processes;     // Processes to exclude from analysis
    size_t exclude_count;         // Number of excluded processes
} memory_options_t;

// String found in memory
typedef struct {
    char* string_value;
    uint64_t address;
    size_t length;
    int is_suspicious;
    char* context;
} memory_string_t;

// Initialize memory analysis options with default values
void memory_options_init(memory_options_t* options);

// Free memory analysis options
void memory_options_free(memory_options_t* options);

// Initialize memory analysis result
void memory_analysis_result_init(memory_analysis_result_t* result);

// Add process to analysis result
int memory_analysis_result_add_process(memory_analysis_result_t* result, const memory_process_t* process);

// Add connection to analysis result
int memory_analysis_result_add_connection(memory_analysis_result_t* result, const memory_connection_t* connection);

// Add region to analysis result
int memory_analysis_result_add_region(memory_analysis_result_t* result, const memory_region_t* region);

// Free memory analysis result
void memory_analysis_result_free(memory_analysis_result_t* result);

// Initialize process information
void memory_process_init(memory_process_t* process);

// Free process information
void memory_process_free(memory_process_t* process);

// Initialize network connection
void memory_connection_init(memory_connection_t* connection);

// Free network connection
void memory_connection_free(memory_connection_t* connection);

// Initialize memory region
void memory_region_init(memory_region_t* region);

// Free memory region
void memory_region_free(memory_region_t* region);

// Initialize memory string
void memory_string_init(memory_string_t* string);

// Free memory string
void memory_string_free(memory_string_t* string);

// Detect memory dump format
// dump_path: path to the memory dump file
// Returns detected format, or MEMORY_DUMP_UNKNOWN on error
memory_dump_format_t memory_detect_format(const char* dump_path);

// Analyze memory dump
// dump_path: path to the memory dump file
// options: analysis options
// result: output analysis result (must be freed with memory_analysis_result_free)
// Returns 0 on success, non-zero on error
int memory_analyze_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Extract processes from memory dump
// dump_path: path to the memory dump file
// options: analysis options
// processes: output array of processes (must be freed)
// count: output number of processes
// Returns 0 on success, non-zero on error
int memory_extract_processes(const char* dump_path, const memory_options_t* options, 
                           memory_process_t** processes, size_t* count);

// Extract network connections from memory dump
// dump_path: path to the memory dump file
// options: analysis options
// connections: output array of connections (must be freed)
// count: output number of connections
// Returns 0 on success, non-zero on error
int memory_extract_connections(const char* dump_path, const memory_options_t* options,
                             memory_connection_t** connections, size_t* count);

// Extract memory regions from memory dump
// dump_path: path to the memory dump file
// options: analysis options
// regions: output array of regions (must be freed)
// count: output number of regions
// Returns 0 on success, non-zero on error
int memory_extract_regions(const char* dump_path, const memory_options_t* options,
                         memory_region_t** regions, size_t* count);

// Extract strings from memory dump
// dump_path: path to the memory dump file
// min_length: minimum string length to extract
// strings: output array of strings (must be freed)
// count: output number of strings
// Returns 0 on success, non-zero on error
int memory_extract_strings(const char* dump_path, size_t min_length,
                         memory_string_t** strings, size_t* count);

// Detect suspicious processes
// processes: array of processes
// count: number of processes
// suspicious_count: output number of suspicious processes
// Returns 0 on success, non-zero on error
int memory_detect_suspicious_processes(const memory_process_t* processes, size_t count, size_t* suspicious_count);

// Detect suspicious connections
// connections: array of connections
// count: number of connections
// suspicious_count: output number of suspicious connections
// Returns 0 on success, non-zero on error
int memory_detect_suspicious_connections(const memory_connection_t* connections, size_t count, size_t* suspicious_count);

// Detect code injection
// dump_path: path to the memory dump file
// processes: array of processes
// count: number of processes
// injected_count: output number of processes with injected code
// Returns 0 on success, non-zero on error
int memory_detect_code_injection(const char* dump_path, const memory_process_t* processes, size_t count, size_t* injected_count);

// Detect rootkit activity
// dump_path: path to the memory dump file
// result: analysis result with processes and connections
// is_rootkit_detected: output flag indicating if rootkit activity is detected
// Returns 0 on success, non-zero on error
int memory_detect_rootkit_activity(const char* dump_path, const memory_analysis_result_t* result, int* is_rootkit_detected);

// Get dump format name
// format: dump format
// Returns name of the dump format
const char* memory_get_format_name(memory_dump_format_t format);

// Get dump format description
// format: dump format
// Returns description of the dump format
const char* memory_get_format_description(memory_dump_format_t format);

// Convert IP address to string
// ip: IP address in network byte order
// buffer: output buffer for string representation
// buffer_size: size of output buffer
// Returns 0 on success, non-zero on error
int memory_ip_to_string(uint32_t ip, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MEMORY_MEMORY_H