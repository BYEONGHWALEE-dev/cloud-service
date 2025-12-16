// src/client/vpn_client.c

#include "protocol.h"
#include "crypto.h"
#include "tun_manager.h"
#include "config.h"
#include "logger.h"
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

#define INITIAL_BACKOFF 1
#define MAX_BACKOFF 60

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
    
    int reconnect_attempts;
    int backoff_seconds;
    char username[64];
    
    vpn_config_t *config;
} vpn_client_t;

volatile sig_atomic_t client_running = 1;

void client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        LOG_INFO("🛑 Client shutting down...");
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
    client->backoff_seconds = INITIAL_BACKOFF;
    
    if (crypto_init() != 0) {
        free(client);
        return NULL;
    }
    
    crypto_generate_keypair(client->client_public_key, client->client_private_key);
    
    LOG_INFO("✅ VPN Client initialized");
    LOG_INFO("   Server: %s:%u", server_ip, server_port);
    
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
            
            LOG_DEBUG("📤 DISCONNECT sent");
        }
        
        if (client->sock_fd >= 0) {
            close(client->sock_fd);
        }
        if (client->tun_fd >= 0) {
            close(client->tun_fd);
        }
        sodium_memzero(client, sizeof(vpn_client_t));
        free(client);
        LOG_INFO("🧹 VPN Client destroyed");
    }
}

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
    client->server_addr.sin_port = htons(client->config->server_port);
    
    if (inet_pton(AF_INET, client->config->server_address, &client->server_addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid server IP");
        close(client->sock_fd);
        client->sock_fd = -1;
        return -1;
    }
    
    return 0;
}

int vpn_connect(vpn_client_t *client, const char *username) {
    uint8_t buffer[2048];
    
    LOG_INFO("🔐 Connecting to VPN server...");
    
    if (recreate_socket(client) != 0) {
        return -1;
    }
    
    connect_request_t *req = (connect_request_t*)buffer;
    init_vpn_header(&req->header, PKT_CONNECT_REQ,
                    sizeof(connect_request_t) - sizeof(vpn_header_t));
    
    strncpy(req->username, username, sizeof(req->username) - 1);
    memcpy(req->auth_token, client->client_public_key, 32);
    
    LOG_DEBUG("   Sending CONNECT_REQ...");
    
    ssize_t sent = sendto(client->sock_fd, buffer, sizeof(connect_request_t), 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    LOG_DEBUG("   Waiting for CONNECT_RESP...");
    
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
        LOG_ERROR("   ❌ Unexpected response type");
        return -1;
    }
    
    if (resp->status != 0) {
        LOG_ERROR("   ❌ Connection failed");
        return -1;
    }
    
    client->vpn_ip = resp->vpn_ip;
    client->session_id = ntohl(resp->session_id);
    memcpy(client->server_public_key, resp->server_public_key, 32);
    
    struct in_addr vpn_addr;
    vpn_addr.s_addr = client->vpn_ip;
    
    LOG_INFO("   ✅ Connection SUCCESS!");
    LOG_INFO("   VPN IP: %s", inet_ntoa(vpn_addr));
    LOG_DEBUG("   Session ID: %u", client->session_id);
    
    LOG_DEBUG("   🔑 Generating session key (ECDH)...");
    
    uint8_t shared_secret[32];
    
    if (crypto_ecdh(shared_secret, client->client_private_key,
                   client->server_public_key) != 0) {
        LOG_ERROR("   ❌ ECDH failed");
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
    
    LOG_DEBUG("   ✅ Session key generated");
    
    client->connected = 1;
    client->reconnect_attempts = 0;
    client->backoff_seconds = INITIAL_BACKOFF;
    
    client->last_pong_received = time(NULL);
    client->last_ping_sent = time(NULL);
    
    return 0;
}

int setup_client_tun(vpn_client_t *client) {
    if (client->tun_fd >= 0) {
        LOG_DEBUG("━━━ Reusing TUN Interface ━━━");
        return 0;
    }
    
    LOG_INFO("━━━ Client TUN Interface ━━━");
    
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
        LOG_DEBUG("🏓 PING sent (keepalive)");
    }
}

int check_keepalive(vpn_client_t *client) {
    time_t now = time(NULL);
    
    if (now - client->last_pong_received > client->config->pong_timeout) {
        LOG_ERROR("❌ No PONG received for %d seconds", client->config->pong_timeout);
        LOG_ERROR("   Connection lost!");
        return -1;
    }
    
    if (now - client->last_ping_sent >= client->config->keepalive_interval) {
        send_ping(client);
    }
    
    return 0;
}

int attempt_reconnect(vpn_client_t *client) {
    if (client->reconnect_attempts >= client->config->max_reconnect_attempts) {
        LOG_ERROR("❌ Max reconnect attempts reached (%d)", client->config->max_reconnect_attempts);
        return -1;
    }
    
    client->reconnect_attempts++;
    
    LOG_WARN("🔄 Reconnecting... (attempt %d/%d)",
           client->reconnect_attempts, client->config->max_reconnect_attempts);
    LOG_INFO("   Waiting %d seconds (exponential backoff)...",
           client->backoff_seconds);
    
    sleep(client->backoff_seconds);
    
    client->backoff_seconds *= 2;
    if (client->backoff_seconds > MAX_BACKOFF) {
        client->backoff_seconds = MAX_BACKOFF;
    }
    
    client->connected = 0;
    
    if (vpn_connect(client, client->username) != 0) {
        LOG_ERROR("   ❌ Reconnection failed");
        return -1;
    }
    
    if (setup_client_tun(client) != 0) {
        LOG_ERROR("   ❌ TUN setup failed");
        return -1;
    }
    
    LOG_INFO("   ✅ Reconnected successfully!");
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
            LOG_DEBUG("🏓 PONG received");
            break;
        }
        
        case PKT_DATA: {
            LOG_DEBUG("📥 Encrypted packet received (%zd bytes)", n);
            
            uint8_t *ciphertext = buffer + sizeof(vpn_header_t);
            size_t ciphertext_len = n - sizeof(vpn_header_t);
            
            LOG_DEBUG("   🔓 Decrypting %zu bytes...", ciphertext_len);
            
            uint8_t *nonce = ciphertext;
            uint8_t *encrypted = ciphertext + CRYPTO_NONCE_SIZE;
            size_t encrypted_len = ciphertext_len - CRYPTO_NONCE_SIZE;
            
            if (crypto_decrypt(encrypted, encrypted_len, plaintext,
                             client->session_key, nonce) != 0) {
                LOG_ERROR("   ❌ Decryption failed");
                return;
            }
            
            size_t plaintext_len = encrypted_len - CRYPTO_MAC_SIZE;
            LOG_DEBUG("   ✅ Decrypted to %zu bytes", plaintext_len);
            
            ssize_t written = write(client->tun_fd, plaintext, plaintext_len);
            if (written > 0) {
                LOG_DEBUG("   → TUN: Written %zd bytes", written);
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
    
    LOG_DEBUG("📤 TUN packet captured (%zd bytes)", n);
    LOG_DEBUG("   🔒 Encrypting...");
    
    uint8_t nonce[CRYPTO_NONCE_SIZE];
    crypto_random_nonce(nonce);
    
    if (crypto_encrypt(buffer, n, ciphertext + CRYPTO_NONCE_SIZE,
                      client->session_key, nonce) != 0) {
        LOG_ERROR("   ❌ Encryption failed");
        return;
    }
    
    memcpy(ciphertext, nonce, CRYPTO_NONCE_SIZE);
    size_t ciphertext_len = CRYPTO_NONCE_SIZE + n + CRYPTO_MAC_SIZE;
    
    LOG_DEBUG("   ✅ Encrypted to %zu bytes", ciphertext_len);
    
    data_packet_t *pkt = (data_packet_t*)packet_buffer;
    init_vpn_header(&pkt->header, PKT_DATA, ciphertext_len);
    memcpy(pkt->data, ciphertext, ciphertext_len);
    
    size_t total_len = sizeof(vpn_header_t) + ciphertext_len;
    
    ssize_t sent = sendto(client->sock_fd, packet_buffer, total_len, 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent > 0) {
        LOG_DEBUG("   → UDP: Sent %zd bytes to server", sent);
    }
}

int main(int argc, char *argv[]) {
    const char *config_file = NULL;
    const char *server_ip_arg = NULL;
    
    // 인자 파싱
    if (argc == 2) {
        // ./vpn_client 127.0.0.1 (구버전 호환)
        server_ip_arg = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--config") == 0) {
        // ./vpn_client --config vpn.conf
        config_file = argv[2];
    } else {
        printf("Usage:\n");
        printf("  %s <server_ip>              (legacy mode)\n", argv[0]);
        printf("  %s --config <config_file>   (config mode)\n", argv[0]);
        return 1;
    }
    
    // 설정 로드
    vpn_config_t *config = config_create_default();
    if (!config) {
        fprintf(stderr, "Failed to create config\n");
        return 1;
    }
    
    if (config_file) {
        if (config_load_from_file(config, config_file) != 0) {
            fprintf(stderr, "Failed to load config from %s\n", config_file);
            config_destroy(config);
            return 1;
        }
    } else if (server_ip_arg) {
        // 구버전 호환: 명령행 인자로 서버 IP 설정
        strncpy(config->server_address, server_ip_arg, sizeof(config->server_address) - 1);
    }
    
    // 로그 레벨 설정
    log_set_level(config->log_level);
    
    LOG_INFO("🔐 VPN Client (Config + Logging)");
    LOG_INFO("═══════════════════════════════════════");
    
    config_print(config);
    
    signal(SIGINT, client_signal_handler);
    signal(SIGTERM, client_signal_handler);
    
    vpn_client_t *client = init_vpn_client(config->server_address, config->server_port);
    if (!client) {
        config_destroy(config);
        return 1;
    }
    
    client->config = config;
    
    strncpy(client->username, config->username, sizeof(client->username) - 1);
    
    sleep(1);
    
    if (vpn_connect(client, client->username) != 0) {
        destroy_vpn_client(client);
        config_destroy(config);
        return 1;
    }
    
    sleep(1);
    
    if (setup_client_tun(client) != 0) {
        destroy_vpn_client(client);
        config_destroy(config);
        return 1;
    }
    
    LOG_INFO("✅ VPN Client is running!");
    LOG_INFO("⏳ Press Ctrl+C to disconnect...");
    
    while (client_running) {
        if (!client->connected) {
            if (config->auto_reconnect) {
                if (attempt_reconnect(client) != 0) {
                    LOG_ERROR("💔 Reconnection failed, exiting...");
                    break;
                }
            } else {
                LOG_ERROR("💔 Connection lost, exiting...");
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
        
        if (check_keepalive(client) != 0) {
            LOG_ERROR("💔 Connection lost");
            client->connected = 0;
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
    
    LOG_INFO("🧹 Cleaning up...");
    destroy_vpn_client(client);
    config_destroy(config);
    
    LOG_INFO("✅ VPN Client stopped.");
    
    return 0;
}
