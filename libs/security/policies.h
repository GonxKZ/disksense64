#ifndef LIBS_SECURITY_POLICIES_H
#define LIBS_SECURITY_POLICIES_H

#include "security.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Security policy templates
typedef struct {
    policy_type_t type;
    char* template_name;
    char* description;
    char* default_setting;
    char* recommended_setting;
    char* enforcement_level;
    char* compliance_standard;
    vulnerability_severity_t severity_if_violated;
} policy_template_t;

// Policy compliance check result
typedef struct {
    policy_entry_t* policy;
    int is_compliant;
    char* violation_details;
    char* recommended_fix;
    double compliance_score;
    time_t check_timestamp;
} policy_compliance_result_t;

// Security baseline
typedef struct {
    char* baseline_name;
    char* description;
    char* version;
    policy_template_t* policies;
    size_t policy_count;
    time_t created_date;
    char* author;
} security_baseline_t;

// Initialize policy template
void policy_template_init(policy_template_t* template_entry);

// Free policy template
void policy_template_free(policy_template_t* template_entry);

// Initialize policy compliance result
void policy_compliance_result_init(policy_compliance_result_t* result);

// Free policy compliance result
void policy_compliance_result_free(policy_compliance_result_t* result);

// Initialize security baseline
void security_baseline_init(security_baseline_t* baseline);

// Free security baseline
void security_baseline_free(security_baseline_t* baseline);

// Load security policies from file
int security_load_policies(const char* policy_file,
                          policy_entry_t** policies,
                          size_t* count);

// Save security policies to file
int security_save_policies(const policy_entry_t* policies,
                          size_t count,
                          const char* policy_file);

// Validate policy syntax
int security_validate_policy_syntax(const policy_entry_t* policy,
                                  int* is_valid,
                                  char** validation_error);

// Apply security policies
int security_apply_policies(const policy_entry_t* policies,
                           size_t count,
                           int* applied_count,
                           int* failed_count);

// Revert security policies
int security_revert_policies(const policy_entry_t* policies,
                           size_t count,
                           int* reverted_count,
                           int* failed_count);

// Check policy compliance
int security_check_policy_compliance_internal(const policy_entry_t* policy,
                                             policy_compliance_result_t* result);

// Generate compliance report
int security_generate_compliance_report(const policy_compliance_result_t* results,
                                      size_t count,
                                      const char* report_path,
                                      const char* format);

// Load policy templates
int security_load_policy_templates(const char* template_file,
                                 policy_template_t** templates,
                                 size_t* count);

// Create policy from template
int security_create_policy_from_template(const policy_template_t* template_entry,
                                        const char* custom_settings,
                                        policy_entry_t* policy);

// Load security baseline
int security_load_security_baseline(const char* baseline_file,
                                  security_baseline_t* baseline);

// Compare against baseline
int security_compare_against_baseline(const security_baseline_t* baseline,
                                    const policy_entry_t* current_policies,
                                    size_t current_count,
                                    policy_compliance_result_t** results,
                                    size_t* result_count);

// Generate remediation plan
int security_generate_remediation_plan(const policy_compliance_result_t* results,
                                     size_t count,
                                     const char* plan_file);

// Check password policy compliance
int security_check_password_policy(const security_options_t* options,
                                  policy_compliance_result_t* result);

// Check account lockout policy compliance
int security_check_account_lockout_policy(const security_options_t* options,
                                        policy_compliance_result_t* result);

// Check audit policy compliance
int security_check_audit_policy(const security_options_t* options,
                              policy_compliance_result_t* result);

// Check privilege policy compliance
int security_check_privilege_policy(const security_options_t* options,
                                  policy_compliance_result_t* result);

// Check network policy compliance
int security_check_network_policy(const security_options_t* options,
                                 policy_compliance_result_t* result);

// Check encryption policy compliance
int security_check_encryption_policy(const security_options_t* options,
                                    policy_compliance_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SECURITY_POLICIES_H