// src/common/ipc_protocol.c

#include "ipc_protocol.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static uint32_t global_request_id = 0;

// IPC 명령 → 문자열
const char* ipc_command_str(uint8_t command) {
    switch (command) {
        case IPC_PING:        return "PING";
        case IPC_ENCRYPT:     return "ENCRYPT";
        case IPC_DECRYPT:     return "DECRYPT";
        case IPC_ADD_KEY:     return "ADD_KEY";
        case IPC_REMOVE_KEY:  return "REMOVE_KEY";
        case IPC_HANDSHAKE:   return "HANDSHAKE";
        case IPC_SHUTDOWN:    return "SHUTDOWN";
        default:              return "UNKNOWN";
    }
}

// IPC 요청 초기화
void init_ipc_request(ipc_request_t *req, uint8_t command,
                      uint32_t vpn_ip, const uint8_t *data, uint16_t data_len) {
    req->command = command;
    req->request_id = htonl(++global_request_id);
    req->vpn_ip = vpn_ip;
    req->data_len = htons(data_len);
    
    if (data && data_len > 0) {
        memcpy(req->data, data, data_len);
    }
}

// IPC 응답 초기화
void init_ipc_response(ipc_response_t *resp, uint32_t request_id,
                       int8_t status, const uint8_t *data, uint16_t data_len) {
    resp->request_id = request_id;
    resp->status = status;
    resp->data_len = htons(data_len);
    
    if (data && data_len > 0) {
        memcpy(resp->data, data, data_len);
    }
}
