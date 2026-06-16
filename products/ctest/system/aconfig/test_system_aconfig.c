/**
 * @file test_system_aconfig.c
 * @brief ACONFIG层系统集成测试
 * @note 测试真实系统场景下ACONFIG与其他模块的集成和交互
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "aconfig.h"
#include "osal.h"

/* ========================================================================
 * 测试数据定义
 * ======================================================================== */

/* 测试用功能ID枚举 */
typedef enum {
	TEST_TC_POWER_ON = 100,
	TEST_TC_POWER_OFF = 101,
	TEST_TC_RESET = 102,
	TEST_TC_SET_MODE = 103,
	TEST_TC_CALIBRATE = 104,
} test_tc_id_t;

typedef enum {
	TEST_TM_VOLTAGE = 200,
	TEST_TM_TEMPERATURE = 201,
	TEST_TM_STATUS = 202,
	TEST_TM_MODE = 203,
	TEST_TM_HEALTH = 204,
} test_tm_id_t;

/* 模拟TC执行结果 */
typedef struct {
	uint32_t tc_id;
	int32_t status;
	uint32_t timestamp;
} tc_execution_result_t;

/* 模拟TM数据 */
typedef struct {
	uint32_t tm_id;
	uint32_t value;
	uint32_t timestamp;
	bool valid;
} tm_data_t;

/* 系统上下文 */
typedef struct {
	aconfig_config_table_t *config_table;
	tc_execution_result_t tc_results[10];
	tm_data_t tm_cache[10];
	uint32_t tc_count;
	uint32_t tm_count;
	osal_mutex_t mutex;
} system_context_t;

static system_context_t g_sys_ctx;

/* ========================================================================
 * 辅助函数
 * ======================================================================== */

/**
 * @brief 模拟TC执行
 */
static int32_t execute_telecommand(uint32_t tc_id, system_context_t *ctx)
{
	const aconfig_tc_config_t *tc_config;
	uint32_t i;
	int32_t ret;

	/* 查询TC配置 */
	tc_config = ACONFIG_GetTcConfig(tc_id);
	if (!tc_config) {
		OSAL_printf("[ ERROR    ] TC %u not found in config\n", tc_id);
		return OSAL_ERR_NOT_FOUND;
	}

	/* 检查是否使能 */
	if (!ACONFIG_IsTcEnabled(tc_id)) {
		OSAL_printf("[ ERROR    ] TC %u is disabled\n", tc_id);
		return OSAL_ERR_NOT_PERMITTED;
	}

	/* 模拟执行（实际场景会调用PDL层） */
	OSAL_pthread_mutex_lock(&ctx->mutex);

	ctx->tc_results[ctx->tc_count].tc_id = tc_id;
	ctx->tc_results[ctx->tc_count].status = OSAL_SUCCESS;
	ctx->tc_results[ctx->tc_count].timestamp = (uint32_t)OSAL_time(NULL);
	ctx->tc_count++;

	/* 处理失效映射 - 使TC配置中内嵌的失效映射 */
	if (tc_config->invalidated_tm_ids && tc_config->invalidated_tm_count > 0) {
		OSAL_printf("[ INFO     ] TC %u affects %u TM items\n",
			    tc_id, tc_config->invalidated_tm_count);

		/* 标记受影响的TM为无效 */
		for (i = 0; i < tc_config->invalidated_tm_count; i++) {
			uint32_t tm_id = tc_config->invalidated_tm_ids[i];
			uint32_t j;

			for (j = 0; j < ctx->tm_count; j++) {
				if (ctx->tm_cache[j].tm_id == tm_id) {
					ctx->tm_cache[j].valid = false;
					OSAL_printf("[ INFO     ] TM %u invalidated by TC %u\n",
						    tm_id, tc_id);
					break;
				}
			}
		}
	}

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	OSAL_printf("[ INFO     ] TC %u executed successfully\n", tc_id);
	return OSAL_SUCCESS;
}

/**
 * @brief 模拟TM采集
 */
static int32_t collect_telemetry(uint32_t tm_id, system_context_t *ctx)
{
	const aconfig_tm_config_t *tm_config;

	/* 查询TM配置 */
	tm_config = ACONFIG_GetTmConfig(tm_id);
	if (!tm_config) {
		OSAL_printf("[ ERROR    ] TM %u not found in config\n", tm_id);
		return OSAL_ERR_NOT_FOUND;
	}

	/* 检查是否使能 */
	if (!ACONFIG_IsTmEnabled(tm_id)) {
		OSAL_printf("[ ERROR    ] TM %u is disabled\n", tm_id);
		return OSAL_ERR_NOT_PERMITTED;
	}

	/* 模拟采集（实际场景会调用PDL层） */
	OSAL_pthread_mutex_lock(&ctx->mutex);

	ctx->tm_cache[ctx->tm_count].tm_id = tm_id;
	ctx->tm_cache[ctx->tm_count].value = tm_id * 10;  /* 模拟数据 */
	ctx->tm_cache[ctx->tm_count].timestamp = (uint32_t)OSAL_time(NULL);
	ctx->tm_cache[ctx->tm_count].valid = true;
	ctx->tm_count++;

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	OSAL_printf("[ INFO     ] TM %u collected: value=%u\n",
		    tm_id, tm_id * 10);
	return OSAL_SUCCESS;
}

/**
 * @brief 初始化测试系统上下文
 */
static void init_system_context(system_context_t *ctx)
{
	OSAL_memset(ctx, 0, sizeof(system_context_t));
	OSAL_pthread_mutex_init(&ctx->mutex, NULL);
}

/**
 * @brief 清理测试系统上下文
 */
static void cleanup_system_context(system_context_t *ctx)
{
	OSAL_pthread_mutex_destroy(&ctx->mutex);
	OSAL_memset(ctx, 0, sizeof(system_context_t));
}

/* ========================================================================
 * 系统测试用例
 * ======================================================================== */

/**
 * @brief 测试应用配置端到端验证
 * @note 测试从初始化、配置注册、查询到执行的完整流程
 */
static void test_system_app_config_end_to_end_validation(void)
{
	OSAL_printf("[ TEST     ] App config end-to-end validation system test\n");

	int32_t ret;
	aconfig_statistics_t stats;

	/* 步骤1: 初始化ACONFIG */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Step 1: ACONFIG initialized\n");

	/* 步骤2: 准备配置表 */
	static const uint32_t tc_power_off_affects[] = {TEST_TM_VOLTAGE, TEST_TM_STATUS};
	static const uint32_t tc_reset_affects[] = {TEST_TM_STATUS, TEST_TM_MODE};

	static const aconfig_tc_entry_t tc_entries[] = {
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
				.invalidated_tm_ids = tc_power_off_affects,
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
				.invalidated_tm_ids = tc_reset_affects,
				.invalidated_tm_count = 2,
				.enabled = true,
				.user_context = NULL
			}
		}
	};

	static const aconfig_tm_entry_t tm_entries[] = {
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
				.enabled = true,
				.user_context = NULL
			}
		}
	};

	static const aconfig_config_table_t config_table = {
		.name = "E2E_TEST_CONFIG",
		.hwid_count = 0,
		.hwid_list = NULL,
		.tc_entries = tc_entries,
		.tc_count = 3,
		.tm_entries = tm_entries,
		.tm_count = 3
	};

	/* 步骤3: 注册配置表 */
	ret = ACONFIG_RegisterTable(&config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Step 2: Configuration table registered\n");

	/* 步骤4: 验证配置统计 */
	ret = ACONFIG_GetStatistics(&stats);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(3, stats.tc_enabled_count);
	TEST_ASSERT_EQUAL(0, stats.tc_disabled_count);
	TEST_ASSERT_EQUAL(3, stats.tm_enabled_count);
	TEST_ASSERT_EQUAL(0, stats.tm_disabled_count);
	OSAL_printf("[ INFO     ] Step 3: Statistics validated (TC:%u/%u TM:%u/%u)\n",
		    stats.tc_enabled_count, stats.tc_disabled_count,
		    stats.tm_enabled_count, stats.tm_disabled_count);

	/* 步骤5: 初始化系统上下文 */
	init_system_context(&g_sys_ctx);
	OSAL_printf("[ INFO     ] Step 4: System context initialized\n");

	/* 步骤6: 执行完整的TC-TM流程 */
	/* 6.1: 采集初始TM数据 */
	ret = collect_telemetry(TEST_TM_VOLTAGE, &g_sys_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	ret = collect_telemetry(TEST_TM_STATUS, &g_sys_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Step 5: Initial telemetry collected\n");

	/* 6.2: 执行TC并观察失效映射生效 */
	ret = execute_telecommand(TEST_TC_POWER_OFF, &g_sys_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 验证失效映射生效 */
	TEST_ASSERT_EQUAL(false, g_sys_ctx.tm_cache[0].valid);  /* TEST_TM_VOLTAGE */
	TEST_ASSERT_EQUAL(false, g_sys_ctx.tm_cache[1].valid);  /* TEST_TM_STATUS */
	OSAL_printf("[ INFO     ] Step 6: TC executed and invalidation maps applied\n");

	/* 6.3: 重新采集TM数据 */
	g_sys_ctx.tm_count = 0;  /* 重置计数器 */
	ret = collect_telemetry(TEST_TM_VOLTAGE, &g_sys_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(true, g_sys_ctx.tm_cache[0].valid);
	OSAL_printf("[ INFO     ] Step 7: Telemetry re-collected and validated\n");

	/* 步骤7: 清理 */
	cleanup_system_context(&g_sys_ctx);
	OSAL_printf("[ PASS     ] End-to-end validation test completed\n");
}

/**
 * @brief 测试遥控遥测配置集成
 * @note 测试多个TC/TM交互场景，验证配置一致性
 */
static void test_system_tc_tm_config_integration(void)
{
	OSAL_printf("[ TEST     ] TC/TM config integration system test\n");

	int32_t ret;
	const aconfig_tc_config_t *tc_config;
	const aconfig_tm_config_t *tm_config;

	/* 步骤1: 初始化 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 步骤2: 准备并注册配置 */
	static const uint32_t tc_set_mode_affects[] = {TEST_TM_MODE, TEST_TM_HEALTH};

	static const aconfig_tc_entry_t tc_entries[] = {
		{
			.function_id = TEST_TC_SET_MODE,
			.config = {
				.function_id = TEST_TC_SET_MODE,
				.device = {.type = PCONFIG_DEVICE_FPGA, .index = 0},
				.invalidated_tm_ids = tc_set_mode_affects,
				.invalidated_tm_count = 2,
				.enabled = true,
				.user_context = (void *)0xDEADBEEF
			}
		},
		{
			.function_id = TEST_TC_CALIBRATE,
			.config = {
				.function_id = TEST_TC_CALIBRATE,
				.device = {.type = PCONFIG_DEVICE_SENSOR, .index = 1},
				.invalidated_tm_ids = NULL,
				.invalidated_tm_count = 0,
				.enabled = false,  /* 禁用状态 */
				.user_context = NULL
			}
		}
	};

	static const aconfig_tm_entry_t tm_entries[] = {
		{
			.function_id = TEST_TM_MODE,
			.config = {
				.function_id = TEST_TM_MODE,
				.device = {.type = PCONFIG_DEVICE_FPGA, .index = 0},
				.poll_period_ms = 2000,
				.validity_period_ms = 5000,
				.enabled = true,
				.user_context = NULL
			}
		},
		{
			.function_id = TEST_TM_HEALTH,
			.config = {
				.function_id = TEST_TM_HEALTH,
				.device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
				.poll_period_ms = 100,
				.validity_period_ms = 500,
				.enabled = true,
				.user_context = (void *)0xCAFEBABE
			}
		}
	};

	static const aconfig_config_table_t config_table = {
		.name = "INTEGRATION_TEST_CONFIG",
		.hwid_count = 0,
		.hwid_list = NULL,
		.tc_entries = tc_entries,
		.tc_count = 2,
		.tm_entries = tm_entries,
		.tm_count = 2
	};

	ret = ACONFIG_RegisterTable(&config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Configuration registered\n");

	/* 步骤3: 验证TC配置 */
	tc_config = ACONFIG_GetTcConfig(TEST_TC_SET_MODE);
	TEST_ASSERT_NOT_NULL(tc_config);
	TEST_ASSERT_EQUAL(TEST_TC_SET_MODE, tc_config->function_id);
	TEST_ASSERT_EQUAL(PCONFIG_DEVICE_FPGA, tc_config->device.type);
	TEST_ASSERT_EQUAL(0, tc_config->device.index);
	TEST_ASSERT_EQUAL(true, tc_config->enabled);
	TEST_ASSERT_EQUAL((void *)0xDEADBEEF, tc_config->user_context);
	TEST_ASSERT_EQUAL(2, tc_config->invalidated_tm_count);
	OSAL_printf("[ INFO     ] TC config validated\n");

	/* 步骤4: 验证TM配置 */
	tm_config = ACONFIG_GetTmConfig(TEST_TM_MODE);
	TEST_ASSERT_NOT_NULL(tm_config);
	TEST_ASSERT_EQUAL(TEST_TM_MODE, tm_config->function_id);
	TEST_ASSERT_EQUAL(PCONFIG_DEVICE_FPGA, tm_config->device.type);
	TEST_ASSERT_EQUAL(0, tm_config->device.index);
	TEST_ASSERT_EQUAL(2000, tm_config->poll_period_ms);
	TEST_ASSERT_EQUAL(5000, tm_config->validity_period_ms);
	TEST_ASSERT_EQUAL(true, tm_config->enabled);
	OSAL_printf("[ INFO     ] TM config validated\n");

	/* 步骤5: 验证失效映射 */
	uint32_t affected_ids[10];
	uint32_t actual_count;

	ret = ACONFIG_GetInvalidationMap(TEST_TC_SET_MODE, affected_ids, 10, &actual_count);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(2, actual_count);
	TEST_ASSERT_EQUAL(TEST_TM_MODE, affected_ids[0]);
	TEST_ASSERT_EQUAL(TEST_TM_HEALTH, affected_ids[1]);
	OSAL_printf("[ INFO     ] Invalidation map validated: %u affected TMs\n", actual_count);

	/* 步骤6: 验证禁用的TC */
	TEST_ASSERT_EQUAL(false, ACONFIG_IsTcEnabled(TEST_TC_CALIBRATE));
	tc_config = ACONFIG_GetTcConfig(TEST_TC_CALIBRATE);
	TEST_ASSERT_NOT_NULL(tc_config);
	TEST_ASSERT_EQUAL(false, tc_config->enabled);
	OSAL_printf("[ INFO     ] Disabled TC verified\n");

	/* 步骤7: 验证设备引用一致性 */
	tm_config = ACONFIG_GetTmConfig(TEST_TM_MODE);
	tc_config = ACONFIG_GetTcConfig(TEST_TC_SET_MODE);
	TEST_ASSERT_EQUAL(tc_config->device.type, tm_config->device.type);
	TEST_ASSERT_EQUAL(tc_config->device.index, tm_config->device.index);
	OSAL_printf("[ INFO     ] Device reference consistency validated\n");

	OSAL_printf("[ PASS     ] TC/TM integration test completed\n");
}

/**
 * @brief 测试配置错误处理流程
 * @note 测试各种错误场景下的系统行为
 */
static void test_system_config_error_handling(void)
{
	OSAL_printf("[ TEST     ] Config error handling system test\n");

	int32_t ret;
	const aconfig_tc_config_t *tc_config;
	const aconfig_tm_config_t *tm_config;

	/* 步骤1: 初始化 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 步骤2: 尝试查询未注册的配置 */
	tc_config = ACONFIG_GetTcConfig(9999);
	TEST_ASSERT_NULL(tc_config);
	OSAL_printf("[ INFO     ] Query non-existent TC returned NULL as expected\n");

	tm_config = ACONFIG_GetTmConfig(9999);
	TEST_ASSERT_NULL(tm_config);
	OSAL_printf("[ INFO     ] Query non-existent TM returned NULL as expected\n");

	/* 步骤3: 检查未配置功能的使能状态 */
	TEST_ASSERT_EQUAL(false, ACONFIG_IsTcEnabled(9999));
	TEST_ASSERT_EQUAL(false, ACONFIG_IsTmEnabled(9999));
	OSAL_printf("[ INFO     ] Non-existent functions correctly reported as disabled\n");

	/* 步骤4: 注册空表测试 */
	static const aconfig_config_table_t empty_table = {
		.name = "EMPTY_TABLE",
		.hwid_count = 0,
		.hwid_list = NULL,
		.tc_entries = NULL,
		.tc_count = 0,
		.tm_entries = NULL,
		.tm_count = 0
	};

	ret = ACONFIG_RegisterTable(&empty_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Empty table registered successfully\n");

	/* 步骤5: 验证空表统计 */
	aconfig_statistics_t stats;
	ret = ACONFIG_GetStatistics(&stats);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, stats.tc_enabled_count);
	TEST_ASSERT_EQUAL(0, stats.tm_enabled_count);
	OSAL_printf("[ INFO     ] Empty table statistics correct\n");

	/* 步骤6: 注册有效配置 */
	static const aconfig_tc_entry_t tc_entries[] = {
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
		}
	};

	static const aconfig_config_table_t valid_table = {
		.name = "VALID_TABLE",
		.hwid_count = 0,
		.hwid_list = NULL,
		.tc_entries = tc_entries,
		.tc_count = 1,
		.tm_entries = NULL,
		.tm_count = 0
	};

	ret = ACONFIG_RegisterTable(&valid_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Valid table registered (overwrote empty table)\n");

	/* 步骤7: 验证表覆盖正确 */
	ret = ACONFIG_GetStatistics(&stats);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(1, stats.tc_enabled_count);
	OSAL_printf("[ INFO     ] Table overwrite verified\n");

	/* 步骤8: 测试GetInvalidationMap错误处理 */
	uint32_t affected_ids[10];
	uint32_t actual_count;

	/* NULL指针测试 */
	ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_ON, NULL, 10, &actual_count);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] NULL pointer correctly rejected\n");

	/* 无效TC ID测试 */
	ret = ACONFIG_GetInvalidationMap(9999, affected_ids, 10, &actual_count);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Invalid TC ID correctly rejected\n");

	/* 缓冲区太小测试 */
	ret = ACONFIG_GetInvalidationMap(TEST_TC_POWER_ON, affected_ids, 0, &actual_count);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Buffer too small correctly handled\n");

	/* 步骤9: 初始化系统上下文并测试执行错误 */
	init_system_context(&g_sys_ctx);

	/* 尝试执行不存在的TC */
	ret = execute_telecommand(9999, &g_sys_ctx);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Non-existent TC execution correctly failed\n");

	/* 尝试采集不存在的TM */
	ret = collect_telemetry(9999, &g_sys_ctx);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Non-existent TM collection correctly failed\n");

	cleanup_system_context(&g_sys_ctx);

	OSAL_printf("[ PASS     ] Error handling test completed\n");
}

/* ========================================================================
 * 测试套件注册
 * ======================================================================== */

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_system_app_config_end_to_end_validation",
		.func = test_system_app_config_end_to_end_validation,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_system_tc_tm_config_integration",
		.func = test_system_tc_tm_config_integration,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_system_config_error_handling",
		.func = test_system_config_error_handling,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "system_aconfig",
	.module_name = "system_aconfig",
	.layer_name = "ACONFIG",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_SLOW | TEST_TAG_INTEGRATION,
		.timeout_ms = 10000,
		.description = "ACONFIG system integration tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_system_aconfig_tests(void)
{
	libutest_register_suite(&test_suite);
}
