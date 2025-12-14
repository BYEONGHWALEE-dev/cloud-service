// src/server/tun_test.c

#include "tun_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define TUN_DEVICE "tun0"
#define TUN_IP "10.8.0.1"
#define TUN_NETMASK 24

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n🛑 Shutting down...\n");
        running = 0;
    }
}

int main() {
    int tun_fd;
    uint8_t buffer[2048];
    ssize_t nread;
    int packet_count = 0;
    
    printf("🚀 VPN Server - TUN Interface Test\n");
    printf("=====================================\n\n");
    
    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 1. TUN 인터페이스 생성
    tun_fd = create_tun_interface(TUN_DEVICE);
    if (tun_fd < 0) {
        return 1;
    }
    
    // 2. IP 설정
    if (configure_tun_ip(TUN_DEVICE, TUN_IP, TUN_NETMASK) < 0) {
        close(tun_fd);
        return 1;
    }
    
    // 3. 인터페이스 UP
    if (bring_tun_up(TUN_DEVICE) < 0) {
        close(tun_fd);
        return 1;
    }
    
    printf("\n✅ TUN interface is ready!\n");
    printf("=====================================\n");
    printf("📡 Now you can test:\n");
    printf("   - Open another terminal\n");
    printf("   - Run: ping %s\n", TUN_IP);
    printf("   - Or: ping 10.8.0.5 (any IP in 10.8.0.0/24)\n");
    printf("=====================================\n\n");
    printf("⏳ Waiting for packets... (Press Ctrl+C to stop)\n\n");
    
    // 4. 패킷 수신 루프
    while (running) {
        nread = read(tun_fd, buffer, sizeof(buffer));
        
        if (nread < 0) {
            if (running) {  // SIGINT가 아니면 에러
                perror("❌ Error reading from TUN");
            }
            break;
        }
        
        if (nread == 0) {
            printf("⚠️  EOF on TUN device\n");
            break;
        }
        
        packet_count++;
        printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Packet #%d received:\n", packet_count);
        print_ip_packet(buffer, nread);
        
        // 옵션: 패킷을 다시 TUN에 써서 응답 (에코)
        // write(tun_fd, buffer, nread);
    }
    
    // 5. 정리
    printf("\n🧹 Cleaning up...\n");
    close(tun_fd);
    
    printf("📊 Statistics:\n");
    printf("   Total packets received: %d\n", packet_count);
    printf("\n✅ TUN test completed successfully!\n");
    
    return 0;
}
