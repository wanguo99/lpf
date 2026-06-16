/************************************************************************
 * PCONFIG BMC 配置类型定义
 *
 * 功能：
 * - BMC外设配置条目
 *
 * 说明：
 * - PCONFIG 层只负责配置管理，实际配置结构由 PDL 层定义
 ************************************************************************/

#ifndef PCONFIG_BMC_H
#define PCONFIG_BMC_H

#include "pconfig_common.h"
#include "pdl_bmc.h"

/* 注意：此头文件必须包含 pdl_bmc.h，因为结构体中嵌入了 pdl_bmc_config_t
 * 源文件仍需按依赖顺序包含：osal.h → pdl.h → pconfig.h
 */

/*===========================================================================
 * BMC外设配置条目
 *===========================================================================*/

/**
 * @brief BMC外设配置条目
 *
 * PCONFIG 层只负责配置管理，实际配置结构由 PDL 层定义
 */
typedef struct {
	/* PCONFIG 配置管理字段 */
	const char *description;	/* 描述信息（用于日志和调试） */
	bool	enabled;		/* 是否启用此BMC */

	/* PDL 配置（来自 PDL 层） */
	pdl_bmc_config_t config;	/* BMC配置（来自 pdl_bmc.h） */

	/* GPIO控制（可选，PCONFIG层扩展） */
	pconfig_gpio_config_t *power_gpio;	/* 电源控制GPIO */
	pconfig_gpio_config_t *reset_gpio;	/* 复位GPIO */
} pconfig_bmc_entry_t;

#endif /* PCONFIG_BMC_H */
