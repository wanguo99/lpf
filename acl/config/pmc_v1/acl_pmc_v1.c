/**
 * @file acl_pmc_v1.c
 * @brief PMC v1.0应用配置
 * @note 命名规范：acl_{project}_{product}_{version}.c
 *       ACL配置关注业务功能，不关注硬件平台
 */

#include "acl_config.h"
#include "pmc_acl_types.h"

/**
 * @brief PMC v1.0遥控配置表
 */
const acl_tc_config_t g_pmc_tc_configs[] = {
    /* 服务器电源控制 → BMC */
    { TC_SERVER_POWER_ON,      ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_OFF,     ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_RESET,   ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_CYCLE,   ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_SOFT_RESET,    ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_HARD_RESET,    ACL_DEVICE_BMC, 0, true },

    /* MCU控制 → MCU[0] */
    { TC_MCU_RESET,            ACL_DEVICE_MCU, 0, true },
    { TC_MCU_POWER_CTRL,       ACL_DEVICE_MCU, 1, true },  /* MCU[1]是电源管理 */

    /* FPGA控制 → FPGA[0] */
    { TC_FPGA_RESET,           ACL_DEVICE_FPGA, 0, true },
    { TC_FPGA_CONFIG_LOAD,     ACL_DEVICE_FPGA, 0, true },

    /* 固件升级 → MCU[0] */
    { TC_FIRMWARE_UPGRADE_START,  ACL_DEVICE_MCU, 0, true },
    { TC_FIRMWARE_UPGRADE_DATA,   ACL_DEVICE_MCU, 0, true },
    { TC_FIRMWARE_UPGRADE_VERIFY, ACL_DEVICE_MCU, 0, true },
    { TC_FIRMWARE_UPGRADE_COMMIT, ACL_DEVICE_MCU, 0, true },

    /* 系统控制 */
    { TC_SYSTEM_RESET,         ACL_DEVICE_MCU, 0, true },
    { TC_WATCHDOG_ENABLE,      ACL_DEVICE_MCU, 2, true },  /* MCU[2]是看门狗 */
    { TC_WATCHDOG_DISABLE,     ACL_DEVICE_MCU, 2, true },
};

const uint32_t g_pmc_tc_config_count = sizeof(g_pmc_tc_configs) / sizeof(acl_tc_config_t);

/**
 * @brief PMC v1.0遥测配置表
 * @note 所有遥测都从缓存读取，后台进程负责更新
 *       validity_ms：有效期，超过此时间标记为STALE
 *       update_period_ms：后台采集周期
 */
const acl_tm_config_t g_pmc_tm_configs[] = {
    /* 服务器遥测 → BMC，1秒更新周期，2秒有效期 */
    { TM_SERVER_CPU_TEMP,      ACL_DEVICE_BMC, 0, 2000, 1000, true },
    { TM_SERVER_BOARD_TEMP,    ACL_DEVICE_BMC, 0, 2000, 1000, true },
    { TM_SERVER_FAN_SPEED,     ACL_DEVICE_BMC, 0, 4000, 2000, true },
    { TM_SERVER_VOLTAGE_12V,   ACL_DEVICE_MCU, 1, 4000, 2000, true },
    { TM_SERVER_VOLTAGE_5V,    ACL_DEVICE_MCU, 1, 4000, 2000, true },
    { TM_SERVER_VOLTAGE_3V3,   ACL_DEVICE_MCU, 1, 4000, 2000, true },
    { TM_SERVER_CURRENT,       ACL_DEVICE_MCU, 1, 4000, 2000, true },
    { TM_SERVER_POWER_STATUS,  ACL_DEVICE_BMC, 0, 500,  100,  true },

    /* MCU遥测 → MCU，2秒更新周期，4秒有效期 */
    { TM_MCU_STATUS,           ACL_DEVICE_MCU, 0, 500,  100,  true },
    { TM_MCU_TEMP,             ACL_DEVICE_MCU, 0, 4000, 2000, true },
    { TM_MCU_VOLTAGE,          ACL_DEVICE_MCU, 0, 4000, 2000, true },
    { TM_MCU_UPTIME,           ACL_DEVICE_MCU, 0, 4000, 2000, true },

    /* FPGA遥测 → FPGA，2秒更新周期，4秒有效期 */
    { TM_FPGA_STATUS,          ACL_DEVICE_FPGA, 0, 500,  100,  true },
    { TM_FPGA_TEMP,            ACL_DEVICE_FPGA, 0, 4000, 2000, true },
    { TM_FPGA_CONFIG_STATUS,   ACL_DEVICE_FPGA, 0, 500,  100,  true },

    /* 系统遥测 */
    { TM_SYSTEM_UPTIME,        ACL_DEVICE_MCU, 0, 4000, 2000, true },
    { TM_WATCHDOG_STATUS,      ACL_DEVICE_MCU, 2, 500,  100,  true },
    { TM_ERROR_COUNT,          ACL_DEVICE_MCU, 0, 4000, 2000, true },
};

const uint32_t g_pmc_tm_config_count = sizeof(g_pmc_tm_configs) / sizeof(acl_tm_config_t);
