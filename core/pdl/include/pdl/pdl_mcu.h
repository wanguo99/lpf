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
 * - 配置类型由 PCONFIG 定义，PDL 使用
 ************************************************************************/

#ifndef PDL_MCU_H
#define PDL_MCU_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * MCU 句柄和状态类型
 *===========================================================================*/

/**
 * @brief MCU服务句柄
 */
typedef void* pdl_mcu_handle_t;

/**
 * @brief MCU设备状态枚举
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

/**
 * @brief MCU版本信息
 */
typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t build;
	char version_string[32];
} pdl_mcu_version_t;

/**
 * @brief MCU状态信息
 */
typedef struct
{
	bool online;                      /* 在线状态 */
	pdl_mcu_state_t state;            /* 设备状态 */
	uint32_t uptime_sec;              /* 运行时间 */
	uint8_t error_code;               /* 错误码 */
	float temperature;                /* 温度 */
	uint16_t voltage_mv;              /* 电压（mV） */
	uint64_t timestamp_us;            /* 数据采集时间戳（微秒） */
} pdl_mcu_status_t;

/*===========================================================================
 * MCU 驱动 API
 *===========================================================================*/

/**
 * @brief 初始化MCU驱动
 *
 * @param[in] index MCU设备索引（从 PCONFIG 获取配置）
 * @param[out] handle 返回MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 函数内部会：
 *       1. 调用 PCONFIG_GetBoard() 获取平台配置
 *       2. 调用 PCONFIG_HW_GetMCU(platform, index) 获取 MCU 配置
 *       3. 检查配置是否启用
 *       4. 将 PCONFIG 配置转换为 HAL 配置并初始化硬件
 */
int32_t PDL_MCU_init(uint32_t index, pdl_mcu_handle_t *handle);

/**
 * @brief 反初始化MCU驱动
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_deinit(pdl_mcu_handle_t handle);

/**
 * @brief 获取MCU版本
 *
 * @param[in] handle MCU句柄
 * @param[out] version 版本信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_get_version(pdl_mcu_handle_t handle, pdl_mcu_version_t *version);

/**
 * @brief 获取MCU状态
 *
 * @param[in] handle MCU句柄
 * @param[out] status 状态信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_get_status(pdl_mcu_handle_t handle, pdl_mcu_status_t *status);

/**
 * @brief 发送命令到MCU
 *
 * @param[in] handle MCU句柄
 * @param[in] cmd 命令字
 * @param[in] data 命令数据
 * @param[in] data_len 数据长度
 * @param[out] response 响应数据
 * @param[in] response_max 响应缓冲区大小
 * @param[out] response_len 实际响应长度
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_send_command(pdl_mcu_handle_t handle,
			  uint8_t cmd,
			  const uint8_t *data,
			  uint32_t data_len,
			  uint8_t *response,
			  uint32_t response_max,
			  uint32_t *response_len);

/**
 * @brief 复位MCU
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_reset(pdl_mcu_handle_t handle);

/**
 * @brief MCU 测试调用链（调试接口）
 *
 * @param[in] index MCU设备索引
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 预留的调试接口，验证完整调用链：
 *       APP → PDL_MCU_test_call → PCONFIG → HAL_XXX_TestCall
 */
int32_t PDL_MCU_test_call(uint32_t index);

#endif /* PDL_MCU_H */
