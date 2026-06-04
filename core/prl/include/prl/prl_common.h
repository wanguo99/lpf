/**
 * @file prl_common.h
 * @brief Protocol Layer Common Definitions
 */

#ifndef PRL_PRL_COMMON_H
#define PRL_PRL_COMMON_H

#include "osal/osal.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * PRL 使用 OSAL 标准错误码体系:
 * - 成功: OSAL_SUCCESS (0)
 * - 参数错误: OSAL_ERR_INVALID_PARAM, OSAL_EINVAL
 * - 缓冲区不足: OSAL_ENOBUFS
 * - 协议错误: OSAL_EPROTO, OSAL_EBADMSG
 */

/* ========== Protocol Version ========== */

#define PRL_VERSION_MAJOR           0
#define PRL_VERSION_MINOR           1

/* ========== Protocol Flags ========== */

#define PRL_FLAG_NONE               0x00
#define PRL_FLAG_IS_ACK             0x01
#define PRL_FLAG_NEED_ACK           0x02

/* ========== Protocol Constants ========== */

#define PRL_MAGIC              0xAA55
#define PRL_VERSION            0x01
#define PRL_HEADER_SIZE        20
#define PRL_MAX_PAYLOAD_SIZE   4096
#define PRL_MAX_PACKET_SIZE    (PRL_HEADER_SIZE + PRL_MAX_PAYLOAD_SIZE)

/* ========== Device Types ========== */

typedef enum {
    PRL_DEV_TYPE_MCU = 0x01,
    PRL_DEV_TYPE_CCM = 0x02,
    PRL_DEV_TYPE_PMC = 0x03,
    PRL_DEV_TYPE_GSC = 0x04,
    PRL_DEV_TYPE_POWER = 0x05,
    PRL_DEV_TYPE_SATELLITE = 0x06,
    PRL_DEV_TYPE_BMC = 0x07
} prl_dev_type_t;

/* ========== Protocol Header ========== */

typedef struct {
    uint16_t magic;
    uint8_t  version;
    uint8_t  dev_type;
    uint8_t  msg_type;
    uint8_t  flags;
    uint16_t length;
    uint32_t seq;
    uint32_t timestamp;
    uint16_t crc16;
    uint16_t reserved;
} __attribute__((packed)) prl_header_t;

/* ========== Internal Functions ========== */

uint16_t prl_calc_crc16(const uint8_t *data, size_t len);
uint32_t prl_get_next_seq(void);
uint32_t prl_get_timestamp(void);
void prl_init_header(prl_header_t *hdr, uint8_t dev_type, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags);

/**
 * @brief 验证协议头
 * @return OSAL_SUCCESS 成功，OSAL_EPROTO/OSAL_EBADMSG/OSAL_EINVAL 失败
 */
int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type);

void prl_set_packet_crc(uint8_t *packet, size_t total_len);
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);

#ifdef __cplusplus
}
#endif

#endif /* PRL_PRL_COMMON_H */
