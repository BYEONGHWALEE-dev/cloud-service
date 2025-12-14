# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I include
LDFLAGS = 

SRC_DIR = src
BUILD_DIR = bin
INCLUDE_DIR = include

.PHONY: all clean test

all: $(BUILD_DIR)/tun_test $(BUILD_DIR)/vpn_server $(BUILD_DIR)/udp_test_client

# TUN 테스트 (기존)
$(BUILD_DIR)/tun_test: $(SRC_DIR)/server/tun_test.c $(SRC_DIR)/server/tun_manager.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# VPN 서버 (메인)
$(BUILD_DIR)/vpn_server: $(SRC_DIR)/server/vpn_server.c \
                          $(SRC_DIR)/server/tun_manager.c \
                          $(SRC_DIR)/server/udp_server.c \
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

clean:
	rm -rf $(BUILD_DIR)/*
	@echo "🧹 Clean complete"

test:
	@echo "Run: sudo ./bin/vpn_server"
