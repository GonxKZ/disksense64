#ifndef INTEGRATIONS_YARA_BRIDGE_H
#define INTEGRATIONS_YARA_BRIDGE_H

#include <stddef.h>

// Minimal C-ABI bridge to libs/yara without including its C header that uses C++ keywords
extern "C" {
    int yara_load_rules(const char* rules_path);
    void yara_options_init(void* options);
    int yara_scan_directory(const char* directory_path, const void* options, void* result);
    void yara_result_free(void* result);
}

typedef struct {
    char* rule_name;
    char* rule_namespace;
    char* matched_string;
    size_t offset;
    size_t length;
    int severity;
} yara_match_t;

typedef struct {
    yara_match_t* matches;
    size_t count;
    size_t capacity;
} yara_result_t;

typedef struct {
    int timeout_seconds;
    int follow_symlinks;
    int scan_compressed;
    int scan_archives;
    char** exclude_patterns;
    size_t exclude_count;
} yara_options_t;

#endif

