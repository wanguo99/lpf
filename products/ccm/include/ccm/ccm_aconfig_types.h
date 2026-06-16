/**
 * @file ccm_aconfig_types.h
 * @brief CCM 产品特定的配置类型定义
 * @note 实现 aconfig_function_map_t 的具体结构
 */

#ifndef CCM_ACONFIG_TYPES_H
#define CCM_ACONFIG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 设备类型枚举（CCM 特定）
 */
typedef enum {
	CCM_DEVICE_MCU = 0,      /* 微控制器 */
	CCM_DEVICE_BMC,          /* 板级管理控制器 */
	CCM_DEVICE_FPGA,         /* 可编程逻辑器件 */
	CCM_DEVICE_SWITCH,       /* 网络交换机 */
	CCM_DEVICE_SENSOR,       /* 传感器 */
	CCM_DEVICE_HOST,         /* 主机接口 */
	CCM_DEVICE_SATELLITE,    /* 卫星设备 */
	CCM_DEVICE_MAX
} ccm_device_type_t;

/**
 * @brief 设备引用结构（CCM 特定）
 */
typedef struct {
	ccm_device_type_t type;  /* 设备类型 */
	uint32_t index;          /* 设备索引 */
} ccm_device_ref_t;

/**
 * @brief 遥测数据新鲜度标记
 */
typedef enum {
	CCM_TM_STATUS_INVALID = 0x00,  /* 无效 */
	CCM_TM_STATUS_FRESH = 0x01,    /* 新鲜 */
	CCM_TM_STATUS_STALE = 0x02     /* 过期 */
} ccm_tm_status_t;

/**
 * @brief 遥控配置结构（CCM 特定）
 */
typedef struct {
	uint32_t function_id;                  /* 遥控功能 ID */
	ccm_device_ref_t device;               /* 目标设备引用 */

	/* 失效映射 */
	const uint32_t *invalidated_tm_ids;    /* 受影响的遥测 ID 列表 */
	uint32_t invalidated_tm_count;         /* 受影响的遥测数量 */

	bool enabled;                          /* 是否启用 */
	void *user_context;                    /* 用户上下文 */
} ccm_tc_config_t;

/**
 * @brief 遥测配置结构（CCM 特定）
 */
typedef struct {
	uint32_t function_id;                  /* 遥测功能 ID */
	ccm_device_ref_t device;               /* 数据源设备引用 */

	/* 遥测特有参数 */
	uint32_t poll_period_ms;               /* 采集周期（毫秒） */
	uint32_t validity_period_ms;           /* 有效期（毫秒） */

	bool enabled;                          /* 是否启用 */
	void *user_context;                    /* 用户上下文 */
} ccm_tm_config_t;

/**
 * @brief 遥控配置条目（稀疏数组元素）
 */
typedef struct {
	uint32_t function_id;          /* 功能 ID */
	ccm_tc_config_t config;        /* 配置数据 */
} ccm_tc_entry_t;

/**
 * @brief 遥测配置条目（稀疏数组元素）
 */
typedef struct {
	uint32_t function_id;          /* 功能 ID */
	ccm_tm_config_t config;        /* 配置数据 */
} ccm_tm_entry_t;

/**
 * @brief CCM 功能映射结构（实现 aconfig_function_map_t）
 * @note 这是 CCM 产品对 aconfig_function_map_t 的具体实现
 */
typedef struct ccm_aconfig_function_map {
	/* 遥控配置（稀疏数组） */
	const ccm_tc_entry_t *tc_entries;
	uint32_t tc_count;

	/* 遥测配置（稀疏数组） */
	const ccm_tm_entry_t *tm_entries;
	uint32_t tm_count;
} ccm_aconfig_function_map_t;

#endif /* CCM_ACONFIG_TYPES_H */
