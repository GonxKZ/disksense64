#ifndef LIBS_SECURITY_AUDIT_H
#define LIBS_SECURITY_AUDIT_H

#include "security.h"

#ifdef __cplusplus
extern "C" {
#endif

// Perform comprehensive file system audit
int security_perform_filesystem_audit(const char* target_path, const security_options_t* options, 
                                      security_audit_result_t* result);

// Perform system configuration audit
int security_perform_system_audit(const security_options_t* options, 
                                 security_audit_result_t* result);

// Perform network security audit
int security_perform_network_audit(const security_options_t* options, 
                                  security_audit_result_t* result);

// Perform application security audit
int security_perform_application_audit(const char* app_path, const security_options_t* options, 
                                       security_audit_result_t* result);

// Check system hardening
int security_check_system_hardening(const security_options_t* options, 
                                   security_audit_result_t* result);

// Check for common misconfigurations
int security_check_common_misconfigurations(const security_options_t* options, 
                                          security_audit_result_t* result);

// Check for known insecure services
int security_check_insecure_services(const security_options_t* options, 
                                   security_audit_result_t* result);

// Check for weak cryptographic configurations
int security_check_weak_crypto(const security_options_t* options, 
                              security_audit_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_SECURITY_AUDIT_H