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
 * - 配置类型由 LPF_CONFIG 定义，LPF MCU service 使用
 ************************************************************************/

#ifndef LPF_MCU_SERVICE_H
#define LPF_MCU_SERVICE_H

#include "osal.h"
#include "lpf/lpf_mcu.h"

#define LPF_MCU_SERVICE_VERSION_MAJOR 0x01
#define LPF_MCU_SERVICE_VERSION_MINOR 0x00
#define LPF_MCU_SERVICE_VERSION_PATCH 0x00

/*===========================================================================
 * MCU 句柄和状态类型
 *===========================================================================*/

/**
 * @brief MCU服务句柄
 */
typedef void *lpf_mcu_handle_t;

typedef enum lpf_mcu_state lpf_mcu_state_t;

/**
 * @brief MCU版本信息
 */
typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t build;
	char version_string[32];
} lpf_mcu_version_t;

/**
 * @brief MCU状态信息
 */
typedef struct {
	bool online; /* 在线状态 */
	lpf_mcu_state_t state; /* 设备状态 */
	uint32_t uptime_sec; /* 运行时间 */
	uint8_t error_code; /* 错误码 */
	int32_t temperature_milli_celsius; /* 温度（毫摄氏度） */
	uint16_t voltage_mv; /* 电压（mV） */
	uint64_t timestamp_us; /* 数据采集时间戳（微秒） */
} lpf_mcu_status_t;

/**
 * @brief MCU 简单命令参数（无发送数据）
 */
typedef struct {
	uint8_t cmd; /* 命令字 */
	uint8_t *response; /* 响应缓冲区 */
	uint32_t response_max; /* 响应缓冲区大小 */
	uint32_t response_len; /* 实际响应长度（输出） */
} lpf_mcu_cmd_t;

/**
 * @brief MCU 数据命令参数（带发送数据）
 */
typedef struct {
	uint8_t cmd; /* 命令字 */
	const uint8_t *data; /* 发送数据 */
	uint32_t data_len; /* 发送数据长度 */
	uint8_t *response; /* 响应缓冲区 */
	uint32_t response_max; /* 响应缓冲区大小 */
	uint32_t response_len; /* 实际响应长度（输出） */
} lpf_mcu_data_t;

/*===========================================================================
 * MCU 驱动 API
 *===========================================================================*/

/**
 * @brief 初始化MCU驱动
 *
 * @param[in] index MCU设备索引（从 LPF_CONFIG 获取配置）
 * @param[out] handle 返回MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 函数内部会：
 *       1. 调用 lpf_config_get_board() 获取平台配置
 *       2. 调用 lpf_config_hw_get_mcu(platform, index) 获取 MCU 配置
 *       3. 检查配置是否启用
 *       4. 将 LPF_CONFIG 配置转换为 LPF HW 配置并初始化硬件
 */
lpf_mcu_handle_t lpf_mcu_get(uint32_t index);
int32_t lpf_mcu_init(uint32_t index, lpf_mcu_handle_t *handle);

/**
 * @brief 反初始化MCU驱动
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_deinit(lpf_mcu_handle_t handle);

/**
 * @brief 获取MCU版本
 *
 * @param[in] handle MCU句柄
 * @param[out] version 版本信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_get_version(lpf_mcu_handle_t handle,
							lpf_mcu_version_t *version);

/**
 * @brief 获取MCU状态
 *
 * @param[in] handle MCU句柄
 * @param[out] status 状态信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_get_status(lpf_mcu_handle_t handle, lpf_mcu_status_t *status);

/**
 * @brief 发送简单命令到MCU（无发送数据）
 *
 * @param[in] handle MCU句柄
 * @param[in,out] cmd 命令参数
 *
 * @return OSAL_SUCCESS 成功
 *
 * @note 适用于 GET_VERSION, GET_STATUS, RESET 等无数据命令
 */
int32_t lpf_mcu_send_cmd(lpf_mcu_handle_t handle, lpf_mcu_cmd_t *cmd);

/**
 * @brief 发送数据命令到MCU（带发送数据）
 *
 * @param[in] handle MCU句柄
 * @param[in,out] data 数据命令参数
 *
 * @return OSAL_SUCCESS 成功
 *
 * @note 适用于 READ_DATA, WRITE_DATA, EXECUTE_CMD 等带数据命令
 */
int32_t lpf_mcu_send_data(lpf_mcu_handle_t handle, lpf_mcu_data_t *data);

/**
 * @brief 读取 MCU 数据
 *
 * @param[in] handle MCU句柄
 * @param[in] addr 读取地址
 * @param[out] data 输出缓冲区
 * @param[in] size 读取长度
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_read_data(lpf_mcu_handle_t handle, uint32_t addr, uint8_t *data,
						  uint32_t size);

/**
 * @brief 写入 MCU 数据
 *
 * @param[in] handle MCU句柄
 * @param[in] addr 写入地址
 * @param[in] data 输入数据
 * @param[in] size 写入长度
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_write_data(lpf_mcu_handle_t handle, uint32_t addr,
						   const uint8_t *data, uint32_t size);

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
int32_t lpf_mcu_send_command(lpf_mcu_handle_t handle, uint8_t cmd,
							 const uint8_t *data, uint32_t data_len,
							 uint8_t *response, uint32_t response_max,
							 uint32_t *response_len);

/**
 * @brief 复位MCU
 *
 * @param[in] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_mcu_reset(lpf_mcu_handle_t handle);

#endif /* LPF_MCU_SERVICE_H */
