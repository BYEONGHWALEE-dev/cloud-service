package com.computer_architecture.cloudservice.domain.vm.controller;

import com.computer_architecture.cloudservice.domain.vm.dto.VmRequestDto;
import com.computer_architecture.cloudservice.domain.vm.dto.VmResponseDto;
import com.computer_architecture.cloudservice.domain.vm.service.VmService;
import com.computer_architecture.cloudservice.global.apiPayload.ApiResponse;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.SuccessStatus;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/vms")
@RequiredArgsConstructor
public class VmController {

    private final VmService vmService;

    /**
     * VM 생성
     */
    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public ApiResponse<VmResponseDto.VmCreateResponse> createVm(
            @Valid @RequestBody VmRequestDto.CreateRequest request) {

        VmResponseDto.VmCreateResponse response = vmService.createVm(request);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.CREATED, "VM201", "VM 생성 요청 성공"),
                response
        );
    }

    /**
     * VM 상세 조회
     */
    @GetMapping("/{vmId}")
    public ApiResponse<VmResponseDto.VmInfo> getVm(@PathVariable Long vmId) {

        VmResponseDto.VmInfo response = vmService.getVm(vmId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "VM 조회 성공"),
                response
        );
    }

    /**
     * 내 VM 목록 조회
     */
    @GetMapping("/my/{memberId}")
    public ApiResponse<List<VmResponseDto.VmInfo>> getMyVms(@PathVariable Long memberId) {

        List<VmResponseDto.VmInfo> response = vmService.getMyVms(memberId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "VM 목록 조회 성공"),
                response
        );
    }

    /**
     * VM 시작
     */
    @PostMapping("/{vmId}/start")
    public ApiResponse<VmResponseDto.VmSimpleResponse> startVm(@PathVariable Long vmId) {

        VmResponseDto.VmSimpleResponse response = vmService.startVm(vmId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "VM 시작 성공"),
                response
        );
    }

    /**
     * VM 정지
     */
    @PostMapping("/{vmId}/stop")
    public ApiResponse<VmResponseDto.VmSimpleResponse> stopVm(@PathVariable Long vmId) {

        VmResponseDto.VmSimpleResponse response = vmService.stopVm(vmId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "VM 정지 성공"),
                response
        );
    }

    /**
     * VM 삭제
     */
    @DeleteMapping("/{vmId}")
    public ApiResponse<VmResponseDto.VmSimpleResponse> deleteVm(@PathVariable Long vmId) {

        VmResponseDto.VmSimpleResponse response = vmService.deleteVm(vmId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "VM 삭제 성공"),
                response
        );
    }

    /**
     * VM 리소스 모니터링 (Proxmox VM ID 사용)
     */
    @GetMapping("/monitoring/proxmox/{proxmoxVmId}")
    public ApiResponse<VmResponseDto.VmMonitoringInfo> getVmMonitoringByProxmoxId(@PathVariable Integer proxmoxVmId) {

        VmResponseDto.VmMonitoringInfo response = vmService.getVmMonitoringByProxmoxId(proxmoxVmId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "VM200", "모니터링 조회 성공"),
                response
        );
    }
}