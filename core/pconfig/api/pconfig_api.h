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
 ************************************************************************/

#ifndef PCONFIG_API_H
#define PCONFIG_API_H

#include "pconfig_types.h"

/*===========================================================================
 * 配置库初始化
 *===========================================================================*/

/**
 * @brief 初始化硬件配置库
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PCONFIG_Init(void);

/**
 * @brief 清理硬件配置库
 */
void PCONFIG_Cleanup(void);

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
int32_t PCONFIG_Register(const pconfig_platform_config_t *config);

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
int32_t PCONFIG_List(const pconfig_platform_config_t **configs, uint32_t *count);

/*===========================================================================
 * 硬件外设配置查询接口（PCONFIG_HW_*）
 *===========================================================================*/

/**
 * @brief 根据名称查找MCU外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] name MCU名称
 *
 * @return MCU配置条目指针，失败返回NULL
 */
const pconfig_mcu_entry_t* PCONFIG_HW_FindMCU(const pconfig_platform_config_t *platform,
				      const char *name);

/**
 * @brief 根据编号获取MCU外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] id MCU编号（第几个）
 *
 * @return MCU配置条目指针，失败返回NULL
 */
const pconfig_mcu_entry_t* PCONFIG_HW_GetMCU(const pconfig_platform_config_t *platform,
				     uint32_t id);

/**
 * @brief 根据名称查找BMC外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] name BMC名称
 *
 * @return BMC配置条目指针，失败返回NULL
 */
const pconfig_bmc_entry_t* PCONFIG_HW_FindBMC(const pconfig_platform_config_t *platform,
				      const char *name);

/**
 * @brief 根据编号获取BMC外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] id BMC编号（第几个）
 *
 * @return BMC配置条目指针，失败返回NULL
 */
const pconfig_bmc_entry_t* PCONFIG_HW_GetBMC(const pconfig_platform_config_t *platform,
				     uint32_t id);

/**
 * @brief 根据名称查找FPGA外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] name FPGA名称
 *
 * @return FPGA配置指针，失败返回NULL
 */
const pconfig_fpga_cfg_t* PCONFIG_HW_FindFPGA(const pconfig_platform_config_t *platform,
					      const char *name);

/**
 * @brief 根据编号获取FPGA外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] id FPGA编号（第几个）
 *
 * @return FPGA配置指针，失败返回NULL
 */
const pconfig_fpga_cfg_t* PCONFIG_HW_GetFPGA(const pconfig_platform_config_t *platform,
					     uint32_t id);

/**
 * @brief 根据名称查找Switch外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] name Switch名称
 *
 * @return Switch配置指针，失败返回NULL
 */
const pconfig_switch_cfg_t* PCONFIG_HW_FindSwitch(const pconfig_platform_config_t *platform,
						  const char *name);

/**
 * @brief 根据编号获取Switch外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] id Switch编号（第几个）
 *
 * @return Switch配置指针，失败返回NULL
 */
const pconfig_switch_cfg_t* PCONFIG_HW_GetSwitch(const pconfig_platform_config_t *platform,
						 uint32_t id);

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
int32_t PCONFIG_Validate(const pconfig_platform_config_t *config);

/**
 * @brief 打印平台配置信息（用于调试）
 *
 * @param[in] config 平台配置
 */
void PCONFIG_Print(const pconfig_platform_config_t *config);

#endif /* PCONFIG_API_H */
