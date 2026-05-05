/**
 * @file test_osal_stress.c
 * @brief OSAL 多线程压力测试
 *
 * 测试同步原语在高并发场景下的稳定性和正确性
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"

/* 压力测试配置 */
#define STRESS_THREAD_COUNT     10
#define STRESS_ITERATIONS       1000
#define STRESS_PRODUCER_COUNT   5
#define STRESS_CONSUMER_COUNT   5
#define STRESS_BUFFER_SIZE      100

/* 共享数据 */
static int32_t stress_counter = 0;
static osal_mutex_t *stress_mutex = NULL;

/* 互斥锁压力测试 - 线程函数 */
static void* mutex_stress_thread(void *arg)
{
    (void)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS; i++) {
        OSAL_MutexLock(stress_mutex);
        stress_counter++;
        OSAL_MutexUnlock(stress_mutex);
    }

    return NULL;
}

/* 测试用例1: 互斥锁压力测试 - 多线程竞争 */
TEST_CASE(test_mutex_stress)
{
    stress_counter = 0;
    OSAL_MutexCreate(&stress_mutex);

    osal_thread_t threads[STRESS_THREAD_COUNT];

    /* 创建多个线程同时增加计数器 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT; i++) {
        OSAL_pthread_create(&threads[i], NULL, mutex_stress_thread, NULL);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证计数器正确 */
    int32_t expected = STRESS_THREAD_COUNT * STRESS_ITERATIONS;
    TEST_ASSERT_EQUAL(expected, stress_counter);

    OSAL_MutexDelete(stress_mutex);
}

/* 信号量压力测试 - 生产者消费者模型 */
static int32_t sem_buffer[STRESS_BUFFER_SIZE];
static int32_t sem_write_pos = 0;
static int32_t sem_read_pos = 0;
static osal_semaphore_t *sem_empty = NULL;
static osal_semaphore_t *sem_full = NULL;
static osal_mutex_t *sem_mutex = NULL;
static int32_t sem_produced = 0;
static int32_t sem_consumed = 0;

static void* sem_producer_thread(void *arg)
{
    int32_t id = *(int32_t *)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS; i++) {
        OSAL_SemaphoreWait(sem_empty);
        OSAL_MutexLock(sem_mutex);

        sem_buffer[sem_write_pos] = id * 10000 + i;
        sem_write_pos = (sem_write_pos + 1) % STRESS_BUFFER_SIZE;
        sem_produced++;

        OSAL_MutexUnlock(sem_mutex);
        OSAL_SemaphorePost(sem_full);
    }

    return NULL;
}

static void* sem_consumer_thread(void *arg)
{
    (void)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS; i++) {
        OSAL_SemaphoreWait(sem_full);
        OSAL_MutexLock(sem_mutex);

        int32_t data = sem_buffer[sem_read_pos];
        (void)data;  /* 使用数据 */
        sem_read_pos = (sem_read_pos + 1) % STRESS_BUFFER_SIZE;
        sem_consumed++;

        OSAL_MutexUnlock(sem_mutex);
        OSAL_SemaphorePost(sem_empty);
    }

    return NULL;
}

/* 测试用例2: 信号量压力测试 - 生产者消费者 */
TEST_CASE(test_semaphore_stress)
{
    sem_write_pos = 0;
    sem_read_pos = 0;
    sem_produced = 0;
    sem_consumed = 0;

    OSAL_SemaphoreCreate(&sem_empty, STRESS_BUFFER_SIZE);
    OSAL_SemaphoreCreate(&sem_full, 0);
    OSAL_MutexCreate(&sem_mutex);

    osal_thread_t producers[STRESS_PRODUCER_COUNT];
    osal_thread_t consumers[STRESS_CONSUMER_COUNT];
    int32_t producer_ids[STRESS_PRODUCER_COUNT];

    /* 创建生产者线程 */
    for (int32_t i = 0; i < STRESS_PRODUCER_COUNT; i++) {
        producer_ids[i] = i;
        OSAL_pthread_create(&producers[i], NULL, sem_producer_thread, &producer_ids[i]);
    }

    /* 创建消费者线程 */
    for (int32_t i = 0; i < STRESS_CONSUMER_COUNT; i++) {
        OSAL_pthread_create(&consumers[i], NULL, sem_consumer_thread, NULL);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < STRESS_PRODUCER_COUNT; i++) {
        OSAL_pthread_join(producers[i], NULL);
    }
    for (int32_t i = 0; i < STRESS_CONSUMER_COUNT; i++) {
        OSAL_pthread_join(consumers[i], NULL);
    }

    /* 验证生产和消费数量相等 */
    int32_t expected = STRESS_PRODUCER_COUNT * STRESS_ITERATIONS;
    TEST_ASSERT_EQUAL(expected, sem_produced);
    TEST_ASSERT_EQUAL(expected, sem_consumed);

    OSAL_SemaphoreDelete(sem_empty);
    OSAL_SemaphoreDelete(sem_full);
    OSAL_MutexDelete(sem_mutex);
}

/* 条件变量压力测试 */
static int32_t cond_ready_count = 0;
static int32_t cond_wait_count = 0;
static osal_cond_t *cond_stress = NULL;
static osal_mutex_t *cond_mutex = NULL;

static void* cond_waiter_thread(void *arg)
{
    (void)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS / 10; i++) {
        OSAL_MutexLock(cond_mutex);
        while (cond_ready_count == 0) {
            OSAL_CondWait(cond_stress, cond_mutex);
        }
        cond_ready_count--;
        cond_wait_count++;
        OSAL_MutexUnlock(cond_mutex);
    }

    return NULL;
}

static void* cond_signaler_thread(void *arg)
{
    (void)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS / 10; i++) {
        OSAL_MutexLock(cond_mutex);
        cond_ready_count++;
        OSAL_CondSignal(cond_stress);
        OSAL_MutexUnlock(cond_mutex);
        OSAL_msleep(1);  /* 短暂延时，让等待线程有机会执行 */
    }

    return NULL;
}

/* 测试用例3: 条件变量压力测试 - 多线程等待/唤醒 */
TEST_CASE(test_cond_stress)
{
    cond_ready_count = 0;
    cond_wait_count = 0;

    OSAL_CondCreate(&cond_stress);
    OSAL_MutexCreate(&cond_mutex);

    osal_thread_t waiters[STRESS_THREAD_COUNT / 2];
    osal_thread_t signalers[STRESS_THREAD_COUNT / 2];

    /* 创建等待线程 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT / 2; i++) {
        OSAL_pthread_create(&waiters[i], NULL, cond_waiter_thread, NULL);
    }

    /* 创建信号线程 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT / 2; i++) {
        OSAL_pthread_create(&signalers[i], NULL, cond_signaler_thread, NULL);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT / 2; i++) {
        OSAL_pthread_join(signalers[i], NULL);
    }
    for (int32_t i = 0; i < STRESS_THREAD_COUNT / 2; i++) {
        OSAL_pthread_join(waiters[i], NULL);
    }

    /* 验证等待次数 */
    int32_t expected = (STRESS_THREAD_COUNT / 2) * (STRESS_ITERATIONS / 10);
    TEST_ASSERT_EQUAL(expected, cond_wait_count);

    OSAL_CondDelete(cond_stress);
    OSAL_MutexDelete(cond_mutex);
}

/* 混合场景压力测试 */
static int32_t mixed_data = 0;
static bool mixed_ready = false;
static osal_mutex_t *mixed_mutex = NULL;
static osal_semaphore_t *mixed_sem = NULL;
static osal_cond_t *mixed_cond = NULL;

static void* mixed_worker_thread(void *arg)
{
    int32_t id = *(int32_t *)arg;

    for (int32_t i = 0; i < STRESS_ITERATIONS / 100; i++) {
        /* 使用信号量控制并发 */
        OSAL_SemaphoreWait(mixed_sem);

        /* 使用互斥锁保护共享数据 */
        OSAL_MutexLock(mixed_mutex);
        mixed_data += id;
        mixed_ready = true;
        OSAL_CondSignal(mixed_cond);
        OSAL_MutexUnlock(mixed_mutex);

        /* 释放信号量 */
        OSAL_SemaphorePost(mixed_sem);

        OSAL_msleep(1);
    }

    return NULL;
}

/* 测试用例4: 混合场景压力测试 */
TEST_CASE(test_mixed_stress)
{
    mixed_data = 0;
    mixed_ready = false;

    OSAL_MutexCreate(&mixed_mutex);
    OSAL_SemaphoreCreate(&mixed_sem, 3);  /* 限制并发数为3 */
    OSAL_CondCreate(&mixed_cond);

    osal_thread_t threads[STRESS_THREAD_COUNT];
    int32_t thread_ids[STRESS_THREAD_COUNT];

    /* 创建工作线程 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT; i++) {
        thread_ids[i] = i + 1;
        OSAL_pthread_create(&threads[i], NULL, mixed_worker_thread, &thread_ids[i]);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < STRESS_THREAD_COUNT; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证数据正确性（所有线程ID之和 * 迭代次数） */
    int32_t sum_of_ids = 0;
    for (int32_t i = 1; i <= STRESS_THREAD_COUNT; i++) {
        sum_of_ids += i;
    }
    int32_t expected = sum_of_ids * (STRESS_ITERATIONS / 100);
    TEST_ASSERT_EQUAL(expected, mixed_data);

    OSAL_MutexDelete(mixed_mutex);
    OSAL_SemaphoreDelete(mixed_sem);
    OSAL_CondDelete(mixed_cond);
}

/* 注册测试套件 */
TEST_SUITE_BEGIN(osal_stress, "osal", "OSAL")
    TEST_CASE_REF(test_mutex_stress)
    TEST_CASE_REF(test_semaphore_stress)
    TEST_CASE_REF(test_cond_stress)
    TEST_CASE_REF(test_mixed_stress)
TEST_SUITE_END(osal_stress, "osal", "OSAL")
