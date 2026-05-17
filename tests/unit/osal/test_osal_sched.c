#include "test_framework.h"
/**
 * @file test_osal_sched.c
 * @brief OSAL实时调度单元测试
 */

#include "osal.h"
#include "sys/osal_sched.h"
#include "sys/osal_thread.h"
#include <unistd.h>

/*===========================================================================
 * 调度策略和优先级测试
 *===========================================================================*/

/* 测试用例: 设置和获取调度策略 - SCHED_FIFO */
TEST_CASE(test_osal_sched_set_policy_fifo)
{
    int32_t policy, priority;
    int32_t ret;

    /* 设置SCHED_FIFO策略，优先级50 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_FIFO, 50);

    /* 如果没有权限，跳过测试 */
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need root permission or CAP_SYS_NICE capability");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取调度策略 */
    ret = OSAL_SchedGetPolicy(0, &policy, &priority);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(OSAL_SCHED_FIFO, policy);
    TEST_ASSERT_EQUAL(50, priority);

    /* 恢复为普通调度策略 */
    OSAL_SchedSetPolicy(0, OSAL_SCHED_OTHER, 0);
}

/* 测试用例: 设置和获取调度策略 - SCHED_RR */
TEST_CASE(test_osal_sched_set_policy_rr)
{
    int32_t policy, priority;
    int32_t ret;

    /* 设置SCHED_RR策略，优先级30 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_RR, 30);

    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need root permission or CAP_SYS_NICE capability");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取调度策略 */
    ret = OSAL_SchedGetPolicy(0, &policy, &priority);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(OSAL_SCHED_RR, policy);
    TEST_ASSERT_EQUAL(30, priority);

    /* 恢复为普通调度策略 */
    OSAL_SchedSetPolicy(0, OSAL_SCHED_OTHER, 0);
}

/* 测试用例: 设置无效的调度策略 */
TEST_CASE(test_osal_sched_set_policy_invalid)
{
    int32_t ret;

    /* 无效的调度策略 */
    ret = OSAL_SchedSetPolicy(0, 999, 50);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/* 测试用例: 设置无效的优先级 */
TEST_CASE(test_osal_sched_set_priority_invalid)
{
    int32_t ret;

    /* 优先级过低 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_FIFO, 0);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 优先级过高 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_FIFO, 100);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/* 测试用例: 设置和获取优先级 */
TEST_CASE(test_osal_sched_set_priority)
{
    int32_t priority;
    int32_t ret;

    /* 先设置为实时调度策略 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_FIFO, 50);
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need root permission");
        return;
    }

    /* 修改优先级 */
    ret = OSAL_SchedSetPriority(0, 70);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取优先级 */
    ret = OSAL_SchedGetPriority(0, &priority);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(70, priority);

    /* 恢复为普通调度策略 */
    OSAL_SchedSetPolicy(0, OSAL_SCHED_OTHER, 0);
}

/*===========================================================================
 * CPU亲和性测试
 *===========================================================================*/

/* 测试用例: 设置和获取CPU亲和性 */
TEST_CASE(test_osal_sched_set_affinity)
{
    int32_t cpu_id;
    int32_t cpu_count;
    int32_t ret;

    /* 获取CPU数量 */
    cpu_count = OSAL_GetCPUCount();
    TEST_ASSERT_TRUE(cpu_count > 0);

    /* 如果只有一个CPU，跳过测试 */
    if (cpu_count == 1) {
        TEST_MESSAGE("SKIPPED: Only one CPU available");
        return;
    }

    /* 设置CPU亲和性到CPU 0 */
    ret = OSAL_SchedSetAffinity(0, 0);

    if (ret == OSAL_ERR_NOT_IMPLEMENTED) {
        TEST_MESSAGE("SKIPPED: CPU affinity not supported on this platform");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取CPU亲和性 */
    ret = OSAL_SchedGetAffinity(0, &cpu_id);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, cpu_id);

    /* 设置CPU亲和性到CPU 1 */
    ret = OSAL_SchedSetAffinity(0, 1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取CPU亲和性 */
    ret = OSAL_SchedGetAffinity(0, &cpu_id);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(1, cpu_id);
}

/* 测试用例: 设置无效的CPU亲和性 */
TEST_CASE(test_osal_sched_set_affinity_invalid)
{
    int32_t cpu_count;
    int32_t ret;

    cpu_count = OSAL_GetCPUCount();

    /* CPU ID过大 */
    ret = OSAL_SchedSetAffinity(0, cpu_count);

    if (ret == OSAL_ERR_NOT_IMPLEMENTED) {
        TEST_MESSAGE("SKIPPED: CPU affinity not supported on this platform");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* CPU ID为负数 */
    ret = OSAL_SchedSetAffinity(0, -1);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/* 测试用例: 获取CPU亲和性 - NULL指针 */
TEST_CASE(test_osal_sched_get_affinity_null)
{
    int32_t ret;

    ret = OSAL_SchedGetAffinity(0, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 内存锁定测试
 *===========================================================================*/

/* 测试用例: 锁定和解锁内存 */
TEST_CASE(test_osal_mem_lock)
{
    int32_t ret;

    /* 锁定当前内存 */
    ret = OSAL_MemLock(false);

    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need root permission or CAP_IPC_LOCK capability");
        return;
    }

    if (ret == OSAL_ERR_NOT_IMPLEMENTED || ret == OSAL_ERR_GENERIC) {
        TEST_MESSAGE("SKIPPED: Memory locking not fully supported on this platform");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 解锁内存 */
    ret = OSAL_MemUnlock();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 锁定所有内存 */
TEST_CASE(test_osal_mem_lock_all)
{
    int32_t ret;

    /* 锁定所有内存（包括未来分配的） */
    ret = OSAL_MemLock(true);

    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need root permission or CAP_IPC_LOCK capability");
        return;
    }

    if (ret == OSAL_ERR_NOT_IMPLEMENTED || ret == OSAL_ERR_GENERIC) {
        TEST_MESSAGE("SKIPPED: Memory locking not fully supported on this platform");
        return;
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 解锁内存 */
    ret = OSAL_MemUnlock();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 系统信息查询测试
 *===========================================================================*/

/* 测试用例: 获取CPU数量 */
TEST_CASE(test_osal_get_cpu_count)
{
    int32_t cpu_count;

    cpu_count = OSAL_GetCPUCount();
    TEST_ASSERT_TRUE(cpu_count > 0);
    TEST_ASSERT_TRUE(cpu_count <= 256);  /* 合理的上限 */
}

/* 测试用例: 获取调度策略的优先级范围 */
TEST_CASE(test_osal_sched_get_priority_range)
{
    int32_t min_priority, max_priority;

    /* SCHED_FIFO */
    min_priority = OSAL_SchedGetPriorityMin(OSAL_SCHED_FIFO);
    max_priority = OSAL_SchedGetPriorityMax(OSAL_SCHED_FIFO);
    TEST_ASSERT_TRUE(min_priority > 0);
    TEST_ASSERT_TRUE(max_priority > min_priority);

    /* SCHED_RR */
    min_priority = OSAL_SchedGetPriorityMin(OSAL_SCHED_RR);
    max_priority = OSAL_SchedGetPriorityMax(OSAL_SCHED_RR);
    TEST_ASSERT_TRUE(min_priority > 0);
    TEST_ASSERT_TRUE(max_priority > min_priority);

    /* SCHED_OTHER */
    min_priority = OSAL_SchedGetPriorityMin(OSAL_SCHED_OTHER);
    max_priority = OSAL_SchedGetPriorityMax(OSAL_SCHED_OTHER);
    TEST_ASSERT_TRUE(min_priority >= 0);
    TEST_ASSERT_TRUE(max_priority >= min_priority);
}

/*===========================================================================
 * 多线程调度测试
 *===========================================================================*/

static volatile bool thread_running = false;
static volatile int32_t thread_priority = 0;

static void *sched_test_thread(void *arg)
{
    int32_t priority;
    int32_t ret;

    (void)arg;  /* 未使用的参数 */
    thread_running = true;

    /* 获取当前优先级 */
    ret = OSAL_SchedGetPriority(0, &priority);
    if (ret == OSAL_SUCCESS) {
        thread_priority = priority;
    }

    /* 运行一段时间 */
    OSAL_msleep(100);  /* 100ms */

    thread_running = false;
    return NULL;
}

/* 测试用例: 为新线程设置调度策略 */
TEST_CASE(test_osal_sched_thread_policy)
{
    osal_thread_t thread;
    int32_t ret;
    int32_t max_priority;
    int32_t test_priority;

    /* 获取FIFO策略的最大优先级 */
    max_priority = OSAL_SchedGetPriorityMax(OSAL_SCHED_FIFO);
    /* 使用中等优先级进行测试 */
    test_priority = (OSAL_SchedGetPriorityMin(OSAL_SCHED_FIFO) + max_priority) / 2;

    /* 创建线程 */
    ret = OSAL_ThreadCreate(&thread, sched_test_thread, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 等待线程启动 */
    OSAL_msleep(10);  /* 10ms */

    /* 设置线程调度策略 */
    ret = OSAL_SchedSetPolicy(thread, OSAL_SCHED_FIFO, test_priority);
    if (ret == OSAL_ERR_PERMISSION) {
        /* 没有权限，等待线程结束 */
        OSAL_ThreadJoin(thread);
        TEST_MESSAGE("SKIPPED: Need root permission");
        return;
    }
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 等待线程结束 */
    ret = OSAL_ThreadJoin(thread);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证线程获取到的优先级 */
    TEST_ASSERT_EQUAL(test_priority, thread_priority);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_sched, "OSAL Realtime Scheduling Tests")
    /* 调度策略和优先级测试 */
    TEST_CASE_REGISTER(test_osal_sched_set_policy_fifo, "Set SCHED_FIFO policy")
    TEST_CASE_REGISTER(test_osal_sched_set_policy_rr, "Set SCHED_RR policy")
    TEST_CASE_REGISTER(test_osal_sched_set_policy_invalid, "Set invalid policy")
    TEST_CASE_REGISTER(test_osal_sched_set_priority_invalid, "Set invalid priority")
    TEST_CASE_REGISTER(test_osal_sched_set_priority, "Set and get priority")

    /* CPU亲和性测试 */
    TEST_CASE_REGISTER(test_osal_sched_set_affinity, "Set CPU affinity")
    TEST_CASE_REGISTER(test_osal_sched_set_affinity_invalid, "Set invalid CPU affinity")
    TEST_CASE_REGISTER(test_osal_sched_get_affinity_null, "Get affinity with NULL pointer")

    /* 内存锁定测试 */
    TEST_CASE_REGISTER(test_osal_mem_lock, "Lock and unlock memory")
    TEST_CASE_REGISTER(test_osal_mem_lock_all, "Lock all memory")

    /* 系统信息查询测试 */
    TEST_CASE_REGISTER(test_osal_get_cpu_count, "Get CPU count")
    TEST_CASE_REGISTER(test_osal_sched_get_priority_range, "Get priority range")

    /* 多线程调度测试 */
    TEST_CASE_REGISTER(test_osal_sched_thread_policy, "Set thread scheduling policy")
TEST_MODULE_END(test_osal_sched, "OSAL")
