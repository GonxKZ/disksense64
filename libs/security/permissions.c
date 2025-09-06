#include "permissions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

int security_analyze_file_permissions(const char* file_path, const security_options_t* options,
                                      permission_entry_t* permission) {
    if (!file_path || !permission) {
        return -1;
    }
    
    permission_entry_init(permission);
    
    printf("Analyzing file permissions: %s\n", file_path);
    
    // Get file statistics
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }
    
    // Fill permission entry
    permission->path = strdup(file_path);
    permission->type = PERMISSION_TYPE_FILE;
    permission->owner_id = st.st_uid;
    permission->group_id = st.st_gid;
    permission->permissions = st.st_mode;
    
    // Get owner and group names
    struct passwd* pwd = getpwuid(st.st_uid);
    if (pwd) {
        permission->owner_name = strdup(pwd->pw_name);
    }
    
    struct group* grp = getgrgid(st.st_gid);
    if (grp) {
        permission->group_name = strdup(grp->gr_name);
    }
    
    // Check permission flags
    permission->is_world_writable = (st.st_mode & S_IWOTH) ? 1 : 0;
    permission->is_world_readable = (st.st_mode & S_IROTH) ? 1 : 0;
    permission->is_suid = (st.st_mode & S_ISUID) ? 1 : 0;
    permission->is_sgid = (st.st_mode & S_ISGID) ? 1 : 0;
    permission->is_sticky = (st.st_mode & S_ISVTX) ? 1 : 0;
    
    // Analyze for security issues
    int is_insecure;
    char* issue_description;
    vulnerability_severity_t severity;
    
    if (security_check_insecure_permission_patterns(permission, &is_insecure, &issue_description, &severity) == 0) {
        permission->has_insecure_permissions = is_insecure;
        permission->security_issue = issue_description;
        permission->severity = severity;
        permission->confidence = 0.9;
    }
    
    return 0;
}

int security_analyze_directory_permissions(const char* dir_path, const security_options_t* options,
                                         permission_entry_t* permission) {
    if (!dir_path || !permission) {
        return -1;
    }
    
    permission_entry_init(permission);
    
    printf("Analyzing directory permissions: %s\n", dir_path);
    
    // Get directory statistics
    struct stat st;
    if (stat(dir_path, &st) != 0) {
        return -1;
    }
    
    // Fill permission entry
    permission->path = strdup(dir_path);
    permission->type = PERMISSION_TYPE_DIRECTORY;
    permission->owner_id = st.st_uid;
    permission->group_id = st.st_gid;
    permission->permissions = st.st_mode;
    
    // Get owner and group names
    struct passwd* pwd = getpwuid(st.st_uid);
    if (pwd) {
        permission->owner_name = strdup(pwd->pw_name);
    }
    
    struct group* grp = getgrgid(st.st_gid);
    if (grp) {
        permission->group_name = strdup(grp->gr_name);
    }
    
    // Check permission flags
    permission->is_world_writable = (st.st_mode & S_IWOTH) ? 1 : 0;
    permission->is_world_readable = (st.st_mode & S_IROTH) ? 1 : 0;
    permission->is_suid = (st.st_mode & S_ISUID) ? 1 : 0;
    permission->is_sgid = (st.st_mode & S_ISGID) ? 1 : 0;
    permission->is_sticky = (st.st_mode & S_ISVTX) ? 1 : 0;
    
    // Analyze for security issues
    int is_insecure;
    char* issue_description;
    vulnerability_severity_t severity;
    
    if (security_check_insecure_permission_patterns(permission, &is_insecure, &issue_description, &severity) == 0) {
        permission->has_insecure_permissions = is_insecure;
        permission->security_issue = issue_description;
        permission->severity = severity;
        permission->confidence = 0.9;
    }
    
    return 0;
}

int security_check_insecure_permission_patterns(const permission_entry_t* permission, 
                                               int* is_insecure, char** issue_description,
                                               vulnerability_severity_t* severity) {
    if (!permission || !is_insecure || !issue_description || !severity) {
        return -1;
    }
    
    *is_insecure = 0;
    *issue_description = NULL;
    *severity = VULNERABILITY_LOW;
    
    // Check for world-writable files/directories
    if (permission->is_world_writable) {
        *is_insecure = 1;
        *issue_description = strdup("World-writable permissions pose security risk");
        *severity = VULNERABILITY_HIGH;
        return 0;
    }
    
    // Check for SUID/SGID bits
    if (permission->is_suid || permission->is_sgid) {
        *is_insecure = 1;
        *issue_description = strdup("SUID/SGID bit set, potential privilege escalation risk");
        *severity = VULNERABILITY_CRITICAL;
        return 0;
    }
    
    // Check for sticky bit on files (unusual)
    if (permission->is_sticky && permission->type == PERMISSION_TYPE_FILE) {
        *is_insecure = 1;
        *issue_description = strdup("Sticky bit set on file, unusual configuration");
        *severity = VULNERABILITY_MEDIUM;
        return 0;
    }
    
    // Check for overly permissive group permissions
    if ((permission->permissions & S_IWGRP) && (permission->permissions & S_IRGRP)) {
        // If group has both read and write, check if it's a sensitive file
        if (permission->path) {
            const char* sensitive_files[] = {
                "shadow", "passwd", "sudoers", "ssh", "ssl", "cert"
            };
            
            int num_sensitive = sizeof(sensitive_files) / sizeof(sensitive_files[0]);
            
            for (int i = 0; i < num_sensitive; i++) {
                if (strstr(permission->path, sensitive_files[i]) != NULL) {
                    *is_insecure = 1;
                    *issue_description = strdup("Sensitive file with group read-write permissions");
                    *severity = VULNERABILITY_HIGH;
                    return 0;
                }
            }
        }
    }
    
    // Check for world-readable sensitive files
    if (permission->is_world_readable && permission->path) {
        const char* sensitive_extensions[] = {
            ".key", ".pem", ".priv", ".private", ".secret", ".pass", ".password"
        };
        
        int num_extensions = sizeof(sensitive_extensions) / sizeof(sensitive_extensions[0]);
        
        for (int i = 0; i < num_extensions; i++) {
            if (strstr(permission->path, sensitive_extensions[i]) != NULL) {
                *is_insecure = 1;
                *issue_description = strdup("Sensitive file with world-readable permissions");
                *severity = VULNERABILITY_CRITICAL;
                return 0;
            }
        }
    }
    
    return 0;
}

int security_check_privilege_escalation(const permission_entry_t* permission,
                                       int* is_escalation_possible, char** escalation_details) {
    if (!permission || !is_escalation_possible || !escalation_details) {
        return -1;
    }
    
    *is_escalation_possible = 0;
    *escalation_details = NULL;
    
    // Check for SUID/SGID binaries
    if (permission->is_suid || permission->is_sgid) {
        *is_escalation_possible = 1;
        *escalation_details = strdup("File has SUID/SGID bit set, potential for privilege escalation");
        return 0;
    }
    
    // Check for world-writable directories that contain executables
    if (permission->is_world_writable && permission->type == PERMISSION_TYPE_DIRECTORY) {
        *is_escalation_possible = 1;
        *escalation_details = strdup("World-writable directory, potential for PATH hijacking");
        return 0;
    }
    
    return 0;
}

int security_check_information_disclosure(const permission_entry_t* permission,
                                         int* is_disclosure_risk, char** disclosure_details) {
    if (!permission || !is_disclosure_risk || !disclosure_details) {
        return -1;
    }
    
    *is_disclosure_risk = 0;
    *disclosure_details = NULL;
    
    // Check for world-readable sensitive files
    if (permission->is_world_readable && permission->path) {
        const char* sensitive_patterns[] = {
            "config", "conf", "settings", "database", "db", "credentials", "creds",
            "secret", "key", "cert", "pem", "log", "debug"
        };
        
        int num_patterns = sizeof(sensitive_patterns) / sizeof(sensitive_patterns[0]);
        
        for (int i = 0; i < num_patterns; i++) {
            if (strstr(permission->path, sensitive_patterns[i]) != NULL) {
                *is_disclosure_risk = 1;
                *disclosure_details = strdup("Potentially sensitive file with world-readable permissions");
                return 0;
            }
        }
    }
    
    return 0;
}

int security_check_unauthorized_access(const permission_entry_t* permission,
                                      int* is_access_risk, char** access_details) {
    if (!permission || !is_access_risk || !access_details) {
        return -1;
    }
    
    *is_access_risk = 0;
    *access_details = NULL;
    
    // Check for overly permissive permissions
    if (permission->is_world_writable) {
        *is_access_risk = 1;
        *access_details = strdup("World-writable permissions allow unauthorized modification");
        return 0;
    }
    
    // Check for group permissions on sensitive files
    if ((permission->permissions & S_IWGRP) && permission->path) {
        const char* sensitive_files[] = {
            "shadow", "passwd", "sudoers", "ssh_config", "sshd_config"
        };
        
        int num_sensitive = sizeof(sensitive_files) / sizeof(sensitive_files[0]);
        
        for (int i = 0; i < num_sensitive; i++) {
            if (strstr(permission->path, sensitive_files[i]) != NULL) {
                *is_access_risk = 1;
                *access_details = strdup("Sensitive system file with group write permissions");
                return 0;
            }
        }
    }
    
    return 0;
}

int security_validate_permission_inheritance(const permission_entry_t* parent_permission,
                                            const permission_entry_t* child_permission,
                                            int* is_valid, char** validation_details) {
    if (!parent_permission || !child_permission || !is_valid || !validation_details) {
        return -1;
    }
    
    *is_valid = 1;
    *validation_details = NULL;
    
    // In a real implementation, this would check complex permission inheritance rules
    // For now, we'll do a basic check
    
    // Child should not be more permissive than parent for sensitive directories
    if (parent_permission->type == PERMISSION_TYPE_DIRECTORY && 
        child_permission->type == PERMISSION_TYPE_FILE) {
        // Basic check - child shouldn't be world-writable if parent isn't
        if (child_permission->is_world_writable && !parent_permission->is_world_writable) {
            *is_valid = 0;
            *validation_details = strdup("Child file more permissive than parent directory");
        }
    }
    
    return 0;
}

int security_check_permission_bypass(const permission_entry_t* permission,
                                   int* is_bypass_possible, char** bypass_details) {
    if (!permission || !is_bypass_possible || !bypass_details) {
        return -1;
    }
    
    *is_bypass_possible = 0;
    *bypass_details = NULL;
    
    // Check for hardlinks to sensitive files
    if (permission->type == PERMISSION_TYPE_FILE) {
        // In a real implementation, this would check for hardlink attacks
        // For now, we'll create a placeholder check
        if (permission->is_suid) {
            *is_bypass_possible = 1;
            *bypass_details = strdup("SUID binary may be vulnerable to hardlink race conditions");
        }
    }
    
    return 0;
}

int security_analyze_acl_permissions(const permission_entry_t* permission,
                                   int* has_insecure_acl, char** acl_issues) {
    if (!permission || !has_insecure_acl || !acl_issues) {
        return -1;
    }
    
    *has_insecure_acl = 0;
    *acl_issues = NULL;
    
    // In a real implementation, this would analyze actual ACLs using system calls
    // For now, we'll create a placeholder that assumes no ACL issues
    
    *acl_issues = strdup("No ACL issues detected (ACL analysis not implemented)");
    
    return 0;
}

int security_check_group_permissions(const permission_entry_t* permission,
                                   int* has_group_issues, char** group_issues) {
    if (!permission || !has_group_issues || !group_issues) {
        return -1;
    }
    
    *has_group_issues = 0;
    *group_issues = NULL;
    
    // Check for group permissions on sensitive files
    if (permission->path) {
        const char* sensitive_groups[] = {
            "root", "wheel", "admin", "sudo"
        };
        
        int num_groups = sizeof(sensitive_groups) / sizeof(sensitive_groups[0]);
        
        for (int i = 0; i < num_groups; i++) {
            if (permission->group_name && strcmp(permission->group_name, sensitive_groups[i]) == 0) {
                // If sensitive group has write permissions
                if (permission->permissions & S_IWGRP) {
                    *has_group_issues = 1;
                    *group_issues = strdup("File owned by privileged group with write permissions");
                    return 0;
                }
            }
        }
    }
    
    return 0;
}