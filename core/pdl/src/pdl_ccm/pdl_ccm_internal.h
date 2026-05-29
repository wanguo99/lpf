/************************************************************************
 * CCM 系统驱动内部头文件
 *
 * 说明：
 * - 定义内部数据结构和函数
 * - 不对外暴露
 ************************************************************************/

#ifndef PDL_CCM_INTERNAL_H
#define PDL_CCM_INTERNAL_H

#include "pdl_ccm.h"
#include "osal.h"

/* 以太网消息最大长度 */
#define CCM_ETH_MAX_MSG_SIZE    4096

/* 重试间隔 */
#define CCM_RETRY_INTERVAL_MS   100

/*
 * CCM 以太网消息结构
 */
typedef struct
{
    uint8_t msg_type;       /* 消息类型 */
    uint32_t seq_num;       /* 序列号 */
    uint32_t payload_len;   /* 负载长度 */
    uint8_t payload[CCM_ETH_MAX_MSG_SIZE];  /* 负载数据 */
} ccm_eth_msg_t;

/*
 * 以太网通信接口（内部模块）
 */
int32_t ccm_eth_init(const pdl_ccm_config_t *config, void **handle);
int32_t ccm_eth_deinit(void *handle);
int32_t ccm_eth_send(void *handle, const ccm_eth_msg_t *msg, uint32_t timeout_ms);
int32_t ccm_eth_recv(void *handle, ccm_eth_msg_t *msg, uint32_t timeout_ms);
int32_t ccm_eth_is_connected(void *handle);

/*
 * 协议处理接口（内部模块）
 */
int32_t ccm_protocol_encode_heartbeat(uint32_t sender_status,
                                       uint32_t link_quality,
                                       uint8_t *buf,
                                       size_t *len);

int32_t ccm_protocol_encode_telemetry(uint32_t tm_id,
                                       uint32_t tm_source,
                                       const uint8_t *data,
                                       size_t data_len,
                                       uint8_t *buf,
                                       size_t *len);

int32_t ccm_protocol_encode_command(uint32_t tc_id,
                                     uint32_t tc_target,
                                     uint32_t tc_action,
                                     const uint8_t *params,
                                     size_t params_len,
                                     uint8_t *buf,
                                     size_t *len);

int32_t ccm_protocol_encode_firmware_update(uint32_t firmware_id,
                                             uint32_t target_device,
                                             uint32_t firmware_version,
                                             uint32_t total_size,
                                             uint32_t offset,
                                             const uint8_t *data,
                                             size_t data_len,
                                             const uint8_t md5[16],
                                             uint8_t *buf,
                                             size_t *len);

int32_t ccm_protocol_encode_node_manage(uint32_t node_id,
                                         uint32_t operation,
                                         uint8_t *buf,
                                         size_t *len);

int32_t ccm_protocol_encode_power_control(uint32_t power_domain,
                                           uint32_t operation,
                                           uint8_t *buf,
                                           size_t *len);

int32_t ccm_protocol_encode_status_query(uint32_t query_type,
                                          uint32_t query_target,
                                          uint8_t *buf,
                                          size_t *len);

int32_t ccm_protocol_decode_telemetry(const uint8_t *buf,
                                       size_t len,
                                       uint32_t *tm_id,
                                       uint32_t *tm_source,
                                       uint8_t **data,
                                       size_t *data_len);

int32_t ccm_protocol_decode_command(const uint8_t *buf,
                                     size_t len,
                                     uint32_t *tc_id,
                                     uint32_t *tc_target,
                                     uint32_t *tc_action,
                                     uint8_t **params,
                                     size_t *params_len);

int32_t ccm_protocol_decode_ack(const uint8_t *buf,
                                 size_t len,
                                 uint32_t *ack_seq,
                                 uint8_t *result,
                                 uint16_t *error_code);

#endif /* PDL_CCM_INTERNAL_H */
