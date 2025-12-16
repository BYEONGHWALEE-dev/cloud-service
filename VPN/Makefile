# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -I include
LDFLAGS = -lsodium

# 디렉토리
SRC_DIR = src
BUILD_DIR = bin
INCLUDE_DIR = include

# 타겟
.PHONY: all clean test help

all: $(BUILD_DIR)/tun_test \
     $(BUILD_DIR)/vpn_server \
     $(BUILD_DIR)/udp_test_client \
     $(BUILD_DIR)/vpn_enclave \
     $(BUILD_DIR)/test_enclave_ipc \
     $(BUILD_DIR)/vpn_client

# TUN 테스트 프로그램 (기존)
$(BUILD_DIR)/tun_test: $(SRC_DIR)/server/tun_test.c $(SRC_DIR)/server/tun_manager.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# VPN 서버 (메인)
$(BUILD_DIR)/vpn_server: $(SRC_DIR)/server/vpn_server.c \
                          $(SRC_DIR)/server/tun_manager.c \
                          $(SRC_DIR)/server/udp_server.c \
                          $(SRC_DIR)/server/client_manager.c \
                          $(SRC_DIR)/server/enclave.c \
                          $(SRC_DIR)/server/enclave_client.c \
                          $(SRC_DIR)/common/protocol.c \
                          $(SRC_DIR)/common/ipc_protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# VPN Enclave 프로세스 (NEW!)
$(BUILD_DIR)/vpn_enclave: $(SRC_DIR)/enclave/enclave_main.c \
                           $(SRC_DIR)/enclave/crypto.c \
                           $(SRC_DIR)/enclave/key_manager.c \
                           $(SRC_DIR)/common/ipc_protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# VPN 클라이언트 (암호화 지원)
$(BUILD_DIR)/vpn_client: $(SRC_DIR)/client/vpn_client.c \
                          $(SRC_DIR)/server/tun_manager.c \
                          $(SRC_DIR)/enclave/crypto.c \
                          $(SRC_DIR)/common/protocol.c \
                          $(SRC_DIR)/common/config.c \
                          $(SRC_DIR)/common/logger.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# UDP 테스트 클라이언트
$(BUILD_DIR)/udp_test_client: $(SRC_DIR)/client/udp_test_client.c \
                               $(SRC_DIR)/common/protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"
# Enclave IPC 테스트
$(BUILD_DIR)/test_enclave_ipc: $(SRC_DIR)/server/test_enclave_ipc.c \
                                $(SRC_DIR)/server/enclave_client.c \
                                $(SRC_DIR)/common/ipc_protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# 테스트 실행
test:
	@echo "VPN Server Test Commands:"
	@echo "  sudo ./bin/vpn_enclave         # Enclave 단독 실행"
	@echo "  sudo ./bin/vpn_server          # VPN 서버 (Enclave 자동 시작)"
	@echo "  ./bin/udp_test_client 127.0.0.1  # 테스트 클라이언트"

# 정리
clean:
	rm -rf $(BUILD_DIR)/*
	rm -f /tmp/vpn-enclave.sock
	@echo "🧹 Clean complete"

# 도움말
help:
	@echo "VPN Server Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build all targets"
	@echo "  vpn_enclave    - Build Enclave process only"
	@echo "  vpn_server     - Build VPN server"
	@echo "  test           - Show test commands"
	@echo "  clean          - Remove built files"
