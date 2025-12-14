// src/common/protocol.c

#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static uint32_t global_sequence = 0;

// 패킷 타입 → 문자열
const char* packet_type_str(uint8_t type) {
    switch (type) {
        case PKT_CONNECT_REQ:  return "CONNECT_REQ";
        case PKT_CONNECT_RESP: return "CONNECT_RESP";
        case PKT_DATA:         return "DATA";
        case PKT_PING:         return "PING";
        case PKT_PONG:         return "PONG";
        case PKT_DISCONNECT:   return "DISCONNECT";
        default:               return "UNKNOWN";
    }
}

// 현재 타임스탬프 (밀리초)
uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

// VPN 헤더 초기화
void init_vpn_header(vpn_header_t *header, uint8_t type, uint16_t length) {
    memset(header, 0, sizeof(vpn_header_t));
    header->type = type;
    header->version = VPN_PROTOCOL_VERSION;
    header->length = htons(length);
    header->sequence = htonl(++global_sequence);
    header->timestamp = htobe64(get_timestamp_ms());
}

// 패킷 정보 출력
void print_vpn_packet(const vpn_header_t *header) {
    printf("┌─ VPN Packet ─────────────────────\n");
    printf("│ Type:     %s (0x%02x)\n", packet_type_str(header->type), header->type);
    printf("│ Version:  0x%02x\n", header->version);
    printf("│ Length:   %u bytes\n", ntohs(header->length));
    printf("│ Sequence: %u\n", ntohl(header->sequence));
    printf("│ Time:     %lu ms\n", (unsigned long)be64toh(header->timestamp));
    printf("└──────────────────────────────────\n");
}
