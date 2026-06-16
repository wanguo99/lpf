/**
 * @file test_stress_aconfig.c
 * @brief ACONFIG层压力测试
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_stress.h>
#include "aconfig.h"

/* ========================================================================
 * 测试数据定义
 * ======================================================================== */

/* 大规模配置表 - 用于性能基线测试 */
#define LARGE_TC_COUNT 100
#define LARGE_TM_COUNT 200

static aconfig_tc_entry_t large_tc_entries[LARGE_TC_COUNT];
static aconfig_tm_entry_t large_tm_entries[LARGE_TM_COUNT];
static const uint32_t test_invalidation_list[] = {10, 20, 30, 40, 50};

/* 小配置表 - 用于并发测试 */
static const uint32_t small_tc_invalidation[] = {1, 2, 3};

static const aconfig_tc_entry_t small_tc_entries[] = {
	{
		.function_id = 0,
		.config = {
			.function_id = 0,
			.device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
			.invalidated_tm_ids = small_tc_invalidation,
			.invalidated_tm_count = 3,
			.enabled = true,
			.user_context = NULL
		}
	},
	{
		.function_id = 1,
		.config = {
			.function_id = 1,
			.device = {.type = PCONFIG_DEVICE_MCU, .index = 0},
			.invalidated_tm_ids = NULL,
			.invalidated_tm_count = 0,
			.enabled = true,
			.user_context = NULL
		}
	},
	{
		.function_id = 5,  /* 稀疏数组 */
		.config = {
			.function_id = 5,
			.device = {.type = PCONFIG_DEVICE_BMC, .index = 1},
			.invalidated_tm_ids = NULL,
			.invalidated_tm_count = 0,
			.enabled = false,
			.user_context = NULL
		}
	}
};

static const aconfig_tm_entry_t small_tm_entries[] = {
	{
		.function_id = 0,
		.config = {
			.function_id = 0,
			.device = {.type = PCONFIG_DEVICE_BMC, .index = 0},
			.poll_period_ms = 1000,
			.validity_period_ms = 2000,
			.enabled = true,
			.user_context = NULL
		}
	},
	{
		.function_id = 1,
		.config = {
			.function_id = 1,
			.device = {.type = PCONFIG_DEVICE_SENSOR, .index = 0},
			.poll_period_ms = 500,
			.validity_period_ms = 1000,
			.enabled = true,
			.user_context = NULL
		}
	},
	{
		.function_id = 2,
		.config = {
			.function_id = 2,
			.device = {.type = PCONFIG_DEVICE_BMC, .index = 1},
			.poll_period_ms = 100,
			.validity_period_ms = 200,
			.enabled = false,
			.user_context = NULL
		}
	}
};

static const aconfig_config_table_t small_config_table = {
	.name = "STRESS_TEST_SMALL",
	.hwid_count = 0,
	.hwid_list = NULL,
	.tc_entries = small_tc_entries,
	.tc_count = sizeof(small_tc_entries) / sizeof(aconfig_tc_entry_t),
	.tm_entries = small_tm_entries,
	.tm_count = sizeof(small_tm_entries) / sizeof(aconfig_tm_entry_t)
};

static aconfig_config_table_t large_config_table = {
	.name = "STRESS_TEST_LARGE",
	.hwid_count = 0,
	.hwid_list = NULL,
	.tc_entries = large_tc_entries,
	.tc_count = LARGE_TC_COUNT,
	.tm_entries = large_tm_entries,
	.tm_count = LARGE_TM_COUNT
};

/* ========================================================================
 * 辅助函数
 * ======================================================================== */

/**
 * @brief 初始化大规模配置表（用于性能测试）
 */
static void init_large_config_table(void)
{
	/* 初始化 TC 条目 */
	for (uint32_t i = 0; i < LARGE_TC_COUNT; i++) {
		large_tc_entries[i].function_id = i;
		large_tc_entries[i].config.function_id = i;
		large_tc_entries[i].config.device.type = PCONFIG_DEVICE_BMC;
		large_tc_entries[i].config.device.index = i % 10;

		/* 部分 TC 有失效映射 */
		if (i % 10 == 0) {
			large_tc_entries[i].config.invalidated_tm_ids = test_invalidation_list;
			large_tc_entries[i].config.invalidated_tm_count =
				sizeof(test_invalidation_list) / sizeof(uint32_t);
		} else {
			large_tc_entries[i].config.invalidated_tm_ids = NULL;
			large_tc_entries[i].config.invalidated_tm_count = 0;
		}

		large_tc_entries[i].config.enabled = (i % 5 != 0);  /* 20% 禁用 */
		large_tc_entries[i].config.user_context = NULL;
	}

	/* 初始化 TM 条目 */
	for (uint32_t i = 0; i < LARGE_TM_COUNT; i++) {
		large_tm_entries[i].function_id = i;
		large_tm_entries[i].config.function_id = i;
		large_tm_entries[i].config.device.type = PCONFIG_DEVICE_SENSOR;
		large_tm_entries[i].config.device.index = i % 20;
		large_tm_entries[i].config.poll_period_ms = 100 + (i % 10) * 100;
		large_tm_entries[i].config.validity_period_ms = 200 + (i % 10) * 200;
		large_tm_entries[i].config.enabled = (i % 7 != 0);  /* ~14% 禁用 */
		large_tm_entries[i].config.user_context = NULL;
	}
}

/* ========================================================================
 * 压力测试工作函数
 * ======================================================================== */

/**
 * @brief TC 查询并发压力工作函数
 */
static int32_t tc_query_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	/* 随机查询不同的 TC ID */
	uint32_t tc_id = iteration % 10;
	const aconfig_tc_config_t *config = ACONFIG_GetTcConfig(tc_id);

	/* 验证结果 */
	if (tc_id < 6) {
		/* 应该找到 */
		if (config == NULL) {
			return OSAL_ERR_GENERIC;
		}
		if (config->function_id != tc_id) {
			return OSAL_ERR_INVALID_STATE;
		}
	} else {
		/* 不应该找到 */
		if (config != NULL) {
			return OSAL_ERR_INVALID_STATE;
		}
	}

	return OSAL_SUCCESS;
}

/**
 * @brief TM 查询并发压力工作函数
 */
static int32_t tm_query_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	/* 随机查询不同的 TM ID */
	uint32_t tm_id = iteration % 10;
	const aconfig_tm_config_t *config = ACONFIG_GetTmConfig(tm_id);

	/* 验证结果 */
	if (tm_id < 3) {
		/* 应该找到 */
		if (config == NULL) {
			return OSAL_ERR_GENERIC;
		}
		if (config->function_id != tm_id) {
			return OSAL_ERR_INVALID_STATE;
		}
	} else {
		/* 不应该找到 */
		if (config != NULL) {
			return OSAL_ERR_INVALID_STATE;
		}
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 混合查询工作函数（TC + TM + IsTcEnabled + IsTmEnabled）
 */
static int32_t mixed_query_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	uint32_t op = iteration % 4;
	uint32_t id = iteration % 10;

	switch (op) {
	case 0:  /* TC 查询 */
		ACONFIG_GetTcConfig(id);
		break;
	case 1:  /* TM 查询 */
		ACONFIG_GetTmConfig(id);
		break;
	case 2:  /* TC 使能检查 */
		ACONFIG_IsTcEnabled(id);
		break;
	case 3:  /* TM 使能检查 */
		ACONFIG_IsTmEnabled(id);
		break;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 统计信息查询工作函数
 */
static int32_t statistics_query_worker(void *user_data, uint32_t iteration)
{
	(void)iteration;
	(void)user_data;

	aconfig_statistics_t stats;
	int32_t ret = ACONFIG_GetStatistics(&stats);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 验证统计信息合理性 */
	if (stats.tc_enabled_count == 0 && stats.tm_enabled_count == 0) {
		return OSAL_ERR_INVALID_STATE;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 大规模查询工作函数（测试 O(n) 查找性能）
 */
static int32_t large_query_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	/* 查询大配置表中的不同 ID */
	uint32_t tc_id = iteration % LARGE_TC_COUNT;
	uint32_t tm_id = iteration % LARGE_TM_COUNT;

	/* TC 查询 */
	const aconfig_tc_config_t *tc_config = ACONFIG_GetTcConfig(tc_id);
	if (tc_config == NULL) {
		return OSAL_ERR_GENERIC;
	}
	if (tc_config->function_id != tc_id) {
		return OSAL_ERR_INVALID_STATE;
	}

	/* TM 查询 */
	const aconfig_tm_config_t *tm_config = ACONFIG_GetTmConfig(tm_id);
	if (tm_config == NULL) {
		return OSAL_ERR_GENERIC;
	}
	if (tm_config->function_id != tm_id) {
		return OSAL_ERR_INVALID_STATE;
	}

	return OSAL_SUCCESS;
}

/* ========================================================================
 * 测试用例
 * ======================================================================== */

/**
 * @brief 测试 TC 查询并发压力
 */
static void test_stress_tc_query_concurrent(void)
{
	const uint32_t thread_count = 10;
	const uint32_t iterations = 10000;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("TC Query Concurrent", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running TC query concurrent test: %u threads, %u iterations\n",
		   thread_count, iterations);
	TEST_ASSERT_EQUAL(stress_run(ctx, tc_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试 TM 查询并发压力
 */
static void test_stress_tm_query_concurrent(void)
{
	const uint32_t thread_count = 10;
	const uint32_t iterations = 10000;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("TM Query Concurrent", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running TM query concurrent test: %u threads, %u iterations\n",
		   thread_count, iterations);
	TEST_ASSERT_EQUAL(stress_run(ctx, tm_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试混合查询并发压力
 */
static void test_stress_mixed_query_concurrent(void)
{
	const uint32_t thread_count = 16;
	const uint32_t iterations = 20000;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("Mixed Query Concurrent", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running mixed query concurrent test: %u threads, %u iterations\n",
		   thread_count, iterations);
	TEST_ASSERT_EQUAL(stress_run(ctx, mixed_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试统计信息并发查询压力
 */
static void test_stress_statistics_concurrent(void)
{
	const uint32_t thread_count = 8;
	const uint32_t iterations = 5000;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("Statistics Query Concurrent", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running statistics query concurrent test: %u threads, %u iterations\n",
		   thread_count, iterations);
	TEST_ASSERT_EQUAL(stress_run(ctx, statistics_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试大规模配置表查询性能（建立 O(n) 基线）
 */
static void test_stress_large_table_performance(void)
{
	const uint32_t thread_count = 4;
	const uint32_t iterations = 1000;
	int32_t ret;

	/* 初始化大配置表 */
	init_large_config_table();

	/* 初始化并注册大配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&large_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("Large Table Performance", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running large table performance test: %u TCs, %u TMs, %u threads\n",
		   LARGE_TC_COUNT, LARGE_TM_COUNT, thread_count);
	TEST_ASSERT_EQUAL(stress_run(ctx, large_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 打印性能信息 */
	stress_stats_t stats;
	int32_t stats_ret = stress_get_stats(ctx, &stats);
	if (stats_ret == 0) {
		OSAL_printf("[ INFO     ] Average query latency: %.2f us (O(n) lookup)\n",
			   stats.avg_latency_us);
	}

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试长时间运行稳定性（资源泄漏检测）
 */
static void test_stress_long_running_stability(void)
{
	const uint32_t thread_count = 8;
	const uint32_t duration_sec = 10;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Long Running Stability", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running long-running stability test: %u threads, %u seconds\n",
		   thread_count, duration_sec);
	TEST_ASSERT_EQUAL(stress_run(ctx, mixed_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 清理 */
	stress_context_destroy(ctx);
}

/**
 * @brief 测试高频查询吞吐量
 */
static void test_stress_high_frequency_throughput(void)
{
	const uint32_t thread_count = 20;
	const uint32_t iterations = 50000;
	int32_t ret;

	/* 初始化并注册小配置表 */
	ret = ACONFIG_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = ACONFIG_RegisterTable(&small_config_table);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 创建压力测试上下文 */
	stress_config_t config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *ctx = stress_context_create("High Frequency Throughput", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running high-frequency throughput test: %u threads, %u iterations\n",
		   thread_count, iterations);
	TEST_ASSERT_EQUAL(stress_run(ctx, mixed_query_worker, NULL), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果 */
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	/* 打印吞吐量信息 */
	stress_stats_t stats;
	int32_t stats_ret = stress_get_stats(ctx, &stats);
	if (stats_ret == 0 && stats.elapsed_time_ms > 0) {
		double throughput = (double)stats.total_operations /
				   (stats.elapsed_time_ms / 1000.0);
		OSAL_printf("[ INFO     ] Throughput: %.0f ops/sec\n", throughput);
	}

	/* 清理 */
	stress_context_destroy(ctx);
}

/* ========================================================================
 * 测试套件注册
 * ======================================================================== */

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_stress_tc_query_concurrent",
		.func = test_stress_tc_query_concurrent,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_tm_query_concurrent",
		.func = test_stress_tm_query_concurrent,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_mixed_query_concurrent",
		.func = test_stress_mixed_query_concurrent,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_statistics_concurrent",
		.func = test_stress_statistics_concurrent,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_large_table_performance",
		.func = test_stress_large_table_performance,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_long_running_stability",
		.func = test_stress_long_running_stability,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_high_frequency_throughput",
		.func = test_stress_high_frequency_throughput,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "stress_aconfig",
	.module_name = "stress_aconfig",
	.layer_name = "ACONFIG",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 60000,  /* 60 seconds timeout for stress tests */
		.description = "ACONFIG stress tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_stress_aconfig_tests(void)
{
	libutest_register_suite(&test_suite);
}
