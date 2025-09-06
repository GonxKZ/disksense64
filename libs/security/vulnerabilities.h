#ifndef LIBS_SECURITY_VULNERABILITIES_H
#define LIBS_SECURITY_VULNERABILITIES_H

#include "security.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Vulnerability database entry
typedef struct {
    char* cve_id;
    char* description;
    char* affected_software;
    char* affected_versions;
    vulnerability_severity_t severity;
    double cvss_base_score;
    double cvss_temporal_score;
    double cvss_environmental_score;
    char* attack_vector;
    char* attack_complexity;
    char* privileges_required;
    char* user_interaction;
    char* scope;
    char* confidentiality_impact;
    char* integrity_impact;
    char* availability_impact;
    char* exploitability;
    char* remediation;
    char* references[20];
    size_t reference_count;
    time_t published_date;
    time_t last_modified_date;
    int is_exploited;
    int is_patch_available;
    char* patch_url;
} vulnerability_database_entry_t;

// Initialize vulnerability database entry
void vulnerability_database_entry_init(vulnerability_database_entry_t* entry);

// Free vulnerability database entry
void vulnerability_database_entry_free(vulnerability_database_entry_t* entry);

// Scan system for known vulnerabilities
int security_scan_system_vulnerabilities(const security_options_t* options,
                                         vulnerability_entry_t** vulnerabilities,
                                         size_t* count);

// Scan application for known vulnerabilities
int security_scan_application_vulnerabilities(const char* app_path,
                                             const security_options_t* options,
                                             vulnerability_entry_t** vulnerabilities,
                                             size_t* count);

// Scan network services for known vulnerabilities
int security_scan_network_vulnerabilities(const security_options_t* options,
                                          vulnerability_entry_t** vulnerabilities,
                                          size_t* count);

// Scan for configuration vulnerabilities
int security_scan_configuration_vulnerabilities(const security_options_t* options,
                                               vulnerability_entry_t** vulnerabilities,
                                               size_t* count);

// Scan for cryptographic vulnerabilities
int security_scan_crypto_vulnerabilities(const security_options_t* options,
                                        vulnerability_entry_t** vulnerabilities,
                                        size_t* count);

// Load vulnerability database
int security_load_vulnerability_database(const char* db_path,
                                       vulnerability_database_entry_t** database,
                                       size_t* count);

// Free vulnerability database
void security_free_vulnerability_database(vulnerability_database_entry_t* database,
                                         size_t count);

// Match vulnerability to system component
int security_match_vulnerability(const vulnerability_database_entry_t* vuln_db_entry,
                                const char* component_name,
                                const char* component_version,
                                int* is_vulnerable,
                                double* match_confidence);

// Calculate CVSS score
int security_calculate_cvss_score(const vulnerability_database_entry_t* vuln,
                                 double* base_score,
                                 double* temporal_score,
                                 double* environmental_score);

// Check exploit availability
int security_check_exploit_availability(const vulnerability_database_entry_t* vuln,
                                       int* is_exploitable,
                                       char** exploit_details);

// Check patch availability
int security_check_patch_availability(const vulnerability_database_entry_t* vuln,
                                      int* is_patch_available,
                                      char** patch_details);

// Generate vulnerability report
int security_generate_vulnerability_report(const vulnerability_entry_t* vulnerabilities,
                                          size_t count,
                                          const char* report_path,
                                          const char* format);

// Prioritize vulnerabilities
int security_prioritize_vulnerabilities(vulnerability_entry_t* vulnerabilities,
                                       size_t count);

// Update vulnerability database
int security_update_vulnerability_database(const char* db_url,
                                          const char* local_db_path);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SECURITY_VULNERABILITIES_H