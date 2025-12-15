// include/key_manager.h

#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <stdint.h>

#define MAX_KEYS 256

// 키 엔트리
typedef struct {
    uint32_t vpn_ip;           // VPN IP (네트워크 바이트 오더)
    uint8_t session_key[32];   // 세션키
    int active;                // 활성 여부
} key_entry_t;

// 키 관리자
typedef struct {
    key_entry_t keys[MAX_KEYS];
    int count;
    uint8_t server_private_key[32];  // 서버 비밀키
    uint8_t server_public_key[32];   // 서버 공개키
} key_manager_t;

// 키 관리자 초기화
key_manager_t* init_key_manager(void);

// 키 관리자 제거
void destroy_key_manager(key_manager_t *km);

// 키 추가
int add_key(key_manager_t *km, uint32_t vpn_ip, const uint8_t *session_key);

// 키 조회
const uint8_t* get_key(key_manager_t *km, uint32_t vpn_ip);

// 키 제거
void remove_key(key_manager_t *km, uint32_t vpn_ip);

// 서버 공개키 가져오기
void get_server_public_key(key_manager_t *km, uint8_t *public_key);

// ECDH 핸드셰이크 수행
int perform_handshake(key_manager_t *km, uint32_t vpn_ip,
                      const uint8_t *client_public_key,
                      uint8_t *session_key_out);

#endif // KEY_MANAGER_H
