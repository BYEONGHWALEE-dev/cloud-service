// src/client/vpn_client.c

#include "protocol.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sodium.h>

typedef struct {
    int sock_fd;
    struct sockaddr_in server_addr;
    
    uint8_t client_private_key[32];
    uint8_t client_public_key[32];
    uint8_t server_public_key[32];
    uint8_t session_key[32];
    
    uint32_t vpn_ip;
    uint32_t session_id;
    int connected;
} vpn_client_t;

vpn_client_t* init_vpn_client(const char *server_ip, uint16_t server_port) {
    vpn_client_t *client = (vpn_client_t*)malloc(sizeof(vpn_client_t));
    if (!client) {
        perror("malloc");
        return NULL;
    }
    
    memset(client, 0, sizeof(vpn_client_t));
    
    if (crypto_init() != 0) {
        free(client);
        return NULL;
    }
    
    client->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->sock_fd < 0) {
        perror("socket");
        free(client);
        return NULL;
    }
    
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &client->server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP: %s\n", server_ip);
        close(client->sock_fd);
        free(client);
        return NULL;
    }
    
    crypto_generate_keypair(client->client_public_key, client->client_private_key);
    
    printf("✅ VPN Client initialized\n");
    printf("   Server: %s:%u\n", server_ip, server_port);
    printf("   Client public key: ");
    for (int i = 0; i < 8; i++) {
        printf("%02x", client->client_public_key[i]);
    }
    printf("...\n");
    
    return client;
}

void destroy_vpn_client(vpn_client_t *client) {
    if (client) {
        if (client->sock_fd >= 0) {
            close(client->sock_fd);
        }
        sodium_memzero(client, sizeof(vpn_client_t));
        free(client);
        printf("🧹 VPN Client destroyed\n");
    }
}

int vpn_connect(vpn_client_t *client, const char *username) {
    uint8_t buffer[2048];
    
    printf("\n🔐 Connecting to VPN server...\n");

    printf("   📤 Sending client public key: ");
    for (int i = 0; i < 8; i++) {
	    printf("%02x", client->client_public_key[i]);
    }
    printf("...\n");
    
    connect_request_t *req = (connect_request_t*)buffer;
    init_vpn_header(&req->header, PKT_CONNECT_REQ,
                    sizeof(connect_request_t) - sizeof(vpn_header_t));
    
    strncpy(req->username, username, sizeof(req->username) - 1);
    memcpy(req->auth_token, client->client_public_key, 32);
    
    printf("   Sending CONNECT_REQ...\n");
    printf("   Username: %s\n", req->username);
    
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
   
    printf("   📩 Received server public key: ");
    for (int i = 0; i < 8; i++) {
	    printf("%02x", client->server_public_key[i]);
    }
    printf("...\n");

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

    printf("   🔑 Shared Secret: ");
    for (int i = 0; i < 8; i++) {
	    printf("%02x", shared_secret[i]);
	}
    printf("...\n");

    crypto_kdf_derive_from_key(
        client->session_key,
        32,                 // key length
        1,                  // subkey ID (서버와 동일!)
        "VPN_SESS",         // context (서버와 동일!)
        shared_secret
    );
    
    sodium_memzero(shared_secret, 32);
    
    printf("   ✅ Session key generated\n");

    printf("   🔑 Session key (client): ");
    for (int i = 0; i < 8; i++) {
	    printf("%02x", client->session_key[i]);
    }
    printf("...\n");
    
    client->connected = 1;
    
    return 0;
}

int vpn_ping(vpn_client_t *client) {
    uint8_t buffer[sizeof(vpn_header_t)];
    
    vpn_header_t *ping = (vpn_header_t*)buffer;
    init_vpn_header(ping, PKT_PING, 0);
    
    ssize_t sent = sendto(client->sock_fd, buffer, sizeof(vpn_header_t), 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    printf("🏓 PING sent\n");
    
    return 0;
}

int vpn_send_data(vpn_client_t *client, const uint8_t *data, size_t data_len) {
    uint8_t buffer[2048];
    uint8_t ciphertext[2048];
    
    if (!client->connected) {
        fprintf(stderr, "❌ Not connected\n");
        return -1;
    }
    
    printf("\n📤 Sending encrypted data...\n");
    printf("   Plaintext: %zu bytes\n", data_len);
    
    uint8_t nonce[CRYPTO_NONCE_SIZE];
    crypto_random_nonce(nonce);
    
    if (crypto_encrypt(data, data_len,
                      ciphertext + CRYPTO_NONCE_SIZE,
                      client->session_key, nonce) != 0) {
        fprintf(stderr, "   ❌ Encryption failed\n");
        return -1;
    }
    
    memcpy(ciphertext, nonce, CRYPTO_NONCE_SIZE);
    size_t ciphertext_len = CRYPTO_NONCE_SIZE + data_len + CRYPTO_MAC_SIZE;
    
    printf("   Ciphertext: %zu bytes\n", ciphertext_len);
    
    data_packet_t *pkt = (data_packet_t*)buffer;
    init_vpn_header(&pkt->header, PKT_DATA, ciphertext_len);
    memcpy(pkt->data, ciphertext, ciphertext_len);
    
    size_t total_len = sizeof(vpn_header_t) + ciphertext_len;
    
    ssize_t sent = sendto(client->sock_fd, buffer, total_len, 0,
                          (struct sockaddr*)&client->server_addr,
                          sizeof(client->server_addr));
    
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    printf("   ✅ Sent %zd bytes (encrypted)\n", sent);
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    
    printf("🔐 VPN Client (with Encryption)\n");
    printf("═══════════════════════════════════════\n\n");
    
    vpn_client_t *client = init_vpn_client(server_ip, 51820);
    if (!client) {
        return 1;
    }
    
    sleep(1);
    
    if (vpn_connect(client, "test_user") != 0) {
        destroy_vpn_client(client);
        return 1;
    }
    
    sleep(1);
    
    printf("\n━━━ PING Test ━━━\n");
    vpn_ping(client);
    
    sleep(1);
    
    printf("\n━━━ Encrypted DATA Test ━━━\n");
    
    uint8_t fake_ip_packet[] = {
        0x45, 0x00, 0x00, 0x54,
        0x12, 0x34, 0x40, 0x00,
        0x40, 0x01, 0x00, 0x00,
        0x0a, 0x08, 0x00, 0x02,
        0xc0, 0xa8, 0x64, 0x0a,
    };
    
    vpn_send_data(client, fake_ip_packet, sizeof(fake_ip_packet));
    
    sleep(1);
    
    printf("\n═══════════════════════════════════════\n");
    printf("✅ All tests complete!\n");
    
    destroy_vpn_client(client);
    
    return 0;
}
