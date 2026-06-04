/**
 * @file aconfig_types.h
 * @brief ACONFIG 对外类型定义 - 通用数据结构和枚举
 * @note 本文件定义 ACONFIG 模块的对外数据类型，不依赖具体业务
 */

#ifndef ACONFIG_ACONFIG_TYPES_H
#define ACONFIG_ACONFIG_TYPES_H

#include "osal/osal.h"

/**
 * @brief ACONFIG 设备类型枚举
 * @note 通用设备类型，可扩展
 */
typedef enum {
	ACONFIG_DEVICE_SATELLITE = 0,	/* 卫星设备（如 BMC） */
	ACONFIG_DEVICE_BMC,		/* 板级管理控制器 */
	ACONFIG_DEVICE_MCU,		/* 微控制器 */
	ACONFIG_DEVICE_FPGA,		/* 可编程逻辑器件 */
	ACONFIG_DEVICE_SWITCH,		/* 网络交换机 */
	ACONFIG_DEVICE_SENSOR,		/* 传感器 */
	ACONFIG_DEVICE_ACTUATOR,	/* 执行器 */
	ACONFIG_DEVICE_MAX
} aconfig_device_type_t;

/**
 * @brief 遥测数据新鲜度标记
 * @note 通用的数据质量标记
 */
typedef enum {
	ACONFIG_TM_STATUS_INVALID = 0,	/* 无效：从未更新或已失效 */
	ACONFIG_TM_STATUS_FRESH = 1,	/* 新鲜：在有效期内 */
	ACONFIG_TM_STATUS_STALE = 2	/* 过期：超过有效期但仍可用 */
} aconfig_tm_status_t;

/**
 * @brief 遥控功能配置（通用）
 * @note 使用 uint32_t 作为功能 ID，不依赖具体业务枚举
 */
typedef struct {
	uint32_t function_id;		/* 功能 ID（由项目定义） */
	aconfig_device_type_t device_type; /* 设备类型 */
	const char *device_name;	/* 设备名称（如 "power_mcu", "payload_bmc"） */
	bool enabled;			/* 是否使能 */
	void *user_context;		/* 用户上下文（项目特定） */
} aconfig_tc_config_t;

/**
 * @brief 遥测功能配置（通用）
 * @note 使用 uint32_t 作为功能 ID，不依赖具体业务枚举
 */
typedef struct {
	uint32_t function_id;		/* 功能 ID（由项目定义） */
	aconfig_device_type_t device_type; /* 设备类型 */
	const char *device_name;	/* 设备名称（如 "power_mcu", "payload_bmc"） */
	uint32_t data_validity_ms;	/* 数据有效期（毫秒），超过此时间标记为 STALE */
	uint32_t background_update_period_ms; /* 后台更新周期（毫秒） */
	bool enabled;			/* 是否使能 */
	void *user_context;		/* 用户上下文（项目特定） */
} aconfig_tm_config_t;

/**
 * @brief 失效映射配置（通用）
 * @note 定义遥测失效时的级联关系
 */
typedef struct {
	uint32_t source_tm_id;		/* 源遥测 ID */
	uint32_t *affected_tm_ids;	/* 受影响的遥测 ID 数组 */
	uint32_t affected_count;	/* 受影响的遥测数量 */
} aconfig_invalidation_map_t;

/**
 * @brief ACONFIG 配置表（通用）
 * @note 项目通过此结构注册配置
 */
typedef struct {
	const char *name;			/* 配置表名称 */
	const aconfig_tc_config_t *tc_table;	/* 遥控配置表 */
	uint32_t tc_count;			/* 遥控配置数量 */
	const aconfig_tm_config_t *tm_table;	/* 遥测配置表 */
	uint32_t tm_count;			/* 遥测配置数量 */
	const aconfig_invalidation_map_t *inv_map; /* 失效映射表 */
	uint32_t inv_count;			/* 失效映射数量 */
} aconfig_config_table_t;

#endif /* ACONFIG_ACONFIG_TYPES_H */
