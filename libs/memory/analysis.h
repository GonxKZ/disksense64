#ifndef LIBS_MEMORY_ANALYSIS_H
#define LIBS_MEMORY_ANALYSIS_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Perform comprehensive memory analysis
int memory_perform_comprehensive_analysis(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Detect injected code in processes
int memory_detect_injected_code_internal(const char* dump_path, const memory_process_t* processes, size_t count, size_t* injected_count);

// Detect rootkit activity in memory
int memory_detect_rootkit_activity_internal(const char* dump_path, const memory_analysis_result_t* result, int* is_rootkit_detected);

// Extract and analyze strings from memory
int memory_extract_and_analyze_strings(const char* dump_path, size_t min_length, memory_string_t** strings, size_t* count);

// Analyze memory regions for suspicious characteristics
int memory_analyze_regions_internal(const char* dump_path, const memory_options_t* options, memory_region_t** regions, size_t* count);

// Generate memory analysis report
int memory_generate_report(const memory_analysis_result_t* result, const char* report_path);

// Export memory analysis results to CSV
int memory_export_to_csv(const memory_analysis_result_t* result, const char* csv_path);

// Export memory analysis results to JSON
int memory_export_to_json(const memory_analysis_result_t* result, const char* json_path);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MEMORY_ANALYSIS_H