package com.computer_architecture.cloudservice.domain.vpn.controller;

import com.computer_architecture.cloudservice.domain.member.service.MemberService;
import com.computer_architecture.cloudservice.domain.member.dto.MemberResponseDto;
import com.computer_architecture.cloudservice.global.apiPayload.ApiResponse;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.SuccessStatus;
import com.computer_architecture.cloudservice.infra.vpn.VpnApiService;
import lombok.RequiredArgsConstructor;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/vpn")
@RequiredArgsConstructor
public class VpnController {

    private final VpnApiService vpnApiService;
    private final MemberService memberService;

    /**
     * VPN 설정파일 다운로드 (studentNumber를 username으로 사용)
     */
    @GetMapping("/config/{memberId}")
    public ResponseEntity<String> downloadVpnConfig(@PathVariable Long memberId) {
        MemberResponseDto.MemberInfo member = memberService.getMemberInfo(memberId);
        String studentNumber = member.getStudentNumber();

        // VPN 설정 파일 생성 (studentNumber를 username으로)
        String configData = generateVpnConfig(studentNumber);

        return ResponseEntity.ok()
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=vpn_config.conf")
                .contentType(MediaType.TEXT_PLAIN)
                .body(configData);
    }

    /**
     * VPN 설치 스크립트 다운로드
     */
    @GetMapping("/install-script")
    public ResponseEntity<Resource> downloadInstallScript() {
        String script = getInstallScript();
        ByteArrayResource resource = new ByteArrayResource(script.getBytes());

        return ResponseEntity.ok()
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=install-vpn-client.sh")
                .contentType(MediaType.TEXT_PLAIN)
                .contentLength(script.getBytes().length)
                .body(resource);
    }

    /**
     * VPN 클라이언트 사용 가이드 조회
     */
    @GetMapping("/guide")
    public ApiResponse<String> getVpnGuide() {
        String guide = "VPN 클라이언트 설치 가이드\n\n" +
                "1. 'VPN 설치 스크립트' 다운로드\n" +
                "2. 'VPN 설정 파일' 다운로드\n" +
                "3. 두 파일을 같은 디렉토리에 저장\n" +
                "4. 터미널에서 실행:\n" +
                "   chmod +x install-vpn-client.sh\n" +
                "   ./install-vpn-client.sh -c vpn_config.conf\n\n" +
                "5. VPN 연결 후 VM에 SSH 접속 가능";

        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VPN200", "가이드 조회 성공"),
                guide
        );
    }

    /**
     * VPN 연결 상태 확인 (TODO: VPN API 연동 후 구현)
     */
    @GetMapping("/status/{memberId}")
    public ApiResponse<String> getVpnStatus(@PathVariable Long memberId) {
        // TODO: VPN 서버에서 연결 상태 조회
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VPN200", "VPN 상태 조회"),
                "connected"  // 임시
        );
    }

    /**
     * VPN 설정 파일 생성 (studentNumber를 username으로 사용)
     */
    private String generateVpnConfig(String studentNumber) {
        return String.format("""
            # VPN Client Configuration
            # 서버 설정
            server_address=3.36.128.179
            server_port=51820
            # 인증
            username=%s
            # 재연결 설정
            auto_reconnect=1
            max_reconnect_attempts=10
            # Keep-alive 설정
            keepalive_interval=30
            pong_timeout=60
            # 로그 레벨 (ERROR, WARN, INFO, DEBUG)
            log_level=INFO
            """, studentNumber);
    }

    /**
     * VPN 설치 스크립트 내용
     */
    private String getInstallScript() {
        return """
                #!/bin/bash
                # install-vpn-client.sh
                # VPN 클라이언트 자동 설치 스크립트
                
                set -e  # 에러 발생 시 중단
                
                CONFIG_FILE=""
                VPN_REPO_DIR="cloud-service" # Git 레포지토리 이름
                VPN_CLIENT_DIR="${VPN_REPO_DIR}/VPN" # 클라이언트 소스 코드 위치
                
                # 색상 코드
                RED='\\033[0;31m'
                GREEN='\\033[0;32m'
                YELLOW='\\033[1;33m'
                NC='\\033[0m' # No Color
                
                # 도움말
                usage() {
                    echo "Usage: $0 -c <config_file>"
                    echo ""
                    echo "Options:"
                    echo "  -c <config_file>    VPN 설정 파일 경로 (필수)"
                    echo ""
                    echo "Example:"
                    echo "  $0 -c vpn_config.conf"
                    exit 1
                }
                
                # 인자 파싱
                while getopts "c:h" opt; do
                    case $opt in
                        c) CONFIG_FILE="$OPTARG" ;;
                        h) usage ;;
                        *) usage ;;
                    esac
                done
                
                # 설정 파일 확인
                if [ -z "$CONFIG_FILE" ]; then
                    echo -e "${RED}❌ Error: 설정 파일이 지정되지 않았습니다.${NC}"
                    usage
                fi
                
                # 원본 설정 파일의 절대 경로 저장 (5단계에서 사용)
                CONFIG_ABSOLUTE_ORIGINAL=$(realpath "$CONFIG_FILE")
                
                if [ ! -f "$CONFIG_ABSOLUTE_ORIGINAL" ]; then
                    echo -e "${RED}❌ Error: 설정 파일을 찾을 수 없습니다: $CONFIG_FILE${NC}"
                    exit 1
                fi
                
                echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
                echo -e "${GREEN}🔐 VPN 클라이언트 설치 시작${NC}"
                echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
                echo ""
                
                # 1. 시스템 업데이트
                echo -e "${YELLOW}[1/6] 시스템 업데이트...${NC}"
                sudo apt-get update -qq
                
                # 2. 빌드 도구 설치
                echo -e "${YELLOW}[2/6] 빌드 도구 확인...${NC}"
                if ! command -v gcc &> /dev/null; then
                    echo "    → gcc 설치 중..."
                    sudo apt-get install -y build-essential
                else
                    echo "    ✅ gcc 이미 설치됨"
                fi
                
                if ! command -v make &> /dev/null; then
                    echo "    → make 설치 중..."
                    sudo apt-get install -y make
                else
                    echo "    ✅ make 이미 설치됨"
                fi
                
                if ! command -v git &> /dev/null; then
                    echo "    → git 설치 중..."
                    sudo apt-get install -y git
                else
                    echo "    ✅ git 이미 설치됨"
                fi
                
                # 3. libsodium 설치
                echo -e "${YELLOW}[3/6] libsodium 확인...${NC}"
                if pkg-config --exists libsodium; then
                    VERSION=$(pkg-config --modversion libsodium)
                    echo "    ✅ libsodium 이미 설치됨 (버전: $VERSION)"
                else
                    echo "    → libsodium 설치 중..."
                    # libsodium-dev가 없으면 설치
                    sudo apt-get install -y libsodium-dev
                    echo "    ✅ libsodium 설치 완료"
                fi
                
                # 4. libcurl 설치
                echo -e "${YELLOW}[4/6] libcurl 확인...${NC}"
                if dpkg -l | grep -q libcurl4-openssl-dev; then
                    echo "    ✅ libcurl 이미 설치됨"
                else
                    echo "    → libcurl 설치 중..."
                    sudo apt-get install -y libcurl4-openssl-dev
                    echo "    ✅ libcurl 설치 완료"
                fi
                
                # 5. 소스 코드 다운로드 및 빌드
                echo -e "${YELLOW}[5/6] VPN 클라이언트 빌드...${NC}"
                
                # 현재 디렉토리에 레포지토리 폴더가 있는지 확인
                if [ -d "$VPN_REPO_DIR" ]; then
                    echo "    ⚠️  $VPN_REPO_DIR 디렉토리가 이미 존재합니다."
                    read -p "    기존 디렉토리를 삭제하고 새로 다운로드하시겠습니까? (y/N): " -n 1 -r
                    echo
                    if [[ $REPLY =~ ^[Yy]$ ]]; then
                        rm -rf "$VPN_REPO_DIR"
                        echo "    → 기존 디렉토리 삭제됨"
                    else
                        echo "    → 기존 디렉토리 사용"
                    fi
                fi
                
                # Git clone (없으면)
                if [ ! -d "$VPN_REPO_DIR" ]; then
                    echo "    → 소스 코드 다운로드 중..."
                    git clone https://github.com/BYEONGHWALEE-dev/cloud-service.git
                    echo "    ✅ 소스 코드 다운로드 완료"
                fi
                
                # 빌드
                cd "$VPN_CLIENT_DIR"
                echo "    → 빌드 시작..."
                # 빌드 전, makefile이 있는지 확인 (실패 방지)
                if [ ! -f "Makefile" ]; then
                    echo -e "${RED}❌ Error: ${VPN_CLIENT_DIR} 내에 Makefile을 찾을 수 없습니다.${NC}"
                    exit 1
                fi
                
                make clean > /dev/null 2>&1 || true
                make
                
                if [ ! -f "bin/vpn_client" ]; then
                    echo -e "${RED}❌ Error: 빌드 실패${NC}"
                    exit 1
                fi
                
                echo "    ✅ 빌드 완료"
                
                # 6. 설정 파일 복사 및 실행
                echo -e "${YELLOW}[6/6] VPN 클라이언트 실행 준비...${NC}"
                
                # 원본 설정 파일 (CONFIG_ABSOLUTE_ORIGINAL)을 빌드된 디렉토리로 복사
                # 클라이언트 실행 파일 (bin/vpn_client)이 있는 곳으로 설정 파일을 복사하는 것이 일반적
                # 여기서는 클라이언트 실행 파일이 있는 bin 디렉토리의 부모 디렉토리 (cloud-service/VPN)로 복사
                CONFIG_FILE_NAME=$(basename "$CONFIG_ABSOLUTE_ORIGINAL")
                CONFIG_DEST_PATH="./$CONFIG_FILE_NAME" # cloud-service/VPN/vpn_config.conf
                
                echo "    → 설정 파일 복사: ${CONFIG_ABSOLUTE_ORIGINAL} -> ${CONFIG_DEST_PATH}"
                cp "$CONFIG_ABSOLUTE_ORIGINAL" "$CONFIG_DEST_PATH"
                
                if [ ! -f "$CONFIG_DEST_PATH" ]; then
                    echo -e "${RED}❌ Error: 설정 파일 복사 실패${NC}"
                    exit 1
                fi
                
                echo "    ✅ 설정 파일 복사 완료"
                echo ""
                
                # 상대 경로를 설정 파일로 사용 (cloud-service/VPN 디렉토리 내에서 실행하므로)
                CONFIG_RELATIVE_TO_CLIENT="./$CONFIG_FILE_NAME"
                
                echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
                echo -e "${GREEN}✅ 설치 완료! (디렉토리: $(pwd))${NC}"
                echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
                echo ""
                echo -e "${YELLOW}VPN 클라이언트 실행 (현재 위치: ${VPN_CLIENT_DIR})${NC}"
                echo -e "  ${GREEN}sudo ./bin/vpn_client --config $CONFIG_RELATIVE_TO_CLIENT${NC}"
                echo ""
                echo -e "${YELLOW}백그라운드 실행:${NC}"
                echo -e "  ${GREEN}sudo nohup ./bin/vpn_client --config $CONFIG_RELATIVE_TO_CLIENT > /dev/null 2>&1 &${NC}"
                echo ""
                
                # 실행 여부 확인
                read -p "지금 바로 VPN 클라이언트를 실행하시겠습니까? (Y/n): " -n 1 -r
                echo
                if [[ ! $REPLY =~ ^[Nn]$ ]]; then
                    echo ""
                    echo -e "${GREEN}🚀 VPN 클라이언트 시작...${NC}"
                    # 실행 시 sudo 권한 필요 (VNIC 설정 등)
                    sudo ./bin/vpn_client --config "$CONFIG_RELATIVE_TO_CLIENT" > /dev/null 2>&1 &
                fi
                """;
    }
}