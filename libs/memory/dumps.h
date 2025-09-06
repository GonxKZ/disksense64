#ifndef LIBS_MEMORY_DUMPS_H
#define LIBS_MEMORY_DUMPS_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse RAW memory dump
int memory_parse_raw_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Parse Crash Dump
int memory_parse_crash_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Parse LiME dump
int memory_parse_lime_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Parse AVML dump
int memory_parse_avml_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Parse VMware VMSS dump
int memory_parse_vmss_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

// Parse VMware VMSN dump
int memory_parse_vmsn_dump(const char* dump_path, const memory_options_t* options, memory_analysis_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MEMORY_DUMPS_H