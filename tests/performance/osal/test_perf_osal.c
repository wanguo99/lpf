/**
 * @file test_perf_osal.c
 * @brief OSAL层性能测试示例
 */

#include "test_framework.h"
#include "test_performance.h"
#include "osal.h"

/* 性能基准定义 */
static const perf_baseline_t mutex_lock_baseline = {
    .name = "OSAL_MutexLock",
    .type = PERF_METRIC_LATENCY,
    .baseline_value = 5.0,      /* 基准：5微秒 */
    .tolerance_percent = 50.0   /* 容差：50% */
};

/**
 * 测试互斥锁性能
 */
TEST_CASE(test_perf_mutex_lock_unlock) {
    const uint32_t iterations = 10000;
    osal_mutex_t *mutex = NULL;

    /* 创建互斥锁 */
    TEST_ASSERT_EQUAL(OSAL_MutexCreate(&mutex), OSAL_SUCCESS);
    TEST_ASSERT_NOT_NULL(mutex);

    /* 创建性能测量上下文 */
    perf_context_t *ctx = perf_context_create("OSAL_MutexLock",
                                               PERF_METRIC_LATENCY,
                                               iterations);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 性能测试 */
    for (uint32_t i = 0; i < iterations; i++) {
        perf_begin(ctx);
        OSAL_MutexLock(mutex);
        OSAL_MutexUnlock(mutex);
        perf_end(ctx);
    }

    /* 打印统计 */
    perf_print_stats(ctx);

    /* 与基准对比 */
    perf_result_t result;
    TEST_ASSERT_EQUAL(perf_compare_baseline(ctx, &mutex_lock_baseline, &result), 0);
    perf_print_result(&result);

    /* 清理 */
    perf_context_destroy(ctx);
    OSAL_MutexDelete(mutex);
}

/**
 * 测试原子操作性能
 */
TEST_CASE(test_perf_atomic_operations) {
    const uint32_t iterations = 100000;
    osal_atomic_uint32_t counter;

    OSAL_AtomicInit(&counter, 0);

    /* 创建性能测量上下文 */
    perf_context_t *ctx = perf_context_create("OSAL_AtomicIncrement",
                                               PERF_METRIC_LATENCY,
                                               iterations);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 性能测试 */
    for (uint32_t i = 0; i < iterations; i++) {
        perf_begin(ctx);
        OSAL_AtomicIncrement(&counter);
        perf_end(ctx);
    }

    /* 打印统计 */
    perf_print_stats(ctx);

    /* 验证结果 */
    TEST_ASSERT_EQUAL(OSAL_AtomicLoad(&counter), iterations);

    /* 清理 */
    perf_context_destroy(ctx);
}

/**
 * 测试时间获取性能
 */
TEST_CASE(test_perf_time_get) {
    const uint32_t iterations = 10000;

    /* 创建性能测量上下文 */
    perf_context_t *ctx = perf_context_create("OSAL_GetMonotonicTime",
                                               PERF_METRIC_LATENCY,
                                               iterations);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 性能测试 */
    for (uint32_t i = 0; i < iterations; i++) {
        perf_begin(ctx);
        OSAL_GetMonotonicTime();
        perf_end(ctx);
    }

    /* 打印统计 */
    perf_print_stats(ctx);

    /* 清理 */
    perf_context_destroy(ctx);
}

/* 注册性能测试模块 */
TEST_MODULE_BEGIN(perf_osal, "PERFORMANCE")
    TEST_CASE_REGISTER(test_perf_mutex_lock_unlock, "Mutex lock/unlock performance")
    TEST_CASE_REGISTER(test_perf_atomic_operations, "Atomic operations performance")
    TEST_CASE_REGISTER(test_perf_time_get, "Time get performance")
TEST_MODULE_END(perf_osal, "PERFORMANCE")
