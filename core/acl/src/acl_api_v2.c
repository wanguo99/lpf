/**
 * @file acl_api_v2.c
 * @brief ACL层实现（纯配置层，V2版本）
 *
 * 重构说明：
 * - ACL层只包含静态配置数据和查询接口
 * - 移除了遥测缓存管理（已移至OSAL层的osal_shm_cache）
 * - 移除了共享内存、互斥锁等运行时资源管理
 * - 保持配置数据的只读特性
 *
 * 注意：此文件提供 V2 版本的 API（函数名带 _V2 后缀），
 * 可以与 acl_api.c 共存，用于测试或对比不同实现。
 */

#include "acl_api_v2.h"
#include "acl_config.h"
#include "osal.h"

/* 配置初始化标志 */
static bool g_acl_initialized = false;

/* 遥控命令配置表（示例，实际应从配置文件加载） */
static const acl_tc_config_t g_tc_configs[] = {
    /* 电源控制 */
    {
        .function_id = TC_POWER_ON,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    },
    {
        .function_id = TC_POWER_OFF,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    },
    {
        .function_id = TC_POWER_RESET,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    },
    /* MCU控制 */
    {
        .function_id = TC_MCU_RESET,
        .device_type = ACL_DEVICE_MCU,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    }
};

/* 遥测配置表（示例） */
static const acl_tm_config_t g_tm_configs[] = {
    /* 服务器遥测 */
    {
        .function_id = TM_CPU_TEMP,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .validity_ms = 2000,      /* 2秒有效期 */
        .update_period_ms = 1000, /* 1秒更新周期 */
        .enabled = true,
        .extra_data = NULL
    },
    {
        .function_id = TM_POWER_STATUS,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .validity_ms = 500,       /* 500ms有效期 */
        .update_period_ms = 100,  /* 100ms更新周期（快遥） */
        .enabled = true,
        .extra_data = NULL
    },
    /* MCU遥测 */
    {
        .function_id = TM_MCU_STATUS,
        .device_type = ACL_DEVICE_MCU,
        .logic_index = 0,
        .validity_ms = 4000,      /* 4秒有效期 */
        .update_period_ms = 2000, /* 2秒更新周期（慢遥） */
        .enabled = true,
        .extra_data = NULL
    }
};

/* 遥控命令失效映射表（示例） */
typedef struct {
    acl_tc_function_t tc_id;
    acl_tm_function_t tm_ids[8];  /* 受影响的遥测ID列表 */
    uint32_t tm_count;
} tc_invalidation_map_t;

static const tc_invalidation_map_t g_invalidation_map[] = {
    /* 电源控制会失效电源状态遥测 */
    {
        .tc_id = TC_POWER_ON,
        .tm_ids = { TM_POWER_STATUS },
        .tm_count = 1
    },
    {
        .tc_id = TC_POWER_OFF,
        .tm_ids = { TM_POWER_STATUS },
        .tm_count = 1
    },
    {
        .tc_id = TC_POWER_RESET,
        .tm_ids = { TM_POWER_STATUS, TM_CPU_TEMP },
        .tm_count = 2
    },
    /* MCU复位会失效MCU状态遥测 */
    {
        .tc_id = TC_MCU_RESET,
        .tm_ids = { TM_MCU_STATUS },
        .tm_count = 1
    }
};

/**
 * @brief 初始化ACL配置系统（V2版本）
 */
int32_t ACL_Init_V2(void)
{
    if (g_acl_initialized) {
        return OSAL_SUCCESS;
    }

    /* ACL层只加载配置，不创建运行时资源 */
    LOG_INFO("ACL", "ACL配置系统初始化完成");
    LOG_INFO("ACL", "遥控命令配置数量: %u",
             (uint32_t)(sizeof(g_tc_configs) / sizeof(g_tc_configs[0])));
    LOG_INFO("ACL", "遥测配置数量: %u",
             (uint32_t)(sizeof(g_tm_configs) / sizeof(g_tm_configs[0])));

    g_acl_initialized = true;

    return OSAL_SUCCESS;
}

/**
 * @brief 清理ACL配置系统（V2版本）
 */
void ACL_Deinit_V2(void)
{
    if (!g_acl_initialized) {
        return;
    }

    /* ACL层没有运行时资源需要清理 */
    LOG_INFO("ACL", "ACL配置系统已清理");

    g_acl_initialized = false;
}

/**
 * @brief 获取遥控命令配置（V2版本）
 */
const acl_tc_config_t* ACL_GetTcConfig_V2(acl_tc_function_t tc_id)
{
    uint32_t i;
    uint32_t count = sizeof(g_tc_configs) / sizeof(g_tc_configs[0]);

    if (!g_acl_initialized) {
        return NULL;
    }

    /* O(n)查找，可优化为O(1)哈希表 */
    for (i = 0; i < count; i++) {
        if (g_tc_configs[i].function_id == tc_id) {
            return &g_tc_configs[i];
        }
    }

    return NULL;
}

/**
 * @brief 获取遥测配置（V2版本）
 */
const acl_tm_config_t* ACL_GetTmConfig_V2(acl_tm_function_t tm_id)
{
    uint32_t i;
    uint32_t count = sizeof(g_tm_configs) / sizeof(g_tm_configs[0]);

    if (!g_acl_initialized) {
        return NULL;
    }

    /* O(n)查找，可优化为O(1)哈希表 */
    for (i = 0; i < count; i++) {
        if (g_tm_configs[i].function_id == tm_id) {
            return &g_tm_configs[i];
        }
    }

    return NULL;
}

/**
 * @brief 获取遥控命令失效的遥测列表（V2版本）
 */
int32_t ACL_GetInvalidatedTelemetry_V2(acl_tc_function_t tc_id,
                                     const acl_tm_function_t **tm_ids,
                                     uint32_t *count)
{
    uint32_t i;
    uint32_t map_count = sizeof(g_invalidation_map) / sizeof(g_invalidation_map[0]);

    if (!g_acl_initialized || tm_ids == NULL || count == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 查找失效映射 */
    for (i = 0; i < map_count; i++) {
        if (g_invalidation_map[i].tc_id == tc_id) {
            *tm_ids = g_invalidation_map[i].tm_ids;
            *count = g_invalidation_map[i].tm_count;
            return OSAL_SUCCESS;
        }
    }

    /* 未找到映射，返回空列表 */
    *tm_ids = NULL;
    *count = 0;
    return OSAL_SUCCESS;
}

/**
 * @brief 获取配置统计信息（V2版本）
 */
int32_t ACL_GetConfigStats_V2(uint32_t *tc_count, uint32_t *tm_count)
{
    if (!g_acl_initialized) {
        return OSAL_ERR_GENERIC;
    }

    if (tc_count) {
        *tc_count = sizeof(g_tc_configs) / sizeof(g_tc_configs[0]);
    }

    if (tm_count) {
        *tm_count = sizeof(g_tm_configs) / sizeof(g_tm_configs[0]);
    }

    return OSAL_SUCCESS;
}
