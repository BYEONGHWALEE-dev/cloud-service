package com.computer_architecture.cloudservice.global.apiPayload.code.status;

import com.computer_architecture.cloudservice.global.apiPayload.code.BaseSuccessCode;
import com.computer_architecture.cloudservice.global.apiPayload.code.dto.ReasonDTO;
import lombok.AllArgsConstructor;
import lombok.Getter;
import org.springframework.http.HttpStatus;

@Getter
@AllArgsConstructor
public class SuccessStatus implements BaseSuccessCode {

    private HttpStatus httpStatus;
    private String code;
    private String message;

    @Override
    public ReasonDTO getHttpReason() {
        return ReasonDTO.builder()
                .message(message)
                .code(code)
                .isSuccess(true)
                .httpStatus(httpStatus)
                .build();
    }
}
