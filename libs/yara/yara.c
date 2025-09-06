#include "yara.h"
#include "rules.h"
#include "scanner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

// Global flag to track if rules are loaded
static int g_rules_loaded = 0;

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

void yara_result_init(yara_result_t* result) {
    if (result) {
        result->matches = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int yara_result_add_match(yara_result_t* result, const yara_match_t* match) {
    if (!result || !match) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = (result->capacity == 0) ? 16 : result->capacity * 2;
        yara_match_t* new_matches = (yara_match_t*)realloc(result->matches, new_capacity * sizeof(yara_match_t));
        if (!new_matches) {
            return -1;
        }
        result->matches = new_matches;
        result->capacity = new_capacity;
    }
    
    // Copy match data
    yara_match_t* new_match = &result->matches[result->count];
    new_match->rule_name = strdup_safe(match->rule_name);
    new_match->rule_namespace = strdup_safe(match->rule_namespace);
    new_match->matched_string = strdup_safe(match->matched_string);
    new_match->offset = match->offset;
    new_match->length = match->length;
    new_match->severity = match->severity;
    
    if ((!new_match->rule_name || !new_match->rule_namespace || !new_match->matched_string) && 
        (match->rule_name || match->rule_namespace || match->matched_string)) {
        // Clean up on failure
        free(new_match->rule_name);
        free(new_match->rule_namespace);
        free(new_match->matched_string);
        return -1;
    }
    
    result->count++;
    return 0;
}

void yara_result_free(yara_result_t* result) {
    if (result) {
        if (result->matches) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->matches[i].rule_name);
                free(result->matches[i].rule_namespace);
                free(result->matches[i].matched_string);
            }
            free(result->matches);
        }
        result->matches = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

void yara_options_init(yara_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(yara_options_t));
        options->timeout_seconds = 60;
        options->follow_symlinks = 1;
        options->scan_compressed = 1;
        options->scan_archives = 1;
    }
}

void yara_options_free(yara_options_t* options) {
    if (options) {
        if (options->exclude_patterns) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_patterns[i]);
            }
            free(options->exclude_patterns);
        }
        memset(options, 0, sizeof(yara_options_t));
    }
}

void yara_rule_info_init(yara_rule_info_t* info) {
    if (info) {
        memset(info, 0, sizeof(yara_rule_info_t));
    }
}

void yara_rule_info_free(yara_rule_info_t* info) {
    if (info) {
        free(info->name);
        free(info->namespace);
        free(info->description);
        free(info->author);
        free(info->version);
        memset(info, 0, sizeof(yara_rule_info_t));
    }
}

int yara_load_rules(const char* rules_path) {
    if (!rules_path) {
        return -1;
    }
    
    // Check if file exists
    if (access(rules_path, R_OK) != 0) {
        return -1;
    }
    
    // In a real implementation, this would load YARA rules using the YARA library
    // For this example, we'll just set a flag
    g_rules_loaded = 1;
    
    printf("Loaded YARA rules from %s\n", rules_path);
    return 0;
}

int yara_load_rules_from_memory(const void* data, size_t size) {
    if (!data || size == 0) {
        return -1;
    }
    
    // In a real implementation, this would load YARA rules from memory using the YARA library
    // For this example, we'll just set a flag
    g_rules_loaded = 1;
    
    printf("Loaded YARA rules from memory (%lu bytes)\n", (unsigned long)size);
    return 0;
}

// Simulate YARA scanning
static int simulate_yara_scan(const char* file_path, yara_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }
    
    yara_result_init(result);
    
    // Simulate finding some matches based on file content
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Read first 1024 bytes to check for suspicious patterns
    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);
    
    if (bytes_read > 0) {
        // Check for suspicious patterns
        if (memmem(buffer, bytes_read, "virus", 5) != NULL) {
            yara_match_t match;
            match.rule_name = strdup("VirusSignature");
            match.rule_namespace = strdup("malware");
            match.matched_string = strdup("virus");
            match.offset = 0;
            match.length = 5;
            match.severity = 9;
            yara_result_add_match(result, &match);
            free(match.rule_name);
            free(match.rule_namespace);
            free(match.matched_string);
        }
        
        if (memmem(buffer, bytes_read, "malware", 7) != NULL) {
            yara_match_t match;
            match.rule_name = strdup("MalwareSignature");
            match.rule_namespace = strdup("malware");
            match.matched_string = strdup("malware");
            match.offset = 0;
            match.length = 7;
            match.severity = 8;
            yara_result_add_match(result, &match);
            free(match.rule_name);
            free(match.rule_namespace);
            free(match.matched_string);
        }
        
        if (memmem(buffer, bytes_read, "trojan", 6) != NULL) {
            yara_match_t match;
            match.rule_name = strdup("TrojanSignature");
            match.rule_namespace = strdup("malware");
            match.matched_string = strdup("trojan");
            match.offset = 0;
            match.length = 6;
            match.severity = 7;
            yara_result_add_match(result, &match);
            free(match.rule_name);
            free(match.rule_namespace);
            free(match.matched_string);
        }
    }
    
    return 0;
}

int yara_scan_file(const char* file_path, const yara_options_t* options, yara_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }
    
    // Check if rules are loaded
    if (!g_rules_loaded) {
        return -1;
    }
    
    // Check if file exists
    if (access(file_path, R_OK) != 0) {
        return -1;
    }
    
    // Check exclude patterns
    if (options && options->exclude_patterns && options->exclude_count > 0) {
        const char* filename = strrchr(file_path, '/');
        if (filename) {
            filename++; // Skip the '/'
        } else {
            filename = file_path;
        }
        
        for (size_t i = 0; i < options->exclude_count; i++) {
            if (strstr(filename, options->exclude_patterns[i]) != NULL) {
                // File matches exclude pattern, skip scanning
                yara_result_init(result);
                return 0;
            }
        }
    }
    
    // In a real implementation, this would scan the file using YARA
    // For this example, we'll simulate the scanning
    return simulate_yara_scan(file_path, result);
}

int yara_scan_data(const void* data, size_t size, const yara_options_t* options, yara_result_t* result) {
    if (!data || !result) {
        return -1;
    }
    
    // Check if rules are loaded
    if (!g_rules_loaded) {
        return -1;
    }
    
    // Create a temporary file to simulate scanning data
    char temp_path[] = "/tmp/yara_scan_XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd < 0) {
        return -1;
    }
    
    // Write data to temporary file
    ssize_t written = write(fd, data, size);
    close(fd);
    
    if (written != (ssize_t)size) {
        unlink(temp_path);
        return -1;
    }
    
    // Scan the temporary file
    int ret = yara_scan_file(temp_path, options, result);
    
    // Clean up temporary file
    unlink(temp_path);
    
    return ret;
}

int yara_scan_directory(const char* directory_path, const yara_options_t* options, yara_result_t* result) {
    if (!directory_path || !result) {
        return -1;
    }
    
    yara_result_init(result);
    
    // Check if rules are loaded
    if (!g_rules_loaded) {
        return -1;
    }
    
    // Open directory
    DIR* dir = opendir(directory_path);
    if (!dir) {
        return -1;
    }
    
    // Process directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Build full path
        size_t path_len = strlen(directory_path);
        size_t name_len = strlen(entry->d_name);
        char* full_path = (char*)malloc(path_len + name_len + 2);
        if (!full_path) {
            continue;
        }
        
        sprintf(full_path, "%s/%s", directory_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // Scan the file
            yara_result_t file_result;
            if (yara_scan_file(full_path, options, &file_result) == 0) {
                // Add matches to overall result
                for (size_t i = 0; i < file_result.count; i++) {
                    yara_result_add_match(result, &file_result.matches[i]);
                }
                yara_result_free(&file_result);
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    return 0;
}

int yara_get_rule_info(yara_rule_info_t** rules, size_t* count) {
    if (!rules || !count) {
        return -1;
    }
    
    // Check if rules are loaded
    if (!g_rules_loaded) {
        return -1;
    }
    
    // For this example, we'll return some sample rule information
    *count = 3;
    *rules = (yara_rule_info_t*)malloc(*count * sizeof(yara_rule_info_t));
    if (!*rules) {
        return -1;
    }
    
    // Rule 1
    yara_rule_info_init(&(*rules)[0]);
    (*rules)[0].name = strdup("VirusSignature");
    (*rules)[0].namespace = strdup("malware");
    (*rules)[0].description = strdup("Detects common virus signatures");
    (*rules)[0].author = strdup("Security Team");
    (*rules)[0].version = strdup("1.0");
    (*rules)[0].severity = 9;
    
    // Rule 2
    yara_rule_info_init(&(*rules)[1]);
    (*rules)[1].name = strdup("MalwareSignature");
    (*rules)[1].namespace = strdup("malware");
    (*rules)[1].description = strdup("Detects common malware signatures");
    (*rules)[1].author = strdup("Security Team");
    (*rules)[1].version = strdup("1.0");
    (*rules)[1].severity = 8;
    
    // Rule 3
    yara_rule_info_init(&(*rules)[2]);
    (*rules)[2].name = strdup("TrojanSignature");
    (*rules)[2].namespace = strdup("malware");
    (*rules)[2].description = strdup("Detects common trojan signatures");
    (*rules)[2].author = strdup("Security Team");
    (*rules)[2].version = strdup("1.0");
    (*rules)[2].severity = 7;
    
    return 0;
}

int yara_add_exclude_pattern(yara_options_t* options, const char* pattern) {
    if (!options || !pattern) {
        return -1;
    }
    
    // Reallocate exclude patterns array
    char** new_patterns = (char**)realloc(options->exclude_patterns, (options->exclude_count + 1) * sizeof(char*));
    if (!new_patterns) {
        return -1;
    }
    
    options->exclude_patterns = new_patterns;
    options->exclude_patterns[options->exclude_count] = strdup_safe(pattern);
    
    if (!options->exclude_patterns[options->exclude_count]) {
        return -1;
    }
    
    options->exclude_count++;
    return 0;
}

void yara_clear_exclude_patterns(yara_options_t* options) {
    if (options) {
        if (options->exclude_patterns) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_patterns[i]);
            }
            free(options->exclude_patterns);
            options->exclude_patterns = NULL;
            options->exclude_count = 0;
        }
    }
}

const char* yara_get_severity_description(int severity) {
    if (severity < 1) severity = 1;
    if (severity > 10) severity = 10;
    
    switch (severity) {
        case 1:
        case 2:
            return "Low risk";
        case 3:
        case 4:
            return "Moderate risk";
        case 5:
        case 6:
            return "High risk";
        case 7:
        case 8:
            return "Very high risk";
        case 9:
        case 10:
            return "Critical risk";
        default:
            return "Unknown risk";
    }
}