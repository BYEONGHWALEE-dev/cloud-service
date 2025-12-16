package com.computer_architecture.cloudservice.global.domain.vmspec;

import com.computer_architecture.cloudservice.global.domain.common.BaseEntity;
import com.computer_architecture.cloudservice.global.domain.vmachine.VMachine;
import jakarta.persistence.*;
import lombok.*;

@Entity
@Table(name = "vm_spec")
@Getter
@NoArgsConstructor(access = AccessLevel.PROTECTED)
@AllArgsConstructor
@Builder
public class VMSpec extends BaseEntity {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "vm_spec_id")
    private Long id;

    @OneToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "vm_id", nullable = false, unique = true)
    private VMachine vMachine;

    @Column(name = "cpu_cores", nullable = false)
    private Integer cpuCores;  // Short → Integer

    @Column(name = "memory_mb", nullable = false)
    private Integer memoryMb;  // Short → Integer (2048MB 등 지원)

    @Column(name = "disk_gb", nullable = false)
    private Integer diskGb;    // Short → Integer

    // 연관관계 편의 메서드
    public void assignToVMachine(VMachine vMachine) {
        this.vMachine = vMachine;
    }
}