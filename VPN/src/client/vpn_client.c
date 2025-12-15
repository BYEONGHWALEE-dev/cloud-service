// src/client/vpn_client.c

#include "protocol.h"
#include "crypto.h"
#include "tun_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <sodium.h>

#define KEEPALIVE_INTERVAL 30
#define PONG_TIMEOUT 60
#define MAX_RECONNECT_ATTEMPTS 10   // 최대 재연결 시도
#define INITIAL_BACKOFF 1           // 초기 백오프 (1초)
#define MAX_BACKOFF 60              // 최대 백오프 (60초)

typedef struct {
    int sock_fd;
    int tun_fd;
    struct sockaddr_in server_addr;
    
    uint8_t client_private_key[32];
    uint8_t client_public_key[32];
    uint8_t server_public_key[32];
    uint8_t session_key[32];
    
    uint32_t vpn_ip;
    uint32_t session_id;
    int connected;
    
    time_t last_ping_sent;
    time_t last_pong_received;
    
    // 재연결 관련
    int reconnect_attempts;
    int backoff_seconds;
    char server_ip[INET_ADDRSTRLEN];
    uint16_t server_port;
    char username[64];
} vpn_client_t;

volatile sig_atomic_t client_running = 1;

void client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n🛑 Client shutting down...\n");
        client_running = 0;
    }
}

vpn_client_t* init_vpn_client(const char *server_ip, uint16_t server_port) {
    vpn_client_t *client = (vpn_client_t*)malloc(sizeof(vpn_client_t));
    if (!client) {
        perror("malloc");
        return NULL;
    }
    
    memset(client, 0, sizeof(vpn_client_t));
    client->tun_fd = -1;
    client->sock_fd = -1;
    
    // 서버 정보 저장 (재연결용)
    strncpy(client->server_ip, server_ip, sizeof(client->server_ip) - 1);
    client->server_port = server_port;
    client->backoff_seconds = INITIAL_BACKOFF;
    
    if (crypto_init() != 0) {
        free(client);
        return NULL;
    }
    
    crypto_generate_keypair(client->client_public_key, client->client_private_key);
    
    printf("✅ VPN Client initialized\n");
    printf("   Server: %s:%u\n", server_ip, server_port);
    
    return client;
}

void destroy_vpn_client(vpn_client_t *client) {
    if (client) {
        if (client->connected && client->sock_fd >= 0) {
            uint8_t buffer[sizeof(vpn_header_t)];
            vpn_header_t *disconnect = (vpn_header_t*)buffer;
            init_vpn_header(disconnect, PKT_DISCONNECT, 0);
            
            sendto(client->sock_fd, buffer, sizeof(vpn_header_t), 0,
                   (struct sockaddr*)&client->server_addr,
                   sizeof(client->server_addr));
            
            printf("📤 DISCONNECT sent\n");
        }
        
        if (client->sock_fd >= 0) {
            close(client->sock_fd);
        }
        if (client->tun_fd >= 0) {
            close(client->tun_fd);
        }
        sodium_memzero(client, sizeof(vpn_client_t));
        free(client);
        printf("🧹 VPN Client destroyed\n");
    }
}

// 소켓 재생성
int recreate_socket(vpn_client_t *client) {
    if (client->sock_fd >= 0) {
        close(client->sock_fd);
    }
    
    client->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->sock_fd < 0) {
        perror("socket");
        return -1;
    }
    
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->server_port);
    
    if (inet_pton(AF_INET, client->server_ip, &client->server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP\n");
        close(client->sock_fd);
        client->sock_fd = -1;
        return -1;
    }
    
    return 0;
}

int vpn_connect(vpn_client_t *client, const char *username) {
    uint8_t buffer[2048];
    
    printf("\n🔐 Connecting to VPN server...\n");
    
    // 소켓 재생성 (재연결 시)
    if (recreate_socket(client) != 0) {
        return -1;
    }
    
    connect_request_t *req = (connect_request_t*)buffer;
    init_vpn_header(&req->header, PKT_CONNECT_REQ,
                    sizeof(connect_request_t) - sizeof(vpn_header_t));
    
    strncpy(req->username, username, sizeof(req->username) - 1);
    memcpy(req->auth_token, client->client_public_key, 32);
    
    printf("   Sending CONNECT_REQ...\n");
    
    ssize_t sent = sendto(client->sock_fd, buffer, sizeof(connect_request_t), 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    printf("   Waiting for CONNECT_RESP...\n");
    
    struct timeval tv = {5, 0};
    setsockopt(client->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in recv_addr;
    socklen_t recv_len = sizeof(recv_addr);
    
    ssize_t n = recvfrom(client->sock_fd, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&recv_addr, &recv_len);
    
    if (n < 0) {
        perror("recvfrom");
        return -1;
    }
    
    connect_response_t *resp = (connect_response_t*)buffer;
    
    if (resp->header.type != PKT_CONNECT_RESP) {
        fprintf(stderr, "   ❌ Unexpected response type\n");
        return -1;
    }
    
    if (resp->status != 0) {
        fprintf(stderr, "   ❌ Connection failed\n");
        return -1;
    }
    
    client->vpn_ip = resp->vpn_ip;
    client->session_id = ntohl(resp->session_id);
    memcpy(client->server_public_key, resp->server_public_key, 32);
    
    struct in_addr vpn_addr;
    vpn_addr.s_addr = client->vpn_ip;
    
    printf("   ✅ Connection SUCCESS!\n");
    printf("   VPN IP: %s\n", inet_ntoa(vpn_addr));
    printf("   Session ID: %u\n", client->session_id);
    
    printf("   🔑 Generating session key (ECDH)...\n");
    
    uint8_t shared_secret[32];
    
    if (crypto_ecdh(shared_secret, client->client_private_key,
                   client->server_public_key) != 0) {
        fprintf(stderr, "   ❌ ECDH failed\n");
        return -1;
    }
    
    crypto_kdf_derive_from_key(
        client->session_key,
        32,
        1,
        "VPN_SESS",
        shared_secret
    );
    
    sodium_memzero(shared_secret, 32);
    
    printf("   ✅ Session key generated\n");
    
    client->connected = 1;
    client->reconnect_attempts = 0;  // 재연결 카운터 리셋
    client->backoff_seconds = INITIAL_BACKOFF;  // 백오프 리셋
    
    client->last_pong_received = time(NULL);
    client->last_ping_sent = time(NULL);
    
    return 0;
}

int setup_client_tun(vpn_client_t *client) {
    // TUN이 이미 있으면 재사용
    if (client->tun_fd >= 0) {
        printf("\n━━━ Reusing TUN Interface ━━━\n");
        return 0;
    }
    
    printf("\n━━━ Client TUN Interface ━━━\n");
    
    client->tun_fd = create_tun_interface("tun1");
    if (client->tun_fd < 0) {
        return -1;
    }
    
    struct in_addr vpn_addr;
    vpn_addr.s_addr = client->vpn_ip;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &vpn_addr, ip_str, sizeof(ip_str));
    
    if (configure_tun_ip("tun1", ip_str, 24) < 0) {
        close(client->tun_fd);
        client->tun_fd = -1;
        return -1;
    }
    
    if (bring_tun_up("tun1") < 0) {
        close(client->tun_fd);
        client->tun_fd = -1;
        return -1;
    }
    
    printf("\n");
    return 0;
}

void send_ping(vpn_client_t *client) {
    uint8_t buffer[sizeof(vpn_header_t)];
    
    vpn_header_t *ping = (vpn_header_t*)buffer;
    init_vpn_header(ping, PKT_PING, 0);
    
    ssize_t sent = sendto(client->sock_fd, buffer, sizeof(vpn_header_t), 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent > 0) {
        client->last_ping_sent = time(NULL);
        printf("🏓 PING sent (keepalive)\n");
    }
}

int check_keepalive(vpn_client_t *client) {
    time_t now = time(NULL);
    
    if (now - client->last_pong_received > PONG_TIMEOUT) {
        fprintf(stderr, "❌ No PONG received for %d seconds\n", PONG_TIMEOUT);
        fprintf(stderr, "   Connection lost!\n");
        return -1;
    }
    
    if (now - client->last_ping_sent >= KEEPALIVE_INTERVAL) {
        send_ping(client);
    }
    
    return 0;
}

// 재연결 시도
int attempt_reconnect(vpn_client_t *client) {
    if (client->reconnect_attempts >= MAX_RECONNECT_ATTEMPTS) {
        fprintf(stderr, "\n❌ Max reconnect attempts reached (%d)\n", MAX_RECONNECT_ATTEMPTS);
        return -1;
    }
    
    client->reconnect_attempts++;
    
    printf("\n🔄 Reconnecting... (attempt %d/%d)\n",
           client->reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
    printf("   Waiting %d seconds (exponential backoff)...\n",
           client->backoff_seconds);
    
    sleep(client->backoff_seconds);
    
    // 지수 백오프
    client->backoff_seconds *= 2;
    if (client->backoff_seconds > MAX_BACKOFF) {
        client->backoff_seconds = MAX_BACKOFF;
    }
    
    // 연결 상태 초기화
    client->connected = 0;
    
    // 재연결 시도
    if (vpn_connect(client, client->username) != 0) {
        fprintf(stderr, "   ❌ Reconnection failed\n");
        return -1;
    }
    
    // TUN 설정 (이미 있으면 재사용)
    if (setup_client_tun(client) != 0) {
        fprintf(stderr, "   ❌ TUN setup failed\n");
        return -1;
    }
    
    printf("   ✅ Reconnected successfully!\n");
    return 0;
}

void handle_udp_to_tun(vpn_client_t *client) {
    uint8_t buffer[2048];
    uint8_t plaintext[2048];
    struct sockaddr_in recv_addr;
    socklen_t recv_len = sizeof(recv_addr);
    
    ssize_t n = recvfrom(client->sock_fd, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&recv_addr, &recv_len);
    
    if (n < 0) return;
    
    if (n < (ssize_t)sizeof(vpn_header_t)) {
        return;
    }
    
    vpn_header_t *header = (vpn_header_t*)buffer;
    
    switch (header->type) {
        case PKT_PONG: {
            client->last_pong_received = time(NULL);
            printf("🏓 PONG received\n");
            break;
        }
        
        case PKT_DATA: {
            printf("\n📥 Encrypted packet received (%zd bytes)\n", n);
            
            uint8_t *ciphertext = buffer + sizeof(vpn_header_t);
            size_t ciphertext_len = n - sizeof(vpn_header_t);
            
            printf("   🔓 Decrypting %zu bytes...\n", ciphertext_len);
            
            uint8_t *nonce = ciphertext;
            uint8_t *encrypted = ciphertext + CRYPTO_NONCE_SIZE;
            size_t encrypted_len = ciphertext_len - CRYPTO_NONCE_SIZE;
            
            if (crypto_decrypt(encrypted, encrypted_len, plaintext,
                             client->session_key, nonce) != 0) {
                printf("   ❌ Decryption failed\n");
                return;
            }
            
            size_t plaintext_len = encrypted_len - CRYPTO_MAC_SIZE;
            printf("   ✅ Decrypted to %zu bytes\n", plaintext_len);
            
            ssize_t written = write(client->tun_fd, plaintext, plaintext_len);
            if (written > 0) {
                printf("   → TUN: Written %zd bytes\n", written);
            }
            break;
        }
        
        default:
            break;
    }
}

void handle_tun_to_udp(vpn_client_t *client) {
    uint8_t buffer[2048];
    uint8_t ciphertext[2048];
    uint8_t packet_buffer[2048];
    
    ssize_t n = read(client->tun_fd, buffer, sizeof(buffer));
    
    if (n < 0) {
        perror("TUN read");
        return;
    }
    
    printf("\n📤 TUN packet captured (%zd bytes)\n", n);
    
    printf("   🔒 Encrypting...\n");
    
    uint8_t nonce[CRYPTO_NONCE_SIZE];
    crypto_random_nonce(nonce);
    
    if (crypto_encrypt(buffer, n, ciphertext + CRYPTO_NONCE_SIZE,
                      client->session_key, nonce) != 0) {
        printf("   ❌ Encryption failed\n");
        return;
    }
    
    memcpy(ciphertext, nonce, CRYPTO_NONCE_SIZE);
    size_t ciphertext_len = CRYPTO_NONCE_SIZE + n + CRYPTO_MAC_SIZE;
    
    printf("   ✅ Encrypted to %zu bytes\n", ciphertext_len);
    
    data_packet_t *pkt = (data_packet_t*)packet_buffer;
    init_vpn_header(&pkt->header, PKT_DATA, ciphertext_len);
    memcpy(pkt->data, ciphertext, ciphertext_len);
    
    size_t total_len = sizeof(vpn_header_t) + ciphertext_len;
    
    ssize_t sent = sendto(client->sock_fd, packet_buffer, total_len, 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent > 0) {
        printf("   → UDP: Sent %zd bytes to server\n", sent);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    
    printf("🔐 VPN Client (Keep-alive + Auto Reconnect)\n");
    printf("═══════════════════════════════════════\n\n");
    
    signal(SIGINT, client_signal_handler);
    signal(SIGTERM, client_signal_handler);
    
    vpn_client_t *client = init_vpn_client(server_ip, 51820);
    if (!client) {
        return 1;
    }
    
    // 초기 연결
    strncpy(client->username, "test_user", sizeof(client->username) - 1);
    
    sleep(1);
    
    if (vpn_connect(client, client->username) != 0) {
        destroy_vpn_client(client);
        return 1;
    }
    
    sleep(1);
    
    if (setup_client_tun(client) != 0) {
        destroy_vpn_client(client);
        return 1;
    }
    
    printf("✅ VPN Client is running!\n");
    printf("═══════════════════════════════════════\n");
    printf("🔄 Full duplex communication enabled\n");
    printf("🏓 Keep-alive: %d seconds\n", KEEPALIVE_INTERVAL);
    printf("⏱️  Timeout: %d seconds\n", PONG_TIMEOUT);
    printf("🔄 Auto reconnect: enabled (max %d attempts)\n", MAX_RECONNECT_ATTEMPTS);
    printf("⏳ Press Ctrl+C to disconnect...\n\n");
    
    while (client_running) {
        if (!client->connected) {
            // 재연결 시도
            if (attempt_reconnect(client) != 0) {
                fprintf(stderr, "💔 Reconnection failed, exiting...\n");
                break;
            }
        }
        
        int max_fd = (client->tun_fd > client->sock_fd) ? client->tun_fd : client->sock_fd;
        
        fd_set read_fds;
        struct timeval timeout = {1, 0};
        
        FD_ZERO(&read_fds);
        FD_SET(client->tun_fd, &read_fds);
        FD_SET(client->sock_fd, &read_fds);
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (client_running) {
                perror("select");
            }
            break;
        }
        
        // Keep-alive 체크
        if (check_keepalive(client) != 0) {
            fprintf(stderr, "💔 Connection lost\n");
            client->connected = 0;  // 재연결 플래그
            continue;
        }
        
        if (activity == 0) {
            continue;
        }
        
        if (FD_ISSET(client->sock_fd, &read_fds)) {
            handle_udp_to_tun(client);
        }
        
        if (FD_ISSET(client->tun_fd, &read_fds)) {
            handle_tun_to_udp(client);
        }
    }
    
    printf("\n🧹 Cleaning up...\n");
    destroy_vpn_client(client);
    
    printf("✅ VPN Client stopped.\n");
    
    return 0;
}
