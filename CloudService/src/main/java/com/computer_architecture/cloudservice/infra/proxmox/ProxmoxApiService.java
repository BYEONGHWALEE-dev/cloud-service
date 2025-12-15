package com.computer_architecture.cloudservice.infra.proxmox;

import com.computer_architecture.cloudservice.infra.proxmox.config.ProxmoxConfig;
import com.computer_architecture.cloudservice.infra.proxmox.dto.*;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.BodyInserters;
import org.springframework.web.reactive.function.client.WebClient;

@Service
@RequiredArgsConstructor
@Slf4j
public class ProxmoxApiService {

    private final WebClient proxmoxWebClient;
    private final ProxmoxConfig proxmoxConfig;

    // 인증 정보 캐시
    private String ticket;
    private String csrfToken;

    /**
     * Proxmox 인증 - 티켓 발급
     */
    public void authenticate() {
        log.info("Proxmox 인증 시도: {}", proxmoxConfig.getUsername());

        ProxmoxAuthResponse response = proxmoxWebClient.post()
                .uri("/api2/json/access/ticket")
                .contentType(MediaType.APPLICATION_FORM_URLENCODED)
                .body(BodyInserters
                        .fromFormData("username", proxmoxConfig.getUsername())
                        .with("password", proxmoxConfig.getPassword()))
                .retrieve()
                .bodyToMono(ProxmoxAuthResponse.class)
                .block();

        if (response != null && response.getData() != null) {
            this.ticket = response.getData().getTicket();
            this.csrfToken = response.getData().getCsrfPreventionToken();
            log.info("Proxmox 인증 성공");
        } else {
            throw new RuntimeException("Proxmox 인증 실패");
        }
    }

    /**
     * VM 복제 (템플릿에서 새 VM 생성)
     */
    public String cloneVm(Integer newVmId, String vmName) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();
        Integer templateId = proxmoxConfig.getTemplateVmid();

        log.info("VM 복제 시작: 템플릿={}, 새VM ID={}, 이름={}", templateId, newVmId, vmName);

        ProxmoxTaskResponse response = proxmoxWebClient.post()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}/clone", node, templateId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .contentType(MediaType.APPLICATION_FORM_URLENCODED)
                .body(BodyInserters
                        .fromFormData("newid", String.valueOf(newVmId))
                        .with("name", vmName)
                        .with("full", "1"))  // full clone
                .retrieve()
                .bodyToMono(ProxmoxTaskResponse.class)
                .block();

        log.info("VM 복제 작업 시작됨: UPID={}", response != null ? response.getData() : null);
        return response != null ? response.getData() : null;
    }

    /**
     * VM에 Cloud-init 설정 (IP, SSH 키 등)
     */
    public void configureCloudInit(Integer vmId, String ipAddress, String sshPublicKey) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();
        String ipConfig = String.format("ip=%s/24,gw=192.168.100.1", ipAddress);

        log.info("Cloud-init 설정: VM={}, IP={}", vmId, ipAddress);

        proxmoxWebClient.put()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}/config", node, vmId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .contentType(MediaType.APPLICATION_FORM_URLENCODED)
                .body(BodyInserters
                        .fromFormData("ipconfig0", ipConfig)
                        .with("sshkeys", encodeUri(sshPublicKey)))
                .retrieve()
                .bodyToMono(String.class)
                .block();

        log.info("Cloud-init 설정 완료");
    }

    /**
     * VM 시작
     */
    public String startVm(Integer vmId) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();
        log.info("VM 시작: {}", vmId);

        ProxmoxTaskResponse response = proxmoxWebClient.post()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}/status/start", node, vmId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .retrieve()
                .bodyToMono(ProxmoxTaskResponse.class)
                .block();

        return response != null ? response.getData() : null;
    }

    /**
     * VM 정지
     */
    public String stopVm(Integer vmId) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();
        log.info("VM 정지: {}", vmId);

        ProxmoxTaskResponse response = proxmoxWebClient.post()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}/status/stop", node, vmId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .retrieve()
                .bodyToMono(ProxmoxTaskResponse.class)
                .block();

        return response != null ? response.getData() : null;
    }

    /**
     * VM 삭제
     */
    public String deleteVm(Integer vmId) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();
        log.info("VM 삭제: {}", vmId);

        ProxmoxTaskResponse response = proxmoxWebClient.delete()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}", node, vmId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .retrieve()
                .bodyToMono(ProxmoxTaskResponse.class)
                .block();

        return response != null ? response.getData() : null;
    }

    /**
     * VM 상태 조회
     */
    public ProxmoxVmStatusResponse.StatusData getVmStatus(Integer vmId) {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();

        ProxmoxVmStatusResponse response = proxmoxWebClient.get()
                .uri("/api2/json/nodes/{node}/qemu/{vmid}/status/current", node, vmId)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .retrieve()
                .bodyToMono(ProxmoxVmStatusResponse.class)
                .block();

        return response != null ? response.getData() : null;
    }

    /**
     * 다음 사용 가능한 VM ID 조회
     */
    public Integer getNextVmId() {
        ensureAuthenticated();

        String node = proxmoxConfig.getNode();

        ProxmoxVmListResponse response = proxmoxWebClient.get()
                .uri("/api2/json/nodes/{node}/qemu", node)
                .header("Cookie", "PVEAuthCookie=" + ticket)
                .header("CSRFPreventionToken", csrfToken)
                .retrieve()
                .bodyToMono(ProxmoxVmListResponse.class)
                .block();

        if (response == null || response.getData() == null) {
            return 100;  // 기본값
        }

        // 현재 VM ID들 중 최대값 + 1 반환 (템플릿 제외)
        return response.getData().stream()
                .map(ProxmoxVmListResponse.VmData::getVmid)
                .filter(vmid -> vmid >= 100 && !vmid.equals(proxmoxConfig.getTemplateVmid()))
                .max(Integer::compareTo)
                .map(max -> max + 1)
                .orElse(100);
    }

    /**
     * 인증 상태 확인 및 재인증
     */
    private void ensureAuthenticated() {
        if (ticket == null || csrfToken == null) {
            authenticate();
        }
    }

    /**
     * SSH 키 URL 인코딩
     */
    private String encodeUri(String value) {
        return java.net.URLEncoder.encode(value, java.nio.charset.StandardCharsets.UTF_8);
    }
}
