/**
 * @file test_stress_osal.c
 * @brief OSAL层压力测试示例
 */

#include <test/test_framework.h>
#include <test/test_stress.h>
#include "osal.h"

/* 共享测试数据 */
typedef struct {
    osal_mutex_t mutex;
    osal_atomic_uint32_t counter;
    uint32_t expected_count;
} mutex_stress_data_t;

/**
 * 互斥锁压力测试工作函数
 */
static int32_t mutex_stress_worker(void *user_data, uint32_t iteration) {
    mutex_stress_data_t *data = (mutex_stress_data_t*)user_data;
    (void)iteration;  /* 未使用的参数 */

    /* 加锁 */
    int32_t ret = OSAL_pthread_mutex_lock(&data->mutex);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 临界区操作 */
    OSAL_atomic_inc(&data->counter);

    /* 解锁 */
    OSAL_pthread_mutex_unlock(&data->mutex);

    return 0;
}

/**
 * 测试互斥锁并发压力
 */
static void test_stress_mutex_concurrency(void) {
    mutex_stress_data_t data;
    const uint32_t thread_count = 10;
    const uint32_t duration_sec = 5;

    /* 初始化测试数据 */
    TEST_ASSERT_EQUAL(OSAL_pthread_mutex_init(&data.mutex, NULL), 0);
    OSAL_atomic_init(&data.counter, 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Mutex Concurrency", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_printf("[ INFO     ] Running mutex concurrency test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, mutex_stress_worker, &data), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_pthread_mutex_destroy(&data.mutex);
}

/**
 * 原子操作压力测试工作函数
 */
static int32_t atomic_stress_worker(void *user_data, uint32_t iteration) {
    osal_atomic_uint32_t *counter = (osal_atomic_uint32_t*)user_data;
    (void)iteration;  /* 未使用的参数 */

    /* 原子自增 */
    OSAL_atomic_inc(counter);

    return 0;
}

/**
 * 测试原子操作并发压力
 */
static void test_stress_atomic_operations(void) {
    osal_atomic_uint32_t counter;
    const uint32_t thread_count = 10;
    const uint32_t iterations = 10000;

    /* 初始化 */
    OSAL_atomic_init(&counter, 0);

    /* 创建压力测试上下文 */
    stress_config_t config = {
        .type = STRESS_TYPE_CONCURRENCY,
        .thread_count = thread_count,
        .duration_sec = 0,
        .iterations = iterations,
        .ramp_up_sec = 0,
        .stop_on_error = false
    };
    stress_context_t *ctx = stress_context_create("Atomic Operations", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_printf("[ INFO     ] Running atomic operations test: %u threads, %u iterations\n",
               thread_count, iterations);
    TEST_ASSERT_EQUAL(stress_run(ctx, atomic_stress_worker, &counter), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果：所有线程的所有迭代都应该成功 */
    uint32_t final_count = OSAL_atomic_load(&counter);
    uint32_t expected_count = thread_count * iterations;
    TEST_ASSERT_EQUAL(final_count, expected_count);

    STRESS_ASSERT_NO_ERRORS(ctx);

    /* 清理 */
    stress_context_destroy(ctx);
}

/* 注册压力测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_stress_mutex_concurrency",
		.func = test_stress_mutex_concurrency,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_atomic_operations",
		.func = test_stress_atomic_operations,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "stress_osal",
	.module_name = "stress_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 10000,
		.description = "OSAL stress_osal tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_stress_osal_tests(void)
{
	libutest_register_suite(&test_suite);
}
