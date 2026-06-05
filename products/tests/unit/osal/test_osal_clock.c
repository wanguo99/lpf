#include "test_framework.h"
/**
 * @file test_osal_clock.c
 * @brief OSAL Clock模块单元测试
 */

#include "osal.h"

/*===========================================================================
 * OSAL_GetLocalTime 测试
 *===========================================================================*/

/* 测试用例: GetLocalTime - 成功获取本地时间 */
TEST_CASE(test_osal_get_local_time_success)
{
    OS_time_t time1, time2;
    int32_t ret;

    ret = OSAL_GetLocalTime(&time1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(time1.seconds > 0);
    TEST_ASSERT_TRUE(time1.microsecs < 1000000);

    /* 等待一小段时间后再次获取 */
    OSAL_msleep(10); /* 10ms */
    ret = OSAL_GetLocalTime(&time2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证时间递增 */
    TEST_ASSERT_TRUE(time2.seconds >= time1.seconds);
    if (time2.seconds == time1.seconds) {
        TEST_ASSERT_TRUE(time2.microsecs >= time1.microsecs);
    }
}

/* 测试用例: GetLocalTime - 空指针 */
TEST_CASE(test_osal_get_local_time_null_pointer)
{
    int32_t ret = OSAL_GetLocalTime(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * OSAL_SetLocalTime 测试
 *===========================================================================*/

/* 测试用例: SetLocalTime - 未实现 */
TEST_CASE(test_osal_set_local_time_not_implemented)
{
    OS_time_t time = {1234567890, 0};
    int32_t ret = OSAL_SetLocalTime(&time);
    TEST_ASSERT_EQUAL(OSAL_ERR_NOT_IMPLEMENTED, ret);
}

/*===========================================================================
 * OSAL_GetTickCount 测试
 *===========================================================================*/

/* 测试用例: GetTickCount - 单调递增 */
TEST_CASE(test_osal_get_tick_count_monotonic)
{
    uint32_t tick1, tick2;

    tick1 = OSAL_GetTickCount();
    TEST_ASSERT_TRUE(tick1 > 0);

    /* 等待一小段时间 */
    OSAL_msleep(50); /* 50ms */

    tick2 = OSAL_GetTickCount();
    TEST_ASSERT_TRUE(tick2 > tick1);

    /* 验证时间差大约是50ms（允许误差） */
    uint32_t diff = tick2 - tick1;
    TEST_ASSERT_TRUE(diff >= 45 && diff <= 100);
}

/* 测试用例: GetTickCount - 多次调用 */
TEST_CASE(test_osal_get_tick_count_multiple_calls)
{
    uint32_t tick1, tick2, tick3;

    tick1 = OSAL_GetTickCount();
    OSAL_msleep(10); /* 10ms */
    tick2 = OSAL_GetTickCount();
    OSAL_msleep(10); /* 10ms */
    tick3 = OSAL_GetTickCount();

    /* 验证单调递增 */
    TEST_ASSERT_TRUE(tick2 > tick1);
    TEST_ASSERT_TRUE(tick3 > tick2);
}

/*===========================================================================
 * OSAL_Milli2Ticks 测试
 *===========================================================================*/

/* 测试用例: Milli2Ticks - 成功转换 */
TEST_CASE(test_osal_milli2ticks_success)
{
    uint32_t ticks;
    int32_t ret;

    ret = OSAL_Milli2Ticks(100, &ticks);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(100, ticks);

    ret = OSAL_Milli2Ticks(0, &ticks);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, ticks);

    ret = OSAL_Milli2Ticks(1000, &ticks);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(1000, ticks);

    ret = OSAL_Milli2Ticks(0xFFFFFFFF, &ticks);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, ticks);
}

/* 测试用例: Milli2Ticks - 空指针 */
TEST_CASE(test_osal_milli2ticks_null_pointer)
{
    int32_t ret = OSAL_Milli2Ticks(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 综合测试
 *===========================================================================*/

/* 测试用例: 时间精度测试 */
TEST_CASE(test_osal_time_precision)
{
    OS_time_t time1, time2;
    uint32_t tick1, tick2;

    /* 同时获取两种时间 */
    OSAL_GetLocalTime(&time1);
    tick1 = OSAL_GetTickCount();

    OSAL_msleep(100); /* 100ms */

    OSAL_GetLocalTime(&time2);
    tick2 = OSAL_GetTickCount();

    /* 验证两种时间测量的一致性 */
    uint64_t time_diff_us = (time2.seconds - time1.seconds) * 1000000ULL +
                            (time2.microsecs - time1.microsecs);
    uint32_t tick_diff_ms = tick2 - tick1;

    /* 时间差应该大约是100ms（允许20%误差） */
    TEST_ASSERT_TRUE(time_diff_us >= 80000 && time_diff_us <= 120000);
    TEST_ASSERT_TRUE(tick_diff_ms >= 80 && tick_diff_ms <= 120);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_clock, "OSAL")
    /* OSAL_GetLocalTime 测试 */
    TEST_CASE_REF(test_osal_get_local_time_success)
    TEST_CASE_REF(test_osal_get_local_time_null_pointer)

    /* OSAL_SetLocalTime 测试 */
    TEST_CASE_REF(test_osal_set_local_time_not_implemented)

    /* OSAL_GetTickCount 测试 */
    TEST_CASE_REF(test_osal_get_tick_count_monotonic)
    TEST_CASE_REF(test_osal_get_tick_count_multiple_calls)

    /* OSAL_Milli2Ticks 测试 */
    TEST_CASE_REF(test_osal_milli2ticks_success)
    TEST_CASE_REF(test_osal_milli2ticks_null_pointer)

    /* 综合测试 */
    TEST_CASE_REF(test_osal_time_precision)
TEST_MODULE_END(test_osal_clock, "OSAL")
