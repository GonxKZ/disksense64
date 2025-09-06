#include "capture.h"
#include "protocols.h"
#include <pcap.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Callback function for pcap_loop
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet_data) {
    network_analysis_result_t* result = (network_analysis_result_t*)user_data;
    network_packet_t p;
    network_packet_init(&p);

    p.timestamp = pkthdr->ts.tv_sec;
    p.packet_size = pkthdr->caplen;

    // Parse Ethernet frame
    if (network_parse_ethernet(packet_data, pkthdr->caplen, &p) == 0) {
        const u_char* ip_packet = packet_data + 14; // Size of Ethernet header
        size_t ip_packet_size = pkthdr->caplen - 14;

        if (p.protocol == NETWORK_PROTOCOL_IPV4) {
            if (network_parse_ipv4(ip_packet, ip_packet_size, &p) == 0) {
                const u_char* transport_packet = ip_packet + ((ip_packet[0] & 0x0F) * 4); // IP header length
                size_t transport_packet_size = ip_packet_size - ((ip_packet[0] & 0x0F) * 4);

                if (p.protocol == NETWORK_PROTOCOL_TCP) {
                    network_parse_tcp(transport_packet, transport_packet_size, &p);
                } else if (p.protocol == NETWORK_PROTOCOL_UDP) {
                    network_parse_udp(transport_packet, transport_packet_size, &p);
                }
            }
        }
    }

    network_analysis_result_add_packet(result, &p);
    network_packet_free(&p);
}

int network_capture_internal(const network_capture_options_t* options, network_analysis_result_t* result) {
    // This would contain the actual packet capture implementation
    return 0;
}

int network_read_pcap_internal(const char* file_path, network_analysis_result_t* result) {
    if (!file_path || !result) {
        return -1;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(file_path, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", file_path, errbuf);
        return -1;
    }

    if (pcap_loop(handle, 0, packet_handler, (u_char*)result) < 0) {
        pcap_close(handle);
        return -1;
    }

    pcap_close(handle);
    return 0;
}
