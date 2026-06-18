/************************************************************************
 * OSAL调度器单元测试
 ************************************************************************/

#include "osal.h"
#include <test_framework/test_framework.h>

/*===========================================================================
 * 调度策略测试
 *===========================================================================*/

static void _test_sched_set_get_policy(void)
{
	osal_thread_t thread = osal_pthread_self();
	int32_t policy;
	int32_t priority;

	/* 获取当前调度策略 */
	int32_t ret = osal_pthread_getschedparam(thread, &policy, &priority);
	TEST_ASSERT_EQUAL(0, ret);

	/* 策略应该是有效值 */
	TEST_ASSERT_TRUE(policy == OSAL_SCHED_OTHER || policy == OSAL_SCHED_FIFO ||
					 policy == OSAL_SCHED_RR);
}

static void _test_sched_priority_range(void)
{
	/* 获取 OSAL_SCHED_OTHER 的优先级范围 */
	int32_t min = osal_sched_get_priority_min(OSAL_SCHED_OTHER);
	int32_t max = osal_sched_get_priority_max(OSAL_SCHED_OTHER);

	TEST_ASSERT_TRUE(min >= 0);
	TEST_ASSERT_TRUE(max >= min);

	/* 获取 OSAL_SCHED_FIFO 的优先级范围 */
	min = osal_sched_get_priority_min(OSAL_SCHED_FIFO);
	max = osal_sched_get_priority_max(OSAL_SCHED_FIFO);

	TEST_ASSERT_TRUE(min > 0);
	TEST_ASSERT_TRUE(max > min);

	/* 获取 OSAL_SCHED_RR 的优先级范围 */
	min = osal_sched_get_priority_min(OSAL_SCHED_RR);
	max = osal_sched_get_priority_max(OSAL_SCHED_RR);

	TEST_ASSERT_TRUE(min > 0);
	TEST_ASSERT_TRUE(max > min);
}

/*===========================================================================
 * 线程优先级测试
 *===========================================================================*/

static void _test_sched_set_priority_other(void)
{
	osal_thread_t thread = osal_pthread_self();
	int32_t policy;
	int32_t priority;

	/* 获取当前参数 */
	int32_t ret = osal_pthread_getschedparam(thread, &policy, &priority);
	TEST_ASSERT_EQUAL(0, ret);

	/* 设置新的优先级（OSAL_SCHED_OTHER 通常只支持 0） */
	ret = osal_pthread_setschedparam(thread, OSAL_SCHED_OTHER, 0);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证设置成功 */
	ret = osal_pthread_getschedparam(thread, &policy, &priority);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(OSAL_SCHED_OTHER, policy);
}

/*===========================================================================
 * CPU亲和性测试
 *===========================================================================*/

static void _test_sched_cpu_affinity(void)
{
	int32_t cpu_id = 0;

	/* 设置当前线程的CPU亲和性到CPU 0 */
	int32_t ret = osal_pthread_setaffinity_np(osal_pthread_self(), cpu_id);

	/* 如果系统支持亲和性设置，应该成功 */
	/* 如果不支持（如单核系统），可能返回错误，但这不是测试失败 */
	if (ret == 0) {
		int32_t get_cpu_id;

		/* 获取CPU亲和性 */
		ret = osal_pthread_getaffinity_np(osal_pthread_self(), &get_cpu_id);
		TEST_ASSERT_EQUAL(0, ret);

		/* 验证CPU 0被设置 */
		TEST_ASSERT_EQUAL(cpu_id, get_cpu_id);
	}
}

static void _test_sched_process_cpu_affinity(void)
{
	int32_t cpu_id = 0;

	/* 设置当前进程的CPU亲和性到CPU 0 */
	int32_t ret = osal_sched_setaffinity(0, cpu_id);

	/* 如果系统支持亲和性设置，应该成功 */
	if (ret == 0) {
		int32_t get_cpu_id;

		/* 获取CPU亲和性 */
		ret = osal_sched_getaffinity(0, &get_cpu_id);
		TEST_ASSERT_EQUAL(0, ret);

		/* 验证CPU 0被设置 */
		TEST_ASSERT_EQUAL(cpu_id, get_cpu_id);
	}
}

static void _test_sched_process_affinity_with_pid(void)
{
	int32_t cpu_id = 0;
	osal_pid_t pid = osal_getpid();

	/* 使用显式PID设置进程的CPU亲和性 */
	int32_t ret = osal_sched_setaffinity(pid, cpu_id);

	/* 如果系统支持亲和性设置，应该成功 */
	if (ret == 0) {
		int32_t get_cpu_id;

		/* 获取CPU亲和性 */
		ret = osal_sched_getaffinity(pid, &get_cpu_id);
		TEST_ASSERT_EQUAL(0, ret);

		/* 验证CPU 0被设置 */
		TEST_ASSERT_EQUAL(cpu_id, get_cpu_id);
	}
}

/*===========================================================================
 * 线程属性调度测试
 *===========================================================================*/

static void _test_sched_attr_set_policy(void)
{
	osal_threadattr_t attr;

	/* 初始化属性 */
	int32_t ret = osal_pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL(0, ret);

	/* 设置调度策略 */
	ret = osal_pthread_attr_setschedpolicy(&attr, OSAL_SCHED_OTHER);
	TEST_ASSERT_EQUAL(0, ret);

	/* 清理 */
	osal_pthread_attr_destroy(&attr);
}

static void _test_sched_attr_set_param(void)
{
	osal_threadattr_t attr;
	osal_sched_param_t param;

	/* 初始化属性 */
	int32_t ret = osal_pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL(0, ret);

	/* 设置调度参数 */
	param.sched_priority = 0;
	ret = osal_pthread_attr_setschedparam(&attr, &param);
	TEST_ASSERT_EQUAL(0, ret);

	/* 清理 */
	osal_pthread_attr_destroy(&attr);
}

/*===========================================================================
 * yield测试
 *===========================================================================*/

static void _test_sched_yield(void)
{
	/* 主动让出CPU */
	int32_t ret = osal_sched_yield();
	TEST_ASSERT_EQUAL(0, ret);

	/* 多次yield应该都成功 */
	for (int32_t i = 0; i < 10; i++) {
		ret = osal_sched_yield();
		TEST_ASSERT_EQUAL(0, ret);
	}
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_sched_set_get_policy",
	  .func = _test_sched_set_get_policy,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_priority_range",
	  .func = _test_sched_priority_range,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_set_priority_other",
	  .func = _test_sched_set_priority_other,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_cpu_affinity",
	  .func = _test_sched_cpu_affinity,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_process_cpu_affinity",
	  .func = _test_sched_process_cpu_affinity,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_process_affinity_with_pid",
	  .func = _test_sched_process_affinity_with_pid,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_attr_set_policy",
	  .func = _test_sched_attr_set_policy,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_attr_set_param",
	  .func = _test_sched_attr_set_param,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sched_yield",
	  .func = _test_sched_yield,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_sched",
	.module_name = "osal_sched",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 500,
				  .description = "OSAL scheduler tests" }
};

void register_osal_sched_tests(void)
{
	libutest_register_suite(&test_suite);
}
