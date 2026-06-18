/************************************************************************
 * PCONFIG 对外 API
 *
 * 功能：
 * - 硬件配置只读查询（以外设为单位）
 * - 配置验证和调试打印
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
#include "pconfig_common.h" /* 通用基础类型 */
#include "pconfig_mcu.h" /* MCU 配置类型 */
#include "pconfig_platform.h" /* 板级配置类型 */

/*===========================================================================
 * 产品配置入口
 *===========================================================================*/

/**
 * @brief 产品层提供的只读平台配置表
 *
 * 产品或测试目标通过 PCONFIG_EXTRA_SRCS 编译进该符号。PCONFIG 只读取该表，
 * 不在运行期注册、切换或释放配置。
 */
extern const pconfig_platform_table_t g_pconfig_platform_table;

/*===========================================================================
 * 板级配置查询
 *===========================================================================*/

/**
 * @brief 获取当前平台配置
 *
 * @return 平台配置指针，失败返回NULL
 */
const pconfig_platform_config_t *pconfig_get_board(void);

/**
 * @brief 根据平台和产品名称查找配置
 *
 * @param[in] platform 平台名称（如"ti/am625"）
 * @param[in] product 产品名称（如"framework"）
 * @param[in] version 版本号（如"v1.0"，当前未使用）
 *
 * @return 平台配置指针，失败返回NULL
 */
const pconfig_platform_config_t *
pconfig_find(const char *platform, const char *product, const char *version);

/**
 * @brief 列出所有配置
 *
 * @param[out] configs 配置列表
 * @param[in,out] count 输入：缓冲区大小，输出：实际数量
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t pconfig_list(const pconfig_platform_config_t **configs,
					 uint32_t *count);

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
static inline const pconfig_mcu_entry_t *
pconfig_hw_get_mcu(const pconfig_platform_config_t *platform, uint32_t index)
{
	if (!platform || !platform->mcu_array || index >= platform->mcu_count) {
		return NULL;
	}
	return &platform->mcu_array[index];
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
int32_t pconfig_validate(const pconfig_platform_config_t *config);

/**
 * @brief 打印平台配置信息（用于调试）
 *
 * @param[in] config 平台配置
 */
void pconfig_print(const pconfig_platform_config_t *config);

#endif /* PCONFIG_H */
