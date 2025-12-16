package com.computer_architecture.cloudservice.domain.member.service;

import com.computer_architecture.cloudservice.domain.member.dto.MemberRequestDto;
import com.computer_architecture.cloudservice.domain.member.dto.MemberResponseDto;
import com.computer_architecture.cloudservice.domain.member.repository.MemberRepository;
import com.computer_architecture.cloudservice.global.apiPayload.code.status.ErrorStatus;
import com.computer_architecture.cloudservice.global.apiPayload.exceptionHandler.exception.base.BaseException;
import com.computer_architecture.cloudservice.global.domain.member.Member;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@RequiredArgsConstructor
@Transactional(readOnly = true)
@Slf4j
public class MemberServiceImpl implements MemberService {

    private final MemberRepository memberRepository;
    private final PasswordEncoder passwordEncoder;

    @Override
    @Transactional
    public MemberResponseDto.MemberInfo signup(MemberRequestDto.SignupRequest request) {
        // 학번 중복 체크
        if (memberRepository.existsByStudentNumber(request.getStudentNumber())) {
            throw new BaseException(ErrorStatus.ALREADY_EXIST);
        }

        Member member = Member.builder()
                .name(request.getName())
                .studentNumber(request.getStudentNumber())
                .password(passwordEncoder.encode(request.getPassword()))
                .build();

        Member savedMember = memberRepository.save(member);
        log.info("회원가입 완료: {}", savedMember.getStudentNumber());

        return MemberResponseDto.MemberInfo.from(savedMember);
    }

    @Override
    public MemberResponseDto.LoginResponse login(MemberRequestDto.LoginRequest request) {
        Member member = memberRepository.findByStudentNumber(request.getStudentNumber())
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        if (!passwordEncoder.matches(request.getPassword(), member.getPassword())) {
            throw new BaseException(ErrorStatus.UNAUTHORIZED);
        }

        log.info("로그인 성공: {}", member.getStudentNumber());

        return MemberResponseDto.LoginResponse.builder()
                .memberId(member.getId())
                .name(member.getName())
                .message("로그인 성공")
                .build();
    }

    @Override
    public MemberResponseDto.MemberInfo getMemberInfo(Long memberId) {
        Member member = memberRepository.findById(memberId)
                .orElseThrow(() -> new BaseException(ErrorStatus.NOT_FOUND));

        return MemberResponseDto.MemberInfo.from(member);
    }
}