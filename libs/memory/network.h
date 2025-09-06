#ifndef LIBS_MEMORY_NETWORK_H
#define LIBS_MEMORY_NETWORK_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extract network connections from memory dump
int memory_extract_connections_internal(const char* dump_path, const memory_options_t* options,
                                       memory_connection_t** connections, size_t* count);

// Analyze connection for suspicious behavior
int memory_analyze_connection_behavior(const memory_connection_t* connection, int* is_suspicious, char** threat_type, double* confidence);

// Check connection against known malicious IP addresses
int memory_check_connection_malicious_ips(const memory_connection_t* connection, int* is_muspicious, char** reason);

// Check connection against known malicious ports
int memory_check_connection_malicious_ports(const memory_connection_t* connection, int* is_malicious, char** reason);

// Resolve IP address to hostname
int memory_resolve_ip_address(uint32_t ip, char** hostname);

#ifdef __cplusplus
}
#endif

#endif // LIBS_MEMORY_NETWORK_H