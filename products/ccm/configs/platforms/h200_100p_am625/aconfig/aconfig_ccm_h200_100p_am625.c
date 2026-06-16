/**
 * @file aconfig_ccm_h200_100p_am625.c
 * @brief CCM H200-100P-AM625 Platform AConfig Configuration
 * @note Product layer: CCM-specific configuration tables
 *       Migrated to optimized format: sparse array + device index + embedded invalidation maps
 */

#include "osal.h"
#include "aconfig.h"
#include "ccm_tc_functions.h"
#include "ccm_tm_functions.h"

/*===========================================================================
 * 失效映射定义（内嵌方式）
 *===========================================================================*/

/* 电源控制相关的失效映射 */
static const uint32_t tc_power_off_affects[] = {
    CCM_TM_POWER_STATUS,
    CCM_TM_CPU_TEMP
};

static const uint32_t tc_power_on_affects[] = {
    CCM_TM_POWER_STATUS
};

static const uint32_t tc_power_reset_affects[] = {
    CCM_TM_POWER_STATUS,
    CCM_TM_MCU_STATUS
};

/* MCU 控制相关的失效映射 */
static const uint32_t tc_mcu_reset_affects[] = {
    CCM_TM_MCU_STATUS
};

/*===========================================================================
 * 遥控配置（稀疏数组 - 只定义实际使用的功能）
 *===========================================================================*/

static const aconfig_tc_entry_t g_ccm_tc_entries[] = {
    /* 电源控制 → BMC */
    {
        .function_id = CCM_TC_POWER_ON,
        .config = {
            .function_id = CCM_TC_POWER_ON,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},  // payload_bmc
            .invalidated_tm_ids = tc_power_on_affects,
            .invalidated_tm_count = sizeof(tc_power_on_affects) / sizeof(uint32_t),
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = CCM_TC_POWER_OFF,
        .config = {
            .function_id = CCM_TC_POWER_OFF,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},  // payload_bmc
            .invalidated_tm_ids = tc_power_off_affects,
            .invalidated_tm_count = sizeof(tc_power_off_affects) / sizeof(uint32_t),
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = CCM_TC_POWER_RESET,
        .config = {
            .function_id = CCM_TC_POWER_RESET,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},  // payload_bmc
            .invalidated_tm_ids = tc_power_reset_affects,
            .invalidated_tm_count = sizeof(tc_power_reset_affects) / sizeof(uint32_t),
            .enabled = true,
            .user_context = NULL
        }
    },

    /* MCU 控制 → MCU */
    {
        .function_id = CCM_TC_MCU_RESET,
        .config = {
            .function_id = CCM_TC_MCU_RESET,
            .device = {.type = PCONFIG_DEVICE_MCU, .index = 0},  // stm32_mcu
            .invalidated_tm_ids = tc_mcu_reset_affects,
            .invalidated_tm_count = sizeof(tc_mcu_reset_affects) / sizeof(uint32_t),
            .enabled = true,
            .user_context = NULL
        }
    },

    /* FPGA 控制 → FPGA */
    {
        .function_id = CCM_TC_FPGA_RESET,
        .config = {
            .function_id = CCM_TC_FPGA_RESET,
            .device = {.type = PCONFIG_DEVICE_FPGA, .index = 0},  // FPGA 未配置
            .invalidated_tm_ids = NULL,
            .invalidated_tm_count = 0,
            .enabled = true,
            .user_context = NULL
        }
    }
};

/*===========================================================================
 * 遥测配置（稀疏数组 - 只定义实际使用的功能）
 *===========================================================================*/

static const aconfig_tm_entry_t g_ccm_tm_entries[] = {
    /* 温度遥测 → 传感器 */
    {
        .function_id = CCM_TM_CPU_TEMP,
        .config = {
            .function_id = CCM_TM_CPU_TEMP,
            .device = {.type = PCONFIG_DEVICE_SENSOR, .index = 0},  // 传感器直接读取
            .poll_period_ms = 1000,
            .validity_period_ms = 2000,
            .enabled = true,
            .user_context = NULL
        }
    },

    /* 状态遥测 → BMC/MCU */
    {
        .function_id = CCM_TM_POWER_STATUS,
        .config = {
            .function_id = CCM_TM_POWER_STATUS,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},  // payload_bmc
            .poll_period_ms = 500,
            .validity_period_ms = 1000,
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = CCM_TM_MCU_STATUS,
        .config = {
            .function_id = CCM_TM_MCU_STATUS,
            .device = {.type = PCONFIG_DEVICE_MCU, .index = 0},  // stm32_mcu
            .poll_period_ms = 500,
            .validity_period_ms = 1000,
            .enabled = true,
            .user_context = NULL
        }
    }
};

/*===========================================================================
 * CCM 配置表（新格式）
 *===========================================================================*/

static const aconfig_config_table_t g_ccm_aconfig_table = {
    .name = "CCM_H200",
    .hwid_count = 0,      // 支持所有 HWID
    .hwid_list = NULL,

    /* 稀疏数组：只存储实际定义的功能 */
    .tc_entries = g_ccm_tc_entries,
    .tc_count = sizeof(g_ccm_tc_entries) / sizeof(aconfig_tc_entry_t),

    .tm_entries = g_ccm_tm_entries,
    .tm_count = sizeof(g_ccm_tm_entries) / sizeof(aconfig_tm_entry_t)
};

/**
 * @brief 初始化 CCM AConfig 配置
 * @return OSAL_SUCCESS 成功，其他值失败
 */
int32_t CCM_ACONFIG_Init(void)
{
    int32_t ret;

    /* 初始化 AConfig 框架 */
    ret = ACONFIG_Init();
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("CCM_ACONFIG", "ACONFIG_Init failed, ret=%d", ret);
        return ret;
    }

    /* 注册 CCM 配置表（新格式） */
    ret = ACONFIG_RegisterTable(&g_ccm_aconfig_table);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("CCM_ACONFIG", "ACONFIG_RegisterTable failed, ret=%d", ret);
        return ret;
    }

    LOG_INFO("CCM_ACONFIG", "Initialized (optimized format)");
    LOG_INFO("CCM_ACONFIG", "  TC entries: %u (sparse array)",
             g_ccm_aconfig_table.tc_count);
    LOG_INFO("CCM_ACONFIG", "  TM entries: %u (sparse array)",
             g_ccm_aconfig_table.tm_count);

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 迁移说明
 *===========================================================================
 *
 * 从旧格式迁移到新格式：
 *
 * 1. 设备引用方式变化：
 *    旧：.device_name = "payload_bmc"
 *    新：.device = {.type = PCONFIG_DEVICE_BMC, .index = 0}
 *
 * 2. 配置存储方式变化：
 *    旧：密集数组 [ACONFIG_TC_FUNC_MAX] (1000个元素)
 *    新：稀疏数组 (只包含实际定义的5个元素)
 *
 * 3. 失效映射变化：
 *    旧：独立的 g_ccm_inv_map[] 数组
 *    新：内嵌到 TC 配置的 .invalidated_tm_ids 字段
 *
 * 4. 字段名称变化：
 *    旧：.background_update_period_ms
 *    新：.poll_period_ms
 *
 *    旧：.data_validity_ms
 *    新：.validity_period_ms
 *
 * 5. 注册 API 变化：
 *    旧：ACONFIG_RegisterTableLegacy(&g_ccm_aconfig_table)
 *    新：ACONFIG_RegisterTable(&g_ccm_aconfig_table)
 *
 * 内存占用对比：
 *   旧格式：~40KB (1000个TC + 1000个TM的密集数组)
 *   新格式：~2KB  (5个TC + 3个TM的稀疏数组)
 *   节省：95%
 *
 *===========================================================================*/
