/************************************************************************
 * OSAL信号处理单元测试
 ************************************************************************/

#include "osal.h"
#include <test/test_framework.h>

#include <signal.h>

/*===========================================================================
 * 测试辅助变量
 *===========================================================================*/

static volatile int32_t signal_received = 0;
static volatile int32_t signal_number = 0;

/*===========================================================================
 * 信号处理函数
 *===========================================================================*/

static void test_signal_handler(int sig)
{
    signal_received = 1;
    signal_number = sig;
}

/*===========================================================================
 * 信号基础测试
 *===========================================================================*/

static void test_signal_raise_and_handle(void)
{
    signal_received = 0;
    signal_number = 0;

    /* 注册信号处理函数 */
    int32_t ret = OSAL_signal(SIGUSR1, test_signal_handler);
    TEST_ASSERT_EQUAL(0, ret);

    /* 发送信号给自己 */
    ret = OSAL_raise(SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待信号处理完成 */
    OSAL_usleep(10000);

    /* 验证信号被接收 */
    TEST_ASSERT_EQUAL(1, signal_received);
    TEST_ASSERT_EQUAL(SIGUSR1, signal_number);

    /* 恢复默认处理 */
    OSAL_signal(SIGUSR1, SIG_DFL);
}

static void test_signal_kill_self(void)
{
    signal_received = 0;
    signal_number = 0;

    /* 注册信号处理函数 */
    OSAL_signal(SIGUSR2, test_signal_handler);

    /* 使用kill发送信号给自己 */
    osal_pid_t pid = OSAL_getpid();
    int32_t ret = OSAL_kill(pid, SIGUSR2);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待信号处理完成 */
    OSAL_usleep(10000);

    /* 验证信号被接收 */
    TEST_ASSERT_EQUAL(1, signal_received);
    TEST_ASSERT_EQUAL(SIGUSR2, signal_number);

    /* 恢复默认处理 */
    OSAL_signal(SIGUSR2, SIG_DFL);
}

static void test_signal_ignore(void)
{
    signal_received = 0;

    /* 设置信号为忽略 */
    int32_t ret = OSAL_signal(SIGUSR1, SIG_IGN);
    TEST_ASSERT_EQUAL(0, ret);

    /* 发送信号 */
    ret = OSAL_raise(SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待一段时间 */
    OSAL_usleep(10000);

    /* 信号应该被忽略，处理函数不应被调用 */
    TEST_ASSERT_EQUAL(0, signal_received);

    /* 恢复默认处理 */
    OSAL_signal(SIGUSR1, SIG_DFL);
}

/*===========================================================================
 * 信号集操作测试
 *===========================================================================*/

static void test_sigset_empty_and_add(void)
{
    sigset_t set;

    /* 初始化为空集 */
    int32_t ret = OSAL_sigemptyset(&set);
    TEST_ASSERT_EQUAL(0, ret);

    /* 验证信号不在集合中 */
    ret = OSAL_sigismember(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 添加信号到集合 */
    ret = OSAL_sigaddset(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 验证信号在集合中 */
    ret = OSAL_sigismember(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(1, ret);
}

static void test_sigset_fill_and_delete(void)
{
    sigset_t set;

    /* 初始化为全集 */
    int32_t ret = OSAL_sigfillset(&set);
    TEST_ASSERT_EQUAL(0, ret);

    /* 验证信号在集合中 */
    ret = OSAL_sigismember(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(1, ret);

    /* 从集合中删除信号 */
    ret = OSAL_sigdelset(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 验证信号不在集合中 */
    ret = OSAL_sigismember(&set, SIGUSR1);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_sigprocmask_block_unblock(void)
{
    sigset_t set, oldset;

    /* 创建信号集 */
    OSAL_sigemptyset(&set);
    OSAL_sigaddset(&set, SIGUSR1);

    /* 阻塞信号 */
    int32_t ret = OSAL_sigprocmask(SIG_BLOCK, &set, &oldset);
    TEST_ASSERT_EQUAL(0, ret);

    /* 解除阻塞 */
    ret = OSAL_sigprocmask(SIG_UNBLOCK, &set, NULL);
    TEST_ASSERT_EQUAL(0, ret);
}

/*===========================================================================
 * sigaction测试
 *===========================================================================*/

static void test_sigaction_basic(void)
{
    struct sigaction act, oldact;

    signal_received = 0;
    signal_number = 0;

    /* 设置信号动作 */
    OSAL_memset(&act, 0, sizeof(act));
    act.sa_handler = test_signal_handler;
    OSAL_sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    int32_t ret = OSAL_sigaction(SIGUSR1, &act, &oldact);
    TEST_ASSERT_EQUAL(0, ret);

    /* 发送信号 */
    OSAL_raise(SIGUSR1);
    OSAL_usleep(10000);

    /* 验证 */
    TEST_ASSERT_EQUAL(1, signal_received);
    TEST_ASSERT_EQUAL(SIGUSR1, signal_number);

    /* 恢复旧的动作 */
    OSAL_sigaction(SIGUSR1, &oldact, NULL);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_signal_raise_and_handle",
        .func = test_signal_raise_and_handle,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_signal_kill_self",
        .func = test_signal_kill_self,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_signal_ignore",
        .func = test_signal_ignore,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_sigset_empty_and_add",
        .func = test_sigset_empty_and_add,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_sigset_fill_and_delete",
        .func = test_sigset_fill_and_delete,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_sigprocmask_block_unblock",
        .func = test_sigprocmask_block_unblock,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_sigaction_basic",
        .func = test_sigaction_basic,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "osal_signal",
    .module_name = "osal_signal",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 500,
        .description = "OSAL signal handling tests"
    }
};

__attribute__((constructor))
static void register_osal_signal_tests(void)
{
    libutest_register_suite(&test_suite);
}
