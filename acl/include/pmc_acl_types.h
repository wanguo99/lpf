/**
 * @file pmc_acl_types.h
 * @brief PMC业务功能枚举定义
 * @note ACL层：业务配置层，定义业务功能到硬件设备的映射
 */

#ifndef PMC_ACL_TYPES_H
#define PMC_ACL_TYPES_H

#include "osal_types.h"

/**
 * @brief 遥测数据新鲜度标记
 * @note 所有遥测都从缓存读取，通过新鲜度标记体现数据质量
 */
typedef enum {
    TM_STATUS_INVALID = 0,   /* 无效：从未更新或已失效 */
    TM_STATUS_FRESH = 1,     /* 新鲜：在有效期内 */
    TM_STATUS_STALE = 2      /* 过期：超过有效期但仍可用 */
} tm_freshness_t;

/**
 * @brief PMC遥控功能枚举
 */
typedef enum {
    /* 服务器电源控制 */
    TC_SERVER_POWER_ON = 0,
    TC_SERVER_POWER_OFF,
    TC_SERVER_POWER_RESET,
    TC_SERVER_POWER_CYCLE,

    /* 服务器复位控制 */
    TC_SERVER_SOFT_RESET,
    TC_SERVER_HARD_RESET,

    /* MCU控制 */
    TC_MCU_RESET,
    TC_MCU_POWER_CTRL,

    /* FPGA控制 */
    TC_FPGA_RESET,
    TC_FPGA_CONFIG_LOAD,

    /* 固件升级 */
    TC_FIRMWARE_UPGRADE_START,
    TC_FIRMWARE_UPGRADE_DATA,
    TC_FIRMWARE_UPGRADE_VERIFY,
    TC_FIRMWARE_UPGRADE_COMMIT,

    /* 系统控制 */
    TC_SYSTEM_RESET,
    TC_WATCHDOG_ENABLE,
    TC_WATCHDOG_DISABLE,

    TC_FUNC_MAX
} pmc_tc_function_t;

/**
 * @brief PMC遥测功能枚举
 * @note 所有遥测都从缓存读取，通过时间戳和新鲜度标记体现数据质量
 */
typedef enum {
    /* 服务器状态遥测 */
    TM_SERVER_CPU_TEMP = 0,
    TM_SERVER_BOARD_TEMP,
    TM_SERVER_FAN_SPEED,
    TM_SERVER_VOLTAGE_12V,
    TM_SERVER_VOLTAGE_5V,
    TM_SERVER_VOLTAGE_3V3,
    TM_SERVER_CURRENT,
    TM_SERVER_POWER_STATUS,

    /* MCU状态遥测 */
    TM_MCU_STATUS,
    TM_MCU_TEMP,
    TM_MCU_VOLTAGE,
    TM_MCU_UPTIME,

    /* FPGA状态遥测 */
    TM_FPGA_STATUS,
    TM_FPGA_TEMP,
    TM_FPGA_CONFIG_STATUS,

    /* 系统健康遥测 */
    TM_SYSTEM_UPTIME,
    TM_WATCHDOG_STATUS,
    TM_ERROR_COUNT,

    TM_FUNC_MAX
} pmc_tm_function_t;

/**
 * @brief PMC健康管理功能枚举
 */
typedef enum {
    /* 健康检查项 */
    HM_SERVER_ALIVE = 0,
    HM_BMC_REACHABLE,
    HM_MCU_RESPONSIVE,
    HM_CAN_BUS_STATUS,
    HM_ETHERNET_STATUS,

    /* 故障检测 */
    HM_POWER_FAULT,
    HM_TEMP_OVER_LIMIT,
    HM_VOLTAGE_OUT_RANGE,

    HM_FUNC_MAX
} pmc_hm_function_t;

#endif /* PMC_ACL_TYPES_H */
