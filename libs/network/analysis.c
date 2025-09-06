#include "analysis.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int network_analyze_threats(const network_analysis_result_t* result, network_analysis_result_t* analyzed_result) {
    if (!result || !analyzed_result) {
        return -1;
    }

    // Copy the result to have a base for analysis
    // In a real implementation, you might want to avoid this deep copy for performance reasons
    // and just modify the input result. For clarity, we'll copy.
    // NOTE: This is a shallow copy. The pointers to packets and connections are copied.
    // A deep copy would be needed if we were to modify the packets/connections themselves.
    *analyzed_result = *result;

    int is_ddos = 0;
    network_detect_ddos(result, &is_ddos);

    int is_scanning = 0;
    network_detect_port_scanning(result, &is_scanning);

    int is_exfiltration = 0;
    network_detect_data_exfiltration(result, &is_exfiltration);

    int is_cnc = 0;
    network_detect_cnc(result, &is_cnc);

    if (is_ddos) {
        for (size_t i = 0; i < analyzed_result->packet_count; ++i) {
            analyzed_result->packets[i].is_suspicious = 1;
            analyzed_result->packets[i].threat_type = strdup("DDoS");
        }
        analyzed_result->suspicious_packets = analyzed_result->packet_count;
    }

    if (is_scanning) {
        // This is a simplified marking. A real implementation would mark the specific packets/connections.
        for (size_t i = 0; i < analyzed_result->connection_count; ++i) {
            analyzed_result->connections[i].is_suspicious = 1;
            analyzed_result->connections[i].threat_type = strdup("Port Scanning");
        }
        analyzed_result->suspicious_connections = analyzed_result->connection_count;
    }

    if (is_exfiltration) {
        for (size_t i = 0; i < analyzed_result->connection_count; ++i) {
            if (analyzed_result->connections[i].bytes_sent > 1000000 && analyzed_result->connections[i].bytes_received < 10000) {
                analyzed_result->connections[i].is_suspicious = 1;
                analyzed_result->connections[i].threat_type = strdup("Data Exfiltration");
            }
        }
    }

    if (is_cnc) {
        for (size_t i = 0; i < analyzed_result->connection_count; ++i) {
            if (analyzed_result->connections[i].dst_port == 4444 || analyzed_result->connections[i].dst_port == 1337 || analyzed_result->connections[i].dst_port == 6667) {
                analyzed_result->connections[i].is_suspicious = 1;
                analyzed_result->connections[i].threat_type = strdup("C&C");
            }
        }
    }

    return 0;
}

int network_detect_ddos(const network_analysis_result_t* result, int* is_ddos) {
    if (!result || !is_ddos) {
        return -1;
    }

    *is_ddos = 0;

    // A simple DDoS detection based on packet rate to a specific destination
    // This is a simplified approach. A real implementation would be more complex.
    if (result->packet_count < 1000) {
        return 0; // Not enough packets for a meaningful analysis
    }

    // Use a simple hash map to store packet counts for each destination IP
    // For simplicity, we'll use an array as a hash map.
    #define MAX_IP_ENTRIES 1024
    struct { uint32_t ip; int count; } ip_counts[MAX_IP_ENTRIES] = {0};

    for (size_t i = 0; i < result->packet_count; ++i) {
        uint32_t dst_ip = result->packets[i].dst_ip;
        int index = dst_ip % MAX_IP_ENTRIES;
        
        int found = 0;
        for (int j = 0; j < MAX_IP_ENTRIES; ++j) {
            int current_index = (index + j) % MAX_IP_ENTRIES;
            if (ip_counts[current_index].ip == dst_ip) {
                ip_counts[current_index].count++;
                found = 1;
                break;
            }
            if (ip_counts[current_index].ip == 0) {
                ip_counts[current_index].ip = dst_ip;
                ip_counts[current_index].count = 1;
                found = 1;
                break;
            }
        }
    }

    // Check if any IP has received a very large number of packets
    for (int i = 0; i < MAX_IP_ENTRIES; ++i) {
        if (ip_counts[i].count > (result->packet_count * 0.8)) {
            *is_ddos = 1;
            return 0;
        }
    }

    return 0;
}

int network_detect_port_scanning(const network_analysis_result_t* result, int* is_scanning) {
    if (!result || !is_scanning) {
        return -1;
    }

    *is_scanning = 0;

    if (result->packet_count < 50) {
        return 0; // Not enough packets
    }

    #define MAX_SCAN_ENTRIES 1024
    struct { 
        uint32_t src_ip;
        uint32_t dst_ip;
        uint16_t* ports;
        int port_count;
        int port_capacity;
    } scan_counts[MAX_SCAN_ENTRIES] = {0};

    for (size_t i = 0; i < result->packet_count; ++i) {
        uint32_t src_ip = result->packets[i].src_ip;
        uint32_t dst_ip = result->packets[i].dst_ip;
        uint16_t dst_port = result->packets[i].dst_port;

        int index = (src_ip ^ dst_ip) % MAX_SCAN_ENTRIES;
        
        int found = 0;
        for (int j = 0; j < MAX_SCAN_ENTRIES; ++j) {
            int current_index = (index + j) % MAX_SCAN_ENTRIES;
            if (scan_counts[current_index].src_ip == src_ip && scan_counts[current_index].dst_ip == dst_ip) {
                // Check if port is already in the list
                int port_found = 0;
                for (int k = 0; k < scan_counts[current_index].port_count; ++k) {
                    if (scan_counts[current_index].ports[k] == dst_port) {
                        port_found = 1;
                        break;
                    }
                }
                if (!port_found) {
                    if (scan_counts[current_index].port_count >= scan_counts[current_index].port_capacity) {
                        int new_capacity = (scan_counts[current_index].port_capacity == 0) ? 16 : scan_counts[current_index].port_capacity * 2;
                        uint16_t* new_ports = (uint16_t*)realloc(scan_counts[current_index].ports, new_capacity * sizeof(uint16_t));
                        if (new_ports) {
                            scan_counts[current_index].ports = new_ports;
                            scan_counts[current_index].port_capacity = new_capacity;
                        }
                    }
                    if (scan_counts[current_index].port_count < scan_counts[current_index].port_capacity) {
                        scan_counts[current_index].ports[scan_counts[current_index].port_count++] = dst_port;
                    }
                }
                found = 1;
                break;
            }
            if (scan_counts[current_index].src_ip == 0) {
                scan_counts[current_index].src_ip = src_ip;
                scan_counts[current_index].dst_ip = dst_ip;
                found = 1;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_SCAN_ENTRIES; ++i) {
        if (scan_counts[i].port_count > 50) { // Threshold
            *is_scanning = 1;
            break;
        }
        if (scan_counts[i].ports) {
            free(scan_counts[i].ports);
        }
    }

    return 0;
}

int network_detect_data_exfiltration(const network_analysis_result_t* result, int* is_exfiltration) {
    if (!result || !is_exfiltration) {
        return -1;
    }

    *is_exfiltration = 0;

    for (size_t i = 0; i < result->connection_count; i++) {
        const network_connection_t* conn = &result->connections[i];

        // Large upload with little download
        if (conn->bytes_sent > 1000000 && conn->bytes_received < 10000) {
            // Check for suspicious protocols for large data transfers
            if (conn->protocol == NETWORK_PROTOCOL_DNS || 
                conn->protocol == NETWORK_PROTOCOL_ICMP) {
                *is_exfiltration = 1;
                break;
            }
        }
    }

    return 0;
}

int network_detect_cnc(const network_analysis_result_t* result, int* is_cnc) {
    if (!result || !is_cnc) {
        return -1;
    }

    *is_cnc = 0;

    // Placeholder for a blacklist of known C&C servers
    const char* blacklist[] = {
        "192.168.100.100", // Example malicious IP
        "10.0.0.123",      // Example malicious IP
        NULL
    };

    for (size_t i = 0; i < result->connection_count; i++) {
        const network_connection_t* conn = &result->connections[i];

        // Check against blacklist
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", 
                 (conn->dst_ip >> 24) & 0xFF, 
                 (conn->dst_ip >> 16) & 0xFF, 
                 (conn->dst_ip >> 8) & 0xFF, 
                 conn->dst_ip & 0xFF);

        for (int j = 0; blacklist[j] != NULL; ++j) {
            if (strcmp(ip_str, blacklist[j]) == 0) {
                *is_cnc = 1;
                return 0;
            }
        }

        // Check for connections to known malicious ports or patterns
        if (conn->dst_port == 4444 || conn->dst_port == 1337 || conn->dst_port == 6667) {
            *is_cnc = 1;
            return 0;
        }

        // Check for periodic communication patterns (e.g., beacons)
        if (conn->packet_count > 100 && 
            (conn->bytes_sent + conn->bytes_received) < 10000) { // Many small packets
            *is_cnc = 1;
            return 0;
        }
    }

    return 0;
}