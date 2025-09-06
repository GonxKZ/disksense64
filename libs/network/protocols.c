#include "protocols.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

// Ethernet header structure
#pragma pack(push, 1)
typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} ethernet_header_t;
#pragma pack(pop)

// IPv4 header structure
#pragma pack(push, 1)
typedef struct {
    uint8_t version_ihl;
    uint8_t type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ipv4_header_t;
#pragma pack(pop)

// TCP header structure
#pragma pack(push, 1)
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_number;
    uint32_t ack_number;
    uint16_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} tcp_header_t;
#pragma pack(pop)

// UDP header structure
#pragma pack(push, 1)
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;
#pragma pack(pop)

int network_parse_ethernet(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < sizeof(ethernet_header_t) || !packet) {
        return -1;
    }
    
    const ethernet_header_t* eth_header = (const ethernet_header_t*)data;
    
    // Set protocol based on ethertype
    switch (ntohs(eth_header->ethertype)) {
        case 0x0800: // IPv4
            packet->protocol = NETWORK_PROTOCOL_IPV4;
            break;
        case 0x86DD: // IPv6
            packet->protocol = NETWORK_PROTOCOL_IPV6;
            break;
        default:
            packet->protocol = NETWORK_PROTOCOL_ETHERNET;
            break;
    }
    
    return 0;
}

int network_parse_ipv4(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < sizeof(ipv4_header_t) || !packet) {
        return -1;
    }
    
    const ipv4_header_t* ip_header = (const ipv4_header_t*)data;
    
    // Extract IP addresses
    packet->src_ip = ip_header->src_ip;
    packet->dst_ip = ip_header->dst_ip;
    
    // Set protocol based on IP protocol field
    switch (ip_header->protocol) {
        case 6:  // TCP
            packet->protocol = NETWORK_PROTOCOL_TCP;
            break;
        case 17: // UDP
            packet->protocol = NETWORK_PROTOCOL_UDP;
            break;
        case 1:  // ICMP
            packet->protocol = NETWORK_PROTOCOL_ICMP;
            break;
        default:
            packet->protocol = NETWORK_PROTOCOL_IPV4;
            break;
    }
    
    return 0;
}

int network_parse_ipv6(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < 40 || !packet) { // Minimum IPv6 header size
        return -1;
    }
    
    // Extract IP addresses (simplified)
    memcpy(&packet->src_ip, data + 8, 4);  // Simplified extraction
    memcpy(&packet->dst_ip, data + 24, 4); // Simplified extraction
    
    packet->protocol = NETWORK_PROTOCOL_IPV6;
    return 0;
}

int network_parse_tcp(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < sizeof(tcp_header_t) || !packet) {
        return -1;
    }
    
    const tcp_header_t* tcp_header = (const tcp_header_t*)data;
    
    // Extract ports
    packet->src_port = ntohs(tcp_header->src_port);
    packet->dst_port = ntohs(tcp_header->dst_port);
    
    packet->protocol = NETWORK_PROTOCOL_TCP;
    return 0;
}

int network_parse_udp(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < sizeof(udp_header_t) || !packet) {
        return -1;
    }
    
    const udp_header_t* udp_header = (const udp_header_t*)data;
    
    // Extract ports
    packet->src_port = ntohs(udp_header->src_port);
    packet->dst_port = ntohs(udp_header->dst_port);
    
    packet->protocol = NETWORK_PROTOCOL_UDP;
    return 0;
}

int network_parse_icmp(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size < 8 || !packet) { // Minimum ICMP header size
        return -1;
    }
    
    packet->protocol = NETWORK_PROTOCOL_ICMP;
    return 0;
}

int network_detect_application_protocol(const uint8_t* data, size_t size, network_packet_t* packet) {
    if (!data || size == 0 || !packet) {
        return -1;
    }
    
    // Check for common application layer protocols based on port numbers
    if (packet->dst_port == 80 || packet->src_port == 80) {
        packet->protocol = NETWORK_PROTOCOL_HTTP;
    } else if (packet->dst_port == 443 || packet->src_port == 443) {
        packet->protocol = NETWORK_PROTOCOL_HTTPS;
    } else if (packet->dst_port == 53 || packet->src_port == 53) {
        packet->protocol = NETWORK_PROTOCOL_DNS;
    } else if (packet->dst_port == 21 || packet->src_port == 21) {
        packet->protocol = NETWORK_PROTOCOL_FTP;
    } else if (packet->dst_port == 22 || packet->src_port == 22) {
        packet->protocol = NETWORK_PROTOCOL_SSH;
    } else if (packet->dst_port == 23 || packet->src_port == 23) {
        packet->protocol = NETWORK_PROTOCOL_TELNET;
    } else if (packet->dst_port == 25 || packet->src_port == 25) {
        packet->protocol = NETWORK_PROTOCOL_SMTP;
    } else if (packet->dst_port == 110 || packet->src_port == 110) {
        packet->protocol = NETWORK_PROTOCOL_POP3;
    } else if (packet->dst_port == 143 || packet->src_port == 143) {
        packet->protocol = NETWORK_PROTOCOL_IMAP;
    }
    
    // Check payload for protocol signatures
    if (size >= 4) {
        // HTTP detection
        if (memcmp(data, "GET ", 4) == 0 || 
            memcmp(data, "POST", 4) == 0 || 
            memcmp(data, "HEAD", 4) == 0 ||
            memcmp(data, "HTTP", 4) == 0) {
            packet->protocol = NETWORK_PROTOCOL_HTTP;
        }
        // DNS detection
        else if (size >= 12 && data[2] == 0x01 && data[3] == 0x00) {
            // Simplified DNS detection
            packet->protocol = NETWORK_PROTOCOL_DNS;
        }
    }
    
    return 0;
}