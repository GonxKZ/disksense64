#include "network.h"
#include "capture.h"
#include "protocols.h"
#include "analysis.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// Helper function to duplicate string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void network_capture_options_init(network_capture_options_t* options) {
    if (options) {
        memset(options, 0, sizeof(network_capture_options_t));
        options->capture_live = 0;
        options->promiscuous_mode = 1;
        options->buffer_size = 1024 * 1024; // 1MB
        options->timeout_ms = 1000;
        options->detect_threats = 1;
        options->analyze_protocols = 1;
        options->extract_payloads = 1;
    }
}

void network_capture_options_free(network_capture_options_t* options) {
    if (options) {
        free(options->interface_name);
        free(options->capture_file);
        free(options->filter_expression);
        memset(options, 0, sizeof(network_capture_options_t));
    }
}

void network_analysis_result_init(network_analysis_result_t* result) {
    if (result) {
        memset(result, 0, sizeof(network_analysis_result_t));
    }
}

int network_analysis_result_add_packet(network_analysis_result_t* result, const network_packet_t* packet) {
    if (!result || !packet) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->packet_count >= result->packet_capacity) {
        size_t new_capacity = (result->packet_capacity == 0) ? 16 : result->packet_capacity * 2;
        network_packet_t* new_packets = (network_packet_t*)realloc(result->packets, new_capacity * sizeof(network_packet_t));
        if (!new_packets) {
            return -1;
        }
        result->packets = new_packets;
        result->packet_capacity = new_capacity;
    }
    
    // Copy packet data
    network_packet_t* new_packet = &result->packets[result->packet_count];
    network_packet_init(new_packet);
    new_packet->timestamp = packet->timestamp;
    new_packet->src_ip = packet->src_ip;
    new_packet->dst_ip = packet->dst_ip;
    new_packet->src_port = packet->src_port;
    new_packet->dst_port = packet->dst_port;
    new_packet->protocol = packet->protocol;
    new_packet->packet_size = packet->packet_size;
    new_packet->payload_size = packet->payload_size;
    new_packet->is_suspicious = packet->is_suspicious;
    new_packet->threat_type = strdup_safe(packet->threat_type);
    new_packet->confidence = packet->confidence;
    
    if (packet->payload_size > 0 && packet->payload) {
        new_packet->payload = (uint8_t*)malloc(packet->payload_size);
        if (new_packet->payload) {
            memcpy(new_packet->payload, packet->payload, packet->payload_size);
        }
    }
    
    result->packet_count++;
    result->total_packets++;
    result->total_bytes += packet->packet_size;
    
    if (packet->is_suspicious) {
        result->suspicious_packets++;
    }
    
    return 0;
}

int network_analysis_result_add_connection(network_analysis_result_t* result, const network_connection_t* connection) {
    if (!result || !connection) {
        return -1;
    }
    
    // Reallocate if needed
    if (result->connection_count >= result->connection_capacity) {
        size_t new_capacity = (result->connection_capacity == 0) ? 16 : result->connection_capacity * 2;
        network_connection_t* new_connections = (network_connection_t*)realloc(result->connections, new_capacity * sizeof(network_connection_t));
        if (!new_connections) {
            return -1;
        }
        result->connections = new_connections;
        result->connection_capacity = new_capacity;
    }
    
    // Copy connection data
    network_connection_t* new_connection = &result->connections[result->connection_count];
    network_connection_init(new_connection);
    new_connection->src_ip = connection->src_ip;
    new_connection->dst_ip = connection->dst_ip;
    new_connection->src_port = connection->src_port;
    new_connection->dst_port = connection->dst_port;
    new_connection->protocol = connection->protocol;
    new_connection->start_time = connection->start_time;
    new_connection->end_time = connection->end_time;
    new_connection->bytes_sent = connection->bytes_sent;
    new_connection->bytes_received = connection->bytes_received;
    new_connection->packet_count = connection->packet_count;
    new_connection->is_suspicious = connection->is_suspicious;
    new_connection->threat_type = strdup_safe(connection->threat_type);
    new_connection->confidence = connection->confidence;
    
    result->connection_count++;
    
    if (connection->is_suspicious) {
        result->suspicious_connections++;
    }
    
    return 0;
}

void network_analysis_result_free(network_analysis_result_t* result) {
    if (result) {
        if (result->packets) {
            for (size_t i = 0; i < result->packet_count; i++) {
                network_packet_free(&result->packets[i]);
            }
            free(result->packets);
        }
        
        if (result->connections) {
            for (size_t i = 0; i < result->connection_count; i++) {
                network_connection_free(&result->connections[i]);
            }
            free(result->connections);
        }
        
        memset(result, 0, sizeof(network_analysis_result_t));
    }
}

void network_packet_init(network_packet_t* packet) {
    if (packet) {
        memset(packet, 0, sizeof(network_packet_t));
    }
}

void network_packet_free(network_packet_t* packet) {
    if (packet) {
        free(packet->payload);
        free(packet->threat_type);
        memset(packet, 0, sizeof(network_packet_t));
    }
}

void network_connection_init(network_connection_t* connection) {
    if (connection) {
        memset(connection, 0, sizeof(network_connection_t));
    }
}

void network_connection_free(network_connection_t* connection) {
    if (connection) {
        free(connection->threat_type);
        memset(connection, 0, sizeof(network_connection_t));
    }
}

void network_statistics_init(network_statistics_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(network_statistics_t));
    }
}

void network_statistics_free(network_statistics_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(network_statistics_t));
    }
}

int network_capture_traffic(const network_capture_options_t* options, network_analysis_result_t* result) {
    if (!options || !result) {
        return -1;
    }
    
    network_analysis_result_init(result);
    
    // In a real implementation, this would capture network traffic
    // For now, we'll simulate capturing some traffic
    
    printf("Capturing network traffic...\n");
    
    if (options->capture_live) {
        printf("Live capture on interface: %s\n", 
               options->interface_name ? options->interface_name : "default");
        if (options->promiscuous_mode) {
            printf("Promiscuous mode enabled\n");
        }
    } else {
        printf("Reading from capture file: %s\n", 
               options->capture_file ? options->capture_file : "none");
    }
    
    if (options->filter_expression) {
        printf("Filter expression: %s\n", options->filter_expression);
    }
    
    // Simulate capturing some packets
    for (int i = 0; i < 10; i++) {
        network_packet_t packet;
        network_packet_init(&packet);
        packet.timestamp = time(NULL) * 1000000 + i * 100000; // Microseconds
        packet.src_ip = 0xC0A80101 + i; // 192.168.1.1 + i
        packet.dst_ip = 0xC0A80164 + i; // 192.168.1.100 + i
        packet.src_port = 1024 + i;
        packet.dst_port = 80;
        packet.protocol = NETWORK_PROTOCOL_TCP;
        packet.packet_size = 100 + i * 10;
        packet.payload_size = 50 + i * 5;
        packet.is_suspicious = (i == 5) ? 1 : 0; // Mark one as suspicious
        packet.threat_type = strdup_safe("Test threat");
        packet.confidence = 0.8;
        
        if (packet.payload_size > 0) {
            packet.payload = (uint8_t*)malloc(packet.payload_size);
            if (packet.payload) {
                for (size_t j = 0; j < packet.payload_size; j++) {
                    packet.payload[j] = (uint8_t)(j % 256);
                }
            }
        }
        
        network_analysis_result_add_packet(result, &packet);
        network_packet_free(&packet);
    }
    
    // Simulate some connections
    for (int i = 0; i < 3; i++) {
        network_connection_t connection;
        network_connection_init(&connection);
        connection.src_ip = 0xC0A80101 + i;
        connection.dst_ip = 0xC0A80164 + i;
        connection.src_port = 1024 + i;
        connection.dst_port = 80;
        connection.protocol = NETWORK_PROTOCOL_TCP;
        connection.start_time = time(NULL) * 1000000;
        connection.end_time = connection.start_time + 10000000; // 10 seconds later
        connection.bytes_sent = 1000 + i * 100;
        connection.bytes_received = 2000 + i * 200;
        connection.packet_count = 20 + i * 5;
        connection.is_suspicious = (i == 1) ? 1 : 0; // Mark one as suspicious
        connection.threat_type = strdup_safe("Test connection threat");
        connection.confidence = 0.7;
        
        network_analysis_result_add_connection(result, &connection);
        network_connection_free(&connection);
    }
    
    return 0;
}

int network_analyze_capture_file(const char* file_path, 
                               const network_capture_options_t* options,
                               network_analysis_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }
    
    network_analysis_result_init(result);
    
    // In a real implementation, this would read and analyze a PCAP file
    // For now, we'll simulate analyzing a file
    
    printf("Analyzing capture file: %s\n", file_path);
    
    // Simulate analysis results
    return network_capture_traffic(options, result);
}

int network_analyze_packet(network_packet_t* packet) {
    if (!packet) {
        return -1;
    }
    
    // In a real implementation, this would analyze the packet for threats
    // For now, we'll just set some basic analysis
    
    // Analyze protocol
    switch (packet->protocol) {
        case NETWORK_PROTOCOL_TCP:
            // Check for suspicious TCP flags or ports
            if (packet->dst_port == 4444 || packet->dst_port == 1337) {
                packet->is_suspicious = 1;
                packet->threat_type = strdup_safe("Suspicious port");
                packet->confidence = 0.9;
            }
            break;
        case NETWORK_PROTOCOL_UDP:
            // Check for suspicious UDP ports
            if (packet->dst_port == 53) {
                // DNS query - check for suspicious content
                packet->is_suspicious = 0; // For now
            }
            break;
        default:
            break;
    }
    
    return 0;
}

int network_analyze_connection(network_connection_t* connection) {
    if (!connection) {
        return -1;
    }
    
    // In a real implementation, this would analyze the connection for threats
    // For now, we'll just set some basic analysis
    
    // Check for suspicious connection patterns
    if (connection->bytes_sent > 1000000 && connection->bytes_received == 0) {
        // Large upload with no download - possibly data exfiltration
        connection->is_suspicious = 1;
        connection->threat_type = strdup_safe("Potential data exfiltration");
        connection->confidence = 0.8;
    } else if (connection->packet_count > 1000 && 
               (connection->bytes_sent + connection->bytes_received) < 10000) {
        // Many small packets - possibly scanning or C&C
        connection->is_suspicious = 1;
        connection->threat_type = strdup_safe("Potential scanning/C&C");
        connection->confidence = 0.7;
    }
    
    return 0;
}

int network_get_statistics(const network_analysis_result_t* result, network_statistics_t* stats) {
    if (!result || !stats) {
        return -1;
    }
    
    network_statistics_init(stats);
    
    // Calculate statistics from analysis result
    for (size_t i = 0; i < result->packet_count; i++) {
        const network_packet_t* packet = &result->packets[i];
        
        switch (packet->protocol) {
            case NETWORK_PROTOCOL_TCP:
                stats->tcp_packets++;
                break;
            case NETWORK_PROTOCOL_UDP:
                stats->udp_packets++;
                break;
            case NETWORK_PROTOCOL_ICMP:
                stats->icmp_packets++;
                break;
            case NETWORK_PROTOCOL_IPV4:
                stats->ipv4_packets++;
                break;
            case NETWORK_PROTOCOL_IPV6:
                stats->ipv6_packets++;
                break;
            case NETWORK_PROTOCOL_HTTP:
                stats->http_requests++;
                break;
            case NETWORK_PROTOCOL_HTTPS:
                stats->https_requests++;
                break;
            case NETWORK_PROTOCOL_DNS:
                stats->dns_queries++;
                break;
            case NETWORK_PROTOCOL_FTP:
                stats->ftp_commands++;
                break;
            case NETWORK_PROTOCOL_SSH:
                stats->ssh_connections++;
                break;
            default:
                break;
        }
        
        if (packet->is_suspicious) {
            stats->suspicious_activities++;
        }
    }
    
    // Calculate averages
    if (result->packet_count > 0) {
        stats->average_packet_size = (double)result->total_bytes / result->packet_count;
        stats->packets_per_second = (double)result->packet_count; // Simplified
    }
    
    return 0;
}

int network_detect_suspicious_activity(const network_analysis_result_t* result, size_t* suspicious_count) {
    if (!result || !suspicious_count) {
        return -1;
    }
    
    *suspicious_count = 0;
    
    // Count suspicious packets
    for (size_t i = 0; i < result->packet_count; i++) {
        if (result->packets[i].is_suspicious) {
            (*suspicious_count)++;
        }
    }
    
    // Count suspicious connections
    for (size_t i = 0; i < result->connection_count; i++) {
        if (result->connections[i].is_suspicious) {
            (*suspicious_count)++;
        }
    }
    
    return 0;
}

int network_export_results(const network_analysis_result_t* result, const char* file_path, const char* format) {
    if (!result || !file_path || !format) {
        return -1;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        return -1;
    }
    
    // Export based on format
    if (strcasecmp(format, "CSV") == 0) {
        // Export as CSV
        fprintf(file, "Timestamp,Source IP,Source Port,Destination IP,Destination Port,Protocol,Size,Suspicious,Threat,Confidence\n");
        
        for (size_t i = 0; i < result->packet_count; i++) {
            const network_packet_t* packet = &result->packets[i];
            char src_ip_str[16], dst_ip_str[16];
            network_ip_to_string(packet->src_ip, src_ip_str, sizeof(src_ip_str));
            network_ip_to_string(packet->dst_ip, dst_ip_str, sizeof(dst_ip_str));
            
            fprintf(file, "%lu,%s,%u,%s,%u,%s,%lu,%s,%s,%.2f\n",
                    (unsigned long)packet->timestamp,
                    src_ip_str,
                    packet->src_port,
                    dst_ip_str,
                    packet->dst_port,
                    network_get_protocol_name(packet->protocol),
                    (unsigned long)packet->packet_size,
                    packet->is_suspicious ? "Yes" : "No",
                    packet->threat_type ? packet->threat_type : "None",
                    packet->confidence);
        }
    } else if (strcasecmp(format, "JSON") == 0) {
        // Export as JSON
        fprintf(file, "{\n");
        fprintf(file, "  \"packets\": [\n");
        
        for (size_t i = 0; i < result->packet_count; i++) {
            const network_packet_t* packet = &result->packets[i];
            char src_ip_str[16], dst_ip_str[16];
            network_ip_to_string(packet->src_ip, src_ip_str, sizeof(src_ip_str));
            network_ip_to_string(packet->dst_ip, dst_ip_str, sizeof(dst_ip_str));
            
            fprintf(file, "    {\n");
            fprintf(file, "      \"timestamp\": %lu,\n", (unsigned long)packet->timestamp);
            fprintf(file, "      \"src_ip\": \"%s\",\n", src_ip_str);
            fprintf(file, "      \"src_port\": %u,\n", packet->src_port);
            fprintf(file, "      \"dst_ip\": \"%s\",\n", dst_ip_str);
            fprintf(file, "      \"dst_port\": %u,\n", packet->dst_port);
            fprintf(file, "      \"protocol\": \"%s\",\n", network_get_protocol_name(packet->protocol));
            fprintf(file, "      \"size\": %lu,\n", (unsigned long)packet->packet_size);
            fprintf(file, "      \"suspicious\": %s,\n", packet->is_suspicious ? "true" : "false");
            fprintf(file, "      \"threat\": \"%s\",\n", packet->threat_type ? packet->threat_type : "None");
            fprintf(file, "      \"confidence\": %.2f\n", packet->confidence);
            fprintf(file, "    }%s\n", (i < result->packet_count - 1) ? "," : "");
        }
        
        fprintf(file, "  ]\n");
        fprintf(file, "}\n");
    }
    
    fclose(file);
    return 0;
}

const char* network_get_protocol_name(network_protocol_t protocol) {
    switch (protocol) {
        case NETWORK_PROTOCOL_ETHERNET:
            return "Ethernet";
        case NETWORK_PROTOCOL_IPV4:
            return "IPv4";
        case NETWORK_PROTOCOL_IPV6:
            return "IPv6";
        case NETWORK_PROTOCOL_TCP:
            return "TCP";
        case NETWORK_PROTOCOL_UDP:
            return "UDP";
        case NETWORK_PROTOCOL_ICMP:
            return "ICMP";
        case NETWORK_PROTOCOL_HTTP:
            return "HTTP";
        case NETWORK_PROTOCOL_HTTPS:
            return "HTTPS";
        case NETWORK_PROTOCOL_DNS:
            return "DNS";
        case NETWORK_PROTOCOL_FTP:
            return "FTP";
        case NETWORK_PROTOCOL_SSH:
            return "SSH";
        case NETWORK_PROTOCOL_TELNET:
            return "Telnet";
        case NETWORK_PROTOCOL_SMTP:
            return "SMTP";
        case NETWORK_PROTOCOL_POP3:
            return "POP3";
        case NETWORK_PROTOCOL_IMAP:
            return "IMAP";
        default:
            return "Unknown";
    }
}

const char* network_get_protocol_description(network_protocol_t protocol) {
    switch (protocol) {
        case NETWORK_PROTOCOL_ETHERNET:
            return "Ethernet frame";
        case NETWORK_PROTOCOL_IPV4:
            return "Internet Protocol version 4";
        case NETWORK_PROTOCOL_IPV6:
            return "Internet Protocol version 6";
        case NETWORK_PROTOCOL_TCP:
            return "Transmission Control Protocol";
        case NETWORK_PROTOCOL_UDP:
            return "User Datagram Protocol";
        case NETWORK_PROTOCOL_ICMP:
            return "Internet Control Message Protocol";
        case NETWORK_PROTOCOL_HTTP:
            return "Hypertext Transfer Protocol";
        case NETWORK_PROTOCOL_HTTPS:
            return "Hypertext Transfer Protocol Secure";
        case NETWORK_PROTOCOL_DNS:
            return "Domain Name System";
        case NETWORK_PROTOCOL_FTP:
            return "File Transfer Protocol";
        case NETWORK_PROTOCOL_SSH:
            return "Secure Shell";
        case NETWORK_PROTOCOL_TELNET:
            return "Telnet protocol";
        case NETWORK_PROTOCOL_SMTP:
            return "Simple Mail Transfer Protocol";
        case NETWORK_PROTOCOL_POP3:
            return "Post Office Protocol version 3";
        case NETWORK_PROTOCOL_IMAP:
            return "Internet Message Access Protocol";
        default:
            return "Unknown protocol";
    }
}

int network_ip_to_string(uint32_t ip, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 16) {
        return -1;
    }
    
    struct in_addr addr;
    addr.s_addr = ip;
    const char* ip_str = inet_ntoa(addr);
    if (ip_str) {
        strncpy(buffer, ip_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return 0;
    }
    
    return -1;
}