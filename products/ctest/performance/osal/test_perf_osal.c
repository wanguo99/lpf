/**
 * @file test_perf_osal.c
 * @brief OSAL层性能测试示例
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_performance.h>
#include "osal.h"

/* 性能基准定义 */
static const perf_baseline_t mutex_lock_baseline = {
	.name = "osal_pthread_mutex_lock",
	.type = PERF_METRIC_LATENCY,
	.baseline_value = 5.0, /* 基准：5微秒 */
	.tolerance_percent = 50.0 /* 容差：50% */
};

/**
 * 测试互斥锁性能
 */
static void _test_perf_mutex_lock_unlock(void)
{
	const uint32_t iterations = 10000;
	osal_mutex_t mutex;

	/* 创建互斥锁 */
	TEST_ASSERT_EQUAL(osal_pthread_mutex_init(&mutex, NULL), OSAL_SUCCESS);

	/* 创建性能测量上下文 */
	perf_context_t *ctx = perf_context_create("osal_pthread_mutex_lock",
											  PERF_METRIC_LATENCY, iterations);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 性能测试 */
	uint32_t i;

	for (i = 0; i < iterations; i++) {
		perf_begin(ctx);
		osal_pthread_mutex_lock(&mutex);
		osal_pthread_mutex_unlock(&mutex);
		perf_end(ctx);
	}

	/* 打印统计 */
	perf_print_stats(ctx);

	/* 与基准对比 */
	perf_result_t result;
	TEST_ASSERT_EQUAL(perf_compare_baseline(ctx, &mutex_lock_baseline, &result),
					  0);
	perf_print_result(&result);

	/* 清理 */
	perf_context_destroy(ctx);
	osal_pthread_mutex_destroy(&mutex);
}

/**
 * 测试原子操作性能
 */
static void _test_perf_atomic_operations(void)
{
	const uint32_t iterations = 100000;
	osal_atomic_uint32_t counter;

	osal_atomic_init(&counter, 0);

	/* 创建性能测量上下文 */
	perf_context_t *ctx = perf_context_create("OSAL_AtomicIncrement",
											  PERF_METRIC_LATENCY, iterations);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 性能测试 */
	uint32_t i;

	for (i = 0; i < iterations; i++) {
		perf_begin(ctx);
		osal_atomic_inc(&counter);
		perf_end(ctx);
	}

	/* 打印统计 */
	perf_print_stats(ctx);

	/* 验证结果 */
	TEST_ASSERT_EQUAL(osal_atomic_load(&counter), iterations);

	/* 清理 */
	perf_context_destroy(ctx);
}

/**
 * 测试时间获取性能
 */
static void _test_perf_time_get(void)
{
	const uint32_t iterations = 10000;

	/* 创建性能测量上下文 */
	perf_context_t *ctx = perf_context_create("osal_get_monotonic_time",
											  PERF_METRIC_LATENCY, iterations);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 性能测试 */
	uint32_t i;

	for (i = 0; i < iterations; i++) {
		perf_begin(ctx);
		osal_get_monotonic_time();
		perf_end(ctx);
	}

	/* 打印统计 */
	perf_print_stats(ctx);

	/* 清理 */
	perf_context_destroy(ctx);
}

/* 注册性能测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_perf_mutex_lock_unlock",
	  .func = _test_perf_mutex_lock_unlock,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_perf_atomic_operations",
	  .func = _test_perf_atomic_operations,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_perf_time_get",
	  .func = _test_perf_time_get,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "perf_osal",
	.module_name = "perf_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_PERFORMANCE,
				  .tags = TEST_TAG_SLOW,
				  .timeout_ms = 5000,
				  .description = "OSAL perf_osal tests" }
};

/* 测试套件注册函数 */
void register_perf_osal_tests(void)
{
	libutest_register_suite(&test_suite);
}
