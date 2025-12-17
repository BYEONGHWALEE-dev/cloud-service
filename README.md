#  Secure VPN System

End-to-End 암호화 기반 VPN 시스템으로, ChaCha20-Poly1305 AEAD와 Curve25519 ECDH를 사용한 안전한 네트워크 통신을 제공합니다.

---

## 목차

- [시스템 개요](#시스템-개요)
- [주요 기능](#주요-기능)
- [시스템 아키텍처](#시스템-아키텍처)
- [설치 방법](#설치-방법)
- [사용 방법](#사용-방법)
- [설정 파일](#설정-파일)
- [트러블슈팅](#트러블슈팅)

---

## 시스템 개요

본 VPN 시스템은 클라우드 환경에서 안전한 VM 간 통신을 제공하기 위해 개발되었습니다.

### 기술 스택

- **암호화:** ChaCha20-Poly1305 (AEAD)
- **키 교환:** Curve25519 ECDH
- **네트워크:** TUN 인터페이스, UDP 프로토콜
- **보안:** Secure Enclave 격리 환경
- **언어:** C (libsodium)
- **백엔드:** Spring Boot (Java)

---

## 주요 기능

**End-to-End 암호화**
- ChaCha20-Poly1305 AEAD로 모든 트래픽 암호화
- Curve25519 ECDH 키 교환

**Secure Enclave**
- 암호화 키를 격리된 프로세스에서 관리
- 메모리 보호 및 키 유출 방지

 **자동 재연결**
- 네트워크 끊김 시 자동 재연결
- 지수 백오프 알고리즘 (1초 ~ 60초)

 **Keep-alive**
- 30초마다 PING/PONG으로 연결 유지
- 60초 타임아웃 감지

 **동적 클라이언트 관리**
- VPN IP 자동 할당 (10.8.0.0/24)
- 최대 254개 클라이언트 동시 지원

 **Spring Backend 연동**
- 사용자 인증 (Spring API)
- VPN IP 자동 등록
- 웹 기반 VM 관리

---

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────────┐
│                   클라이언트                         │
│  ┌──────────────────────────────────────────────┐   │
│  │ VPN 클라이언트 (vpn_client)                   │   │
│  │ - TUN 인터페이스 (tun1: 10.8.0.x)            │   │
│  │ - 암호화/복호화                               │   │
│  │ - Keep-alive                                  │   │
│  └──────────────────────────────────────────────┘   │
└──────────────────────┬──────────────────────────────┘
                       │ UDP 51820 (암호화)
                       │ 인터넷
                       ↓
┌─────────────────────────────────────────────────────┐
│                  VPN 서버 (EC2)                      │
│  ┌──────────────────────────────────────────────┐   │
│  │ VPN 서버 (vpn_server)                         │   │
│  │ - TUN 인터페이스 (tun0: 10.8.0.1)            │   │
│  │ - UDP 서버 (0.0.0.0:51820)                   │   │
│  │ - 클라이언트 관리                             │   │
│  └──────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────┐   │
│  │ Secure Enclave (enclave)                      │   │
│  │ - 키 관리 (VPN IP → Session Key)             │   │
│  │ - 암호화/복호화 전담                          │   │
│  └──────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────┐   │
│  │ Spring Backend                                │   │
│  │ - 사용자 인증 (DB)                            │   │
│  │ - VPN IP 관리                                 │   │
│  │ - VM 생성/관리                                │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

---

## 설치 방법

### 서버 설치 (EC2 또는 Linux 서버)

#### 1. 의존성 설치

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential libsodium-dev libcurl4-openssl-dev git

# CentOS/RHEL
sudo yum install -y gcc make libsodium-devel libcurl-devel git
```

#### 2. 소스 코드 다운로드

```bash
git clone https://github.com/YOUR_USERNAME/VPN.git
cd VPN
```

#### 3. 빌드

```bash
make clean
make
```

#### 4. IP Forwarding 활성화

```bash
# 즉시 활성화
sudo sysctl -w net.ipv4.ip_forward=1

# 재부팅 후에도 유지
echo "net.ipv4.ip_forward=1" | sudo tee -a /etc/sysctl.conf
```

#### 5. 방화벽 설정

```bash
# iptables
sudo iptables -A FORWARD -i tun0 -o tun0 -j ACCEPT
sudo iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE

# 규칙 저장
sudo apt-get install -y iptables-persistent
sudo netfilter-persistent save
```

```bash
# AWS Security Group (콘솔에서 설정)
Type: Custom UDP
Port: 51820
Source: 0.0.0.0/0
```

#### 6. 서버 실행

```bash
# Spring API와 함께 실행 (인증 활성화)
sudo ./bin/vpn_server http://localhost:8080

# 또는 개발 모드 (인증 비활성화)
sudo ./bin/vpn_server
```

---

### 클라이언트 설치 (자동 설치 스크립트)

#### 1. 파일 다운로드

```bash
# 설치 스크립트 다운로드
curl -o install-vpn-client.sh http://YOUR_SERVER/api/vpn/install-script
chmod +x install-vpn-client.sh

# 설정 파일 다운로드 (Spring API에서 제공)
curl -o vpn_config.conf http://YOUR_SERVER/api/vpn/config/YOUR_USERNAME
```

#### 2. 설치 및 실행

```bash
./install-vpn-client.sh -c vpn_config.conf
```

스크립트가 자동으로:
- 필요한 라이브러리 설치 (libsodium, libcurl)
- 빌드 도구 설치 (gcc, make, git)
- 소스 다운로드 및 빌드
- VPN 클라이언트 실행

#### 3. 연결 확인

```bash
# VPN 인터페이스 확인
ip addr show tun1
# inet 10.8.0.x/24

# 서버 연결 테스트
ping 10.8.0.1

# 다른 클라이언트와 통신
ping 10.8.0.2
```

---

### 클라이언트 설치 (수동)

#### 1. 의존성 설치

```bash
sudo apt-get update
sudo apt-get install -y build-essential libsodium-dev libcurl4-openssl-dev git
```

#### 2. 소스 다운로드 및 빌드

```bash
git clone https://github.com/YOUR_USERNAME/VPN.git
cd VPN
make clean
make
```

#### 3. 설정 파일 생성

```bash
cat > vpn_config.conf << EOF
server_address=YOUR_SERVER_IP
server_port=51820
username=YOUR_USERNAME
auto_reconnect=1
max_reconnect_attempts=10
keepalive_interval=30
pong_timeout=60
log_level=INFO
EOF
```

#### 4. 클라이언트 실행

```bash
sudo ./bin/vpn_client --config vpn_config.conf
```

---

## 사용 방법

### 서버 관리

#### 서버 시작

```bash
# Spring API 인증 사용
sudo ./bin/vpn_server http://localhost:8080

# 인증 없이 (개발 모드)
sudo ./bin/vpn_server
```

#### 연결된 클라이언트 확인

```bash
# 서버 로그에서 확인
# 출력 예시:
# ➕ Client added:
#    VPN IP:     10.8.0.2
#    Username:   alice
```

#### 서버 중지

```bash
# Ctrl+C
# 또는
sudo pkill vpn_server
```

---

### 클라이언트 관리

#### 클라이언트 시작

```bash
sudo ./bin/vpn_client --config vpn_config.conf
```

#### 백그라운드 실행

```bash
sudo nohup ./bin/vpn_client --config vpn_config.conf > /var/log/vpn-client.log 2>&1 &
```

#### 클라이언트 중지

```bash
# Ctrl+C
# 또는
sudo pkill vpn_client
```

#### 로그 확인

```bash
# 포그라운드 실행 시: 터미널에 출력
# 백그라운드 실행 시:
tail -f /var/log/vpn-client.log
```

---

### VM Template 생성 (Proxmox)

VPN 클라이언트가 자동으로 시작되는 VM Template을 생성할 수 있습니다.

#### 1. 기본 VM 준비

```bash
# VM에 SSH 접속
ssh user@vm-ip

# VPN 클라이언트 설치
sudo mkdir -p /opt/vpn
sudo cp vpn_client /opt/vpn/
sudo chmod +x /opt/vpn/vpn_client
```

#### 2. Systemd 서비스 생성

```bash
sudo tee /etc/systemd/system/vpn-client.service > /dev/null << 'EOF'
[Unit]
Description=VPN Client Service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/vpn
ExecStartPre=/opt/vpn/setup-vpn.sh
ExecStart=/opt/vpn/vpn_client --config /opt/vpn/vpn_config.conf
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
```

#### 3. 초기화 스크립트

```bash
sudo tee /opt/vpn/setup-vpn.sh > /dev/null << 'EOF'
#!/bin/bash
VM_ID=$(hostname | grep -oP 'vm-\K\d+' || echo "unknown")
USERNAME="vm-${VM_ID}"
sed -i "s/TEMPLATE_USERNAME/${USERNAME}/" /opt/vpn/vpn_config.conf
echo "VPN username set to: ${USERNAME}"
EOF

sudo chmod +x /opt/vpn/setup-vpn.sh
```

#### 4. 설정 파일 템플릿

```bash
sudo tee /opt/vpn/vpn_config.conf > /dev/null << 'EOF'
server_address=YOUR_SERVER_IP
server_port=51820
username=TEMPLATE_USERNAME
auto_reconnect=1
max_reconnect_attempts=10
keepalive_interval=30
pong_timeout=60
log_level=INFO
EOF
```

#### 5. 서비스 활성화

```bash
sudo systemctl enable vpn-client
sudo systemctl daemon-reload
```

#### 6. 정리 및 Template 변환

```bash
# 로그 정리
sudo apt-get clean
sudo rm -rf /var/log/*.log
sudo rm -f /etc/machine-id
sudo truncate -s 0 /etc/machine-id

# 히스토리 정리
history -c
rm -f ~/.bash_history

# VM 종료
sudo shutdown -h now
```

#### 7. Proxmox에서 Template 변환

```bash
# Proxmox 호스트에서
qm template VM_ID

# 또는 Web UI에서
# VM 우클릭 → Convert to template
```

#### 8. Template에서 VM 생성

```bash
# CLI
qm clone TEMPLATE_ID NEW_VM_ID --name vm-NEW_VM_ID --full
qm start NEW_VM_ID

# Web UI
# Template 우클릭 → Clone
```

---

## 설정 파일

### 서버 설정

서버는 명령행 인자로 Spring API URL을 받습니다:

```bash
./bin/vpn_server [SPRING_API_URL]

# 예시:
./bin/vpn_server http://localhost:8080
```

### 클라이언트 설정 (vpn_config.conf)

```ini
# VPN 서버 주소
server_address=YOUR_SERVER_IP

# VPN 서버 포트
server_port=51820

# 사용자 이름 (Spring에 등록된 username)
username=alice

# 자동 재연결 활성화 (0=비활성화, 1=활성화)
auto_reconnect=1

# 최대 재연결 시도 횟수
max_reconnect_attempts=10

# Keep-alive 간격 (초)
keepalive_interval=30

# PONG 타임아웃 (초)
pong_timeout=60

# 로그 레벨 (ERROR, WARN, INFO, DEBUG)
log_level=INFO
```

---

## 네트워크 구성

### IP 주소 할당

- **서버:** 10.8.0.1/24
- **클라이언트:** 10.8.0.2 ~ 10.8.0.254
- **최대 클라이언트:** 254개

### 포트

- **VPN 서버:** UDP 51820
- **Spring API:** HTTP 8080 (선택)

---

## Spring API 연동

### 필수 API 엔드포인트

#### 1. 사용자 인증

```
GET /api/vpn/validate/{username}

Response (200 OK):
{
  "authorized": true,
  "username": "alice"
}

Response (403 Forbidden):
{
  "authorized": false,
  "message": "User not active"
}
```

#### 2. VPN IP 등록 (Webhook)

```
POST /api/vpn/webhook

Request:
{
  "event": "client_connected",
  "username": "alice",
  "vpnIp": "10.8.0.5"
}

Response (200 OK)
```

#### 3. 설정 파일 다운로드

```
GET /api/vpn/config/{username}

Response: vpn_config.conf 파일
```

#### 4. 설치 스크립트 다운로드

```
GET /api/vpn/install-script

Response: install-vpn-client.sh 파일
```

---

## 트러블슈팅

### 문제: 클라이언트가 연결되지 않음

**확인 사항:**

1. 서버가 실행 중인지 확인
```bash
ps aux | grep vpn_server
```

2. 방화벽 포트 열림 확인
```bash
# 서버에서
sudo netstat -tulpn | grep 51820
# 0.0.0.0:51820 이어야 함

# AWS Security Group 확인
# UDP 51820 포트가 열려있는지
```

3. 네트워크 연결 확인
```bash
# 클라이언트에서
nc -vuz SERVER_IP 51820
# Connection succeeded 이어야 함
```

4. 설정 파일 확인
```bash
cat vpn_config.conf
# server_address가 올바른지 확인
```

---

### 문제: 서버는 되는데 클라이언트끼리 통신 안 됨

**해결 방법:**

1. IP Forwarding 확인
```bash
cat /proc/sys/net/ipv4/ip_forward
# 1이어야 함

# 활성화
sudo sysctl -w net.ipv4.ip_forward=1
```

2. iptables 규칙 확인
```bash
sudo iptables -L FORWARD -v -n
# tun0 → tun0 ACCEPT 규칙이 있어야 함

# 추가
sudo iptables -A FORWARD -i tun0 -o tun0 -j ACCEPT
```

---

### 문제: 인증 실패 (User not authorized)

**확인 사항:**

1. Spring API 연결 확인
```bash
curl http://localhost:8080/api/vpn/validate/alice
# {"authorized":true} 응답이 와야 함
```

2. Spring DB에 사용자 등록 확인
```sql
SELECT * FROM vpn_users WHERE username = 'alice';
```

3. username 일치 확인
```bash
# 설정 파일
cat vpn_config.conf | grep username
# Spring DB의 username과 동일해야 함
```

---

### 문제: VPN 연결 후 인터넷 안 됨

**해결 방법:**

NAT 설정 추가:
```bash
sudo iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE
sudo netfilter-persistent save
```

---

### 문제: 빌드 실패

**libsodium 없음:**
```bash
sudo apt-get install -y libsodium-dev
```

**libcurl 없음:**
```bash
sudo apt-get install -y libcurl4-openssl-dev
```

**gcc 없음:**
```bash
sudo apt-get install -y build-essential
```

---

## 성능 최적화

### Keep-alive 간격 조정

네트워크 상황에 따라 조정:

```ini
# 안정적인 네트워크
keepalive_interval=60
pong_timeout=120

# 불안정한 네트워크
keepalive_interval=15
pong_timeout=30
```

### 로그 레벨

프로덕션에서는 INFO 또는 WARN 권장:

```ini
# 개발: 모든 로그
log_level=DEBUG

# 프로덕션: 중요한 로그만
log_level=INFO
```

---

## 보안 권고사항

1. **서버 보안**
   - 최소 권한 원칙으로 실행
   - 정기적인 보안 업데이트
   - 로그 모니터링

2. **키 관리**
   - 세션키는 메모리에만 존재 (디스크 저장 안 함)
   - Enclave 프로세스 격리
   - 주기적인 키 갱신 권장

3. **네트워크**
   - 방화벽으로 불필요한 포트 차단
   - Spring API는 localhost만 접근 권장
   - VPN 네트워크와 인터넷 분리

---
## 링크
https://carlota-used-averi.ngrok-free.dev

## 문의

문제가 있으면 GitHub Issues에 등록해주세요.
