/**
 * @file acl_api.c
 * @brief ACL层API实现（通用）
 */

#include "acl_api.h"
#include "osal.h"

/* 全局配置表 */
static const acl_config_table_t *g_acl_table = NULL;

/* 读写锁保护全局配置表（读多写少场景） */
static osal_rwlock_t *g_acl_rwlock = NULL;

/**
 * @brief 初始化ACL层
 */
int32_t ACL_Init(void)
{
    int32_t ret;

    g_acl_table = NULL;

    /* 创建读写锁保护全局配置表 */
    ret = OSAL_RwlockCreate(&g_acl_rwlock);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("ACL", "Failed to create rwlock: %d", ret);
        return ret;
    }

    LOG_INFO("ACL", "Initialized");
    return OSAL_SUCCESS;
}

/**
 * @brief 注册配置表
 */
int32_t ACL_RegisterTable(const acl_config_table_t *table)
{
    int32_t ret;

    if (NULL == table) {
        LOG_ERROR("ACL", "Invalid table pointer");
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 获取写锁（独占访问） */
    ret = OSAL_RwlockWrlock(g_acl_rwlock);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("ACL", "Failed to acquire write lock: %d", ret);
        return ret;
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

    /* 释放写锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    return OSAL_SUCCESS;
}

/**
 * @brief 查询遥控配置
 */
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id)
{
    const acl_tc_config_t *config = NULL;

    /* 获取读锁（允许多个读者） */
    if (OSAL_SUCCESS != OSAL_RwlockRdlock(g_acl_rwlock)) {
        return NULL;
    }

    if (NULL != g_acl_table && NULL != g_acl_table->tc_table) {
        /* O(1)直接索引 */
        if (function_id < g_acl_table->tc_count) {
            config = &g_acl_table->tc_table[function_id];
        }
    }

    /* 释放读锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    return config;
}

/**
 * @brief 查询遥测配置
 */
const acl_tm_config_t* ACL_GetTmConfig(uint32_t function_id)
{
    const acl_tm_config_t *config = NULL;

    /* 获取读锁（允许多个读者） */
    if (OSAL_SUCCESS != OSAL_RwlockRdlock(g_acl_rwlock)) {
        return NULL;
    }

    if (NULL != g_acl_table && NULL != g_acl_table->tm_table) {
        /* O(1)直接索引 */
        if (function_id < g_acl_table->tm_count) {
            config = &g_acl_table->tm_table[function_id];
        }
    }

    /* 释放读锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    return config;
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
    int32_t ret = OSAL_SUCCESS;

    if (NULL == affected_ids || NULL == actual_count) {
        return OSAL_ERR_INVALID_POINTER;
    }

    *actual_count = 0;

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_RwlockRdlock(g_acl_rwlock)) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == g_acl_table || NULL == g_acl_table->inv_map) {
        OSAL_RwlockUnlock(g_acl_rwlock);
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
            break;
        }
    }

    /* 释放读锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    return ret;
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

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_RwlockRdlock(g_acl_rwlock)) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == g_acl_table) {
        OSAL_RwlockUnlock(g_acl_rwlock);
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

    /* 释放读锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    return OSAL_SUCCESS;
}

/**
 * @brief 打印配置信息
 */
void ACL_PrintConfig(void)
{
    acl_statistics_t stats = {0};

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_RwlockRdlock(g_acl_rwlock)) {
        LOG_ERROR("ACL", "Failed to acquire read lock");
        return;
    }

    if (NULL == g_acl_table) {
        LOG_INFO("ACL", "No table registered");
        OSAL_RwlockUnlock(g_acl_rwlock);
        return;
    }

    LOG_INFO("ACL", "Configuration: %s", g_acl_table->name);
    LOG_INFO("ACL", "  TC entries: %u", g_acl_table->tc_count);
    LOG_INFO("ACL", "  TM entries: %u", g_acl_table->tm_count);
    LOG_INFO("ACL", "  Invalidation maps: %u", g_acl_table->inv_count);

    /* 释放读锁 */
    OSAL_RwlockUnlock(g_acl_rwlock);

    if (OSAL_SUCCESS == ACL_GetStatistics(&stats)) {
        LOG_INFO("ACL", "  TC enabled: %u, disabled: %u",
                   stats.tc_enabled_count, stats.tc_disabled_count);
        LOG_INFO("ACL", "  TM enabled: %u, disabled: %u",
                   stats.tm_enabled_count, stats.tm_disabled_count);
    }
}
