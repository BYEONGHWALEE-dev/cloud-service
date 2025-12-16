// src/common/config.c

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

vpn_config_t* config_create_default(void) {
    vpn_config_t *config = (vpn_config_t*)malloc(sizeof(vpn_config_t));
    if (!config) {
        return NULL;
    }
    
    // 기본값 설정
    strncpy(config->server_address, "127.0.0.1", sizeof(config->server_address) - 1);
    config->server_port = 51820;
    strncpy(config->username, "vpn_user", sizeof(config->username) - 1);
    config->auto_reconnect = 1;
    config->max_reconnect_attempts = 10;
    config->keepalive_interval = 30;
    config->pong_timeout = 60;
    config->log_level = 2;  // INFO
    
    return config;
}

void config_destroy(vpn_config_t *config) {
    if (config) {
        free(config);
    }
}

// 문자열 trim
static void trim(char *str) {
    char *start = str;
    char *end;
    
    // 앞 공백 제거
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) {
        *str = 0;
        return;
    }
    
    // 뒤 공백 제거
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    // 복사
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

int config_load_from_file(vpn_config_t *config, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    char line[512];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // 주석 제거
        char *comment = strchr(line, '#');
        if (comment) {
            *comment = '\0';
        }
        
        trim(line);
        
        // 빈 줄 무시
        if (strlen(line) == 0) {
            continue;
        }
        
        // key=value 파싱
        char *equals = strchr(line, '=');
        if (!equals) {
            fprintf(stderr, "Warning: Invalid line %d: %s\n", line_num, line);
            continue;
        }
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        trim(key);
        trim(value);
        
        // 설정 값 적용
        if (strcmp(key, "server_address") == 0) {
            strncpy(config->server_address, value, sizeof(config->server_address) - 1);
        } else if (strcmp(key, "server_port") == 0) {
            config->server_port = (uint16_t)atoi(value);
        } else if (strcmp(key, "username") == 0) {
            strncpy(config->username, value, sizeof(config->username) - 1);
        } else if (strcmp(key, "auto_reconnect") == 0) {
            config->auto_reconnect = atoi(value);
        } else if (strcmp(key, "max_reconnect_attempts") == 0) {
            config->max_reconnect_attempts = atoi(value);
        } else if (strcmp(key, "keepalive_interval") == 0) {
            config->keepalive_interval = atoi(value);
        } else if (strcmp(key, "pong_timeout") == 0) {
            config->pong_timeout = atoi(value);
        } else if (strcmp(key, "log_level") == 0) {
            if (strcmp(value, "ERROR") == 0) config->log_level = 0;
            else if (strcmp(value, "WARN") == 0) config->log_level = 1;
            else if (strcmp(value, "INFO") == 0) config->log_level = 2;
            else if (strcmp(value, "DEBUG") == 0) config->log_level = 3;
            else config->log_level = atoi(value);
        } else {
            fprintf(stderr, "Warning: Unknown key '%s' at line %d\n", key, line_num);
        }
    }
    
    fclose(file);
    return 0;
}

void config_print(const vpn_config_t *config) {
    printf("━━━ VPN Configuration ━━━\n");
    printf("  Server:              %s:%u\n", config->server_address, config->server_port);
    printf("  Username:            %s\n", config->username);
    printf("  Auto Reconnect:      %s\n", config->auto_reconnect ? "enabled" : "disabled");
    printf("  Max Reconnect:       %d attempts\n", config->max_reconnect_attempts);
    printf("  Keep-alive Interval: %d seconds\n", config->keepalive_interval);
    printf("  PONG Timeout:        %d seconds\n", config->pong_timeout);
    printf("  Log Level:           ");
    switch (config->log_level) {
        case 0: printf("ERROR\n"); break;
        case 1: printf("WARN\n"); break;
        case 2: printf("INFO\n"); break;
        case 3: printf("DEBUG\n"); break;
        default: printf("%d\n", config->log_level);
    }
    printf("═══════════════════════════════════════\n");
}
