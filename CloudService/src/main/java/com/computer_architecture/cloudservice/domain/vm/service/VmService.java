package com.computer_architecture.cloudservice.domain.vm.service;

import com.computer_architecture.cloudservice.domain.vm.dto.VmRequestDto;
import com.computer_architecture.cloudservice.domain.vm.dto.VmResponseDto;

import java.util.List;

public interface VmService {

    // VM 생성
    VmResponseDto.VmCreateResponse createVm(VmRequestDto.CreateRequest request);

    // VM 조회
    VmResponseDto.VmInfo getVm(Long vmId);

    // 내 VM 목록 조회
    List<VmResponseDto.VmInfo> getMyVms(Long memberId);

    // VM 시작
    VmResponseDto.VmSimpleResponse startVm(Long vmId);

    // VM 정지
    VmResponseDto.VmSimpleResponse stopVm(Long vmId);

    // VM 삭제
    VmResponseDto.VmSimpleResponse deleteVm(Long vmId);
}