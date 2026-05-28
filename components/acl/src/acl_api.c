/**
 * @file acl_api.c
 * @brief ACL层API实现（通用）
 */

#include "acl/acl_api.h"
#include "osal/osal.h"

/* 全局配置表 */
static const acl_config_table_t *g_acl_table = NULL;

/**
 * @brief 初始化ACL层
 */
int32_t ACL_Init(void)
{
    g_acl_table = NULL;
    LOG_INFO("ACL", "Initialized");
    return OSAL_SUCCESS;
}

/**
 * @brief 注册配置表
 */
int32_t ACL_RegisterTable(const acl_config_table_t *table)
{
    if (NULL == table) {
        LOG_ERROR("ACL", "Invalid table pointer");
        return OSAL_ERR_INVALID_POINTER;
    }

    if (NULL != g_acl_table) {
        LOG_WARN("ACL", "Table already registered, overwriting");
    }

    g_acl_table = table;

    LOG_INFO("ACL", "Registered table '%s' (TC:%u, TM:%u, INV:%u)",
               table->name,
               table->tc_count,
               table->tm_count,
               table->inv_count);

    return OSAL_SUCCESS;
}

/**
 * @brief 查询遥控配置
 */
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id)
{
    if (NULL == g_acl_table || NULL == g_acl_table->tc_table) {
        return NULL;
    }

    /* O(1)直接索引 */
    if (function_id < g_acl_table->tc_count) {
        return &g_acl_table->tc_table[function_id];
    }

    return NULL;
}

/**
 * @brief 查询遥测配置
 */
const acl_tm_config_t* ACL_GetTmConfig(uint32_t function_id)
{
    if (NULL == g_acl_table || NULL == g_acl_table->tm_table) {
        return NULL;
    }

    /* O(1)直接索引 */
    if (function_id < g_acl_table->tm_count) {
        return &g_acl_table->tm_table[function_id];
    }

    return NULL;
}

/**
 * @brief 检查遥控功能是否使能
 */
bool ACL_IsTcEnabled(uint32_t function_id)
{
    const acl_tc_config_t *config = ACL_GetTcConfig(function_id);
    return (NULL != config) && config->enabled;
}

/**
 * @brief 检查遥测功能是否使能
 */
bool ACL_IsTmEnabled(uint32_t function_id)
{
    const acl_tm_config_t *config = ACL_GetTmConfig(function_id);
    return (NULL != config) && config->enabled;
}

/**
 * @brief 获取失效映射
 */
int32_t ACL_GetInvalidationMap(uint32_t source_tm_id,
                                uint32_t *affected_ids,
                                uint32_t max_count,
                                uint32_t *actual_count)
{
    uint32_t i;

    if (NULL == affected_ids || NULL == actual_count) {
        return OSAL_ERR_INVALID_POINTER;
    }

    *actual_count = 0;

    if (NULL == g_acl_table || NULL == g_acl_table->inv_map) {
        return OSAL_SUCCESS;
    }

    /* 查找源遥测ID */
    for (i = 0; i < g_acl_table->inv_count; i++) {
        const acl_invalidation_map_t *map = &g_acl_table->inv_map[i];
        if (map->source_tm_id == source_tm_id) {
            /* 复制受影响的ID */
            uint32_t copy_count = (map->affected_count < max_count) ?
                                   map->affected_count : max_count;
            OSAL_Memcpy(affected_ids, map->affected_tm_ids, copy_count * sizeof(uint32_t));
            *actual_count = map->affected_count;
            return OSAL_SUCCESS;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 获取配置统计信息
 */
int32_t ACL_GetStatistics(acl_statistics_t *stats)
{
    if (NULL == stats) {
        return OSAL_ERR_INVALID_POINTER;
    }

    OSAL_Memset(stats, 0, sizeof(acl_statistics_t));

    if (NULL == g_acl_table) {
        return OSAL_SUCCESS;
    }

    /* 统计遥控配置 */
    if (NULL != g_acl_table->tc_table) {
        uint32_t i;

        for (i = 0; i < g_acl_table->tc_count; i++) {
            if (g_acl_table->tc_table[i].enabled) {
                stats->tc_enabled_count++;
            } else {
                stats->tc_disabled_count++;
            }
        }
    }

    /* 统计遥测配置 */
    if (NULL != g_acl_table->tm_table) {
        uint32_t i;

        for (i = 0; i < g_acl_table->tm_count; i++) {
            if (g_acl_table->tm_table[i].enabled) {
                stats->tm_enabled_count++;
            } else {
                stats->tm_disabled_count++;
            }
        }
    }

    stats->invalidation_map_count = g_acl_table->inv_count;

    return OSAL_SUCCESS;
}

/**
 * @brief 打印配置信息
 */
void ACL_PrintConfig(void)
{
    acl_statistics_t stats = {0};

    if (NULL == g_acl_table) {
        LOG_INFO("ACL", "No table registered");
        return;
    }

    LOG_INFO("ACL", "Configuration: %s", g_acl_table->name);
    LOG_INFO("ACL", "  TC entries: %u", g_acl_table->tc_count);
    LOG_INFO("ACL", "  TM entries: %u", g_acl_table->tm_count);
    LOG_INFO("ACL", "  Invalidation maps: %u", g_acl_table->inv_count);
    if (OSAL_SUCCESS == ACL_GetStatistics(&stats)) {
        LOG_INFO("ACL", "  TC enabled: %u, disabled: %u",
                   stats.tc_enabled_count, stats.tc_disabled_count);
        LOG_INFO("ACL", "  TM enabled: %u, disabled: %u",
                   stats.tm_enabled_count, stats.tm_disabled_count);
    }
}
