// src/server/udp_server.c

#include "udp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// UDP 서버 생성
int create_udp_server(uint16_t port) {
    int udp_fd;
    struct sockaddr_in server_addr;
    
    // 1. UDP 소켓 생성
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("❌ Failed to create UDP socket");
        return -1;
    }
    
    // 2. 소켓 옵션 설정 (주소 재사용)
    int reuse = 1;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("⚠️  Warning: setsockopt SO_REUSEADDR failed");
    }
    
    // 3. 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스
    server_addr.sin_port = htons(port);
    
    // 4. 바인딩
    if (bind(udp_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("❌ Failed to bind UDP socket");
        fprintf(stderr, "   Hint: Port %u may be in use or requires sudo\n", port);
        close(udp_fd);
        return -1;
    }
    
    printf("✅ UDP server created and bound to port %u (fd=%d)\n", port, udp_fd);
    
    return udp_fd;
}

// UDP 패킷 수신
ssize_t udp_recv(int udp_fd, uint8_t *buffer, size_t buffer_size,
                 struct sockaddr_in *client_addr) {
    socklen_t addr_len = sizeof(*client_addr);
    
    ssize_t n = recvfrom(udp_fd, buffer, buffer_size, 0,
                         (struct sockaddr*)client_addr, &addr_len);
    
    if (n < 0) {
        perror("❌ UDP recvfrom failed");
        return -1;
    }
    
    return n;
}

// UDP 패킷 전송
ssize_t udp_send(int udp_fd, const uint8_t *buffer, size_t length,
                 const struct sockaddr_in *dest_addr) {
    ssize_t n = sendto(udp_fd, buffer, length, 0,
                       (const struct sockaddr*)dest_addr,
                       sizeof(*dest_addr));
    
    if (n < 0) {
        perror("❌ UDP sendto failed");
        return -1;
    }
    
    return n;
}
