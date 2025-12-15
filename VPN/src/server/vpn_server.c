// src/server/vpn_server.c

#include "tun_manager.h"
#include "udp_server.h"
#include "protocol.h"
#include "client_manager.h"  // ← 추가!
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
void handle_udp_to_tun(int udp_fd, int tun_fd, client_table_t *table) {
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
    
    // 프로토콜 헤더 확인
    if (n < (ssize_t)sizeof(vpn_header_t)) {
        printf("   ⚠️  Packet too short\n");
        return;
    }
    
    vpn_header_t *header = (vpn_header_t*)buffer;
    print_vpn_packet(header);
    
    // 패킷 타입별 처리
    switch (header->type) {
        case PKT_CONNECT_REQ: {
            printf("   → Processing CONNECT_REQ\n");
            
            // VPN IP 할당
            uint32_t vpn_ip = add_client(table, &client_addr);
            
            if (vpn_ip == 0) {
                printf("   ❌ Failed to add client\n");
                return;
            }
            
            // 응답 패킷 생성
            connect_response_t resp;
            init_vpn_header(&resp.header, PKT_CONNECT_RESP, 
                           sizeof(resp) - sizeof(vpn_header_t));
            resp.status = 0;  // 성공
            resp.vpn_ip = vpn_ip;
            
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            resp.session_id = htonl(client->session_id);
            
            // 응답 전송
            udp_send(udp_fd, (uint8_t*)&resp, sizeof(resp), &client_addr);
            
            printf("   → CONNECT_RESP sent\n");
            print_client_table(table);
            break;
        }
        
        case PKT_DATA: {
            // 클라이언트 찾기 및 활동 갱신
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (client) {
                update_client_activity(client);
            }
            
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
            break;
        }
        
        case PKT_PING: {
            printf("   → PING received, sending PONG\n");
            
            // 클라이언트 활동 갱신
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (client) {
                update_client_activity(client);
            }
            
            // PONG 응답
            vpn_header_t pong;
            init_vpn_header(&pong, PKT_PONG, 0);
            udp_send(udp_fd, (uint8_t*)&pong, sizeof(pong), &client_addr);
            break;
        }
        
        case PKT_DISCONNECT: {
            printf("   → DISCONNECT received\n");
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (client) {
                remove_client(table, client->vpn_ip);
                print_client_table(table);
            }
            break;
        }
        
        default:
            printf("   ⚠️  Unknown packet type: 0x%02x\n", header->type);
    }
}

// TUN에서 받은 패킷 처리 (수정 - TODO 제거!)
void handle_tun_to_udp(int tun_fd, int udp_fd, client_table_t *table) {
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
    
    // IP 헤더에서 목적지 확인
    if (n < 20) {  // 최소 IP 헤더 크기
        printf("   ⚠️  Packet too short for IP\n");
        return;
    }
    
    struct iphdr {
        uint8_t  ihl:4, version:4;
        uint8_t  tos;
        uint16_t tot_len;
        uint16_t id;
        uint16_t frag_off;
        uint8_t  ttl;
        uint8_t  protocol;
        uint16_t check;
        uint32_t saddr;
        uint32_t daddr;
    } __attribute__((packed));
    
    struct iphdr *ip = (struct iphdr*)buffer;
    uint32_t dst_ip = ip->daddr;
    
    // 목적지 클라이언트 찾기
    client_entry_t *client = find_client_by_vpn_ip(table, dst_ip);
    
    if (!client) {
        struct in_addr dst_addr;
        dst_addr.s_addr = dst_ip;
        printf("   ⚠️  No client found for VPN IP: %s\n", inet_ntoa(dst_addr));
        return;
    }
    
    // VPN 헤더 추가
    data_packet_t *pkt = (data_packet_t*)packet_buffer;
    init_vpn_header(&pkt->header, PKT_DATA, n);
    memcpy(pkt->data, buffer, n);
    
    size_t total_len = sizeof(vpn_header_t) + n;
    
    // UDP로 전송
    ssize_t sent = udp_send(udp_fd, packet_buffer, total_len, &client->real_addr);
    
    if (sent > 0) {
        printf("   → UDP: Sent %zd bytes to %s:%d\n",
               sent,
               inet_ntoa(client->real_addr.sin_addr),
               ntohs(client->real_addr.sin_port));
        
        update_client_activity(client);
    }
}

int main() {
    int tun_fd, udp_fd;
    fd_set read_fds;
    int max_fd;
    client_table_t *client_table;  // ← 추가!
    
    printf("🚀 VPN Server Starting...\n");
    printf("═══════════════════════════════════════\n\n");
    
    // 시그널 핸들러
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 랜덤 시드 초기화
    srand(time(NULL));
    
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
    
    // 3. 클라이언트 테이블 초기화
    printf("━━━ Client Table ━━━\n");
    client_table = init_client_table();
    if (!client_table) {
        close(udp_fd);
        close(tun_fd);
        return 1;
    }
    printf("\n");
    
    // 4. 파일 디스크립터 정보
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
    
    // 5. 이벤트 루프
    max_fd = (tun_fd > udp_fd) ? tun_fd : udp_fd;
    
    time_t last_timeout_check = time(NULL);
    
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
            // 타임아웃: 클라이언트 타임아웃 체크 (30초마다)
            time_t now = time(NULL);
            if (now - last_timeout_check >= 30) {
                check_client_timeouts(client_table);
                last_timeout_check = now;
            }
            continue;
        }
        
        // UDP 소켓에서 패킷 수신
        if (FD_ISSET(udp_fd, &read_fds)) {
            handle_udp_to_tun(udp_fd, tun_fd, client_table);
        }
        
        // TUN 인터페이스에서 패킷 수신
        if (FD_ISSET(tun_fd, &read_fds)) {
            handle_tun_to_udp(tun_fd, udp_fd, client_table);
        }
    }
    
    // 6. 정리
    printf("\n🧹 Cleaning up...\n");
    destroy_client_table(client_table);
    close(udp_fd);
    close(tun_fd);
    
    printf("✅ VPN Server stopped.\n");
    
    return 0;
}
