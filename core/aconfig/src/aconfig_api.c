/**
 * @file aconfig_api.c
 * @brief ACONFIG 层 API 实现（优化版）
 * @note 优化要点：
 *       1. 适配稀疏数组结构（tc_entries/tm_entries）
 *       2. 失效映射从 TC 配置中内嵌读取
 */

#include "osal.h"
#include "aconfig.h"
#include "aconfig_internal.h"

/* 全局配置表 */
static const aconfig_config_table_t *g_aconfig_table = NULL;

/* 读写锁保护全局配置表（读多写少场景） */
static osal_rwlock_t g_aconfig_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * @brief 初始化ACONFIG层
 */
int32_t ACONFIG_Init(void)
{
    g_aconfig_table = NULL;

    LOG_INFO("ACONFIG", "Initialized (optimized version)");
    return OSAL_SUCCESS;
}

/**
 * @brief 注册配置表
 */
int32_t ACONFIG_RegisterTable(const aconfig_config_table_t *table)
{
    int32_t ret;

    if (NULL == table) {
        LOG_ERROR("ACONFIG", "Invalid table pointer");
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 获取写锁（独占访问） */
    ret = OSAL_pthread_rwlock_wrlock(&g_aconfig_rwlock);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("ACONFIG", "Failed to acquire write lock: %d", ret);
        return ret;
    }

    if (NULL != g_aconfig_table) {
        LOG_WARN("ACONFIG", "Table already registered, overwriting");
    }

    g_aconfig_table = table;

    LOG_INFO("ACONFIG", "Registered table '%s' (TC:%u entries, TM:%u entries)",
               table->name,
               table->tc_count,
               table->tm_count);

    /* 释放写锁 */
    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    return OSAL_SUCCESS;
}

/**
 * @brief 查询遥控配置（稀疏数组查找）
 */
const aconfig_tc_config_t* ACONFIG_GetTcConfig(uint32_t function_id)
{
    const aconfig_tc_config_t *config = NULL;
    uint32_t i;

    /* 获取读锁（允许多个读者） */
    if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
        return NULL;
    }

    /* NULL 检查 */
    if (NULL == g_aconfig_table || NULL == g_aconfig_table->tc_entries) {
        OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
        return NULL;
    }

    /* 查找匹配的 function_id（稀疏数组线性查找）*/
    for (i = 0; i < g_aconfig_table->tc_count; i++) {
        if (g_aconfig_table->tc_entries[i].function_id == function_id) {
            config = &g_aconfig_table->tc_entries[i].config;
            break;
        }
    }

    /* 释放读锁 */
    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    return config;
}

/**
 * @brief 查询遥测配置（稀疏数组查找）
 */
const aconfig_tm_config_t* ACONFIG_GetTmConfig(uint32_t function_id)
{
    const aconfig_tm_config_t *config = NULL;
    uint32_t i;

    /* 获取读锁（允许多个读者） */
    if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
        return NULL;
    }

    /* NULL 检查 */
    if (NULL == g_aconfig_table || NULL == g_aconfig_table->tm_entries) {
        OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
        return NULL;
    }

    /* 查找匹配的 function_id（稀疏数组线性查找）*/
    for (i = 0; i < g_aconfig_table->tm_count; i++) {
        if (g_aconfig_table->tm_entries[i].function_id == function_id) {
            config = &g_aconfig_table->tm_entries[i].config;
            break;
        }
    }

    /* 释放读锁 */
    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    return config;
}

/**
 * @brief 检查遥控功能是否使能
 */
bool ACONFIG_IsTcEnabled(uint32_t function_id)
{
    const aconfig_tc_config_t *config = ACONFIG_GetTcConfig(function_id);
    return (NULL != config) && config->enabled;
}

/**
 * @brief 检查遥测功能是否使能
 */
bool ACONFIG_IsTmEnabled(uint32_t function_id)
{
    const aconfig_tm_config_t *config = ACONFIG_GetTmConfig(function_id);
    return (NULL != config) && config->enabled;
}

/**
 * @brief 获取失效映射（优化版 - 从 TC 配置中读取）
 * @note 新版本：失效映射已内嵌到 TC 配置中，通过 TC function_id 查找
 */
int32_t ACONFIG_GetInvalidationMap(uint32_t tc_function_id,
                                uint32_t *affected_ids,
                                uint32_t max_count,
                                uint32_t *actual_count)
{
    const aconfig_tc_config_t *tc_config;
    uint32_t copy_count;

    if (NULL == affected_ids || NULL == actual_count) {
        return OSAL_ERR_INVALID_POINTER;
    }

    *actual_count = 0;

    /* 查找对应的 TC 配置 */
    tc_config = ACONFIG_GetTcConfig(tc_function_id);
    if (NULL == tc_config) {
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 检查是否有失效映射 */
    if (NULL == tc_config->invalidated_tm_ids || 0 == tc_config->invalidated_tm_count) {
        return OSAL_SUCCESS;  /* 没有失效映射，不是错误 */
    }

    /* 复制受影响的 TM ID */
    copy_count = (tc_config->invalidated_tm_count < max_count) ?
                  tc_config->invalidated_tm_count : max_count;
    OSAL_memcpy(affected_ids, tc_config->invalidated_tm_ids,
                copy_count * OSAL_sizeof(uint32_t));
    *actual_count = tc_config->invalidated_tm_count;

    return OSAL_SUCCESS;
}

/**
 * @brief 获取配置统计信息（优化版）
 */
int32_t ACONFIG_GetStatistics(aconfig_statistics_t *stats)
{
    uint32_t i;
    uint32_t total_inv_maps = 0;

    if (NULL == stats) {
        return OSAL_ERR_INVALID_POINTER;
    }

    OSAL_memset(stats, 0, OSAL_sizeof(aconfig_statistics_t));

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == g_aconfig_table) {
        OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
        return OSAL_SUCCESS;
    }

    /* 统计遥控配置 */
    if (NULL != g_aconfig_table->tc_entries) {
        for (i = 0; i < g_aconfig_table->tc_count; i++) {
            if (g_aconfig_table->tc_entries[i].config.enabled) {
                stats->tc_enabled_count++;
            } else {
                stats->tc_disabled_count++;
            }

            /* 统计失效映射 */
            if (g_aconfig_table->tc_entries[i].config.invalidated_tm_count > 0) {
                total_inv_maps++;
            }
        }
    }

    /* 统计遥测配置 */
    if (NULL != g_aconfig_table->tm_entries) {
        for (i = 0; i < g_aconfig_table->tm_count; i++) {
            if (g_aconfig_table->tm_entries[i].config.enabled) {
                stats->tm_enabled_count++;
            } else {
                stats->tm_disabled_count++;
            }
        }
    }

    stats->total_invalidation_maps = total_inv_maps;

    /* 释放读锁 */
    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    return OSAL_SUCCESS;
}

/**
 * @brief 打印配置信息（优化版）
 */
void ACONFIG_PrintConfig(void)
{
    aconfig_statistics_t stats = {0};

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
        LOG_ERROR("ACONFIG", "Failed to acquire read lock");
        return;
    }

    if (NULL == g_aconfig_table) {
        LOG_INFO("ACONFIG", "No table registered");
        OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
        return;
    }

    LOG_INFO("ACONFIG", "Configuration: %s", g_aconfig_table->name);
    LOG_INFO("ACONFIG", "  TC entries: %u (sparse array)", g_aconfig_table->tc_count);
    LOG_INFO("ACONFIG", "  TM entries: %u (sparse array)", g_aconfig_table->tm_count);

    /* 释放读锁 */
    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    if (OSAL_SUCCESS == ACONFIG_GetStatistics(&stats)) {
        LOG_INFO("ACONFIG", "  TC enabled: %u, disabled: %u",
                   stats.tc_enabled_count, stats.tc_disabled_count);
        LOG_INFO("ACONFIG", "  TM enabled: %u, disabled: %u",
                   stats.tm_enabled_count, stats.tm_disabled_count);
        LOG_INFO("ACONFIG", "  Invalidation maps: %u (embedded in TC)",
                   stats.total_invalidation_maps);
    }
}

/*===========================================================================
 * HWID 相关接口
 *===========================================================================*/

#ifdef CONFIG_PDL
#include "pdl_misc.h"

const aconfig_config_table_t* ACONFIG_FindTableByHWID(const pdl_hwid_t *hwid)
{
    /* TODO: 需要维护一个全局的配置表注册列表 */
    /* 当前简化实现：直接返回已注册的表（假设匹配） */

    if (hwid == NULL) {
        LOG_ERROR("ACONFIG", "Invalid HWID pointer");
        return NULL;
    }

    /* 获取读锁 */
    if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
        return NULL;
    }

    /* 检查当前表是否支持该 HWID */
    if (g_aconfig_table != NULL) {
        /* 如果配置表的 hwid_count == 0，表示支持所有 HWID */
        if (g_aconfig_table->hwid_count == 0 || g_aconfig_table->hwid_list == NULL) {
            OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
            LOG_INFO("ACONFIG", "Current table '%s' supports all HWIDs", g_aconfig_table->name);
            return g_aconfig_table;
        }

        /* TODO: 实现精确的 HWID 匹配 */
        LOG_WARN("ACONFIG", "HWID matching not fully implemented yet");
    }

    OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

    LOG_INFO("ACONFIG", "HWID: product=0x%04X, project=0x%04X",
             hwid->product_id, hwid->project_id);

    return g_aconfig_table;  /* 简化实现：返回当前表 */
}

int32_t ACONFIG_LoadByHWID(void)
{
    pdl_hwid_t hwid;
    const aconfig_config_table_t *table;
    int32_t ret;

    /* 从 PDL_MISC 读取 HWID */
    ret = PDL_MISC_GetHWID(&hwid);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("ACONFIG", "Failed to read HWID: %d", ret);
        return ret;
    }

    LOG_INFO("ACONFIG", "Read HWID: product=0x%04X, project=0x%04X, board=0x%02X, hw_rev=0x%02X",
             hwid.product_id, hwid.project_id, hwid.board_type, hwid.hw_revision);

    /* 根据 HWID 查找配置表 */
    table = ACONFIG_FindTableByHWID(&hwid);
    if (table == NULL) {
        LOG_ERROR("ACONFIG", "No matching config table for HWID");
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 注册配置表 */
    ret = ACONFIG_RegisterTable(table);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("ACONFIG", "Failed to register config table: %d", ret);
        return ret;
    }

    LOG_INFO("ACONFIG", "Successfully loaded config for HWID");
    return OSAL_SUCCESS;
}

#else
/* PDL 未启用时的占位实现 */
const aconfig_config_table_t* ACONFIG_FindTableByHWID(const pdl_hwid_t *hwid)
{
    (void)hwid;
    LOG_ERROR("ACONFIG", "HWID support requires CONFIG_PDL=y");
    return NULL;
}

int32_t ACONFIG_LoadByHWID(void)
{
    LOG_ERROR("ACONFIG", "HWID support requires CONFIG_PDL=y");
    return OSAL_ERR_NOT_SUPPORTED;
}
#endif /* CONFIG_PDL */
