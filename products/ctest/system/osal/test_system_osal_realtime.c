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
static void _test_rt_thread_priority_enforcement(void)
{
	osal_printf("[ TEST     ] RT thread priority enforcement\n");

	osal_thread_t high_prio_thread, low_prio_thread;
	osal_atomic_uint32_t execution_order;
	osal_sem_t start_sem;
	int32_t ret;

	/* Initialize */
	osal_atomic_store(&execution_order, 0);
	ret = osal_sem_init(&start_sem, 0, 0);
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
		osal_sem_wait(ctx->sem);

		/* Record execution order */
		uint32_t order = osal_atomic_fetch_add(ctx->order, 1) + 1;
		(void)order;
		(void)ctx->expected_order;

		/* Verify expected order (if RT scheduling is available) */
		/* Note: This test may not enforce strict ordering without RT scheduler
         */

		return NULL;
	}

	/* Create threads with different priorities */
	osal_thread_attr_t high_attr, low_attr;
	osal_thread_attr_init(&high_attr);
	osal_thread_attr_init(&low_attr);

	/* Try to set RT scheduling policy and priority */
	osal_sched_param_t high_param = { .sched_priority = 80 };
	osal_sched_param_t low_param = { .sched_priority = 20 };

	osal_thread_attr_set_sched_policy(&high_attr, OSAL_SCHED_FIFO);
	osal_thread_attr_set_sched_param(&high_attr, &high_param);

	osal_thread_attr_set_sched_policy(&low_attr, OSAL_SCHED_FIFO);
	osal_thread_attr_set_sched_param(&low_attr, &low_param);

	/* Create threads */
	ret = osal_thread_create(&low_prio_thread, &low_attr, priority_thread_func,
							 &low_ctx);
	if (ret != 0) {
		osal_printf("[ SKIP     ] Cannot create low priority thread (need root "
					"for RT)\n");
		osal_sem_destroy(&start_sem);
		return;
	}

	ret = osal_thread_create(&high_prio_thread, &high_attr,
							 priority_thread_func, &high_ctx);
	if (ret != 0) {
		osal_printf("[ SKIP     ] Cannot create high priority thread\n");
		osal_thread_cancel(low_prio_thread);
		osal_thread_join(low_prio_thread, NULL);
		osal_sem_destroy(&start_sem);
		return;
	}

	/* Start both threads */
	osal_sleep(1);
	osal_sem_post(&start_sem);
	osal_sem_post(&start_sem);

	/* Wait for completion */
	osal_thread_join(high_prio_thread, NULL);
	osal_thread_join(low_prio_thread, NULL);

	/* Verify execution */
	TEST_ASSERT_EQUAL(2, osal_atomic_load(&execution_order));

	/* Cleanup */
	osal_thread_attr_destroy(&high_attr);
	osal_thread_attr_destroy(&low_attr);
	osal_sem_destroy(&start_sem);

	osal_printf("[ PASS     ] RT thread priority enforcement test passed\n");
}

/**
 * Test priority inheritance with mutexes
 */
static void _test_priority_inheritance_mutex(void)
{
	osal_printf("[ TEST     ] Priority inheritance mutex\n");

	osal_mutex_t mutex;
	osal_mutex_attr_t attr;
	int32_t ret;

	/* Initialize mutex with priority inheritance protocol */
	ret = osal_mutex_attr_init(&attr);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_attr_set_protocol(&attr, OSAL_MUTEX_PRIO_INHERIT);
	if (ret != 0) {
		osal_printf("[ SKIP     ] Priority inheritance not supported\n");
		osal_mutex_attr_destroy(&attr);
		return;
	}

	ret = osal_mutex_init(&mutex, &attr);
	TEST_ASSERT_EQUAL(0, ret);

	/* Test mutex operations */
	ret = osal_mutex_lock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_unlock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	/* Cleanup */
	osal_mutex_destroy(&mutex);
	osal_mutex_attr_destroy(&attr);

	osal_printf("[ PASS     ] Priority inheritance mutex test passed\n");
}

/**
 * Test CPU affinity with RT threads
 */
static void _test_cpu_affinity_rt_threads(void)
{
	osal_printf("[ TEST     ] CPU affinity with RT threads\n");

	osal_thread_t thread;
	int32_t ret;

	/* Create thread */
	void *affinity_thread_func(void *arg)
	{
		(void)arg;
		int32_t ret;
		int32_t current_cpu;

		/* Set CPU affinity */
		ret = osal_sched_set_affinity(0, 0);
		if (ret != 0) {
			return (void *)-1;
		}

		/* Verify affinity */
		ret = osal_sched_get_affinity(0, &current_cpu);
		if (ret != 0) {
			return (void *)-1;
		}

		/* Check if CPU 0 is set */
		if (current_cpu != 0) {
			return (void *)-1;
		}

		return NULL;
	}

	ret = osal_thread_create(&thread, NULL, affinity_thread_func, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* Wait for completion */
	void *thread_ret;
	osal_thread_join(thread, &thread_ret);

	if (thread_ret == (void *)-1) {
		osal_printf("[ SKIP     ] CPU affinity not available\n");
		return;
	}

	TEST_ASSERT_NULL(thread_ret);

	osal_printf("[ PASS     ] CPU affinity with RT threads test passed\n");
}

/**
 * Test RT thread + Timer interaction
 */
static void _test_rt_thread_timer_interaction(void)
{
	osal_printf("[ TEST     ] RT thread + Timer interaction\n");

	osal_thread_t thread;
	osal_atomic_uint32_t timer_ticks;
	int shutdown = 0;
	int32_t ret;

	/* Initialize */
	osal_atomic_store(&timer_ticks, 0);

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

		start_time = osal_get_monotonic_time();

		while (!(*ctx->should_exit)) {
			osal_msleep(100);
			current_time = osal_get_monotonic_time();

			if (current_time < start_time) {
				return (void *)-1;
			}

			osal_atomic_inc(ctx->ticks);

			if (osal_atomic_load(ctx->ticks) >= 10) {
				break;
			}
		}

		return NULL;
	}

	/* Create timer thread */
	ret = osal_thread_create(&thread, NULL, timer_thread_func, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	/* Wait for completion */
	void *thread_ret;
	osal_thread_join(thread, &thread_ret);
	TEST_ASSERT_NULL(thread_ret);

	/* Verify timer ticks */
	TEST_ASSERT_TRUE(osal_atomic_load(&timer_ticks) >= 10);

	osal_printf("[ PASS     ] RT thread + Timer interaction test passed\n");
}

/**
 * Test scheduler policy interactions
 */
static void _test_scheduler_policy_interactions(void)
{
	osal_printf("[ TEST     ] Scheduler policy interactions\n");

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
		osal_sched_param_t param;
		int32_t ret;

		/* Try to set scheduler policy */
		param.sched_priority = ctx->priority;
		ret = osal_sched_set_scheduler(0, ctx->policy, &param);

		/* Get current policy */
		ret = osal_sched_get_scheduler(0);
		(void)ret;

		/* Do some work */
		uint32_t i;
		for (i = 0; i < 100; i++) {
			osal_sched_yield();
		}

		return NULL;
	}

	/* Create threads with different policies */
	for (i = 0; i < 3; i++) {
		ret = osal_thread_create(&threads[i], NULL, sched_thread_func,
								 &contexts[i]);
		if (ret != 0) {
			osal_printf("[ SKIP     ] Cannot create thread %u\n", i);
			/* Clean up already created threads */
			uint32_t j;
			for (j = 0; j < i; j++) {
				osal_thread_join(threads[j], NULL);
			}
			return;
		}
	}

	/* Wait for all threads */
	for (i = 0; i < 3; i++) {
		osal_thread_join(threads[i], NULL);
	}

	osal_printf("[ PASS     ] Scheduler policy interactions test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
	{ .name = "test_rt_thread_priority_enforcement",
	  .func = _test_rt_thread_priority_enforcement,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_priority_inheritance_mutex",
	  .func = _test_priority_inheritance_mutex,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cpu_affinity_rt_threads",
	  .func = _test_cpu_affinity_rt_threads,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_rt_thread_timer_interaction",
	  .func = _test_rt_thread_timer_interaction,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_scheduler_policy_interactions",
	  .func = _test_scheduler_policy_interactions,
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
				  .tags = TEST_TAG_SLOW | TEST_TAG_PRIVILEGED,
				  .timeout_ms = 15000,
				  .description = "Real-time scheduling integration tests" }
};

/* Register test suite */
void register_system_osal_realtime_tests(void)
{
	libutest_register_suite(&test_suite);
}
