// src/enclave/enclave_main.c

#include "enclave.h"
#include "crypto.h"
#include "key_manager.h"
#include "ipc_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <errno.h>

volatile sig_atomic_t enclave_running = 1;

void enclave_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n🛑 Enclave shutting down...\n");
        enclave_running = 0;
    }
}

// 메모리 보안 설정
int setup_memory_security(void) {
    // 메모리를 RAM에 고정 (swap 방지)
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("⚠️  mlockall failed (requires root)");
        // 경고만 하고 계속 진행
    } else {
        printf("✅ Memory locked (no swap)\n");
    }
    
    // 메모리 덤프 방지
    if (madvise(NULL, 0, MADV_DONTDUMP) == 0) {
        printf("✅ Memory dump disabled\n");
    }
    
    return 0;
}

// 코어 덤프 비활성화
int disable_core_dumps(void) {
    struct rlimit rl = {0, 0};
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        perror("⚠️  setrlimit RLIMIT_CORE failed");
        return -1;
    }
    printf("✅ Core dumps disabled\n");
    return 0;
}

// Seccomp 필터 (간단 버전)
int setup_seccomp_filter(void) {
    // TODO: 나중에 libseccomp 사용
    // 지금은 스킵 (복잡도 때문에)
    printf("⏸️  Seccomp filter skipped (implement later)\n");
    return 0;
}

// Unix Socket 서버 생성
int create_unix_socket_server(const char *socket_path) {
    int sock_fd;
    struct sockaddr_un addr;
    
    // 기존 소켓 파일 삭제
    unlink(socket_path);
    
    // 소켓 생성
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // 주소 설정
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    // 바인드
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }
    
    // 리슨
    if (listen(sock_fd, 5) < 0) {
        perror("listen");
        close(sock_fd);
        return -1;
    }
    
    printf("✅ Unix socket listening: %s\n", socket_path);
    
    return sock_fd;
}

// IPC 요청 처리
void handle_ipc_request(int client_fd, key_manager_t *km) {
    uint8_t request_buffer[sizeof(ipc_request_t) + IPC_MAX_DATA_SIZE];
    uint8_t response_buffer[sizeof(ipc_response_t) + IPC_MAX_DATA_SIZE];
    
    // 요청 수신 (헤더 먼저)
    ssize_t n = recv(client_fd, request_buffer, sizeof(ipc_request_t), MSG_PEEK);
    if (n < (ssize_t)sizeof(ipc_request_t)) {
        if (n == 0) {
            // 연결 종료
            return;
        }
        perror("recv header");
        return;
    }
    
    ipc_request_t *req = (ipc_request_t*)request_buffer;
    uint16_t data_len = ntohs(req->data_len);
    size_t total_len = sizeof(ipc_request_t) + data_len;
    
    // 전체 요청 수신
    n = recv(client_fd, request_buffer, total_len, 0);
    if (n != (ssize_t)total_len) {
        perror("recv full request");
        return;
    }
    
    printf("📥 IPC Request: %s (ID=%u, VPN IP=%08x, len=%u)\n",
           ipc_command_str(req->command),
           ntohl(req->request_id),
           ntohl(req->vpn_ip),
           data_len);
    
    // 응답 준비
    ipc_response_t *resp = (ipc_response_t*)response_buffer;
    resp->request_id = req->request_id;
    resp->status = 0;
    resp->data_len = 0;
    
    // 명령별 처리
    switch (req->command) {
        case IPC_PING: {
            printf("   → PONG\n");
            resp->status = 0;
            break;
        }
        
        case IPC_ADD_KEY: {
            if (data_len != sizeof(ipc_add_key_data_t)) {
                fprintf(stderr, "   ❌ Invalid data length\n");
                resp->status = -1;
                break;
            }
            
            ipc_add_key_data_t *key_data = (ipc_add_key_data_t*)req->data;
            
            if (add_key(km, req->vpn_ip, key_data->session_key) == 0) {
                printf("   → Key added\n");
                resp->status = 0;
            } else {
                fprintf(stderr, "   ❌ Failed to add key\n");
                resp->status = -1;
            }
            break;
        }
        
        case IPC_REMOVE_KEY: {
            remove_key(km, req->vpn_ip);
            printf("   → Key removed\n");
            resp->status = 0;
            break;
        }
        
        case IPC_ENCRYPT: {
            const uint8_t *key = get_key(km, req->vpn_ip);
            if (!key) {
                fprintf(stderr, "   ❌ Key not found\n");
                resp->status = -1;
                break;
            }
            
            // Nonce 생성
            uint8_t nonce[CRYPTO_NONCE_SIZE];
            crypto_random_nonce(nonce);
            
            // 암호화: nonce(12) + ciphertext(data_len + 16)
            uint8_t *output = resp->data;
            memcpy(output, nonce, CRYPTO_NONCE_SIZE);
            
            if (crypto_encrypt(req->data, data_len,
                              output + CRYPTO_NONCE_SIZE,
                              key, nonce) == 0) {
                resp->data_len = htons(CRYPTO_NONCE_SIZE + data_len + CRYPTO_MAC_SIZE);
                printf("   → Encrypted %u bytes\n", data_len);
                resp->status = 0;
            } else {
                fprintf(stderr, "   ❌ Encryption failed\n");
                resp->status = -1;
            }
            break;
        }
        
        case IPC_DECRYPT: {
            const uint8_t *key = get_key(km, req->vpn_ip);
            if (!key) {
                fprintf(stderr, "   ❌ Key not found\n");
                resp->status = -1;
                break;
            }
            
            if (data_len < CRYPTO_NONCE_SIZE + CRYPTO_MAC_SIZE) {
                fprintf(stderr, "   ❌ Data too short\n");
                resp->status = -1;
                break;
            }
            
            // nonce 추출
            uint8_t *nonce = req->data;
            uint8_t *ciphertext = req->data + CRYPTO_NONCE_SIZE;
            size_t ciphertext_len = data_len - CRYPTO_NONCE_SIZE;
            
            // 복호화
            if (crypto_decrypt(ciphertext, ciphertext_len,
                              resp->data, key, nonce) == 0) {
                resp->data_len = htons(ciphertext_len - CRYPTO_MAC_SIZE);
                printf("   → Decrypted %zu bytes\n", ciphertext_len - CRYPTO_MAC_SIZE);
                resp->status = 0;
            } else {
                fprintf(stderr, "   ❌ Decryption failed\n");
                resp->status = -1;
            }
            break;
        }
        
        case IPC_HANDSHAKE: {
            if (data_len != sizeof(ipc_handshake_data_t)) {
                fprintf(stderr, "   ❌ Invalid handshake data\n");
                resp->status = -1;
                break;
            }
            
            ipc_handshake_data_t *hs_data = (ipc_handshake_data_t*)req->data;
            ipc_handshake_response_t *hs_resp = (ipc_handshake_response_t*)resp->data;
            
            // 서버 공개키 제공
            get_server_public_key(km, hs_resp->server_public_key);
            
            // ECDH 핸드셰이크
            if (perform_handshake(km, req->vpn_ip,
                                 hs_data->client_public_key,
                                 hs_resp->session_key) == 0) {
                resp->data_len = htons(sizeof(ipc_handshake_response_t));
                printf("   → Handshake complete\n");
                resp->status = 0;
            } else {
                fprintf(stderr, "   ❌ Handshake failed\n");
                resp->status = -1;
            }
            break;
        }
        
        case IPC_SHUTDOWN: {
            printf("   → Shutdown requested\n");
            resp->status = 0;
            enclave_running = 0;
            break;
        }
        
        default: {
            fprintf(stderr, "   ❌ Unknown command: 0x%02x\n", req->command);
            resp->status = -1;
            break;
        }
    }
    
    // 응답 전송
    size_t response_len = sizeof(ipc_response_t) + ntohs(resp->data_len);
    send(client_fd, response_buffer, response_len, 0);
}

// Enclave 메인
int main(void) {
    int sock_fd, client_fd;
    key_manager_t *km = NULL;
    
    printf("🔐 VPN Enclave Process Starting...\n");
    printf("═══════════════════════════════════════\n\n");
    
    // 시그널 핸들러
    signal(SIGINT, enclave_signal_handler);
    signal(SIGTERM, enclave_signal_handler);
    
    // 1. 보안 설정
    printf("━━━ Security Setup ━━━\n");
    disable_core_dumps();
    setup_memory_security();
    setup_seccomp_filter();
    printf("\n");
    
    // 2. libsodium 초기화
    printf("━━━ Crypto Initialization ━━━\n");
    if (crypto_init() != 0) {
        return 1;
    }
    printf("\n");
    
    // 3. 키 관리자 초기화
    printf("━━━ Key Manager ━━━\n");
    km = init_key_manager();
    if (!km) {
        return 1;
    }
    printf("\n");
    
    // 4. Unix Socket 서버 생성
    printf("━━━ IPC Server ━━━\n");
    sock_fd = create_unix_socket_server(IPC_SOCKET_PATH);
    if (sock_fd < 0) {
        destroy_key_manager(km);
        return 1;
    }
    printf("\n");
    
    printf("✅ Enclave is ready!\n");
    printf("═══════════════════════════════════════\n");
    printf("⏳ Waiting for IPC connections...\n\n");
    
    // 5. 메인 루프
    while (enclave_running) {
        fd_set read_fds;
        struct timeval tv = {1, 0};  // 1초 타임아웃
        
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);
        
        int activity = select(sock_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity < 0) {
            if (enclave_running) {
                perror("select");
            }
            break;
        }
        
        if (activity == 0) {
            // 타임아웃
            continue;
        }
        
        // 새 연결 수락
        client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        printf("📞 Client connected (fd=%d)\n", client_fd);
        
        // 클라이언트 요청 처리 (동기)
        while (enclave_running) {
            fd_set client_fds;
            struct timeval client_tv = {5, 0};  // 5초 타임아웃
            
            FD_ZERO(&client_fds);
            FD_SET(client_fd, &client_fds);
            
            int ret = select(client_fd + 1, &client_fds, NULL, NULL, &client_tv);
            
            if (ret <= 0) {
                // 타임아웃 또는 에러
                break;
            }
            
            handle_ipc_request(client_fd, km);
        }
        
        close(client_fd);
        printf("📞 Client disconnected\n");
    }
    
    // 6. 정리
    printf("\n🧹 Cleaning up...\n");
    close(sock_fd);
    unlink(IPC_SOCKET_PATH);
    destroy_key_manager(km);
    
    printf("✅ Enclave stopped.\n");
    
    return 0;
}
