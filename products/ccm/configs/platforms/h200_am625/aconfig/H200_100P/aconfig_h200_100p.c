/**
 * @file aconfig_h200_100p.c
 * @brief H200_100P应用配置
 * @note 命名规范：aconfig_{project}_{product}_{version}.c
 *       AConfig配置关注业务功能，不关注硬件平台
 */

#include "aconfig_config.h"
#include "aconfig_tc.h"
#include "aconfig_tm.h"

/**
 * @brief PMC v1.0遥控配置表
 */
const aconfig_tc_config_t g_pmc_tc_configs[] = {
    /* 服务器电源控制 → BMC */
    { TC_POWER_ON,             ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },
    { TC_POWER_OFF,            ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },
    { TC_POWER_RESET,          ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },
    { TC_POWER_CYCLE,          ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },
    { TC_SOFT_RESET,           ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },
    { TC_HARD_RESET,           ACONFIG_DEVICE_BMC, "payload_bmc", 0, true, NULL },

    /* MCU控制 → MCU */
    { TC_MCU_RESET,            ACONFIG_DEVICE_MCU, "power_mcu", 0, true, NULL },
    { TC_MCU_POWER_CTRL,       ACONFIG_DEVICE_MCU, "power_mcu", 0, true, NULL },

    /* FPGA控制 → FPGA */
    { TC_FPGA_RESET,           ACONFIG_DEVICE_FPGA, "main_fpga", 0, true, NULL },
    { TC_FPGA_CONFIG_LOAD,     ACONFIG_DEVICE_FPGA, "main_fpga", 0, true, NULL },

    /* 固件升级 → MCU[0] */
    { TC_FIRMWARE_UPGRADE_START,  ACONFIG_DEVICE_MCU, 0, true, NULL },
    { TC_FIRMWARE_UPGRADE_DATA,   ACONFIG_DEVICE_MCU, 0, true, NULL },
    { TC_FIRMWARE_UPGRADE_VERIFY, ACONFIG_DEVICE_MCU, 0, true, NULL },
    { TC_FIRMWARE_UPGRADE_COMMIT, ACONFIG_DEVICE_MCU, 0, true, NULL },

    /* 系统控制 */
    { TC_SYSTEM_RESET,         ACONFIG_DEVICE_MCU, 0, true, NULL },
    { TC_WATCHDOG_ENABLE,      ACONFIG_DEVICE_MCU, 2, true, NULL },  /* MCU[2]是看门狗 */
    { TC_WATCHDOG_DISABLE,     ACONFIG_DEVICE_MCU, 2, true, NULL },
};

const uint32_t g_pmc_tc_config_count = sizeof(g_pmc_tc_configs) / sizeof(aconfig_tc_config_t);

/**
 * @brief PMC v1.0遥测配置表
 * @note 所有遥测都从缓存读取，后台进程负责更新
 *       validity_ms：有效期，超过此时间标记为STALE
 *       update_period_ms：后台采集周期
 */
const aconfig_tm_config_t g_pmc_tm_configs[] = {
    /* 服务器遥测 → BMC，1秒更新周期，2秒有效期 */
    { TM_CPU_TEMP,             ACONFIG_DEVICE_BMC, "payload_bmc", 0, 2000, 1000, true, NULL },
    { TM_BOARD_TEMP,           ACONFIG_DEVICE_BMC, "payload_bmc", 0, 2000, 1000, true, NULL },
    { TM_FAN_SPEED,            ACONFIG_DEVICE_BMC, "payload_bmc", 0, 4000, 2000, true, NULL },
    { TM_VOLTAGE_12V,          ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_VOLTAGE_5V,           ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_VOLTAGE_3V3,          ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_CURRENT,              ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_POWER_STATUS,         ACONFIG_DEVICE_BMC, "payload_bmc", 0, 500,  100,  true, NULL },

    /* MCU遥测 → MCU，2秒更新周期，4秒有效期 */
    { TM_MCU_STATUS,           ACONFIG_DEVICE_MCU, "power_mcu", 0, 500,  100,  true, NULL },
    { TM_MCU_TEMP,             ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_MCU_UPTIME,           ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },

    /* FPGA遥测 → FPGA，2秒更新周期，4秒有效期 */
    { TM_FPGA_STATUS,          ACONFIG_DEVICE_FPGA, "main_fpga", 0, 500,  100,  true, NULL },
    { TM_FPGA_TEMP,            ACONFIG_DEVICE_FPGA, "main_fpga", 0, 4000, 2000, true, NULL },
    { TM_FPGA_CONFIG_STATUS,   ACONFIG_DEVICE_FPGA, "main_fpga", 0, 500,  100,  true, NULL },

    /* 系统遥测 */
    { TM_SYSTEM_UPTIME,        ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
    { TM_WATCHDOG_STATUS,      ACONFIG_DEVICE_MCU, "watchdog_mcu", 0, 500,  100,  true, NULL },
    { TM_ERROR_COUNT,          ACONFIG_DEVICE_MCU, "power_mcu", 0, 4000, 2000, true, NULL },
};

const uint32_t g_pmc_tm_config_count = sizeof(g_pmc_tm_configs) / sizeof(aconfig_tm_config_t);
