#ifndef LIBS_NETWORK_NETWORK_H
#define LIBS_NETWORK_NETWORK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Network protocol types
typedef enum {
    NETWORK_PROTOCOL_UNKNOWN = 0,
    NETWORK_PROTOCOL_ETHERNET = 1,
    NETWORK_PROTOCOL_IPV4 = 2,
    NETWORK_PROTOCOL_IPV6 = 3,
    NETWORK_PROTOCOL_TCP = 4,
    NETWORK_PROTOCOL_UDP = 5,
    NETWORK_PROTOCOL_ICMP = 6,
    NETWORK_PROTOCOL_HTTP = 7,
    NETWORK_PROTOCOL_HTTPS = 8,
    NETWORK_PROTOCOL_DNS = 9,
    NETWORK_PROTOCOL_FTP = 10,
    NETWORK_PROTOCOL_SSH = 11,
    NETWORK_PROTOCOL_TELNET = 12,
    NETWORK_PROTOCOL_SMTP = 13,
    NETWORK_PROTOCOL_POP3 = 14,
    NETWORK_PROTOCOL_IMAP = 15
} network_protocol_t;

// Network packet structure
typedef struct {
    uint64_t timestamp;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    network_protocol_t protocol;
    size_t packet_size;
    uint8_t* payload;
    size_t payload_size;
    int is_suspicious;
    char* threat_type;
    double confidence;
} network_packet_t;

// Network connection structure
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    network_protocol_t protocol;
    uint64_t start_time;
    uint64_t end_time;
    size_t bytes_sent;
    size_t bytes_received;
    size_t packet_count;
    int is_suspicious;
    char* threat_type;
    double confidence;
} network_connection_t;

// Network capture options
typedef struct {
    int capture_live;          // Capture live traffic
    char* interface_name;      // Network interface to capture
    char* capture_file;        // File to read captured data from
    int promiscuous_mode;      // Promiscuous mode for live capture
    size_t buffer_size;        // Capture buffer size
    int timeout_ms;            // Capture timeout in milliseconds
    char* filter_expression;   // BPF filter expression
    int detect_threats;        // Detect network threats
    int analyze_protocols;     // Analyze protocols
    int extract_payloads;      // Extract packet payloads
} network_capture_options_t;

// Network analysis result
typedef struct {
    network_packet_t* packets;
    size_t packet_count;
    size_t packet_capacity;
    network_connection_t* connections;
    size_t connection_count;
    size_t connection_capacity;
    size_t total_packets;
    size_t total_bytes;
    size_t suspicious_packets;
    size_t suspicious_connections;
} network_analysis_result_t;

// Network statistics
typedef struct {
    size_t tcp_packets;
    size_t udp_packets;
    size_t icmp_packets;
    size_t ipv4_packets;
    size_t ipv6_packets;
    size_t http_requests;
    size_t https_requests;
    size_t dns_queries;
    size_t ftp_commands;
    size_t ssh_connections;
    size_t suspicious_activities;
    double average_packet_size;
    double packets_per_second;
} network_statistics_t;

// Initialize network capture options with default values
void network_capture_options_init(network_capture_options_t* options);

// Free network capture options
void network_capture_options_free(network_capture_options_t* options);

// Initialize network analysis result
void network_analysis_result_init(network_analysis_result_t* result);

// Add a packet to the analysis result
int network_analysis_result_add_packet(network_analysis_result_t* result, const network_packet_t* packet);

// Add a connection to the analysis result
int network_analysis_result_add_connection(network_analysis_result_t* result, const network_connection_t* connection);

// Free network analysis result
void network_analysis_result_free(network_analysis_result_t* result);

// Initialize network packet
void network_packet_init(network_packet_t* packet);

// Free network packet
void network_packet_free(network_packet_t* packet);

// Initialize network connection
void network_connection_init(network_connection_t* connection);

// Free network connection
void network_connection_free(network_connection_t* connection);

// Initialize network statistics
void network_statistics_init(network_statistics_t* stats);

// Free network statistics
void network_statistics_free(network_statistics_t* stats);

// Capture network traffic
// options: capture options
// result: output analysis result (must be freed with network_analysis_result_free)
// Returns 0 on success, non-zero on error
int network_capture_traffic(const network_capture_options_t* options, network_analysis_result_t* result);

// Analyze network capture file
// file_path: path to the capture file (PCAP format)
// options: analysis options
// result: output analysis result (must be freed with network_analysis_result_free)
// Returns 0 on success, non-zero on error
int network_analyze_capture_file(const char* file_path, 
                               const network_capture_options_t* options,
                               network_analysis_result_t* result);

// Analyze network packet
// packet: packet to analyze
// Returns 0 on success, non-zero on error
int network_analyze_packet(network_packet_t* packet);

// Analyze network connection
// connection: connection to analyze
// Returns 0 on success, non-zero on error
int network_analyze_connection(network_connection_t* connection);

// Get network statistics
// result: analysis result to calculate statistics for
// stats: output statistics (must be freed with network_statistics_free)
// Returns 0 on success, non-zero on error
int network_get_statistics(const network_analysis_result_t* result, network_statistics_t* stats);

// Detect suspicious network activity
// result: analysis result to scan for suspicious activity
// suspicious_count: output number of suspicious activities found
// Returns 0 on success, non-zero on error
int network_detect_suspicious_activity(const network_analysis_result_t* result, size_t* suspicious_count);

// Export network analysis results
// result: analysis result to export
// file_path: output file path
// format: export format (CSV, JSON, etc.)
// Returns 0 on success, non-zero on error
int network_export_results(const network_analysis_result_t* result, const char* file_path, const char* format);

// Get protocol name
// protocol: protocol type
// Returns name of the protocol
const char* network_get_protocol_name(network_protocol_t protocol);

// Get protocol description
// protocol: protocol type
// Returns description of the protocol
const char* network_get_protocol_description(network_protocol_t protocol);

// Convert IP address to string
// ip: IP address in network byte order
// buffer: output buffer for string representation
// buffer_size: size of output buffer
// Returns 0 on success, non-zero on error
int network_ip_to_string(uint32_t ip, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // LIBS_NETWORK_NETWORK_H