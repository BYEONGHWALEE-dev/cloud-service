// src/enclave/crypto.c

#include "crypto.h"
#include <stdio.h>
#include <string.h>
#include <sodium.h>

// libsodium 초기화
int crypto_init(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "❌ libsodium initialization failed\n");
        return -1;
    }
    printf("✅ libsodium initialized\n");
    return 0;
}

// ChaCha20-Poly1305 암호화
int crypto_encrypt(const uint8_t *plaintext, size_t plaintext_len,
                   uint8_t *ciphertext, const uint8_t *key,
                   const uint8_t *nonce) {
    
    unsigned long long ciphertext_len;
    
    int ret = crypto_aead_chacha20poly1305_ietf_encrypt(
        ciphertext, &ciphertext_len,
        plaintext, plaintext_len,
        NULL, 0,  // 추가 인증 데이터 없음
        NULL,     // nsec (사용 안 함)
        nonce,
        key
    );
    
    if (ret != 0) {
        fprintf(stderr, "❌ Encryption failed\n");
        return -1;
    }
    
    return 0;
}

// ChaCha20-Poly1305 복호화
int crypto_decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                   uint8_t *plaintext, const uint8_t *key,
                   const uint8_t *nonce) {
    
    unsigned long long plaintext_len;
    
    int ret = crypto_aead_chacha20poly1305_ietf_decrypt(
        plaintext, &plaintext_len,
        NULL,     // nsec
        ciphertext, ciphertext_len,
        NULL, 0,  // 추가 인증 데이터
        nonce,
        key
    );
    
    if (ret != 0) {
        fprintf(stderr, "❌ Decryption failed (authentication error)\n");
        return -1;
    }
    
    return 0;
}

// Curve25519 키 쌍 생성
void crypto_generate_keypair(uint8_t *public_key, uint8_t *private_key) {
    crypto_box_keypair(public_key, private_key);
}

// ECDH 공유 비밀 계산
int crypto_ecdh(uint8_t *shared_secret,
                const uint8_t *my_private_key,
                const uint8_t *their_public_key) {
    
    //int ret = crypto_scalarmult(shared_secret, my_private_key, their_public_key);
    int ret = crypto_box_beforenm(shared_secret, their_public_key, my_private_key);

    if (ret != 0) {
        fprintf(stderr, "❌ ECDH failed\n");
        return -1;
    }
    
    return 0;
}

// 공유 비밀 → 세션키 (HKDF)
void crypto_derive_session_key(uint8_t *session_key,
                               const uint8_t *shared_secret,
                               const uint8_t *salt, size_t salt_len) {
    
    // KDF를 사용해 세션키 생성
    crypto_kdf_derive_from_key(
        session_key, CRYPTO_KEY_SIZE,
        1,  // subkey ID
        "VPN_SESS",  // context
        shared_secret
    );
    
    // salt가 있으면 XOR (추가 엔트로피)
    if (salt && salt_len > 0) {
        for (size_t i = 0; i < CRYPTO_KEY_SIZE && i < salt_len; i++) {
            session_key[i] ^= salt[i];
        }
    }
}

// 랜덤 nonce 생성
void crypto_random_nonce(uint8_t *nonce) {
    randombytes_buf(nonce, CRYPTO_NONCE_SIZE);
}

// 랜덤 키 생성
void crypto_random_key(uint8_t *key) {
    randombytes_buf(key, CRYPTO_KEY_SIZE);
}
