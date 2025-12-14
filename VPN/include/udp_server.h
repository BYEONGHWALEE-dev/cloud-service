// include/udp_server.h

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <stdint.h>
#include <netinet/in.h>

// UDP 서버 생성
// port: 바인딩할 포트 번호
// 반환값: UDP 소켓 파일 디스크립터 (성공), -1 (실패)
int create_udp_server(uint16_t port);

// UDP 패킷 수신
// udp_fd: UDP 소켓 파일 디스크립터
// buffer: 수신 버퍼
// buffer_size: 버퍼 크기
// client_addr: 송신자 주소 (출력)
// 반환값: 수신한 바이트 수, -1 (실패)
ssize_t udp_recv(int udp_fd, uint8_t *buffer, size_t buffer_size,
                 struct sockaddr_in *client_addr);

// UDP 패킷 전송
// udp_fd: UDP 소켓 파일 디스크립터
// buffer: 전송 버퍼
// length: 전송할 바이트 수
// dest_addr: 목적지 주소
// 반환값: 전송한 바이트 수, -1 (실패)
ssize_t udp_send(int udp_fd, const uint8_t *buffer, size_t length,
                 const struct sockaddr_in *dest_addr);

#endif // UDP_SERVER_H
