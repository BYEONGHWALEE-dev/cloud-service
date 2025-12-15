// include/client_manager.h

#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <stdint.h>
#include <time.h>
#include <netinet/in.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 클라이언트 관리
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#define MAX_CLIENTS 254
#define CLIENT_TIMEOUT 300  // 5분 (초)

// 클라이언트 정보
typedef struct {
    uint32_t vpn_ip;              // VPN IP (네트워크 바이트 오더)
    struct sockaddr_in real_addr; // 실제 주소 (IP:포트)
    time_t last_seen;             // 마지막 통신 시간
    uint32_t session_id;          // 세션 ID
    int active;                   // 활성 상태 (1=활성, 0=비활성)
} client_entry_t;

// 클라이언트 테이블
typedef struct {
    client_entry_t clients[MAX_CLIENTS];
    int count;                    // 현재 활성 클라이언트 수
    uint32_t next_ip;             // 다음 할당할 VPN IP (호스트 바이트 오더)
} client_table_t;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 함수 선언
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 클라이언트 테이블 생성
client_table_t* init_client_table(void);

// 클라이언트 테이블 제거
void destroy_client_table(client_table_t *table);

// 클라이언트 추가 (VPN IP 자동 할당)
// 반환값: 할당된 VPN IP (네트워크 바이트 오더), 0 (실패)
uint32_t add_client(client_table_t *table, struct sockaddr_in *addr);

// VPN IP로 클라이언트 찾기
client_entry_t* find_client_by_vpn_ip(client_table_t *table, uint32_t vpn_ip);

// 실제 주소로 클라이언트 찾기
client_entry_t* find_client_by_addr(client_table_t *table, struct sockaddr_in *addr);

// 클라이언트 제거
void remove_client(client_table_t *table, uint32_t vpn_ip);

// 타임아웃된 클라이언트 제거
void check_client_timeouts(client_table_t *table);

// 클라이언트 마지막 통신 시간 갱신
void update_client_activity(client_entry_t *client);

// 세션 ID 생성
uint32_t generate_session_id(void);

// 클라이언트 정보 출력
void print_client_info(const client_entry_t *client);

// 클라이언트 테이블 출력 (디버깅용)
void print_client_table(const client_table_t *table);

#endif // CLIENT_MANAGER_H
