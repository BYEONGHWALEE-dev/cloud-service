package com.computer_architecture.cloudservice.domain.vm.repository;

import com.computer_architecture.cloudservice.global.domain.member.Member;
import com.computer_architecture.cloudservice.global.domain.vmachine.VMachine;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface VMachineRepository extends JpaRepository<VMachine, Long> {

    List<VMachine> findAllByMember(Member member);

    List<VMachine> findAllByMemberId(Long memberId);

    Optional<VMachine> findByProxmoxVmId(Integer proxmoxVmId);

    boolean existsByProxmoxVmId(Integer proxmoxVmId);

    boolean existsByInternalIp(String internalIp);

    // 사용 중인 IP 목록 조회
    @Query("SELECT v.internalIp FROM VMachine v WHERE v.internalIp IS NOT NULL")
    List<String> findAllUsedInternalIps();
}