/**
 * @file aconfig_types.h
 * @brief ACONFIG 核心类型定义 - 优化版
 * @note 本文件定义 ACONFIG 模块的核心数据类型
 *       优化要点：
 *       1. 使用设备索引引用替代字符串查找
 *       2. 采用稀疏数组存储配置
 *       3. 失效映射内嵌到 TC 配置中
 */

#ifndef ACONFIG_TYPES_H
#define ACONFIG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 设备类型枚举（从 PConfig 复用）
 */
typedef enum {
    PCONFIG_DEVICE_MCU = 0,      /* 微控制器 */
    PCONFIG_DEVICE_BMC,           /* 板级管理控制器 */
    PCONFIG_DEVICE_FPGA,          /* 可编程逻辑器件 */
    PCONFIG_DEVICE_SWITCH,        /* 网络交换机 */
    PCONFIG_DEVICE_SENSOR,        /* 传感器 */
    PCONFIG_DEVICE_HOST,          /* 主机接口 */
    PCONFIG_DEVICE_SATELLITE,     /* 卫星设备 */
    PCONFIG_DEVICE_MAX
} pconfig_device_type_t;

/**
 * @brief 设备引用结构（统一索引方式）
 * @note 通过类型+索引的方式引用 PConfig 中的设备，实现 O(1) 查找
 */
typedef struct {
    pconfig_device_type_t type;  /* 设备类型: MCU/BMC/FPGA/... */
    uint32_t index;              /* 在 PConfig 对应类型表中的索引 */
} aconfig_device_ref_t;

/**
 * @brief 遥测数据新鲜度标记
 */
typedef enum {
    ACONFIG_TM_STATUS_INVALID = 0x00,  /* 无效（从未更新） */
    ACONFIG_TM_STATUS_FRESH = 0x01,    /* 新鲜（在有效期内） */
    ACONFIG_TM_STATUS_STALE = 0x02     /* 过期（超过有效期但可用） */
} aconfig_tm_status_t;

/**
 * @brief 遥控配置结构
 * @note 优化：
 *       - 使用设备索引替代字符串查找
 *       - 失效映射直接内嵌
 */
typedef struct {
    uint32_t function_id;                  /* 遥控功能 ID */
    aconfig_device_ref_t device;           /* 目标设备引用（索引方式） */

    /* 失效映射（内嵌） */
    const uint32_t *invalidated_tm_ids;    /* 受影响的遥测 ID 列表 */
    uint32_t invalidated_tm_count;         /* 受影响的遥测数量 */

    bool enabled;                          /* 是否启用 */
    void *user_context;                    /* 用户上下文 */
} aconfig_tc_config_t;

/**
 * @brief 遥测配置结构
 * @note 优化：使用设备索引替代字符串查找
 */
typedef struct {
    uint32_t function_id;                  /* 遥测功能 ID */
    aconfig_device_ref_t device;           /* 数据源设备引用（索引方式） */

    /* 遥测特有参数 */
    uint32_t poll_period_ms;               /* 采集周期（毫秒） */
    uint32_t validity_period_ms;           /* 有效期（毫秒） */

    bool enabled;                          /* 是否启用 */
    void *user_context;                    /* 用户上下文 */
} aconfig_tm_config_t;

/**
 * @brief 遥控配置条目（稀疏数组元素）
 * @note 优化：只存储实际定义的功能，不浪费内存
 */
typedef struct {
    uint32_t function_id;                  /* 功能 ID（用于查找） */
    aconfig_tc_config_t config;            /* 配置数据 */
} aconfig_tc_entry_t;

/**
 * @brief 遥测配置条目（稀疏数组元素）
 * @note 优化：只存储实际定义的功能，不浪费内存
 */
typedef struct {
    uint32_t function_id;                  /* 功能 ID（用于查找） */
    aconfig_tm_config_t config;            /* 配置数据 */
} aconfig_tm_entry_t;

/**
 * @brief ACONFIG 配置表（优化版）
 * @note 优化：采用稀疏数组 + 哈希表方式
 *       - 数组：只存储实际定义的功能
 *       - 哈希表：运行时构建，加速查找
 */
typedef struct {
    const char *name;                      /* 配置表名称 */

    /* 遥控配置（稀疏数组） */
    const aconfig_tc_entry_t *tc_entries;  /* 遥控配置条目数组 */
    uint32_t tc_count;                     /* 实际配置数量（不是枚举最大值） */

    /* 遥测配置（稀疏数组） */
    const aconfig_tm_entry_t *tm_entries;  /* 遥测配置条目数组 */
    uint32_t tm_count;                     /* 实际配置数量（不是枚举最大值） */
} aconfig_config_table_t;

/**
 * @brief ACONFIG 统计信息
 */
typedef struct {
    uint32_t tc_enabled_count;             /* 启用的遥控功能数量 */
    uint32_t tc_disabled_count;            /* 禁用的遥控功能数量 */
    uint32_t tm_enabled_count;             /* 启用的遥测功能数量 */
    uint32_t tm_disabled_count;            /* 禁用的遥测功能数量 */
    uint32_t total_invalidation_maps;      /* 失效映射总数 */
} aconfig_statistics_t;

#endif /* ACONFIG_TYPES_H */
