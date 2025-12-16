// include/config.h

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef struct {
    char server_address[256];
    uint16_t server_port;
    char username[64];
    int auto_reconnect;
    int max_reconnect_attempts;
    int keepalive_interval;
    int pong_timeout;
    int log_level;  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG
} vpn_config_t;

// 기본 설정
vpn_config_t* config_create_default(void);

// 설정 파일 로드
int config_load_from_file(vpn_config_t *config, const char *filename);

// 설정 해제
void config_destroy(vpn_config_t *config);

// 설정 출력
void config_print(const vpn_config_t *config);

#endif // CONFIG_H
