#include "test_framework.h"
/**
 * @file test_mutex.c
 * @brief OSAL互斥锁单元测试
 *
 * 使用新的libtest框架，测试自动注册
 */

#include "osal/osal.h"

static int32_t shared_counter = 0;

/* 测试用例1: 互斥锁创建成功 */
TEST_CASE(test_mutex_create_success)
{
    osal_mutex_t *mutex = NULL;

    int32_t ret = OSAL_MutexCreate(&mutex);

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_EQUAL(NULL, mutex);

    OSAL_MutexDelete(mutex);
}

/* 测试用例2: 互斥锁创建失败 - 空指针 */
TEST_CASE(test_mutex_create_nullpointer)
{
    int32_t ret = OSAL_MutexCreate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例3: 互斥锁加锁解锁 */
TEST_CASE(test_mutex_lockunlock_success)
{
    osal_mutex_t *mutex = NULL;
    OSAL_MutexCreate(&mutex);

    int32_t ret = OSAL_MutexLock(mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_MutexUnlock(mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_MutexDelete(mutex);
}

/* 测试用例4: 互斥锁加锁失败 - 空指针 */
TEST_CASE(test_mutex_lock_nullpointer)
{
    int32_t ret = OSAL_MutexLock(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例5: 互斥锁解锁失败 - 空指针 */
TEST_CASE(test_mutex_unlock_nullpointer)
{
    int32_t ret = OSAL_MutexUnlock(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例6: 互斥锁删除 */
TEST_CASE(test_mutex_delete_success)
{
    osal_mutex_t *mutex = NULL;
    OSAL_MutexCreate(&mutex);

    int32_t ret = OSAL_MutexDelete(mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例7: 互斥锁删除失败 - 空指针 */
TEST_CASE(test_mutex_delete_nullpointer)
{
    int32_t ret = OSAL_MutexDelete(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 线程函数 - 用于测试互斥锁保护 */
static void* increment_thread(void *arg)
{
    osal_mutex_t *mutex = (osal_mutex_t *)arg;

    int32_t i;


    for (i = 0; i < 1000; i++) {
        OSAL_MutexLock(mutex);
        shared_counter++;
        OSAL_MutexUnlock(mutex);
    }

    return NULL;
}

/* 测试用例8: 互斥锁保护共享资源 */
TEST_CASE(test_mutex_protect_shared_resource)
{
    shared_counter = 0;
    osal_mutex_t *mutex = NULL;
    OSAL_MutexCreate(&mutex);

    osal_thread_t thread1, thread2;

    /* 创建两个线程同时增加计数器 */
    OSAL_ThreadCreate(&thread1, increment_thread, mutex);
    OSAL_ThreadCreate(&thread2, increment_thread, mutex);

    /* 等待线程完成 */
    OSAL_ThreadJoin(thread1);
    OSAL_ThreadJoin(thread2);

    /* 验证计数器正确 */
    TEST_ASSERT_EQUAL(2000, shared_counter);

    OSAL_MutexDelete(mutex);
}

/* 注册测试套件 - 自动注册 */
TEST_MODULE_BEGIN(osal_mutex, "OSAL")
    TEST_CASE_REF(test_mutex_create_success)
    TEST_CASE_REF(test_mutex_create_nullpointer)
    TEST_CASE_REF(test_mutex_lockunlock_success)
    TEST_CASE_REF(test_mutex_lock_nullpointer)
    TEST_CASE_REF(test_mutex_unlock_nullpointer)
    TEST_CASE_REF(test_mutex_delete_success)
    TEST_CASE_REF(test_mutex_delete_nullpointer)
    TEST_CASE_REF(test_mutex_protect_shared_resource)
TEST_MODULE_END(osal_mutex, "OSAL")
