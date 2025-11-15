package com.computer_architecture.cloudservice.global.apiPayload.exceptionHandler.exception.base;

import com.computer_architecture.cloudservice.global.apiPayload.code.BaseErrorCode;
import com.computer_architecture.cloudservice.global.apiPayload.code.dto.ErrorReasonDTO;
import lombok.AllArgsConstructor;
import lombok.Getter;

@Getter
@AllArgsConstructor
public class BaseException extends RuntimeException {
    private BaseErrorCode code;

    public ErrorReasonDTO getErrorHttpReason() {
        return this.code.getHttpReason();
    }
}
