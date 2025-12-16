package com.computer_architecture.cloudservice.domain.vm.repository;

import com.computer_architecture.cloudservice.global.domain.sshcredential.SSHCredential;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface SSHCredentialRepository extends JpaRepository<SSHCredential, Long> {

    @Query("SELECT sc FROM SSHCredential sc WHERE sc.vMachine.id = :vmId")
    Optional<SSHCredential> findByVmId(@Param("vmId") Long vmId);
}