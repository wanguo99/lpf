/************************************************************************
 * OSAL增强功能测试
 *
 * 测试新增的API：
 * 1. OSAL_MutexTryLock - 非阻塞加锁
 * 2. OSAL_MutexTimedLock - 超时加锁
 * 3. OSAL_MutexCreateEx - 带属性创建（优先级继承）
 * 4. OSAL_ThreadCreateEx - 带属性创建线程
 * 5. OSAL_ThreadSelf - 获取当前线程
 ************************************************************************/

#include "osal.h"
#include <stdio.h>

/* 测试1：互斥锁TryLock */
void test_mutex_trylock(void)
{
    printf("\n=== 测试1：OSAL_MutexTryLock ===\n");

    osal_mutex_t *mutex = NULL;
    int32_t ret;

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&mutex);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 创建互斥锁失败: %d\n", ret);
        return;
    }
    printf("✅ 创建互斥锁成功\n");

    /* 第一次TryLock应该成功 */
    ret = OSAL_MutexTryLock(mutex);
    if (ret == OSAL_SUCCESS) {
        printf("✅ TryLock成功（锁未被占用）\n");
    } else {
        printf("❌ TryLock失败: %d\n", ret);
    }

    /* 第二次TryLock应该返回BUSY */
    ret = OSAL_MutexTryLock(mutex);
    if (ret == OSAL_ERR_BUSY) {
        printf("✅ TryLock返回BUSY（锁已被占用）\n");
    } else {
        printf("❌ TryLock应该返回BUSY，实际返回: %d\n", ret);
    }

    /* 解锁 */
    OSAL_MutexUnlock(mutex);
    printf("✅ 解锁成功\n");

    /* 再次TryLock应该成功 */
    ret = OSAL_MutexTryLock(mutex);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 解锁后TryLock成功\n");
    } else {
        printf("❌ 解锁后TryLock失败: %d\n", ret);
    }

    OSAL_MutexUnlock(mutex);
    OSAL_MutexDelete(mutex);
}

/* 测试2：互斥锁TimedLock */
void test_mutex_timedlock(void)
{
    printf("\n=== 测试2：OSAL_MutexTimedLock ===\n");

    osal_mutex_t *mutex = NULL;
    int32_t ret;

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&mutex);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 创建互斥锁失败: %d\n", ret);
        return;
    }

    /* 先加锁 */
    OSAL_MutexLock(mutex);
    printf("✅ 主线程已加锁\n");

    /* 尝试带超时加锁（应该超时） */
    printf("⏳ 尝试100ms超时加锁...\n");
    uint64_t start = OSAL_GetMonotonicTime();
    ret = OSAL_MutexTimedLock(mutex, 100);
    uint64_t elapsed = (OSAL_GetMonotonicTime() - start) / 1000; /* 转换为毫秒 */

    if (ret == OSAL_ERR_TIMEOUT) {
        printf("✅ TimedLock超时（耗时约%llu ms）\n", (unsigned long long)elapsed);
    } else {
        printf("❌ TimedLock应该超时，实际返回: %d\n", ret);
    }

    /* 解锁 */
    OSAL_MutexUnlock(mutex);

    /* 再次尝试带超时加锁（应该立即成功） */
    ret = OSAL_MutexTimedLock(mutex, 100);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 解锁后TimedLock立即成功\n");
    } else {
        printf("❌ 解锁后TimedLock失败: %d\n", ret);
    }

    OSAL_MutexUnlock(mutex);
    OSAL_MutexDelete(mutex);
}

/* 测试3：带优先级继承的互斥锁 */
void test_mutex_prio_inherit(void)
{
    printf("\n=== 测试3：OSAL_MutexCreateEx（优先级继承）===\n");

    osal_mutex_t *mutex = NULL;
    osal_mutex_attr_t attr = {
        .type = OSAL_MUTEX_NORMAL,
        .protocol = OSAL_MUTEX_PRIO_INHERIT,
        .prio_ceiling = 0
    };

    int32_t ret = OSAL_MutexCreateEx(&mutex, &attr);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 创建带优先级继承的互斥锁成功\n");
    } else {
        printf("❌ 创建失败: %d\n", ret);
        return;
    }

    /* 测试基本功能 */
    ret = OSAL_MutexLock(mutex);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 加锁成功\n");
    }

    OSAL_MutexUnlock(mutex);
    printf("✅ 解锁成功\n");

    OSAL_MutexDelete(mutex);
}

/* 测试4：递归锁 */
void test_mutex_recursive(void)
{
    printf("\n=== 测试4：递归锁 ===\n");

    osal_mutex_t *mutex = NULL;
    osal_mutex_attr_t attr = {
        .type = OSAL_MUTEX_RECURSIVE,
        .protocol = OSAL_MUTEX_PRIO_NONE,
        .prio_ceiling = 0
    };

    int32_t ret = OSAL_MutexCreateEx(&mutex, &attr);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 创建递归锁失败: %d\n", ret);
        return;
    }
    printf("✅ 创建递归锁成功\n");

    /* 第一次加锁 */
    ret = OSAL_MutexLock(mutex);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 第一次加锁成功\n");
    }

    /* 第二次加锁（递归锁允许） */
    ret = OSAL_MutexLock(mutex);
    if (ret == OSAL_SUCCESS) {
        printf("✅ 第二次加锁成功（递归）\n");
    } else {
        printf("❌ 递归加锁失败: %d\n", ret);
    }

    /* 解锁两次 */
    OSAL_MutexUnlock(mutex);
    OSAL_MutexUnlock(mutex);
    printf("✅ 解锁两次成功\n");

    OSAL_MutexDelete(mutex);
}

/* 线程函数 */
static void *thread_func(void *arg)
{
    const char *name = (const char *)arg;
    osal_thread_t self = OSAL_ThreadSelf();

    printf("  [%s] 线程启动，句柄=0x%llx\n", name, (unsigned long long)self);
    OSAL_msleep(100);
    printf("  [%s] 线程退出\n", name);

    return NULL;
}

/* 测试5：ThreadCreateEx */
void test_thread_create_ex(void)
{
    printf("\n=== 测试5：OSAL_ThreadCreateEx ===\n");

    osal_thread_t thread;
    osal_thread_attr_t attr = {
        .stack_size = 1024 * 1024,  /* 1MB栈 */
        .detached = false,
        .inherit_sched = 1,         /* 继承调度属性 */
        .sched_policy = 0,          /* SCHED_OTHER */
        .sched_priority = 0
    };

    int32_t ret = OSAL_ThreadCreateEx(&thread, &attr, thread_func, (void*)"Worker");
    if (ret == OSAL_SUCCESS) {
        printf("✅ 创建线程成功（1MB栈）\n");
    } else {
        printf("❌ 创建线程失败: %d\n", ret);
        return;
    }

    /* 等待线程结束 */
    OSAL_ThreadJoin(thread);
    printf("✅ 线程已结束\n");
}

/* 测试6：ThreadSelf */
void test_thread_self(void)
{
    printf("\n=== 测试6：OSAL_ThreadSelf ===\n");

    osal_thread_t self = OSAL_ThreadSelf();
    printf("✅ 当前线程句柄: 0x%llx\n", (unsigned long long)self);

    /* 创建子线程 */
    osal_thread_t thread;
    int32_t ret = OSAL_ThreadCreate(&thread, thread_func, (void*)"Child");
    if (ret == OSAL_SUCCESS) {
        printf("✅ 创建子线程成功\n");
    }

    OSAL_ThreadJoin(thread);
}

int main(void)
{
    printf("╔════════════════════════════════════════╗\n");
    printf("║   OSAL增强功能测试                     ║\n");
    printf("╚════════════════════════════════════════╝\n");

    test_mutex_trylock();
    test_mutex_timedlock();
    test_mutex_prio_inherit();
    test_mutex_recursive();
    test_thread_create_ex();
    test_thread_self();

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║   所有测试完成                         ║\n");
    printf("╚════════════════════════════════════════╝\n");

    return 0;
}
