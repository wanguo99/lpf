/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCONFIG_*       - 通用接口
 * - PCONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "pconfig/pconfig.h"

__attribute__((weak))
const pconfig_platform_table_t g_pconfig_platform_table = { .configs = NULL,
                                                            .count = 0,
                                                            .current_index =
                                                                0 };

/*===========================================================================
 * 平台配置查询
 *===========================================================================*/

const pconfig_platform_config_t *PCONFIG_GetBoard(void)
{
    if (NULL == g_pconfig_platform_table.configs ||
        0u == g_pconfig_platform_table.count ||
        g_pconfig_platform_table.current_index >=
            g_pconfig_platform_table.count) {
        return NULL;
    }

    return g_pconfig_platform_table
        .configs[g_pconfig_platform_table.current_index];
}

const pconfig_platform_config_t *PCONFIG_Find(const char *platform,
                                              const char *product,
                                              const char *version
                                              __attribute__((unused)))
{
    uint32_t i;
    const pconfig_platform_config_t *config;

    if (NULL == platform || NULL == product ||
        NULL == g_pconfig_platform_table.configs) {
        return NULL;
    }

    for (i = 0; i < g_pconfig_platform_table.count; i++) {
        config = g_pconfig_platform_table.configs[i];
        if (NULL == config) {
            continue;
        }

        if (NULL == config->platform_name || NULL == config->product_name) {
            continue;
        }

        if (0 != OSAL_strcmp(config->platform_name, platform)) {
            continue;
        }

        if (0 != OSAL_strcmp(config->product_name, product)) {
            continue;
        }

        return config;
    }

    return NULL;
}

int32_t PCONFIG_list(const pconfig_platform_config_t **configs, uint32_t *count)
{
    uint32_t max_count;
    uint32_t actual_count;
    uint32_t i;

    if (NULL == configs || NULL == count) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == g_pconfig_platform_table.configs) {
        *count = 0;
        return OSAL_SUCCESS;
    }

    max_count = *count;
    actual_count = (g_pconfig_platform_table.count < max_count)
                       ? g_pconfig_platform_table.count
                       : max_count;

    for (i = 0; i < actual_count; i++) {
        configs[i] = g_pconfig_platform_table.configs[i];
    }

    *count = actual_count;

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 硬件外设配置查询接口（PCONFIG_HW_*）
 *
 * 说明：所有 Get 函数已改为 inline 函数，在头文件中实现
 *===========================================================================*/

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t PCONFIG_validate(const pconfig_platform_config_t *config)
{
    if (NULL == config) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config->platform_name || NULL == config->product_name) {
        LOG_ERROR("PCONFIG", "Missing platform or product name");
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

void PCONFIG_print(const pconfig_platform_config_t *config)
{
    uint32_t i;

    if (NULL == config) {
        return;
    }

    LOG_INFO("PCONFIG", "Platform: %s", config->platform_name);
    LOG_INFO("PCONFIG", "Product: %s", config->product_name);

    /* 打印MCU配置 */
    if (config->mcu_array) {
        for (i = 0; i < config->mcu_count; i++) {
            LOG_INFO("PCONFIG",
                     "  MCU[%u]: %s",
                     i,
                     config->mcu_array[i].description
                         ? config->mcu_array[i].description
                         : "N/A");
        }
    }
}
