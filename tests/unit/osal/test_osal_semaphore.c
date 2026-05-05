/**
 * @file test_osal_semaphore.c
 * @brief OSAL信号量单元测试
 *
 * 使用新的libtest框架，测试自动注册
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"

static int32_t shared_counter = 0;

/* 测试用例1: 信号量创建成功 */
TEST_CASE(test_semaphore_create_success)
{
    osal_semaphore_t *sem = NULL;

    int32_t ret = OSAL_SemaphoreCreate(&sem, 1);

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_EQUAL(NULL, sem);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例2: 信号量创建失败 - 空指针 */
TEST_CASE(test_semaphore_create_nullpointer)
{
    int32_t ret = OSAL_SemaphoreCreate(NULL, 1);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例3: 信号量创建失败 - 初始值过大 */
TEST_CASE(test_semaphore_create_invalid_value)
{
    osal_semaphore_t *sem = NULL;
    int32_t ret = OSAL_SemaphoreCreate(&sem, (uint32_t)INT32_MAX + 1);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_SEM_VALUE, ret);
}

/* 测试用例4: 信号量等待和释放 */
TEST_CASE(test_semaphore_wait_post_success)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 1);

    int32_t ret = OSAL_SemaphoreWait(sem);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_SemaphorePost(sem);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例5: 信号量等待失败 - 空指针 */
TEST_CASE(test_semaphore_wait_nullpointer)
{
    int32_t ret = OSAL_SemaphoreWait(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例6: 信号量释放失败 - 空指针 */
TEST_CASE(test_semaphore_post_nullpointer)
{
    int32_t ret = OSAL_SemaphorePost(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例7: 信号量超时等待 - 超时 */
TEST_CASE(test_semaphore_timedwait_timeout)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 0);

    int32_t ret = OSAL_SemaphoreTimedWait(sem, 100);
    TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例8: 信号量超时等待 - 成功 */
TEST_CASE(test_semaphore_timedwait_success)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 1);

    int32_t ret = OSAL_SemaphoreTimedWait(sem, 100);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例9: 信号量非阻塞等待 - 超时 */
TEST_CASE(test_semaphore_trywait_timeout)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 0);

    int32_t ret = OSAL_SemaphoreTimedWait(sem, 0);
    TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例10: 信号量非阻塞等待 - 成功 */
TEST_CASE(test_semaphore_trywait_success)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 1);

    int32_t ret = OSAL_SemaphoreTimedWait(sem, 0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_SemaphoreDelete(sem);
}

/* 测试用例11: 信号量删除 */
TEST_CASE(test_semaphore_delete_success)
{
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 1);

    int32_t ret = OSAL_SemaphoreDelete(sem);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例12: 信号量删除失败 - 空指针 */
TEST_CASE(test_semaphore_delete_nullpointer)
{
    int32_t ret = OSAL_SemaphoreDelete(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 生产者线程 */
static void* producer_thread(void *arg)
{
    osal_semaphore_t *sem = (osal_semaphore_t *)arg;

    for (int32_t i = 0; i < 10; i++) {
        OSAL_msleep(10);
        shared_counter++;
        OSAL_SemaphorePost(sem);
    }

    return NULL;
}

/* 消费者线程 */
static void* consumer_thread(void *arg)
{
    osal_semaphore_t *sem = (osal_semaphore_t *)arg;

    for (int32_t i = 0; i < 10; i++) {
        OSAL_SemaphoreWait(sem);
        shared_counter--;
    }

    return NULL;
}

/* 测试用例13: 信号量生产者-消费者模式 */
TEST_CASE(test_semaphore_producer_consumer)
{
    shared_counter = 0;
    osal_semaphore_t *sem = NULL;
    OSAL_SemaphoreCreate(&sem, 0);

    osal_thread_t producer, consumer;

    /* 创建生产者和消费者线程 */
    OSAL_pthread_create(&producer, NULL, producer_thread, sem);
    OSAL_pthread_create(&consumer, NULL, consumer_thread, sem);

    /* 等待线程完成 */
    OSAL_pthread_join(producer, NULL);
    OSAL_pthread_join(consumer, NULL);

    /* 验证计数器归零 */
    TEST_ASSERT_EQUAL(0, shared_counter);

    OSAL_SemaphoreDelete(sem);
}

/* 注册测试套件 - 自动注册 */
TEST_SUITE_BEGIN(osal_semaphore, "osal", "OSAL")
    TEST_CASE_REF(test_semaphore_create_success)
    TEST_CASE_REF(test_semaphore_create_nullpointer)
    TEST_CASE_REF(test_semaphore_create_invalid_value)
    TEST_CASE_REF(test_semaphore_wait_post_success)
    TEST_CASE_REF(test_semaphore_wait_nullpointer)
    TEST_CASE_REF(test_semaphore_post_nullpointer)
    TEST_CASE_REF(test_semaphore_timedwait_timeout)
    TEST_CASE_REF(test_semaphore_timedwait_success)
    TEST_CASE_REF(test_semaphore_trywait_timeout)
    TEST_CASE_REF(test_semaphore_trywait_success)
    TEST_CASE_REF(test_semaphore_delete_success)
    TEST_CASE_REF(test_semaphore_delete_nullpointer)
    TEST_CASE_REF(test_semaphore_producer_consumer)
TEST_SUITE_END(osal_semaphore, "osal", "OSAL")
