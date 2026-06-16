#include <test_framework/test_framework.h>
/**
 * @file test_osal_clock.c
 * @brief OSAL Clock模块单元测试
 */

#include "osal.h"

/*===========================================================================
 * OSAL_get_local_time 测试
 *===========================================================================*/

/* 测试用例: GetLocalTime - 成功获取本地时间 */
static void test_osal_get_local_time_success(void)
{
    OS_time_t time1, time2;
    int32_t ret;

    ret = OSAL_get_local_time(&time1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(time1.seconds > 0);
    TEST_ASSERT_TRUE(time1.microsecs < 1000000);

    /* 等待一小段时间后再次获取 */
    OSAL_msleep(10); /* 10ms */
    ret = OSAL_get_local_time(&time2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证时间递增 */
    TEST_ASSERT_TRUE(time2.seconds >= time1.seconds);
    if (time2.seconds == time1.seconds) {
        TEST_ASSERT_TRUE(time2.microsecs >= time1.microsecs);
    }
}

/* 测试用例: GetLocalTime - 空指针 */
static void test_osal_get_local_time_null_pointer(void)
{
    int32_t ret = OSAL_get_local_time(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * OSAL_set_local_time 测试
 *===========================================================================*/

/* 测试用例: SetLocalTime - 未实现 */
static void test_osal_set_local_time_not_implemented(void)
{
    OS_time_t time = {1234567890, 0};
    int32_t ret = OSAL_set_local_time(&time);
    TEST_ASSERT_EQUAL(OSAL_ERR_NOT_IMPLEMENTED, ret);
}

/*===========================================================================
 * OSAL_get_tick_count 测试
 *===========================================================================*/

/* 测试用例: GetTickCount - 单调递增 */
static void test_osal_get_tick_count_monotonic(void)
{
    uint32_t tick1, tick2;

    tick1 = OSAL_get_tick_count();
    TEST_ASSERT_TRUE(tick1 > 0);

    /* 等待一小段时间 */
    OSAL_msleep(50); /* 50ms */

    tick2 = OSAL_get_tick_count();
    TEST_ASSERT_TRUE(tick2 > tick1);

    /* 验证时间差大约是50ms（允许误差） */
    uint32_t diff = tick2 - tick1;
    TEST_ASSERT_TRUE(diff >= 45 && diff <= 100);
}

/* 测试用例: GetTickCount - 多次调用 */
static void test_osal_get_tick_count_multiple_calls(void)
{
    uint32_t tick1, tick2, tick3;

    tick1 = OSAL_get_tick_count();
    OSAL_msleep(10); /* 10ms */
    tick2 = OSAL_get_tick_count();
    OSAL_msleep(10); /* 10ms */
    tick3 = OSAL_get_tick_count();

    /* 验证单调递增 */
    TEST_ASSERT_TRUE(tick2 > tick1);
    TEST_ASSERT_TRUE(tick3 > tick2);
}

/*===========================================================================
 * 综合测试
 *===========================================================================*/

/* 测试用例: 时间精度测试 */
static void test_osal_time_precision(void)
{
    OS_time_t time1, time2;
    uint32_t tick1, tick2;

    /* 同时获取两种时间 */
    OSAL_get_local_time(&time1);
    tick1 = OSAL_get_tick_count();

    OSAL_msleep(100); /* 100ms */

    OSAL_get_local_time(&time2);
    tick2 = OSAL_get_tick_count();

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

/* OSAL_get_local_time 测试 */
    /* OSAL_set_local_time 测试 */
    /* OSAL_get_tick_count 测试 */
    /* 综合测试 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_osal_get_local_time_success",
		.func = test_osal_get_local_time_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_get_local_time_null_pointer",
		.func = test_osal_get_local_time_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_set_local_time_not_implemented",
		.func = test_osal_set_local_time_not_implemented,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_get_tick_count_monotonic",
		.func = test_osal_get_tick_count_monotonic,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_get_tick_count_multiple_calls",
		.func = test_osal_get_tick_count_multiple_calls,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_time_precision",
		.func = test_osal_time_precision,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_clock",
	.module_name = "osal_clock",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "OSAL osal_clock tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_osal_clock_tests(void)
{
	libutest_register_suite(&test_suite);
}
