/**
 * @file prl_usage_example.c
 * @brief PRL Protocol Usage Example
 * @details 演示如何在 Products 层使用 PRL 协议
 */

#include "prl_pmc_ccm.h"
#include "osal_socket.h"
#include <stdio.h>
#include <string.h>

/* ========== PMC 端示例 ========== */

/**
 * @brief PMC 发送心跳到 CCM
 */
void pmc_send_heartbeat_to_ccm(int socket_fd)
{
    /* 1. 准备心跳消息 */
    prl_pmc_ccm_heartbeat_t heartbeat = {
        .sender_status = 0x01,      /* PMC 状态正常 */
        .link_quality = 95,         /* 链路质量 95% */
        .packet_loss = 50,          /* 丢包率 0.5% */
        .rtt_ms = 10,               /* 往返时延 10ms */
    };

    /* 2. 编码为协议报文 */
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len = sizeof(buf);

    int ret = prl_pmc_ccm_encode_heartbeat(&heartbeat, buf, &len);
    if (ret != PRL_OK) {
        printf("Failed to encode heartbeat: %d\n", ret);
        return;
    }

    /* 3. 通过网络发送 */
    osal_socket_send(socket_fd, buf, len);

    printf("PMC: Sent heartbeat to CCM (%zu bytes)\n", len);
}

/**
 * @brief PMC 发送遥控指令到 CCM
 */
void pmc_send_command_to_ccm(int socket_fd, uint32_t tc_id, uint32_t target)
{
    /* 1. 准备遥控消息 */
    prl_pmc_ccm_command_t command = {
        .tc_id = tc_id,
        .tc_target = target,
        .tc_action = 0x01,          /* 动作：启动 */
        .priority = 100,            /* 高优先级 */
        .param_length = 0,
    };

    /* 2. 准备参数（可选） */
    uint8_t params[64] = {0x01, 0x02, 0x03, 0x04};
    size_t params_len = 4;

    /* 3. 编码为协议报文 */
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len = sizeof(buf);

    int ret = prl_pmc_ccm_encode_command(&command, params, params_len, buf, &len);
    if (ret != PRL_OK) {
        printf("Failed to encode command: %d\n", ret);
        return;
    }

    /* 4. 通过网络发送 */
    osal_socket_send(socket_fd, buf, len);

    printf("PMC: Sent command to CCM (tc_id=%u, target=%u, %zu bytes)\n",
           tc_id, target, len);
}

/**
 * @brief PMC 接收来自 CCM 的消息
 */
void pmc_recv_from_ccm(int socket_fd)
{
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len;

    /* 1. 从网络接收 */
    int ret = osal_socket_recv(socket_fd, buf, &len);
    if (ret < 0) {
        return;
    }

    /* 2. 解析协议头，判断消息类型 */
    const prl_header_t *hdr = (const prl_header_t *)buf;

    /* 3. 根据消息类型解码 */
    switch (hdr->msg_type) {
        case PRL_PMC_CCM_MSG_HEARTBEAT: {
            prl_pmc_ccm_heartbeat_t heartbeat;
            ret = prl_pmc_ccm_decode_heartbeat(buf, len, &heartbeat);
            if (ret == PRL_OK) {
                printf("PMC: Received heartbeat from CCM (status=0x%x)\n",
                       heartbeat.sender_status);
            }
            break;
        }

        case PRL_PMC_CCM_MSG_TELEMETRY: {
            prl_pmc_ccm_telemetry_t telemetry;
            uint8_t *data;
            size_t data_len;

            ret = prl_pmc_ccm_decode_telemetry(buf, len, &telemetry, &data, &data_len);
            if (ret == PRL_OK) {
                printf("PMC: Received telemetry from CCM (tm_id=%u, data_len=%zu)\n",
                       telemetry.tm_id, data_len);
                /* 处理遥测数据 */
            }
            break;
        }

        case PRL_PMC_CCM_MSG_ACK: {
            prl_pmc_ccm_ack_t ack;
            uint8_t *ack_data;
            size_t ack_data_len;

            ret = prl_pmc_ccm_decode_ack(buf, len, &ack, &ack_data, &ack_data_len);
            if (ret == PRL_OK) {
                printf("PMC: Received ACK from CCM (seq=%u, result=%u)\n",
                       ack.ack_seq, ack.result);
            }
            break;
        }

        default:
            printf("PMC: Unknown message type: 0x%02x\n", hdr->msg_type);
            break;
    }
}

/* ========== CCM 端示例 ========== */

/**
 * @brief CCM 发送心跳到 PMC
 */
void ccm_send_heartbeat_to_pmc(int socket_fd)
{
    /* 1. 准备心跳消息 */
    prl_pmc_ccm_heartbeat_t heartbeat = {
        .sender_status = 0x01,      /* CCM 状态正常 */
        .link_quality = 98,         /* 链路质量 98% */
        .packet_loss = 20,          /* 丢包率 0.2% */
        .rtt_ms = 8,                /* 往返时延 8ms */
    };

    /* 2. 编码为协议报文 */
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len = sizeof(buf);

    int ret = prl_pmc_ccm_encode_heartbeat(&heartbeat, buf, &len);
    if (ret != PRL_OK) {
        printf("Failed to encode heartbeat: %d\n", ret);
        return;
    }

    /* 3. 通过网络发送 */
    osal_socket_send(socket_fd, buf, len);

    printf("CCM: Sent heartbeat to PMC (%zu bytes)\n", len);
}

/**
 * @brief CCM 发送遥测数据到 PMC
 */
void ccm_send_telemetry_to_pmc(int socket_fd, uint32_t tm_id)
{
    /* 1. 准备遥测消息 */
    prl_pmc_ccm_telemetry_t telemetry = {
        .tm_id = tm_id,
        .tm_source = 0x100,         /* 设备 ID */
        .timestamp_us = 1234567890,
        .data_type = 0x01,
        .data_length = 0,
    };

    /* 2. 准备遥测数据 */
    uint8_t tm_data[128];
    size_t tm_data_len = 64;
    memset(tm_data, 0xAA, tm_data_len);

    /* 3. 编码为协议报文 */
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len = sizeof(buf);

    int ret = prl_pmc_ccm_encode_telemetry(&telemetry, tm_data, tm_data_len, buf, &len);
    if (ret != PRL_OK) {
        printf("Failed to encode telemetry: %d\n", ret);
        return;
    }

    /* 4. 通过网络发送 */
    osal_socket_send(socket_fd, buf, len);

    printf("CCM: Sent telemetry to PMC (tm_id=%u, %zu bytes)\n", tm_id, len);
}

/**
 * @brief CCM 接收来自 PMC 的消息
 */
void ccm_recv_from_pmc(int socket_fd)
{
    uint8_t buf[PRL_MAX_PACKET_SIZE];
    size_t len;

    /* 1. 从网络接收 */
    int ret = osal_socket_recv(socket_fd, buf, &len);
    if (ret < 0) {
        return;
    }

    /* 2. 解析协议头，判断消息类型 */
    const prl_header_t *hdr = (const prl_header_t *)buf;

    /* 3. 根据消息类型解码 */
    switch (hdr->msg_type) {
        case PRL_PMC_CCM_MSG_COMMAND: {
            prl_pmc_ccm_command_t command;
            uint8_t *params;
            size_t params_len;

            ret = prl_pmc_ccm_decode_command(buf, len, &command, &params, &params_len);
            if (ret == PRL_OK) {
                printf("CCM: Received command from PMC (tc_id=%u, target=%u)\n",
                       command.tc_id, command.tc_target);

                /* 处理遥控指令 */

                /* 发送应答 */
                prl_pmc_ccm_ack_t ack = {
                    .ack_seq = hdr->seq,
                    .ack_type = PRL_PMC_CCM_MSG_COMMAND,
                    .result = 0,            /* 成功 */
                    .error_code = 0,
                    .data_length = 0,
                };

                uint8_t ack_buf[PRL_MAX_PACKET_SIZE];
                size_t ack_len = sizeof(ack_buf);
                prl_pmc_ccm_encode_ack(&ack, NULL, 0, ack_buf, &ack_len);
                osal_socket_send(socket_fd, ack_buf, ack_len);
            }
            break;
        }

        case PRL_PMC_CCM_MSG_FIRMWARE_UPDATE: {
            prl_pmc_ccm_firmware_update_t firmware;
            uint8_t *fw_data;
            size_t fw_data_len;

            ret = prl_pmc_ccm_decode_firmware_update(buf, len, &firmware,
                                                     &fw_data, &fw_data_len);
            if (ret == PRL_OK) {
                printf("CCM: Received firmware update (id=%u, offset=%u, size=%u)\n",
                       firmware.firmware_id, firmware.offset, firmware.chunk_size);
                /* 处理固件升级 */
            }
            break;
        }

        default:
            printf("CCM: Unknown message type: 0x%02x\n", hdr->msg_type);
            break;
    }
}

/* ========== 主函数示例 ========== */

int main(void)
{
    printf("PRL Protocol Usage Example\n");
    printf("===========================\n\n");

    /* 模拟 PMC 端 */
    printf("=== PMC Side ===\n");
    int pmc_socket = 0; /* 实际应用中需要创建 socket */
    pmc_send_heartbeat_to_ccm(pmc_socket);
    pmc_send_command_to_ccm(pmc_socket, 0x1001, 0x200);

    printf("\n");

    /* 模拟 CCM 端 */
    printf("=== CCM Side ===\n");
    int ccm_socket = 0; /* 实际应用中需要创建 socket */
    ccm_send_heartbeat_to_pmc(ccm_socket);
    ccm_send_telemetry_to_pmc(ccm_socket, 0x2001);

    return 0;
}
