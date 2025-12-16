package com.computer_architecture.cloudservice.infra.vpn.config;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.reactive.function.client.WebClient;

@Configuration
@ConfigurationProperties(prefix = "vpn")
@Getter
@Setter
public class VpnConfig {

    private String host;  // VPN 서버 주소

    @Bean
    public WebClient vpnWebClient() {
        return WebClient.builder()
                .baseUrl(host != null ? host : "http://localhost:8081")
                .build();
    }
}