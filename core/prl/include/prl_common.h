/**
 * @file prl_common.h
 * @brief Protocol Layer Common Definitions
 * @details 协议层通用定义，包括协议头、错误码、工具函数等
 */

#ifndef PRL_COMMON_H
#define PRL_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 协议魔数 */
#define PRL_MAGIC_NUMBER        0xAA55

/* 协议版本 */
#define PRL_VERSION_MAJOR       1
#define PRL_VERSION_MINOR       0
#define PRL_VERSION             ((PRL_VERSION_MAJOR << 8) | PRL_VERSION_MINOR)

/* 协议错误码 */
typedef enum {
    PRL_OK                  = 0,    /* 成功 */
    PRL_ERR_INVALID_PARAM   = -1,   /* 无效参数 */
    PRL_ERR_INVALID_MAGIC   = -2,   /* 无效魔数 */
    PRL_ERR_INVALID_VERSION = -3,   /* 无效版本 */
    PRL_ERR_INVALID_TYPE    = -4,   /* 无效消息类型 */
    PRL_ERR_INVALID_LENGTH  = -5,   /* 无效长度 */
    PRL_ERR_CRC_FAILED      = -6,   /* CRC 校验失败 */
    PRL_ERR_BUFFER_TOO_SMALL = -7,  /* 缓冲区太小 */
    PRL_ERR_ENCODE_FAILED   = -8,   /* 编码失败 */
    PRL_ERR_DECODE_FAILED   = -9,   /* 解码失败 */
} prl_error_t;

/* 协议通用头（所有协议共用） */
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint16_t version;       /* 协议版本 */
    uint8_t  msg_type;      /* 消息类型 */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 数据长度（不包括头和CRC） */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳（秒） */
    uint16_t crc16;         /* CRC16校验（整个报文） */
    uint16_t reserved;      /* 保留字段 */
} __attribute__((packed)) prl_header_t;

/* 协议头大小 */
#define PRL_HEADER_SIZE         sizeof(prl_header_t)

/* 最大报文长度 */
#define PRL_MAX_PAYLOAD_SIZE    4096
#define PRL_MAX_PACKET_SIZE     (PRL_HEADER_SIZE + PRL_MAX_PAYLOAD_SIZE)

/* 标志位定义 */
#define PRL_FLAG_ACK_REQUIRED   0x01    /* 需要应答 */
#define PRL_FLAG_IS_ACK         0x02    /* 是应答报文 */
#define PRL_FLAG_ENCRYPTED      0x04    /* 加密报文 */
#define PRL_FLAG_COMPRESSED     0x08    /* 压缩报文 */

/**
 * @brief 计算 CRC16
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 值
 */
uint16_t prl_calc_crc16(const uint8_t *data, size_t len);

/**
 * @brief 获取下一个序列号
 * @return 序列号
 */
uint32_t prl_get_next_seq(void);

/**
 * @brief 获取当前时间戳
 * @return 时间戳（秒）
 */
uint32_t prl_get_timestamp(void);

/**
 * @brief 初始化协议头
 * @param hdr 协议头指针
 * @param msg_type 消息类型
 * @param payload_len 负载长度
 * @param flags 标志位
 */
void prl_init_header(prl_header_t *hdr, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags);

/**
 * @brief 验证协议头
 * @param hdr 协议头指针
 * @param expected_type 期望的消息类型（0表示不检查）
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type);

/**
 * @brief 计算并设置报文 CRC
 * @param packet 报文指针
 * @param total_len 报文总长度
 */
void prl_set_packet_crc(uint8_t *packet, size_t total_len);

/**
 * @brief 验证报文 CRC
 * @param packet 报文指针
 * @param total_len 报文总长度
 * @return true 校验通过，false 校验失败
 */
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);

#ifdef __cplusplus
}
#endif

#endif /* PRL_COMMON_H */
