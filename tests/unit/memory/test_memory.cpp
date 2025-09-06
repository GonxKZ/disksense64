#include "libs/memory/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// Create a test memory dump file with realistic content
static int create_test_memory_dump(const char* dump_path) {
    printf("Creating test memory dump: %s\n", dump_path);
    
    // Create a file with some realistic content
    int fd = open(dump_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create test memory dump");
        return -1;
    }
    
    // Write some recognizable strings that our analysis will detect
    const char* test_strings[] = {
        "This is a normal system process",
        "Windows Explorer",
        "Google Chrome",
        "malware_payload_detected_here",
        "backdoor_communication_channel",
        "C:\\Program Files\\",
        "http://suspicious-domain.com",
        "keylogger_active_process",
        "System process with normal behavior",
        "User application running normally"
    };
    
    size_t num_strings = sizeof(test_strings) / sizeof(test_strings[0]);
    
    // Write strings with padding to simulate memory layout
    for (size_t i = 0; i < num_strings; i++) {
        // Write the string
        write(fd, test_strings[i], strlen(test_strings[i]));
        
        // Add padding to simulate memory gaps
        char padding[256];
        memset(padding, 0x00, sizeof(padding));
        write(fd, padding, 100);
    }
    
    // Add some binary data to make it more realistic
    unsigned char binary_data[1024];
    for (int i = 0; i < 1024; i++) {
        binary_data[i] = (unsigned char)(rand() % 256);
    }
    write(fd, binary_data, sizeof(binary_data));
    
    close(fd);
    printf("Created test memory dump with %lu bytes\n", (unsigned long)(num_strings * 200 + 1024));
    return 0;
}

// Clean up test files
static void cleanup_test_files(const char* dump_path, const char* report_path, 
                               const char* csv_path, const char* json_path) {
    unlink(dump_path);
    unlink(report_path);
    unlink(csv_path);
    unlink(json_path);
}

int test_memory_format_detection() {
    printf("\n=== Testing Memory Format Detection ===\n");
    
    const char* test_files[] = {
        "/tmp/test_memory.raw",
        "/tmp/test_memory.dmp",
        "/tmp/test_memory.lime",
        "/tmp/test_memory.avml",
        "/tmp/test_memory.vmss",
        "/tmp/test_memory.vmsn",
        "/tmp/test_memory.unknown"
    };
    
    int num_files = sizeof(test_files) / sizeof(test_files[0]);
    
    for (int i = 0; i < num_files; i++) {
        // Create empty test files
        int fd = open(test_files[i], O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            close(fd);
        }
        
        memory_dump_format_t format = memory_detect_format(test_files[i]);
        const char* format_name = memory_get_format_name(format);
        const char* format_desc = memory_get_format_description(format);
        
        printf("File: %s -> Format: %s (%s)\n", 
               test_files[i], format_name, format_desc);
    }
    
    // Clean up test files
    for (int i = 0; i < num_files; i++) {
        unlink(test_files[i]);
    }
    
    printf("Memory format detection test passed!\n");
    return 0;
}

int test_memory_analysis() {
    printf("\n=== Testing Memory Analysis ===\n");
    
    const char* dump_path = "/tmp/test_memory_analysis.raw";
    const char* report_path = "/tmp/memory_analysis_report.txt";
    const char* csv_path = "/tmp/memory_analysis.csv";
    const char* json_path = "/tmp/memory_analysis.json";
    
    // Create test memory dump
    if (create_test_memory_dump(dump_path) != 0) {
        printf("Failed to create test memory dump\n");
        return 1;
    }
    
    // Initialize analysis options
    memory_options_t options;
    memory_options_init(&options);
    options.analyze_processes = 1;
    options.analyze_network = 1;
    options.analyze_regions = 1;
    options.detect_malware = 1;
    options.detect_rootkits = 1;
    options.detect_code_injection = 1;
    options.extract_strings = 1;
    options.max_processes = 50;
    options.max_connections = 100;
    options.max_regions = 200;
    
    // Perform comprehensive memory analysis
    memory_analysis_result_t result;
    int ret = memory_analyze_dump(dump_path, &options, &result);
    
    if (ret != 0) {
        printf("Failed to analyze memory dump\n");
        cleanup_test_files(dump_path, report_path, csv_path, json_path);
        memory_options_free(&options);
        return 1;
    }
    
    printf("Memory analysis completed successfully\n");
    printf("Results:\n");
    printf("  Total processes: %lu\n", (unsigned long)result.process_count);
    printf("  Total connections: %lu\n", (unsigned long)result.connection_count);
    printf("  Total regions: %lu\n", (unsigned long)result.region_count);
    printf("  Suspicious processes: %s\n", result.has_suspicious_processes ? "Yes" : "No");
    printf("  Suspicious connections: %s\n", result.has_suspicious_connections ? "Yes" : "No");
    printf("  Injected code: %s\n", result.has_injected_code ? "Yes" : "No");
    printf("  Rootkit activity: %s\n", result.has_rootkit_activity ? "Yes" : "No");
    
    // Generate reports
    printf("\nGenerating reports...\n");
    
    ret = memory_generate_report(&result, report_path);
    if (ret == 0) {
        printf("Text report generated: %s\n", report_path);
    } else {
        printf("Failed to generate text report\n");
    }
    
    ret = memory_export_to_csv(&result, csv_path);
    if (ret == 0) {
        printf("CSV export generated: %s\n", csv_path);
    } else {
        printf("Failed to generate CSV export\n");
    }
    
    ret = memory_export_to_json(&result, json_path);
    if (ret == 0) {
        printf("JSON export generated: %s\n", json_path);
    } else {
        printf("Failed to generate JSON export\n");
    }
    
    // Clean up
    memory_analysis_result_free(&result);
    memory_options_free(&options);
    cleanup_test_files(dump_path, report_path, csv_path, json_path);
    
    printf("Memory analysis test passed!\n");
    return 0;
}

int test_process_extraction() {
    printf("\n=== Testing Process Extraction ===\n");
    
    const char* dump_path = "/tmp/test_process_extraction.raw";
    
    // Create test memory dump
    if (create_test_memory_dump(dump_path) != 0) {
        printf("Failed to create test memory dump\n");
        return 1;
    }
    
    // Initialize options
    memory_options_t options;
    memory_options_init(&options);
    options.max_processes = 25;
    
    // Extract processes
    memory_process_t* processes;
    size_t process_count;
    
    int ret = memory_extract_processes(dump_path, &options, &processes, &process_count);
    
    if (ret != 0) {
        printf("Failed to extract processes\n");
        unlink(dump_path);
        memory_options_free(&options);
        return 1;
    }
    
    printf("Extracted %lu processes:\n", (unsigned long)process_count);
    
    // Display first 10 processes
    for (size_t i = 0; i < process_count && i < 10; i++) {
        const memory_process_t* process = &processes[i];
        printf("  Process %lu:\n", (unsigned long)i);
        printf("    PID: %u\n", process->process_id);
        printf("    Parent PID: %u\n", process->parent_process_id);
        printf("    Name: %s\n", process->process_name ? process->process_name : "Unknown");
        printf("    Base Address: 0x%08lX\n", (unsigned long)process->base_address);
        printf("    Image Size: %lu bytes\n", (unsigned long)process->image_size);
        printf("    Threads: %u\n", process->thread_count);
        printf("    Virtual Size: %lu bytes\n", (unsigned long)process->virtual_size);
        printf("    Working Set: %lu bytes\n", (unsigned long)process->working_set_size);
        printf("    Suspicious: %s\n", process->is_suspicious ? "Yes" : "No");
        if (process->is_suspicious) {
            printf("    Threat: %s (Confidence: %.2f)\n", 
                   process->threat_type ? process->threat_type : "Unknown",
                   process->confidence);
        }
    }
    
    if (process_count > 10) {
        printf("  ... and %lu more processes\n", (unsigned long)(process_count - 10));
    }
    
    // Clean up
    for (size_t i = 0; i < process_count; i++) {
        memory_process_free(&processes[i]);
    }
    free(processes);
    memory_options_free(&options);
    unlink(dump_path);
    
    printf("Process extraction test passed!\n");
    return 0;
}

int test_connection_extraction() {
    printf("\n=== Testing Connection Extraction ===\n");
    
    const char* dump_path = "/tmp/test_connection_extraction.raw";
    
    // Create test memory dump
    if (create_test_memory_dump(dump_path) != 0) {
        printf("Failed to create test memory dump\n");
        return 1;
    }
    
    // Initialize options
    memory_options_t options;
    memory_options_init(&options);
    options.max_connections = 50;
    
    // Extract connections
    memory_connection_t* connections;
    size_t connection_count;
    
    int ret = memory_extract_connections(dump_path, &options, &connections, &connection_count);
    
    if (ret != 0) {
        printf("Failed to extract connections\n");
        unlink(dump_path);
        memory_options_free(&options);
        return 1;
    }
    
    printf("Extracted %lu connections:\n", (unsigned long)connection_count);
    
    // Display first 15 connections
    for (size_t i = 0; i < connection_count && i < 15; i++) {
        const memory_connection_t* connection = &connections[i];
        char local_ip[16], remote_ip[16];
        memory_ip_to_string(connection->local_ip, local_ip, sizeof(local_ip));
        memory_ip_to_string(connection->remote_ip, remote_ip, sizeof(remote_ip));
        
        printf("  Connection %lu:\n", (unsigned long)i);
        printf("    PID: %u\n", connection->process_id);
        printf("    Local: %s:%u\n", local_ip, connection->local_port);
        printf("    Remote: %s:%u\n", remote_ip, connection->remote_port);
        printf("    Protocol: %s\n", connection->protocol == 6 ? "TCP" : 
                                     connection->protocol == 17 ? "UDP" : "Other");
        printf("    State: %d\n", connection->state);
        printf("    Suspicious: %s\n", connection->is_suspicious ? "Yes" : "No");
        if (connection->is_suspicious) {
            printf("    Threat: %s (Confidence: %.2f)\n", 
                   connection->threat_type ? connection->threat_type : "Unknown",
                   connection->confidence);
        }
    }
    
    if (connection_count > 15) {
        printf("  ... and %lu more connections\n", (unsigned long)(connection_count - 15));
    }
    
    // Clean up
    for (size_t i = 0; i < connection_count; i++) {
        memory_connection_free(&connections[i]);
    }
    free(connectiones);
    memory_options_free(&options);
    unlink(dump_path);
    
    printf("Connection extraction test passed!\n");
    return 0;
}

int test_string_extraction() {
    printf("\n=== Testing String Extraction ===\n");
    
    const char* dump_path = "/tmp/test_string_extraction.raw";
    
    // Create test memory dump
    if (create_test_memory_dump(dump_path) != 0) {
        printf("Failed to create test memory dump\n");
        return 1;
    }
    
    // Extract strings
    memory_string_t* strings;
    size_t string_count;
    
    int ret = memory_extract_strings(dump_path, 10, &strings, &string_count);
    
    if (ret != 0) {
        printf("Failed to extract strings\n");
        unlink(dump_path);
        return 1;
    }
    
    printf("Extracted %lu strings:\n", (unsigned long)string_count);
    
    // Display first 20 strings
    for (size_t i = 0; i < string_count && i < 20; i++) {
        const memory_string_t* string = &strings[i];
        printf("  String %lu:\n", (unsigned long)i);
        printf("    Value: %s\n", string->string_value ? string->string_value : "Unknown");
        printf("    Address: 0x%08lX\n", (unsigned long)string->address);
        printf("    Length: %lu\n", (unsigned long)string->length);
        printf("    Suspicious: %s\n", string->is_suspicious ? "Yes" : "No");
        if (string->is_suspicious) {
            printf("    Context: %s\n", string->context ? string->context : "Unknown");
        }
    }
    
    if (string_count > 20) {
        printf("  ... and %lu more strings\n", (unsigned long)(string_count - 20));
    }
    
    // Count suspicious strings
    size_t suspicious_count = 0;
    for (size_t i = 0; i < string_count; i++) {
        if (strings[i].is_suspicious) {
            suspicious_count++;
        }
    }
    printf("Total suspicious strings: %lu\n", (unsigned long)suspicious_count);
    
    // Clean up
    for (size_t i = 0; i < string_count; i++) {
        memory_string_free(&strings[i]);
    }
    free(strings);
    unlink(dump_path);
    
    printf("String extraction test passed!\n");
    return 0;
}

int test_suspicious_detection() {
    printf("\n=== Testing Suspicious Activity Detection ===\n");
    
    const char* dump_path = "/tmp/test_suspicious_detection.raw";
    
    // Create test memory dump
    if (create_test_memory_dump(dump_path) != 0) {
        printf("Failed to create test memory dump\n");
        return 1;
    }
    
    // Initialize options
    memory_options_t options;
    memory_options_init(&options);
    
    // Extract processes for testing
    memory_process_t* processes;
    size_t process_count;
    
    if (memory_extract_processes(dump_path, &options, &processes, &process_count) != 0) {
        printf("Failed to extract processes for testing\n");
        unlink(dump_path);
        memory_options_free(&options);
        return 1;
    }
    
    // Extract connections for testing
    memory_connection_t* connections;
    size_t connection_count;
    
    if (memory_extract_connections(dump_path, &options, &connections, &connection_count) != 0) {
        printf("Failed to extract connections for testing\n");
        // Clean up processes
        for (size_t i = 0; i < process_count; i++) {
            memory_process_free(&processes[i]);
        }
        free(processes);
        unlink(dump_path);
        memory_options_free(&options);
        return 1;
    }
    
    // Test suspicious process detection
    size_t suspicious_processes;
    int ret = memory_detect_suspicious_processes(processes, process_count, &suspicious_processes);
    
    if (ret == 0) {
        printf("Detected %lu suspicious processes\n", (unsigned long)suspicious_processes);
    } else {
        printf("Failed to detect suspicious processes\n");
    }
    
    // Test suspicious connection detection
    size_t suspicious_connections;
    ret = memory_detect_suspicious_connections(connections, connection_count, &suspicious_connections);
    
    if (ret == 0) {
        printf("Detected %lu suspicious connections\n", (unsigned long)suspicious_connections);
    } else {
        printf("Failed to detect suspicious connections\n");
    }
    
    // Test code injection detection
    size_t injected_processes;
    ret = memory_detect_code_injection(dump_path, processes, process_count, &injected_processes);
    
    if (ret == 0) {
        printf("Detected code injection in %lu processes\n", (unsigned long)injected_processes);
    } else {
        printf("Failed to detect code injection\n");
    }
    
    // Test rootkit detection
    memory_analysis_result_t dummy_result;
    memory_analysis_result_init(&dummy_result);
    dummy_result.processes = processes;
    dummy_result.process_count = process_count;
    dummy_result.connections = connections;
    dummy_result.connection_count = connection_count;
    
    int rootkit_detected;
    ret = memory_detect_rootkit_activity(dump_path, &dummy_result, &rootkit_detected);
    
    if (ret == 0) {
        printf("Rootkit activity detected: %s\n", rootkit_detected ? "Yes" : "No");
    } else {
        printf("Failed to detect rootkit activity\n");
    }
    
    // Clean up
    memory_analysis_result_free(&dummy_result); // This will clean up the arrays
    memory_options_free(&options);
    unlink(dump_path);
    
    printf("Suspicious activity detection test passed!\n");
    return 0;
}

int test_ip_conversion() {
    printf("\n=== Testing IP Conversion ===\n");
    
    // Test IP address conversion
    uint32_t test_ips[] = {
        0xC0A80101, // 192.168.1.1
        0x0A000001, // 10.0.0.1
        0x7F000001, // 127.0.0.1
        0x08080808  // 8.8.8.8
    };
    
    int num_ips = sizeof(test_ips) / sizeof(test_ips[0]);
    
    for (int i = 0; i < num_ips; i++) {
        char ip_string[16];
        int ret = memory_ip_to_string(test_ips[i], ip_string, sizeof(ip_string));
        
        if (ret == 0) {
            printf("IP 0x%08X -> %s\n", test_ips[i], ip_string);
        } else {
            printf("Failed to convert IP 0x%08X\n", test_ips[i]);
        }
    }
    
    printf("IP conversion test passed!\n");
    return 0;
}

int main() {
    srand((unsigned int)time(NULL)); // Initialize random seed
    
    printf("Running comprehensive memory analysis tests...\n");
    
    int result1 = test_memory_format_detection();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_memory_analysis();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_process_extraction();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_connection_extraction();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_string_extraction();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_suspicious_detection();
    if (result6 != 0) {
        return result6;
    }
    
    int result7 = test_ip_conversion();
    if (result7 != 0) {
        return result7;
    }
    
    printf("\n=== All Memory Analysis Tests Passed! ===\n");
    return 0;
}