package com.computer_architecture.cloudservice.domain.vm.dto;

import com.computer_architecture.cloudservice.global.domain.vmachine.VMachine;
import com.computer_architecture.cloudservice.global.domain.vmachine.VmStatus;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;

import java.time.LocalDateTime;

public class VmResponseDto {

    @Getter
    @Builder
    @AllArgsConstructor
    public static class VmInfo {
        private Long id;
        private Integer proxmoxVmId;
        private String vmName;
        private String internalIp;
        private VmStatus status;
        private VmSpecInfo spec;
        private SshInfo ssh;
        private LocalDateTime createdAt;

        public static VmInfo from(VMachine vm) {
            VmSpecInfo specInfo = null;
            if (vm.getVmSpec() != null) {
                specInfo = VmSpecInfo.builder()
                        .cpuCores(vm.getVmSpec().getCpuCores())
                        .memoryMb(vm.getVmSpec().getMemoryMb())
                        .diskGb(vm.getVmSpec().getDiskGb())
                        .build();
            }

            SshInfo sshInfo = null;
            if (vm.getSshCredential() != null) {
                sshInfo = SshInfo.builder()
                        .username(vm.getSshCredential().getUsername())
                        .build();
            }

            return VmInfo.builder()
                    .id(vm.getId())
                    .proxmoxVmId(vm.getProxmoxVmId())
                    .vmName(vm.getVmName())
                    .internalIp(vm.getInternalIp())
                    .status(vm.getStatus())
                    .spec(specInfo)
                    .ssh(sshInfo)
                    .createdAt(vm.getCreatedAt())
                    .build();
        }
    }

    @Getter
    @Builder
    @AllArgsConstructor
    public static class VmSpecInfo {
        private Integer cpuCores;
        private Integer memoryMb;
        private Integer diskGb;
    }

    @Getter
    @Builder
    @AllArgsConstructor
    public static class SshInfo {
        private String username;
    }

    @Getter
    @Builder
    @AllArgsConstructor
    public static class VmCreateResponse {
        private Long vmId;
        private Integer proxmoxVmId;
        private String vmName;
        private String internalIp;
        private String message;
    }

    @Getter
    @Builder
    @AllArgsConstructor
    public static class VmSimpleResponse {
        private Long vmId;
        private String message;
    }
}