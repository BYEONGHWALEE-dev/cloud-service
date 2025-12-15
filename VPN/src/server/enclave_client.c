// src/server/enclave_client.c

#include "enclave_client.h"
#include "ipc_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

// Unix Socket 연결
int enclave_connect(void) {
    int sock_fd;
    struct sockaddr_un addr;
    
    // 소켓 생성
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // 주소 설정
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // 연결
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect to enclave");
        close(sock_fd);
        return -1;
    }
    
    printf("✅ Connected to Enclave (fd=%d)\n", sock_fd);
    
    return sock_fd;
}

// 연결 종료
void enclave_disconnect(int enclave_fd) {
    if (enclave_fd >= 0) {
        close(enclave_fd);
        printf("🔌 Disconnected from Enclave\n");
    }
}

// IPC 요청 전송 및 응답 수신 (내부 함수)
static int send_ipc_request(int enclave_fd, const ipc_request_t *req,
                            size_t req_len, ipc_response_t *resp,
                            size_t resp_max_len) {
    // 요청 전송
    ssize_t sent = send(enclave_fd, req, req_len, 0);
    if (sent != (ssize_t)req_len) {
        perror("send to enclave");
        return -1;
    }
    
    // 응답 수신 (헤더 먼저)
    ssize_t n = recv(enclave_fd, resp, sizeof(ipc_response_t), MSG_PEEK);
    if (n < (ssize_t)sizeof(ipc_response_t)) {
        perror("recv from enclave (header)");
        return -1;
    }
    
    // 전체 응답 수신
    uint16_t data_len = ntohs(resp->data_len);
    size_t total_len = sizeof(ipc_response_t) + data_len;
    
    if (total_len > resp_max_len) {
        fprintf(stderr, "Response too large: %zu > %zu\n", total_len, resp_max_len);
        return -1;
    }
    
    n = recv(enclave_fd, resp, total_len, 0);
    if (n != (ssize_t)total_len) {
        perror("recv from enclave (full)");
        return -1;
    }
    
    // 상태 확인
    if (resp->status != 0) {
        fprintf(stderr, "❌ Enclave returned error status: %d\n", resp->status);
        return -1;
    }
    
    return 0;
}

// PING
int enclave_ping(int enclave_fd) {
    uint8_t req_buffer[sizeof(ipc_request_t)];
    uint8_t resp_buffer[sizeof(ipc_response_t)];
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    init_ipc_request(req, IPC_PING, 0, NULL, 0);
    
    if (send_ipc_request(enclave_fd, req, sizeof(ipc_request_t),
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    printf("🏓 Enclave PING OK\n");
    return 0;
}

// 키 추가
int enclave_add_key(int enclave_fd, uint32_t vpn_ip, const uint8_t *session_key) {
    uint8_t req_buffer[sizeof(ipc_request_t) + sizeof(ipc_add_key_data_t)];
    uint8_t resp_buffer[sizeof(ipc_response_t)];
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    ipc_add_key_data_t key_data;
    memcpy(key_data.session_key, session_key, 32);
    
    init_ipc_request(req, IPC_ADD_KEY, vpn_ip,
                    (uint8_t*)&key_data, sizeof(key_data));
    
    size_t req_len = sizeof(ipc_request_t) + sizeof(ipc_add_key_data_t);
    
    if (send_ipc_request(enclave_fd, req, req_len,
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    struct in_addr addr;
    addr.s_addr = vpn_ip;
    printf("🔑 Key added to Enclave for %s\n", inet_ntoa(addr));
    
    return 0;
}

// 키 제거
int enclave_remove_key(int enclave_fd, uint32_t vpn_ip) {
    uint8_t req_buffer[sizeof(ipc_request_t)];
    uint8_t resp_buffer[sizeof(ipc_response_t)];
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    init_ipc_request(req, IPC_REMOVE_KEY, vpn_ip, NULL, 0);
    
    if (send_ipc_request(enclave_fd, req, sizeof(ipc_request_t),
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    struct in_addr addr;
    addr.s_addr = vpn_ip;
    printf("🔓 Key removed from Enclave for %s\n", inet_ntoa(addr));
    
    return 0;
}

// ECDH 핸드셰이크
int enclave_handshake(int enclave_fd, uint32_t vpn_ip,
                      const uint8_t *client_public_key,
                      uint8_t *server_public_key,
                      uint8_t *session_key) {
    uint8_t req_buffer[sizeof(ipc_request_t) + sizeof(ipc_handshake_data_t)];
    uint8_t resp_buffer[sizeof(ipc_response_t) + sizeof(ipc_handshake_response_t)];
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    ipc_handshake_data_t hs_data;
    memcpy(hs_data.client_public_key, client_public_key, 32);
    
    init_ipc_request(req, IPC_HANDSHAKE, vpn_ip,
                    (uint8_t*)&hs_data, sizeof(hs_data));
    
    size_t req_len = sizeof(ipc_request_t) + sizeof(ipc_handshake_data_t);
    
    if (send_ipc_request(enclave_fd, req, req_len,
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    // 응답 데이터 추출
    ipc_handshake_response_t *hs_resp = (ipc_handshake_response_t*)resp->data;
    
    memcpy(server_public_key, hs_resp->server_public_key, 32);
    memcpy(session_key, hs_resp->session_key, 32);
    
    struct in_addr addr;
    addr.s_addr = vpn_ip;
    printf("🤝 Handshake complete for %s\n", inet_ntoa(addr));
    
    return 0;
}

// 암호화
int enclave_encrypt(int enclave_fd, uint32_t vpn_ip,
                    const uint8_t *plaintext, size_t plaintext_len,
                    uint8_t *ciphertext, size_t *ciphertext_len) {
    uint8_t req_buffer[sizeof(ipc_request_t) + IPC_MAX_DATA_SIZE];
    uint8_t resp_buffer[sizeof(ipc_response_t) + IPC_MAX_DATA_SIZE];
    
    if (plaintext_len > IPC_MAX_DATA_SIZE) {
        fprintf(stderr, "Plaintext too large: %zu > %d\n", plaintext_len, IPC_MAX_DATA_SIZE);
        return -1;
    }
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    init_ipc_request(req, IPC_ENCRYPT, vpn_ip, plaintext, plaintext_len);
    
    size_t req_len = sizeof(ipc_request_t) + plaintext_len;
    
    if (send_ipc_request(enclave_fd, req, req_len,
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    // 암호문 복사
    *ciphertext_len = ntohs(resp->data_len);
    memcpy(ciphertext, resp->data, *ciphertext_len);
    
    return 0;
}

// 복호화
int enclave_decrypt(int enclave_fd, uint32_t vpn_ip,
                    const uint8_t *ciphertext, size_t ciphertext_len,
                    uint8_t *plaintext, size_t *plaintext_len) {
    uint8_t req_buffer[sizeof(ipc_request_t) + IPC_MAX_DATA_SIZE];
    uint8_t resp_buffer[sizeof(ipc_response_t) + IPC_MAX_DATA_SIZE];
    
    if (ciphertext_len > IPC_MAX_DATA_SIZE) {
        fprintf(stderr, "Ciphertext too large: %zu > %d\n", ciphertext_len, IPC_MAX_DATA_SIZE);
        return -1;
    }
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    init_ipc_request(req, IPC_DECRYPT, vpn_ip, ciphertext, ciphertext_len);
    
    size_t req_len = sizeof(ipc_request_t) + ciphertext_len;
    
    if (send_ipc_request(enclave_fd, req, req_len,
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    // 평문 복사
    *plaintext_len = ntohs(resp->data_len);
    memcpy(plaintext, resp->data, *plaintext_len);
    
    return 0;
}

// Enclave 종료
int enclave_shutdown(int enclave_fd) {
    uint8_t req_buffer[sizeof(ipc_request_t)];
    uint8_t resp_buffer[sizeof(ipc_response_t)];
    
    ipc_request_t *req = (ipc_request_t*)req_buffer;
    ipc_response_t *resp = (ipc_response_t*)resp_buffer;
    
    init_ipc_request(req, IPC_SHUTDOWN, 0, NULL, 0);
    
    if (send_ipc_request(enclave_fd, req, sizeof(ipc_request_t),
                        resp, sizeof(resp_buffer)) != 0) {
        return -1;
    }
    
    printf("🛑 Enclave shutdown requested\n");
    
    return 0;
}
