#include "test_framework.h"
/**
 * @file test_osal_time.c
 * @brief OSAL时间操作单元测试
 */

#include "osal.h"

/* 辅助函数：获取当前时间（微秒） */
static uint64_t get_time_in_micros(void)
{
    return (uint64_t)OSAL_GetMonotonicTime();
}

/*===========================================================================
 * 延时函数测试
 *===========================================================================*/

/* 测试用例: msleep - 毫秒延时 */
TEST_CASE(test_osal_msleep_success)
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
TEST_CASE(test_osal_usleep_success)
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
TEST_CASE(test_osal_sleep_success)
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
TEST_CASE(test_osal_nanosleep_success)
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
TEST_CASE(test_osal_task_delay_success)
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
TEST_CASE(test_osal_get_local_time_success)
{
    OS_time_t time1, time2;
    int32_t ret;

    /* 获取第一次时间 */
    ret = OSAL_GetLocalTime(&time1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(time1.seconds > 0);

    /* 短暂延时 */
    OSAL_msleep(10);

    /* 获取第二次时间 */
    ret = OSAL_GetLocalTime(&time2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证时间递增 */
    TEST_ASSERT_TRUE(time2.seconds >= time1.seconds);
    if (time2.seconds == time1.seconds) {
        TEST_ASSERT_TRUE(time2.microsecs > time1.microsecs);
    }
}

/* 测试用例: GetLocalTime - 空指针 */
TEST_CASE(test_osal_get_local_time_null_pointer)
{
    int32_t ret = OSAL_GetLocalTime(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: GetTickCount - 获取系统滴答 */
TEST_CASE(test_osal_get_tick_count)
{
    uint32_t tick1, tick2;

    /* 获取第一次滴答 */
    tick1 = OSAL_GetTickCount();

    /* 短暂延时 */
    OSAL_msleep(50);

    /* 获取第二次滴答 */
    tick2 = OSAL_GetTickCount();

    /* 验证滴答递增（允许±20ms误差） */
    uint32_t diff = tick2 - tick1;
    TEST_ASSERT_TRUE(diff >= 30 && diff <= 70);
}

/* 测试用例: 时间单调性 */
TEST_CASE(test_osal_time_monotonic)
{
    OS_time_t prev_time, curr_time;

    OSAL_GetLocalTime(&prev_time);

    /* 连续获取10次时间，验证单调递增 */
    int32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_usleep(1000);  /* 1毫秒 */
        OSAL_GetLocalTime(&curr_time);

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
TEST_CASE(test_osal_msleep_zero)
{
    int32_t ret = OSAL_msleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: usleep - 零延时 */
TEST_CASE(test_osal_usleep_zero)
{
    int32_t ret = OSAL_usleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: sleep - 零延时 */
TEST_CASE(test_osal_sleep_zero)
{
    int32_t ret = OSAL_sleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: msleep - 零延时（替代TaskDelay） */
TEST_CASE(test_osal_task_delay_zero)
{
    int32_t ret = OSAL_msleep(0);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: 短延时精度 */
TEST_CASE(test_osal_short_delay_precision)
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
TEST_CASE(test_osal_long_delay_precision)
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
TEST_CASE(test_osal_time_get_performance)
{
    uint64_t start_time, end_time;
    const int32_t iterations = 1000;
    OS_time_t time_struct;

    /* 测试GetLocalTime性能 */
    start_time = get_time_in_micros();
    int32_t i;

    for (i = 0; i < iterations; i++) {
        OSAL_GetLocalTime(&time_struct);
    }
    end_time = get_time_in_micros();

    /* 平均每次调用应该小于10微秒 */
    uint64_t avg_time = (end_time - start_time) / iterations;
    TEST_ASSERT_TRUE(avg_time < 10);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_time, "OSAL")
    // OSAL时间操作测试
    /* 延时函数 */
    TEST_CASE_REF(test_osal_msleep_success)
    TEST_CASE_REF(test_osal_usleep_success)
    TEST_CASE_REF(test_osal_sleep_success)
    TEST_CASE_REF(test_osal_nanosleep_success)
    TEST_CASE_REF(test_osal_task_delay_success)

    /* 时间获取 */
    TEST_CASE_REF(test_osal_get_local_time_success)
    TEST_CASE_REF(test_osal_get_local_time_null_pointer)
    TEST_CASE_REF(test_osal_get_tick_count)
    TEST_CASE_REF(test_osal_time_monotonic)

    /* 边界条件 */
    TEST_CASE_REF(test_osal_msleep_zero)
    TEST_CASE_REF(test_osal_usleep_zero)
    TEST_CASE_REF(test_osal_sleep_zero)
    TEST_CASE_REF(test_osal_task_delay_zero)
    TEST_CASE_REF(test_osal_short_delay_precision)
    TEST_CASE_REF(test_osal_long_delay_precision)

    /* 性能测试 */
    TEST_CASE_REF(test_osal_time_get_performance)
TEST_MODULE_END(test_osal_time, "OSAL")
