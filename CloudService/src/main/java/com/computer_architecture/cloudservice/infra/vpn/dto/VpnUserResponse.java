package com.computer_architecture.cloudservice.infra.vpn.dto;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;

@Getter
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class VpnUserResponse {
    private String vpnIp;       // 할당된 VPN IP (10.8.0.x)
    private String configData;  // VPN 설정 파일 내용
}