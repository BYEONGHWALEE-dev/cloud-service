package com.computer_architecture.cloudservice.domain.vm.dto;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.NoArgsConstructor;

public class VmRequestDto {

    @Getter
    @NoArgsConstructor
    public static class CreateRequest {

        @NotNull(message = "회원 ID는 필수입니다")
        private Long memberId;

        @NotBlank(message = "VM 이름은 필수입니다")
        private String vmName;

        @NotNull(message = "CPU 코어 수는 필수입니다")
        @Min(value = 1, message = "CPU는 최소 1코어 이상이어야 합니다")
        @Max(value = 4, message = "CPU는 최대 4코어까지 가능합니다")
        private Integer cpuCores;

        @NotNull(message = "메모리 용량은 필수입니다")
        @Min(value = 512, message = "메모리는 최소 512MB 이상이어야 합니다")
        @Max(value = 8192, message = "메모리는 최대 8192MB까지 가능합니다")
        private Integer memoryMb;

        @NotNull(message = "디스크 용량은 필수입니다")
        @Min(value = 10, message = "디스크는 최소 10GB 이상이어야 합니다")
        @Max(value = 100, message = "디스크는 최대 100GB까지 가능합니다")
        private Integer diskGb;

        // SSH 공개키 (선택)
        private String sshPublicKey;
    }
}