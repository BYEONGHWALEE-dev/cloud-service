package com.computer_architecture.cloudservice;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.data.jpa.repository.config.EnableJpaAuditing;

@SpringBootApplication
@EnableJpaAuditing
public class CloudserviceApplication {

	public static void main(String[] args) {
		SpringApplication.run(CloudserviceApplication.class, args);
	}

}
