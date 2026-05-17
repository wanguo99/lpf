/**
 * @file test_system_integration.c
 * @brief 系统集成测试示例
 */

#include "test_framework.h"
#include "test_system.h"
#include "osal.h"
#include "hal.h"
#include "pdl.h"

/**
 * 环境初始化：OSAL + HAL + PDL
 */
SYSTEM_ENV_SETUP(full_stack) {
    OSAL_Printf("[ SETUP    ] Initializing full stack environment\n");

    /* 初始化OSAL */
    if (OSAL_Init() != 0) {
        OSAL_Printf("[ SETUP FAIL ] OSAL_Init failed\n");
        return -1;
    }
    env->osal_initialized = true;

    /* 初始化HAL */
    if (HAL_Init() != 0) {
        OSAL_Printf("[ SETUP FAIL ] HAL_Init failed\n");
        return -1;
    }
    env->hal_initialized = true;

    /* 初始化PDL */
    if (PDL_Init() != 0) {
        OSAL_Printf("[ SETUP FAIL ] PDL_Init failed\n");
        return -1;
    }
    env->pdl_initialized = true;

    OSAL_Printf("[ SETUP OK ] Full stack initialized\n");
    return 0;
}

/**
 * 环境清理
 */
SYSTEM_ENV_TEARDOWN(full_stack) {
    OSAL_Printf("[ TEARDOWN ] Cleaning up full stack environment\n");

    if (env->pdl_initialized) {
        PDL_Deinit();
    }

    if (env->hal_initialized) {
        HAL_Deinit();
    }

    if (env->osal_initialized) {
        OSAL_Deinit();
    }

    OSAL_Printf("[ TEARDOWN OK ] Full stack cleaned up\n");
}

/**
 * 系统测试：OSAL + HAL 集成
 */
SYSTEM_TEST_CASE(osal_hal_integration) {
    OSAL_Printf("[ TEST     ] Testing OSAL + HAL integration\n");

    /* 检查点1：OSAL初始化 */
    SYSTEM_CHECKPOINT(NULL, "OSAL initialized", env->osal_initialized);

    /* 检查点2：HAL初始化 */
    SYSTEM_CHECKPOINT(NULL, "HAL initialized", env->hal_initialized);

    /* 检查点3：GPIO操作 */
    hal_gpio_handle_t gpio = NULL;
    int32_t ret = HAL_GPIO_Open(0, &gpio);
    SYSTEM_CHECKPOINT(NULL, "GPIO open", ret == 0);

    if (ret == 0) {
        ret = HAL_GPIO_SetDirection(gpio, HAL_GPIO_DIR_OUTPUT);
        SYSTEM_CHECKPOINT(NULL, "GPIO set direction", ret == 0);

        ret = HAL_GPIO_Write(gpio, 1);
        SYSTEM_CHECKPOINT(NULL, "GPIO write", ret == 0);

        uint32_t value = 0;
        ret = HAL_GPIO_Read(gpio, &value);
        SYSTEM_CHECKPOINT(NULL, "GPIO read", ret == 0 && value == 1);

        HAL_GPIO_Close(gpio);
    }

    /* 检查点4：线程创建 */
    osal_thread_t thread;
    auto thread_func = [](void *arg) -> void* {
        OSAL_Sleep(100);
        return NULL;
    };

    ret = OSAL_ThreadCreate(&thread, "test_thread", thread_func, NULL,
                            OSAL_PRIORITY_NORMAL, 0);
    SYSTEM_CHECKPOINT(NULL, "Thread create", ret == 0);

    if (ret == 0) {
        OSAL_ThreadJoin(thread, NULL);
        SYSTEM_CHECKPOINT(NULL, "Thread join", true);
    }

    return 0;
}

/**
 * 系统测试：HAL + PDL 集成
 */
SYSTEM_TEST_CASE(hal_pdl_integration) {
    OSAL_Printf("[ TEST     ] Testing HAL + PDL integration\n");

    /* 检查点1：HAL初始化 */
    SYSTEM_CHECKPOINT(NULL, "HAL initialized", env->hal_initialized);

    /* 检查点2：PDL初始化 */
    SYSTEM_CHECKPOINT(NULL, "PDL initialized", env->pdl_initialized);

    /* 检查点3：Watchdog操作 */
    pdl_watchdog_config_t wdt_config = {
        .timeout_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&wdt_config);
    SYSTEM_CHECKPOINT(NULL, "Watchdog init", ret == 0);

    if (ret == 0) {
        ret = PDL_WATCHDOG_Start();
        SYSTEM_CHECKPOINT(NULL, "Watchdog start", ret == 0);

        ret = PDL_WATCHDOG_Kick();
        SYSTEM_CHECKPOINT(NULL, "Watchdog kick", ret == 0);

        watchdog_status_t status;
        ret = PDL_WATCHDOG_GetStatus(&status);
        SYSTEM_CHECKPOINT(NULL, "Watchdog get status",
                         ret == 0 && status.is_running);

        ret = PDL_WATCHDOG_Stop();
        SYSTEM_CHECKPOINT(NULL, "Watchdog stop", ret == 0);

        PDL_WATCHDOG_Deinit();
    }

    return 0;
}

/**
 * 系统测试：完整栈端到端测试
 */
SYSTEM_TEST_CASE(full_stack_e2e) {
    OSAL_Printf("[ TEST     ] Testing full stack end-to-end\n");

    /* 检查点1：所有层初始化 */
    SYSTEM_CHECKPOINT(NULL, "OSAL initialized", env->osal_initialized);
    SYSTEM_CHECKPOINT(NULL, "HAL initialized", env->hal_initialized);
    SYSTEM_CHECKPOINT(NULL, "PDL initialized", env->pdl_initialized);

    /* 检查点2：创建通信队列 */
    osal_queue_t *queue = NULL;
    int32_t ret = OSAL_QueueCreate(&queue, 10, sizeof(uint32_t));
    SYSTEM_CHECKPOINT(NULL, "Queue create", ret == 0);

    /* 检查点3：创建生产者线程 */
    osal_thread_t producer;
    auto producer_func = [](void *arg) -> void* {
        osal_queue_t *q = (osal_queue_t*)arg;
        for (uint32_t i = 0; i < 10; i++) {
            OSAL_QueuePut(q, &i, OSAL_WAIT_FOREVER);
            OSAL_Sleep(10);
        }
        return NULL;
    };

    ret = OSAL_ThreadCreate(&producer, "producer", producer_func, queue,
                            OSAL_PRIORITY_NORMAL, 0);
    SYSTEM_CHECKPOINT(NULL, "Producer thread create", ret == 0);

    /* 检查点4：创建消费者线程 */
    osal_thread_t consumer;
    auto consumer_func = [](void *arg) -> void* {
        osal_queue_t *q = (osal_queue_t*)arg;
        for (uint32_t i = 0; i < 10; i++) {
            uint32_t value;
            OSAL_QueueGet(q, &value, OSAL_WAIT_FOREVER);
            if (value != i) {
                return (void*)-1;
            }
        }
        return NULL;
    };

    ret = OSAL_ThreadCreate(&consumer, "consumer", consumer_func, queue,
                            OSAL_PRIORITY_NORMAL, 0);
    SYSTEM_CHECKPOINT(NULL, "Consumer thread create", ret == 0);

    /* 检查点5：等待线程完成 */
    void *producer_result = NULL;
    void *consumer_result = NULL;

    OSAL_ThreadJoin(producer, &producer_result);
    OSAL_ThreadJoin(consumer, &consumer_result);

    SYSTEM_CHECKPOINT(NULL, "Producer completed", producer_result == NULL);
    SYSTEM_CHECKPOINT(NULL, "Consumer completed", consumer_result == NULL);

    /* 清理 */
    OSAL_QueueDestroy(queue);

    return 0;
}

/**
 * 系统测试：并发场景测试
 */
SYSTEM_TEST_CASE(concurrent_scenario) {
    OSAL_Printf("[ TEST     ] Testing concurrent scenario\n");

    const uint32_t num_threads = 5;
    osal_thread_t threads[5];
    osal_mutex_t *mutex = NULL;
    osal_atomic_uint32_t counter;

    OSAL_AtomicInit32(&counter, 0);

    /* 检查点1：创建互斥锁 */
    int32_t ret = OSAL_MutexCreate(&mutex);
    SYSTEM_CHECKPOINT(NULL, "Mutex create", ret == 0);

    /* 检查点2：创建多个并发线程 */
    auto thread_func = [](void *arg) -> void* {
        struct {
            osal_mutex_t *mutex;
            osal_atomic_uint32_t *counter;
        } *data = (typeof(data))arg;

        for (uint32_t i = 0; i < 1000; i++) {
            OSAL_MutexLock(data->mutex, OSAL_WAIT_FOREVER);
            OSAL_AtomicInc32(data->counter);
            OSAL_MutexUnlock(data->mutex);
        }
        return NULL;
    };

    struct {
        osal_mutex_t *mutex;
        osal_atomic_uint32_t *counter;
    } thread_data = { mutex, &counter };

    bool all_created = true;
    for (uint32_t i = 0; i < num_threads; i++) {
        char name[32];
        snprintf(name, sizeof(name), "worker_%u", i);
        ret = OSAL_ThreadCreate(&threads[i], name, thread_func, &thread_data,
                                OSAL_PRIORITY_NORMAL, 0);
        if (ret != 0) {
            all_created = false;
            break;
        }
    }
    SYSTEM_CHECKPOINT(NULL, "All threads created", all_created);

    /* 检查点3：等待所有线程完成 */
    for (uint32_t i = 0; i < num_threads; i++) {
        OSAL_ThreadJoin(threads[i], NULL);
    }
    SYSTEM_CHECKPOINT(NULL, "All threads completed", true);

    /* 检查点4：验证计数器 */
    uint32_t final_count = OSAL_AtomicLoad32(&counter);
    uint32_t expected_count = num_threads * 1000;
    SYSTEM_CHECKPOINT(NULL, "Counter correct",
                     final_count == expected_count);

    OSAL_Printf("[ INFO     ] Final count: %u (expected: %u)\n",
               final_count, expected_count);

    /* 清理 */
    OSAL_MutexDestroy(mutex);

    return 0;
}

/**
 * 运行系统集成测试
 */
TEST_CASE(test_system_integration_suite) {
    system_test_context_t *ctx;

    /* 测试1：OSAL + HAL 集成 */
    ctx = system_test_create("OSAL+HAL Integration", SYSTEM_TEST_INTEGRATION);
    TEST_ASSERT_NOT_NULL(ctx);
    system_test_set_env_funcs(ctx,
                              system_env_setup_full_stack,
                              system_env_teardown_full_stack);
    TEST_ASSERT_EQUAL(system_test_run(ctx, system_test_osal_hal_integration), 0);
    system_test_print_report(ctx);
    system_test_destroy(ctx);

    /* 测试2：HAL + PDL 集成 */
    ctx = system_test_create("HAL+PDL Integration", SYSTEM_TEST_INTEGRATION);
    TEST_ASSERT_NOT_NULL(ctx);
    system_test_set_env_funcs(ctx,
                              system_env_setup_full_stack,
                              system_env_teardown_full_stack);
    TEST_ASSERT_EQUAL(system_test_run(ctx, system_test_hal_pdl_integration), 0);
    system_test_print_report(ctx);
    system_test_destroy(ctx);

    /* 测试3：完整栈端到端 */
    ctx = system_test_create("Full Stack E2E", SYSTEM_TEST_E2E);
    TEST_ASSERT_NOT_NULL(ctx);
    system_test_set_env_funcs(ctx,
                              system_env_setup_full_stack,
                              system_env_teardown_full_stack);
    TEST_ASSERT_EQUAL(system_test_run(ctx, system_test_full_stack_e2e), 0);
    system_test_print_report(ctx);
    system_test_destroy(ctx);

    /* 测试4：并发场景 */
    ctx = system_test_create("Concurrent Scenario", SYSTEM_TEST_SCENARIO);
    TEST_ASSERT_NOT_NULL(ctx);
    system_test_set_env_funcs(ctx,
                              system_env_setup_full_stack,
                              system_env_teardown_full_stack);
    TEST_ASSERT_EQUAL(system_test_run(ctx, system_test_concurrent_scenario), 0);
    system_test_print_report(ctx);
    system_test_destroy(ctx);
}

/* 注册系统测试模块 */
TEST_MODULE_BEGIN(system_integration, "SYSTEM")
    TEST_CASE_REGISTER(test_system_integration_suite, "System integration test suite")
TEST_MODULE_END(system_integration, "SYSTEM")
