package com.computer_architecture.cloudservice.domain.vpn.controller;

import com.computer_architecture.cloudservice.global.apiPayload.ApiResponse;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.SuccessStatus;
import com.computer_architecture.cloudservice.infra.vpn.VpnApiService;
import lombok.RequiredArgsConstructor;
import org.springframework.core.io.ClassPathResource;
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

    /**
     * VPN 설정파일 다운로드
     */
    @GetMapping("/config/{memberId}")
    public ResponseEntity<String> downloadVpnConfig(@PathVariable Long memberId) {
        String configData = vpnApiService.getUserConfig(memberId);

        return ResponseEntity.ok()
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=vpn-config.conf")
                .contentType(MediaType.TEXT_PLAIN)
                .body(configData);
    }

    /**
     * VPN 클라이언트 다운로드 (exe 파일)
     * 실제 파일은 resources/static/vpn-client.exe 에 위치
     */
    @GetMapping("/client/download")
    public ResponseEntity<Resource> downloadVpnClient() {
        try {
            Resource resource = new ClassPathResource("static/vpn-client.exe");

            return ResponseEntity.ok()
                    .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=vpn-client.exe")
                    .contentType(MediaType.APPLICATION_OCTET_STREAM)
                    .body(resource);
        } catch (Exception e) {
            return ResponseEntity.notFound().build();
        }
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
}