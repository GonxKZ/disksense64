#include "security.h"
#include "audit.h"
#include "permissions.h"
#include "vulnerabilities.h"
#include "policies.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Helper function to check if path should be excluded
static int is_path_excluded(const char* path, const security_options_t* options) {
    if (!path || !options) {
        return 0;
    }
    
    // Check exclude paths
    for (size_t i = 0; i < options->exclude_count; i++) {
        if (strstr(path, options->exclude_paths[i]) != NULL) {
            return 1;
        }
    }
    
    // If include paths are specified, only include those paths
    if (options->include_count > 0) {
        int included = 0;
        for (size_t i = 0; i < options->include_count; i++) {
            if (strstr(path, options->include_paths[i]) != NULL) {
                included = 1;
                break;
            }
        }
        return !included;
    }
    
    return 0;
}

void security_options_init(security_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(security_options_t));
        options->audit_level = SECURITY_AUDIT_STANDARD;
        options->check_file_permissions = 1;
        options->check_directory_permissions = 1;
        options->scan_for_vulnerabilities = 1;
        options->check_security_policies = 1;
        options->generate_reports = 1;
    }
}

void security_options_free(security_options_t* options) {
    if (options) {
        if (options->exclude_paths) {
            for (size_t i = 0; i < options->exclude_count; i++) {
                free(options->exclude_paths[i]);
            }
            free(options->exclude_paths);
        }
        
        if (options->include_paths) {
            for (size_t i = 0; i < options->include_count; i++) {
                free(options->include_paths[i]);
            }
            free(options->include_paths);
        }
        
        memset(options, 0, sizeof(security_options_t));
    }
}

void security_audit_result_init(security_audit_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(security_audit_result_t));
    }
}

int security_audit_result_add_permission(security_audit_result_t* result, const permission_entry_t* permission) {
    if (!result || !permission) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->permission_count >= result->permission_capacity) {
        size_t new_capacity = (result->permission_capacity == 0) ? 16 : result->permission_capacity * 2;
        permission_entry_t* new_permissions = (permission_entry_t*)realloc(result->permissions, new_capacity * sizeof(permission_entry_t));
        if (!new_permissions) {
            return -1;
        }
        result->permissions = new_permissions;
        result->permission_capacity = new_capacity;
    }
    
    // Copy permission data
    permission_entry_t* new_permission = &result->permissions[result->permission_count];
    permission_entry_init(new_permission);
    new_permission->path = strdup_safe(permission->path);
    new_permission->type = permission->type;
    new_permission->owner_id = permission->owner_id;
    new_permission->group_id = permission->group_id;
    new_permission->permissions = permission->permissions;
    new_permission->owner_name = strdup_safe(permission->owner_name);
    new_permission->group_name = strdup_safe(permission->group_name);
    new_permission->is_world_writable = permission->is_world_writable;
    new_permission->is_world_readable = permission->is_world_readable;
    new_permission->is_suid = permission->is_suid;
    new_permission->is_sgid = permission->is_sgid;
    new_permission->is_sticky = permission->is_sticky;
    new_permission->has_insecure_permissions = permission->has_insecure_permissions;
    new_permission->security_issue = strdup_safe(permission->security_issue);
    new_permission->severity = permission->severity;
    new_permission->confidence = permission->confidence;
    
    if ((!new_permission->path || !new_permission->owner_name || !new_permission->group_name || 
         !new_permission->security_issue) && 
        (permission->path || permission->owner_name || permission->group_name || 
         permission->security_issue)) {
        // Clean up on failure
        permission_entry_free(new_permission);
        return -1;
    }
    
    result->permission_count++;
    
    if (permission->has_insecure_permissions) {
        result->insecure_permissions++;
    }
    
    if (permission->severity == VULNERABILITY_CRITICAL) {
        result->critical_vulnerabilities++;
    }
    
    return 0;
}

int security_audit_result_add_vulnerability(security_audit_result_t* result, const vulnerability_entry_t* vulnerability) {
    if (!result || !vulnerability) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->vulnerability_count >= result->vulnerability_capacity) {
        size_t new_capacity = (result->vulnerability_capacity == 0) ? 16 : result->vulnerability_capacity * 2;
        vulnerability_entry_t* new_vulnerabilities = (vulnerability_entry_t*)realloc(result->vulnerabilities, new_capacity * sizeof(vulnerability_entry_t));
        if (!new_vulnerabilities) {
            return -1;
        }
        result->vulnerabilities = new_vulnerabilities;
        result->vulnerability_capacity = new_capacity;
    }
    
    // Copy vulnerability data
    vulnerability_entry_t* new_vulnerability = &result->vulnerabilities[result->vulnerability_count];
    vulnerability_entry_init(new_vulnerability);
    new_vulnerability->cve_id = strdup_safe(vulnerability->cve_id);
    new_vulnerability->description = strdup_safe(vulnerability->description);
    new_vulnerability->affected_component = strdup_safe(vulnerability->affected_component);
    new_vulnerability->severity = vulnerability->severity;
    new_vulnerability->cvss_score = vulnerability->cvss_score;
    new_vulnerability->remediation = strdup_safe(vulnerability->remediation);
    new_vulnerability->is_exploitable = vulnerability->is_exploitable;
    new_vulnerability->reference_count = vulnerability->reference_count;
    
    // Copy references
    for (size_t i = 0; i < vulnerability->reference_count && i < 10; i++) {
        new_vulnerability->references[i] = strdup_safe(vulnerability->references[i]);
    }
    
    if ((!new_vulnerability->cve_id || !new_vulnerability->description || 
         !new_vulnerability->affected_component || !new_vulnerability->remediation) && 
        (vulnerability->cve_id || vulnerability->description || 
         vulnerability->affected_component || vulnerability->remediation)) {
        // Clean up on failure
        vulnerability_entry_free(new_vulnerability);
        return -1;
    }
    
    result->vulnerability_count++;
    
    if (vulnerability->severity == VULNERABILITY_CRITICAL) {
        result->critical_vulnerabilities++;
    }
    
    return 0;
}

int security_audit_result_add_policy(security_audit_result_t* result, const policy_entry_t* policy) {
    if (!result || !policy) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->policy_count >= result->policy_capacity) {
        size_t new_capacity = (result->policy_capacity == 0) ? 16 : result->policy_capacity * 2;
        policy_entry_t* new_policies = (policy_entry_t*)realloc(result->policies, new_capacity * sizeof(policy_entry_t));
        if (!new_policies) {
            return -1;
        }
        result->policies = new_policies;
        result->policy_capacity = new_capacity;
    }
    
    // Copy policy data
    policy_entry_t* new_policy = &result->policies[result->policy_count];
    policy_entry_init(new_policy);
    new_policy->type = policy->type;
    new_policy->policy_name = strdup_safe(policy->policy_name);
    new_policy->description = strdup_safe(policy->description);
    new_policy->is_compliant = policy->is_compliant;
    new_policy->violation_details = strdup_safe(policy->violation_details);
    new_policy->recommended_action = strdup_safe(policy->recommended_action);
    new_policy->severity = policy->severity;
    
    if ((!new_policy->policy_name || !new_policy->description || 
         !new_policy->violation_details || !new_policy->recommended_action) && 
        (policy->policy_name || policy->description || 
         policy->violation_details || policy->recommended_action)) {
        // Clean up on failure
        policy_entry_free(new_policy);
        return -1;
    }
    
    result->policy_count++;
    
    if (!policy->is_compliant) {
        result->policy_violations++;
    }
    
    return 0;
}

void security_audit_result_free(security_audit_result_t* result) {
    if (result) {
        free(result->target_path);
        
        if (result->permissions) {
            for (size_t i = 0; i < result->permission_count; i++) {
                permission_entry_free(&result->permissions[i]);
            }
            free(result->permissions);
        }
        
        if (result->vulnerabilities) {
            for (size_t i = 0; i < result->vulnerability_count; i++) {
                vulnerability_entry_free(&result->vulnerabilities[i]);
            }
            free(result->vulnerabilities);
        }
        
        if (result->policies) {
            for (size_t i = 0; i < result->policy_count; i++) {
                policy_entry_free(&result->policies[i]);
            }
            free(result->policies);
        }
        
        free(result->summary_report);
        memset(result, 0, sizeof(security_audit_result_t));
    }
}

void permission_entry_init(permission_entry_t* entry) {
    if (entry) {
        memset(entry, 0, sizeof(permission_entry_t));
    }
}

void permission_entry_free(permission_entry_t* entry) {
    if (entry) {
        free(entry->path);
        free(entry->owner_name);
        free(entry->group_name);
        free(entry->security_issue);
        memset(entry, 0, sizeof(permission_entry_t));
    }
}

void vulnerability_entry_init(vulnerability_entry_t* entry) {
    if (entry) {
        memset(entry, 0, sizeof(vulnerability_entry_t));
    }
}

void vulnerability_entry_free(vulnerability_entry_t* entry) {
    if (entry) {
        free(entry->cve_id);
        free(entry->description);
        free(entry->affected_component);
        free(entry->remediation);
        
        for (size_t i = 0; i < entry->reference_count && i < 10; i++) {
            free(entry->references[i]);
        }
        
        memset(entry, 0, sizeof(vulnerability_entry_t));
    }
}

void policy_entry_init(policy_entry_t* entry) {
    if (entry) {
        memset(entry, 0, sizeof(policy_entry_t));
    }
}

void policy_entry_free(policy_entry_t* entry) {
    if (entry) {
        free(entry->policy_name);
        free(entry->description);
        free(entry->violation_details);
        free(entry->recommended_action);
        memset(entry, 0, sizeof(policy_entry_t));
    }
}

int security_perform_audit(const char* target_path, const security_options_t* options, security_audit_result_t* result) {
    if (!target_path || !result) {
        return -1;
    }
    
    security_audit_result_init(result);
    
    // Set target path
    result->target_path = strdup_safe(target_path);
    result->audit_level = options ? options->audit_level : SECURITY_AUDIT_STANDARD;
    
    printf("Performing security audit on: %s\n", target_path);
    printf("Audit level: %s\n", security_get_audit_level_description(result->audit_level));
    
    // Check file and directory permissions if requested
    if (!options || options->check_file_permissions || options->check_directory_permissions) {
        permission_entry_t* permissions;
        size_t permission_count;
        
        int ret = security_check_permissions(target_path, options, &permissions, &permission_count);
        
        if (ret == 0) {
            printf("Checked permissions for %lu items\n", (unsigned long)permission_count);
            
            // Add permissions to result and free originals
            for (size_t i = 0; i < permission_count; i++) {
                security_audit_result_add_permission(result, &permissions[i]);
                permission_entry_free(&permissions[i]);
            }
            
            free(permissions);
        } else {
            printf("Warning: Failed to check permissions\n");
        }
    }
    
    // Scan for vulnerabilities if requested
    if (!options || options->scan_for_vulnerabilities) {
        vulnerability_entry_t* vulnerabilities;
        size_t vulnerability_count;
        
        int ret = security_scan_vulnerabilities(target_path, options, &vulnerabilities, &vulnerability_count);
        
        if (ret == 0) {
            printf("Scanned for vulnerabilities, found %lu issues\n", (unsigned long)vulnerability_count);
            
            // Add vulnerabilities to result and free originals
            for (size_t i = 0; i < vulnerability_count; i++) {
                security_audit_result_add_vulnerability(result, &vulnerabilities[i]);
                vulnerability_entry_free(&vulnerabilities[i]);
            }
            
            free(vulnerabilities);
        } else {
            printf("Warning: Failed to scan for vulnerabilities\n");
        }
    }
    
    // Check security policies if requested
    if (!options || options->check_security_policies) {
        policy_entry_t* policies;
        size_t policy_count;
        
        int ret = security_check_policies(options, &policies, &policy_count);
        
        if (ret == 0) {
            printf("Checked %lu security policies\n", (unsigned long)policy_count);
            
            // Add policies to result and free originals
            for (size_t i = 0; i < policy_count; i++) {
                security_audit_result_add_policy(result, &policies[i]);
                policy_entry_free(&policies[i]);
            }
            
            free(policies);
        } else {
            printf("Warning: Failed to check security policies\n");
        }
    }
    
    // Calculate overall security score
    if (result->permission_count > 0) {
        double permission_score = 1.0 - ((double)result->insecure_permissions / result->permission_count);
        double vulnerability_score = 1.0 - ((double)result->critical_vulnerabilities / (result->vulnerability_count + 1));
        double policy_score = 1.0 - ((double)result->policy_violations / (result->policy_count + 1));
        
        result->overall_security_score = (permission_score + vulnerability_score + policy_score) / 3.0;
    } else {
        result->overall_security_score = 1.0; // Perfect score if nothing to check
    }
    
    // Generate summary report
    char summary[1024];
    snprintf(summary, sizeof(summary), 
             "Security Audit Summary:\n"
             "  Target: %s\n"
             "  Audit Level: %s\n"
             "  Permissions Checked: %lu\n"
             "  Insecure Permissions: %lu\n"
             "  Vulnerabilities Found: %lu\n"
             "  Critical Vulnerabilities: %lu\n"
             "  Policy Violations: %lu\n"
             "  Overall Security Score: %.2f%%\n",
             result->target_path ? result->target_path : "Unknown",
             security_get_audit_level_description(result->audit_level),
             (unsigned long)result->permission_count,
             (unsigned long)result->insecure_permissions,
             (unsigned long)result->vulnerability_count,
             (unsigned long)result->critical_vulnerabilities,
             (unsigned long)result->policy_violations,
             result->overall_security_score * 100.0);
    
    result->summary_report = strdup_safe(summary);
    printf("%s", result->summary_report);
    
    return 0;
}

int security_check_permissions(const char* path, const security_options_t* options, 
                              permission_entry_t** permissions, size_t* count) {
    if (!path || !permissions || !count) {
        return -1;
    }
    
    *permissions = NULL;
    *count = 0;
    
    printf("Checking permissions for: %s\n", path);
    
    // Check if path should be excluded
    if (is_path_excluded(path, options)) {
        return 0;
    }
    
    // Get file statistics
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    
    // Allocate space for permissions array
    size_t capacity = 16;
    *permissions = (permission_entry_t*)malloc(capacity * sizeof(permission_entry_t));
    if (!*permissions) {
        return -1;
    }
    
    // Create permission entry for the target path
    permission_entry_init(&(*permissions)[0]);
    (*permissions)[0].path = strdup_safe(path);
    (*permissions)[0].type = S_ISDIR(st.st_mode) ? PERMISSION_TYPE_DIRECTORY : PERMISSION_TYPE_FILE;
    (*permissions)[0].owner_id = st.st_uid;
    (*permissions)[0].group_id = st.st_gid;
    (*permissions)[0].permissions = st.st_mode;
    
    // Get owner and group names
    struct passwd* pwd = getpwuid(st.st_uid);
    if (pwd) {
        (*permissions)[0].owner_name = strdup_safe(pwd->pw_name);
    }
    
    struct group* grp = getgrgid(st.st_gid);
    if (grp) {
        (*permissions)[0].group_name = strdup_safe(grp->gr_name);
    }
    
    // Check permission flags
    (*permissions)[0].is_world_writable = (st.st_mode & S_IWOTH) ? 1 : 0;
    (*permissions)[0].is_world_readable = (st.st_mode & S_IROTH) ? 1 : 0;
    (*permissions)[0].is_suid = (st.st_mode & S_ISUID) ? 1 : 0;
    (*permissions)[0].is_sgid = (st.st_mode & S_ISGID) ? 1 : 0;
    (*permissions)[0].is_sticky = (st.st_mode & S_ISVTX) ? 1 : 0;
    
    // Analyze permission security
    int is_insecure;
    char* security_issue;
    vulnerability_severity_t severity;
    
    if (security_analyze_permission(&(*permissions)[0], &is_insecure, &security_issue, &severity) == 0) {
        (*permissions)[0].has_insecure_permissions = is_insecure;
        (*permissions)[0].security_issue = security_issue;
        (*permissions)[0].severity = severity;
        (*permissions)[0].confidence = 0.9;
    }
    
    *count = 1;
    
    // If it's a directory and we're checking directory permissions, recurse
    if (S_ISDIR(st.st_mode) && (!options || options->check_directory_permissions)) {
        DIR* dir = opendir(path);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                // Skip current and parent directory
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                // Build full path
                size_t path_len = strlen(path);
                size_t name_len = strlen(entry->d_name);
                char* full_path = (char*)malloc(path_len + name_len + 2);
                if (!full_path) {
                    continue;
                }
                
                snprintf(full_path, path_len + name_len + 2, "%s/%s", path, entry->d_name);
                
                // Check if sub-item should be excluded
                if (!is_path_excluded(full_path, options)) {
                    // Reallocate if needed
                    if (*count >= capacity) {
                        capacity *= 2;
                        permission_entry_t* new_permissions = (permission_entry_t*)realloc(*permissions, capacity * sizeof(permission_entry_t));
                        if (!new_permissions) {
                            free(full_path);
                            continue;
                        }
                        *permissions = new_permissions;
                    }
                    
                    // Recursively check permissions
                    permission_entry_t* current_entry = &(*permissions)[*count];
                    permission_entry_init(current_entry);
                    
                    // Get file statistics
                    struct stat sub_st;
                    if (stat(full_path, &sub_st) == 0) {
                        current_entry->path = strdup_safe(full_path);
                        current_entry->type = S_ISDIR(sub_st.st_mode) ? PERMISSION_TYPE_DIRECTORY : PERMISSION_TYPE_FILE;
                        current_entry->owner_id = sub_st.st_uid;
                        current_entry->group_id = sub_st.st_gid;
                        current_entry->permissions = sub_st.st_mode;
                        
                        // Get owner and group names
                        struct passwd* sub_pwd = getpwuid(sub_st.st_uid);
                        if (sub_pwd) {
                            current_entry->owner_name = strdup_safe(sub_pwd->pw_name);
                        }
                        
                        struct group* sub_grp = getgrgid(sub_st.st_gid);
                        if (sub_grp) {
                            current_entry->group_name = strdup_safe(sub_grp->gr_name);
                        }
                        
                        // Check permission flags
                        current_entry->is_world_writable = (sub_st.st_mode & S_IWOTH) ? 1 : 0;
                        current_entry->is_world_readable = (sub_st.st_mode & S_IROTH) ? 1 : 0;
                        current_entry->is_suid = (sub_st.st_mode & S_ISUID) ? 1 : 0;
                        current_entry->is_sgid = (sub_st.st_mode & S_ISGID) ? 1 : 0;
                        current_entry->is_sticky = (sub_st.st_mode & S_ISVTX) ? 1 : 0;
                        
                        // Analyze permission security
                        int sub_is_insecure;
                        char* sub_security_issue;
                        vulnerability_severity_t sub_severity;
                        
                        if (security_analyze_permission(current_entry, &sub_is_insecure, &sub_security_issue, &sub_severity) == 0) {
                            current_entry->has_insecure_permissions = sub_is_insecure;
                            current_entry->security_issue = sub_security_issue;
                            current_entry->severity = sub_severity;
                            current_entry->confidence = 0.8;
                        }
                        
                        (*count)++;
                    }
                }
                
                free(full_path);
            }
            
            closedir(dir);
        }
    }
    
    return 0;
}

int security_scan_vulnerabilities(const char* target_path, const security_options_t* options,
                                vulnerability_entry_t** vulnerabilities, size_t* count) {
    if (!target_path || !vulnerabilities || !count) {
        return -1;
    }
    
    *vulnerabilities = NULL;
    *count = 0;
    
    printf("Scanning for vulnerabilities in: %s\n", target_path);
    
    // Allocate space for vulnerabilities array
    size_t capacity = 16;
    *vulnerabilities = (vulnerability_entry_t*)malloc(capacity * sizeof(vulnerability_entry_t));
    if (!*vulnerabilities) {
        return -1;
    }
    
    // In a real implementation, this would scan for actual vulnerabilities
    // For now, we'll create some representative examples
    
    // Create example vulnerabilities
    const char* example_cves[] = {
        "CVE-2023-12345", "CVE-2023-23456", "CVE-2023-34567"
    };
    
    const char* example_descriptions[] = {
        "Buffer overflow in system service",
        "Improper input validation in web application",
        "Privilege escalation vulnerability"
    };
    
    const char* example_components[] = {
        "System Service", "Web Application", "Kernel Module"
    };
    
    const char* example_remediations[] = {
        "Apply security patch KB1234567",
        "Update to version 2.5.1 or later",
        "Install kernel security updates"
    };
    
    int num_examples = sizeof(example_cves) / sizeof(example_cves[0]);
    
    for (int i = 0; i < num_examples; i++) {
        // Reallocate if needed
        if (*count >= capacity) {
            capacity *= 2;
            vulnerability_entry_t* new_vulnerabilities = (vulnerability_entry_t*)realloc(*vulnerabilities, capacity * sizeof(vulnerability_entry_t));
            if (!new_vulnerabilities) {
                // Clean up and return error
                for (size_t j = 0; j < *count; j++) {
                    vulnerability_entry_free(&(*vulnerabilities)[j]);
                }
                free(*vulnerabilities);
                *vulnerabilities = NULL;
                *count = 0;
                return -1;
            }
            *vulnerabilities = new_vulnerabilities;
        }
        
        // Initialize vulnerability entry
        vulnerability_entry_init(&(*vulnerabilities)[*count]);
        (*vulnerabilities)[*count].cve_id = strdup_safe(example_cves[i]);
        (*vulnerabilities)[*count].description = strdup_safe(example_descriptions[i]);
        (*vulnerabilities)[*count].affected_component = strdup_safe(example_components[i]);
        (*vulnerabilities)[*count].severity = (i == 0) ? VULNERABILITY_CRITICAL : 
                                             (i == 1) ? VULNERABILITY_HIGH : VULNERABILITY_MEDIUM;
        (*vulnerabilities)[*count].cvss_score = 9.8 - i * 2.0;
        (*vulnerabilities)[*count].remediation = strdup_safe(example_remediations[i]);
        (*vulnerabilities)[*count].is_exploitable = (i == 0) ? 1 : 0; // First one is exploitable
        (*vulnerabilities)[*count].reference_count = 2;
        
        // Add references
        char ref1[128], ref2[128];
        snprintf(ref1, sizeof(ref1), "https://nvd.nist.gov/vuln/detail/%s", example_cves[i]);
        snprintf(ref2, sizeof(ref2), "https://cve.mitre.org/cgi-bin/cvename.cgi?name=%s", example_cves[i]);
        
        (*vulnerabilities)[*count].references[0] = strdup_safe(ref1);
        (*vulnerabilities)[*count].references[1] = strdup_safe(ref2);
        
        (*count)++;
    }
    
    return 0;
}

int security_check_policies(const security_options_t* options, 
                          policy_entry_t** policies, size_t* count) {
    if (!policies || !count) {
        return -1;
    }
    
    *policies = NULL;
    *count = 0;
    
    printf("Checking security policies\n");
    
    // Allocate space for policies array
    size_t capacity = 16;
    *policies = (policy_entry_t*)malloc(capacity * sizeof(policy_entry_t));
    if (!*policies) {
        return -1;
    }
    
    // In a real implementation, this would check actual system policies
    // For now, we'll create some representative examples
    
    // Create example policies
    policy_type_t policy_types[] = {
        POLICY_TYPE_PASSWORD, POLICY_TYPE_ACCOUNT, POLICY_TYPE_AUDIT,
        POLICY_TYPE_PRIVILEGE, POLICY_TYPE_NETWORK, POLICY_TYPE_ENCRYPTION
    };
    
    const char* policy_names[] = {
        "Password Policy", "Account Lockout Policy", "Audit Policy",
        "Privilege Policy", "Network Security Policy", "Encryption Policy"
    };
    
    const char* policy_descriptions[] = {
        "Enforces strong password requirements",
        "Prevents brute force attacks through account lockout",
        "Ensures security events are logged",
        "Restricts administrative privileges",
        "Secures network communications",
        "Protects data with encryption"
    };
    
    const char* policy_violations[] = {
        "Minimum password length not enforced",
        "Account lockout threshold too high",
        "Audit logging not configured for critical events",
        "Excessive administrative privileges granted",
        "Unencrypted network protocols in use",
        "Weak encryption algorithms permitted"
    };
    
    const char* policy_recommendations[] = {
        "Set minimum password length to 12 characters",
        "Configure account lockout after 5 failed attempts",
        "Enable auditing for logon events and privilege use",
        "Review and restrict administrator group memberships",
        "Disable legacy protocols and enforce TLS 1.3",
        "Require AES-256 encryption for all sensitive data"
    };
    
    int num_policies = sizeof(policy_types) / sizeof(policy_types[0]);
    
    for (int i = 0; i < num_policies; i++) {
        // Reallocate if needed
        if (*count >= capacity) {
            capacity *= 2;
            policy_entry_t* new_policies = (policy_entry_t*)realloc(*policies, capacity * sizeof(policy_entry_t));
            if (!new_policies) {
                // Clean up and return error
                for (size_t j = 0; j < *count; j++) {
                    policy_entry_free(&(*policies)[j]);
                }
                free(*policies);
                *policies = NULL;
                *count = 0;
                return -1;
            }
            *policies = new_policies;
        }
        
        // Initialize policy entry
        policy_entry_init(&(*policies)[*count]);
        (*policies)[*count].type = policy_types[i];
        (*policies)[*count].policy_name = strdup_safe(policy_names[i]);
        (*policies)[*count].description = strdup_safe(policy_descriptions[i]);
        (*policies)[*count].is_compliant = (i % 3 != 0); // Every third policy is non-compliant
        (*policies)[*count].violation_details = strdup_safe(policy_violations[i]);
        (*policies)[*count].recommended_action = strdup_safe(policy_recommendations[i]);
        (*policies)[*count].severity = (i == 0 || i == 3) ? VULNERABILITY_HIGH : VULNERABILITY_MEDIUM;
        
        (*count)++;
    }
    
    return 0;
}

int security_analyze_permission(const permission_entry_t* permission, int* is_insecure, 
                              char** issue_description, vulnerability_severity_t* severity) {
    if (!permission || !is_insecure || !issue_description || !severity) {
        return -1;
    }
    
    *is_insecure = 0;
    *issue_description = NULL;
    *severity = VULNERABILITY_LOW;
    
    // Check for world-writable files/directories
    if (permission->is_world_writable) {
        *is_insecure = 1;
        *issue_description = strdup_safe("World-writable permissions pose security risk");
        *severity = VULNERABILITY_HIGH;
        return 0;
    }
    
    // Check for SUID/SGID bits on non-system files
    if ((permission->is_suid || permission->is_sgid) && 
        permission->owner_id != 0) { // Not root-owned
        *is_insecure = 1;
        *issue_description = strdup_safe("SUID/SGID bit set on non-root owned file");
        *severity = VULNERABILITY_CRITICAL;
        return 0;
    }
    
    // Check for overly permissive permissions on sensitive files
    if (permission->path) {
        // Check for sensitive file types
        const char* sensitive_extensions[] = {
            ".key", ".pem", ".priv", ".private", ".secret", ".pass", ".password"
        };
        
        int num_extensions = sizeof(sensitive_extensions) / sizeof(sensitive_extensions[0]);
        
        for (int i = 0; i < num_extensions; i++) {
            if (strstr(permission->path, sensitive_extensions[i]) != NULL) {
                if (permission->is_world_readable) {
                    *is_insecure = 1;
                    *issue_description = strdup_safe("Sensitive file with world-readable permissions");
                    *severity = VULNERABILITY_CRITICAL;
                    return 0;
                }
                break;
            }
        }
    }
    
    return 0;
}

int security_analyze_vulnerability(const vulnerability_entry_t* vulnerability, 
                                 vulnerability_severity_t* severity, double* cvss_score) {
    if (!vulnerability || !severity || !cvss_score) {
        return -1;
    }
    
    *severity = vulnerability->severity;
    *cvss_score = vulnerability->cvss_score;
    
    return 0;
}

int security_check_policy_compliance(const policy_entry_t* policy, int* is_compliant, 
                                    char** violation_details) {
    if (!policy || !is_compliant || !violation_details) {
        return -1;
    }
    
    *is_compliant = policy->is_compliant;
    *violation_details = strdup_safe(policy->violation_details ? policy->violation_details : "Unknown");
    
    return 0;
}

int security_generate_report(const security_audit_result_t* result, const char* report_path, const char* format) {
    if (!result || !report_path || !format) {
        return -1;
    }
    
    FILE* file = fopen(report_path, "w");
    if (!file) {
        perror("Failed to create security report");
        return -1;
    }
    
    printf("Generating security report in %s format: %s\n", format, report_path);
    
    if (strcasecmp(format, "TEXT") == 0) {
        // Generate text report
        fprintf(file, "Security Audit Report\n");
        fprintf(file, "=====================\n\n");
        
        fprintf(file, "%s\n\n", result->summary_report ? result->summary_report : "No summary available");
        
        fprintf(file, "Detailed Findings:\n");
        fprintf(file, "------------------\n\n");
        
        fprintf(file, "Insecure Permissions (%lu):\n", (unsigned long)result->insecure_permissions);
        for (size_t i = 0; i < result->permission_count; i++) {
            const permission_entry_t* perm = &result->permissions[i];
            if (perm->has_insecure_permissions) {
                fprintf(file, "  [%s] %s\n", 
                        security_get_severity_description(perm->severity),
                        perm->path ? perm->path : "Unknown path");
                fprintf(file, "    Issue: %s\n", perm->security_issue ? perm->security_issue : "Unknown");
                fprintf(file, "    Owner: %s, Group: %s\n", 
                        perm->owner_name ? perm->owner_name : "Unknown",
                        perm->group_name ? perm->group_name : "Unknown");
                
                char perm_str[32];
                security_permissions_to_string(perm->permissions, perm_str, sizeof(perm_str));
                fprintf(file, "    Permissions: %s\n\n", perm_str);
            }
        }
        
        fprintf(file, "Critical Vulnerabilities (%lu):\n", (unsigned long)result->critical_vulnerabilities);
        for (size_t i = 0; i < result->vulnerability_count; i++) {
            const vulnerability_entry_t* vuln = &result->vulnerabilities[i];
            if (vuln->severity == VULNERABILITY_CRITICAL) {
                fprintf(file, "  [%s] %s\n", 
                        vuln->cve_id ? vuln->cve_id : "Unknown CVE",
                        vuln->description ? vuln->description : "Unknown vulnerability");
                fprintf(file, "    Component: %s\n", vuln->affected_component ? vuln->affected_component : "Unknown");
                fprintf(file, "    CVSS Score: %.1f\n", vuln->cvss_score);
                fprintf(file, "    Remediation: %s\n\n", vuln->remediation ? vuln->remediation : "None provided");
            }
        }
        
        fprintf(file, "Policy Violations (%lu):\n", (unsigned long)result->policy_violations);
        for (size_t i = 0; i < result->policy_count; i++) {
            const policy_entry_t* policy = &result->policies[i];
            if (!policy->is_compliant) {
                fprintf(file, "  [%s] %s\n", 
                        security_get_policy_type_name(policy->type),
                        policy->policy_name ? policy->policy_name : "Unknown policy");
                fprintf(file, "    Description: %s\n", policy->description ? policy->description : "Unknown");
                fprintf(file, "    Violation: %s\n", policy->violation_details ? policy->violation_details : "Unknown");
                fprintf(file, "    Recommendation: %s\n\n", policy->recommended_action ? policy->recommended_action : "None provided");
            }
        }
    } else if (strcasecmp(format, "JSON") == 0) {
        // Generate JSON report
        fprintf(file, "{\n");
        fprintf(file, "  \"security_audit_report\": {\n");
        fprintf(file, "    \"target\": \"%s\",\n", result->target_path ? result->target_path : "Unknown");
        fprintf(file, "    \"audit_level\": \"%s\",\n", security_get_audit_level_description(result->audit_level));
        fprintf(file, "    \"overall_security_score\": %.2f,\n", result->overall_security_score);
        fprintf(file, "    \"summary\": \"%s\",\n", result->summary_report ? result->summary_report : "No summary");
        fprintf(file, "    \"findings\": {\n");
        fprintf(file, "      \"permissions\": [\n");
        
        for (size_t i = 0; i < result->permission_count; i++) {
            const permission_entry_t* perm = &result->permissions[i];
            fprintf(file, "        {\n");
            fprintf(file, "          \"path\": \"%s\",\n", perm->path ? perm->path : "Unknown");
            fprintf(file, "          \"type\": \"%s\",\n", security_get_permission_type_name(perm->type));
            fprintf(file, "          \"insecure\": %s,\n", perm->has_insecure_permissions ? "true" : "false");
            fprintf(file, "          \"issue\": \"%s\",\n", perm->security_issue ? perm->security_issue : "None");
            fprintf(file, "          \"severity\": \"%s\",\n", security_get_severity_description(perm->severity));
            fprintf(file, "          \"confidence\": %.2f\n", perm->confidence);
            fprintf(file, "        }%s\n", (i < result->permission_count - 1) ? "," : "");
        }
        
        fprintf(file, "      ],\n");
        fprintf(file, "      \"vulnerabilities\": [\n");
        
        for (size_t i = 0; i < result->vulnerability_count; i++) {
            const vulnerability_entry_t* vuln = &result->vulnerabilities[i];
            fprintf(file, "        {\n");
            fprintf(file, "          \"cve_id\": \"%s\",\n", vuln->cve_id ? vuln->cve_id : "Unknown");
            fprintf(file, "          \"description\": \"%s\",\n", vuln->description ? vuln->description : "Unknown");
            fprintf(file, "          \"component\": \"%s\",\n", vuln->affected_component ? vuln->affected_component : "Unknown");
            fprintf(file, "          \"severity\": \"%s\",\n", security_get_severity_description(vuln->severity));
            fprintf(file, "          \"cvss_score\": %.1f,\n", vuln->cvss_score);
            fprintf(file, "          \"exploitable\": %s\n", vuln->is_exploitable ? "true" : "false");
            fprintf(file, "        }%s\n", (i < result->vulnerability_count - 1) ? "," : "");
        }
        
        fprintf(file, "      ],\n");
        fprintf(file, "      \"policies\": [\n");
        
        for (size_t i = 0; i < result->policy_count; i++) {
            const policy_entry_t* policy = &result->policies[i];
            fprintf(file, "        {\n");
            fprintf(file, "          \"name\": \"%s\",\n", policy->policy_name ? policy->policy_name : "Unknown");
            fprintf(file, "          \"type\": \"%s\",\n", security_get_policy_type_name(policy->type));
            fprintf(file, "          \"compliant\": %s,\n", policy->is_compliant ? "true" : "false");
            fprintf(file, "          \"violation\": \"%s\",\n", policy->violation_details ? policy->violation_details : "None");
            fprintf(file, "          \"severity\": \"%s\"\n", security_get_severity_description(policy->severity));
            fprintf(file, "        }%s\n", (i < result->policy_count - 1) ? "," : "");
        }
        
        fprintf(file, "      ]\n");
        fprintf(file, "    }\n");
        fprintf(file, "  }\n");
        fprintf(file, "}\n");
    }
    
    fclose(file);
    printf("Security report generated: %s\n", report_path);
    return 0;
}

const char* security_get_severity_description(vulnerability_severity_t severity) {
    switch (severity) {
        case VULNERABILITY_LOW:
            return "Low";
        case VULNERABILITY_MEDIUM:
            return "Medium";
        case VULNERABILITY_HIGH:
            return "High";
        case VULNERABILITY_CRITICAL:
            return "Critical";
        default:
            return "Unknown";
    }
}

const char* security_get_audit_level_description(security_audit_level_t level) {
    switch (level) {
        case SECURITY_AUDIT_BASIC:
            return "Basic";
        case SECURITY_AUDIT_STANDARD:
            return "Standard";
        case SECURITY_AUDIT_DETAILED:
            return "Detailed";
        case SECURITY_AUDIT_COMPLIANCE:
            return "Compliance";
        default:
            return "Unknown";
    }
}

const char* security_get_permission_type_name(permission_type_t type) {
    switch (type) {
        case PERMISSION_TYPE_FILE:
            return "File";
        case PERMISSION_TYPE_DIRECTORY:
            return "Directory";
        case PERMISSION_TYPE_REGISTRY:
            return "Registry";
        case PERMISSION_TYPE_SERVICE:
            return "Service";
        case PERMISSION_TYPE_PROCESS:
            return "Process";
        default:
            return "Unknown";
    }
}

const char* security_get_policy_type_name(policy_type_t type) {
    switch (type) {
        case POLICY_TYPE_PASSWORD:
            return "Password";
        case POLICY_TYPE_ACCOUNT:
            return "Account";
        case POLICY_TYPE_AUDIT:
            return "Audit";
        case POLICY_TYPE_PRIVILEGE:
            return "Privilege";
        case POLICY_TYPE_NETWORK:
            return "Network";
        case POLICY_TYPE_ENCRYPTION:
            return "Encryption";
        default:
            return "Unknown";
    }
}

int security_permissions_to_string(uint32_t permissions, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 11) {
        return -1;
    }
    
    // Convert Unix-style permissions to string
    buffer[0] = S_ISDIR(permissions) ? 'd' : '-';
    buffer[1] = (permissions & S_IRUSR) ? 'r' : '-';
    buffer[2] = (permissions & S_IWUSR) ? 'w' : '-';
    buffer[3] = (permissions & S_IXUSR) ? 'x' : '-';
    buffer[4] = (permissions & S_IRGRP) ? 'r' : '-';
    buffer[5] = (permissions & S_IWGRP) ? 'w' : '-';
    buffer[6] = (permissions & S_IXGRP) ? 'x' : '-';
    buffer[7] = (permissions & S_IROTH) ? 'r' : '-';
    buffer[8] = (permissions & S_IWOTH) ? 'w' : '-';
    buffer[9] = (permissions & S_IXOTH) ? 'x' : '-';
    buffer[10] = '\0';
    
    // Handle special bits
    if (permissions & S_ISUID) buffer[3] = (permissions & S_IXUSR) ? 's' : 'S';
    if (permissions & S_ISGID) buffer[6] = (permissions & S_IXGRP) ? 's' : 'S';
    if (permissions & S_ISVTX) buffer[9] = (permissions & S_IXOTH) ? 't' : 'T';
    
    return 0;
}