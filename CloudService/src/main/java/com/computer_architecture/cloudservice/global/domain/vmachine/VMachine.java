package com.computer_architecture.cloudservice.global.domain.vmachine;

import com.computer_architecture.cloudservice.global.domain.common.BaseEntity;
import com.computer_architecture.cloudservice.global.domain.member.Member;
import com.computer_architecture.cloudservice.global.domain.sshcredential.SSHCredential;
import com.computer_architecture.cloudservice.global.domain.vmspec.VMSpec;
import jakarta.persistence.*;
import lombok.*;

@Entity
@Table(name = "vmachine")
@Getter
@NoArgsConstructor(access = AccessLevel.PROTECTED)
@AllArgsConstructor
@Builder
public class VMachine extends BaseEntity {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "vm_id")
    private Long id;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "member_id", nullable = false)
    private Member member;

    @Column(name = "proxmox_vm_id", nullable = false)
    private Integer proxmoxVmId;  // String → Integer로 변경 (Proxmox VM ID는 숫자)

    @Column(name = "vm_name", nullable = false)
    private String vmName;  // 추가: VM 표시 이름

    @Column(name = "internal_ip")
    private String internalIp;  // 추가: 192.168.100.x

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false)
    @Builder.Default
    private VmStatus status = VmStatus.CREATING;  // 추가: Enum으로 변경

    @OneToOne(mappedBy = "vMachine", cascade = CascadeType.ALL, orphanRemoval = true)
    private SSHCredential sshCredential;

    @OneToOne(mappedBy = "vMachine", cascade = CascadeType.ALL, orphanRemoval = true)
    private VMSpec vmSpec;

    // 연관관계 편의 메서드
    public void assignToMember(Member member) {
        this.member = member;
        member.getVMachines().add(this);
    }

    public void updateStatus(VmStatus status) {
        this.status = status;
    }

    public void assignInternalIp(String internalIp) {
        this.internalIp = internalIp;
    }
}