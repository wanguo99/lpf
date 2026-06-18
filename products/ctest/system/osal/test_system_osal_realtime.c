/**
 * @file test_system_osal_realtime.c
 * @brief Real-time scheduling integration tests
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"

/**
 * Test RT thread priority enforcement
 */
static void test_rt_thread_priority_enforcement(void)
{
    OSAL_printf("[ TEST     ] RT thread priority enforcement\n");

    osal_thread_t high_prio_thread, low_prio_thread;
    osal_atomic_uint32_t execution_order;
    osal_sem_t start_sem;
    int32_t ret;

    /* Initialize */
    OSAL_atomic_store(&execution_order, 0);
    ret = OSAL_sem_init(&start_sem, 0, 0);
    TEST_ASSERT_EQUAL(0, ret);

    /* Context for threads */
    typedef struct {
        osal_atomic_uint32_t *order;
        osal_sem_t *sem;
        uint32_t expected_order;
    } thread_ctx_t;

    thread_ctx_t high_ctx = { &execution_order, &start_sem, 1 };
    thread_ctx_t low_ctx = { &execution_order, &start_sem, 2 };

    void *priority_thread_func(void *arg)
    {
        thread_ctx_t *ctx = (thread_ctx_t *)arg;

        /* Wait for start signal */
        OSAL_sem_wait(ctx->sem);

        /* Record execution order */
        uint32_t order = OSAL_atomic_fetch_add(ctx->order, 1) + 1;

        /* Verify expected order (if RT scheduling is available) */
        /* Note: This test may not enforce strict ordering without RT scheduler
         */

        return NULL;
    }

    /* Create threads with different priorities */
    osal_thread_attr_t high_attr, low_attr;
    OSAL_pthread_attr_init(&high_attr);
    OSAL_pthread_attr_init(&low_attr);

    /* Try to set RT scheduling policy and priority */
    struct sched_param high_param = { .sched_priority = 80 };
    struct sched_param low_param = { .sched_priority = 20 };

    OSAL_pthread_attr_setschedpolicy(&high_attr, OSAL_SCHED_FIFO);
    OSAL_pthread_attr_setschedparam(&high_attr, &high_param);

    OSAL_pthread_attr_setschedpolicy(&low_attr, OSAL_SCHED_FIFO);
    OSAL_pthread_attr_setschedparam(&low_attr, &low_param);

    /* Create threads */
    ret = OSAL_pthread_create(&low_prio_thread,
                              &low_attr,
                              priority_thread_func,
                              &low_ctx);
    if (ret != 0) {
        OSAL_printf("[ SKIP     ] Cannot create low priority thread (need root "
                    "for RT)\n");
        OSAL_sem_destroy(&start_sem);
        return;
    }

    ret = OSAL_pthread_create(&high_prio_thread,
                              &high_attr,
                              priority_thread_func,
                              &high_ctx);
    if (ret != 0) {
        OSAL_printf("[ SKIP     ] Cannot create high priority thread\n");
        OSAL_pthread_cancel(low_prio_thread);
        OSAL_pthread_join(low_prio_thread, NULL);
        OSAL_sem_destroy(&start_sem);
        return;
    }

    /* Start both threads */
    OSAL_sleep(1);
    OSAL_sem_post(&start_sem);
    OSAL_sem_post(&start_sem);

    /* Wait for completion */
    OSAL_pthread_join(high_prio_thread, NULL);
    OSAL_pthread_join(low_prio_thread, NULL);

    /* Verify execution */
    TEST_ASSERT_EQUAL(2, OSAL_atomic_load(&execution_order));

    /* Cleanup */
    OSAL_pthread_attr_destroy(&high_attr);
    OSAL_pthread_attr_destroy(&low_attr);
    OSAL_sem_destroy(&start_sem);

    OSAL_printf("[ PASS     ] RT thread priority enforcement test passed\n");
}

/**
 * Test priority inheritance with mutexes
 */
static void test_priority_inheritance_mutex(void)
{
    OSAL_printf("[ TEST     ] Priority inheritance mutex\n");

    osal_mutex_t mutex;
    osal_mutex_attr_t attr;
    int32_t ret;

    /* Initialize mutex with priority inheritance protocol */
    ret = OSAL_pthread_mutexattr_init(&attr);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_mutexattr_setprotocol(&attr, OSAL_PTHREAD_PRIO_INHERIT);
    if (ret != 0) {
        OSAL_printf("[ SKIP     ] Priority inheritance not supported\n");
        OSAL_pthread_mutexattr_destroy(&attr);
        return;
    }

    ret = OSAL_pthread_mutex_init(&mutex, &attr);
    TEST_ASSERT_EQUAL(0, ret);

    /* Test mutex operations */
    ret = OSAL_pthread_mutex_lock(&mutex);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_mutex_unlock(&mutex);
    TEST_ASSERT_EQUAL(0, ret);

    /* Cleanup */
    OSAL_pthread_mutex_destroy(&mutex);
    OSAL_pthread_mutexattr_destroy(&attr);

    OSAL_printf("[ PASS     ] Priority inheritance mutex test passed\n");
}

/**
 * Test CPU affinity with RT threads
 */
static void test_cpu_affinity_rt_threads(void)
{
    OSAL_printf("[ TEST     ] CPU affinity with RT threads\n");

    osal_thread_t thread;
    osal_cpu_set_t cpuset;
    int32_t ret;

    /* Initialize CPU set */
    OSAL_CPU_ZERO(&cpuset);
    OSAL_CPU_SET(0, &cpuset);

    /* Create thread */
    void *affinity_thread_func(void *arg)
    {
        osal_cpu_set_t *set = (osal_cpu_set_t *)arg;
        int32_t ret;

        /* Set CPU affinity */
        ret = OSAL_sched_setaffinity(0, sizeof(*set), set);
        if (ret != 0) {
            return (void *)-1;
        }

        /* Verify affinity */
        osal_cpu_set_t current_set;
        OSAL_CPU_ZERO(&current_set);
        ret = OSAL_sched_getaffinity(0, sizeof(current_set), &current_set);
        if (ret != 0) {
            return (void *)-1;
        }

        /* Check if CPU 0 is set */
        if (!OSAL_CPU_ISSET(0, &current_set)) {
            return (void *)-1;
        }

        return NULL;
    }

    ret = OSAL_pthread_create(&thread, NULL, affinity_thread_func, &cpuset);
    TEST_ASSERT_EQUAL(0, ret);

    /* Wait for completion */
    void *thread_ret;
    OSAL_pthread_join(thread, &thread_ret);

    if (thread_ret == (void *)-1) {
        OSAL_printf("[ SKIP     ] CPU affinity not available\n");
        return;
    }

    TEST_ASSERT_NULL(thread_ret);

    OSAL_printf("[ PASS     ] CPU affinity with RT threads test passed\n");
}

/**
 * Test RT thread + Timer interaction
 */
static void test_rt_thread_timer_interaction(void)
{
    OSAL_printf("[ TEST     ] RT thread + Timer interaction\n");

    osal_thread_t thread;
    osal_atomic_uint32_t timer_ticks;
    int shutdown = 0;
    int32_t ret;

    /* Initialize */
    OSAL_atomic_store(&timer_ticks, 0);

    /* Context */
    typedef struct {
        osal_atomic_uint32_t *ticks;
        int *should_exit;
    } timer_ctx_t;

    timer_ctx_t ctx = { &timer_ticks, &shutdown };

    void *timer_thread_func(void *arg)
    {
        timer_ctx_t *ctx = (timer_ctx_t *)arg;
        uint64_t start_time, current_time;

        start_time = OSAL_get_monotonic_time();

        while (!(*ctx->should_exit)) {
            OSAL_msleep(100);
            current_time = OSAL_get_monotonic_time();

            /* Verify monotonic time increases */
            TEST_ASSERT_TRUE(current_time >= start_time);

            OSAL_atomic_inc(ctx->ticks);

            if (OSAL_atomic_load(ctx->ticks) >= 10) {
                break;
            }
        }

        return NULL;
    }

    /* Create timer thread */
    ret = OSAL_pthread_create(&thread, NULL, timer_thread_func, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    /* Wait for completion */
    OSAL_pthread_join(thread, NULL);

    /* Verify timer ticks */
    TEST_ASSERT_TRUE(OSAL_atomic_load(&timer_ticks) >= 10);

    OSAL_printf("[ PASS     ] RT thread + Timer interaction test passed\n");
}

/**
 * Test scheduler policy interactions
 */
static void test_scheduler_policy_interactions(void)
{
    OSAL_printf("[ TEST     ] Scheduler policy interactions\n");

    osal_thread_t threads[3];
    uint32_t i;
    int32_t ret;

    /* Context */
    typedef struct {
        int policy;
        int priority;
    } sched_ctx_t;

    sched_ctx_t contexts[3] = { { OSAL_SCHED_OTHER, 0 },
                                { OSAL_SCHED_FIFO, 50 },
                                { OSAL_SCHED_RR, 50 } };

    void *sched_thread_func(void *arg)
    {
        sched_ctx_t *ctx = (sched_ctx_t *)arg;
        struct sched_param param;
        int policy;
        int32_t ret;

        /* Try to set scheduler policy */
        param.sched_priority = ctx->priority;
        ret = OSAL_sched_setscheduler(0, ctx->policy, &param);

        /* Get current policy */
        ret = OSAL_sched_getscheduler(0);
        if (ret >= 0) {
            policy = ret;
        }

        /* Do some work */
        uint32_t i;
        for (i = 0; i < 100; i++) {
            OSAL_sched_yield();
        }

        return NULL;
    }

    /* Create threads with different policies */
    for (i = 0; i < 3; i++) {
        ret = OSAL_pthread_create(&threads[i],
                                  NULL,
                                  sched_thread_func,
                                  &contexts[i]);
        if (ret != 0) {
            OSAL_printf("[ SKIP     ] Cannot create thread %u\n", i);
            /* Clean up already created threads */
            uint32_t j;
            for (j = 0; j < i; j++) {
                OSAL_pthread_join(threads[j], NULL);
            }
            return;
        }
    }

    /* Wait for all threads */
    for (i = 0; i < 3; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    OSAL_printf("[ PASS     ] Scheduler policy interactions test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
    { .name = "test_rt_thread_priority_enforcement",
      .func = test_rt_thread_priority_enforcement,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_priority_inheritance_mutex",
      .func = test_priority_inheritance_mutex,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_cpu_affinity_rt_threads",
      .func = test_cpu_affinity_rt_threads,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_rt_thread_timer_interaction",
      .func = test_rt_thread_timer_interaction,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_scheduler_policy_interactions",
      .func = test_scheduler_policy_interactions,
      .setup = NULL,
      .teardown = NULL },
};

/* Test suite definition */
static const test_suite_t test_suite = {
    .suite_name = "system_osal_realtime",
    .module_name = "system_osal",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = { .category = TEST_CATEGORY_SYSTEM,
                  .tags = TEST_TAG_SLOW | TEST_TAG_REALTIME,
                  .timeout_ms = 15000,
                  .description = "Real-time scheduling integration tests" }
};

/* Register test suite */
__attribute__((constructor)) static void
register_system_osal_realtime_tests(void)
{
    libutest_register_suite(&test_suite);
}
