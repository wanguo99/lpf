/************************************************************************
 * PRL 公共定义
 *
 * 功能：
 * - 协议魔数、版本定义
 * - 设备类型枚举
 * - 协议头结构
 * - 内部辅助函数声明
 ************************************************************************/

#ifndef PRL_COMMON_H
#define PRL_COMMON_H

#include <stdint.h>
#include <stdbool.h>

/* ========== Protocol Constants ========== */

#define PRL_MAGIC              0xAA55
#define PRL_VERSION            0x01
#define PRL_VERSION_MAJOR      0x00
#define PRL_VERSION_MINOR      0x01
#define PRL_HEADER_SIZE        20
#define PRL_MAX_PAYLOAD_SIZE   1024
#define PRL_MAX_PACKET_SIZE    (PRL_HEADER_SIZE + PRL_MAX_PAYLOAD_SIZE)

/* ========== Device Types ========== */

typedef enum {
	PRL_DEV_TYPE_MCU = 0x01,
	PRL_DEV_TYPE_PMC = 0x02,
	PRL_DEV_TYPE_GSC = 0x04,
	PRL_DEV_TYPE_POWER = 0x05
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

/**
 * @brief 计算 CRC16 校验和
 *
 * @param[in] data 数据指针
 * @param[in] len 数据长度
 * @return CRC16 校验和
 */
uint16_t prl_crc16(const uint8_t *data, uint16_t len);

/**
 * @brief 验证设备类型是否有效
 *
 * @param[in] dev_type 设备类型
 * @return true 有效，false 无效
 *
 * @note 有效的设备类型范围：PRL_DEV_TYPE_MCU ~ PRL_DEV_TYPE_POWER
 */
bool PRL_IsDeviceTypeValid(uint8_t dev_type);

/**
 * @brief 获取设备类型名称
 *
 * @param[in] dev_type 设备类型
 * @return 设备类型名称字符串（静态字符串，无需释放）
 */
const char *PRL_GetDeviceTypeName(uint8_t dev_type);

/**
 * @brief 获取错误码描述
 *
 * @param[in] error_code 错误码（负数）
 * @return 错误码描述字符串
 */
const char *PRL_GetErrorString(int32_t error_code);

#endif /* PRL_COMMON_H */
