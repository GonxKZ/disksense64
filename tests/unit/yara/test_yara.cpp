#include "libs/yara/yara.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

// Create test files with suspicious content
static int create_test_files(const char* test_dir) {
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    // Create clean file
    char clean_file[1024];
    snprintf(clean_file, sizeof(clean_file), "%s/clean_file.txt", test_dir);
    int fd = open(clean_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "This is a clean file with no malware.", 37);
        close(fd);
    }
    
    // Create suspicious file 1
    char suspicious_file1[1024];
    snprintf(suspicious_file1, sizeof(suspicious_file1), "%s/suspicious_file1.txt", test_dir);
    fd = open(suspicious_file1, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "This file contains a virus signature pattern.", 45);
        close(fd);
    }
    
    // Create suspicious file 2
    char suspicious_file2[1024];
    snprintf(suspicious_file2, sizeof(suspicious_file2), "%s/suspicious_file2.txt", test_dir);
    fd = open(suspicious_file2, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "This file contains malware code.", 32);
        close(fd);
    }
    
    // Create suspicious file 3
    char suspicious_file3[1024];
    snprintf(suspicious_file3, sizeof(suspicious_file3), "%s/trojan_file.txt", test_dir);
    fd = open(suspicious_file3, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, "This file contains a trojan horse.", 34);
        close(fd);
    }
    
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* test_dir) {
    DIR* dir = opendir(test_dir);
    if (!dir) {
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", test_dir, entry->d_name);
        unlink(filepath);
    }
    
    closedir(dir);
    rmdir(test_dir);
}

// Create sample YARA rules
static int create_sample_rules(const char* rules_path) {
    FILE* file = fopen(rules_path, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "rule VirusSignature {\n");
    fprintf(file, "  strings:\n");
    fprintf(file, "    $virus = \"virus\"\n");
    fprintf(file, "  condition:\n");
    fprintf(file, "    $virus\n");
    fprintf(file, "}\n\n");
    
    fprintf(file, "rule MalwareSignature {\n");
    fprintf(file, "  strings:\n");
    fprintf(file, "    $malware = \"malware\"\n");
    fprintf(file, "  condition:\n");
    fprintf(file, "    $malware\n");
    fprintf(file, "}\n\n");
    
    fprintf(file, "rule TrojanSignature {\n");
    fprintf(file, "  strings:\n");
    fprintf(file, "    $trojan = \"trojan\"\n");
    fprintf(file, "  condition:\n");
    fprintf(file, "    $trojan\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

int test_rule_loading() {
    printf("Testing YARA rule loading...\n");
    
    const char* rules_path = "/tmp/test_yara_rules.yar";
    
    // Create sample rules
    if (create_sample_rules(rules_path) != 0) {
        printf("Failed to create sample YARA rules\n");
        return 1;
    }
    
    // Load rules from file
    int ret = yara_load_rules(rules_path);
    if (ret != 0) {
        printf("Failed to load YARA rules from file\n");
        unlink(rules_path);
        return 1;
    }
    
    printf("Loaded YARA rules from file successfully\n");
    
    // Load rules from memory
    const char* rules_data = "rule TestRule { condition: true }";
    ret = yara_load_rules_from_memory(rules_data, strlen(rules_data));
    if (ret != 0) {
        printf("Failed to load YARA rules from memory\n");
        unlink(rules_path);
        return 1;
    }
    
    printf("Loaded YARA rules from memory successfully\n");
    
    // Clean up
    unlink(rules_path);
    
    printf("YARA rule loading test passed!\n");
    return 0;
}

int test_file_scanning() {
    printf("Testing YARA file scanning...\n");
    
    const char* test_dir = "/tmp/yara_test_dir";
    const char* rules_path = "/tmp/test_yara_rules.yar";
    
    // Create sample rules
    if (create_sample_rules(rules_path) != 0) {
        printf("Failed to create sample YARA rules\n");
        return 1;
    }
    
    // Load rules
    if (yara_load_rules(rules_path) != 0) {
        printf("Failed to load YARA rules\n");
        unlink(rules_path);
        return 1;
    }
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        unlink(rules_path);
        return 1;
    }
    
    // Scan clean file
    char clean_file[1024];
    snprintf(clean_file, sizeof(clean_file), "%s/clean_file.txt", test_dir);
    
    yara_result_t clean_result;
    int ret = yara_scan_file(clean_file, NULL, &clean_result);
    
    if (ret != 0) {
        printf("Failed to scan clean file\n");
        yara_result_free(&clean_result);
        cleanup_test_environment(test_dir);
        unlink(rules_path);
        return 1;
    }
    
    printf("Scanned clean file: %lu matches\n", (unsigned long)clean_result.count);
    yara_result_free(&clean_result);
    
    // Scan suspicious file 1
    char suspicious_file1[1024];
    snprintf(suspicious_file1, sizeof(suspicious_file1), "%s/suspicious_file1.txt", test_dir);
    
    yara_result_t suspicious_result1;
    ret = yara_scan_file(suspicious_file1, NULL, &suspicious_result1);
    
    if (ret != 0) {
        printf("Failed to scan suspicious file 1\n");
        yara_result_free(&suspicious_result1);
        cleanup_test_environment(test_dir);
        unlink(rules_path);
        return 1;
    }
    
    printf("Scanned suspicious file 1: %lu matches\n", (unsigned long)suspicious_result1.count);
    for (size_t i = 0; i < suspicious_result1.count; i++) {
        yara_match_t* match = &suspicious_result1.matches[i];
        printf("  Match %lu: Rule=%s, String=%s, Severity=%d (%s)\n",
               (unsigned long)i, 
               match->rule_name ? match->rule_name : "unknown",
               match->matched_string ? match->matched_string : "unknown",
               match->severity,
               yara_get_severity_description(match->severity));
    }
    yara_result_free(&suspicious_result1);
    
    // Clean up
    cleanup_test_environment(test_dir);
    unlink(rules_path);
    
    printf("YARA file scanning test passed!\n");
    return 0;
}

int test_data_scanning() {
    printf("Testing YARA data scanning...\n");
    
    const char* rules_path = "/tmp/test_yara_rules.yar";
    
    // Create sample rules
    if (create_sample_rules(rules_path) != 0) {
        printf("Failed to create sample YARA rules\n");
        return 1;
    }
    
    // Load rules
    if (yara_load_rules(rules_path) != 0) {
        printf("Failed to load YARA rules\n");
        unlink(rules_path);
        return 1;
    }
    
    // Scan clean data
    const char* clean_data = "This is clean data with no malware signatures.";
    yara_result_t clean_result;
    int ret = yara_scan_data(clean_data, strlen(clean_data), NULL, &clean_result);
    
    if (ret != 0) {
        printf("Failed to scan clean data\n");
        yara_result_free(&clean_result);
        unlink(rules_path);
        return 1;
    }
    
    printf("Scanned clean data: %lu matches\n", (unsigned long)clean_result.count);
    yara_result_free(&clean_result);
    
    // Scan suspicious data
    const char* suspicious_data = "This data contains a virus signature that should be detected.";
    yara_result_t suspicious_result;
    ret = yara_scan_data(suspicious_data, strlen(suspicious_data), NULL, &suspicious_result);
    
    if (ret != 0) {
        printf("Failed to scan suspicious data\n");
        yara_result_free(&suspicious_result);
        unlink(rules_path);
        return 1;
    }
    
    printf("Scanned suspicious data: %lu matches\n", (unsigned long)suspicious_result.count);
    for (size_t i = 0; i < suspicious_result.count; i++) {
        yara_match_t* match = &suspicious_result.matches[i];
        printf("  Match %lu: Rule=%s, String=%s, Severity=%d (%s)\n",
               (unsigned long)i, 
               match->rule_name ? match->rule_name : "unknown",
               match->matched_string ? match->matched_string : "unknown",
               match->severity,
               yara_get_severity_description(match->severity));
    }
    yara_result_free(&suspicious_result);
    
    // Clean up
    unlink(rules_path);
    
    printf("YARA data scanning test passed!\n");
    return 0;
}

int test_directory_scanning() {
    printf("Testing YARA directory scanning...\n");
    
    const char* test_dir = "/tmp/yara_test_dir";
    const char* rules_path = "/tmp/test_yara_rules.yar";
    
    // Create sample rules
    if (create_sample_rules(rules_path) != 0) {
        printf("Failed to create sample YARA rules\n");
        return 1;
    }
    
    // Load rules
    if (yara_load_rules(rules_path) != 0) {
        printf("Failed to load YARA rules\n");
        unlink(rules_path);
        return 1;
    }
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        unlink(rules_path);
        return 1;
    }
    
    // Scan directory
    yara_result_t result;
    int ret = yara_scan_directory(test_dir, NULL, &result);
    
    if (ret != 0) {
        printf("Failed to scan directory\n");
        yara_result_free(&result);
        cleanup_test_environment(test_dir);
        unlink(rules_path);
        return 1;
    }
    
    printf("Scanned directory: %lu total matches\n", (unsigned long)result.count);
    for (size_t i = 0; i < result.count; i++) {
        yara_match_t* match = &result.matches[i];
        printf("  Match %lu: Rule=%s, String=%s, Severity=%d (%s)\n",
               (unsigned long)i, 
               match->rule_name ? match->rule_name : "unknown",
               match->matched_string ? match->matched_string : "unknown",
               match->severity,
               yara_get_severity_description(match->severity));
    }
    
    yara_result_free(&result);
    
    // Clean up
    cleanup_test_environment(test_dir);
    unlink(rules_path);
    
    printf("YARA directory scanning test passed!\n");
    return 0;
}

int test_rule_information() {
    printf("Testing YARA rule information...\n");
    
    const char* rules_path = "/tmp/test_yara_rules.yar";
    
    // Create sample rules
    if (create_sample_rules(rules_path) != 0) {
        printf("Failed to create sample YARA rules\n");
        return 1;
    }
    
    // Load rules
    if (yara_load_rules(rules_path) != 0) {
        printf("Failed to load YARA rules\n");
        unlink(rules_path);
        return 1;
    }
    
    // Get rule information
    yara_rule_info_t* rules;
    size_t count;
    int ret = yara_get_rule_info(&rules, &count);
    
    if (ret != 0) {
        printf("Failed to get rule information\n");
        unlink(rules_path);
        return 1;
    }
    
    printf("Loaded %lu YARA rules:\n", (unsigned long)count);
    for (size_t i = 0; i < count; i++) {
        yara_rule_info_t* rule = &rules[i];
        printf("  Rule %lu:\n", (unsigned long)i);
        printf("    Name: %s\n", rule->name ? rule->name : "unknown");
        printf("    Namespace: %s\n", rule->namespace ? rule->namespace : "unknown");
        printf("    Description: %s\n", rule->description ? rule->description : "unknown");
        printf("    Author: %s\n", rule->author ? rule->author : "unknown");
        printf("    Version: %s\n", rule->version ? rule->version : "unknown");
        printf("    Severity: %d\n", rule->severity);
    }
    
    // Clean up
    for (size_t i = 0; i < count; i++) {
        yara_rule_info_free(&rules[i]);
    }
    free(rules);
    unlink(rules_path);
    
    printf("YARA rule information test passed!\n");
    return 0;
}

int test_exclude_patterns() {
    printf("Testing YARA exclude patterns...\n");
    
    yara_options_t options;
    yara_options_init(&options);
    
    // Add exclude patterns
    yara_add_exclude_pattern(&options, ".tmp");
    yara_add_exclude_pattern(&options, "cache");
    yara_add_exclude_pattern(&options, "log");
    
    printf("Added %lu exclude patterns:\n", (unsigned long)options.exclude_count);
    for (size_t i = 0; i < options.exclude_count; i++) {
        printf("  Pattern %lu: %s\n", (unsigned long)i, options.exclude_patterns[i]);
    }
    
    // Test exclude pattern matching
    const char* test_files[] = {
        "/tmp/test.txt",
        "/tmp/test.tmp",
        "/var/cache/data.bin",
        "/var/log/system.log",
        "/home/user/document.pdf",
        NULL
    };
    
    for (int i = 0; test_files[i] != NULL; i++) {
        int excluded = 0;
        const char* filename = strrchr(test_files[i], '/');
        if (filename) {
            filename++; // Skip the '/'
        } else {
            filename = test_files[i];
        }
        
        for (size_t j = 0; j < options.exclude_count; j++) {
            if (strstr(filename, options.exclude_patterns[j]) != NULL) {
                excluded = 1;
                break;
            }
        }
        
        printf("File %s: %s\n", test_files[i], excluded ? "excluded" : "included");
    }
    
    // Clean up
    yara_options_free(&options);
    
    printf("YARA exclude patterns test passed!\n");
    return 0;
}

int test_severity_descriptions() {
    printf("Testing YARA severity descriptions...\n");
    
    for (int i = 1; i <= 10; i++) {
        const char* description = yara_get_severity_description(i);
        printf("Severity %d: %s\n", i, description);
    }
    
    // Test out of range values
    const char* desc_low = yara_get_severity_description(0);
    const char* desc_high = yara_get_severity_description(15);
    printf("Severity 0: %s\n", desc_low);
    printf("Severity 15: %s\n", desc_high);
    
    printf("YARA severity descriptions test passed!\n");
    return 0;
}

int main() {
    printf("Running YARA malware detection tests...\n");
    
    int result1 = test_rule_loading();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_file_scanning();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_data_scanning();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_directory_scanning();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_rule_information();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_exclude_patterns();
    if (result6 != 0) {
        return result6;
    }
    
    int result7 = test_severity_descriptions();
    if (result7 != 0) {
        return result7;
    }
    
    printf("All YARA malware detection tests passed!\n");
    return 0;
}