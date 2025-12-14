package com.computer_architecture.cloudservice.global.domain.entity;

import com.computer_architecture.cloudservice.global.domain.common.BaseEntity;
import jakarta.persistence.*;
import lombok.*;

@Entity
@Getter
@NoArgsConstructor(access = AccessLevel.PROTECTED)
public class Member extends BaseEntity {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Integer memberId;

    @Column(nullable = false)
    private String studentNumber;

    @Column(nullable = false)
    private String name;

    @Column(nullable = false)
    private String password;

    @Builder
    public Member(String studentNumber, String name, String password) {
        this.studentNumber = studentNumber;
        this.name = name;
        this.password = password;
    }

    public void changePassword(String oldPassword, String newPassword) {
        validatePassword(oldPassword, newPassword);
        this.password = newPassword;
    }

    public void validatePassword(String oldPassword, String newPassword) {
        if(newPassword.equals(oldPassword)) {
            throw new IllegalArgumentException("Passwords do not match");
        }
    }
}
