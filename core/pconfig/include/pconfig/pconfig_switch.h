/************************************************************************
 * PCONFIG Switch 配置类型定义
 *
 * 功能：
 * - Switch外设配置
 *
 * 说明：
 * - Switch 配置目前由 PCONFIG 层直接定义
 ************************************************************************/

#ifndef PCONFIG_SWITCH_H
#define PCONFIG_SWITCH_H

#include "pconfig_common.h"

/*===========================================================================
 * Switch外设配置
 *===========================================================================*/

/**
 * @brief Switch外设配置
 */
typedef struct {
	/* 外设基本信息 */
	const char *name;		/* 外设名称 */
	const char *description;	/* 描述信息 */
	bool enabled;			/* 是否启用 */

	/* Switch特定配置 */
	const char *device;		/* 设备路径 */
	uint32_t cmd_timeout_ms;	/* 命令超时时间 */
	uint32_t retry_count;		/* 重试次数 */
} pconfig_switch_cfg_t;

#endif /* PCONFIG_SWITCH_H */
