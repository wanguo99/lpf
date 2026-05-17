/**
 * @file acl_config.h
 * @brief ACL配置结构定义
 * @note ACL层：业务配置层，定义业务功能到硬件设备的映射
 */

#ifndef ACL_CONFIG_H
#define ACL_CONFIG_H

#include "osal_types.h"
#include "pmc_acl_types.h"

/**
 * @brief ACL设备类型枚举
 */
typedef enum {
    ACL_DEVICE_SATELLITE = 0,
    ACL_DEVICE_BMC,
    ACL_DEVICE_MCU,
    ACL_DEVICE_FPGA,
    ACL_DEVICE_MAX
} acl_device_type_t;

/**
 * @brief 遥控功能配置
 */
typedef struct {
    pmc_tc_function_t function;    /* 遥控功能枚举 */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引（第几个同类设备） */
    bool enabled;                  /* 是否使能 */
} acl_tc_config_t;

/**
 * @brief 遥测功能配置
 * @note 所有遥测都从缓存读取，通过时间戳和新鲜度标记体现数据质量
 */
typedef struct {
    pmc_tm_function_t function;    /* 遥测功能枚举 */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引（第几个同类设备） */
    uint32_t validity_ms;          /* 有效期（毫秒），超过此时间标记为STALE */
    uint32_t update_period_ms;     /* 后台更新周期（毫秒） */
    bool enabled;                  /* 是否使能 */
} acl_tm_config_t;

/**
 * @brief ACL查找表（O(1)直接索引）
 */
typedef struct {
    acl_tc_config_t tc_table[TC_FUNC_MAX];
    acl_tm_config_t tm_table[TM_FUNC_MAX];
} acl_lookup_table_t;

/**
 * @brief 遥控命令失效映射
 */
typedef struct {
    pmc_tc_function_t tc_function;      /* 遥控功能 */
    pmc_tm_function_t affected_tm[8];   /* 受影响的遥测项（最多8个） */
    uint32_t affected_count;            /* 受影响的遥测项数量 */
} tc_tm_invalidation_map_t;

#endif /* ACL_CONFIG_H */
