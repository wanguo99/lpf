/************************************************************************
 * OSAL线程管理单元测试
 ************************************************************************/

#include "test_framework.h"
#include "sys/osal_thread.h"
#include "sys/osal_time.h"
#include <string.h>

/*===========================================================================
 * 测试辅助变量
 *===========================================================================*/

static volatile int thread_counter = 0;

/*===========================================================================
 * 测试线程函数
 *===========================================================================*/

static void* simple_thread_func(void *arg)
{
    int *value = (int *)arg;
    if (value) {
        *value = 42;
    }
    return (void *)123;
}

static void* counter_thread_func(void *arg)
{
    (void)arg;
    thread_counter++;
    OSAL_msleep(100);
    thread_counter++;
    return NULL;
}

static void* sleep_thread_func(void *arg)
{
    int sleep_ms = *(int *)arg;
    OSAL_msleep(sleep_ms);
    return NULL;
}

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_thread_create_join)
{
    osal_thread_t thread;
    int value = 0;
    void *retval = NULL;

    int32_t ret = OSAL_pthread_create(&thread, NULL, simple_thread_func, &value);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_pthread_join(thread, &retval);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(42, value);
    TEST_ASSERT_EQUAL(123, (int)(intptr_t)retval);
}

TEST_CASE(test_thread_create_simplified)
{
    osal_thread_t thread;
    int value = 0;

    int32_t ret = OSAL_ThreadCreate(&thread, simple_thread_func, &value);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_ThreadJoin(thread);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(42, value);
}

TEST_CASE(test_thread_multiple_threads)
{
    osal_thread_t threads[5];
    int values[5] = {0};

    /* 创建多个线程 */
    for (int i = 0; i < 5; i++) {
        int32_t ret = OSAL_ThreadCreate(&threads[i], simple_thread_func, &values[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (int i = 0; i < 5; i++) {
        int32_t ret = OSAL_ThreadJoin(threads[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(42, values[i]);
    }
}

TEST_CASE(test_thread_counter)
{
    osal_thread_t thread;
    thread_counter = 0;

    int32_t ret = OSAL_ThreadCreate(&thread, counter_thread_func, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 等待线程完成 */
    ret = OSAL_ThreadJoin(thread);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, thread_counter);
}

TEST_CASE(test_thread_concurrent_counter)
{
    osal_thread_t threads[10];
    thread_counter = 0;

    /* 创建10个线程，每个增加计数器2次 */
    for (int i = 0; i < 10; i++) {
        int32_t ret = OSAL_ThreadCreate(&threads[i], counter_thread_func, NULL);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (int i = 0; i < 10; i++) {
        int32_t ret = OSAL_ThreadJoin(threads[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 验证计数器值（注意：没有同步，可能有竞态） */
    TEST_ASSERT_EQUAL(20, thread_counter);
}

TEST_CASE(test_thread_null_params)
{
    osal_thread_t thread;

    /* NULL线程指针 */
    int32_t ret = OSAL_ThreadCreate(NULL, simple_thread_func, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL函数指针 */
    ret = OSAL_ThreadCreate(&thread, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_thread_with_null_arg)
{
    osal_thread_t thread;

    /* 线程函数可以接受NULL参数 */
    int32_t ret = OSAL_ThreadCreate(&thread, simple_thread_func, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_ThreadJoin(thread);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_thread_timing)
{
    osal_thread_t threads[3];
    int sleep_times[3] = {50, 100, 150};
    uint64_t start_time, end_time;

    start_time = OSAL_GetTickCount();

    /* 创建3个不同睡眠时间的线程 */
    for (int i = 0; i < 3; i++) {
        int32_t ret = OSAL_ThreadCreate(&threads[i], sleep_thread_func, &sleep_times[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (int i = 0; i < 3; i++) {
        int32_t ret = OSAL_ThreadJoin(threads[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    end_time = OSAL_GetTickCount();
    uint64_t elapsed = end_time - start_time;

    /* 总时间应该接近最长的睡眠时间（150ms），因为线程并发执行 */
    TEST_ASSERT_TRUE(elapsed >= 150 && elapsed < 250);
}

TEST_CASE(test_thread_sequential_execution)
{
    osal_thread_t thread1, thread2;
    int value1 = 0, value2 = 0;

    /* 顺序创建和等待线程 */
    int32_t ret = OSAL_ThreadCreate(&thread1, simple_thread_func, &value1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_ThreadJoin(thread1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(42, value1);

    ret = OSAL_ThreadCreate(&thread2, simple_thread_func, &value2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_ThreadJoin(thread2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(42, value2);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_thread, "OSAL")
    TEST_CASE_REGISTER(test_thread_create_join, "Thread create and join")
    TEST_CASE_REGISTER(test_thread_create_simplified, "Thread simplified API")
    TEST_CASE_REGISTER(test_thread_multiple_threads, "Multiple threads")
    TEST_CASE_REGISTER(test_thread_counter, "Thread counter")
    TEST_CASE_REGISTER(test_thread_concurrent_counter, "Concurrent counter")
    TEST_CASE_REGISTER(test_thread_null_params, "NULL parameters")
    TEST_CASE_REGISTER(test_thread_with_null_arg, "Thread with NULL arg")
    TEST_CASE_REGISTER(test_thread_timing, "Thread timing")
    TEST_CASE_REGISTER(test_thread_sequential_execution, "Sequential execution")
TEST_MODULE_END(test_osal_thread, "OSAL")
