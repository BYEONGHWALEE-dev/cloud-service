// src/server/test_enclave_ipc.c

#include "enclave_client.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int enclave_fd;
    
    printf("🧪 Enclave IPC Test\n");
    printf("═══════════════════════════════════\n\n");
    
    // Enclave 연결
    printf("1. Connecting to Enclave...\n");
    enclave_fd = enclave_connect();
    if (enclave_fd < 0) {
        fprintf(stderr, "Failed to connect. Is Enclave running?\n");
        fprintf(stderr, "Run: sudo ./bin/vpn_enclave\n");
        return 1;
    }
    
    sleep(1);
    
    // PING 테스트
    printf("\n2. PING Test...\n");
    if (enclave_ping(enclave_fd) == 0) {
        printf("   ✅ PING successful\n");
    }
    
    sleep(1);
    
    // 키 추가 테스트
    printf("\n3. Add Key Test...\n");
    uint32_t test_vpn_ip = inet_addr("10.8.0.5");
    uint8_t test_key[32] = {0xAB, 0xCD, 0xEF};  // 테스트 키
    
    if (enclave_add_key(enclave_fd, test_vpn_ip, test_key) == 0) {
        printf("   ✅ Key added\n");
    }
    
    sleep(1);
    
    // 암호화 테스트
    printf("\n4. Encrypt Test...\n");
    const char *plaintext = "Hello Enclave!";
    uint8_t ciphertext[256];
    size_t ciphertext_len;
    
    if (enclave_encrypt(enclave_fd, test_vpn_ip,
                       (uint8_t*)plaintext, strlen(plaintext),
                       ciphertext, &ciphertext_len) == 0) {
        printf("   ✅ Encrypted: %zu bytes\n", ciphertext_len);
        printf("   Ciphertext (hex): ");
        for (size_t i = 0; i < (ciphertext_len < 32 ? ciphertext_len : 32); i++) {
            printf("%02x", ciphertext[i]);
        }
        printf("...\n");
    }
    
    sleep(1);
    
    // 복호화 테스트
    printf("\n5. Decrypt Test...\n");
    uint8_t decrypted[256];
    size_t decrypted_len;
    
    if (enclave_decrypt(enclave_fd, test_vpn_ip,
                       ciphertext, ciphertext_len,
                       decrypted, &decrypted_len) == 0) {
        decrypted[decrypted_len] = '\0';
        printf("   ✅ Decrypted: %zu bytes\n", decrypted_len);
        printf("   Plaintext: %s\n", decrypted);
    }
    
    sleep(1);
    
    // 키 제거 테스트
    printf("\n6. Remove Key Test...\n");
    if (enclave_remove_key(enclave_fd, test_vpn_ip) == 0) {
        printf("   ✅ Key removed\n");
    }
    
    sleep(1);
    
    // 연결 종료
    printf("\n7. Disconnecting...\n");
    enclave_disconnect(enclave_fd);
    
    printf("\n═══════════════════════════════════\n");
    printf("✅ All tests completed!\n");
    
    return 0;
}
