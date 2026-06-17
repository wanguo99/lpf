/************************************************************************
 * PCONFIG 对外 API
 *
 * 功能：
 * - 硬件配置注册和查询（以外设为单位）
 * - 配置验证和加载
 * - 运行时配置切换
 *
 * 命名规范：
 * - PCONFIG_*      - 通用接口
 * - PCONFIG_HW_*   - 硬件配置相关接口
 *
 * 设计原则：
 * - 参考 PDL 模式，直接包含各模块类型头文件
 * - 避免中间层，保持结构简洁
 ************************************************************************/

#ifndef PCONFIG_H
#define PCONFIG_H

/* 类型定义 - 按模块组织 */
#include "pconfig_common.h"    /* 通用基础类型 */
#include "pconfig_mcu.h"       /* MCU 配置类型 */
#include "pconfig_bmc.h"       /* BMC 配置类型 */
#include "pconfig_fpga.h"      /* FPGA 配置类型 */
#include "pconfig_switch.h"    /* Switch 配置类型 */
#include "pconfig_satellite.h" /* Satellite 配置类型 */
#include "pconfig_ccm.h"       /* CCM 配置类型 */
#include "pconfig_watchdog.h"  /* Watchdog 配置类型 */
#include "pconfig_platform.h"  /* 板级配置类型 */

/*===========================================================================
 * 配置库初始化
 *===========================================================================*/

/**
 * @brief 初始化硬件配置库
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PCONFIG_init(void);

/**
 * @brief 清理硬件配置库
 */
void PCONFIG_cleanup(void);

/*===========================================================================
 * 板级配置注册和查询
 *===========================================================================*/

/**
 * @brief 注册平台配置
 *
 * @param[in] config 平台配置指针
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PCONFIG_register(const pconfig_platform_config_t *config);

/**
 * @brief 获取当前平台配置
 *
 * @return 平台配置指针，失败返回NULL
 */
const pconfig_platform_config_t* PCONFIG_GetBoard(void);

/**
 * @brief 根据平台和产品名称查找配置
 *
 * @param[in] platform 平台名称（如"ti/am625"）
 * @param[in] product 产品名称（如"h200_payload"）
 * @param[in] version 版本号（如"v1.0"，可选）
 *
 * @return 平台配置指针，失败返回NULL
 */
const pconfig_platform_config_t* PCONFIG_Find(const char *platform,
					    const char *product,
					    const char *version);

/**
 * @brief 列出所有已注册的配置
 *
 * @param[out] configs 配置列表
 * @param[in,out] count 输入：缓冲区大小，输出：实际数量
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PCONFIG_list(const pconfig_platform_config_t **configs, uint32_t *count);

/*===========================================================================
 * 硬件外设配置查询接口（PCONFIG_HW_*）
 *===========================================================================*/

/**
 * @brief 根据索引获取MCU外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index MCU索引（数组下标）
 *
 * @return MCU配置条目指针，失败返回NULL
 */
static inline const pconfig_mcu_entry_t*
PCONFIG_HW_GetMCU(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->mcu_array || index >= platform->mcu_count) {
		return NULL;
	}
	return &platform->mcu_array[index];
}

/**
 * @brief 根据索引获取BMC外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index BMC索引（数组下标）
 *
 * @return BMC配置条目指针，失败返回NULL
 */
static inline const pconfig_bmc_entry_t*
PCONFIG_HW_GetBMC(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->bmc_array || index >= platform->bmc_count) {
		return NULL;
	}
	return &platform->bmc_array[index];
}

/**
 * @brief 根据索引获取FPGA外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index FPGA索引（数组下标）
 *
 * @return FPGA配置指针，失败返回NULL
 */
static inline const pconfig_fpga_cfg_t*
PCONFIG_HW_GetFPGA(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->fpga_array || index >= platform->fpga_count) {
		return NULL;
	}
	return &platform->fpga_array[index];
}

/**
 * @brief 根据索引获取Switch外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index Switch索引（数组下标）
 *
 * @return Switch配置指针，失败返回NULL
 */
static inline const pconfig_switch_cfg_t*
PCONFIG_HW_GetSwitch(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->switch_array || index >= platform->switch_count) {
		return NULL;
	}
	return &platform->switch_array[index];
}

/**
 * @brief 根据索引获取Satellite外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index Satellite索引（数组下标）
 *
 * @return Satellite配置条目指针，失败返回NULL
 */
static inline const pconfig_satellite_entry_t*
PCONFIG_HW_GetSatellite(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->satellite_array || index >= platform->satellite_count) {
		return NULL;
	}
	return &platform->satellite_array[index];
}

/**
 * @brief 根据索引获取CCM外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index CCM索引（数组下标）
 *
 * @return CCM配置条目指针，失败返回NULL
 */
static inline const pconfig_ccm_entry_t*
PCONFIG_HW_GetCCM(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->ccm_array || index >= platform->ccm_count) {
		return NULL;
	}
	return &platform->ccm_array[index];
}

/**
 * @brief 根据索引获取Watchdog外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index Watchdog索引（数组下标）
 *
 * @return Watchdog配置条目指针，失败返回NULL
 */
static inline const pconfig_watchdog_entry_t*
PCONFIG_HW_GetWatchdog(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->watchdog_array || index >= platform->watchdog_count) {
		return NULL;
	}
	return &platform->watchdog_array[index];
}

/*===========================================================================
 * 配置验证
 *===========================================================================*/

/**
 * @brief 验证平台配置的完整性和正确性
 *
 * @param[in] config 平台配置
 *
 * @return OSAL_SUCCESS 验证通过
 * @return OSAL_ERR_GENERIC 验证失败
 */
int32_t PCONFIG_validate(const pconfig_platform_config_t *config);

/**
 * @brief 打印平台配置信息（用于调试）
 *
 * @param[in] config 平台配置
 */
void PCONFIG_print(const pconfig_platform_config_t *config);

#endif /* PCONFIG_H */
