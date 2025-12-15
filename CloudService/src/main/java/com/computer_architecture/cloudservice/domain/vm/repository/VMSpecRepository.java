package com.computer_architecture.cloudservice.domain.vm.repository;

import com.computer_architecture.cloudservice.global.domain.vmspec.VMSpec;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface VMSpecRepository extends JpaRepository<VMSpec, Long> {

    Optional<VMSpec> findByVMachineId(Long vmId);
}