/************************************************************************
 * PDL 配置类型对外定义
 *
 * 功能：
 * - 定义 PDL 层各设备的配置结构体
 * - 作为独立模块，被 PCL 和 PDL 共同依赖
 * - 解决 PCL 和 PDL 之间的循环依赖问题
 *
 * 架构设计：
 * - PCL 依赖 types（使用配置类型）
 * - PDL 依赖 types（使用配置类型）
 * - PDL 依赖 PCL（使用配置数据）
 * - 依赖关系：PCL→types←PDL, PDL→PCL（无循环）
 *
 * 设计原则：
 * - 只包含数据类型定义，不包含函数声明
 * - 不依赖其他 core 模块（除了 osal_types.h）
 * - 保持类型定义的稳定性和向后兼容性
 ************************************************************************/

#ifndef PDL_TYPES_H
#define PDL_TYPES_H

#include "osal.h"

/*===========================================================================
 * MCU 配置类型
 *===========================================================================*/

/**
 * @brief MCU通信接口类型
 */
typedef enum
{
	PDL_MCU_INTERFACE_CAN = 0x00,     /* CAN总线 */
	PDL_MCU_INTERFACE_SERIAL = 0x01,  /* 串口 */
	PDL_MCU_INTERFACE_I2C = 0x02,     /* I2C（预留） */
	PDL_MCU_INTERFACE_SPI = 0x03      /* SPI（预留） */
} pdl_mcu_interface_t;

/**
 * @brief MCU配置
 *
 * 说明：直接嵌入 HAL 层配置结构体，避免重复定义
 */
typedef struct
{
	char name[0x40];                  /* MCU名称 */
	pdl_mcu_interface_t interface;  /* 通信接口 */

	/* CAN配置 - 嵌入 HAL 配置 */
	struct {
		const char *device;           /* CAN设备（如can0，传递给 HAL） */
		uint32_t bitrate;               /* 波特率（传递给 HAL） */
		uint32_t rx_timeout;            /* 接收超时（传递给 HAL） */
		uint32_t tx_timeout;            /* 发送超时（传递给 HAL） */
		uint32_t tx_id;                 /* 发送CAN ID（PDL层使用） */
		uint32_t rx_id;                 /* 接收CAN ID（PDL层使用） */
	} can;

	/* 串口配置 - 嵌入 HAL 配置 */
	struct {
		const char *device;           /* 串口设备（如/dev/ttyS1，传递给 HAL） */
		uint32_t baudrate;              /* 波特率（传递给 HAL） */
		uint8_t data_bits;              /* 数据位（5-8，传递给 HAL） */
		uint8_t stop_bits;              /* 停止位（1-2，传递给 HAL） */
		uint8_t parity;                /* 校验位（传递给 HAL） */
		uint8_t flow_control;           /* 流控（传递给 HAL） */
	} serial;

	/* 通用配置 */
	uint32_t cmd_timeout_ms;            /* 命令超时（ms） */
	uint32_t retry_count;               /* 重试次数 */
	/* 注意：串口通信强制启用 CRC 校验以确保数据完整性（航天环境要求） */
} pdl_mcu_config_t;

/*===========================================================================
 * BMC 配置类型
 *===========================================================================*/

/**
 * @brief BMC通信通道类型
 */
typedef enum
{
	PDL_BMC_CHANNEL_NETWORK = 0x00,  /* 网络通道（IPMI over LAN） */
	PDL_BMC_CHANNEL_SERIAL  = 0x01   /* 串口通道（IPMI over Serial） */
} pdl_bmc_channel_t;

/**
 * @brief BMC协议类型
 */
typedef enum
{
	PDL_BMC_PROTOCOL_IPMI = 0x00,    /* IPMI协议 */
	PDL_BMC_PROTOCOL_REDFISH = 0x01  /* Redfish协议 */
} pdl_bmc_protocol_t;

/**
 * @brief BMC配置
 *
 * 说明：直接嵌入 HAL 层配置结构体，避免重复定义
 */
typedef struct
{
	/* 网络配置 */
	struct {
		bool enabled;             /* 是否启用 */
		const char *ip_addr;      /* IP地址 */
		uint16_t port;              /* 端口（默认623） */
		const char *username;     /* 用户名 */
		const char *password;     /* 密码 */
		uint32_t timeout_ms;        /* 超时时间 */
	} network;

	/* 串口配置 - 嵌入 HAL 配置 */
	struct {
		bool enabled;             /* 是否启用 */
		const char *device;       /* 串口设备（传递给 HAL） */
		uint32_t baudrate;          /* 波特率（传递给 HAL） */
		uint8_t data_bits;          /* 数据位（传递给 HAL，默认8） */
		uint8_t stop_bits;          /* 停止位（传递给 HAL，默认1） */
		uint8_t parity;             /* 校验位（传递给 HAL，默认NONE） */
		uint32_t timeout_ms;        /* 超时时间 */
	} serial;

	/* 服务配置 */
	pdl_bmc_channel_t primary_channel;  /* 主通道 */
	bool auto_switch;             /* 自动切换通道 */
	uint32_t retry_count;           /* 重试次数 */
	uint32_t health_check_interval; /* 健康检查间隔(ms) */
} pdl_bmc_config_t;

/*===========================================================================
 * Satellite 配置类型
 *===========================================================================*/

/**
 * @brief 卫星平台服务配置
 *
 * 说明：直接嵌入 HAL 层配置结构体，避免重复定义
 */
typedef struct
{
	/* CAN配置 - 嵌入 HAL 配置 */
	const char *can_device;        /* CAN设备名（传递给 HAL） */
	uint32_t can_bitrate;            /* CAN波特率（传递给 HAL） */
	uint32_t can_rx_timeout;         /* CAN接收超时（传递给 HAL） */
	uint32_t can_tx_timeout;         /* CAN发送超时（传递给 HAL） */

	/* 业务配置 */
	uint32_t heartbeat_interval_ms;  /* 心跳间隔(ms) */
	uint32_t cmd_timeout_ms;         /* 命令超时(ms) */
} pdl_satellite_config_t;

/*===========================================================================
 * Watchdog 配置类型
 *===========================================================================*/

/**
 * @brief Watchdog工作模式
 */
typedef enum
{
	PDL_WATCHDOG_MODE_MANUAL = 0x00,    /* 手动模式：应用自己调用Kick */
	PDL_WATCHDOG_MODE_AUTO = 0x01       /* 自动模式：PDL内部线程自动喂狗 */
} pdl_watchdog_mode_t;

/**
 * @brief Watchdog配置
 */
typedef struct
{
	char name[0x40];                  /* Watchdog名称 */
	const char *device;             /* 设备路径（如/dev/watchdog） */
	uint32_t timeout_sec;           /* 超时时间（秒） */
	pdl_watchdog_mode_t mode;           /* 工作模式 */
	uint32_t kick_interval_ms;      /* 自动模式下的喂狗间隔（毫秒） */
	bool enable_on_init;            /* 初始化时是否启用看门狗 */
} pdl_watchdog_config_t;

#endif /* PDL_TYPES_H */
