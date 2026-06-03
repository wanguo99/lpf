/**
 * @file prl_types.h
 * @brief Protocol Layer Public Types
 * @details PRL 协议层公共类型定义，包括协议头、设备类型、错误码、标志位等
 */

#ifndef PRL_TYPES_H
#define PRL_TYPES_H

/* PRL 层必须且只能依赖 OSAL，不依赖标准库 */
#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 协议魔数 */
#define PRL_MAGIC_NUMBER        0xAA55

/* 协议版本 */
#define PRL_VERSION_MAJOR       1
#define PRL_VERSION_MINOR       0
#define PRL_VERSION             ((PRL_VERSION_MAJOR << 4) | PRL_VERSION_MINOR)

/* 设备类型定义 */
typedef enum {
	PRL_DEV_TYPE_UNKNOWN    = 0x00,     /* 未知设备 */
	PRL_DEV_TYPE_MCU        = 0x01,     /* 微控制器 */
	PRL_DEV_TYPE_CCM        = 0x02,     /* 通信管理板 */
	PRL_DEV_TYPE_PMC        = 0x03,     /* 载荷管理器 */
	PRL_DEV_TYPE_GSC        = 0x04,     /* 地面站控制器 */
	PRL_DEV_TYPE_SATELLITE  = 0x05,     /* 卫星平台 */
	PRL_DEV_TYPE_POWER      = 0x06,     /* 电源板 */
	PRL_DEV_TYPE_BMC        = 0x07,     /* 基板管理控制器 */
} prl_dev_type_t;

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
	PRL_ERR_INVALID_DEV_TYPE = -10, /* 无效设备类型 */
} prl_error_t;

/* 协议通用头（所有设备共用） */
typedef struct {
	uint16_t magic;         /* 魔数：0xAA55 */
	uint8_t  version;       /* 协议版本 */
	uint8_t  dev_type;      /* 设备类型 */
	uint8_t  msg_type;      /* 消息类型 */
	uint8_t  flags;         /* 标志位 */
	uint16_t length;        /* 数据长度（不包括头） */
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

#ifdef __cplusplus
}
#endif

#endif /* PRL_TYPES_H */
