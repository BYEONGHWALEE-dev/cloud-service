package com.computer_architecture.cloudservice.domain.member.controller;

import com.computer_architecture.cloudservice.domain.member.dto.MemberRequestDto;
import com.computer_architecture.cloudservice.domain.member.dto.MemberResponseDto;
import com.computer_architecture.cloudservice.domain.member.service.MemberService;
import com.computer_architecture.cloudservice.global.apiPayload.ApiResponse;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.SuccessStatus;
import jakarta.servlet.http.HttpSession;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/members")
@RequiredArgsConstructor
public class MemberController {

    private final MemberService memberService;

    /**
     * 회원가입
     */
    @PostMapping("/signup")
    @ResponseStatus(HttpStatus.CREATED)
    public ApiResponse<MemberResponseDto.MemberInfo> signup(
            @Valid @RequestBody MemberRequestDto.SignupRequest request) {

        MemberResponseDto.MemberInfo response = memberService.signup(request);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.CREATED, "MEMBER201", "회원가입 성공"),
                response
        );
    }

    /**
     * 로그인 (세션 저장 추가)
     */
    @PostMapping("/login")
    public ApiResponse<MemberResponseDto.LoginResponse> login(
            @Valid @RequestBody MemberRequestDto.LoginRequest request,
            HttpSession session) {

        MemberResponseDto.LoginResponse response = memberService.login(request);

        // 세션에 로그인 정보 저장
        session.setAttribute("memberId", response.getMemberId());
        session.setAttribute("memberName", response.getName());

        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "MEMBER200", "로그인 성공"),
                response
        );
    }

    /**
     * 회원정보 조회
     */
    @GetMapping("/{memberId}")
    public ApiResponse<MemberResponseDto.MemberInfo> getMemberInfo(
            @PathVariable Long memberId) {

        MemberResponseDto.MemberInfo response = memberService.getMemberInfo(memberId);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "MEMBER200", "조회 성공"),
                response
        );
    }

    /**
     * 학번 중복 체크
     */
    @GetMapping("/check/{studentNumber}")
    public ApiResponse<Boolean> checkStudentNumber(@PathVariable String studentNumber) {
        boolean exists = memberService.existsByStudentNumber(studentNumber);
        return ApiResponse.onSuccess(
                new SuccessStatus(HttpStatus.OK, "MEMBER200", "조회 성공"),
                exists
        );
    }
}