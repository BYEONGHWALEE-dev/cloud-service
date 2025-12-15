// src/server/client_manager.c

#include "client_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// 클라이언트 테이블 생성
client_table_t* init_client_table(void) {
    client_table_t *table = (client_table_t*)malloc(sizeof(client_table_t));
    if (!table) {
        perror("malloc failed");
        return NULL;
    }
    
    memset(table, 0, sizeof(client_table_t));
    table->count = 0;
    table->next_ip = 0x0a080002;  // 10.8.0.2 (호스트 바이트 오더)
    
    printf("✅ Client table initialized (capacity: %d)\n", MAX_CLIENTS);
    
    return table;
}

// 클라이언트 테이블 제거
void destroy_client_table(client_table_t *table) {
    if (table) {
        printf("🧹 Destroying client table (%d active clients)\n", table->count);
        free(table);
    }
}

// 세션 ID 생성 (간단한 랜덤)
uint32_t generate_session_id(void) {
    return (uint32_t)time(NULL) ^ (uint32_t)rand();
}

// 클라이언트 추가
uint32_t add_client(client_table_t *table, struct sockaddr_in *addr) {
    if (table->count >= MAX_CLIENTS) {
        fprintf(stderr, "❌ Client table full!\n");
        return 0;
    }
    
    // 이미 존재하는 클라이언트인지 확인
    client_entry_t *existing = find_client_by_addr(table, addr);
    if (existing) {
        printf("⚠️  Client already exists, updating activity\n");
        update_client_activity(existing);
        return existing->vpn_ip;
    }
    
    // VPN IP 할당 (10.8.0.2 ~ 10.8.0.255)
    uint32_t vpn_ip_host = table->next_ip;
    
    // 255를 넘으면 2부터 다시 시작
    if (vpn_ip_host > 0x0a0800ff) {
        vpn_ip_host = 0x0a080002;
    }
    
    uint32_t vpn_ip = htonl(vpn_ip_host);
    
    // 빈 슬롯 찾기
    int index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!table->clients[i].active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        fprintf(stderr, "❌ No available slot!\n");
        return 0;
    }
    
    // 클라이언트 정보 저장
    client_entry_t *client = &table->clients[index];
    client->vpn_ip = vpn_ip;
    client->real_addr = *addr;
    client->last_seen = time(NULL);
    client->session_id = generate_session_id();
    client->active = 1;
    
    table->count++;
    table->next_ip = vpn_ip_host + 1;
    
    printf("➕ Client added:\n");
    print_client_info(client);
    
    return vpn_ip;
}

// VPN IP로 클라이언트 찾기
client_entry_t* find_client_by_vpn_ip(client_table_t *table, uint32_t vpn_ip) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (table->clients[i].active && table->clients[i].vpn_ip == vpn_ip) {
            return &table->clients[i];
        }
    }
    return NULL;
}

// 실제 주소로 클라이언트 찾기
client_entry_t* find_client_by_addr(client_table_t *table, struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (table->clients[i].active &&
            table->clients[i].real_addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            table->clients[i].real_addr.sin_port == addr->sin_port) {
            return &table->clients[i];
        }
    }
    return NULL;
}

// 클라이언트 제거
void remove_client(client_table_t *table, uint32_t vpn_ip) {
    client_entry_t *client = find_client_by_vpn_ip(table, vpn_ip);
    if (client) {
        printf("➖ Client removed:\n");
        print_client_info(client);
        
        client->active = 0;
        table->count--;
    }
}

// 타임아웃된 클라이언트 제거
void check_client_timeouts(client_table_t *table) {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (table->clients[i].active) {
            if (now - table->clients[i].last_seen > CLIENT_TIMEOUT) {
                struct in_addr vpn_addr;
                vpn_addr.s_addr = table->clients[i].vpn_ip;
                
                printf("⏱️  Client timeout: %s\n", inet_ntoa(vpn_addr));
                
                table->clients[i].active = 0;
                table->count--;
            }
        }
    }
}

// 클라이언트 활동 갱신
void update_client_activity(client_entry_t *client) {
    client->last_seen = time(NULL);
}

// 클라이언트 정보 출력
void print_client_info(const client_entry_t *client) {
    struct in_addr vpn_addr, real_addr;
    vpn_addr.s_addr = client->vpn_ip;
    real_addr = client->real_addr.sin_addr;
    
    printf("   VPN IP:     %s\n", inet_ntoa(vpn_addr));
    printf("   Real Addr:  %s:%d\n", 
           inet_ntoa(real_addr), 
           ntohs(client->real_addr.sin_port));
    printf("   Session ID: %u\n", client->session_id);
    printf("   Last Seen:  %ld seconds ago\n", 
           time(NULL) - client->last_seen);
}

// 클라이언트 테이블 출력
void print_client_table(const client_table_t *table) {
    printf("\n━━━ Client Table ━━━\n");
    printf("Active Clients: %d / %d\n", table->count, MAX_CLIENTS);
    
    if (table->count == 0) {
        printf("(No active clients)\n");
        return;
    }
    
    printf("\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (table->clients[i].active) {
            printf("Client #%d:\n", i);
            print_client_info(&table->clients[i]);
            printf("\n");
        }
    }
    printf("━━━━━━━━━━━━━━━━━━━━\n\n");
}
