// src/server/vpn_server.c

#include "tun_manager.h"
#include "udp_server.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define TUN_DEVICE "tun0"
#define TUN_IP "10.8.0.1"
#define TUN_NETMASK 24
#define UDP_PORT 51820

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n🛑 Shutting down...\n");
        running = 0;
    }
}

// UDP에서 받은 패킷 처리
void handle_udp_to_tun(int udp_fd, int tun_fd) {
    uint8_t buffer[2048];
    struct sockaddr_in client_addr;
    
    // UDP에서 패킷 수신
    ssize_t n = udp_recv(udp_fd, buffer, sizeof(buffer), &client_addr);
    
    if (n < 0) return;
    
    printf("\n📥 UDP Packet Received:\n");
    printf("   From: %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
    printf("   Size: %zd bytes\n", n);
    
    // 프로토콜 헤더 확인 (최소 크기 체크)
    if (n >= sizeof(vpn_header_t)) {
        vpn_header_t *header = (vpn_header_t*)buffer;
        print_vpn_packet(header);
        
        // 데이터 패킷이면 TUN으로 전달
        if (header->type == PKT_DATA) {
            // 헤더 이후 데이터를 TUN에 쓰기
            uint8_t *ip_packet = buffer + sizeof(vpn_header_t);
            size_t ip_packet_len = n - sizeof(vpn_header_t);
            
            if (ip_packet_len > 0) {
                ssize_t written = write(tun_fd, ip_packet, ip_packet_len);
                if (written > 0) {
                    printf("   → TUN: Written %zd bytes\n", written);
                    print_ip_packet(ip_packet, written);
                }
            }
        }
    }
}

// TUN에서 받은 패킷 처리
void handle_tun_to_udp(int tun_fd, int udp_fd) {
    uint8_t buffer[2048];
    uint8_t packet_buffer[2048];
    
    // TUN에서 패킷 읽기
    ssize_t n = read(tun_fd, buffer, sizeof(buffer));
    
    if (n < 0) {
        perror("❌ TUN read failed");
        return;
    }
    
    printf("\n📤 TUN Packet Received:\n");
    printf("   Size: %zd bytes\n", n);
    print_ip_packet(buffer, n);
    
    // VPN 헤더 추가
    data_packet_t *pkt = (data_packet_t*)packet_buffer;
    init_vpn_header(&pkt->header, PKT_DATA, n);
    memcpy(pkt->data, buffer, n);
    
    size_t total_len = sizeof(vpn_header_t) + n;
    
    // TODO: 실제로는 목적지 클라이언트를 찾아야 함
    // 지금은 테스트용으로 마지막 클라이언트에게 전송
    // (나중에 클라이언트 테이블 구현 필요)
    
    printf("   → UDP: Ready to send %zu bytes\n", total_len);
    printf("   (클라이언트 테이블 구현 후 전송 가능)\n");
}

int main() {
    int tun_fd, udp_fd;
    fd_set read_fds;
    int max_fd;
    
    printf("🚀 VPN Server Starting...\n");
    printf("═══════════════════════════════════════\n\n");
    
    // 시그널 핸들러
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 1. TUN 인터페이스 생성
    printf("━━━ TUN Interface ━━━\n");
    tun_fd = create_tun_interface(TUN_DEVICE);
    if (tun_fd < 0) {
        return 1;
    }
    
    if (configure_tun_ip(TUN_DEVICE, TUN_IP, TUN_NETMASK) < 0) {
        close(tun_fd);
        return 1;
    }
    
    if (bring_tun_up(TUN_DEVICE) < 0) {
        close(tun_fd);
        return 1;
    }
    printf("\n");
    
    // 2. UDP 서버 생성
    printf("━━━ UDP Server ━━━\n");
    udp_fd = create_udp_server(UDP_PORT);
    if (udp_fd < 0) {
        close(tun_fd);
        return 1;
    }
    printf("\n");
    
    // 3. 파일 디스크립터 정보
    printf("━━━ File Descriptors ━━━\n");
    printf("  TUN Interface: fd=%d\n", tun_fd);
    printf("  UDP Socket:    fd=%d\n", udp_fd);
    printf("\n");
    
    printf("✅ VPN Server is running!\n");
    printf("═══════════════════════════════════════\n");
    printf("📡 Listening on:\n");
    printf("   - TUN: %s/%d\n", TUN_IP, TUN_NETMASK);
    printf("   - UDP: 0.0.0.0:%d\n", UDP_PORT);
    printf("═══════════════════════════════════════\n");
    printf("⏳ Waiting for packets... (Ctrl+C to stop)\n\n");
    
    // 4. 이벤트 루프
    max_fd = (tun_fd > udp_fd) ? tun_fd : udp_fd;
    
    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(tun_fd, &read_fds);
        FD_SET(udp_fd, &read_fds);
        
        struct timeval timeout = {1, 0};  // 1초 타임아웃
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (running) {
                perror("❌ select failed");
            }
            break;
        }
        
        if (activity == 0) {
            // 타임아웃 (아무 일도 없음)
            continue;
        }
        
        // UDP 소켓에서 패킷 수신
        if (FD_ISSET(udp_fd, &read_fds)) {
            handle_udp_to_tun(udp_fd, tun_fd);
        }
        
        // TUN 인터페이스에서 패킷 수신
        if (FD_ISSET(tun_fd, &read_fds)) {
            handle_tun_to_udp(tun_fd, udp_fd);
        }
    }
    
    // 5. 정리
    printf("\n🧹 Cleaning up...\n");
    close(udp_fd);
    close(tun_fd);
    
    printf("✅ VPN Server stopped.\n");
    
    return 0;
}
