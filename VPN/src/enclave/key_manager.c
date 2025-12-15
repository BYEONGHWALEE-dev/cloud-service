ger(key_manager_t *km) {
	    if (km) {
		            // 민감한 데이터 제거
			    //         sodium_memzero(km, sizeof(key_manager_t));
			    //                 free(km);
			    //                         printf("🧹 Key manager destroyed\n");
			    //                             }
			    //                             }
			    //
			    //                             // 키 추가
			    //                             int add_key(key_manager_t *km, uint32_t vpn_ip, const uint8_t *session_key) {
			    //                                 // 빈 슬롯 찾기
			    //                                     int index = -1;
			    //                                         for (int i = 0; i < MAX_KEYS; i++) {
			    //                                                 if (!km->keys[i].active) {
			    //                                                             index = i;
			    //                                                                         break;
			    //                                                                                 }
			    //                                                                                     }
			    //                                                                                         
			    //                                                                                             if (index == -1) {
			    //                                                                                                     fprintf(stderr, "❌ Key table full\n");
			    //                                                                                                             return -1;
			    //                                                                                                                 }
			    //                                                                                                                     
			    //                                                                                                                         km->keys[index].vpn_ip = vpn_ip;
			    //                                                                                                                             memcpy(km->keys[index].session_key, session_key, 32);
			    //                                                                                                                                 km->keys[index].active = 1;
			    //                                                                                                                                     km->count++;
			    //                                                                                                                                         
			    //                                                                                                                                             struct in_addr addr;
			    //                                                                                                                                                 addr.s_addr = vpn_ip;
			    //                                                                                                                                                     printf("🔑 Key// src/enclave/key_manager.c

#include "key_manager.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// 키 관리자 초기화
key_manager_t* init_key_manager(void) {
    key_manager_t *km = (key_manager_t*)malloc(sizeof(key_manager_t));
    if (!km) {
        perror("malloc");
        return NULL;
    }
    
    memset(km, 0, sizeof(key_manager_t));
    km->count = 0;
    
    // 서버 키 쌍 생성
    crypto_generate_keypair(km->server_public_key, km->server_private_key);
    
    printf("✅ Key manager initialized\n");
    printf("   Server public key: ");
    for (int i = 0; i < 8; i++) {
        printf("%02x", km->server_public_key[i]);
    }
    printf("...\n");
    
    return km;
}

// 키 관리자 제거
void destroy_key added for %s\n", inet_ntoa(addr));
    
    return 0;
}

// 키 조회
const uint8_t* get_key(key_manager_t *km, uint32_t vpn_ip) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (km->keys[i].active && km->keys[i].vpn_ip == vpn_ip) {
            return km->keys[i].session_key;
        }
    }
    return NULL;
}

// 키 제거
void remove_key(key_manager_t *km, uint32_t vpn_ip) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (km->keys[i].active && km->keys[i].vpn_ip == vpn_ip) {
            sodium_memzero(km->keys[i].session_key, 32);
            km->keys[i].active = 0;
            km->count--;
            
            struct in_addr addr;
            addr.s_addr = vpn_ip;
            printf("🔓 Key removed for %s\n", inet_ntoa(addr));
            return;
        }
    }
}

// 서버 공개키 가져오기
void get_server_public_key(key_manager_t *km, uint8_t *public_key) {
    memcpy(public_key, km->server_public_key, 32);
}

// ECDH 핸드셰이크
int perform_handshake(key_manager_t *km, uint32_t vpn_ip,
                      const uint8_t *client_public_key,
                      uint8_t *session_key_out) {
    
    uint8_t shared_secret[32];
    
    // ECDH 계산
    if (crypto_ecdh(shared_secret, km->server_private_key, client_public_key) != 0) {
        return -1;
    }
    
    // 세션키 생성
    crypto_derive_session_key(session_key_out, shared_secret, NULL, 0);
    
    // 키 테이블에 추가
    add_key(km, vpn_ip, session_key_out);
    
    // 공유 비밀 제거
    sodium_memzero(shared_secret, 32);
    
    return 0;
}
