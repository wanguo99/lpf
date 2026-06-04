/**
 * @file test_stress_osal.c
 * @brief OSAL层压力测试示例
 */

#include "test_framework.h"
#include "test_stress.h"
#include "osal/osal.h"

/* 共享测试数据 */
typedef struct {
    osal_mutex_t *mutex;
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
    int32_t ret = OSAL_MutexLock(data->mutex);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 临界区操作 */
    OSAL_AtomicIncrement(&data->counter);

    /* 解锁 */
    OSAL_MutexUnlock(data->mutex);

    return 0;
}

/**
 * 测试互斥锁并发压力
 */
TEST_CASE(test_stress_mutex_concurrency) {
    mutex_stress_data_t data;
    const uint32_t thread_count = 10;
    const uint32_t duration_sec = 5;

    /* 初始化测试数据 */
    TEST_ASSERT_EQUAL(OSAL_MutexCreate(&data.mutex), OSAL_SUCCESS);
    OSAL_AtomicInit(&data.counter, 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Mutex Concurrency", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_Printf("[ INFO     ] Running mutex concurrency test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, mutex_stress_worker, &data), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_MutexDelete(data.mutex);
}

/**
 * 原子操作压力测试工作函数
 */
static int32_t atomic_stress_worker(void *user_data, uint32_t iteration) {
    osal_atomic_uint32_t *counter = (osal_atomic_uint32_t*)user_data;
    (void)iteration;  /* 未使用的参数 */

    /* 原子自增 */
    OSAL_AtomicIncrement(counter);

    return 0;
}

/**
 * 测试原子操作并发压力
 */
TEST_CASE(test_stress_atomic_operations) {
    osal_atomic_uint32_t counter;
    const uint32_t thread_count = 10;
    const uint32_t iterations = 10000;

    /* 初始化 */
    OSAL_AtomicInit(&counter, 0);

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
    OSAL_Printf("[ INFO     ] Running atomic operations test: %u threads, %u iterations\n",
               thread_count, iterations);
    TEST_ASSERT_EQUAL(stress_run(ctx, atomic_stress_worker, &counter), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果：所有线程的所有迭代都应该成功 */
    uint32_t final_count = OSAL_AtomicLoad(&counter);
    uint32_t expected_count = thread_count * iterations;
    TEST_ASSERT_EQUAL(final_count, expected_count);

    STRESS_ASSERT_NO_ERRORS(ctx);

    /* 清理 */
    stress_context_destroy(ctx);
}

/* 注册压力测试模块 */
TEST_MODULE_BEGIN(stress_osal, "STRESS")
    TEST_CASE_REGISTER(test_stress_mutex_concurrency, "Mutex concurrency stress test")
    TEST_CASE_REGISTER(test_stress_atomic_operations, "Atomic operations stress test")
TEST_MODULE_END(stress_osal, "STRESS")
