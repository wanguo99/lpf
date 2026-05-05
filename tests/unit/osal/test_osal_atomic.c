/************************************************************************
 * OSAL测试 - 原子操作测试
 ************************************************************************/

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"
#include <pthread.h>

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

TEST_CASE(test_atomic_init_and_load)
{
    osal_atomic_uint32_t atomic;

    /* 测试初始化为0 */
    OSAL_AtomicInit(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));

    /* 测试初始化为非0值 */
    OSAL_AtomicInit(&atomic, 100);
    TEST_ASSERT_EQUAL(100, OSAL_AtomicLoad(&atomic));

    /* 测试初始化为最大值 */
    OSAL_AtomicInit(&atomic, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_AtomicLoad(&atomic));
}

TEST_CASE(test_atomic_store)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 0);

    /* 测试存储不同值 */
    OSAL_AtomicStore(&atomic, 42);
    TEST_ASSERT_EQUAL(42, OSAL_AtomicLoad(&atomic));

    OSAL_AtomicStore(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));

    OSAL_AtomicStore(&atomic, 0xDEADBEEF);
    TEST_ASSERT_EQUAL(0xDEADBEEF, OSAL_AtomicLoad(&atomic));
}

TEST_CASE(test_atomic_increment)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 0);

    /* 测试自增 */
    TEST_ASSERT_EQUAL(1, OSAL_AtomicIncrement(&atomic));
    TEST_ASSERT_EQUAL(1, OSAL_AtomicLoad(&atomic));

    TEST_ASSERT_EQUAL(2, OSAL_AtomicIncrement(&atomic));
    TEST_ASSERT_EQUAL(2, OSAL_AtomicLoad(&atomic));

    /* 测试从非0值开始自增 */
    OSAL_AtomicInit(&atomic, 100);
    TEST_ASSERT_EQUAL(101, OSAL_AtomicIncrement(&atomic));
    TEST_ASSERT_EQUAL(101, OSAL_AtomicLoad(&atomic));
}

TEST_CASE(test_atomic_decrement)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 10);

    /* 测试自减 */
    TEST_ASSERT_EQUAL(9, OSAL_AtomicDecrement(&atomic));
    TEST_ASSERT_EQUAL(9, OSAL_AtomicLoad(&atomic));

    TEST_ASSERT_EQUAL(8, OSAL_AtomicDecrement(&atomic));
    TEST_ASSERT_EQUAL(8, OSAL_AtomicLoad(&atomic));

    /* 测试减到0 */
    OSAL_AtomicInit(&atomic, 1);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicDecrement(&atomic));
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));
}

TEST_CASE(test_atomic_fetch_add)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 10);

    /* 测试fetch_add返回旧值 */
    uint32_t old_value = OSAL_AtomicFetchAdd(&atomic, 5);
    TEST_ASSERT_EQUAL(10, old_value);
    TEST_ASSERT_EQUAL(15, OSAL_AtomicLoad(&atomic));

    /* 测试加0 */
    old_value = OSAL_AtomicFetchAdd(&atomic, 0);
    TEST_ASSERT_EQUAL(15, old_value);
    TEST_ASSERT_EQUAL(15, OSAL_AtomicLoad(&atomic));

    /* 测试加大值 */
    old_value = OSAL_AtomicFetchAdd(&atomic, 1000);
    TEST_ASSERT_EQUAL(15, old_value);
    TEST_ASSERT_EQUAL(1015, OSAL_AtomicLoad(&atomic));
}

TEST_CASE(test_atomic_fetch_sub)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 100);

    /* 测试fetch_sub返回旧值 */
    uint32_t old_value = OSAL_AtomicFetchSub(&atomic, 30);
    TEST_ASSERT_EQUAL(100, old_value);
    TEST_ASSERT_EQUAL(70, OSAL_AtomicLoad(&atomic));

    /* 测试减0 */
    old_value = OSAL_AtomicFetchSub(&atomic, 0);
    TEST_ASSERT_EQUAL(70, old_value);
    TEST_ASSERT_EQUAL(70, OSAL_AtomicLoad(&atomic));

    /* 测试减到0 */
    old_value = OSAL_AtomicFetchSub(&atomic, 70);
    TEST_ASSERT_EQUAL(70, old_value);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));

    return;
}

TEST_CASE(test_atomic_compare_exchange)
{
    osal_atomic_uint32_t atomic;

    OSAL_AtomicInit(&atomic, 42);

    /* 测试CAS成功 */
    bool result = OSAL_AtomicCompareExchange(&atomic, 42, 100);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, OSAL_AtomicLoad(&atomic));

    /* 测试CAS失败（期望值不匹配） */
    result = OSAL_AtomicCompareExchange(&atomic, 42, 200);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(100, OSAL_AtomicLoad(&atomic));  /* 值不变 */

    /* 测试CAS成功（交换为0） */
    result = OSAL_AtomicCompareExchange(&atomic, 100, 0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));

    return;
}

/*===========================================================================
 * 多线程测试
 *===========================================================================*/

#define THREAD_COUNT 4
#define ITERATIONS_PER_THREAD 10000

typedef struct {
    osal_atomic_uint32_t *counter;
    uint32_t iterations;
} atomic_thread_data_t;

static void* atomic_increment_thread(void *arg)
{
    atomic_thread_data_t *data = (atomic_thread_data_t *)arg;

    for (uint32_t i = 0; i < data->iterations; i++) {
        OSAL_AtomicIncrement(data->counter);
    }

    return NULL;
}

TEST_CASE(test_atomic_multithread_increment)
{
    osal_atomic_uint32_t counter;
    osal_thread_t threads[THREAD_COUNT];
    atomic_thread_data_t thread_data[THREAD_COUNT];

    OSAL_AtomicInit(&counter, 0);

    /* 创建多个线程同时自增 */
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].counter = &counter;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_ThreadCreate(&threads[i],
                                       atomic_increment_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);  /* 确保所有线程完成 */

    /* 验证计数器值 */
    uint32_t expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint32_t actual = OSAL_AtomicLoad(&counter);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */
    for (int i = 0; i < THREAD_COUNT; i++) {
        OSAL_ThreadJoin(threads[i]);
    }

    return;
}

static void* atomic_cas_thread(void *arg)
{
    atomic_thread_data_t *data = (atomic_thread_data_t *)arg;

    for (uint32_t i = 0; i < data->iterations; i++) {
        uint32_t old_value, new_value;
        bool success;

        do {
            old_value = OSAL_AtomicLoad(data->counter);
            new_value = old_value + 1;
            success = OSAL_AtomicCompareExchange(data->counter, old_value, new_value);
        } while (!success);
    }

    return NULL;
}

TEST_CASE(test_atomic_multithread_cas)
{
    osal_atomic_uint32_t counter;
    osal_thread_t threads[THREAD_COUNT];
    atomic_thread_data_t thread_data[THREAD_COUNT];

    OSAL_AtomicInit(&counter, 0);

    /* 创建多个线程使用CAS自增 */
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].counter = &counter;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_ThreadCreate(&threads[i],
                                       atomic_cas_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证计数器值 */
    uint32_t expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint32_t actual = OSAL_AtomicLoad(&counter);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */
    for (int i = 0; i < THREAD_COUNT; i++) {
        OSAL_ThreadJoin(threads[i]);
    }

    return;
}

/*===========================================================================
 * 边界测试
 *===========================================================================*/

TEST_CASE(test_atomic_overflow)
{
    osal_atomic_uint32_t atomic;

    /* 测试上溢 */
    OSAL_AtomicInit(&atomic, 0xFFFFFFFF);
    OSAL_AtomicIncrement(&atomic);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));  /* 溢出回0 */

    /* 测试下溢 */
    OSAL_AtomicInit(&atomic, 0);
    OSAL_AtomicDecrement(&atomic);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_AtomicLoad(&atomic));  /* 下溢到最大值 */

    return;
}

TEST_CASE(test_atomic_boundary_values)
{
    osal_atomic_uint32_t atomic;

    /* 测试最小值 */
    OSAL_AtomicInit(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_AtomicLoad(&atomic));

    /* 测试最大值 */
    OSAL_AtomicInit(&atomic, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_AtomicLoad(&atomic));

    /* 测试中间值 */
    OSAL_AtomicInit(&atomic, 0x80000000);
    TEST_ASSERT_EQUAL(0x80000000, OSAL_AtomicLoad(&atomic));

    return;
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_SUITE_BEGIN(osal_atomic, "osal_atomic", "OSAL")
    TEST_CASE_REF(test_atomic_init_and_load)
    TEST_CASE_REF(test_atomic_store)
    TEST_CASE_REF(test_atomic_increment)
    TEST_CASE_REF(test_atomic_decrement)
    TEST_CASE_REF(test_atomic_fetch_add)
    TEST_CASE_REF(test_atomic_fetch_sub)
    TEST_CASE_REF(test_atomic_compare_exchange)
    TEST_CASE_REF(test_atomic_multithread_increment)
    TEST_CASE_REF(test_atomic_multithread_cas)
    TEST_CASE_REF(test_atomic_overflow)
    TEST_CASE_REF(test_atomic_boundary_values)
TEST_SUITE_END(osal_atomic, "test_osal_atomic", "OSAL")
