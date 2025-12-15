package com.computer_architecture.cloudservice.domain.member.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.NoArgsConstructor;

public class MemberRequestDto {

    @Getter
    @NoArgsConstructor
    public static class SignupRequest {

        @NotBlank(message = "이름은 필수입니다")
        private String name;

        @NotBlank(message = "학번은 필수입니다")
        private String studentNumber;

        @NotBlank(message = "비밀번호는 필수입니다")
        @Size(min = 4, message = "비밀번호는 최소 4자 이상이어야 합니다")
        private String password;
    }

    @Getter
    @NoArgsConstructor
    public static class LoginRequest {

        @NotBlank(message = "학번은 필수입니다")
        private String studentNumber;

        @NotBlank(message = "비밀번호는 필수입니다")
        private String password;
    }
}