package com.computer_architecture.cloudservice.global.domain.sshcredential;

import com.computer_architecture.cloudservice.global.domain.common.BaseEntity;
import com.computer_architecture.cloudservice.global.domain.vmachine.VMachine;
import jakarta.persistence.*;
import lombok.*;

@Entity
@Table(name = "ssh_credential")
@Getter
@NoArgsConstructor(access = AccessLevel.PROTECTED)
@AllArgsConstructor
@Builder
public class SSHCredential extends BaseEntity {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "credential_id")
    private Long id;

    @OneToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "vm_id", nullable = false, unique = true)
    private VMachine vMachine;

    @Column(name = "username", nullable = false)
    private String username;

    @Column(name = "ssh_key", nullable = false, columnDefinition = "TEXT")
    private String sshKey;

    // 연관관계 편의 메서드
    public void assignToVMachine(VMachine vMachine) {
        this.vMachine = vMachine;
    }
}