/**
 * @file lpf_protocol_mcu.h
 * @brief MCU Device Protocol Messages
 * @details MCU（微控制器）设备的消息类型和结构体定义
 */

#ifndef LPF_PROTOCOL_MCU_H
#define LPF_PROTOCOL_MCU_H

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCU 消息类型
 * @note 使用设备类型 LPF_PROTOCOL_DEV_TYPE_MCU
 */
typedef enum {
	LPF_PROTOCOL_MCU_MSG_GET_VERSION = 0x01, /* 获取版本信息 */
	LPF_PROTOCOL_MCU_MSG_GET_STATUS = 0x02, /* 获取状态 */
	LPF_PROTOCOL_MCU_MSG_RESET = 0x03, /* 复位 */
	LPF_PROTOCOL_MCU_MSG_HEARTBEAT = 0x04, /* 心跳 */
	LPF_PROTOCOL_MCU_MSG_SET_CONFIG = 0x05, /* 设置配置 */
	LPF_PROTOCOL_MCU_MSG_GET_CONFIG = 0x06, /* 获取配置 */
	LPF_PROTOCOL_MCU_MSG_READ_DATA = 0x10, /* 读取数据 */
	LPF_PROTOCOL_MCU_MSG_WRITE_DATA = 0x11, /* 写入数据 */
	LPF_PROTOCOL_MCU_MSG_EXECUTE_CMD = 0x12, /* 执行命令 */
	LPF_PROTOCOL_MCU_MSG_POWER_ON = 0x20, /* 上电 */
	LPF_PROTOCOL_MCU_MSG_POWER_OFF = 0x21, /* 下电 */
	LPF_PROTOCOL_MCU_MSG_CUSTOM = 0xFF, /* 自定义命令 */
} lpf_protocol_mcu_msg_type_t;

/**
 * @brief MCU 版本信息
 */
typedef struct {
	uint8_t major; /* 主版本号 */
	uint8_t minor; /* 次版本号 */
	uint8_t patch; /* 补丁版本号 */
	uint8_t reserved; /* 保留 */
	uint32_t build_time; /* 编译时间戳 */
	char git_hash[8]; /* Git 提交哈希（前8字节） */
} lpf_protocol_mcu_version_t;

/**
 * @brief MCU 状态信息
 */
typedef struct {
	uint8_t state; /* 运行状态 */
	uint8_t error_code; /* 错误码 */
	uint16_t uptime; /* 运行时间（秒） */
	uint32_t cpu_usage; /* CPU 使用率（千分比） */
	uint32_t mem_usage; /* 内存使用率（千分比） */
} lpf_protocol_mcu_status_t;

#ifdef __cplusplus
}
#endif

#endif /* LPF_PROTOCOL_MCU_H */
