/************************************************************************
 * OSAL测试 - 原子操作测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

static void test_atomic_init_and_load(void)
{
    osal_atomic_uint32_t atomic;

    /* 测试初始化为0 */
    OSAL_atomic_init(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));

    /* 测试初始化为非0值 */
    OSAL_atomic_init(&atomic, 100);
    TEST_ASSERT_EQUAL(100, OSAL_atomic_load(&atomic));

    /* 测试初始化为最大值 */
    OSAL_atomic_init(&atomic, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_atomic_load(&atomic));
}

static void test_atomic_store(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 0);

    /* 测试存储不同值 */
    OSAL_atomic_store(&atomic, 42);
    TEST_ASSERT_EQUAL(42, OSAL_atomic_load(&atomic));

    OSAL_atomic_store(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));

    OSAL_atomic_store(&atomic, 0xDEADBEEF);
    TEST_ASSERT_EQUAL(0xDEADBEEF, OSAL_atomic_load(&atomic));
}

static void test_atomic_increment(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 0);

    /* 测试自增 */
    TEST_ASSERT_EQUAL(1, OSAL_atomic_inc(&atomic));
    TEST_ASSERT_EQUAL(1, OSAL_atomic_load(&atomic));

    TEST_ASSERT_EQUAL(2, OSAL_atomic_inc(&atomic));
    TEST_ASSERT_EQUAL(2, OSAL_atomic_load(&atomic));

    /* 测试从非0值开始自增 */
    OSAL_atomic_init(&atomic, 100);
    TEST_ASSERT_EQUAL(101, OSAL_atomic_inc(&atomic));
    TEST_ASSERT_EQUAL(101, OSAL_atomic_load(&atomic));
}

static void test_atomic_decrement(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 10);

    /* 测试自减 */
    TEST_ASSERT_EQUAL(9, OSAL_atomic_dec(&atomic));
    TEST_ASSERT_EQUAL(9, OSAL_atomic_load(&atomic));

    TEST_ASSERT_EQUAL(8, OSAL_atomic_dec(&atomic));
    TEST_ASSERT_EQUAL(8, OSAL_atomic_load(&atomic));

    /* 测试减到0 */
    OSAL_atomic_init(&atomic, 1);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_dec(&atomic));
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));
}

static void test_atomic_fetch_add(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 10);

    /* 测试fetch_add返回旧值 */
    uint32_t old_value = OSAL_atomic_fetch_add(&atomic, 5);
    TEST_ASSERT_EQUAL(10, old_value);
    TEST_ASSERT_EQUAL(15, OSAL_atomic_load(&atomic));

    /* 测试加0 */
    old_value = OSAL_atomic_fetch_add(&atomic, 0);
    TEST_ASSERT_EQUAL(15, old_value);
    TEST_ASSERT_EQUAL(15, OSAL_atomic_load(&atomic));

    /* 测试加大值 */
    old_value = OSAL_atomic_fetch_add(&atomic, 1000);
    TEST_ASSERT_EQUAL(15, old_value);
    TEST_ASSERT_EQUAL(1015, OSAL_atomic_load(&atomic));
}

static void test_atomic_fetch_sub(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 100);

    /* 测试fetch_sub返回旧值 */
    uint32_t old_value = OSAL_atomic_fetch_sub(&atomic, 30);
    TEST_ASSERT_EQUAL(100, old_value);
    TEST_ASSERT_EQUAL(70, OSAL_atomic_load(&atomic));

    /* 测试减0 */
    old_value = OSAL_atomic_fetch_sub(&atomic, 0);
    TEST_ASSERT_EQUAL(70, old_value);
    TEST_ASSERT_EQUAL(70, OSAL_atomic_load(&atomic));

    /* 测试减到0 */
    old_value = OSAL_atomic_fetch_sub(&atomic, 70);
    TEST_ASSERT_EQUAL(70, old_value);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));
}

static void test_atomic_compare_exchange(void)
{
    osal_atomic_uint32_t atomic;

    OSAL_atomic_init(&atomic, 42);

    /* 测试CAS成功 */
    uint32_t expected = 42;
    bool result = OSAL_atomic_compare_exchange_strong(&atomic, &expected, 100);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, OSAL_atomic_load(&atomic));

    /* 测试CAS失败（期望值不匹配），expected应被更新为实际值 */
    expected = 42;
    result = OSAL_atomic_compare_exchange_strong(&atomic, &expected, 200);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(100, OSAL_atomic_load(&atomic));  /* 值不变 */
    TEST_ASSERT_EQUAL(100, expected);  /* expected被更新为实际值 */

    /* 测试CAS成功（交换为0） */
    expected = 100;
    result = OSAL_atomic_compare_exchange_strong(&atomic, &expected, 0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));
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
        OSAL_atomic_inc(data->counter);
    }

    return NULL;
}

static void test_atomic_multithread_increment(void)
{
    osal_atomic_uint32_t counter;
    osal_thread_t threads[THREAD_COUNT];
    atomic_thread_data_t thread_data[THREAD_COUNT];

    OSAL_atomic_init(&counter, 0);

    /* 创建多个线程同时自增 */
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].counter = &counter;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_pthread_create(&threads[i], NULL,
                                       atomic_increment_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证计数器值 */
    uint32_t expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint32_t actual = OSAL_atomic_load(&counter);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }
}

static void* atomic_cas_thread(void *arg)
{
    atomic_thread_data_t *data = (atomic_thread_data_t *)arg;

    uint32_t i;

    for (i = 0; i < data->iterations; i++) {
        uint32_t expected, new_value;
        bool success;

        do {
            expected = OSAL_atomic_load(data->counter);
            new_value = expected + 1;
            success = OSAL_atomic_compare_exchange_strong(data->counter, &expected, new_value);
        } while (!success);
    }

    return NULL;
}

static void test_atomic_multithread_cas(void)
{
    osal_atomic_uint32_t counter;
    osal_thread_t threads[THREAD_COUNT];
    atomic_thread_data_t thread_data[THREAD_COUNT];

    OSAL_atomic_init(&counter, 0);

    /* 创建多个线程使用CAS自增 */
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].counter = &counter;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_pthread_create(&threads[i], NULL,
                                       atomic_cas_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证计数器值 */
    uint32_t expected = THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint32_t actual = OSAL_atomic_load(&counter);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }
}

/*===========================================================================
 * 边界测试
 *===========================================================================*/

static void test_atomic_overflow(void)
{
    osal_atomic_uint32_t atomic;

    /* 测试上溢 */
    OSAL_atomic_init(&atomic, 0xFFFFFFFF);
    OSAL_atomic_inc(&atomic);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));  /* 溢出回0 */

    /* 测试下溢 */
    OSAL_atomic_init(&atomic, 0);
    OSAL_atomic_dec(&atomic);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_atomic_load(&atomic));  /* 下溢到最大值 */
}

static void test_atomic_boundary_values(void)
{
    osal_atomic_uint32_t atomic;

    /* 测试最小值 */
    OSAL_atomic_init(&atomic, 0);
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&atomic));

    /* 测试最大值 */
    OSAL_atomic_init(&atomic, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, OSAL_atomic_load(&atomic));

    /* 测试中间值 */
    OSAL_atomic_init(&atomic, 0x80000000);
    TEST_ASSERT_EQUAL(0x80000000, OSAL_atomic_load(&atomic));
}

/*===========================================================================
 * 64位原子操作测试
 *===========================================================================*/

static void test_atomic64_init_and_load(void)
{
    osal_atomic_uint64_t atomic;

    /* 测试初始化为0 */
    OSAL_atomic_init_u64(&atomic, 0);
    TEST_ASSERT_EQUAL(0ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试初始化为大值 */
    OSAL_atomic_init_u64(&atomic, 0x123456789ABCDEF0ULL);
    TEST_ASSERT_EQUAL(0x123456789ABCDEF0ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试初始化为最大值 */
    OSAL_atomic_init_u64(&atomic, UINT64_MAX);
    TEST_ASSERT_EQUAL(UINT64_MAX, OSAL_atomic_load_u64(&atomic));
}

static void test_atomic64_store(void)
{
    osal_atomic_uint64_t atomic;

    OSAL_atomic_init_u64(&atomic, 0);

    /* 测试存储不同值 */
    OSAL_atomic_store_u64(&atomic, 1000000000000ULL);
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_atomic_load_u64(&atomic));

    OSAL_atomic_store_u64(&atomic, 0xFEDCBA9876543210ULL);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_atomic_load_u64(&atomic));
}

static void test_atomic64_increment_decrement(void)
{
    osal_atomic_uint64_t atomic;

    OSAL_atomic_init_u64(&atomic, 1000000000000ULL);

    /* 测试自增 */
    TEST_ASSERT_EQUAL(1000000000001ULL, OSAL_atomic_inc_u64(&atomic));
    TEST_ASSERT_EQUAL(1000000000001ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试自减 */
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_atomic_dec_u64(&atomic));
    TEST_ASSERT_EQUAL(1000000000000ULL, OSAL_atomic_load_u64(&atomic));
}

static void test_atomic64_fetch_add_sub(void)
{
    osal_atomic_uint64_t atomic;

    OSAL_atomic_init_u64(&atomic, 1000000000000ULL);

    /* 测试fetch_add */
    uint64_t old_value = OSAL_atomic_fetch_add_u64(&atomic, 500000000000ULL);
    TEST_ASSERT_EQUAL(1000000000000ULL, old_value);
    TEST_ASSERT_EQUAL(1500000000000ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试fetch_sub */
    old_value = OSAL_atomic_fetch_sub_u64(&atomic, 300000000000ULL);
    TEST_ASSERT_EQUAL(1500000000000ULL, old_value);
    TEST_ASSERT_EQUAL(1200000000000ULL, OSAL_atomic_load_u64(&atomic));
}

static void test_atomic64_compare_exchange(void)
{
    osal_atomic_uint64_t atomic;

    OSAL_atomic_init_u64(&atomic, 0x123456789ABCDEF0ULL);

    /* 测试CAS成功 */
    uint64_t expected = 0x123456789ABCDEF0ULL;
    bool result = OSAL_atomic_compare_exchange_strong_u64(&atomic, &expected, 0xFEDCBA9876543210ULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试CAS失败，expected应被更新为实际值 */
    expected = 0x123456789ABCDEF0ULL;
    result = OSAL_atomic_compare_exchange_strong_u64(&atomic, &expected, 0x1111111111111111ULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, OSAL_atomic_load_u64(&atomic));
    TEST_ASSERT_EQUAL(0xFEDCBA9876543210ULL, expected);  /* expected被更新 */
}

static void test_atomic64_overflow(void)
{
    osal_atomic_uint64_t atomic;

    /* 测试上溢 */
    OSAL_atomic_init_u64(&atomic, UINT64_MAX);
    OSAL_atomic_inc_u64(&atomic);
    TEST_ASSERT_EQUAL(0ULL, OSAL_atomic_load_u64(&atomic));

    /* 测试下溢 */
    OSAL_atomic_init_u64(&atomic, 0);
    OSAL_atomic_dec_u64(&atomic);
    TEST_ASSERT_EQUAL(UINT64_MAX, OSAL_atomic_load_u64(&atomic));
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
        OSAL_atomic_inc_u64(data->timestamp);
    }

    return NULL;
}

static void test_atomic64_multithread_timestamp(void)
{
    osal_atomic_uint64_t timestamp;
    osal_thread_t threads[THREAD_COUNT];
    atomic64_thread_data_t thread_data[THREAD_COUNT];

    OSAL_atomic_init_u64(&timestamp, 0);

    /* 创建多个线程同时更新时间戳 */
    int32_t i;

    for (i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].timestamp = &timestamp;
        thread_data[i].iterations = ITERATIONS_PER_THREAD;

        int32_t ret = OSAL_pthread_create(&threads[i], NULL,
                                       atomic64_increment_thread, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    OSAL_msleep(1000);

    /* 验证时间戳值 */
    uint64_t expected = (uint64_t)THREAD_COUNT * ITERATIONS_PER_THREAD;
    uint64_t actual = OSAL_atomic_load_u64(&timestamp);
    TEST_ASSERT_EQUAL(expected, actual);

    /* 清理线程 */

    for (i = 0; i < THREAD_COUNT; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 32位原子操作测试 */
    /* 64位原子操作测试 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_atomic_init_and_load",
		.func = test_atomic_init_and_load,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_store",
		.func = test_atomic_store,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_increment",
		.func = test_atomic_increment,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_decrement",
		.func = test_atomic_decrement,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_fetch_add",
		.func = test_atomic_fetch_add,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_fetch_sub",
		.func = test_atomic_fetch_sub,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_compare_exchange",
		.func = test_atomic_compare_exchange,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_multithread_increment",
		.func = test_atomic_multithread_increment,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_multithread_cas",
		.func = test_atomic_multithread_cas,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_overflow",
		.func = test_atomic_overflow,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic_boundary_values",
		.func = test_atomic_boundary_values,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_init_and_load",
		.func = test_atomic64_init_and_load,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_store",
		.func = test_atomic64_store,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_increment_decrement",
		.func = test_atomic64_increment_decrement,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_fetch_add_sub",
		.func = test_atomic64_fetch_add_sub,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_compare_exchange",
		.func = test_atomic64_compare_exchange,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_overflow",
		.func = test_atomic64_overflow,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_atomic64_multithread_timestamp",
		.func = test_atomic64_multithread_timestamp,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_atomic",
	.module_name = "osal_atomic",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "OSAL osal_atomic tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_osal_atomic_tests(void)
{
	libutest_register_suite(&test_suite);
}
