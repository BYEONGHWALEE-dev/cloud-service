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

        private Double cpu;        // CPU 사용률 (0~1)
        private Long mem;          // 메모리 사용량 (bytes)
        private Long disk;         // 디스크 사용량 (bytes)
        private Long uptime;       // 가동 시간 (seconds)
        private Long netin;        // 네트워크 수신 (bytes)
        private Long netout;       // 네트워크 송신 (bytes)
    }
}