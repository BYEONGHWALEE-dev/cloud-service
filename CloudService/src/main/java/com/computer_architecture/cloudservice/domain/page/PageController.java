package com.computer_architecture.cloudservice.domain.page;

import com.computer_architecture.cloudservice.domain.vm.dto.VmResponseDto;
import com.computer_architecture.cloudservice.domain.vm.service.VmService;
import com.computer_architecture.cloudservice.domain.member.service.MemberService;
import com.computer_architecture.cloudservice.domain.member.dto.MemberResponseDto;
import jakarta.servlet.http.HttpSession;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@Controller
@RequiredArgsConstructor
public class PageController {

    private final VmService vmService;
    private final MemberService memberService;

    /**
     * 메인 페이지 → 로그인 페이지로 리다이렉트
     */
    @GetMapping("/")
    public String index() {
        return "redirect:/login";
    }

    /**
     * 로그인 페이지
     */
    @GetMapping("/login")
    public String loginPage() {
        return "login";
    }

    /**
     * 회원가입 페이지
     */
    @GetMapping("/signup")
    public String signupPage() {
        return "signup";
    }

    /**
     * 대시보드 (VM 목록)
     */
    @GetMapping("/dashboard")
    public String dashboard(HttpSession session, Model model) {
        Long memberId = (Long) session.getAttribute("memberId");
        if (memberId == null) {
            return "redirect:/login";
        }

        String memberName = (String) session.getAttribute("memberName");
        List<VmResponseDto.VmInfo> vms = vmService.getMyVms(memberId);

        model.addAttribute("memberName", memberName);
        model.addAttribute("memberId", memberId);
        model.addAttribute("vms", vms);
        return "dashboard";
    }

    /**
     * VM 생성 페이지
     */
    @GetMapping("/vms/create")
    public String createVmPage(HttpSession session, Model model) {
        Long memberId = (Long) session.getAttribute("memberId");
        if (memberId == null) {
            return "redirect:/login";
        }

        model.addAttribute("memberId", memberId);
        return "vm-create";
    }

    /**
     * VM 상세 페이지
     */
    @GetMapping("/vms/{vmId}")
    public String vmDetailPage(@PathVariable Long vmId, HttpSession session, Model model) {
        Long memberId = (Long) session.getAttribute("memberId");
        if (memberId == null) {
            return "redirect:/login";
        }

        VmResponseDto.VmInfo vm = vmService.getVm(vmId);
        model.addAttribute("vm", vm);
        return "vm-detail";
    }

    /**
     * VPN 다운로드 페이지
     */
    @GetMapping("/vpn")
    public String vpnPage(HttpSession session, Model model) {
        Long memberId = (Long) session.getAttribute("memberId");
        if (memberId == null) {
            return "redirect:/login";
        }

        model.addAttribute("memberId", memberId);
        return "vpn";
    }

    /**
     * 로그아웃
     */
    @GetMapping("/logout")
    public String logout(HttpSession session) {
        session.invalidate();
        return "redirect:/login";
    }
}