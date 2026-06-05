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
#include "pdl.h"

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
 * @brief 通过设备名称初始化MCU驱动（便捷接口）
 *
 * 此接口内部会调用 PCONFIG_GetBoard() 和 PCONFIG_HW_FindMCU() 查询配置，
 * 然后调用 PDL_MCU_Init() 完成初始化。
 *
 * @param[in] device_name 设备名称（如 "power_mcu"）
 * @param[out] handle MCU句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_FOUND 设备未找到
 * @return OSAL_ERR_GENERIC 初始化失败
 *
 * @note 使用此接口前需要先调用 PCONFIG_Register() 注册平台配置
 */
int32_t PDL_MCU_InitByName(const char *device_name, pdl_mcu_handle_t *handle);

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
