#ifndef LIBS_MEMORY_PROCESSES_H
#define LIBS_MEMORY_PROCESSES_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extract processes from memory dump
int memory_extract_processes_internal(const char* dump_path, const memory_options_t* options, 
                                    memory_process_t** processes, size_t* count);

// Analyze process for suspicious behavior
int memory_analyze_process_behavior(const memory_process_t* process, int* is_suspicious, char** threat_type, double* confidence);

// Check process for known malware signatures
int memory_check_process_malware(const memory_process_t* process, int* is_malware, char** malware_name);

// Check process for rootkit characteristics
int memory_check_process_rootkit(const memory_process_t* process, int* is_rootkit, char** rootkit_name);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MEMORY_PROCESSES_H