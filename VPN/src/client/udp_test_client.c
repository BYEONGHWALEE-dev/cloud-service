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
    
    printf("📤 Sending test packets...\n\n");
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 2. CONNECT_REQ 전송 (NEW!)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    printf("━━━ Step 1: Connection Request ━━━\n");
    
    connect_request_t conn_req;
    init_vpn_header(&conn_req.header, PKT_CONNECT_REQ, 
                    sizeof(conn_req) - sizeof(vpn_header_t));
    
    strncpy(conn_req.username, "test_user", sizeof(conn_req.username) - 1);
    memset(conn_req.auth_token, 0xAB, sizeof(conn_req.auth_token)); // 가짜 토큰
    
    printf("Sending CONNECT_REQ...\n");
    print_vpn_packet(&conn_req.header);
    printf("  Username: %s\n", conn_req.username);
    
    ssize_t sent = sendto(sock_fd, &conn_req, sizeof(conn_req), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent < 0) {
        perror("sendto");
        close(sock_fd);
        return 1;
    }
    
    printf("✅ Sent %zd bytes\n\n", sent);
    
    // 응답 대기
    printf("⏳ Waiting for CONNECT_RESP...\n");
    
    uint8_t recv_buffer[2048];
    struct sockaddr_in recv_addr;
    socklen_t recv_len = sizeof(recv_addr);
    
    struct timeval tv = {5, 0};  // 5초 타임아웃
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    ssize_t n = recvfrom(sock_fd, recv_buffer, sizeof(recv_buffer), 0,
                         (struct sockaddr*)&recv_addr, &recv_len);
    
    if (n > 0) {
        connect_response_t *resp = (connect_response_t*)recv_buffer;
        
        printf("📥 CONNECT_RESP received:\n");
        print_vpn_packet(&resp->header);
        
        if (resp->status == 0) {
            struct in_addr vpn_ip;
            vpn_ip.s_addr = resp->vpn_ip;
            
            printf("✅ Connection SUCCESS!\n");
            printf("  VPN IP:     %s\n", inet_ntoa(vpn_ip));
            printf("  Session ID: %u\n", ntohl(resp->session_id));
        } else {
            printf("❌ Connection FAILED!\n");
            printf("  Status: %u\n", resp->status);
        }
    } else {
        printf("⚠️  No response (timeout or error)\n");
    }
    
    printf("\n");
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 3. PING 패킷 (기존)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    printf("━━━ Step 2: PING Test ━━━\n");
    
    vpn_header_t ping_pkt;
    init_vpn_header(&ping_pkt, PKT_PING, 0);
    
    printf("Sending PING...\n");
    print_vpn_packet(&ping_pkt);
    
    sent = sendto(sock_fd, &ping_pkt, sizeof(ping_pkt), 0,
                  (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent < 0) {
        perror("sendto");
        close(sock_fd);
        return 1;
    }
    
    printf("✅ Sent %zd bytes\n\n", sent);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 4. DATA 패킷 (기존)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    printf("━━━ Step 3: DATA Test ━━━\n");
    
    uint8_t packet_buffer[2048];
    data_packet_t *data_pkt = (data_packet_t*)packet_buffer;
    
    // 간단한 ICMP echo request (가짜)
    uint8_t fake_icmp[] = {
        0x45, 0x00, 0x00, 0x54,  // IP header
        0x12, 0x34, 0x40, 0x00,
        0x40, 0x01, 0x00, 0x00,
        0x0a, 0x08, 0x00, 0x05,  // Src: 10.8.0.5
        0xc0, 0xa8, 0x64, 0x0a,  // Dst: 192.168.100.10
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
    printf("✅ All tests complete! Check server logs.\n");
    
    close(sock_fd);
    return 0;
}
