package com.computer_architecture.cloudservice.global.domain.vmachine;

import lombok.Getter;
import lombok.RequiredArgsConstructor;

@Getter
@RequiredArgsConstructor
public enum VmStatus {
    CREATING("생성 중"),
    STOPPED("정지됨"),
    RUNNING("실행 중"),
    ERROR("오류");

    private final String description;
}