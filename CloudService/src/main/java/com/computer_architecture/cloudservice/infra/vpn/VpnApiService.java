package com.computer_architecture.cloudservice.infra.vpn;

import com.computer_architecture.cloudservice.infra.vpn.config.VpnConfig;
import com.computer_architecture.cloudservice.infra.vpn.dto.VpnUserResponse;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;

@Service
@RequiredArgsConstructor
@Slf4j
public class VpnApiService {

    private final WebClient vpnWebClient;
    private final VpnConfig vpnConfig;

    /**
     * VPN 사용자 등록
     */
    public VpnUserResponse registerUser(Long memberId, String username) {
        log.info("VPN 사용자 등록: memberId={}, username={}", memberId, username);

        // TODO: VPN API 스펙 확정 후 구현
        // VpnUserResponse response = vpnWebClient.post()
        //         .uri("/api/users")
        //         .bodyValue(new VpnUserRequest(memberId, username))
        //         .retrieve()
        //         .bodyToMono(VpnUserResponse.class)
        //         .block();

        return VpnUserResponse.builder()
                .vpnIp("10.8.0." + memberId)  // 임시 로직
                .configData("# VPN Config Placeholder")
                .build();
    }

    /**
     * VPN 사용자 설정파일 조회
     */
    public String getUserConfig(Long memberId) {
        log.info("VPN 설정파일 조회: memberId={}", memberId);

        // TODO: VPN API 스펙 확정 후 구현
        return "# VPN Config for member " + memberId;
    }

    /**
     * VPN 사용자 삭제
     */
    public void deleteUser(Long memberId) {
        log.info("VPN 사용자 삭제: memberId={}", memberId);

        // TODO: VPN API 스펙 확정 후 구현
    }
}