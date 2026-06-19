/************************************************************************
 * PDM protocol common definitions
 *
 * 功能：
 * - 协议魔数、版本定义
 * - 设备类型枚举
 * - 协议头结构
 * - 内部辅助函数声明
 ************************************************************************/

#ifndef PDM_PROTOCOL_COMMON_H
#define PDM_PROTOCOL_COMMON_H

#include "osal.h"

/* ========== Protocol Constants ========== */

#define PDM_PROTOCOL_MAGIC 0xAA55
#define PDM_PROTOCOL_VERSION 0x01
#define PDM_PROTOCOL_VERSION_MAJOR 0x00
#define PDM_PROTOCOL_VERSION_MINOR 0x01
#define PDM_PROTOCOL_HEADER_SIZE 20

#ifdef CONFIG_PDM_PROTOCOL_MAX_PAYLOAD_SIZE
#define PDM_PROTOCOL_MAX_PAYLOAD_SIZE CONFIG_PDM_PROTOCOL_MAX_PAYLOAD_SIZE
#else
#define PDM_PROTOCOL_MAX_PAYLOAD_SIZE 1024
#endif

#define PDM_PROTOCOL_MAX_PACKET_SIZE \
	(PDM_PROTOCOL_HEADER_SIZE + PDM_PROTOCOL_MAX_PAYLOAD_SIZE)

/* ========== Device Types ========== */

typedef enum {
	PDM_PROTOCOL_DEV_TYPE_INVALID = 0x00,
	PDM_PROTOCOL_DEV_TYPE_MCU = 0x01,
	PDM_PROTOCOL_DEV_TYPE_FPGA = 0x02,
} pdm_protocol_dev_type_t;

/* ========== Protocol Header ========== */

typedef struct {
	uint16_t magic;
	uint8_t version;
	uint8_t dev_type;
	uint8_t msg_type;
	uint8_t flags;
	uint16_t length;
	uint32_t seq;
	uint32_t timestamp;
	uint16_t crc16;
	uint16_t reserved;
} __attribute__((packed)) pdm_protocol_header_t;

/* ========== Internal Functions ========== */

/**
 * @brief 计算 CRC16 校验和
 *
 * @param[in] data 数据指针
 * @param[in] len 数据长度
 * @return CRC16 校验和
 */
uint16_t pdm_protocol_crc16(const uint8_t *data, uint16_t len);
uint32_t pdm_protocol_get_next_seq(void);
uint32_t pdm_protocol_get_timestamp(void);
void pdm_protocol_init_header(pdm_protocol_header_t *hdr, uint8_t dev_type,
			      uint8_t msg_type, uint16_t payload_len,
			      uint8_t flags);
int pdm_protocol_validate_header(const pdm_protocol_header_t *hdr,
				 uint8_t expected_dev_type);
void pdm_protocol_set_packet_crc(uint8_t *packet, size_t total_len);
bool pdm_protocol_verify_packet_crc(const uint8_t *packet, size_t total_len);

/**
 * @brief 验证设备类型是否有效
 *
 * @param[in] dev_type 设备类型
 * @return true 有效，false 无效
 */
bool pdm_protocol_is_device_type_valid(uint8_t dev_type);

/**
 * @brief 获取设备类型名称
 *
 * @param[in] dev_type 设备类型
 * @return 设备类型名称字符串（静态字符串，无需释放）
 */
const char *pdm_protocol_get_device_type_name(uint8_t dev_type);

/**
 * @brief 获取错误码描述
 *
 * @param[in] error_code 错误码（负数）
 * @return 错误码描述字符串
 */
const char *pdm_protocol_get_error_string(int32_t error_code);
void pdm_protocol_get_version(uint8_t *major, uint8_t *minor);
void pdm_protocol_reset_sequence(uint32_t seq);
uint32_t pdm_protocol_get_current_sequence(void);

#endif /* PDM_PROTOCOL_COMMON_H */
