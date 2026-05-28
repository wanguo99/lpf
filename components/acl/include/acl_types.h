/**
 * @file acl_types.h
 * @brief ACL通用类型定义
 * @note ACL框架核心：提供通用的配置管理机制，不依赖具体业务
 */

#ifndef ACL_TYPES_H
#define ACL_TYPES_H

#include "osal/osal_types.h"

/**
 * @brief ACL设备类型枚举
 * @note 通用设备类型，可扩展
 */
typedef enum {
    ACL_DEVICE_SATELLITE = 0,  /* 卫星设备（如BMC） */
    ACL_DEVICE_BMC,            /* 板级管理控制器 */
    ACL_DEVICE_MCU,            /* 微控制器 */
    ACL_DEVICE_FPGA,           /* 可编程逻辑器件 */
    ACL_DEVICE_SWITCH,         /* 网络交换机 */
    ACL_DEVICE_SENSOR,         /* 传感器 */
    ACL_DEVICE_ACTUATOR,       /* 执行器 */
    ACL_DEVICE_MAX
} acl_device_type_t;

/**
 * @brief 遥测数据新鲜度标记
 * @note 通用的数据质量标记
 */
typedef enum {
    ACL_TM_STATUS_INVALID = 0,   /* 无效：从未更新或已失效 */
    ACL_TM_STATUS_FRESH = 1,     /* 新鲜：在有效期内 */
    ACL_TM_STATUS_STALE = 2      /* 过期：超过有效期但仍可用 */
} acl_tm_status_t;

/**
 * @brief 遥控功能配置（通用）
 * @note 使用 uint32_t 作为功能ID，不依赖具体业务枚举
 */
typedef struct {
    uint32_t function_id;          /* 功能ID（由项目定义） */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引（第几个同类设备） */
    bool enabled;                  /* 是否使能 */
    void *extra_data;              /* 扩展数据（项目特定） */
} acl_tc_config_t;

/**
 * @brief 遥测功能配置（通用）
 * @note 使用 uint32_t 作为功能ID，不依赖具体业务枚举
 */
typedef struct {
    uint32_t function_id;          /* 功能ID（由项目定义） */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引（第几个同类设备） */
    uint32_t validity_ms;          /* 有效期（毫秒），超过此时间标记为STALE */
    uint32_t update_period_ms;     /* 后台更新周期（毫秒） */
    bool enabled;                  /* 是否使能 */
    void *extra_data;              /* 扩展数据（项目特定） */
} acl_tm_config_t;

/**
 * @brief 失效映射配置（通用）
 * @note 定义遥测失效时的级联关系
 */
typedef struct {
    uint32_t source_tm_id;         /* 源遥测ID */
    uint32_t *affected_tm_ids;     /* 受影响的遥测ID数组 */
    uint32_t affected_count;       /* 受影响的遥测数量 */
} acl_invalidation_map_t;

/**
 * @brief ACL配置表（通用）
 * @note 项目通过此结构注册配置
 */
typedef struct {
    const char *name;                      /* 配置表名称 */
    const acl_tc_config_t *tc_table;       /* 遥控配置表 */
    uint32_t tc_count;                     /* 遥控配置数量 */
    const acl_tm_config_t *tm_table;       /* 遥测配置表 */
    uint32_t tm_count;                     /* 遥测配置数量 */
    const acl_invalidation_map_t *inv_map; /* 失效映射表 */
    uint32_t inv_count;                    /* 失效映射数量 */
} acl_config_table_t;

#endif /* ACL_TYPES_H */
