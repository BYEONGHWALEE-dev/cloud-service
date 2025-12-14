# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I include
LDFLAGS = 

# 디렉토리
SRC_DIR = src
BUILD_DIR = bin
INCLUDE_DIR = include

# 타겟
.PHONY: all clean test

all: $(BUILD_DIR)/tun_test

# TUN 테스트 프로그램
$(BUILD_DIR)/tun_test: $(SRC_DIR)/server/tun_test.c $(SRC_DIR)/server/tun_manager.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $@"

# 테스트 실행
test: $(BUILD_DIR)/tun_test
	@echo "Running TUN interface test..."
	@echo "⚠️  Run with sudo: sudo ./bin/tun_test"

# 정리
clean:
	rm -rf $(BUILD_DIR)/*
	@echo "🧹 Clean complete"

# 도움말
help:
	@echo "VPN Server Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build all targets"
	@echo "  test    - Show test instructions"
	@echo "  clean   - Remove built files"
	@echo "  help    - Show this help"
