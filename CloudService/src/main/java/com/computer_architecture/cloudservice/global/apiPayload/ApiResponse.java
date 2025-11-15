package com.computer_architecture.cloudservice.global.apiPayload;

import com.computer_architecture.cloudservice.global.apiPayload.code.BaseSuccessCode;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.ErrorStatus;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.SuccessStatus;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;
import lombok.AllArgsConstructor;
import lombok.Getter;

@Getter
@AllArgsConstructor
@JsonPropertyOrder({"isSuccess", "code", "message", "result"})
public class ApiResponse<T> {

    @JsonProperty("isSuccess")
    private final boolean success;
    private String code;
    private String message;
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private T Result;

    // isSuccess
    public static <T> ApiResponse<T> onSuccess(SuccessStatus status, T result) {
        return new ApiResponse<>(true, status.getCode(), status.getMessage(), result);
    }

    public static <T> ApiResponse<T> of(BaseSuccessCode code, T result) {
        return new ApiResponse<>(true, code.getHttpReason().getCode(), code.getHttpReason().getMessage(), result);
    }

    //ExceptionAdvice용
    public static <T> ApiResponse<T> onFailure(String code, String message, T data){
        return new ApiResponse<>(false, code, message, data);
    }

    // 실사용용
    public static <T> ApiResponse<T> onFailure(ErrorStatus errorStatus, T data){
        return new ApiResponse<>(false, errorStatus.getHttpReason().getCode(), errorStatus.getHttpReason().getMessage(), data);
    }

    // 데이터 없을 때용
    public static <T> ApiResponse<T> onFailure(ErrorStatus errorStatus){
        return new ApiResponse<>(false, errorStatus.getHttpReason().getCode(), errorStatus.getHttpReason().getMessage(), null);
    }

}
