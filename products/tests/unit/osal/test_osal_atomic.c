/************************************************************************
 * OSAL测试 - 原子操作测试
 ************************************************************************/

#include "test_framework.h"
#include "osal/osal.h"

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

    uint32_t i;


    for (i = 0; i < data->iterations; i++) {
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
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].counter = &counter;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_ThreadCreate(&threads[i],
                                       atomic_increment_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证计数器值 */
    uint32_t expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint32_t actual = OSAL_AtomicLoad(&counter);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_ThreadJoin(threads[i]);
    }
}

static void* atomic_cas_thread(void *arg)
{
    atomic_thread_data_t *data = (atomic_thread_data_t *)arg;

    uint32_t i;


    for (i = 0; i < data->iterations; i++) {
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
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
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

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_ThreadJoin(threads[i]);
    }
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
}

/*===========================================================================
 * 64位原子操作测试
 *===========================================================================*/

TEST_CASE(test_atomic64_init_and_load)
{
    osal_atomic_uint64_t atomic;

    /* 测试初始化为0 */
    OSAL_AtomicInit64(&atomic, 0);
    TEST_ASSERT_EQUAL(0ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试初始化为大值 */
    OSAL_AtomicInit64(&atomic, 0x123456789ABCDEF0ULL);
    TEST_ASSERT_EQUAL(0x123456789ABCDEF0ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试初始化为最大值 */
    OSAL_AtomicInit64(&atomic, UINT64_MAX);
    TEST_ASSERT_EQUAL(UINT64_MAX, OSAL_AtomicLoad64(&atomic));
}

TEST_CASE(test_atomic64_store)
{
    osal_atomic_uint64_t atomic;

    OSAL_AtomicInit64(&atomic, 0);

    /* 测试存储不同值 */
    OSAL_AtomicStore64(&atomic, 1000000000000ULL);
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_AtomicLoad64(&atomic));

    OSAL_AtomicStore64(&atomic, 0xFEDCBA9876543210ULL);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_AtomicLoad64(&atomic));
}

TEST_CASE(test_atomic64_increment_decrement)
{
    osal_atomic_uint64_t atomic;

    OSAL_AtomicInit64(&atomic, 1000000000000ULL);

    /* 测试自增 */
    TEST_ASSERT_EQUAL(1000000000001ULL, OSAL_AtomicIncrement64(&atomic));
    TEST_ASSERT_EQUAL(1000000000001ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试自减 */
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_AtomicDecrement64(&atomic));
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_AtomicLoad64(&atomic));
}

TEST_CASE(test_atomic64_fetch_add_sub)
{
    osal_atomic_uint64_t atomic;

    OSAL_AtomicInit64(&atomic, 1000000000000ULL);

    /* 测试fetch_add */
    uint64_t old_value = OSAL_AtomicFetchAdd64(&atomic, 500000000000ULL);
    TEST_ASSERT_EQUAL(1000000000000ULL, old_value);
    TEST_ASSERT_EQUAL(1500000000000ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试fetch_sub */
    old_value = OSAL_AtomicFetchSub64(&atomic, 300000000000ULL);
    TEST_ASSERT_EQUAL(1500000000000ULL, old_value);
    TEST_ASSERT_EQUAL(1200000000000ULL, OSAL_AtomicLoad64(&atomic));
}

TEST_CASE(test_atomic64_compare_exchange)
{
    osal_atomic_uint64_t atomic;

    OSAL_AtomicInit64(&atomic, 0x123456789ABCDEF0ULL);

    /* 测试CAS成功 */
    bool result = OSAL_AtomicCompareExchange64(&atomic, 0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试CAS失败 */
    result = OSAL_AtomicCompareExchange64(&atomic, 0x123456789ABCDEF0ULL, 0x1111111111111111ULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_AtomicLoad64(&atomic));
}

TEST_CASE(test_atomic64_overflow)
{
    osal_atomic_uint64_t atomic;

    /* 测试上溢 */
    OSAL_AtomicInit64(&atomic, UINT64_MAX);
    OSAL_AtomicIncrement64(&atomic);
    TEST_ASSERT_EQUAL(0ULL, OSAL_AtomicLoad64(&atomic));

    /* 测试下溢 */
    OSAL_AtomicInit64(&atomic, 0);
    OSAL_AtomicDecrement64(&atomic);
    TEST_ASSERT_EQUAL(UINT64_MAX, OSAL_AtomicLoad64(&atomic));
}

/* 64位原子时间戳多线程测试 */
typedef struct {
    osal_atomic_uint64_t *timestamp;
    uint32_t iterations;
} atomic64_thread_data_t;

static void* atomic64_increment_thread(void *arg)
{
    atomic64_thread_data_t *data = (atomic64_thread_data_t *)arg;

    uint32_t i;


    for (i = 0; i < data->iterations; i++) {
        OSAL_AtomicIncrement64(data->timestamp);
    }

    return NULL;
}

TEST_CASE(test_atomic64_multithread_timestamp)
{
    osal_atomic_uint64_t timestamp;
    osal_thread_t threads[THREAD_COUNT];
    atomic64_thread_data_t thread_data[THREAD_COUNT];

    OSAL_AtomicInit64(&timestamp, 0);

    /* 创建多个线程同时更新时间戳 */
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].timestamp = &timestamp;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_ThreadCreate(&threads[i],
                                       atomic64_increment_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证时间戳值 */
    uint64_t expected = (uint64_t)THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint64_t actual = OSAL_AtomicLoad64(&timestamp);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_ThreadJoin(threads[i]);
    }
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_osal_atomic, "OSAL")
    /* 32位原子操作测试 */
    TEST_CASE_REGISTER(test_atomic_init_and_load, "32-bit init and load")
    TEST_CASE_REGISTER(test_atomic_store, "32-bit store")
    TEST_CASE_REGISTER(test_atomic_increment, "32-bit increment")
    TEST_CASE_REGISTER(test_atomic_decrement, "32-bit decrement")
    TEST_CASE_REGISTER(test_atomic_fetch_add, "32-bit fetch add")
    TEST_CASE_REGISTER(test_atomic_fetch_sub, "32-bit fetch sub")
    TEST_CASE_REGISTER(test_atomic_compare_exchange, "32-bit compare exchange")
    TEST_CASE_REGISTER(test_atomic_multithread_increment, "32-bit multithread increment")
    TEST_CASE_REGISTER(test_atomic_multithread_cas, "32-bit multithread CAS")
    TEST_CASE_REGISTER(test_atomic_overflow, "32-bit overflow")
    TEST_CASE_REGISTER(test_atomic_boundary_values, "32-bit boundary values")

    /* 64位原子操作测试 */
    TEST_CASE_REGISTER(test_atomic64_init_and_load, "64-bit init and load")
    TEST_CASE_REGISTER(test_atomic64_store, "64-bit store")
    TEST_CASE_REGISTER(test_atomic64_increment_decrement, "64-bit increment/decrement")
    TEST_CASE_REGISTER(test_atomic64_fetch_add_sub, "64-bit fetch add/sub")
    TEST_CASE_REGISTER(test_atomic64_compare_exchange, "64-bit compare exchange")
    TEST_CASE_REGISTER(test_atomic64_overflow, "64-bit overflow")
    TEST_CASE_REGISTER(test_atomic64_multithread_timestamp, "64-bit multithread timestamp")
TEST_MODULE_END(test_osal_atomic, "OSAL")
