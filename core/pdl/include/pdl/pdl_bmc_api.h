/************************************************************************
 * BMC通信服务对外API
 *
 * 功能：
 * - 与带BMC的载荷通信（IPMI/Redfish协议）
 * - 支持多通道（网络/串口）自动切换
 * - 电源控制、状态查询、传感器读取
 * - 故障检测和自动恢复
 ************************************************************************/

#ifndef PDL_BMC_API_H
#define PDL_BMC_API_H

#include "osal/osal.h"
#include "pdl_types_api.h"

/*
 * BMC服务句柄
 */
typedef void* pdl_bmc_handle_t;

/*
 * 电源状态
 */
typedef enum
{
	PDL_BMC_POWER_OFF = 0,
	PDL_BMC_POWER_ON  = 1,
	PDL_BMC_POWER_UNKNOWN = 2
} pdl_bmc_power_state_t;

/*
 * BMC状态
 */
typedef struct
{
	pdl_bmc_power_state_t power_state;
	bool bmc_ready;
	uint32_t uptime_sec;
	float cpu_temp;
	float inlet_temp;
	uint64_t timestamp_us;  /* 数据采集时间戳（微秒） */
} pdl_bmc_status_t;

/*
 * 传感器类型
 */
typedef enum
{
	PDL_BMC_SENSOR_TEMP = 0,      /* 温度 */
	PDL_BMC_SENSOR_VOLTAGE = 1,   /* 电压 */
	PDL_BMC_SENSOR_CURRENT = 2,   /* 电流 */
	PDL_BMC_SENSOR_FAN = 3        /* 风扇转速 */
} pdl_bmc_sensor_type_t;

/*
 * 传感器读数
 */
typedef struct
{
	pdl_bmc_sensor_type_t type;
	char name[64];
	float value;
	char unit[16];
	bool valid;
	uint64_t timestamp_us;  /* 数据采集时间戳（微秒） */
} pdl_bmc_sensor_reading_t;

/**
 * @brief 初始化BMC服务
 *
 * @param[in] config 配置参数
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_BMC_Init(const pdl_bmc_config_t *config,
                   pdl_bmc_handle_t *handle);

/**
 * @brief 通过设备名称初始化BMC服务（便捷接口）
 *
 * 此接口内部会调用 PCONFIG_GetBoard() 和 PCONFIG_HW_FindBMC() 查询配置，
 * 然后调用 PDL_BMC_Init() 完成初始化。
 *
 * @param[in] device_name 设备名称（如 "payload_bmc"）
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_FOUND 设备未找到
 * @return OSAL_ERR_GENERIC 初始化失败
 *
 * @note 使用此接口前需要先调用 PCONFIG_Register() 注册平台配置
 */
int32_t PDL_BMC_InitByName(const char *device_name, pdl_bmc_handle_t *handle);

/**
 * @brief 反初始化BMC服务
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_Deinit(pdl_bmc_handle_t handle);

/**
 * @brief 电源开机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_BMC_PowerOn(pdl_bmc_handle_t handle);

/**
 * @brief 电源关机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_PowerOff(pdl_bmc_handle_t handle);

/**
 * @brief 电源复位
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_PowerReset(pdl_bmc_handle_t handle);

/**
 * @brief 查询电源状态
 *
 * @param[in] handle 服务句柄
 * @param[out] state 电源状态
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_GetPowerState(pdl_bmc_handle_t handle,
                            pdl_bmc_power_state_t *state);

/**
 * @brief 读取传感器
 *
 * @param[in] handle 服务句柄
 * @param[in] type 传感器类型
 * @param[out] readings 读数数组
 * @param[in] max_count 数组大小
 * @param[out] actual_count 实际读取数量
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_ReadSensors(pdl_bmc_handle_t handle,
                          pdl_bmc_sensor_type_t type,
                          pdl_bmc_sensor_reading_t *readings,
                          uint32_t max_count,
                          uint32_t *actual_count);

/**
 * @brief 执行原始IPMI命令
 *
 * @param[in] handle 服务句柄
 * @param[in] cmd 命令字符串
 * @param[out] response 响应缓冲区
 * @param[in] resp_size 缓冲区大小
 *
 * @return 实际接收字节数
 * @return <0 错误码
 */
int32_t PDL_BMC_ExecuteCommand(pdl_bmc_handle_t handle,
                             const char *cmd,
                             char *response,
                             uint32_t resp_size);

/**
 * @brief 切换通信通道
 *
 * @param[in] handle 服务句柄
 * @param[in] channel 目标通道
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_SwitchChannel(pdl_bmc_handle_t handle,
                            pdl_bmc_channel_t channel);

/**
 * @brief 获取当前通道
 *
 * @param[in] handle 服务句柄
 *
 * @return pdl_bmc_channel_t 当前通道
 */
pdl_bmc_channel_t PDL_BMC_GetChannel(pdl_bmc_handle_t handle);

/**
 * @brief 检查连接状态
 *
 * @param[in] handle 服务句柄
 *
 * @return true 已连接
 * @return false 未连接
 */
bool PDL_BMC_IsConnected(pdl_bmc_handle_t handle);

/**
 * @brief 获取服务统计信息
 *
 * @param[in] handle 服务句柄
 * @param[out] cmd_count 命令总数
 * @param[out] success_count 成功数
 * @param[out] fail_count 失败数
 * @param[out] switch_count 通道切换次数
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_GetStats(pdl_bmc_handle_t handle,
                       uint32_t *cmd_count,
                       uint32_t *success_count,
                       uint32_t *fail_count,
                       uint32_t *switch_count);

#endif /* PDL_BMC_API_H */
