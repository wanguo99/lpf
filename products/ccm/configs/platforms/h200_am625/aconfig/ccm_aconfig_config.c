/**
 * @file ccm_acl_config.c
 * @brief PMC项目ACL配置实现
 * @note 项目层：提供PMC特定的配置表
 */

#include "aconfig_config.h"
#include "aconfig_api.h"
#include "osal.h"

/* 遥控配置表 */
static const aconfig_tc_config_t g_pmc_tc_table[TC_FUNC_MAX] = {
    /* 电源控制 */
    [TC_POWER_ON] = {
        .function_id = TC_POWER_ON,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .extra_data = NULL
    },
    [TC_POWER_OFF] = {
        .function_id = TC_POWER_OFF,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .extra_data = NULL
    },
    [TC_POWER_RESET] = {
        .function_id = TC_POWER_RESET,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .extra_data = NULL
    },

    /* MCU控制 */
    [TC_MCU_RESET] = {
        .function_id = TC_MCU_RESET,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "stm32_mcu",
        .enabled = true,
        .extra_data = NULL
    },

    /* FPGA控制 */
    [TC_FPGA_RESET] = {
        .function_id = TC_FPGA_RESET,
        .device_type = ACONFIG_DEVICE_FPGA,
        .device_name = NULL,  /* FPGA 未配置 */
        .enabled = true,
        .extra_data = NULL
    }
};

/* 遥测配置表 */
static const aconfig_tm_config_t g_pmc_tm_table[TM_FUNC_MAX] = {
    /* 温度遥测 */
    [TM_CPU_TEMP] = {
        .function_id = TM_CPU_TEMP,
        .device_type = ACONFIG_DEVICE_SENSOR,
        .device_name = NULL,  /* 传感器直接读取 */
        .validity_ms = 2000,
        .update_period_ms = 1000,
        .enabled = true,
        .extra_data = NULL
    },

    /* 状态遥测 */
    [TM_POWER_STATUS] = {
        .function_id = TM_POWER_STATUS,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .validity_ms = 1000,
        .update_period_ms = 500,
        .enabled = true,
        .extra_data = NULL
    },
    [TM_MCU_STATUS] = {
        .function_id = TM_MCU_STATUS,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "stm32_mcu",
        .validity_ms = 1000,
        .update_period_ms = 500,
        .enabled = true,
        .extra_data = NULL
    }
};

/* 失效映射表 */
static uint32_t g_affected_by_power_off[] = {
    TM_POWER_STATUS,
    TM_CPU_TEMP
};

static const aconfig_invalidation_map_t g_pmc_inv_map[] = {
    {
        .source_tm_id = TC_POWER_OFF,
        .affected_tm_ids = g_affected_by_power_off,
        .affected_count = sizeof(g_affected_by_power_off) / sizeof(uint32_t)
    }
};

/* PMC配置表 */
static const aconfig_config_table_t g_pmc_acl_table = {
    .name = "PMC_H200",
    .tc_table = g_pmc_tc_table,
    .tc_count = TC_FUNC_MAX,
    .tm_table = g_pmc_tm_table,
    .tm_count = TM_FUNC_MAX,
    .inv_map = g_pmc_inv_map,
    .inv_count = sizeof(g_pmc_inv_map) / sizeof(aconfig_invalidation_map_t)
};

/**
 * @brief 初始化PMC ACL配置
 * @return OSAL_SUCCESS 成功，其他值失败
 */
int32_t PMC_ACL_Init(void)
{
    int32_t ret;

    /* 初始化ACL框架 */
    ret = ACONFIG_Init();
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PMC_ACL", "ACONFIG_Init failed, ret=%d", ret);
        return ret;
    }

    /* 注册PMC配置表 */
    ret = ACONFIG_RegisterTable(&g_pmc_acl_table);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PMC_ACL", "ACONFIG_RegisterTable failed, ret=%d", ret);
        return ret;
    }

    LOG_INFO("PMC_ACL", "Initialized");
    return OSAL_SUCCESS;
}
