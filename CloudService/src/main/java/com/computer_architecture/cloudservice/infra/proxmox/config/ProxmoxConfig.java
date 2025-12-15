package com.computer_architecture.cloudservice.infra.proxmox.config;

import io.netty.handler.ssl.SslContext;
import io.netty.handler.ssl.SslContextBuilder;
import io.netty.handler.ssl.util.InsecureTrustManagerFactory;
import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.client.reactive.ReactorClientHttpConnector;
import org.springframework.web.reactive.function.client.WebClient;
import reactor.netty.http.client.HttpClient;

import javax.net.ssl.SSLException;

@Configuration
@ConfigurationProperties(prefix = "proxmox")
@Getter
@Setter
public class ProxmoxConfig {

    private String host;
    private String node;
    private String username;
    private String password;
    private Integer templateVmid;

    @Bean
    public WebClient proxmoxWebClient() throws SSLException {
        // Proxmox는 자체 서명 인증서를 사용하므로 SSL 검증 비활성화
        SslContext sslContext = SslContextBuilder
                .forClient()
                .trustManager(InsecureTrustManagerFactory.INSTANCE)
                .build();

        HttpClient httpClient = HttpClient.create()
                .secure(t -> t.sslContext(sslContext));

        return WebClient.builder()
                .baseUrl(host)
                .clientConnector(new ReactorClientHttpConnector(httpClient))
                .build();
    }
}