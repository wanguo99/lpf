#include <test_framework/test_framework.h>
/**
 * @file test_osal_time.c
 * @brief OSAL时间操作单元测试
 */

#include "osal.h"

/* 辅助函数：获取当前时间（微秒） */
static uint64_t get_time_in_micros(void)
{
    return (uint64_t)OSAL_get_monotonic_time();
}

/*===========================================================================
 * 延时函数测试
 *===========================================================================*/

/* 测试用例: msleep - 毫秒延时 */
static void test_osal_msleep_success(void)
{
    uint64_t start_time, end_time, elapsed_ms;

    /* 获取开始时间 */
    start_time = get_time_in_micros();

    /* 延时100毫秒 */
    int32_t ret = OSAL_msleep(100);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取结束时间 */
    end_time = get_time_in_micros();

    /* 计算实际延时（允许±20ms误差） */
    elapsed_ms = (end_time - start_time) / 1000;
    TEST_ASSERT_TRUE(elapsed_ms >= 80 && elapsed_ms <= 120);
}

/* 测试用例: usleep - 微秒延时 */
static void test_osal_usleep_success(void)
{
    uint64_t start_time, end_time, elapsed_us;

    /* 获取开始时间 */
    start_time = get_time_in_micros();

    /* 延时50000微秒（50毫秒） */
    int32_t ret = OSAL_usleep(50000);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取结束时间 */
    end_time = get_time_in_micros();

    /* 计算实际延时（允许±10ms误差） */
    elapsed_us = end_time - start_time;
    TEST_ASSERT_TRUE(elapsed_us >= 40000 && elapsed_us <= 60000);
}

/* 测试用例: sleep - 秒延时 */
static void test_osal_sleep_success(void)
{
    uint64_t start_time, end_time, elapsed_ms;

    /* 获取开始时间 */
    start_time = get_time_in_micros();

    /* 延时1秒 */
    int32_t ret = OSAL_sleep(1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取结束时间 */
    end_time = get_time_in_micros();

    /* 计算实际延时（允许±50ms误差） */
    elapsed_ms = (end_time - start_time) / 1000;
    TEST_ASSERT_TRUE(elapsed_ms >= 950 && elapsed_ms <= 1050);
}

/* 测试用例: nanosleep - 纳秒延时 */
static void test_osal_nanosleep_success(void)
{
    uint64_t start_time, end_time, elapsed_us;

    /* 获取开始时间 */
    start_time = get_time_in_micros();

    /* 延时10000000纳秒（10毫秒） */
    int32_t ret = OSAL_nanosleep(10000000);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取结束时间 */
    end_time = get_time_in_micros();

    /* 计算实际延时（允许±5ms误差） */
    elapsed_us = end_time - start_time;
    TEST_ASSERT_TRUE(elapsed_us >= 5000 && elapsed_us <= 15000);
}

/* 测试用例: msleep - OSAL毫秒延时（替代TaskDelay） */
static void test_osal_task_delay_success(void)
{
    uint64_t start_time, end_time, elapsed_ms;

    /* 获取开始时间 */
    start_time = get_time_in_micros();

    /* 延时200毫秒 */
    int32_t ret = OSAL_msleep(200);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取结束时间 */
    end_time = get_time_in_micros();

    /* 计算实际延时（允许±30ms误差） */
    elapsed_ms = (end_time - start_time) / 1000;
    TEST_ASSERT_TRUE(elapsed_ms >= 170 && elapsed_ms <= 230);
}

/*===========================================================================
 * 时间获取测试
 *===========================================================================*/

/* 测试用例: GetLocalTime - 获取本地时间 */
static void test_osal_get_local_time_success(void)
{
    OS_time_t time1, time2;
    int32_t ret;

    /* 获取第一次时间 */
    ret = OSAL_get_local_time(&time1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(time1.seconds > 0);

    /* 短暂延时 */
    OSAL_msleep(10);

    /* 获取第二次时间 */
    ret = OSAL_get_local_time(&time2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证时间递增 */
    TEST_ASSERT_TRUE(time2.seconds >= time1.seconds);
    if (time2.seconds == time1.seconds) {
        TEST_ASSERT_TRUE(time2.microsecs > time1.microsecs);
    }
}

/* 测试用例: GetLocalTime - 空指针 */
static void test_osal_get_local_time_null_pointer(void)
{
    int32_t ret = OSAL_get_local_time(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: GetTickCount - 获取系统滴答 */
static void test_osal_get_tick_count(void)
{
    uint32_t tick1, tick2;

    /* 获取第一次滴答 */
    tick1 = OSAL_get_tick_count();

    /* 短暂延时 */
    OSAL_msleep(50);

    /* 获取第二次滴答 */
    tick2 = OSAL_get_tick_count();

    /* 验证滴答递增（允许±20ms误差） */
    uint32_t diff = tick2 - tick1;
    TEST_ASSERT_TRUE(diff >= 30 && diff <= 70);
}

/* 测试用例: 时间单调性 */
static void test_osal_time_monotonic(void)
{
    OS_time_t prev_time, curr_time;

    OSAL_get_local_time(&prev_time);

    /* 连续获取10次时间，验证单调递增 */
    int32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_usleep(1000); /* 1毫秒 */
        OSAL_get_local_time(&curr_time);

        /* 验证时间递增 */
        if (curr_time.seconds == prev_time.seconds) {
            TEST_ASSERT_TRUE(curr_time.microsecs >= prev_time.microsecs);
        } else {
            TEST_ASSERT_TRUE(curr_time.seconds > prev_time.seconds);
        }

        prev_time = curr_time;
    }
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: msleep - 零延时 */
static void test_osal_msleep_zero(void)
{
    int32_t ret = OSAL_msleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: usleep - 零延时 */
static void test_osal_usleep_zero(void)
{
    int32_t ret = OSAL_usleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: sleep - 零延时 */
static void test_osal_sleep_zero(void)
{
    int32_t ret = OSAL_sleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: msleep - 零延时（替代TaskDelay） */
static void test_osal_task_delay_zero(void)
{
    int32_t ret = OSAL_msleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: 短延时精度 */
static void test_osal_short_delay_precision(void)
{
    uint64_t start_time, end_time, elapsed_us;

    /* 测试1毫秒延时 */
    start_time = get_time_in_micros();
    OSAL_msleep(1);
    end_time = get_time_in_micros();

    elapsed_us = end_time - start_time;
    /* 短延时允许较大误差（0.5-5ms） */
    TEST_ASSERT_TRUE(elapsed_us >= 500 && elapsed_us <= 5000);
}

/* 测试用例: 长延时精度 */
static void test_osal_long_delay_precision(void)
{
    uint64_t start_time, end_time, elapsed_ms;

    /* 测试500毫秒延时 */
    start_time = get_time_in_micros();
    OSAL_msleep(500);
    end_time = get_time_in_micros();

    elapsed_ms = (end_time - start_time) / 1000;
    /* 长延时精度应该更高（±50ms） */
    TEST_ASSERT_TRUE(elapsed_ms >= 450 && elapsed_ms <= 550);
}

/*===========================================================================
 * 性能测试
 *===========================================================================*/

/* 测试用例: 时间获取性能 */
static void test_osal_time_get_performance(void)
{
    uint64_t start_time, end_time;
    const int32_t iterations = 1000;
    OS_time_t time_struct;

    /* 测试GetLocalTime性能 */
    start_time = get_time_in_micros();
    int32_t i;

    for (i = 0; i < iterations; i++) {
        OSAL_get_local_time(&time_struct);
    }
    end_time = get_time_in_micros();

    /* 平均每次调用应该小于10微秒 */
    uint64_t avg_time = (end_time - start_time) / iterations;
    TEST_ASSERT_TRUE(avg_time < 10);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// OSAL时间操作测试
/* 延时函数 */
/* 时间获取 */
/* 边界条件 */
/* 性能测试 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
    { .name = "test_osal_msleep_success",
      .func = test_osal_msleep_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_usleep_success",
      .func = test_osal_usleep_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_sleep_success",
      .func = test_osal_sleep_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_nanosleep_success",
      .func = test_osal_nanosleep_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_task_delay_success",
      .func = test_osal_task_delay_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_get_local_time_success",
      .func = test_osal_get_local_time_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_get_local_time_null_pointer",
      .func = test_osal_get_local_time_null_pointer,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_get_tick_count",
      .func = test_osal_get_tick_count,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_time_monotonic",
      .func = test_osal_time_monotonic,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_msleep_zero",
      .func = test_osal_msleep_zero,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_usleep_zero",
      .func = test_osal_usleep_zero,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_sleep_zero",
      .func = test_osal_sleep_zero,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_task_delay_zero",
      .func = test_osal_task_delay_zero,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_short_delay_precision",
      .func = test_osal_short_delay_precision,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_long_delay_precision",
      .func = test_osal_long_delay_precision,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_time_get_performance",
      .func = test_osal_time_get_performance,
      .setup = NULL,
      .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
    .suite_name = "osal_time",
    .module_name = "osal_time",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = { .category = TEST_CATEGORY_UNIT,
                  .tags = TEST_TAG_FAST,
                  .timeout_ms = 100,
                  .description = "OSAL osal_time tests" }
};

/* 测试套件注册函数 */
__attribute__((constructor)) static void register_osal_time_tests(void)
{
    libutest_register_suite(&test_suite);
}
