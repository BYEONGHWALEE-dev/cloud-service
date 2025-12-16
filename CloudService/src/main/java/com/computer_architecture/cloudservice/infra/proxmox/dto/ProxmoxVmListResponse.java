package com.computer_architecture.cloudservice.infra.proxmox.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.NoArgsConstructor;

import java.util.List;

@Getter
@NoArgsConstructor
public class ProxmoxVmListResponse {

    @JsonProperty("data")
    private List<VmData> data;

    @Getter
    @NoArgsConstructor
    public static class VmData {
        private Integer vmid;
        private String name;
        private String status;
        private Long maxmem;
        private Long maxdisk;
        private Integer cpus;
    }
}