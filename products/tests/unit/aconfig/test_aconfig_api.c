/**
 * @file test_aconfig_api.c
 * @brief ACONFIG API单元测试
 */

#include "test_framework.h"
#include "aconfig.h"
#include "aconfig_tc.h"
#include "aconfig_tm.h"

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
    .tc_count = OSAL_sizeof(test_tc_table) / OSAL_sizeof(test_tc_table[0]),
    .tm_table = test_tm_table,
    .tm_count = OSAL_sizeof(test_tm_table) / OSAL_sizeof(test_tm_table[0]),
    .inv_map = test_inv_map,
    .inv_count = OSAL_sizeof(test_inv_map) / OSAL_sizeof(test_inv_map[0])
};

/*===========================================================================
 * 初始化和注册测试
 *===========================================================================*/

static void test_aconfig_init_success(void)
{
    int32_t ret = ACONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_aconfig_register_table_success(void)
{
    ACONFIG_Init();
    int32_t ret = ACONFIG_RegisterTable(&test_config_table);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_aconfig_register_table_null(void)
{
    ACONFIG_Init();
    int32_t ret = ACONFIG_RegisterTable(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 遥控配置查询测试
 *===========================================================================*/

static void test_aconfig_get_tc_config_valid(void)
{
    const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(ACONFIG_TC_POWER_ON);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(ACONFIG_TC_POWER_ON, cfg->function_id);
    TEST_ASSERT_EQUAL(ACONFIG_DEVICE_MCU, cfg->device_type);
    TEST_ASSERT_TRUE(cfg->enabled);
}

static void test_aconfig_get_tc_config_invalid(void)
{
    const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(9999);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

static void test_aconfig_is_tc_enabled_true(void)
{
    bool enabled = ACONFIG_IsTcEnabled(ACONFIG_TC_POWER_ON);
    TEST_ASSERT_TRUE(enabled);
}

static void test_aconfig_is_tc_enabled_false(void)
{
    bool enabled = ACONFIG_IsTcEnabled(ACONFIG_TC_MCU_RESET);
    TEST_ASSERT_FALSE(enabled);
}

static void test_aconfig_is_tc_enabled_invalid(void)
{
    bool enabled = ACONFIG_IsTcEnabled(9999);
    TEST_ASSERT_FALSE(enabled);
}

/*===========================================================================
 * 遥测配置查询测试
 *===========================================================================*/

static void test_aconfig_get_tm_config_valid(void)
{
    const aconfig_tm_config_t *cfg = ACONFIG_GetTmConfig(ACONFIG_TM_VOLTAGE_5V);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(ACONFIG_TM_VOLTAGE_5V, cfg->function_id);
    TEST_ASSERT_EQUAL(ACONFIG_DEVICE_MCU, cfg->device_type);
    TEST_ASSERT_EQUAL(1000, cfg->data_validity_ms);
    TEST_ASSERT_EQUAL(500, cfg->background_update_period_ms);
    TEST_ASSERT_TRUE(cfg->enabled);
}

static void test_aconfig_get_tm_config_invalid(void)
{
    const aconfig_tm_config_t *cfg = ACONFIG_GetTmConfig(9999);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

static void test_aconfig_is_tm_enabled_true(void)
{
    bool enabled = ACONFIG_IsTmEnabled(ACONFIG_TM_VOLTAGE_5V);
    TEST_ASSERT_TRUE(enabled);
}

static void test_aconfig_is_tm_enabled_false(void)
{
    bool enabled = ACONFIG_IsTmEnabled(ACONFIG_TM_CPU_TEMP);
    TEST_ASSERT_FALSE(enabled);
}

static void test_aconfig_is_tm_enabled_invalid(void)
{
    bool enabled = ACONFIG_IsTmEnabled(9999);
    TEST_ASSERT_FALSE(enabled);
}

/*===========================================================================
 * 失效映射测试
 *===========================================================================*/

static void test_aconfig_get_invalidation_map_valid(void)
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

static void test_aconfig_get_invalidation_map_invalid(void)
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

static void test_aconfig_get_invalidation_map_null_pointer(void)
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

static void test_aconfig_get_statistics(void)
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

static void test_aconfig_get_statistics_null(void)
{
    int32_t ret = ACONFIG_GetStatistics(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 打印配置测试（仅验证不崩溃）
 *===========================================================================*/

static void test_aconfig_print_config(void)
{
    /* 仅验证函数不崩溃 */
    ACONFIG_PrintConfig();
    TEST_ASSERT_TRUE(true);
}

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_aconfig_init_success",
		.func = test_aconfig_init_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_register_table_success",
		.func = test_aconfig_register_table_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_register_table_null",
		.func = test_aconfig_register_table_null,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_tc_config_valid",
		.func = test_aconfig_get_tc_config_valid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_tc_config_invalid",
		.func = test_aconfig_get_tc_config_invalid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tc_enabled_true",
		.func = test_aconfig_is_tc_enabled_true,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tc_enabled_false",
		.func = test_aconfig_is_tc_enabled_false,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tc_enabled_invalid",
		.func = test_aconfig_is_tc_enabled_invalid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_tm_config_valid",
		.func = test_aconfig_get_tm_config_valid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_tm_config_invalid",
		.func = test_aconfig_get_tm_config_invalid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tm_enabled_true",
		.func = test_aconfig_is_tm_enabled_true,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tm_enabled_false",
		.func = test_aconfig_is_tm_enabled_false,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_is_tm_enabled_invalid",
		.func = test_aconfig_is_tm_enabled_invalid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_invalidation_map_valid",
		.func = test_aconfig_get_invalidation_map_valid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_invalidation_map_invalid",
		.func = test_aconfig_get_invalidation_map_invalid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_invalidation_map_null_pointer",
		.func = test_aconfig_get_invalidation_map_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_statistics",
		.func = test_aconfig_get_statistics,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_get_statistics_null",
		.func = test_aconfig_get_statistics_null,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_aconfig_print_config",
		.func = test_aconfig_print_config,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "aconfig_api",
	.module_name = "aconfig_api",
	.layer_name = "ACONFIG",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "ACONFIG aconfig_api tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_aconfig_api_tests(void)
{
	libutest_register_suite(&test_suite);
}
