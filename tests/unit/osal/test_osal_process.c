/************************************************************************
 * OSAL进程管理单元测试
 ************************************************************************/

#include "test_framework.h"
#include "sys/osal_process.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_process_getpid)
{
    int32_t pid1 = OSAL_Getpid();
    int32_t pid2 = OSAL_Getpid();

    /* 同一进程的PID应该相同 */
    TEST_ASSERT_EQUAL(pid1, pid2);

    /* PID应该是正数 */
    TEST_ASSERT_TRUE(pid1 > 0);
}

TEST_CASE(test_process_kill_invalid_pid)
{
    /* 尝试向不存在的大PID发送信号应该失败 */
    int32_t ret = OSAL_Kill(999999, 0);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

TEST_CASE(test_process_kill_signal_zero)
{
    /* 信号0用于检查进程是否存在，不会实际发送信号 */
    int32_t pid = OSAL_Getpid();
    int32_t ret = OSAL_Kill(pid, 0);

    /* 向自己发送信号0应该成功 */
    TEST_ASSERT_EQUAL(0, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_process, "OSAL")
    TEST_CASE_REGISTER(test_process_getpid, "Get process ID")
    TEST_CASE_REGISTER(test_process_kill_invalid_pid, "Kill invalid PID")
    TEST_CASE_REGISTER(test_process_kill_signal_zero, "Kill with signal 0")
TEST_MODULE_END(test_osal_process, "OSAL")
