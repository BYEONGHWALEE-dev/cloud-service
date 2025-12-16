package com.computer_architecture.cloudservice.infra.proxmox.dto;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;

@Getter
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class VmResourceUsage {
    private Integer vmId;
    private Double cpuUsage;      // 0.0 ~ 1.0
    private Long memoryUsed;      // bytes
    private Long memoryTotal;     // bytes
    private Long diskUsed;        // bytes
    private Long diskTotal;       // bytes
    private Long uptime;          // seconds
    private String status;        // "running", "stopped"

    // 편의 메서드
    public Double getCpuUsagePercent() {
        return cpuUsage != null ? cpuUsage * 100 : 0.0;
    }

    public Double getMemoryUsagePercent() {
        if (memoryTotal == null || memoryTotal == 0) return 0.0;
        return (memoryUsed * 100.0) / memoryTotal;
    }

    public Long getMemoryUsedMb() {
        return memoryUsed != null ? memoryUsed / (1024 * 1024) : 0;
    }

    public Long getMemoryTotalMb() {
        return memoryTotal != null ? memoryTotal / (1024 * 1024) : 0;
    }
}