#ifndef LIBS_NETWORK_PROTOCOLS_H
#define LIBS_NETWORK_PROTOCOLS_H

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse Ethernet frame
int network_parse_ethernet(const uint8_t* data, size_t size, network_packet_t* packet);

// Parse IPv4 packet
int network_parse_ipv4(const uint8_t* data, size_t size, network_packet_t* packet);

// Parse IPv6 packet
int network_parse_ipv6(const uint8_t* data, size_t size, network_packet_t* packet);

// Parse TCP segment
int network_parse_tcp(const uint8_t* data, size_t size, network_packet_t* packet);

// Parse UDP datagram
int network_parse_udp(const uint8_t* data, size_t size, network_packet_t* packet);

// Parse ICMP packet
int network_parse_icmp(const uint8_t* data, size_t size, network_packet_t* packet);

// Detect application layer protocol
int network_detect_application_protocol(const uint8_t* data, size_t size, network_packet_t* packet);

#ifdef __cplusplus
}
#endif

#endif // LIBS_NETWORK_PROTOCOLS_H