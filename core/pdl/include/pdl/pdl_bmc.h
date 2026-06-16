/************************************************************************
 * BMC通信服务对外API
 *
 * 功能：
 * - 与带BMC的载荷通信（IPMI/Redfish协议）
 * - 支持多通道（网络/串口）自动切换
 * - 电源控制、状态查询、传感器读取
 * - 故障检测和自动恢复
 *
 * 设计理念：
 * - 配置类型由 PCONFIG 定义，PDL 使用
 ************************************************************************/

#ifndef PDL_BMC_H
#define PDL_BMC_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * BMC 句柄和状态类型
 *===========================================================================*/

/**
 * @brief BMC服务句柄
 */
typedef void* pdl_bmc_handle_t;

/**
 * @brief 电源状态
 */
typedef enum
{
	PDL_BMC_POWER_OFF = 0x00,
	PDL_BMC_POWER_ON  = 0x01,
	PDL_BMC_POWER_UNKNOWN = 0x02
} pdl_bmc_power_state_t;

/**
 * @brief BMC状态
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

/**
 * @brief 传感器类型
 */
typedef enum
{
	PDL_BMC_SENSOR_TEMP = 0x00,      /* 温度 */
	PDL_BMC_SENSOR_VOLTAGE = 0x01,   /* 电压 */
	PDL_BMC_SENSOR_CURRENT = 0x02,   /* 电流 */
	PDL_BMC_SENSOR_FAN = 0x03        /* 风扇转速 */
} pdl_bmc_sensor_type_t;

/**
 * @brief 传感器读数
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

/*===========================================================================
 * BMC 驱动 API
 *===========================================================================*/

/**
 * @brief 初始化BMC服务
 *
 * @param[in] index BMC设备索引（从 PCONFIG 获取配置）
 * @param[out] handle 返回BMC句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 函数内部会：
 *       1. 调用 PCONFIG_GetBoard() 获取平台配置
 *       2. 调用 PCONFIG_HW_GetBMC(platform, index) 获取 BMC 配置
 *       3. 检查配置是否启用
 *       4. 将 PCONFIG 配置转换为 HAL 配置并初始化硬件
 */
int32_t PDL_BMC_init(uint32_t index, pdl_bmc_handle_t *handle);

/**
 * @brief 反初始化BMC服务
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_deinit(pdl_bmc_handle_t handle);

/**
 * @brief 电源开机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_power_on(pdl_bmc_handle_t handle);

/**
 * @brief 电源关机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_power_off(pdl_bmc_handle_t handle);

/**
 * @brief 电源复位
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_power_reset(pdl_bmc_handle_t handle);

/**
 * @brief 获取BMC状态
 *
 * @param[in] handle 服务句柄
 * @param[out] status 状态信息
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_get_status(pdl_bmc_handle_t handle, pdl_bmc_status_t *status);

/**
 * @brief 读取传感器
 *
 * @param[in] handle 服务句柄
 * @param[in] sensor_id 传感器ID
 * @param[out] reading 传感器读数
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_read_sensor(pdl_bmc_handle_t handle,
			uint32_t sensor_id,
			pdl_bmc_sensor_reading_t *reading);

/**
 * @brief BMC 测试调用链（调试接口）
 *
 * @param[in] index BMC设备索引
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 预留的调试接口，验证完整调用链
 */
int32_t PDL_BMC_test_call(uint32_t index);

#endif /* PDL_BMC_H */
