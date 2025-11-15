package com.computer_architecture.cloudservice.global.apiPayload.code;

import com.computer_architecture.cloudservice.global.apiPayload.code.dto.ErrorReasonDTO;

public interface BaseErrorCode {

    public ErrorReasonDTO getHttpReason();
}