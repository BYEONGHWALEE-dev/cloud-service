// src/client/udp_test_client.c

#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 127.0.0.1\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    uint16_t server_port = 51820;
    
    printf("🧪 UDP Test Client\n");
    printf("═══════════════════════════════════\n");
    printf("Server: %s:%u\n\n", server_ip, server_port);
    
    // 1. UDP 소켓 생성
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(server_port)
    };
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP: %s\n", server_ip);
        close(sock_fd);
        return 1;
    }
    
    // 2. 테스트 패킷 전송
    printf("📤 Sending test packets...\n\n");
    
    // PING 패킷
    vpn_header_t ping_pkt;
    init_vpn_header(&ping_pkt, PKT_PING, 0);
    
    printf("Sending PING...\n");
    print_vpn_packet(&ping_pkt);
    
    ssize_t sent = sendto(sock_fd, &ping_pkt, sizeof(ping_pkt), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent < 0) {
        perror("sendto");
        close(sock_fd);
        return 1;
    }
    
    printf("✅ Sent %zd bytes\n\n", sent);
    
    // 3. 간단한 IP 패킷 전송 (DATA)
    uint8_t packet_buffer[2048];
    data_packet_t *data_pkt = (data_packet_t*)packet_buffer;
    
    // 간단한 ICMP echo request (가짜)
    uint8_t fake_icmp[] = {
        0x45, 0x00, 0x00, 0x54,  // IP header
        0x12, 0x34, 0x40, 0x00,
        0x40, 0x01, 0x00, 0x00,
        0x0a, 0x08, 0x00, 0x05,  // Src: 10.8.0.5
        0xc0, 0xa8, 0x64, 0x0a,  // Dst: 192.168.100.10
        // ICMP data...
    };
    
    init_vpn_header(&data_pkt->header, PKT_DATA, sizeof(fake_icmp));
    memcpy(data_pkt->data, fake_icmp, sizeof(fake_icmp));
    
    size_t total_len = sizeof(vpn_header_t) + sizeof(fake_icmp);
    
    printf("Sending DATA packet...\n");
    print_vpn_packet(&data_pkt->header);
    
    sent = sendto(sock_fd, packet_buffer, total_len, 0,
                  (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent < 0) {
        perror("sendto");
        close(sock_fd);
        return 1;
    }
    
    printf("✅ Sent %zd bytes\n\n", sent);
    
    printf("═══════════════════════════════════\n");
    printf("✅ Test complete! Check server logs.\n");
    
    close(sock_fd);
    return 0;
}
