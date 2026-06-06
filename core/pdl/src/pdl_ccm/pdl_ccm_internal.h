/************************************************************************
 * CCM 系统驱动内部头文件
 *
 * 说明：
 * - 定义内部数据结构和函数
 * - 不对外暴露
 ************************************************************************/

#ifndef PDL_CCM_INTERNAL_H
#define PDL_CCM_INTERNAL_H

#include "osal.h"
#include "pdl_ccm.h"

/* 以太网消息最大长度 */
#define CCM_ETH_MAX_MSG_SIZE    0x1000

/* 重试间隔 */
#define CCM_RETRY_INTERVAL_MS   0x64

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

#endif /* PDL_CCM_INTERNAL_H */
