/************************************************************************
 * OSAL进程管理单元测试
 ************************************************************************/

#include "osal.h"

#include <test_framework/test_framework.h>

/*===========================================================================
 * 测试用例
 *===========================================================================*/

static void _test_process_getpid(void)
{
	osal_pid_t pid1 = osal_getpid();
	osal_pid_t pid2 = osal_getpid();

	/* 同一进程的PID应该相同 */
	TEST_ASSERT_EQUAL(pid1, pid2);

	/* PID应该是正数 */
	TEST_ASSERT_TRUE(pid1 > 0);
}

static void _test_process_kill_invalid_pid(void)
{
	/* 尝试向不存在的大PID发送信号应该失败 */
	int32_t ret = osal_kill(999999, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}

static void _test_process_kill_signal_zero(void)
{
	/* 信号0用于检查进程是否存在，不会实际发送信号 */
	osal_pid_t pid = osal_getpid();
	int32_t ret = osal_kill(pid, 0);

	/* 向自己发送信号0应该成功 */
	TEST_ASSERT_EQUAL(0, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_process_getpid",
	  .func = _test_process_getpid,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_process_kill_invalid_pid",
	  .func = _test_process_kill_invalid_pid,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_process_kill_signal_zero",
	  .func = _test_process_kill_signal_zero,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_process",
	.module_name = "osal_process",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "OSAL osal_process tests" }
};

/* 测试套件注册函数 */
void register_osal_process_tests(void)
{
	libutest_register_suite(&test_suite);
}
