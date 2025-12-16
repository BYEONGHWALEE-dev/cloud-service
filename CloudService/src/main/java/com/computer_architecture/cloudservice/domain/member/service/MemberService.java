package com.computer_architecture.cloudservice.domain.member.service;

import com.computer_architecture.cloudservice.domain.member.dto.MemberRequestDto;
import com.computer_architecture.cloudservice.domain.member.dto.MemberResponseDto;

public interface MemberService {

    MemberResponseDto.MemberInfo signup(MemberRequestDto.SignupRequest request);

    MemberResponseDto.LoginResponse login(MemberRequestDto.LoginRequest request);

    MemberResponseDto.MemberInfo getMemberInfo(Long memberId);

    boolean existsByStudentNumber(String studentNumber);
}