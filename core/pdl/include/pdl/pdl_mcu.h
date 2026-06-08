/************************************************************************
 * MCU外设驱动对外API
 *
 * 功能：
 * - 提供MCU通信、状态监控、固件升级等功能
 * - 支持CAN/串口等多种通信接口
 * - 设备状态管理和错误处理
 *
 * 设计理念：
 * - 对外只暴露业务接口（版本查询、状态读取、命令执行等）
 * - 内部封装通信细节（CAN/串口协议、帧封装、CRC校验等）
 * - 支持多种通信方式，由配置决定使用哪种
 ************************************************************************/

#ifndef PDL_MCU_H
#define PDL_MCU_H

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
 * 说明：
 * - 使用 Tagged Union 模式表示互斥的硬件接口选择
 * - interface 字段指定使用哪个接口（discriminator/tag）
 * - hw union 包含各接口的具体配置（只占用最大成员的空间）
 * - 避免同时定义多个接口造成的内存浪费
 */
typedef struct
{
	char name[0x40];                  /* MCU名称 */
	pdl_mcu_interface_t interface;    /* 通信接口类型（必须先设置） */

	/* 硬件接口配置（互斥选择，由interface字段决定使用哪个） */
	union {
		/* CAN配置 - 嵌入 HAL 配置 */
		struct {
			const char *device;           /* CAN设备（如can0，传递给 HAL） */
			uint32_t bitrate;             /* 波特率（传递给 HAL） */
			uint32_t rx_timeout;          /* 接收超时（传递给 HAL） */
			uint32_t tx_timeout;          /* 发送超时（传递给 HAL） */
			uint32_t tx_id;               /* 发送CAN ID（PDL层使用） */
			uint32_t rx_id;               /* 接收CAN ID（PDL层使用） */
		} can;

		/* 串口配置 - 嵌入 HAL 配置 */
		struct {
			const char *device;           /* 串口设备（如/dev/ttyS1，传递给 HAL） */
			uint32_t baudrate;            /* 波特率（传递给 HAL） */
			uint8_t data_bits;            /* 数据位（5-8，传递给 HAL） */
			uint8_t stop_bits;            /* 停止位（1-2，传递给 HAL） */
			uint8_t parity;               /* 校验位（传递给 HAL） */
			uint8_t flow_control;         /* 流控（传递给 HAL） */
		} serial;
	} hw;

	/* 通用配置 */
	uint32_t cmd_timeout_ms;            /* 命令超时（ms） */
	uint32_t retry_count;               /* 重试次数 */
	/* 注意：串口通信强制启用 CRC 校验以确保数据完整性（航天环境要求） */
} pdl_mcu_config_t;

/*
 * MCU服务句柄
 */
typedef void* pdl_mcu_handle_t;

/*
 * MCU设备状态枚举
 */
typedef enum
{
	PDL_MCU_STATE_UNINITIALIZED = 0x00,  /* 未初始化 */
	PDL_MCU_STATE_INIT          = 0x01,  /* 已初始化 */
	PDL_MCU_STATE_READY         = 0x02,  /* 就绪（通信正常） */
	PDL_MCU_STATE_BUSY          = 0x03,  /* 忙碌（命令执行中） */
	PDL_MCU_STATE_ERROR         = 0x04,  /* 错误状态 */
	PDL_MCU_STATE_OFFLINE       = 0x05   /* 离线（通信失败） */
} pdl_mcu_state_t;

/*
 * MCU版本信息
 */
typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t build;
	char version_string[32];
} pdl_mcu_version_t;

/*
 * MCU状态信息
 */
typedef struct
{
	bool online;                      /* 在线状态 */
	pdl_mcu_state_t state;            /* 设备状态 */
	uint32_t uptime_sec;                /* 运行时间 */
	uint8_t error_code;                 /* 错误码 */
	float temperature;                /* 温度 */
	uint16_t voltage_mv;                /* 电压（mV） */
	uint64_t timestamp_us;              /* 数据采集时间戳（微秒） */
} pdl_mcu_status_t;

/**
 * @brief 初始化MCU驱动
 *
 * @param[in] config 配置参数
 * @param[out] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_MCU_Init(const pdl_mcu_config_t *config, pdl_mcu_handle_t *handle);

/**
 * @brief 反初始化MCU驱动
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_Deinit(pdl_mcu_handle_t handle);

/**
 * @brief 获取MCU版本
 *
 * @param[in] handle MCU句柄
 * @param[out] version 版本信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_GetVersion(pdl_mcu_handle_t handle, pdl_mcu_version_t *version);

/**
 * @brief 获取MCU状态
 *
 * @param[in] handle MCU句柄
 * @param[out] status 状态信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_GetStatus(pdl_mcu_handle_t handle, pdl_mcu_status_t *status);

/**
 * @brief MCU复位
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_Reset(pdl_mcu_handle_t handle);

/**
 * @brief 读取MCU寄存器
 *
 * @param[in] handle MCU句柄
 * @param[in] reg_addr 寄存器地址
 * @param[out] value 读取值
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_ReadRegister(pdl_mcu_handle_t handle, uint8_t reg_addr, uint8_t *value);

/**
 * @brief 写入MCU寄存器
 *
 * @param[in] handle MCU句柄
 * @param[in] reg_addr 寄存器地址
 * @param[in] value 写入值
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_WriteRegister(pdl_mcu_handle_t handle, uint8_t reg_addr, uint8_t value);

/**
 * @brief 发送自定义命令到MCU
 *
 * @param[in] handle MCU句柄
 * @param[in] cmd_code 命令码
 * @param[in] data 命令数据
 * @param[in] data_len 数据长度
 * @param[out] response 响应缓冲区
 * @param[in] resp_size 缓冲区大小
 * @param[out] actual_size 实际响应长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_MCU_SendCommand(pdl_mcu_handle_t handle,
                          uint8_t cmd_code,
                          const uint8_t *data,
                          uint32_t data_len,
                          uint8_t *response,
                          uint32_t resp_size,
                          uint32_t *actual_size);

/**
 * @brief MCU固件升级
 *
 * @param[in] handle MCU句柄
 * @param[in] firmware_path 固件文件路径
 * @param[in] progress_callback 进度回调（可选）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_MCU_FirmwareUpdate(pdl_mcu_handle_t handle,
                             const char *firmware_path,
                             void (*progress_callback)(uint32_t percent));

/**
 * @brief 获取MCU设备状态
 *
 * @param[in] handle MCU句柄
 *
 * @return pdl_mcu_state_t 设备状态枚举
 */
pdl_mcu_state_t PDL_MCU_GetDeviceState(pdl_mcu_handle_t handle);

/**
 * @brief 获取设备状态名称
 *
 * @param[in] state 设备状态
 *
 * @return 状态名称字符串
 */
const char* PDL_MCU_GetStateName(pdl_mcu_state_t state);

#endif /* PDL_MCU_H */
