// src/server/tun_manager.c

#include "tun_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

// TUN 인터페이스 생성
int create_tun_interface(const char *dev_name) {
    int tun_fd, err;
    struct ifreq ifr;
    
    // 1. /dev/net/tun 열기
    tun_fd = open("/dev/net/tun", O_RDWR);
    if (tun_fd < 0) {
        perror("Failed to open /dev/net/tun");
        fprintf(stderr, "   Hint: Run with sudo or check if TUN module is loaded\n");
        return -1;
    }
    
    // 2. TUN 설정
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  // TUN 모드, 패킷 정보 헤더 제거
    
    if (dev_name) {
        strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);
    }
    
    // 3. ioctl로 TUN 생성
    err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr);
    if (err < 0) {
        perror("Failed to create TUN interface (ioctl TUNSETIFF)");
        close(tun_fd);
        return -1;
    }
    
    printf("TUN interface '%s' created successfully (fd=%d)\n", 
           ifr.ifr_name, tun_fd);
    
    return tun_fd;
}

// TUN IP 설정
int configure_tun_ip(const char *dev, const char *ip, int netmask) {
    char cmd[256];
    int ret;
    
    // ip addr add 10.8.0.1/24 dev tun0
    snprintf(cmd, sizeof(cmd), "ip addr add %s/%d dev %s", ip, netmask, dev);
    
    printf("🔧 Configuring IP: %s\n", cmd);
    ret = system(cmd);
    
    if (ret != 0) {
        fprintf(stderr, "Failed to configure IP address\n");
        return -1;
    }
    
    printf("IP configured: %s/%d\n", ip, netmask);
    return 0;
}

// TUN 인터페이스 UP
int bring_tun_up(const char *dev) {
    char cmd[256];
    int ret;
    
    // ip link set tun0 up
    snprintf(cmd, sizeof(cmd), "ip link set %s up", dev);
    
    printf("🔧 Bringing interface up: %s\n", cmd);
    ret = system(cmd);
    
    if (ret != 0) {
        fprintf(stderr, "❌ Failed to bring interface up\n");
        return -1;
    }
    
    printf("Interface is UP\n");
    return 0;
}

// IP 패킷 정보 출력
void print_ip_packet(const uint8_t *packet, ssize_t len) {
    if (len <(ssize_t)sizeof(struct iphdr)) {
        printf("Packet too short (%zd bytes)\n", len);
        return;
    }
    
    struct iphdr *ip = (struct iphdr *)packet;
    
    // IP 주소 변환
    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = ip->saddr;
    dst_addr.s_addr = ip->daddr;
    
    printf("IP Packet (%zd bytes):\n", len);
    printf("   Version: %d\n", ip->version);
    printf("   Protocol: %d ", ip->protocol);
    
    switch (ip->protocol) {
        case 1:  printf("(ICMP)\n"); break;
        case 6:  printf("(TCP)\n"); break;
        case 17: printf("(UDP)\n"); break;
        default: printf("(Other)\n"); break;
    }
    
    printf("   Src: %s\n", inet_ntoa(src_addr));
    printf("   Dst: %s\n", inet_ntoa(dst_addr));
    printf("   TTL: %d\n", ip->ttl);
    
    // 처음 32바이트를 16진수로 출력
    printf("   Hex: ");
    for (int i = 0; i < len && i < 32; i++) {
        printf("%02x ", packet[i]);
        if ((i + 1) % 16 == 0) printf("\n        ");
    }
    printf("\n");
}
