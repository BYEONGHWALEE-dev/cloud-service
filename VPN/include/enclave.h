// include/enclave.h

#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <stdint.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Enclave 프로세스 제어
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// Enclave 프로세스 시작
// 반환값: Enclave PID (성공), -1 (실패)
pid_t start_enclave_process(void);

// Enclave 프로세스 중지
void stop_enclave_process(pid_t enclave_pid);

// Enclave 프로세스가 실행 중인지 확인
int is_enclave_running(pid_t enclave_pid);

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 메모리 보안
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// 메모리 보안 설정 (mlock, madvise)
int setup_memory_security(void);

// Seccomp 시스템콜 필터 적용
int setup_seccomp_filter(void);

// 코어 덤프 비활성화
int disable_core_dumps(void);

#endif // ENCLAVE_H
