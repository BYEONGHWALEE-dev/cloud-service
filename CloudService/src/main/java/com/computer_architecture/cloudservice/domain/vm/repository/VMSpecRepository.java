package com.computer_architecture.cloudservice.domain.vm.repository;

import com.computer_architecture.cloudservice.global.domain.vmspec.VMSpec;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface VMSpecRepository extends JpaRepository<VMSpec, Long> {

    @Query("SELECT vs FROM VMSpec vs WHERE vs.vMachine.id = :vmId")
    Optional<VMSpec> findByVmId(@Param("vmId") Long vmId);
}