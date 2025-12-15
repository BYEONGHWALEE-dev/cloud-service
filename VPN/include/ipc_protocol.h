// include/ipc_protocol.h

#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Enclave IPC 프로토콜
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#define IPC_SOCKET_PATH "/tmp/vpn-enclave.sock"
#define IPC_MAX_DATA_SIZE 4096

// IPC 명령 타입
typedef enum {
    IPC_PING = 0x01,           // 연결 테스트
    IPC_ENCRYPT = 0x02,        // 평문 → 암호문
    IPC_DECRYPT = 0x03,        // 암호문 → 평문
    IPC_ADD_KEY = 0x04,        // 키 추가 (VPN IP → 세션키)
    IPC_REMOVE_KEY = 0x05,     // 키 제거
    IPC_HANDSHAKE = 0x06,      // ECDH 핸드셰이크
    IPC_SHUTDOWN = 0xFF,       // Enclave 종료
} ipc_command_t;

// IPC 요청 패킷
#pragma pack(push, 1)
typedef struct {
    uint8_t command;           // ipc_command_t
    uint32_t request_id;       // 요청 ID (응답 매칭용)
    uint32_t vpn_ip;           // 클라이언트 VPN IP (네트워크 바이트 오더)
    uint16_t data_len;         // 데이터 길이
    uint8_t data[];            // 가변 길이 데이터
} ipc_request_t;
#pragma pack(pop)

// IPC 응답 패킷
#pragma pack(push, 1)
typedef struct {
    uint32_t request_id;       // 요청 ID
    int8_t status;             // 0=성공, -1=실패
    uint16_t data_len;         // 응답 데이터 길이
    uint8_t data[];            // 가변 길이 응답 데이터
} ipc_response_t;
#pragma pack(pop)

// ADD_KEY 요청 데이터
#pragma pack(push, 1)
typedef struct {
    uint8_t session_key[32];   // ChaCha20-Poly1305 키
} ipc_add_key_data_t;
#pragma pack(pop)

// HANDSHAKE 요청 데이터
#pragma pack(push, 1)
typedef struct {
    uint8_t client_public_key[32];  // 클라이언트 공개키
} ipc_handshake_data_t;
#pragma pack(pop)

// HANDSHAKE 응답 데이터
#pragma pack(push, 1)
typedef struct {
    uint8_t server_public_key[32];  // 서버 공개키
    uint8_t session_key[32];         // 생성된 세션키
} ipc_handshake_response_t;
#pragma pack(pop)

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 헬퍼 함수
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// IPC 명령 → 문자열
const char* ipc_command_str(uint8_t command);

// IPC 요청 패킷 초기화
void init_ipc_request(ipc_request_t *req, uint8_t command, 
                      uint32_t vpn_ip, const uint8_t *data, uint16_t data_len);

// IPC 응답 패킷 초기화
void init_ipc_response(ipc_response_t *resp, uint32_t request_id,
                       int8_t status, const uint8_t *data, uint16_t data_len);

#endif // IPC_PROTOCOL_H
