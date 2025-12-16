// include/protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VPN 프로토콜 정의
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 패킷 타입
#define PKT_CONNECT_REQ     0x01  // 클라이언트 → 서버: 연결 요청
#define PKT_CONNECT_RESP    0x02  // 서버 → 클라이언트: 연결 응답
#define PKT_DATA            0x03  // 데이터 패킷
#define PKT_PING            0x04  // Keep-alive
#define PKT_PONG            0x05  // Keep-alive 응답
#define PKT_DISCONNECT      0x06  // 연결 종료

// 프로토콜 버전
#define VPN_PROTOCOL_VERSION 0x01

// 패킷 헤더 (16 bytes)
#pragma pack(push, 1)
typedef struct {
    uint8_t  type;           // 패킷 타입
    uint8_t  version;        // 프로토콜 버전
    uint16_t length;         // 페이로드 길이
    uint32_t sequence;       // 시퀀스 번호
    uint64_t timestamp;      // 타임스탬프 (밀리초)
} vpn_header_t;
#pragma pack(pop)

// 연결 요청 패킷
#pragma pack(push, 1)
typedef struct {
    vpn_header_t header;
    char username[32];       // 사용자 이름
    uint8_t auth_token[32];  // 인증 토큰
} connect_request_t;
#pragma pack(pop)

// 연결 응답 패킷
#pragma pack(push, 1)
typedef struct {
    vpn_header_t header;
    uint8_t status;          // 0=성공, 1=실패
    uint32_t vpn_ip;         // 할당된 VPN IP (네트워크 바이트 오더)
    uint32_t session_id;     // 세션 ID
    uint8_t server_public_key[32];
} __attribute__((packed)) connect_response_t;
#pragma pack(pop)

// 데이터 패킷
#pragma pack(push, 1)
typedef struct {
    vpn_header_t header;
    uint8_t data[];          // 가변 길이 데이터
} data_packet_t;
#pragma pack(pop)

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 유틸리티 함수
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 패킷 타입 → 문자열
const char* packet_type_str(uint8_t type);

// 현재 타임스탬프 (밀리초)
uint64_t get_timestamp_ms(void);

// VPN 헤더 초기화
void init_vpn_header(vpn_header_t *header, uint8_t type, uint16_t length);

// 패킷 정보 출력
void print_vpn_packet(const vpn_header_t *header);

#endif // PROTOCOL_H
