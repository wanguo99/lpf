/**
 * @file test_aconfig_extended.c
 * @brief ACONFIG 扩展 API 单元测试
 * @note 测试 ACONFIG_GetInvalidationMap, ACONFIG_GetStatistics 等扩展功能
 */

#include <test_framework/test_framework.h>
#include "aconfig.h"
#include "test_aconfig_definitions.h"

/* ========================================================================
 * 测试数据定义
 * ======================================================================== */

/* 测试用失效映射（多个 TC 有不同的失效映射） */
static const uint32_t test_tc_power_off_affects[] = {
    TEST_TM_VOLTAGE,
    TEST_TM_STATUS
};

static const uint32_t test_tc_reset_affects[] = {
    TEST_TM_TEMPERATURE,
    TEST_TM_STATUS,
    TEST_TM_VOLTAGE
};

/* 空失效映射的 TC */
static const aconfig_tc_entry_t test_tc_entries_extended[] = {
    {
        .function_id = TEST_TC_POWER_ON,
        .config = {
            .function_id = TEST_TC_POWER_ON,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
            .invalidated_tm_ids = NULL,
            .invalidated_tm_count = 0,
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = TEST_TC_POWER_OFF,
        .config = {
            .function_id = TEST_TC_POWER_OFF,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
            .invalidated_tm_ids = test_tc_power_off_affects,
            .invalidated_tm_count = 2,
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = TEST_TC_RESET,
        .config = {
            .function_id = TEST_TC_RESET,
            .device = {.type = PCONFIG_DEVICE_MCU, .index = 0},
            .invalidated_tm_ids = test_tc_reset_affects,
            .invalidated_tm_count = 3,
            .enabled = false,  // 禁用状态
            .user_context = NULL
        }
    }
};

static const aconfig_tm_entry_t test_tm_entries_extended[] = {
    {
        .function_id = TEST_TM_VOLTAGE,
        .config = {
            .function_id = TEST_TM_VOLTAGE,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
            .poll_period_ms = 1000,
            .validity_period_ms = 2000,
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = TEST_TM_TEMPERATURE,
        .config = {
            .function_id = TEST_TM_TEMPERATURE,
            .device = {.type = PCONFIG_DEVICE_SENSOR, .index = 0},
            .poll_period_ms = 500,
            .validity_period_ms = 1000,
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = TEST_TM_STATUS,
        .config = {
            .function_id = TEST_TM_STATUS,
            .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
            .poll_period_ms = 100,
            .validity_period_ms = 200,
            .enabled = false,  // 禁用状态
            .user_context = NULL
        }
    }
};

static const aconfig_config_table_t test_config_table_extended = {
    .name = "TEST_ACONFIG_EXTENDED",
    .hwid_count = 0,
    .hwid_list = NULL,
    .tc_entries = test_tc_entries_extended,
    .tc_count = 3,
    .tm_entries = test_tm_entries_extended,
    .tm_count = 3
};

/* ========================================================================
 * 测试用例
 * ======================================================================== */

/**
 * @brief 测试 ACONFIG_GetInvalidationMap - 正常情况
 */
static void test_aconfig_get_invalidation_map_normal(void)
{
    int32_t ret;
    uint32_t affected_ids[10];
    uint32_t actual_count = 0;

    /* 初始化并注册配置 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试有失效映射的 TC (TEST_TC_POWER_OFF -> 2 个 TM) */
    ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_OFF, affected_ids, 10, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, actual_count);
    TEST_ASSERT_EQUAL(TEST_TM_VOLTAGE, affected_ids[0]);
    TEST_ASSERT_EQUAL(TEST_TM_STATUS, affected_ids[1]);

    /* 测试另一个有失效映射的 TC (TEST_TC_RESET -> 3 个 TM) */
    actual_count = 0;
    ret = ACONFIG_GetInvalidationMap(TEST_TC_RESET, affected_ids, 10, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(3, actual_count);
    TEST_ASSERT_EQUAL(TEST_TM_TEMPERATURE, affected_ids[0]);
    TEST_ASSERT_EQUAL(TEST_TM_STATUS, affected_ids[1]);
    TEST_ASSERT_EQUAL(TEST_TM_VOLTAGE, affected_ids[2]);
}

/**
 * @brief 测试 ACONFIG_GetInvalidationMap - 空失效映射
 */
static void test_aconfig_get_invalidation_map_empty(void)
{
    int32_t ret;
    uint32_t affected_ids[10];
    uint32_t actual_count = 0;

    /* 测试无失效映射的 TC (TEST_TC_POWER_ON) */
    ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_ON, affected_ids, 10, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, actual_count);
}

/**
 * @brief 测试 ACONFIG_GetInvalidationMap - 缓冲区不足
 */
static void test_aconfig_get_invalidation_map_buffer_small(void)
{
    int32_t ret;
    uint32_t affected_ids[2];
    uint32_t actual_count = 0;

    /* 缓冲区只有 2 个元素，但失效映射有 3 个 */
    ret = ACONFIG_GetInvalidationMap(TEST_TC_RESET, affected_ids, 2, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(3, actual_count);  // actual_count 仍然是完整数量
    /* 只复制了 2 个 */
    TEST_ASSERT_EQUAL(TEST_TM_TEMPERATURE, affected_ids[0]);
    TEST_ASSERT_EQUAL(TEST_TM_STATUS, affected_ids[1]);
}

/**
 * @brief 测试 ACONFIG_GetInvalidationMap - 无效参数
 */
static void test_aconfig_get_invalidation_map_invalid_params(void)
{
    int32_t ret;
    uint32_t affected_ids[10];
    uint32_t actual_count = 0;

    /* NULL affected_ids 指针 */
    ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_OFF, NULL, 10, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* NULL actual_count 指针 */
    ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_OFF, affected_ids, 10, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* 不存在的 TC ID */
    ret = ACONFIG_GetInvalidationMap(999, affected_ids, 10, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_ERR_NAME_NOT_FOUND, ret);
}

/**
 * @brief 测试 ACONFIG_GetStatistics - 正常情况
 */
static void test_aconfig_get_statistics_normal(void)
{
    int32_t ret;
    aconfig_statistics_t stats;

    /* 初始化并注册配置 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取统计信息 */
    ret = ACONFIG_GetStatistics(&stats);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证统计数据：
     * TC: 2 enabled (POWER_ON, POWER_OFF), 1 disabled (RESET)
     * TM: 2 enabled (VOLTAGE, TEMPERATURE), 1 disabled (STATUS)
     * Invalidation maps: 2 (POWER_OFF, RESET)
     */
    TEST_ASSERT_EQUAL(2, stats.tc_enabled_count);
    TEST_ASSERT_EQUAL(1, stats.tc_disabled_count);
    TEST_ASSERT_EQUAL(2, stats.tm_enabled_count);
    TEST_ASSERT_EQUAL(1, stats.tm_disabled_count);
    TEST_ASSERT_EQUAL(2, stats.total_invalidation_maps);
}

/**
 * @brief 测试 ACONFIG_GetStatistics - 未注册配置表
 */
static void test_aconfig_get_statistics_no_table(void)
{
    int32_t ret;
    aconfig_statistics_t stats;

    /* 初始化但不注册配置表 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取统计信息 - 应该返回全 0 */
    ret = ACONFIG_GetStatistics(&stats);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, stats.tc_enabled_count);
    TEST_ASSERT_EQUAL(0, stats.tc_disabled_count);
    TEST_ASSERT_EQUAL(0, stats.tm_enabled_count);
    TEST_ASSERT_EQUAL(0, stats.tm_disabled_count);
    TEST_ASSERT_EQUAL(0, stats.total_invalidation_maps);
}

/**
 * @brief 测试 ACONFIG_GetStatistics - 无效参数
 */
static void test_aconfig_get_statistics_invalid_params(void)
{
    int32_t ret;

    /* NULL 指针 */
    ret = ACONFIG_GetStatistics(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/**
 * @brief 测试 ACONFIG_PrintConfig - 基本功能
 */
static void test_aconfig_print_config(void)
{
    int32_t ret;

    /* 初始化并注册配置 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 调用 PrintConfig - 仅验证不崩溃 */
    ACONFIG_PrintConfig();

    /* 测试未注册配置表时的情况 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ACONFIG_PrintConfig();
}

/**
 * @brief 测试 ACONFIG_RegisterTable - 错误处理
 */
static void test_aconfig_register_table_error_handling(void)
{
    int32_t ret;

    /* 初始化 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL 表指针 */
    ret = ACONFIG_RegisterTable(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/**
 * @brief 测试表覆盖行为
 */
static void test_aconfig_table_overwrite(void)
{
    int32_t ret;
    const aconfig_tc_config_t *config;

    /* 第一次注册 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证可以查询 */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(config);

    /* 再次注册相同的表（应该覆盖） */
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证仍然可以查询 */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(config);
}

/**
 * @brief 测试空表边界情况
 */
static void test_aconfig_empty_table_edge_cases(void)
{
    int32_t ret;
    const aconfig_tc_config_t *tc_config;
    const aconfig_tm_config_t *tm_config;
    aconfig_statistics_t stats;

    /* 创建空表 */
    static const aconfig_config_table_t empty_table = {
        .name = "EMPTY_TABLE",
        .hwid_count = 0,
        .hwid_list = NULL,
        .tc_entries = NULL,
        .tc_count = 0,
        .tm_entries = NULL,
        .tm_count = 0
    };

    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&empty_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 查询应该返回 NULL */
    tc_config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NULL(tc_config);

    tm_config = ACONFIG_GetTmConfig(TEST_TM_VOLTAGE);
    TEST_ASSERT_NULL(tm_config);

    /* 统计应该全为 0 */
    ret = ACONFIG_GetStatistics(&stats);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, stats.tc_enabled_count);
    TEST_ASSERT_EQUAL(0, stats.tc_disabled_count);
    TEST_ASSERT_EQUAL(0, stats.tm_enabled_count);
    TEST_ASSERT_EQUAL(0, stats.tm_disabled_count);
}

/**
 * @brief 测试用户上下文保留
 */
static void test_aconfig_user_context_preserved(void)
{
    int32_t ret;
    const aconfig_tc_config_t *tc_config;
    const aconfig_tm_config_t *tm_config;

    /* 创建带用户上下文的配置 */
    static const uint32_t dummy_data_tc = 0x12345678;
    static const uint32_t dummy_data_tm = 0xABCDEF00;

    static const aconfig_tc_entry_t tc_with_context[] = {
        {
            .function_id = TEST_TC_POWER_ON,
            .config = {
                .function_id = TEST_TC_POWER_ON,
                .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
                .invalidated_tm_ids = NULL,
                .invalidated_tm_count = 0,
                .enabled = true,
                .user_context = (void *)&dummy_data_tc
            }
        }
    };

    static const aconfig_tm_entry_t tm_with_context[] = {
        {
            .function_id = TEST_TM_VOLTAGE,
            .config = {
                .function_id = TEST_TM_VOLTAGE,
                .device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
                .poll_period_ms = 1000,
                .validity_period_ms = 2000,
                .enabled = true,
                .user_context = (void *)&dummy_data_tm
            }
        }
    };

    static const aconfig_config_table_t context_table = {
        .name = "CONTEXT_TEST",
        .hwid_count = 0,
        .hwid_list = NULL,
        .tc_entries = tc_with_context,
        .tc_count = 1,
        .tm_entries = tm_with_context,
        .tm_count = 1
    };

    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&context_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证用户上下文正确保留 */
    tc_config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(tc_config);
    TEST_ASSERT_EQUAL(&dummy_data_tc, tc_config->user_context);
    TEST_ASSERT_EQUAL(0x12345678, *(uint32_t *)tc_config->user_context);

    tm_config = ACONFIG_GetTmConfig(TEST_TM_VOLTAGE);
    TEST_ASSERT_NOT_NULL(tm_config);
    TEST_ASSERT_EQUAL(&dummy_data_tm, tm_config->user_context);
    TEST_ASSERT_EQUAL(0xABCDEF00, *(uint32_t *)tm_config->user_context);
}

/**
 * @brief 测试大型稀疏数组性能基线
 */
static void test_aconfig_large_sparse_array_baseline(void)
{
    int32_t ret;
    const aconfig_tc_config_t *config;

    /* 初始化并注册配置 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    ret = ACONFIG_RegisterTable(&test_config_table_extended);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 执行多次查询以建立性能基线 */
    for (int i = 0; i < 100; i++) {
        config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
        TEST_ASSERT_NOT_NULL(config);

        config = ACONFIG_GetTcConfig(TEST_TC_POWER_OFF);
        TEST_ASSERT_NOT_NULL(config);

        config = ACONFIG_GetTcConfig(TEST_TC_RESET);
        TEST_ASSERT_NOT_NULL(config);

        /* 测试不存在的 ID */
        config = ACONFIG_GetTcConfig(999);
        TEST_ASSERT_NULL(config);
    }
}

/* ========================================================================
 * 测试套件定义
 * ======================================================================== */

static test_case_t test_cases[] = {
    {"test_aconfig_get_invalidation_map_normal", test_aconfig_get_invalidation_map_normal},
    {"test_aconfig_get_invalidation_map_empty", test_aconfig_get_invalidation_map_empty},
    {"test_aconfig_get_invalidation_map_buffer_small", test_aconfig_get_invalidation_map_buffer_small},
    {"test_aconfig_get_invalidation_map_invalid_params", test_aconfig_get_invalidation_map_invalid_params},
    {"test_aconfig_get_statistics_normal", test_aconfig_get_statistics_normal},
    {"test_aconfig_get_statistics_no_table", test_aconfig_get_statistics_no_table},
    {"test_aconfig_get_statistics_invalid_params", test_aconfig_get_statistics_invalid_params},
    {"test_aconfig_print_config", test_aconfig_print_config},
    {"test_aconfig_register_table_error_handling", test_aconfig_register_table_error_handling},
    {"test_aconfig_table_overwrite", test_aconfig_table_overwrite},
    {"test_aconfig_empty_table_edge_cases", test_aconfig_empty_table_edge_cases},
    {"test_aconfig_user_context_preserved", test_aconfig_user_context_preserved},
    {"test_aconfig_large_sparse_array_baseline", test_aconfig_large_sparse_array_baseline},
};

static test_suite_t test_suite = {
    .module_name = "aconfig_extended",
    .layer_name = "ACONFIG",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 2000,
        .description = "ACONFIG extended API tests (GetInvalidationMap, GetStatistics, error handling)"
    }
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_aconfig_extended_tests(void)
{
    libutest_register_suite(&test_suite);
}
