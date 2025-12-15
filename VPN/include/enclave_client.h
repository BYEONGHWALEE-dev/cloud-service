// include/enclave_client.h

#ifndef ENCLAVE_CLIENT_H
#define ENCLAVE_CLIENT_H

#include <stdint.h>
#include <sys/types.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Enclave IPC 클라이언트
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// Enclave 연결
// 반환값: 소켓 fd (성공), -1 (실패)
int enclave_connect(void);

// Enclave 연결 종료
void enclave_disconnect(int enclave_fd);

// PING 요청 (연결 테스트)
int enclave_ping(int enclave_fd);

// 키 추가 (VPN IP → 세션키)
int enclave_add_key(int enclave_fd, uint32_t vpn_ip, const uint8_t *session_key);

// 키 제거
int enclave_remove_key(int enclave_fd, uint32_t vpn_ip);

// ECDH 핸드셰이크 (클라이언트 공개키 → 세션키)
// server_public_key: 서버 공개키 출력 (32 bytes)
// session_key: 생성된 세션키 출력 (32 bytes)
int enclave_handshake(int enclave_fd, uint32_t vpn_ip,
                      const uint8_t *client_public_key,
                      uint8_t *server_public_key,
                      uint8_t *session_key);

// 암호화 (평문 → 암호문)
// plaintext: 평문 데이터
// plaintext_len: 평문 길이
// ciphertext: 암호문 출력 버퍼 (최소 plaintext_len + 28)
// ciphertext_len: 암호문 길이 출력
// 반환값: 0 (성공), -1 (실패)
int enclave_encrypt(int enclave_fd, uint32_t vpn_ip,
                    const uint8_t *plaintext, size_t plaintext_len,
                    uint8_t *ciphertext, size_t *ciphertext_len);

// 복호화 (암호문 → 평문)
// ciphertext: 암호문 데이터 (nonce + encrypted + mac)
// ciphertext_len: 암호문 길이
// plaintext: 평문 출력 버퍼
// plaintext_len: 평문 길이 출력
// 반환값: 0 (성공), -1 (실패)
int enclave_decrypt(int enclave_fd, uint32_t vpn_ip,
                    const uint8_t *ciphertext, size_t ciphertext_len,
                    uint8_t *plaintext, size_t *plaintext_len);

// Enclave 종료 요청
int enclave_shutdown(int enclave_fd);

#endif // ENCLAVE_CLIENT_H
