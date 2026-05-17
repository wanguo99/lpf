/**
 * @file acl_api.c
 * @brief ACL层API实现
 */

#include "acl_api.h"
#include "acl_config.h"
#include "osal.h"

/* 全局查找表（只读，无需加锁） */
static acl_lookup_table_t g_acl_table;
static bool g_acl_initialized = false;

/* 外部配置数据（由配置文件提供） */
extern const acl_tc_config_t g_pmc_tc_configs[];
extern const uint32_t g_pmc_tc_config_count;
extern const acl_tm_config_t g_pmc_tm_configs[];
extern const uint32_t g_pmc_tm_config_count;
extern const tc_tm_invalidation_map_t g_invalidation_map[];
extern const uint32_t g_invalidation_map_count;

/**
 * @brief 初始化ACL层
 */
int32_t ACL_Init(void)
{
    if (g_acl_initialized) {
        LOG_WARN("ACL", "ACL已初始化，跳过");
        return OSAL_SUCCESS;
    }

    /* 1. 清零查找表 */
    OSAL_Memset(&g_acl_table, 0, sizeof(acl_lookup_table_t));

    /* 2. 初始化遥控查找表 */
    for (uint32_t i = 0; i < g_pmc_tc_config_count; i++) {
        const acl_tc_config_t *cfg = &g_pmc_tc_configs[i];

        /* 检查枚举值范围 */
        if (cfg->function >= TC_FUNC_MAX) {
            LOG_ERROR("ACL", "遥控功能枚举越界: %d", cfg->function);
            return OSAL_ERR_INVALID_SIZE;
        }

        /* 检查重复配置 */
        if (g_acl_table.tc_table[cfg->function].enabled) {
            LOG_ERROR("ACL", "遥控功能重复配置: %d", cfg->function);
            return OSAL_ERR_INVALID_SIZE;
        }

        g_acl_table.tc_table[cfg->function] = *cfg;
    }

    /* 3. 初始化遥测查找表 */
    for (uint32_t i = 0; i < g_pmc_tm_config_count; i++) {
        const acl_tm_config_t *cfg = &g_pmc_tm_configs[i];

        /* 检查枚举值范围 */
        if (cfg->function >= TM_FUNC_MAX) {
            LOG_ERROR("ACL", "遥测功能枚举越界: %d", cfg->function);
            return OSAL_ERR_INVALID_SIZE;
        }

        /* 检查重复配置 */
        if (g_acl_table.tm_table[cfg->function].enabled) {
            LOG_ERROR("ACL", "遥测功能重复配置: %d", cfg->function);
            return OSAL_ERR_INVALID_SIZE;
        }

        /* 检查有效期配置 */
        if (cfg->validity_ms == 0) {
            LOG_ERROR("ACL", "遥测未配置有效期: %d", cfg->function);
            return OSAL_ERR_INVALID_SIZE;
        }

        g_acl_table.tm_table[cfg->function] = *cfg;
    }

    g_acl_initialized = true;
    LOG_INFO("ACL", "ACL初始化完成: TC=%u, TM=%u",
             g_pmc_tc_config_count, g_pmc_tm_config_count);

    return OSAL_SUCCESS;
}

/**
 * @brief 查询遥控配置
 */
const acl_tc_config_t* ACL_GetTcConfig(pmc_tc_function_t function)
{
    if (!g_acl_initialized) {
        LOG_ERROR("ACL", "ACL未初始化");
        return NULL;
    }

    if (function >= TC_FUNC_MAX) {
        LOG_ERROR("ACL", "遥控功能枚举越界: %d", function);
        return NULL;
    }

    const acl_tc_config_t *cfg = &g_acl_table.tc_table[function];
    if (!cfg->enabled) {
        LOG_DEBUG("ACL", "遥控功能未使能: %d", function);
        return NULL;
    }

    return cfg;
}

/**
 * @brief 查询遥测配置
 */
const acl_tm_config_t* ACL_GetTmConfig(pmc_tm_function_t function)
{
    if (!g_acl_initialized) {
        LOG_ERROR("ACL", "ACL未初始化");
        return NULL;
    }

    if (function >= TM_FUNC_MAX) {
        LOG_ERROR("ACL", "遥测功能枚举越界: %d", function);
        return NULL;
    }

    const acl_tm_config_t *cfg = &g_acl_table.tm_table[function];
    if (!cfg->enabled) {
        LOG_DEBUG("ACL", "遥测功能未使能: %d", function);
        return NULL;
    }

    return cfg;
}

/**
 * @brief 检查遥控功能是否使能
 */
bool ACL_IsTcEnabled(pmc_tc_function_t function)
{
    if (!g_acl_initialized || function >= TC_FUNC_MAX) {
        return false;
    }
    return g_acl_table.tc_table[function].enabled;
}

/**
 * @brief 检查遥测功能是否使能
 */
bool ACL_IsTmEnabled(pmc_tm_function_t function)
{
    if (!g_acl_initialized || function >= TM_FUNC_MAX) {
        return false;
    }
    return g_acl_table.tm_table[function].enabled;
}

/**
 * @brief 遥控命令执行后，失效相关遥测缓存
 */
void ACL_InvalidateAffectedTelemetry(pmc_tc_function_t tc_function)
{
    for (uint32_t i = 0; i < g_invalidation_map_count; i++) {
        const tc_tm_invalidation_map_t *map = &g_invalidation_map[i];

        if (map->tc_function == tc_function) {
            for (uint32_t j = 0; j < map->affected_count; j++) {
                pmc_tm_function_t tm_func = map->affected_tm[j];
                LOG_DEBUG("ACL", "遥测缓存标记为STALE: %d (由TC %d触发)",
                         tm_func, tc_function);
            }
            break;
        }
    }
}

/**
 * @brief 验证ACL配置
 */
int32_t ACL_ValidateConfig(void)
{
    uint32_t error_count = 0;

    /* 1. 验证遥控配置 */
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        const acl_tc_config_t *cfg = &g_acl_table.tc_table[i];

        if (!cfg->enabled) {
            continue;
        }

        /* 检查枚举值是否匹配索引 */
        if (cfg->function != i) {
            LOG_ERROR("ACL", "TC[%u]: 枚举值不匹配 (expected=%u, actual=%u)",
                     i, i, cfg->function);
            error_count++;
        }

        /* 检查设备类型 */
        if (cfg->device_type >= ACL_DEVICE_MAX) {
            LOG_ERROR("ACL", "TC[%u]: 无效的设备类型 %d", i, cfg->device_type);
            error_count++;
        }

        /* 检查逻辑索引（假设最多支持4个同类设备） */
        if (cfg->logic_index >= 4) {
            LOG_WARN("ACL", "TC[%u]: 逻辑索引过大 %u", i, cfg->logic_index);
        }
    }

    /* 2. 验证遥测配置 */
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];

        if (!cfg->enabled) {
            continue;
        }

        /* 检查枚举值是否匹配索引 */
        if (cfg->function != i) {
            LOG_ERROR("ACL", "TM[%u]: 枚举值不匹配 (expected=%u, actual=%u)",
                     i, i, cfg->function);
            error_count++;
        }

        /* 检查设备类型 */
        if (cfg->device_type >= ACL_DEVICE_MAX) {
            LOG_ERROR("ACL", "TM[%u]: 无效的设备类型 %d", i, cfg->device_type);
            error_count++;
        }

        /* 检查有效期配置 */
        if (cfg->validity_ms == 0) {
            LOG_ERROR("ACL", "TM[%u]: 未配置有效期", i);
            error_count++;
        }

        /* 检查更新周期配置 */
        if (cfg->update_period_ms == 0) {
            LOG_ERROR("ACL", "TM[%u]: 未配置更新周期", i);
            error_count++;
        }

        /* 检查更新周期与有效期的关系 */
        if (cfg->update_period_ms > cfg->validity_ms) {
            LOG_WARN("ACL", "TM[%u]: 更新周期(%ums) > 有效期(%ums)",
                     i, cfg->update_period_ms, cfg->validity_ms);
        }
    }

    /* 3. 验证失效映射表 */
    for (uint32_t i = 0; i < g_invalidation_map_count; i++) {
        const tc_tm_invalidation_map_t *map = &g_invalidation_map[i];

        /* 检查遥控功能枚举范围 */
        if (map->tc_function >= TC_FUNC_MAX) {
            LOG_ERROR("ACL", "失效映射[%u]: 遥控功能枚举越界 %d", i, map->tc_function);
            error_count++;
            continue;
        }

        /* 检查遥控功能是否使能 */
        if (!ACL_IsTcEnabled(map->tc_function)) {
            LOG_WARN("ACL", "失效映射[%u]: 遥控功能未使能 %d",
                     i, map->tc_function);
        }

        /* 检查受影响的遥测功能 */
        for (uint32_t j = 0; j < map->affected_count; j++) {
            pmc_tm_function_t tm_func = map->affected_tm[j];

            if (tm_func >= TM_FUNC_MAX) {
                LOG_ERROR("ACL", "失效映射[%u]: 遥测功能枚举越界 %d", i, tm_func);
                error_count++;
            }
        }
    }

    if (error_count > 0) {
        LOG_ERROR("ACL", "配置验证失败: %u个错误", error_count);
        return OSAL_ERR_INVALID_SIZE;
    }

    LOG_INFO("ACL", "配置验证通过");
    return OSAL_SUCCESS;
}

/**
 * @brief 获取ACL配置统计信息
 */
void ACL_GetStatistics(acl_statistics_t *stats)
{
    OSAL_Memset(stats, 0, sizeof(acl_statistics_t));

    /* 统计遥控配置 */
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        if (g_acl_table.tc_table[i].enabled) {
            stats->tc_enabled_count++;
        } else {
            stats->tc_disabled_count++;
        }
    }

    /* 统计遥测配置 */
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];

        if (cfg->enabled) {
            stats->tm_enabled_count++;
        } else {
            stats->tm_disabled_count++;
        }
    }

    stats->invalidation_map_count = g_invalidation_map_count;
}

/**
 * @brief 打印ACL配置统计信息
 */
void ACL_PrintStatistics(void)
{
    acl_statistics_t stats;
    ACL_GetStatistics(&stats);

    LOG_INFO("ACL", "========== ACL配置统计 ==========");
    LOG_INFO("ACL", "遥控功能: 使能=%u, 禁用=%u, 总计=%u",
             stats.tc_enabled_count, stats.tc_disabled_count, TC_FUNC_MAX);
    LOG_INFO("ACL", "遥测功能: 使能=%u, 禁用=%u, 总计=%u",
             stats.tm_enabled_count, stats.tm_disabled_count, TM_FUNC_MAX);
    LOG_INFO("ACL", "失效映射: %u", stats.invalidation_map_count);
    LOG_INFO("ACL", "================================");
}

/**
 * @brief 打印ACL配置详情（调试用）
 */
void ACL_DumpConfig(void)
{
    LOG_INFO("ACL", "========== 遥控配置 ==========");
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        const acl_tc_config_t *cfg = &g_acl_table.tc_table[i];
        if (cfg->enabled) {
            LOG_INFO("ACL", "TC[%2u]: device=%u, index=%u",
                     i, cfg->device_type, cfg->logic_index);
        }
    }

    LOG_INFO("ACL", "========== 遥测配置 ==========");
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];
        if (cfg->enabled) {
            LOG_INFO("ACL", "TM[%2u]: device=%u, index=%u, validity=%ums, period=%ums",
                     i, cfg->device_type, cfg->logic_index,
                     cfg->validity_ms, cfg->update_period_ms);
        }
    }
}
