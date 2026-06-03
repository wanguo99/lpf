/**
 * @file ccm_acl_config.c
 * @brief CCM项目ACL配置实现
 * @note 项目层：提供CCM特定的配置表
 */

#include <aconfig/aconfig_api.h>
#include "aconfig_api.h"
#include "osal.h"

/* 遥控配置表 */
static const aconfig_tc_config_t g_ccm_tc_table[ACONFIG_TC_FUNC_MAX] = {
    /* 电源控制 */
    [ACONFIG_TC_POWER_ON] = {
        .function_id = ACONFIG_TC_POWER_ON,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .user_context = NULL
    },
    [ACONFIG_TC_POWER_OFF] = {
        .function_id = ACONFIG_TC_POWER_OFF,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .user_context = NULL
    },
    [ACONFIG_TC_POWER_RESET] = {
        .function_id = ACONFIG_TC_POWER_RESET,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .enabled = true,
        .user_context = NULL
    },

    /* MCU控制 */
    [ACONFIG_TC_MCU_RESET] = {
        .function_id = ACONFIG_TC_MCU_RESET,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "stm32_mcu",
        .enabled = true,
        .user_context = NULL
    },

    /* FPGA控制 */
    [ACONFIG_TC_FPGA_RESET] = {
        .function_id = ACONFIG_TC_FPGA_RESET,
        .device_type = ACONFIG_DEVICE_FPGA,
        .device_name = NULL,  /* FPGA 未配置 */
        .enabled = true,
        .user_context = NULL
    }
};

/* 遥测配置表 */
static const aconfig_tm_config_t g_ccm_tm_table[ACONFIG_TM_FUNC_MAX] = {
    /* 温度遥测 */
    [ACONFIG_TM_CPU_TEMP] = {
        .function_id = ACONFIG_TM_CPU_TEMP,
        .device_type = ACONFIG_DEVICE_SENSOR,
        .device_name = NULL,  /* 传感器直接读取 */
        .data_validity_ms = 2000,
        .background_update_period_ms = 1000,
        .enabled = true,
        .user_context = NULL
    },

    /* 状态遥测 */
    [ACONFIG_TM_POWER_STATUS] = {
        .function_id = ACONFIG_TM_POWER_STATUS,
        .device_type = ACONFIG_DEVICE_BMC,
        .device_name = "payload_bmc",
        .data_validity_ms = 1000,
        .background_update_period_ms = 500,
        .enabled = true,
        .user_context = NULL
    },
    [ACONFIG_TM_MCU_STATUS] = {
        .function_id = ACONFIG_TM_MCU_STATUS,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "stm32_mcu",
        .data_validity_ms = 1000,
        .background_update_period_ms = 500,
        .enabled = true,
        .user_context = NULL
    }
};

/* 失效映射表 */
static uint32_t g_affected_by_power_off[] = {
    ACONFIG_TM_POWER_STATUS,
    ACONFIG_TM_CPU_TEMP
};

static const aconfig_invalidation_map_t g_ccm_inv_map[] = {
    {
        .source_tm_id = ACONFIG_TC_POWER_OFF,
        .affected_tm_ids = g_affected_by_power_off,
        .affected_count = sizeof(g_affected_by_power_off) / sizeof(uint32_t)
    }
};

/* CCM配置表 */
static const aconfig_config_table_t g_ccm_acl_table = {
    .name = "CCM_H200",
    .tc_table = g_ccm_tc_table,
    .tc_count = ACONFIG_TC_FUNC_MAX,
    .tm_table = g_ccm_tm_table,
    .tm_count = ACONFIG_TM_FUNC_MAX,
    .inv_map = g_ccm_inv_map,
    .inv_count = sizeof(g_ccm_inv_map) / sizeof(aconfig_invalidation_map_t)
};

/**
 * @brief 初始化CCM ACL配置
 * @return OSAL_SUCCESS 成功，其他值失败
 */
int32_t CCM_ACL_Init(void)
{
    int32_t ret;

    /* 初始化ACL框架 */
    ret = ACONFIG_Init();
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("CCM_ACL", "ACONFIG_Init failed, ret=%d", ret);
        return ret;
    }

    /* 注册CCM配置表 */
    ret = ACONFIG_RegisterTable(&g_ccm_acl_table);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("CCM_ACL", "ACONFIG_RegisterTable failed, ret=%d", ret);
        return ret;
    }

    LOG_INFO("CCM_ACL", "Initialized");
    return OSAL_SUCCESS;
}
