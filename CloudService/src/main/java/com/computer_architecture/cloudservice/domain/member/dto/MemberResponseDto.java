package com.computer_architecture.cloudservice.domain.member.dto;

import com.computer_architecture.cloudservice.global.domain.member.Member;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;

import java.time.LocalDateTime;

public class MemberResponseDto {

    @Getter
    @Builder
    @AllArgsConstructor
    public static class MemberInfo {
        private Long id;
        private String name;
        private String studentNumber;
        private LocalDateTime createdAt;

        public static MemberInfo from(Member member) {
            return MemberInfo.builder()
                    .id(member.getId())
                    .name(member.getName())
                    .studentNumber(member.getStudentNumber())
                    .createdAt(member.getCreatedAt())
                    .build();
        }
    }

    @Getter
    @Builder
    @AllArgsConstructor
    public static class LoginResponse {
        private Long memberId;
        private String name;
        private String message;
    }
}