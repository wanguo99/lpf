/**
 * @file test_aconfig_api.c
 * @brief AConfig API 单元测试（只测试核心功能，不依赖具体产品）
 */

#include <test/test_framework.h>
#include "aconfig.h"
#include "test_aconfig_definitions.h"

/* ========================================================================
 * 测试数据定义（自包含，不依赖 CCM）
 * ======================================================================== */

/* 测试用失效映射 */
static const uint32_t test_tc_power_off_affects[] = {
    TEST_TM_VOLTAGE,
    TEST_TM_STATUS
};

/* 测试用 TC 配置（稀疏数组） */
static const aconfig_tc_entry_t test_tc_entries[] = {
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
            .invalidated_tm_count = sizeof(test_tc_power_off_affects) / sizeof(uint32_t),
            .enabled = true,
            .user_context = NULL
        }
    },
    {
        .function_id = TEST_TC_RESET,
        .config = {
            .function_id = TEST_TC_RESET,
            .device = {.type = PCONFIG_DEVICE_MCU, .index = 0},
            .invalidated_tm_ids = NULL,
            .invalidated_tm_count = 0,
            .enabled = false,  // 测试禁用状态
            .user_context = (void *)0x12345678  // 测试用户上下文
        }
    }
};

/* 测试用 TM 配置（稀疏数组） */
static const aconfig_tm_entry_t test_tm_entries[] = {
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
            .enabled = false,  // 测试禁用状态
            .user_context = (void *)0xABCDEF00
        }
    }
};

/* 测试配置表 */
static const aconfig_config_table_t test_config_table = {
    .name = "TEST_ACONFIG",
    .hwid_count = 0,
    .hwid_list = NULL,
    .tc_entries = test_tc_entries,
    .tc_count = sizeof(test_tc_entries) / sizeof(aconfig_tc_entry_t),
    .tm_entries = test_tm_entries,
    .tm_count = sizeof(test_tm_entries) / sizeof(aconfig_tm_entry_t)
};

/* ========================================================================
 * 测试用例
 * ======================================================================== */

/**
 * @brief 测试 AConfig 初始化和注册
 */
static void test_aconfig_init_register(void)
{
    int32_t ret;

    /* 测试初始化 */
    ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试注册配置表 */
    ret = ACONFIG_RegisterTable(&test_config_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试重复注册（应该覆盖，但返回成功） */
    ret = ACONFIG_RegisterTable(&test_config_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试 TC 配置查询
 */
static void test_aconfig_tc_query(void)
{
    const aconfig_tc_config_t *config;

    /* 查询有效的 TC */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(TEST_TC_POWER_ON, config->function_id);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_BMC, config->device.type);
    TEST_ASSERT_EQUAL(0, config->device.index);
    TEST_ASSERT_TRUE(config->enabled);

    /* 查询带失效映射的 TC */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_OFF);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(TEST_TC_POWER_OFF, config->function_id);
    TEST_ASSERT_NOT_NULL(config->invalidated_tm_ids);
    TEST_ASSERT_EQUAL(2, config->invalidated_tm_count);
    TEST_ASSERT_EQUAL(TEST_TM_VOLTAGE, config->invalidated_tm_ids[0]);
    TEST_ASSERT_EQUAL(TEST_TM_STATUS, config->invalidated_tm_ids[1]);

    /* 查询禁用的 TC */
    config = ACONFIG_GetTcConfig(TEST_TC_RESET);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_FALSE(config->enabled);
    TEST_ASSERT_EQUAL((void *)0x12345678, config->user_context);

    /* 查询不存在的 TC */
    config = ACONFIG_GetTcConfig(999);
    TEST_ASSERT_NULL(config);
}

/**
 * @brief 测试 TM 配置查询
 */
static void test_aconfig_tm_query(void)
{
    const aconfig_tm_config_t *config;

    /* 查询有效的 TM */
    config = ACONFIG_GetTmConfig(TEST_TM_VOLTAGE);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(TEST_TM_VOLTAGE, config->function_id);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_BMC, config->device.type);
    TEST_ASSERT_EQUAL(1000, config->poll_period_ms);
    TEST_ASSERT_EQUAL(2000, config->validity_period_ms);
    TEST_ASSERT_TRUE(config->enabled);

    /* 查询不同设备类型的 TM */
    config = ACONFIG_GetTmConfig(TEST_TM_TEMPERATURE);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_SENSOR, config->device.type);

    /* 查询禁用的 TM */
    config = ACONFIG_GetTmConfig(TEST_TM_STATUS);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_FALSE(config->enabled);
    TEST_ASSERT_EQUAL((void *)0xABCDEF00, config->user_context);

    /* 查询不存在的 TM */
    config = ACONFIG_GetTmConfig(999);
    TEST_ASSERT_NULL(config);
}

/**
 * @brief 测试 TC/TM 启用状态检查
 */
static void test_aconfig_enabled_check(void)
{
    /* 测试启用的 TC */
    TEST_ASSERT_TRUE(ACONFIG_IsTcEnabled(TEST_TC_POWER_ON));
    TEST_ASSERT_TRUE(ACONFIG_IsTcEnabled(TEST_TC_POWER_OFF));

    /* 测试禁用的 TC */
    TEST_ASSERT_FALSE(ACONFIG_IsTcEnabled(TEST_TC_RESET));

    /* 测试不存在的 TC */
    TEST_ASSERT_FALSE(ACONFIG_IsTcEnabled(999));

    /* 测试启用的 TM */
    TEST_ASSERT_TRUE(ACONFIG_IsTmEnabled(TEST_TM_VOLTAGE));
    TEST_ASSERT_TRUE(ACONFIG_IsTmEnabled(TEST_TM_TEMPERATURE));

    /* 测试禁用的 TM */
    TEST_ASSERT_FALSE(ACONFIG_IsTmEnabled(TEST_TM_STATUS));

    /* 测试不存在的 TM */
    TEST_ASSERT_FALSE(ACONFIG_IsTmEnabled(999));
}

/**
 * @brief 测试设备引用（索引方式）
 */
static void test_aconfig_device_reference(void)
{
    const aconfig_tc_config_t *tc_config;
    const aconfig_tm_config_t *tm_config;

    /* 验证设备索引引用 */
    tc_config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(tc_config);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_BMC, tc_config->device.type);
    TEST_ASSERT_EQUAL(0, tc_config->device.index);

    tc_config = ACONFIG_GetTcConfig(TEST_TC_RESET);
    TEST_ASSERT_NOT_NULL(tc_config);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_MCU, tc_config->device.type);
    TEST_ASSERT_EQUAL(0, tc_config->device.index);

    tm_config = ACONFIG_GetTmConfig(TEST_TM_TEMPERATURE);
    TEST_ASSERT_NOT_NULL(tm_config);
    TEST_ASSERT_EQUAL(PCONFIG_DEVICE_SENSOR, tm_config->device.type);
    TEST_ASSERT_EQUAL(0, tm_config->device.index);
}

/**
 * @brief 测试失效映射（内嵌方式）
 */
static void test_aconfig_invalidation_embedded(void)
{
    const aconfig_tc_config_t *config;

    /* 有失效映射的 TC */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_OFF);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->invalidated_tm_ids);
    TEST_ASSERT_EQUAL(2, config->invalidated_tm_count);

    /* 验证失效的 TM ID */
    TEST_ASSERT_EQUAL(TEST_TM_VOLTAGE, config->invalidated_tm_ids[0]);
    TEST_ASSERT_EQUAL(TEST_TM_STATUS, config->invalidated_tm_ids[1]);

    /* 无失效映射的 TC */
    config = ACONFIG_GetTcConfig(TEST_TC_POWER_ON);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NULL(config->invalidated_tm_ids);
    TEST_ASSERT_EQUAL(0, config->invalidated_tm_count);
}

/* ========================================================================
 * 测试套件定义
 * ======================================================================== */

static test_case_t test_cases[] = {
    {"test_aconfig_init_register", test_aconfig_init_register},
    {"test_aconfig_tc_query", test_aconfig_tc_query},
    {"test_aconfig_tm_query", test_aconfig_tm_query},
    {"test_aconfig_enabled_check", test_aconfig_enabled_check},
    {"test_aconfig_device_reference", test_aconfig_device_reference},
    {"test_aconfig_invalidation_embedded", test_aconfig_invalidation_embedded},
};

static test_suite_t test_suite = {
    .module_name = "aconfig_api",
    .layer_name = "ACONFIG",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 1000,
        .description = "AConfig API core functionality tests (product-independent)"
    }
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_aconfig_api_tests(void)
{
    libutest_register_suite(&test_suite);
}

