// include/crypto.h

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <sodium.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 암호화 상수
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#define CRYPTO_KEY_SIZE 32
#define CRYPTO_NONCE_SIZE 12
#define CRYPTO_MAC_SIZE 16

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ChaCha20-Poly1305 암호화/복호화
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 암호화
// plaintext: 평문 데이터
// plaintext_len: 평문 길이
// ciphertext: 암호문 출력 버퍼 (plaintext_len + CRYPTO_MAC_SIZE)
// key: 32-byte 암호화 키
// nonce: 12-byte nonce (매번 새로 생성)
// 반환값: 0 (성공), -1 (실패)
int crypto_encrypt(const uint8_t *plaintext, size_t plaintext_len,
                   uint8_t *ciphertext, const uint8_t *key,
                   const uint8_t *nonce);

// 복호화
// ciphertext: 암호문 데이터
// ciphertext_len: 암호문 길이 (plaintext_len + CRYPTO_MAC_SIZE)
// plaintext: 평문 출력 버퍼 (ciphertext_len - CRYPTO_MAC_SIZE)
// key: 32-byte 암호화 키
// nonce: 12-byte nonce
// 반환값: 0 (성공), -1 (실패)
int crypto_decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                   uint8_t *plaintext, const uint8_t *key,
                   const uint8_t *nonce);

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Curve25519 ECDH (키 교환)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 키 쌍 생성
// public_key: 32-byte 공개키 출력
// private_key: 32-byte 비밀키 출력
void crypto_generate_keypair(uint8_t *public_key, uint8_t *private_key);

// 공유 비밀 계산 (ECDH)
// shared_secret: 32-byte 공유 비밀 출력
// my_private_key: 내 비밀키
// their_public_key: 상대방 공개키
// 반환값: 0 (성공), -1 (실패)
int crypto_ecdh(uint8_t *shared_secret,
                const uint8_t *my_private_key,
                const uint8_t *their_public_key);

// 공유 비밀 → 세션키 (HKDF)
// session_key: 32-byte 세션키 출력
// shared_secret: ECDH 공유 비밀
// salt: 추가 salt (선택사항, NULL 가능)
// salt_len: salt 길이
void crypto_derive_session_key(uint8_t *session_key,
                               const uint8_t *shared_secret,
                               const uint8_t *salt, size_t salt_len);

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 유틸리티
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 랜덤 nonce 생성
void crypto_random_nonce(uint8_t *nonce);

// 랜덤 키 생성
void crypto_random_key(uint8_t *key);

// libsodium 초기화
int crypto_init(void);

#endif // CRYPTO_H
