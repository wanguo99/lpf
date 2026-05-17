/**
 * @file test_stress_osal.c
 * @brief OSAL层压力测试示例
 */

#include "test_framework.h"
#include "test_stress.h"
#include "osal.h"

/* 共享测试数据 */
typedef struct {
    osal_mutex_t *mutex;
    osal_atomic_uint32_t counter;
    uint32_t expected_count;
} mutex_stress_data_t;

typedef struct {
    osal_queue_t *queue;
    osal_atomic_uint32_t send_count;
    osal_atomic_uint32_t recv_count;
} queue_stress_data_t;

/**
 * 互斥锁压力测试工作函数
 */
static int32_t mutex_stress_worker(void *user_data, uint32_t iteration) {
    mutex_stress_data_t *data = (mutex_stress_data_t*)user_data;

    /* 加锁 */
    int32_t ret = OSAL_MutexLock(data->mutex, OSAL_WAIT_FOREVER);
    if (ret != 0) {
        return ret;
    }

    /* 临界区操作 */
    OSAL_AtomicInc32(&data->counter);

    /* 解锁 */
    OSAL_MutexUnlock(data->mutex);

    return 0;
}

/**
 * 测试互斥锁并发压力
 */
TEST_CASE(test_stress_mutex_concurrency) {
    mutex_stress_data_t data;
    const uint32_t thread_count = 10;
    const uint32_t duration_sec = 5;

    /* 初始化测试数据 */
    TEST_ASSERT_EQUAL(OSAL_MutexCreate(&data.mutex), 0);
    OSAL_AtomicInit32(&data.counter, 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Mutex Concurrency", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_Printf("[ INFO     ] Running mutex concurrency test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, mutex_stress_worker, &data), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_MutexDestroy(data.mutex);
}

/**
 * 队列压力测试工作函数（发送）
 */
static int32_t queue_send_worker(void *user_data, uint32_t iteration) {
    queue_stress_data_t *data = (queue_stress_data_t*)user_data;
    uint32_t value = iteration;

    int32_t ret = OSAL_QueuePut(data->queue, &value, 100);
    if (ret == 0) {
        OSAL_AtomicInc32(&data->send_count);
    }

    return ret;
}

/**
 * 测试队列长时间运行
 */
TEST_CASE(test_stress_queue_duration) {
    queue_stress_data_t data;
    const uint32_t queue_depth = 100;
    const uint32_t duration_sec = 10;

    /* 初始化测试数据 */
    TEST_ASSERT_EQUAL(OSAL_QueueCreate(&data.queue, queue_depth, sizeof(uint32_t)), 0);
    OSAL_AtomicInit32(&data.send_count, 0);
    OSAL_AtomicInit32(&data.recv_count, 0);

    /* 创建消费者线程 */
    osal_thread_t consumer_thread;
    bool consumer_running = true;

    auto consumer_func = [](void *arg) -> void* {
        queue_stress_data_t *d = (queue_stress_data_t*)arg;
        bool *running = (bool*)((char*)arg + sizeof(queue_stress_data_t));

        while (*running) {
            uint32_t value;
            if (OSAL_QueueGet(d->queue, &value, 100) == 0) {
                OSAL_AtomicInc32(&d->recv_count);
            }
        }
        return NULL;
    };

    struct {
        queue_stress_data_t data;
        bool running;
    } consumer_arg = { data, true };

    TEST_ASSERT_EQUAL(OSAL_ThreadCreate(&consumer_thread, "consumer",
                                        consumer_func, &consumer_arg,
                                        OSAL_PRIORITY_NORMAL, 0), 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_DURATION(duration_sec);
    stress_context_t *ctx = stress_context_create("Queue Duration", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_Printf("[ INFO     ] Running queue duration test: %u seconds\n", duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, queue_send_worker, &data), 0);

    /* 停止消费者 */
    consumer_running = false;
    OSAL_ThreadJoin(consumer_thread, NULL);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    OSAL_Printf("[ INFO     ] Sent: %u, Received: %u\n",
               OSAL_AtomicLoad32(&data.send_count),
               OSAL_AtomicLoad32(&data.recv_count));

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_QueueDestroy(data.queue);
}

/**
 * 信号量压力测试工作函数
 */
static int32_t semaphore_stress_worker(void *user_data, uint32_t iteration) {
    osal_semaphore_t *sem = (osal_semaphore_t*)user_data;
    (void)iteration;  /* Unused parameter */

    /* 等待信号量 */
    int32_t ret = OSAL_SemaphoreWait(sem);
    if (ret != 0) {
        return ret;
    }

    /* 模拟工作 */
    OSAL_sleep(1);

    /* 释放信号量 */
    OSAL_SemaphorePost(sem);

    return 0;
}

/**
 * 测试信号量资源限制
 */
TEST_CASE(test_stress_semaphore_resource) {
    osal_semaphore_t *sem = NULL;
    const uint32_t max_resources = 5;
    const uint32_t thread_count = 20;
    const uint32_t duration_sec = 5;

    /* 创建信号量（限制并发资源数） */
    TEST_ASSERT_EQUAL(OSAL_SemaphoreCreate(&sem, max_resources), 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Semaphore Resource", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_Printf("[ INFO     ] Running semaphore resource test: %u threads, %u max resources\n",
               thread_count, max_resources);
    TEST_ASSERT_EQUAL(stress_run(ctx, semaphore_stress_worker, sem), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 95.0);

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_SemaphoreDelete(sem);
}

/**
 * 原子操作压力测试工作函数
 */
static int32_t atomic_stress_worker(void *user_data, uint32_t iteration) {
    osal_atomic_uint32_t *counter = (osal_atomic_uint32_t*)user_data;
    (void)iteration;  /* Unused parameter */

    /* 原子递增 */
    OSAL_AtomicInc32(counter);

    /* 原子递减（通过加-1实现） */
    OSAL_AtomicFetchAdd32(counter, -1);

    return 0;
}

/**
 * 测试原子操作高并发
 */
TEST_CASE(test_stress_atomic_high_concurrency) {
    osal_atomic_uint32_t counter;
    const uint32_t thread_count = 50;
    const uint32_t iterations = 100000;

    OSAL_AtomicInit32(&counter, 0);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_ITERATIONS(iterations);
    config.thread_count = thread_count;
    stress_context_t *ctx = stress_context_create("Atomic High Concurrency", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_Printf("[ INFO     ] Running atomic high concurrency test: %u threads, %u iterations\n",
               thread_count, iterations);
    TEST_ASSERT_EQUAL(stress_run(ctx, atomic_stress_worker, &counter), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果：计数器应该回到0 */
    uint32_t final_value = OSAL_AtomicLoad32(&counter);
    OSAL_Printf("[ INFO     ] Final counter value: %u (expected: 0)\n", final_value);
    TEST_ASSERT_EQUAL(final_value, 0);

    STRESS_ASSERT_NO_ERRORS(ctx);

    /* 清理 */
    stress_context_destroy(ctx);
}

/* 注册压力测试模块 */
TEST_MODULE_BEGIN(stress_osal, "STRESS")
    TEST_CASE_REGISTER(test_stress_mutex_concurrency, "Mutex concurrency stress test")
    TEST_CASE_REGISTER(test_stress_queue_duration, "Queue duration stress test")
    TEST_CASE_REGISTER(test_stress_semaphore_resource, "Semaphore resource stress test")
    TEST_CASE_REGISTER(test_stress_atomic_high_concurrency, "Atomic high concurrency stress test")
TEST_MODULE_END(stress_osal, "STRESS")
