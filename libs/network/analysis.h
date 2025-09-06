#ifndef LIBS_NETWORK_ANALYSIS_H
#define LIBS_NETWORK_ANALYSIS_H

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

// Analyze network traffic for threats
int network_analyze_threats(const network_analysis_result_t* result, network_analysis_result_t* analyzed_result);

// Detect DDoS attacks
int network_detect_ddos(const network_analysis_result_t* result, int* is_ddos);

// Detect port scanning
int network_detect_port_scanning(const network_analysis_result_t* result, int* is_scanning);

// Detect data exfiltration
int network_detect_data_exfiltration(const network_analysis_result_t* result, int* is_exfiltration);

// Detect malware C&C communication
int network_detect_cnc(const network_analysis_result_t* result, int* is_cnc);

#ifdef __cplusplus
}
#endif

#endif // LIBS_NETWORK_ANALYSIS_H