package com.computer_architecture.cloudservice.domain.vm.repository;

import com.computer_architecture.cloudservice.global.domain.sshcredential.SSHCredential;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface SSHCredentialRepository extends JpaRepository<SSHCredential, Long> {

    Optional<SSHCredential> findByVMachineId(Long vmId);
}