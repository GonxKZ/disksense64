#include "libs/security/security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// Create test files with various permission settings
static int create_test_files(const char* test_dir) {
    printf("Creating test files in: %s\n", test_dir);
    
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create test directory");
        return -1;
    }
    
    // Create normal file
    char normal_file[1024];
    snprintf(normal_file, sizeof(normal_file), "%s/normal_file.txt", test_dir);
    int fd = open(normal_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        const char* content = "This is a normal test file with standard permissions.";
        write(fd, content, strlen(content));
        close(fd);
    }
    
    // Create world-writable file (insecure)
    char world_writable_file[1024];
    snprintf(world_writable_file, sizeof(world_writable_file), "%s/world_writable.txt", test_dir);
    fd = open(world_writable_file, O_CREAT | O_WRONLY, 0666);
    if (fd >= 0) {
        const char* content = "This file has world-writable permissions (security risk).";
        write(fd, content, strlen(content));
        close(fd);
        chmod(world_writable_file, 0666); // Ensure world-writable
    }
    
    // Create sensitive file with world-readable permissions (insecure)
    char sensitive_file[1024];
    snprintf(sensitive_file, sizeof(sensitive_file), "%s/database.key", test_dir);
    fd = open(sensitive_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        const char* content = "This is a fake database key file.";
        write(fd, content, strlen(content));
        close(fd);
        chmod(sensitive_file, 0644); // World-readable
    }
    
    // Create directory with normal permissions
    char normal_dir[1024];
    snprintf(normal_dir, sizeof(normal_dir), "%s/normal_directory", test_dir);
    if (mkdir(normal_dir, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create normal directory");
    }
    
    // Create directory with world-writable permissions (insecure)
    char world_writable_dir[1024];
    snprintf(world_writable_dir, sizeof(world_writable_dir), "%s/world_writable_directory", test_dir);
    if (mkdir(world_writable_dir, 0777) != 0 && errno != EEXIST) {
        perror("Failed to create world-writable directory");
    }
    
    return 0;
}

// Clean up test environment
static void cleanup_test_environment(const char* test_dir) {
    printf("Cleaning up test environment: %s\n", test_dir);
    
    // Remove files
    char normal_file[1024];
    snprintf(normal_file, sizeof(normal_file), "%s/normal_file.txt", test_dir);
    unlink(normal_file);
    
    char world_writable_file[1024];
    snprintf(world_writable_file, sizeof(world_writable_file), "%s/world_writable.txt", test_dir);
    unlink(world_writable_file);
    
    char sensitive_file[1024];
    snprintf(sensitive_file, sizeof(sensitive_file), "%s/database.key", test_dir);
    unlink(sensitive_file);
    
    // Remove directories
    char normal_dir[1024];
    snprintf(normal_dir, sizeof(normal_dir), "%s/normal_directory", test_dir);
    rmdir(normal_dir);
    
    char world_writable_dir[1024];
    snprintf(world_writable_dir, sizeof(world_writable_dir), "%s/world_writable_directory", test_dir);
    rmdir(world_writable_dir);
    
    // Remove test directory
    rmdir(test_dir);
}

int test_security_options() {
    printf("\n=== Testing Security Options ===\n");
    
    security_options_t options;
    security_options_init(&options);
    
    printf("Default security options:\n");
    printf("  Audit level: %s\n", security_get_audit_level_description(options.audit_level));
    printf("  Check file permissions: %s\n", options.check_file_permissions ? "Yes" : "No");
    printf("  Check directory permissions: %s\n", options.check_directory_permissions ? "Yes" : "No");
    printf("  Scan for vulnerabilities: %s\n", options.scan_for_vulnerabilities ? "Yes" : "No");
    printf("  Check security policies: %s\n", options.check_security_policies ? "Yes" : "No");
    printf("  Generate reports: %s\n", options.generate_reports ? "Yes" : "No");
    
    // Modify some options
    options.audit_level = SECURITY_AUDIT_DETAILED;
    options.check_weak_passwords = 1;
    options.check_account_lockout = 1;
    
    printf("\nModified security options:\n");
    printf("  Audit level: %s\n", security_get_audit_level_description(options.audit_level));
    printf("  Check weak passwords: %s\n", options.check_weak_passwords ? "Yes" : "No");
    printf("  Check account lockout: %s\n", options.check_account_lockout ? "Yes" : "No");
    
    security_options_free(&options);
    printf("Security options test passed!\n");
    return 0;
}

int test_permission_analysis() {
    printf("\n=== Testing Permission Analysis ===\n");
    
    const char* test_dir = "/tmp/security_test_dir";
    
    // Create test files
    if (create_test_files(test_dir) != 0) {
        printf("Failed to create test files\n");
        return 1;
    }
    
    // Initialize options
    security_options_t options;
    security_options_init(&options);
    options.audit_level = SECURITY_AUDIT_DETAILED;
    options.check_file_permissions = 1;
    options.check_directory_permissions = 1;
    
    // Analyze file permissions
    permission_entry_t* permissions;
    size_t permission_count;
    
    int ret = security_check_permissions(test_dir, &options, &permissions, &permission_count);
    
    if (ret != 0) {
        printf("Failed to check permissions\n");
        cleanup_test_environment(test_dir);
        security_options_free(&options);
        return 1;
    }
    
    printf("Analyzed permissions for %lu items:\n", (unsigned long)permission_count);
    
    // Display results
    for (size_t i = 0; i < permission_count && i < 10; i++) {
        const permission_entry_t* perm = &permissions[i];
        printf("  Item %lu: %s\n", (unsigned long)i, perm->path ? strrchr(perm->path, '/') + 1 : "Unknown");
        printf("    Type: %s\n", security_get_permission_type_name(perm->type));
        printf("    Owner: %s (UID: %u)\n", perm->owner_name ? perm->owner_name : "Unknown", perm->owner_id);
        printf("    Group: %s (GID: %u)\n", perm->group_name ? perm->group_name : "Unknown", perm->group_id);
        printf("    World-writable: %s\n", perm->is_world_writable ? "Yes" : "No");
        printf("    World-readable: %s\n", perm->is_world_readable ? "Yes" : "No");
        printf("    SUID: %s\n", perm->is_suid ? "Yes" : "No");
        printf("    SGID: %s\n", perm->is_sgid ? "Yes" : "No");
        printf("    Has insecure permissions: %s\n", perm->has_insecure_permissions ? "Yes" : "No");
        
        if (perm->has_insecure_permissions) {
            printf("    Security issue: %s\n", perm->security_issue ? perm->security_issue : "Unknown");
            printf("    Severity: %s\n", security_get_severity_description(perm->severity));
            printf("    Confidence: %.2f\n", perm->confidence);
        }
        
        char perm_str[32];
        security_permissions_to_string(perm->permissions, perm_str, sizeof(perm_str));
        printf("    Permissions: %s\n", perm_str);
        printf("\n");
    }
    
    if (permission_count > 10) {
        printf("  ... and %lu more items\n", (unsigned long)(permission_count - 10));
    }
    
    // Clean up permissions
    for (size_t i = 0; i < permission_count; i++) {
        permission_entry_free(&permissions[i]);
    }
    free(permissions);
    
    // Clean up test environment
    cleanup_test_environment(test_dir);
    security_options_free(&options);
    
    printf("Permission analysis test passed!\n");
    return 0;
}

int test_vulnerability_scanning() {
    printf("\n=== Testing Vulnerability Scanning ===\n");
    
    // Initialize options
    security_options_t options;
    security_options_init(&options);
    options.audit_level = SECURITY_AUDIT_STANDARD;
    options.scan_for_vulnerabilities = 1;
    
    // Scan for vulnerabilities
    vulnerability_entry_t* vulnerabilities;
    size_t vulnerability_count;
    
    int ret = security_scan_vulnerabilities("/", &options, &vulnerabilities, &vulnerability_count);
    
    if (ret != 0) {
        printf("Failed to scan for vulnerabilities\n");
        security_options_free(&options);
        return 1;
    }
    
    printf("Scanned for vulnerabilities, found %lu issues:\n", (unsigned long)vulnerability_count);
    
    // Display results
    for (size_t i = 0; i < vulnerability_count && i < 5; i++) {
        const vulnerability_entry_t* vuln = &vulnerabilities[i];
        printf("  Vulnerability %lu:\n", (unsigned long)i);
        printf("    CVE ID: %s\n", vuln->cve_id ? vuln->cve_id : "Unknown");
        printf("    Description: %s\n", vuln->description ? vuln->description : "Unknown");
        printf("    Affected component: %s\n", vuln->affected_component ? vuln->affected_component : "Unknown");
        printf("    Severity: %s\n", security_get_severity_description(vuln->severity));
        printf("    CVSS Score: %.1f\n", vuln->cvss_score);
        printf("    Remediation: %s\n", vuln->remediation ? vuln->remediation : "None provided");
        printf("    Exploitable: %s\n", vuln->is_exploitable ? "Yes" : "No");
        
        if (vuln->reference_count > 0) {
            printf("    References:\n");
            for (size_t j = 0; j < vuln->reference_count && j < 3; j++) {
                printf("      - %s\n", vuln->references[j] ? vuln->references[j] : "Unknown");
            }
        }
        
        printf("\n");
    }
    
    if (vulnerability_count > 5) {
        printf("  ... and %lu more vulnerabilities\n", (unsigned long)(vulnerability_count - 5));
    }
    
    // Clean up vulnerabilities
    for (size_t i = 0; i < vulnerability_count; i++) {
        vulnerability_entry_free(&vulnerabilities[i]);
    }
    free(vulnerabilities);
    
    security_options_free(&options);
    
    printf("Vulnerability scanning test passed!\n");
    return 0;
}

int test_policy_checking() {
    printf("\n=== Testing Policy Checking ===\n");
    
    // Initialize options
    security_options_t options;
    security_options_init(&options);
    options.audit_level = SECURITY_AUDIT_STANDARD;
    options.check_security_policies = 1;
    
    // Check security policies
    policy_entry_t* policies;
    size_t policy_count;
    
    int ret = security_check_policies(&options, &policies, &policy_count);
    
    if (ret != 0) {
        printf("Failed to check security policies\n");
        security_options_free(&options);
        return 1;
    }
    
    printf("Checked %lu security policies:\n", (unsigned long)policy_count);
    
    // Display results
    for (size_t i = 0; i < policy_count && i < 5; i++) {
        const policy_entry_t* policy = &policies[i];
        printf("  Policy %lu:\n", (unsigned long)i);
        printf("    Type: %s\n", security_get_policy_type_name(policy->type));
        printf("    Name: %s\n", policy->policy_name ? policy->policy_name : "Unknown");
        printf("    Description: %s\n", policy->description ? policy->description : "Unknown");
        printf("    Compliant: %s\n", policy->is_compliant ? "Yes" : "No");
        printf("    Severity: %s\n", security_get_severity_description(policy->severity));
        
        if (!policy->is_compliant) {
            printf("    Violation: %s\n", policy->violation_details ? policy->violation_details : "Unknown");
            printf("    Recommendation: %s\n", policy->recommended_action ? policy->recommended_action : "None provided");
        }
        
        printf("\n");
    }
    
    if (policy_count > 5) {
        printf("  ... and %lu more policies\n", (unsigned long)(policy_count - 5));
    }
    
    // Clean up policies
    for (size_t i = 0; i < policy_count; i++) {
        policy_entry_free(&policies[i]);
    }
    free(policies);
    
    security_options_free(&options);
    
    printf("Policy checking test passed!\n");
    return 0;
}

int test_comprehensive_audit() {
    printf("\n=== Testing Comprehensive Security Audit ===\n");
    
    const char* target_path = "/";
    
    // Initialize options
    security_options_t options;
    security_options_init(&options);
    options.audit_level = SECURITY_AUDIT_STANDARD;
    options.check_file_permissions = 1;
    options.check_directory_permissions = 1;
    options.scan_for_vulnerabilities = 1;
    options.check_security_policies = 1;
    options.generate_reports = 1;
    
    // Perform comprehensive audit
    security_audit_result_t result;
    int ret = security_perform_audit(target_path, &options, &result);
    
    if (ret != 0) {
        printf("Failed to perform comprehensive security audit\n");
        security_options_free(&options);
        return 1;
    }
    
    printf("Comprehensive security audit completed:\n");
    printf("  Target: %s\n", result.target_path ? result.target_path : "Unknown");
    printf("  Audit level: %s\n", security_get_audit_level_description(result.audit_level));
    printf("  Overall security score: %.2f\n", result.overall_security_score);
    printf("  Permissions checked: %lu\n", (unsigned long)result.permission_count);
    printf("  Insecure permissions: %lu\n", (unsigned long)result.insecure_permissions);
    printf("  Vulnerabilities found: %lu\n", (unsigned long)result.vulnerability_count);
    printf("  Critical vulnerabilities: %lu\n", (unsigned long)result.critical_vulnerabilities);
    printf("  Policy violations: %lu\n", (unsigned long)result.policy_violations);
    
    // Display summary if available
    if (result.summary_report) {
        printf("\nSummary Report:\n%s\n", result.summary_report);
    }
    
    // Generate reports
    const char* text_report = "/tmp/security_audit_report.txt";
    const char* json_report = "/tmp/security_audit_report.json";
    
    ret = security_generate_report(&result, text_report, "TEXT");
    if (ret == 0) {
        printf("Text report generated: %s\n", text_report);
    } else {
        printf("Failed to generate text report\n");
    }
    
    ret = security_generate_report(&result, json_report, "JSON");
    if (ret == 0) {
        printf("JSON report generated: %s\n", json_report);
    } else {
        printf("Failed to generate JSON report\n");
    }
    
    // Clean up
    security_audit_result_free(&result);
    security_options_free(&options);
    unlink(text_report);
    unlink(json_report);
    
    printf("Comprehensive security audit test passed!\n");
    return 0;
}

int test_permission_utilities() {
    printf("\n=== Testing Permission Utilities ===\n");
    
    // Test permission string conversion
    uint32_t test_permissions[] = {
        0755, // rwxr-xr-x
        0644, // rw-r--r--
        0777, // rwxrwxrwx
        0600, // rw-------
        0444  // r--r--r--
    };
    
    int num_tests = sizeof(test_permissions) / sizeof(test_permissions[0]);
    
    for (int i = 0; i < num_tests; i++) {
        char perm_str[32];
        int ret = security_permissions_to_string(test_permissions[i], perm_str, sizeof(perm_str));
        
        if (ret == 0) {
            printf("Permission 0%o -> %s\n", test_permissions[i], perm_str);
        } else {
            printf("Failed to convert permission 0%o\n", test_permissions[i]);
        }
    }
    
    printf("Permission utilities test passed!\n");
    return 0;
}

int test_severity_descriptions() {
    printf("\n=== Testing Severity Descriptions ===\n");
    
    vulnerability_severity_t severities[] = {
        VULNERABILITY_LOW,
        VULNERABILITY_MEDIUM,
        VULNERABILITY_HIGH,
        VULNERABILITY_CRITICAL
    };
    
    const char* severity_names[] = {
        "Low",
        "Medium", 
        "High",
        "Critical"
    };
    
    int num_severities = sizeof(severities) / sizeof(severities[0]);
    
    for (int i = 0; i < num_severities; i++) {
        const char* description = security_get_severity_description(severities[i]);
        printf("Severity %s: %s\n", severity_names[i], description);
    }
    
    printf("Severity descriptions test passed!\n");
    return 0;
}

int test_audit_levels() {
    printf("\n=== Testing Audit Levels ===\n");
    
    security_audit_level_t levels[] = {
        SECURITY_AUDIT_BASIC,
        SECURITY_AUDIT_STANDARD,
        SECURITY_AUDIT_DETAILED,
        SECURITY_AUDIT_COMPLIANCE
    };
    
    const char* level_names[] = {
        "Basic",
        "Standard",
        "Detailed",
        "Compliance"
    };
    
    int num_levels = sizeof(levels) / sizeof(levels[0]);
    
    for (int i = 0; i < num_levels; i++) {
        const char* description = security_get_audit_level_description(levels[i]);
        printf("Audit level %s: %s\n", level_names[i], description);
    }
    
    printf("Audit levels test passed!\n");
    return 0;
}

int test_policy_types() {
    printf("\n=== Testing Policy Types ===\n");
    
    policy_type_t policy_types[] = {
        POLICY_TYPE_PASSWORD,
        POLICY_TYPE_ACCOUNT,
        POLICY_TYPE_AUDIT,
        POLICY_TYPE_PRIVILEGE,
        POLICY_TYPE_NETWORK,
        POLICY_TYPE_ENCRYPTION
    };
    
    const char* policy_names[] = {
        "Password",
        "Account",
        "Audit",
        "Privilege",
        "Network",
        "Encryption"
    };
    
    int num_types = sizeof(policy_types) / sizeof(policy_types[0]);
    
    for (int i = 0; i < num_types; i++) {
        const char* description = security_get_policy_type_name(policy_types[i]);
        printf("Policy type %s: %s\n", policy_names[i], description);
    }
    
    printf("Policy types test passed!\n");
    return 0;
}

int test_permission_types() {
    printf("\n=== Testing Permission Types ===\n");
    
    permission_type_t permission_types[] = {
        PERMISSION_TYPE_UNKNOWN,
        PERMISSION_TYPE_FILE,
        PERMISSION_TYPE_DIRECTORY,
        PERMISSION_TYPE_REGISTRY,
        PERMISSION_TYPE_SERVICE,
        PERMISSION_TYPE_PROCESS
    };
    
    const char* permission_names[] = {
        "Unknown",
        "File",
        "Directory",
        "Registry",
        "Service",
        "Process"
    };
    
    int num_types = sizeof(permission_types) / sizeof(permission_types[0]);
    
    for (int i = 0; i < num_types; i++) {
        const char* description = security_get_permission_type_name(permission_types[i]);
        printf("Permission type %s: %s\n", permission_names[i], description);
    }
    
    printf("Permission types test passed!\n");
    return 0;
}

int main() {
    srand((unsigned int)time(NULL)); // Initialize random seed
    
    printf("Running comprehensive security audit tests...\n");
    
    int result1 = test_security_options();
    if (result1 != 0) {
        return result1;
    }
    
    int result2 = test_permission_analysis();
    if (result2 != 0) {
        return result2;
    }
    
    int result3 = test_vulnerability_scanning();
    if (result3 != 0) {
        return result3;
    }
    
    int result4 = test_policy_checking();
    if (result4 != 0) {
        return result4;
    }
    
    int result5 = test_comprehensive_audit();
    if (result5 != 0) {
        return result5;
    }
    
    int result6 = test_permission_utilities();
    if (result6 != 0) {
        return result6;
    }
    
    int result7 = test_severity_descriptions();
    if (result7 != 0) {
        return result7;
    }
    
    int result8 = test_audit_levels();
    if (result8 != 0) {
        return result8;
    }
    
    int result9 = test_policy_types();
    if (result9 != 0) {
        return result9;
    }
    
    int result10 = test_permission_types();
    if (result10 != 0) {
        return result10;
    }
    
    printf("\n=== All Security Audit Tests Passed! ===\n");
    return 0;
}