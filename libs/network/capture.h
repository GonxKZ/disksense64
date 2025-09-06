#ifndef LIBS_NETWORK_CAPTURE_H
#define LIBS_NETWORK_CAPTURE_H

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

// Capture network traffic (internal implementation)
int network_capture_internal(const network_capture_options_t* options, network_analysis_result_t* result);

// Read PCAP file (internal implementation)
int network_read_pcap_internal(const char* file_path, network_analysis_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // LIBS_NETWORK_CAPTURE_H