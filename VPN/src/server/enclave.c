// src/server/enclave.c

#include "enclave.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

// Enclave 프로세스 시작
pid_t start_enclave_process(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        // 자식 프로세스: Enclave 실행
        execl("./bin/vpn_enclave", "vpn_enclave", (char*)NULL);
        
        // exec 실패 시
        perror("execl");
        exit(1);
    }
    
    // 부모 프로세스
    printf("✅ Enclave process started (PID=%d)\n", pid);
    
    // Enclave가 준비될 때까지 대기 (간단히 2초)
    sleep(2);
    
    return pid;
}

// Enclave 프로세스 중지
void stop_enclave_process(pid_t enclave_pid) {
    if (enclave_pid <= 0) {
        return;
    }
    
    printf("🛑 Stopping Enclave process (PID=%d)...\n", enclave_pid);
    
    // SIGTERM 전송
    if (kill(enclave_pid, SIGTERM) == 0) {
        // 정상 종료 대기 (5초)
        int status;
        pid_t result = waitpid(enclave_pid, &status, WNOHANG);
        
        if (result == 0) {
            // 아직 종료 안 됨, 5초 대기
            sleep(5);
            result = waitpid(enclave_pid, &status, WNOHANG);
        }
        
        if (result == 0) {
            // 강제 종료
            printf("⚠️  Enclave not responding, sending SIGKILL\n");
            kill(enclave_pid, SIGKILL);
            waitpid(enclave_pid, &status, 0);
        }
        
        printf("✅ Enclave process stopped\n");
    } else {
        perror("kill");
    }
}

// Enclave 실행 중 확인
int is_enclave_running(pid_t enclave_pid) {
    if (enclave_pid <= 0) {
        return 0;
    }
    
    // kill(pid, 0)은 시그널을 보내지 않고 프로세스 존재만 확인
    if (kill(enclave_pid, 0) == 0) {
        return 1;  // 실행 중
    }
    
    if (errno == ESRCH) {
        return 0;  // 프로세스 없음
    }
    
    return 0;
}
