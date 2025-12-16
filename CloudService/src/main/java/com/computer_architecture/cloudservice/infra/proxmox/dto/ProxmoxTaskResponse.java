package com.computer_architecture.cloudservice.infra.proxmox.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.NoArgsConstructor;

@Getter
@NoArgsConstructor
public class ProxmoxTaskResponse {

    @JsonProperty("data")
    private String data;  // UPID (Task ID)
}