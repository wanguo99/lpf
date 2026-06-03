/**
 * @file test_aconfig_api.c
 * @brief ACONFIG API单元测试
 */

#include "test_framework.h"
#include "aconfig_api.h"
#include <aconfig/aconfig_tc.h>
#include <aconfig/aconfig_tm.h>
#include <string.h>

/* 测试用的配置表 */
static const aconfig_tc_config_t test_tc_table[] = {
    {
        .function_id = ACONFIG_TC_POWER_ON,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "power_mcu",
        .enabled = true,
        .user_context = NULL
    },
    {
        .function_id = ACONFIG_TC_POWER_OFF,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "power_mcu",
        .enabled = true,
        .user_context = NULL
    },
    {
        .function_id = ACONFIG_TC_MCU_RESET,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "power_mcu",
        .enabled = false,  /* 禁用 */
        .user_context = NULL
    }
};

static const aconfig_tm_config_t test_tm_table[] = {
    {
        .function_id = ACONFIG_TM_VOLTAGE_5V,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "power_mcu",
        .data_validity_ms = 1000,
        .background_update_period_ms = 500,
        .enabled = true,
        .user_context = NULL
    },
    {
        .function_id = ACONFIG_TM_CURRENT,
        .device_type = ACONFIG_DEVICE_MCU,
        .device_name = "power_mcu",
        .data_validity_ms = 1000,
        .background_update_period_ms = 500,
        .enabled = true,
        .user_context = NULL
    },
    {
        .function_id = ACONFIG_TM_CPU_TEMP,
        .device_type = ACONFIG_DEVICE_SENSOR,
        .device_name = "temp_sensor",
        .data_validity_ms = 2000,
        .background_update_period_ms = 1000,
        .enabled = false,  /* 禁用 */
        .user_context = NULL
    }
};

static uint32_t voltage_affected[] = {ACONFIG_TM_CURRENT};
static const aconfig_invalidation_map_t test_inv_map[] = {
    {
        .source_tm_id = ACONFIG_TM_VOLTAGE_5V,
        .affected_tm_ids = voltage_affected,
        .affected_count = 1
    }
};

static const aconfig_config_table_t test_config_table = {
    .name = "test_config",
    .tc_table = test_tc_table,
    .tc_count = sizeof(test_tc_table) / sizeof(test_tc_table[0]),
    .tm_table = test_tm_table,
    .tm_count = sizeof(test_tm_table) / sizeof(test_tm_table[0]),
    .inv_map = test_inv_map,
    .inv_count = sizeof(test_inv_map) / sizeof(test_inv_map[0])
};

/* 测试夹具 */
static void aconfig_test_setup(void)
{
    ACONFIG_Init();
    ACONFIG_RegisterTable(&test_config_table);
}

static void aconfig_test_teardown(void)
{
    /* ACONFIG 没有清理函数，下次 Init 会重置 */
}

/*===========================================================================
 * 初始化和注册测试
 *===========================================================================*/

TEST_CASE(test_aconfig_init_success)
{
    int32_t ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_aconfig_register_table_success)
{
    ACONFIG_Init();
    int32_t ret = ACONFIG_RegisterTable(&test_config_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_aconfig_register_table_null)
{
    ACONFIG_Init();
    int32_t ret = ACONFIG_RegisterTable(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 遥控配置查询测试
 *===========================================================================*/

TEST_CASE(test_aconfig_get_tc_config_valid)
{
    const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(ACONFIG_TC_POWER_ON);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(ACONFIG_TC_POWER_ON, cfg->function_id);
    TEST_ASSERT_EQUAL(ACONFIG_DEVICE_MCU, cfg->device_type);
    TEST_ASSERT_TRUE(cfg->enabled);
}

TEST_CASE(test_aconfig_get_tc_config_invalid)
{
    const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(9999);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

TEST_CASE(test_aconfig_is_tc_enabled_true)
{
    bool enabled = ACONFIG_IsTcEnabled(ACONFIG_TC_POWER_ON);
    TEST_ASSERT_TRUE(enabled);
}

TEST_CASE(test_aconfig_is_tc_enabled_false)
{
    bool enabled = ACONFIG_IsTcEnabled(ACONFIG_TC_MCU_RESET);
    TEST_ASSERT_FALSE(enabled);
}

TEST_CASE(test_aconfig_is_tc_enabled_invalid)
{
    bool enabled = ACONFIG_IsTcEnabled(9999);
    TEST_ASSERT_FALSE(enabled);
}

/*===========================================================================
 * 遥测配置查询测试
 *===========================================================================*/

TEST_CASE(test_aconfig_get_tm_config_valid)
{
    const aconfig_tm_config_t *cfg = ACONFIG_GetTmConfig(ACONFIG_TM_VOLTAGE_5V);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(ACONFIG_TM_VOLTAGE_5V, cfg->function_id);
    TEST_ASSERT_EQUAL(ACONFIG_DEVICE_MCU, cfg->device_type);
    TEST_ASSERT_EQUAL(1000, cfg->data_validity_ms);
    TEST_ASSERT_EQUAL(500, cfg->background_update_period_ms);
    TEST_ASSERT_TRUE(cfg->enabled);
}

TEST_CASE(test_aconfig_get_tm_config_invalid)
{
    const aconfig_tm_config_t *cfg = ACONFIG_GetTmConfig(9999);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

TEST_CASE(test_aconfig_is_tm_enabled_true)
{
    bool enabled = ACONFIG_IsTmEnabled(ACONFIG_TM_VOLTAGE_5V);
    TEST_ASSERT_TRUE(enabled);
}

TEST_CASE(test_aconfig_is_tm_enabled_false)
{
    bool enabled = ACONFIG_IsTmEnabled(ACONFIG_TM_CPU_TEMP);
    TEST_ASSERT_FALSE(enabled);
}

TEST_CASE(test_aconfig_is_tm_enabled_invalid)
{
    bool enabled = ACONFIG_IsTmEnabled(9999);
    TEST_ASSERT_FALSE(enabled);
}

/*===========================================================================
 * 失效映射测试
 *===========================================================================*/

TEST_CASE(test_aconfig_get_invalidation_map_valid)
{
    uint32_t affected_ids[10];
    uint32_t actual_count = 0;

    int32_t ret = ACONFIG_GetInvalidationMap(ACONFIG_TM_VOLTAGE_5V,
                                             affected_ids,
                                             10,
                                             &actual_count);

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(1, actual_count);
    TEST_ASSERT_EQUAL(ACONFIG_TM_CURRENT, affected_ids[0]);
}

TEST_CASE(test_aconfig_get_invalidation_map_invalid)
{
    uint32_t affected_ids[10];
    uint32_t actual_count = 0;

    int32_t ret = ACONFIG_GetInvalidationMap(9999,
                                             affected_ids,
                                             10,
                                             &actual_count);

    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
    TEST_ASSERT_EQUAL(0, actual_count);
}

TEST_CASE(test_aconfig_get_invalidation_map_null_pointer)
{
    uint32_t actual_count = 0;

    int32_t ret = ACONFIG_GetInvalidationMap(ACONFIG_TM_VOLTAGE_5V,
                                             NULL,
                                             10,
                                             &actual_count);

    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 统计信息测试
 *===========================================================================*/

TEST_CASE(test_aconfig_get_statistics)
{
    aconfig_statistics_t stats;
    int32_t ret = ACONFIG_GetStatistics(&stats);

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, stats.tc_enabled_count);   /* POWER_ON, POWER_OFF */
    TEST_ASSERT_EQUAL(1, stats.tc_disabled_count);  /* RESET */
    TEST_ASSERT_EQUAL(2, stats.tm_enabled_count);   /* VOLTAGE, CURRENT */
    TEST_ASSERT_EQUAL(1, stats.tm_disabled_count);  /* TEMPERATURE */
    TEST_ASSERT_EQUAL(1, stats.invalidation_map_count);
}

TEST_CASE(test_aconfig_get_statistics_null)
{
    int32_t ret = ACONFIG_GetStatistics(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 打印配置测试（仅验证不崩溃）
 *===========================================================================*/

TEST_CASE(test_aconfig_print_config)
{
    /* 仅验证函数不崩溃 */
    ACONFIG_PrintConfig();
    TEST_ASSERT_TRUE(true);
}

/* 注册测试套件 */
static const test_case_t aconfig_api_cases[] = {
    TEST_CASE_REF(test_aconfig_init_success)
    TEST_CASE_REF(test_aconfig_register_table_success)
    TEST_CASE_REF(test_aconfig_register_table_null)
    TEST_CASE_REF(test_aconfig_get_tc_config_valid)
    TEST_CASE_REF(test_aconfig_get_tc_config_invalid)
    TEST_CASE_REF(test_aconfig_is_tc_enabled_true)
    TEST_CASE_REF(test_aconfig_is_tc_enabled_false)
    TEST_CASE_REF(test_aconfig_is_tc_enabled_invalid)
    TEST_CASE_REF(test_aconfig_get_tm_config_valid)
    TEST_CASE_REF(test_aconfig_get_tm_config_invalid)
    TEST_CASE_REF(test_aconfig_is_tm_enabled_true)
    TEST_CASE_REF(test_aconfig_is_tm_enabled_false)
    TEST_CASE_REF(test_aconfig_is_tm_enabled_invalid)
    TEST_CASE_REF(test_aconfig_get_invalidation_map_valid)
    TEST_CASE_REF(test_aconfig_get_invalidation_map_invalid)
    TEST_CASE_REF(test_aconfig_get_invalidation_map_null_pointer)
    TEST_CASE_REF(test_aconfig_get_statistics)
    TEST_CASE_REF(test_aconfig_get_statistics_null)
    TEST_CASE_REF(test_aconfig_print_config)
};

static const test_suite_t aconfig_api_suite = {
    .suite_name = "aconfig_api",
    .module_name = "aconfig_api",
    .layer_name = "ACONFIG",
    .cases = aconfig_api_cases,
    .case_count = sizeof(aconfig_api_cases) / sizeof(test_case_t),
    .suite_setup = aconfig_test_setup,
    .suite_teardown = aconfig_test_teardown
};

__attribute__((constructor))
static void register_aconfig_api(void) {
    libutest_register_suite(&aconfig_api_suite);
}
