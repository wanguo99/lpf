/************************************************************************
 * OSAL压力测试
 ************************************************************************/

#include "osal.h"
#include <test/test_framework.h>

#include <pthread.h>

/*===========================================================================
 * 互斥锁压力测试
 *===========================================================================*/

static osal_mutex_t stress_mutex;
static int32_t stress_counter = 0;

static void *mutex_stress_thread(void *arg)
{
    int32_t iterations = *(int32_t *)arg;

    for (int32_t i = 0; i < iterations; i++) {
        OSAL_pthread_mutex_lock(&stress_mutex);
        stress_counter++;
        OSAL_pthread_mutex_unlock(&stress_mutex);
    }

    return NULL;
}

static void test_stress_mutex_high_contention(void)
{
    const int32_t NUM_THREADS = 10;
    const int32_t ITERATIONS = 1000;
    osal_thread_t threads[NUM_THREADS];
    int32_t iterations = ITERATIONS;

    stress_counter = 0;

    /* 初始化互斥锁 */
    int32_t ret = OSAL_pthread_mutex_init(&stress_mutex, NULL);
    TEST_ASSERT_EQUAL(0, ret);

    /* 创建多个线程 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        ret = OSAL_pthread_create(&threads[i], NULL, mutex_stress_thread, &iterations);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证计数器值 */
    TEST_ASSERT_EQUAL(NUM_THREADS * ITERATIONS, stress_counter);

    /* 清理 */
    OSAL_pthread_mutex_destroy(&stress_mutex);
}

/*===========================================================================
 * 内存分配压力测试
 *===========================================================================*/

static void test_stress_memory_allocation(void)
{
    const int32_t NUM_ALLOCS = 1000;
    const int32_t ALLOC_SIZE = 1024;
    void *ptrs[NUM_ALLOCS];

    /* 大量分配 */
    for (int32_t i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = OSAL_malloc(ALLOC_SIZE);
        TEST_ASSERT_NOT_NULL(ptrs[i]);

        /* 写入数据 */
        OSAL_memset(ptrs[i], i % 256, ALLOC_SIZE);
    }

    /* 验证数据 */
    for (int32_t i = 0; i < NUM_ALLOCS; i++) {
        uint8_t *data = (uint8_t *)ptrs[i];
        for (int32_t j = 0; j < ALLOC_SIZE; j++) {
            TEST_ASSERT_EQUAL(i % 256, data[j]);
        }
    }

    /* 释放 */
    for (int32_t i = 0; i < NUM_ALLOCS; i++) {
        OSAL_free(ptrs[i]);
    }
}

static void *memory_stress_thread(void *arg)
{
    const int32_t NUM_ALLOCS = 100;
    const int32_t ALLOC_SIZE = 512;

    for (int32_t i = 0; i < NUM_ALLOCS; i++) {
        void *ptr = OSAL_malloc(ALLOC_SIZE);
        if (ptr) {
            OSAL_memset(ptr, 0xAA, ALLOC_SIZE);
            OSAL_free(ptr);
        }
    }

    return NULL;
}

static void test_stress_memory_multithread(void)
{
    const int32_t NUM_THREADS = 8;
    osal_thread_t threads[NUM_THREADS];

    /* 创建多个线程 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        int32_t ret = OSAL_pthread_create(&threads[i], NULL, memory_stress_thread, NULL);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }
}

/*===========================================================================
 * 信号量压力测试
 *===========================================================================*/

static osal_sem_t stress_sem;
static int32_t sem_counter = 0;

static void *semaphore_stress_thread(void *arg)
{
    int32_t iterations = *(int32_t *)arg;

    for (int32_t i = 0; i < iterations; i++) {
        OSAL_sem_wait(&stress_sem);
        __sync_fetch_and_add(&sem_counter, 1);
        OSAL_sem_post(&stress_sem);
    }

    return NULL;
}

static void test_stress_semaphore_contention(void)
{
    const int32_t NUM_THREADS = 8;
    const int32_t ITERATIONS = 500;
    osal_thread_t threads[NUM_THREADS];
    int32_t iterations = ITERATIONS;

    sem_counter = 0;

    /* 初始化信号量 */
    int32_t ret = OSAL_sem_init(&stress_sem, 0, 1);
    TEST_ASSERT_EQUAL(0, ret);

    /* 创建多个线程 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        ret = OSAL_pthread_create(&threads[i], NULL, semaphore_stress_thread, &iterations);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证计数器 */
    TEST_ASSERT_EQUAL(NUM_THREADS * ITERATIONS, sem_counter);

    /* 清理 */
    OSAL_sem_destroy(&stress_sem);
}

/*===========================================================================
 * 线程创建压力测试
 *===========================================================================*/

static void *dummy_thread_func(void *arg)
{
    OSAL_usleep(1000);
    return NULL;
}

static void test_stress_thread_creation(void)
{
    const int32_t NUM_ITERATIONS = 100;

    for (int32_t i = 0; i < NUM_ITERATIONS; i++) {
        osal_thread_t thread;

        /* 创建线程 */
        int32_t ret = OSAL_pthread_create(&thread, NULL, dummy_thread_func, NULL);
        TEST_ASSERT_EQUAL(0, ret);

        /* 等待线程结束 */
        ret = OSAL_pthread_join(thread, NULL);
        TEST_ASSERT_EQUAL(0, ret);
    }
}

/*===========================================================================
 * 原子操作压力测试
 *===========================================================================*/

static osal_atomic_uint32_t stress_atomic;

static void *atomic_stress_thread(void *arg)
{
    int32_t iterations = *(int32_t *)arg;

    for (int32_t i = 0; i < iterations; i++) {
        OSAL_atomic_inc(&stress_atomic);
    }

    return NULL;
}

static void test_stress_atomic_operations(void)
{
    const int32_t NUM_THREADS = 10;
    const int32_t ITERATIONS = 10000;
    osal_thread_t threads[NUM_THREADS];
    int32_t iterations = ITERATIONS;

    OSAL_atomic_init(&stress_atomic, 0);

    /* 创建多个线程 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        int32_t ret = OSAL_pthread_create(&threads[i], NULL, atomic_stress_thread, &iterations);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证结果 */
    uint32_t final_value = OSAL_atomic_load(&stress_atomic);
    TEST_ASSERT_EQUAL(NUM_THREADS * ITERATIONS, final_value);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_stress_mutex_high_contention",
        .func = test_stress_mutex_high_contention,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_memory_allocation",
        .func = test_stress_memory_allocation,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_memory_multithread",
        .func = test_stress_memory_multithread,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_semaphore_contention",
        .func = test_stress_semaphore_contention,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_thread_creation",
        .func = test_stress_thread_creation,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_atomic_operations",
        .func = test_stress_atomic_operations,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "osal_stress",
    .module_name = "osal_stress",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_STRESS,
        .tags = TEST_TAG_SLOW,
        .timeout_ms = 10000,
        .description = "OSAL stress tests"
    }
};

__attribute__((constructor))
static void register_osal_stress_tests(void)
{
    libutest_register_suite(&test_suite);
}
