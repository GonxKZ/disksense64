#ifndef LIBS_SECURITY_PERMISSIONS_H
#define LIBS_SECURITY_PERMISSIONS_H

#include "security.h"

#ifdef __cplusplus
extern "C" {
#endif

// Analyze file permissions for security issues
int security_analyze_file_permissions(const char* file_path, const security_options_t* options,
                                      permission_entry_t* permission);

// Analyze directory permissions for security issues
int security_analyze_directory_permissions(const char* dir_path, const security_options_t* options,
                                         permission_entry_t* permission);

// Check for insecure permission patterns
int security_check_insecure_permission_patterns(const permission_entry_t* permission, 
                                               int* is_insecure, char** issue_description,
                                               vulnerability_severity_t* severity);

// Check for privilege escalation opportunities
int security_check_privilege_escalation(const permission_entry_t* permission,
                                       int* is_escalation_possible, char** escalation_details);

// Check for information disclosure risks
int security_check_information_disclosure(const permission_entry_t* permission,
                                         int* is_disclosure_risk, char** disclosure_details);

// Check for unauthorized access risks
int security_check_unauthorized_access(const permission_entry_t* permission,
                                      int* is_access_risk, char** access_details);

// Validate permission inheritance
int security_validate_permission_inheritance(const permission_entry_t* parent_permission,
                                            const permission_entry_t* child_permission,
                                            int* is_valid, char** validation_details);

// Check for permission bypass mechanisms
int security_check_permission_bypass(const permission_entry_t* permission,
                                   int* is_bypass_possible, char** bypass_details);

// Analyze ACL (Access Control List) permissions
int security_analyze_acl_permissions(const permission_entry_t* permission,
                                   int* has_insecure_acl, char** acl_issues);

// Check for group permission issues
int security_check_group_permissions(const permission_entry_t* permission,
                                   int* has_group_issues, char** group_issues);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SECURITY_PERMISSIONS_H