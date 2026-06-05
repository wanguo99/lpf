#include "test_framework.h"
/**
 * @file test_osal_mutex.c
 * @brief OSAL Mutex Unit Tests
 *
 * Tests OSAL mutex operations using the new simplified test framework.
 * Demonstrates the new registration system with metadata support.
 *
 * Test Coverage:
 * - Mutex creation and destruction
 * - Mutex locking and unlocking
 * - Null pointer handling
 * - Multi-threaded synchronization
 */

#include "osal.h"

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

/**
 * Test suite registration using new simplified system
 *
 * This demonstrates the new TEST_SUITE_REGISTER macro which:
 * - Eliminates TEST_MODULE_BEGIN/END boilerplate
 * - Adds test metadata (category, tags, timeout, description)
 * - Uses TEST_CASE_ENTRY for cleaner test case array definition
 * - Auto-registers at program startup via constructor attribute
 *
 * Metadata:
 * - Category: UNIT (isolated module testing)
 * - Tags: FAST (<100ms execution time)
 * - Timeout: 100ms (expected maximum runtime)
 * - Description: Human-readable summary of what this suite tests
 */

/* Define test case array using simplified TEST_CASE_ENTRY macro */
static const test_case_t osal_mutex_cases[] = {
    TEST_CASE_ENTRY(test_mutex_create_success),
    TEST_CASE_ENTRY(test_mutex_create_nullpointer),
    TEST_CASE_ENTRY(test_mutex_lockunlock_success),
    TEST_CASE_ENTRY(test_mutex_lock_nullpointer),
    TEST_CASE_ENTRY(test_mutex_unlock_nullpointer),
    TEST_CASE_ENTRY(test_mutex_delete_success),
    TEST_CASE_ENTRY(test_mutex_delete_nullpointer),
    TEST_CASE_ENTRY(test_mutex_protect_shared_resource),
};

/*
 * Register test suite with metadata
 *
 * Parameters:
 * - suite_id: osal_mutex (matches test_osal_mutex.c naming convention)
 * - layer_name: "OSAL" (OSAL layer)
 * - cases_array: osal_mutex_cases (test case array defined above)
 * - category: TEST_CATEGORY_UNIT (unit test)
 * - tags: TEST_TAG_FAST (fast execution, <100ms)
 * - timeout: 100 (expected maximum runtime in milliseconds)
 * - description: Human-readable summary of what this suite tests
 */
TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases, TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "OSAL mutex operations (create, lock, unlock, destroy)")
