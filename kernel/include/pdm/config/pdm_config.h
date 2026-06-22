/************************************************************************
 * LPF_CONFIG 对外 API
 *
 * 功能：
 * - 硬件配置后端选择和只读查询（以外设为单位）
 * - 配置验证和调试打印
 *
 * 命名规范：
 * - LPF_CONFIG_*      - 通用接口
 * - LPF_CONFIG_HW_*   - 硬件配置相关接口
 *
 * 设计原则：
 * - LPF_CONFIG 负责屏蔽静态表、Device Tree、产品配置等来源差异
 * - LPF peripheral configuration consumes the unified device list and typed
 *   configuration entries
 ************************************************************************/

#ifndef LPF_CONFIG_H
#define LPF_CONFIG_H

#define LPF_CONFIG_VERSION_MAJOR 0x1
#define LPF_CONFIG_VERSION_MINOR 0x0
#define LPF_CONFIG_VERSION_PATCH 0x0

/* 类型定义 - 按模块组织 */
#include "lpf/config/lpf_config_common.h" /* 通用基础类型 */
#include "lpf/config/lpf_config_mcu.h" /* MCU 配置类型 */
#include "lpf/config/lpf_config_led.h" /* LED 配置类型 */
#include "lpf/config/lpf_config_platform.h" /* 板级配置类型 */

/*===========================================================================
 * 板级配置查询
 *===========================================================================*/

/**
 * @brief 获取当前后端选中的平台配置
 *
 * @return 平台配置指针，失败返回NULL
 */
const lpf_config_platform_config_t *lpf_config_get_board(void);
int32_t lpf_config_load(void);
void lpf_config_unload(void);

const lpf_config_device_node_t *lpf_config_get_device_nodes(uint32_t *count);

/**
 * @brief 根据产品、项目和版本查找配置
 *
 * @param[in] product 产品名称（如"kernel"）
 * @param[in] project 项目名称（如"x86_modules"）
 * @param[in] version 版本号（如"1.0.0"）
 *
 * @return 平台配置指针，失败返回NULL
 */
const lpf_config_platform_config_t *
lpf_config_find(const char *product, const char *project, const char *version);

/**
 * @brief 列出所有配置
 *
 * @param[out] configs 配置列表
 * @param[in,out] count 输入：缓冲区大小，输出：实际数量
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t lpf_config_list(const lpf_config_platform_config_t **configs,
					 uint32_t *count);

/*===========================================================================
 * 硬件外设配置查询接口（LPF_CONFIG_HW_*）
 *===========================================================================*/

/**
 * @brief 根据索引获取MCU外设配置
 *
 * @param[in] platform 平台配置
 * @param[in] index MCU索引（数组下标）
 *
 * @return MCU配置条目指针，失败返回NULL
 */
static inline const lpf_config_mcu_entry_t *
lpf_config_hw_get_mcu(const lpf_config_platform_config_t *platform, uint32_t index)
{
	uint32_t i;

	if (!platform) {
		return NULL;
	}

	if (platform->mcu_array && index < platform->mcu_count)
		return &platform->mcu_array[index];

	for (i = 0; platform->device_nodes &&
	     i < platform->device_node_count; i++) {
		const lpf_config_device_node_t *node = &platform->device_nodes[i];

		if (node->device_type == LPF_CONFIG_DEVICE_TYPE_MCU &&
		    node->index == index &&
		    node->payload_size == sizeof(lpf_config_mcu_entry_t))
			return (const lpf_config_mcu_entry_t *)node->payload;
	}

	return NULL;
}

static inline const lpf_config_led_entry_t *
lpf_config_hw_get_led(const lpf_config_platform_config_t *platform, uint32_t index)
{
	uint32_t i;

	if (!platform) {
		return NULL;
	}

	if (platform->led_array && index < platform->led_count)
		return &platform->led_array[index];

	for (i = 0; platform->device_nodes &&
	     i < platform->device_node_count; i++) {
		const lpf_config_device_node_t *node = &platform->device_nodes[i];

		if (node->device_type == LPF_CONFIG_DEVICE_TYPE_LED &&
		    node->index == index &&
		    node->payload_size == sizeof(lpf_config_led_entry_t))
			return (const lpf_config_led_entry_t *)node->payload;
	}

	return NULL;
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
int32_t lpf_config_validate(const lpf_config_platform_config_t *config);

/**
 * @brief 打印平台配置信息（用于调试）
 *
 * @param[in] config 平台配置
 */
void lpf_config_print(const lpf_config_platform_config_t *config);
void lpf_config_print_version(void);

#endif /* LPF_CONFIG_H */
