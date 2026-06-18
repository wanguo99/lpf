/************************************************************************
 * OSAL信号处理单元测试
 ************************************************************************/

#include "osal.h"
#include <test_framework/test_framework.h>

#include <signal.h>

/*===========================================================================
 * 测试辅助变量
 *===========================================================================*/

static volatile int32_t signal_received = 0;
static volatile int32_t signal_number = 0;

/*===========================================================================
 * 信号处理函数
 *===========================================================================*/

static void _test_signal_handler(int sig)
{
	signal_received = 1;
	signal_number = sig;
}

/*===========================================================================
 * 信号基础测试
 *===========================================================================*/

static void _test_signal_raise_and_handle(void)
{
	signal_received = 0;
	signal_number = 0;

	/* 注册信号处理函数 */
	int32_t ret = osal_signal(SIGUSR1, _test_signal_handler);
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送信号给自己 */
	ret = osal_raise(SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待信号处理完成 */
	osal_usleep(10000);

	/* 验证信号被接收 */
	TEST_ASSERT_EQUAL(1, signal_received);
	TEST_ASSERT_EQUAL(SIGUSR1, signal_number);

	/* 恢复默认处理 */
	osal_signal(SIGUSR1, SIG_DFL);
}

static void _test_signal_kill_self(void)
{
	signal_received = 0;
	signal_number = 0;

	/* 注册信号处理函数 */
	osal_signal(SIGUSR2, _test_signal_handler);

	/* 使用kill发送信号给自己 */
	osal_pid_t pid = osal_getpid();
	int32_t ret = osal_kill(pid, SIGUSR2);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待信号处理完成 */
	osal_usleep(10000);

	/* 验证信号被接收 */
	TEST_ASSERT_EQUAL(1, signal_received);
	TEST_ASSERT_EQUAL(SIGUSR2, signal_number);

	/* 恢复默认处理 */
	osal_signal(SIGUSR2, SIG_DFL);
}

static void _test_signal_ignore(void)
{
	signal_received = 0;

	/* 设置信号为忽略 */
	int32_t ret = osal_signal(SIGUSR1, SIG_IGN);
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送信号 */
	ret = osal_raise(SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待一段时间 */
	osal_usleep(10000);

	/* 信号应该被忽略，处理函数不应被调用 */
	TEST_ASSERT_EQUAL(0, signal_received);

	/* 恢复默认处理 */
	osal_signal(SIGUSR1, SIG_DFL);
}

/*===========================================================================
 * 信号集操作测试
 *===========================================================================*/

static void _test_sigset_empty_and_add(void)
{
	sigset_t set;

	/* 初始化为空集 */
	int32_t ret = osal_sigemptyset(&set);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证信号不在集合中 */
	ret = osal_sigismember(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);

	/* 添加信号到集合 */
	ret = osal_sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证信号在集合中 */
	ret = osal_sigismember(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(1, ret);
}

static void _test_sigset_fill_and_delete(void)
{
	sigset_t set;

	/* 初始化为全集 */
	int32_t ret = osal_sigfillset(&set);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证信号在集合中 */
	ret = osal_sigismember(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(1, ret);

	/* 从集合中删除信号 */
	ret = osal_sigdelset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证信号不在集合中 */
	ret = osal_sigismember(&set, SIGUSR1);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_sigprocmask_block_unblock(void)
{
	sigset_t set, oldset;

	/* 创建信号集 */
	osal_sigemptyset(&set);
	osal_sigaddset(&set, SIGUSR1);

	/* 阻塞信号 */
	int32_t ret = osal_sigprocmask(SIG_BLOCK, &set, &oldset);
	TEST_ASSERT_EQUAL(0, ret);

	/* 解除阻塞 */
	ret = osal_sigprocmask(SIG_UNBLOCK, &set, NULL);
	TEST_ASSERT_EQUAL(0, ret);
}

/*===========================================================================
 * sigaction测试
 *===========================================================================*/

static void _test_sigaction_basic(void)
{
	struct sigaction act, oldact;

	signal_received = 0;
	signal_number = 0;

	/* 设置信号动作 */
	osal_memset(&act, 0, sizeof(act));
	act.sa_handler = _test_signal_handler;
	osal_sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	int32_t ret = osal_sigaction(SIGUSR1, &act, &oldact);
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送信号 */
	osal_raise(SIGUSR1);
	osal_usleep(10000);

	/* 验证 */
	TEST_ASSERT_EQUAL(1, signal_received);
	TEST_ASSERT_EQUAL(SIGUSR1, signal_number);

	/* 恢复旧的动作 */
	osal_sigaction(SIGUSR1, &oldact, NULL);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_signal_raise_and_handle",
	  .func = _test_signal_raise_and_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_signal_kill_self",
	  .func = _test_signal_kill_self,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_signal_ignore",
	  .func = _test_signal_ignore,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sigset_empty_and_add",
	  .func = _test_sigset_empty_and_add,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sigset_fill_and_delete",
	  .func = _test_sigset_fill_and_delete,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sigprocmask_block_unblock",
	  .func = _test_sigprocmask_block_unblock,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_sigaction_basic",
	  .func = _test_sigaction_basic,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_signal",
	.module_name = "osal_signal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 500,
				  .description = "OSAL signal handling tests" }
};

void register_osal_signal_tests(void)
{
	libutest_register_suite(&test_suite);
}
