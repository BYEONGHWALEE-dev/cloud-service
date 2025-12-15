package com.computer_architecture.cloudservice.infra.proxmox.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.NoArgsConstructor;

@Getter
@NoArgsConstructor
public class ProxmoxVmStatusResponse {

    @JsonProperty("data")
    private StatusData data;

    @Getter
    @NoArgsConstructor
    public static class StatusData {
        private String status;  // "running", "stopped"
        private Integer vmid;
        private String name;
        private Long maxmem;
        private Long maxdisk;
        private Integer cpus;
    }
}