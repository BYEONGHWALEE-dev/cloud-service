package com.computer_architecture.cloudservice.domain.vm.service;

import com.computer_architecture.cloudservice.domain.member.repository.MemberRepository;
import com.computer_architecture.cloudservice.domain.vm.dto.VmRequestDto;
import com.computer_architecture.cloudservice.domain.vm.dto.VmResponseDto;
import com.computer_architecture.cloudservice.domain.vm.repository.SSHCredentialRepository;
import com.computer_architecture.cloudservice.domain.vm.repository.VMachineRepository;
import com.computer_architecture.cloudservice.domain.vm.repository.VMSpecRepository;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.ErrorStatus;
import com.computer_architecture.cloudservice.global.apiPayload.exceptionHandler.exception.base.BaseException;
import com.computer_architecture.cloudservice.global.domain.member.Member;
import com.computer_architecture.cloudservice.global.domain.sshcredential.SSHCredential;
import com.computer_architecture.cloudservice.global.domain.vmachine.VMachine;
import com.computer_architecture.cloudservice.global.domain.vmachine.VmStatus;
import com.computer_architecture.cloudservice.global.domain.vmspec.VMSpec;
import com.computer_architecture.cloudservice.infra.proxmox.ProxmoxApiService;
import com.computer_architecture.cloudservice.infra.proxmox.dto.ProxmoxVmStatusResponse;
import com.computer_architecture.cloudservice.infra.proxmox.dto.VmResourceUsage;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

@Service
@RequiredArgsConstructor
@Transactional(readOnly = true)
@Slf4j
public class VmServiceImpl implements VmService {

    private final VMachineRepository vmRepository;
    private final VMSpecRepository vmSpecRepository;
    private final SSHCredentialRepository sshCredentialRepository;
    private final MemberRepository memberRepository;
    private final ProxmoxApiService proxmoxApiService;

    @Value("${vm.ip-range.start:10}")
    private int ipRangeStart;

    @Value("${vm.ip-range.end:99}")
    private int ipRangeEnd;

    private static final String IP_PREFIX = "192.168.100.";
    private static final String DEFAULT_SSH_USER = "ubuntu";

    @Override
    @Transactional
    public VmResponseDto.VmCreateResponse createVm(VmRequestDto.CreateRequest request) {
        // 1. 회원 조회
        Member member = memberRepository.findById(request.getMemberId())
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        // 2. 다음 VM ID 조회
        Integer newVmId = proxmoxApiService.getNextVmId();
        log.info("새 VM ID 할당: {}", newVmId);

        // 3. 사용 가능한 IP 할당
        String internalIp = allocateInternalIp();
        log.info("내부 IP 할당: {}", internalIp);

        // 4. Proxmox에 VM 복제
        String taskId = proxmoxApiService.cloneVm(newVmId, request.getVmName());
        log.info("VM 복제 작업 시작: taskId={}", taskId);

        // 5. VM 스펙 설정 (CPU, 메모리) - 추가
        try {
            // clone 작업 완료 대기 (간단히 sleep, 실제로는 task 상태 확인 필요)
            Thread.sleep(5000);

            proxmoxApiService.configureVmSpec(newVmId, request.getCpuCores(), request.getMemoryMb());

            // 디스크 리사이즈 (템플릿보다 큰 경우만)
            if (request.getDiskGb() > 20) {  // 템플릿 기본값이 20GB라고 가정
                proxmoxApiService.resizeVmDisk(newVmId, request.getDiskGb());
            }

            // Cloud-init 설정 (IP, SSH 키)
            String sshKey = request.getSshPublicKey() != null ? request.getSshPublicKey() : "";
            if (!sshKey.isEmpty()) {
                proxmoxApiService.configureCloudInit(newVmId, internalIp, sshKey);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            log.warn("VM 설정 중 인터럽트 발생");
        } catch (Exception e) {
            log.error("VM 스펙 설정 실패: {}", e.getMessage());
            // 실패해도 VM은 생성됨, 로그만 남김
        }

        // 6. DB에 VM 정보 저장
        VMachine vm = VMachine.builder()
                .proxmoxVmId(newVmId)
                .vmName(request.getVmName())
                .internalIp(internalIp)
                .status(VmStatus.STOPPED)  // 생성 직후는 STOPPED
                .build();
        vm.assignToMember(member);
        VMachine savedVm = vmRepository.save(vm);

        // 7. VM 스펙 저장
        VMSpec vmSpec = VMSpec.builder()
                .cpuCores(request.getCpuCores())
                .memoryMb(request.getMemoryMb())
                .diskGb(request.getDiskGb())
                .build();
        vmSpec.assignToVMachine(savedVm);
        vmSpecRepository.save(vmSpec);

        // 8. SSH 인증정보 저장
        String sshKey = request.getSshPublicKey() != null ? request.getSshPublicKey() : "";
        SSHCredential sshCredential = SSHCredential.builder()
                .username(DEFAULT_SSH_USER)
                .sshKey(sshKey)
                .build();
        sshCredential.assignToVMachine(savedVm);
        sshCredentialRepository.save(sshCredential);

        log.info("VM 생성 완료: vmId={}, proxmoxVmId={}, ip={}", savedVm.getId(), newVmId, internalIp);

        return VmResponseDto.VmCreateResponse.builder()
                .vmId(savedVm.getId())
                .proxmoxVmId(newVmId)
                .vmName(request.getVmName())
                .internalIp(internalIp)
                .message("VM 생성이 완료되었습니다.")
                .build();
    }

    @Override
    public VmResponseDto.VmInfo getVm(Long vmId) {
        VMachine vm = vmRepository.findById(vmId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        // Proxmox에서 실시간 상태 조회
        try {
            ProxmoxVmStatusResponse.StatusData statusData = proxmoxApiService.getVmStatus(vm.getProxmoxVmId());
            if (statusData != null) {
                VmStatus currentStatus = mapProxmoxStatus(statusData.getStatus());
                if (vm.getStatus() != currentStatus) {
                    vm.updateStatus(currentStatus);
                    vmRepository.save(vm);
                }
            }
        } catch (Exception e) {
            log.warn("Proxmox 상태 조회 실패: vmId={}, error={}", vmId, e.getMessage());
        }

        return VmResponseDto.VmInfo.from(vm);
    }

    @Override
    public List<VmResponseDto.VmInfo> getMyVms(Long memberId) {
        List<VMachine> vms = vmRepository.findAllByMemberId(memberId);
        return vms.stream()
                .map(VmResponseDto.VmInfo::from)
                .collect(Collectors.toList());
    }

    @Override
    @Transactional
    public VmResponseDto.VmSimpleResponse startVm(Long vmId) {
        VMachine vm = vmRepository.findById(vmId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        proxmoxApiService.startVm(vm.getProxmoxVmId());
        vm.updateStatus(VmStatus.RUNNING);
        vmRepository.save(vm);

        log.info("VM 시작: vmId={}, proxmoxVmId={}", vmId, vm.getProxmoxVmId());

        return VmResponseDto.VmSimpleResponse.builder()
                .vmId(vmId)
                .message("VM이 시작되었습니다.")
                .build();
    }

    @Override
    @Transactional
    public VmResponseDto.VmSimpleResponse stopVm(Long vmId) {
        VMachine vm = vmRepository.findById(vmId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        proxmoxApiService.stopVm(vm.getProxmoxVmId());
        vm.updateStatus(VmStatus.STOPPED);
        vmRepository.save(vm);

        log.info("VM 정지: vmId={}, proxmoxVmId={}", vmId, vm.getProxmoxVmId());

        return VmResponseDto.VmSimpleResponse.builder()
                .vmId(vmId)
                .message("VM이 정지되었습니다.")
                .build();
    }

    @Override
    @Transactional
    public VmResponseDto.VmSimpleResponse deleteVm(Long vmId) {
        VMachine vm = vmRepository.findById(vmId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        // Proxmox에서 VM 삭제
        proxmoxApiService.deleteVm(vm.getProxmoxVmId());

        // DB에서 삭제
        vmRepository.delete(vm);

        log.info("VM 삭제: vmId={}, proxmoxVmId={}", vmId, vm.getProxmoxVmId());

        return VmResponseDto.VmSimpleResponse.builder()
                .vmId(vmId)
                .message("VM이 삭제되었습니다.")
                .build();
    }

    /**
     * 사용 가능한 내부 IP 할당
     */
    private String allocateInternalIp() {
        Set<String> usedIps = vmRepository.findAllUsedInternalIps()
                .stream()
                .collect(Collectors.toSet());

        for (int i = ipRangeStart; i <= ipRangeEnd; i++) {
            String candidateIp = IP_PREFIX + i;
            if (!usedIps.contains(candidateIp)) {
                return candidateIp;
            }
        }

        throw new BaseException(ErrorStatus.BAD_REQUEST);  // IP 풀 소진
    }

    /**
     * Proxmox 상태를 VmStatus로 변환
     */
    private VmStatus mapProxmoxStatus(String proxmoxStatus) {
        if (proxmoxStatus == null) {
            return VmStatus.ERROR;
        }
        return switch (proxmoxStatus.toLowerCase()) {
            case "running" -> VmStatus.RUNNING;
            case "stopped" -> VmStatus.STOPPED;
            default -> VmStatus.ERROR;
        };
    }

    @Override
    public VmResponseDto.VmMonitoringInfo getVmMonitoring(Long vmId) {
        VMachine vm = vmRepository.findById(vmId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        VmResourceUsage usage = proxmoxApiService.getVmResourceUsage(vm.getProxmoxVmId());

        if (usage == null) {
            throw new BaseException(ErrorStatus.INTERNAL_SERVER_ERROR);
        }

        return VmResponseDto.VmMonitoringInfo.from(usage);
    }
}