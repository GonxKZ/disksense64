#ifndef LIBS_SECURITY_SECURITY_H
#define LIBS_SECURITY_SECURITY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Security audit levels
typedef enum {
    SECURITY_AUDIT_BASIC = 0,
    SECURITY_AUDIT_STANDARD = 1,
    SECURITY_AUDIT_DETAILED = 2,
    SECURITY_AUDIT_COMPLIANCE = 3
} security_audit_level_t;

// Permission types
typedef enum {
    PERMISSION_TYPE_UNKNOWN = 0,
    PERMISSION_TYPE_FILE = 1,
    PERMISSION_TYPE_DIRECTORY = 2,
    PERMISSION_TYPE_REGISTRY = 3,
    PERMISSION_TYPE_SERVICE = 4,
    PERMISSION_TYPE_PROCESS = 5
} permission_type_t;

// Vulnerability severity levels
typedef enum {
    VULNERABILITY_LOW = 0,
    VULNERABILITY_MEDIUM = 1,
    VULNERABILITY_HIGH = 2,
    VULNERABILITY_CRITICAL = 3
} vulnerability_severity_t;

// Security policy types
typedef enum {
    POLICY_TYPE_PASSWORD = 0,
    POLICY_TYPE_ACCOUNT = 1,
    POLICY_TYPE_AUDIT = 2,
    POLICY_TYPE_PRIVILEGE = 3,
    POLICY_TYPE_NETWORK = 4,
    POLICY_TYPE_ENCRYPTION = 5
} policy_type_t;

// Permission entry
typedef struct {
    char* path;
    permission_type_t type;
    uint32_t owner_id;
    uint32_t group_id;
    uint32_t permissions;
    char* owner_name;
    char* group_name;
    int is_world_writable;
    int is_world_readable;
    int is_suid;
    int is_sgid;
    int is_sticky;
    int has_insecure_permissions;
    char* security_issue;
    vulnerability_severity_t severity;
    double confidence;
} permission_entry_t;

// Vulnerability entry
typedef struct {
    char* cve_id;
    char* description;
    char* affected_component;
    vulnerability_severity_t severity;
    double cvss_score;
    char* remediation;
    int is_exploitable;
    char* references[10];
    size_t reference_count;
} vulnerability_entry_t;

// Security policy entry
typedef struct {
    policy_type_t type;
    char* policy_name;
    char* description;
    int is_compliant;
    char* violation_details;
    char* recommended_action;
    vulnerability_severity_t severity;
} policy_entry_t;

// Security audit result
typedef struct {
    char* target_path;
    security_audit_level_t audit_level;
    permission_entry_t* permissions;
    size_t permission_count;
    size_t permission_capacity;
    vulnerability_entry_t* vulnerabilities;
    size_t vulnerability_count;
    size_t vulnerability_capacity;
    policy_entry_t* policies;
    size_t policy_count;
    size_t policy_capacity;
    size_t insecure_permissions;
    size_t critical_vulnerabilities;
    size_t policy_violations;
    double overall_security_score;
    char* summary_report;
} security_audit_result_t;

// Security audit options
typedef struct {
    security_audit_level_t audit_level;
    int check_file_permissions;
    int check_directory_permissions;
    int check_registry_permissions;
    int scan_for_vulnerabilities;
    int check_security_policies;
    int generate_reports;
    int check_weak_passwords;
    int check_account_lockout;
    int check_audit_settings;
    int check_privilege_escalation;
    int check_network_security;
    int check_encryption;
    char** exclude_paths;
    size_t exclude_count;
    char** include_paths;
    size_t include_count;
} security_options_t;

// Initialize security audit options with default values
void security_options_init(security_options_t* options);

// Free security audit options
void security_options_free(security_options_t* options);

// Initialize security audit result
void security_audit_result_init(security_audit_result_t* result);

// Add permission entry to audit result
int security_audit_result_add_permission(security_audit_result_t* result, const permission_entry_t* permission);

// Add vulnerability entry to audit result
int security_audit_result_add_vulnerability(security_audit_result_t* result, const vulnerability_entry_t* vulnerability);

// Add policy entry to audit result
int security_audit_result_add_policy(security_audit_result_t* result, const policy_entry_t* policy);

// Free security audit result
void security_audit_result_free(security_audit_result_t* result);

// Initialize permission entry
void permission_entry_init(permission_entry_t* entry);

// Free permission entry
void permission_entry_free(permission_entry_t* entry);

// Initialize vulnerability entry
void vulnerability_entry_init(vulnerability_entry_t* entry);

// Free vulnerability entry
void vulnerability_entry_free(vulnerability_entry_t* entry);

// Initialize policy entry
void policy_entry_init(policy_entry_t* entry);

// Free policy entry
void policy_entry_free(policy_entry_t* entry);

// Perform comprehensive security audit
// target_path: path to audit (file, directory, or system)
// options: audit options
// result: output audit result (must be freed with security_audit_result_free)
// Returns 0 on success, non-zero on error
int security_perform_audit(const char* target_path, const security_options_t* options, security_audit_result_t* result);

// Check file and directory permissions
// path: path to check
// options: audit options
// permissions: output array of permission entries (must be freed)
// count: output number of permission entries
// Returns 0 on success, non-zero on error
int security_check_permissions(const char* path, const security_options_t* options, 
                              permission_entry_t** permissions, size_t* count);

// Scan for known vulnerabilities
// target_path: path to scan
// options: audit options
// vulnerabilities: output array of vulnerability entries (must be freed)
// count: output number of vulnerability entries
// Returns 0 on success, non-zero on error
int security_scan_vulnerabilities(const char* target_path, const security_options_t* options,
                                vulnerability_entry_t** vulnerabilities, size_t* count);

// Check security policies compliance
// options: audit options
// policies: output array of policy entries (must be freed)
// count: output number of policy entries
// Returns 0 on success, non-zero on error
int security_check_policies(const security_options_t* options, 
                          policy_entry_t** policies, size_t* count);

// Analyze permission security
// permission: permission entry to analyze
// is_insecure: output flag indicating if permission is insecure
// issue_description: output description of security issue
// severity: output severity level
// Returns 0 on success, non-zero on error
int security_analyze_permission(const permission_entry_t* permission, int* is_insecure, 
                              char** issue_description, vulnerability_severity_t* severity);

// Analyze vulnerability severity
// vulnerability: vulnerability entry to analyze
// severity: output severity level
// cvss_score: output CVSS score
// Returns 0 on success, non-zero on error
int security_analyze_vulnerability(const vulnerability_entry_t* vulnerability, 
                                 vulnerability_severity_t* severity, double* cvss_score);

// Check policy compliance
// policy: policy entry to check
// is_compliant: output flag indicating if policy is compliant
// violation_details: output details of policy violations
// Returns 0 on success, non-zero on error
int security_check_policy_compliance(const policy_entry_t* policy, int* is_compliant, 
                                    char** violation_details);

// Generate security audit report
// result: audit result to generate report for
// report_path: output report file path
// format: report format (TEXT, HTML, JSON, CSV)
// Returns 0 on success, non-zero on error
int security_generate_report(const security_audit_result_t* result, const char* report_path, const char* format);

// Get vulnerability severity description
// severity: vulnerability severity level
// Returns description of the severity level
const char* security_get_severity_description(vulnerability_severity_t severity);

// Get audit level description
// level: audit level
// Returns description of the audit level
const char* security_get_audit_level_description(security_audit_level_t level);

// Get permission type name
// type: permission type
// Returns name of the permission type
const char* security_get_permission_type_name(permission_type_t type);

// Get policy type name
// type: policy type
// Returns name of the policy type
const char* security_get_policy_type_name(policy_type_t type);

// Convert permissions to string representation
// permissions: permission bits
// buffer: output buffer for string representation
// buffer_size: size of output buffer
// Returns 0 on success, non-zero on error
int security_permissions_to_string(uint32_t permissions, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SECURITY_SECURITY_H