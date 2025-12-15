# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -I include
LDFLAGS =

# 디렉토리
SRC_DIR = src
BUILD_DIR = bin
INCLUDE_DIR = include

# 타겟
.PHONY: all clean test help

all: $(BUILD_DIR)/tun_test $(BUILD_DIR)/vpn_server $(BUILD_DIR)/udp_test_client

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
                          $(SRC_DIR)/common/protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# UDP 테스트 클라이언트
$(BUILD_DIR)/udp_test_client: $(SRC_DIR)/client/udp_test_client.c \
                               $(SRC_DIR)/common/protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# 테스트 실행
test:
	@echo "VPN Server Test Commands:"
	@echo "  sudo ./bin/vpn_server          # VPN 서버 실행"
	@echo "  ./bin/udp_test_client 127.0.0.1  # 테스트 클라이언트"
	@echo "  sudo ./bin/tun_test              # TUN 테스트만"

# 정리
clean:
	rm -rf $(BUILD_DIR)/*
	@echo "🧹 Clean complete"

# 도움말
help:
	@echo "VPN Server Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build all targets"
	@echo "  tun_test       - Build TUN test only"
	@echo "  vpn_server     - Build VPN server"
	@echo "  udp_test_client - Build test client"
	@echo "  test           - Show test commands"
	@echo "  clean          - Remove built files"
	@echo "  help           - Show this help"
