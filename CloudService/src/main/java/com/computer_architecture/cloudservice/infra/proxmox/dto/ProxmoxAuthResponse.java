package com.computer_architecture.cloudservice.infra.proxmox.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.NoArgsConstructor;

@Getter
@NoArgsConstructor
public class ProxmoxAuthResponse {

    @JsonProperty("data")
    private AuthData data;

    @Getter
    @NoArgsConstructor
    public static class AuthData {
        private String ticket;

        @JsonProperty("CSRFPreventionToken")
        private String csrfPreventionToken;

        private String username;
    }
}