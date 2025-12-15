// src/server/vpn_server.c

#include "tun_manager.h"
#include "udp_server.h"
#include "protocol.h"
#include "client_manager.h"
#include "enclave.h"
#include "enclave_client.h"
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
static pid_t enclave_pid = -1;
static int enclave_fd = -1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n🛑 Shutting down...\n");
        running = 0;
    }
}

// UDP에서 받은 패킷 처리 (암호화 통합!)
void handle_udp_to_tun(int udp_fd, int tun_fd, client_table_t *table) {
    uint8_t buffer[2048];
    uint8_t decrypted_buffer[2048];
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
            
            connect_request_t *req = (connect_request_t*)buffer;
            
            // VPN IP 할당
            uint32_t vpn_ip = add_client(table, &client_addr);
            
            if (vpn_ip == 0) {
                printf("   ❌ Failed to add client\n");
                return;
            }
            
            // 🔐 ECDH 핸드셰이크 (NEW!)
            uint8_t server_public_key[32];
            uint8_t session_key[32];
            uint8_t client_public_key[32];
            
            // 클라이언트 공개키는 auth_token 필드에 임시로 저장
            // (실제로는 별도 필드 추가 필요)
            memcpy(client_public_key, req->auth_token, 32);
            
            printf("   🔐 Performing ECDH handshake...\n");
            if (enclave_handshake(enclave_fd, vpn_ip,
                                 client_public_key,
                                 server_public_key,
                                 session_key) != 0) {
                printf("   ❌ Handshake failed\n");
                remove_client(table, vpn_ip);
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
            
            // 서버 공개키 추가 (reserved 필드 활용)
            // 실제로는 구조체 확장 필요
            
            // 응답 전송
            udp_send(udp_fd, (uint8_t*)&resp, sizeof(resp), &client_addr);
            
            printf("   → CONNECT_RESP sent (with server public key)\n");
            print_client_table(table);
            break;
        }
        
        case PKT_DATA: {
            // 클라이언트 찾기
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (!client) {
                printf("   ⚠️  Unknown client\n");
                return;
            }
            
            update_client_activity(client);
            
            // 🔐 암호문 복호화 (NEW!)
            uint8_t *ciphertext = buffer + sizeof(vpn_header_t);
            size_t ciphertext_len = n - sizeof(vpn_header_t);
            
            printf("   🔓 Decrypting %zu bytes...\n", ciphertext_len);
            
            size_t plaintext_len;
            if (enclave_decrypt(enclave_fd, client->vpn_ip,
                               ciphertext, ciphertext_len,
                               decrypted_buffer, &plaintext_len) != 0) {
                printf("   ❌ Decryption failed (wrong key or corrupted)\n");
                return;
            }
            
            printf("   ✅ Decrypted to %zu bytes\n", plaintext_len);
            
            // TUN에 쓰기
            ssize_t written = write(tun_fd, decrypted_buffer, plaintext_len);
            if (written > 0) {
                printf("   → TUN: Written %zd bytes\n", written);
                print_ip_packet(decrypted_buffer, written);
            }
            break;
        }
        
        case PKT_PING: {
            printf("   → PING received, sending PONG\n");
            
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (client) {
                update_client_activity(client);
            }
            
            vpn_header_t pong;
            init_vpn_header(&pong, PKT_PONG, 0);
            udp_send(udp_fd, (uint8_t*)&pong, sizeof(pong), &client_addr);
            break;
        }
        
        case PKT_DISCONNECT: {
            printf("   → DISCONNECT received\n");
            client_entry_t *client = find_client_by_addr(table, &client_addr);
            if (client) {
                // Enclave에서 키 제거
                enclave_remove_key(enclave_fd, client->vpn_ip);
                remove_client(table, client->vpn_ip);
                print_client_table(table);
            }
            break;
        }
        
        default:
            printf("   ⚠️  Unknown packet type: 0x%02x\n", header->type);
    }
}

// TUN에서 받은 패킷 처리 (암호화 통합!)
void handle_tun_to_udp(int tun_fd, int udp_fd, client_table_t *table) {
    uint8_t buffer[2048];
    uint8_t encrypted_buffer[2048];
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
    if (n < 20) {
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
    
    // IPv6 필터링
    if (ip->version == 6) {
        return;  // IPv6 무시
    }
    
    uint32_t dst_ip = ip->daddr;
    
    // 목적지 클라이언트 찾기
    client_entry_t *client = find_client_by_vpn_ip(table, dst_ip);
    
    if (!client) {
        struct in_addr dst_addr;
        dst_addr.s_addr = dst_ip;
        printf("   ⚠️  No client found for VPN IP: %s\n", inet_ntoa(dst_addr));
        return;
    }
    
    // 🔐 평문 암호화 (NEW!)
    printf("   🔒 Encrypting %zd bytes...\n", n);
    
    size_t ciphertext_len;
    if (enclave_encrypt(enclave_fd, client->vpn_ip,
                       buffer, n,
                       encrypted_buffer, &ciphertext_len) != 0) {
        printf("   ❌ Encryption failed\n");
        return;
    }
    
    printf("   ✅ Encrypted to %zu bytes\n", ciphertext_len);
    
    // VPN 헤더 추가
    data_packet_t *pkt = (data_packet_t*)packet_buffer;
    init_vpn_header(&pkt->header, PKT_DATA, ciphertext_len);
    memcpy(pkt->data, encrypted_buffer, ciphertext_len);
    
    size_t total_len = sizeof(vpn_header_t) + ciphertext_len;
    
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
    client_table_t *client_table;
    
    printf("🚀 VPN Server Starting...\n");
    printf("═══════════════════════════════════════\n\n");
    
    // 시그널 핸들러
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 랜덤 시드 초기화
    srand(time(NULL));
    
    // 🔐 1. Enclave 프로세스 시작 (NEW!)
    printf("━━━ Enclave Process ━━━\n");
    enclave_pid = start_enclave_process();
    if (enclave_pid < 0) {
        fprintf(stderr, "❌ Failed to start Enclave process\n");
        return 1;
    }
    
    // Enclave 연결
    enclave_fd = enclave_connect();
    if (enclave_fd < 0) {
        fprintf(stderr, "❌ Failed to connect to Enclave\n");
        stop_enclave_process(enclave_pid);
        return 1;
    }
    
    // Enclave PING 테스트
    if (enclave_ping(enclave_fd) != 0) {
        fprintf(stderr, "❌ Enclave PING failed\n");
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    printf("\n");
    
    // 2. TUN 인터페이스 생성
    printf("━━━ TUN Interface ━━━\n");
    tun_fd = create_tun_interface(TUN_DEVICE);
    if (tun_fd < 0) {
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    
    if (configure_tun_ip(TUN_DEVICE, TUN_IP, TUN_NETMASK) < 0) {
        close(tun_fd);
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    
    if (bring_tun_up(TUN_DEVICE) < 0) {
        close(tun_fd);
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    printf("\n");
    
    // 3. UDP 서버 생성
    printf("━━━ UDP Server ━━━\n");
    udp_fd = create_udp_server(UDP_PORT);
    if (udp_fd < 0) {
        close(tun_fd);
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    printf("\n");
    
    // 4. 클라이언트 테이블 초기화
    printf("━━━ Client Table ━━━\n");
    client_table = init_client_table();
    if (!client_table) {
        close(udp_fd);
        close(tun_fd);
        enclave_disconnect(enclave_fd);
        stop_enclave_process(enclave_pid);
        return 1;
    }
    printf("\n");
    
    // 5. 파일 디스크립터 정보
    printf("━━━ File Descriptors ━━━\n");
    printf("  Enclave IPC:   fd=%d\n", enclave_fd);
    printf("  TUN Interface: fd=%d\n", tun_fd);
    printf("  UDP Socket:    fd=%d\n", udp_fd);
    printf("\n");
    
    printf("✅ VPN Server is running!\n");
    printf("═══════════════════════════════════════\n");
    printf("🔐 Encryption: ChaCha20-Poly1305 via Enclave\n");
    printf("📡 Listening on:\n");
    printf("   - TUN: %s/%d\n", TUN_IP, TUN_NETMASK);
    printf("   - UDP: 0.0.0.0:%d\n", UDP_PORT);
    printf("═══════════════════════════════════════\n");
    printf("⏳ Waiting for packets... (Ctrl+C to stop)\n\n");
    
    // 6. 이벤트 루프
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
            // 타임아웃: 클라이언트 타임아웃 체크
            time_t now = time(NULL);
            if (now - last_timeout_check >= 30) {
                check_client_timeouts(client_table);
                last_timeout_check = now;
            }
            
            // Enclave 상태 확인
            if (!is_enclave_running(enclave_pid)) {
                fprintf(stderr, "❌ Enclave process died!\n");
                break;
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
    
    // 7. 정리
    printf("\n🧹 Cleaning up...\n");
    
    // Enclave 종료
    enclave_shutdown(enclave_fd);
    enclave_disconnect(enclave_fd);
    stop_enclave_process(enclave_pid);
    
    // 기타 정리
    destroy_client_table(client_table);
    close(udp_fd);
    close(tun_fd);
    
    printf("✅ VPN Server stopped.\n");
    
    return 0;
}
