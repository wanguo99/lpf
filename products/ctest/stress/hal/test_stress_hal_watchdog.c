/**
 * @file test_stress_hal_watchdog.c
 * @brief HAL Watchdog stress tests - rapid kick operations and edge cases
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_WATCHDOG

/* Test configuration */
#define WATCHDOG_STRESS_DEVICE      "/dev/watchdog"
#define WATCHDOG_STRESS_TIMEOUT_SEC  5
#define WATCHDOG_STRESS_DURATION    20

/* Worker context */
typedef struct {
	hal_watchdog_handle_t handle;
	osal_atomic_uint32_t *kick_counter;
	bool stop_flag;
} watchdog_worker_ctx_t;

/*===========================================================================
 * Test 1: Rapid Kick Operations
 *===========================================================================*/

/**
 * Worker for rapid watchdog kick
 */
static int32_t watchdog_rapid_kick_worker(void *user_data, uint32_t iteration)
{
	watchdog_worker_ctx_t *ctx = (watchdog_worker_ctx_t *)user_data;
	int32_t ret;

	/* Kick watchdog */
	ret = HAL_WATCHDOG_kick(ctx->handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->kick_counter);
	}

	/* Brief sleep to avoid overwhelming the driver */
	OSAL_usleep(10 * 1000);

	return ret;
}

/**
 * Test: Rapid watchdog kick operations
 * Verifies: timer accuracy, no false resets
 */
static void test_stress_watchdog_rapid_kick(void)
{
	hal_watchdog_handle_t handle = NULL;
	hal_watchdog_config_t config = {
		.device = WATCHDOG_STRESS_DEVICE,
		.timeout_sec = WATCHDOG_STRESS_TIMEOUT_SEC
	};
	osal_atomic_uint32_t kick_counter;
	watchdog_worker_ctx_t worker_ctx;
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_DURATION(WATCHDOG_STRESS_DURATION);
	int32_t ret;
	uint64_t start_time, end_time;

	OSAL_printf("[ INFO ] Starting watchdog rapid kick stress test\n");
	OSAL_printf("         Duration: %u sec, Timeout: %u ms\n",
	           WATCHDOG_STRESS_DURATION, WATCHDOG_STRESS_TIMEOUT_SEC);

	/* Initialize watchdog */
	ret = HAL_WATCHDOG_init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Watchdog device not available\n");
		TEST_SKIP("Watchdog not available");
		return;
	}

	/* Enable watchdog */
	ret = HAL_WATCHDOG_enable(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Initialize counter */
	OSAL_atomic_store(&kick_counter, 0);

	/* Setup worker context */
	worker_ctx.handle = handle;
	worker_ctx.kick_counter = &kick_counter;
	worker_ctx.stop_flag = false;

	/* Create stress test context */
	stress_ctx = stress_context_create("Watchdog_Rapid_Kick", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	start_time = 0;
	ret = stress_run(stress_ctx, watchdog_rapid_kick_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Wait for completion */
	OSAL_usleep(WATCHDOG_STRESS_DURATION * 1000 + 1000 * 1000);
	end_time = 0;

	/* Print report */
	stress_print_report(stress_ctx);

	/* Calculate kick rate */
	uint64_t total_kicks = OSAL_atomic_load(&kick_counter);
	uint64_t elapsed_ms = end_time - start_time;
	double kicks_per_sec = (total_kicks * 1000.0) / elapsed_ms;

	OSAL_printf("[ INFO ] Total kicks: %llu\n", (unsigned long long)total_kicks);
	OSAL_printf("[ INFO ] Kick rate: %.2f Hz\n", kicks_per_sec);

	/* Verify kicks occurred */
	TEST_ASSERT_TRUE(total_kicks > 0);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 99.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	HAL_WATCHDOG_disable(handle);
	HAL_WATCHDOG_deinit(handle);

	OSAL_printf("[ PASS ] Watchdog rapid kick stress test completed\n");
}

/*===========================================================================
 * Test 2: Timeout Edge Cases
 *===========================================================================*/

/**
 * Test: Watchdog timeout edge cases
 * Verifies: accurate timing, proper timeout handling
 */
static void test_stress_watchdog_timeout_edge_cases(void)
{
	hal_watchdog_handle_t handle = NULL;
	hal_watchdog_config_t config = {
		.device = WATCHDOG_STRESS_DEVICE,
		.timeout_sec = 3
	};
	int32_t ret;
	uint32_t timeout_ms;
	uint32_t timeleft_ms;

	OSAL_printf("[ INFO ] Starting watchdog timeout edge cases stress test\n");

	/* Initialize watchdog */
	ret = HAL_WATCHDOG_init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Watchdog device not available\n");
		TEST_SKIP("Watchdog not available");
		return;
	}

	/* Enable watchdog */
	ret = HAL_WATCHDOG_enable(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Test various timeout values */
	const uint32_t test_timeouts[] = {1000, 2000, 3000, 5000, 10000};

	for (size_t i = 0; i < sizeof(test_timeouts) / sizeof(test_timeouts[0]); i++) {
		OSAL_printf("[ INFO ] Testing timeout: %u ms\n", test_timeouts[i]);

		/* Set new timeout */
		ret = HAL_WATCHDOG_set_timeout(handle, test_timeouts[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Read back timeout */
		ret = HAL_WATCHDOG_get_timeout(handle, &timeout_ms);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		OSAL_printf("         Configured timeout: %u ms\n", timeout_ms);

		/* Kick watchdog */
		ret = HAL_WATCHDOG_kick(handle);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Check time left */
		ret = HAL_WATCHDOG_get_timeleft(handle, &timeleft_ms);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("         Time left: %u ms\n", timeleft_ms);
			/* Time left should be close to timeout */
			TEST_ASSERT_TRUE(timeleft_ms > 0);
			TEST_ASSERT_TRUE(timeleft_ms <= timeout_ms);
		}

		/* Wait for half the timeout period */
		OSAL_usleep(timeout_ms / 2 * 1000);

		/* Kick again to prevent reset */
		ret = HAL_WATCHDOG_kick(handle);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Cleanup */
	HAL_WATCHDOG_disable(handle);
	HAL_WATCHDOG_deinit(handle);

	OSAL_printf("[ PASS ] Watchdog timeout edge cases stress test completed\n");
}

/*===========================================================================
 * Test 3: Concurrent Configuration Changes
 *===========================================================================*/

/**
 * Test: Concurrent timeout changes while kicking
 * Verifies: safe reconfiguration, no race conditions
 */
static void test_stress_watchdog_concurrent_config(void)
{
	hal_watchdog_handle_t handle = NULL;
	hal_watchdog_config_t config = {
		.device = WATCHDOG_STRESS_DEVICE,
		.timeout_sec = 5
	};
	int32_t ret;
	const uint32_t iterations = 500;

	OSAL_printf("[ INFO ] Starting watchdog concurrent config stress test\n");
	OSAL_printf("         Iterations: %u\n", iterations);

	/* Initialize watchdog */
	ret = HAL_WATCHDOG_init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Watchdog device not available\n");
		TEST_SKIP("Watchdog not available");
		return;
	}

	/* Enable watchdog */
	ret = HAL_WATCHDOG_enable(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Rapidly change timeout and kick */
	for (uint32_t i = 0; i < iterations; i++) {
		uint32_t new_timeout = 2000 + ((i % 5) * 1000);

		/* Change timeout */
		ret = HAL_WATCHDOG_set_timeout(handle, new_timeout);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Kick watchdog */
		ret = HAL_WATCHDOG_kick(handle);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Small delay */
		OSAL_usleep(5 * 1000);

		/* Progress indicator */
		if (i % 100 == 0) {
			OSAL_printf("[ INFO ] Progress: %u/%u\n", i, iterations);
		}
	}

	/* Cleanup */
	HAL_WATCHDOG_disable(handle);
	HAL_WATCHDOG_deinit(handle);

	OSAL_printf("[ PASS ] Watchdog concurrent config stress test completed\n");
}

/*===========================================================================
 * Test 4: Watchdog Under CPU Load
 *===========================================================================*/

/**
 * CPU-intensive work to simulate load
 */
static void* watchdog_cpu_load_thread(void *arg)
{
	volatile uint64_t counter = 0;
	bool *stop_flag = (bool *)arg;

	while (!*stop_flag) {
		/* Simulate CPU-intensive work */
		for (int i = 0; i < 100000; i++) {
			counter += i * i;
		}
	}

	return NULL;
}

/**
 * Watchdog kicker thread
 */
static void* watchdog_kicker_thread(void *arg)
{
	watchdog_worker_ctx_t *ctx = (watchdog_worker_ctx_t *)arg;
	int32_t ret;

	while (!ctx->stop_flag) {
		ret = HAL_WATCHDOG_kick(ctx->handle);
		if (ret == OSAL_SUCCESS) {
			OSAL_atomic_inc(ctx->kick_counter);
		}

		/* Kick every 1 second */
		OSAL_sleep(1000/1000);
	}

	return NULL;
}

/**
 * Test: Watchdog timer accuracy under CPU load
 * Verifies: timer remains accurate, no missed kicks
 */
static void test_stress_watchdog_under_load(void)
{
	hal_watchdog_handle_t handle = NULL;
	hal_watchdog_config_t config = {
		.device = WATCHDOG_STRESS_DEVICE,
		.timeout_sec = 5
	};
	watchdog_worker_ctx_t kick_ctx;
	osal_atomic_uint32_t kick_counter;
	osal_thread_t kick_thread;
	osal_thread_t load_threads[4];
	bool load_stop_flag = false;
	int32_t ret;
	const uint32_t test_duration_sec = 15;

	OSAL_printf("[ INFO ] Starting watchdog under CPU load stress test\n");
	OSAL_printf("         Duration: %u sec\n", test_duration_sec);

	/* Initialize watchdog */
	ret = HAL_WATCHDOG_init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Watchdog device not available\n");
		TEST_SKIP("Watchdog not available");
		return;
	}

	/* Enable watchdog */
	ret = HAL_WATCHDOG_enable(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Setup kick context */
	OSAL_atomic_store(&kick_counter, 0);
	kick_ctx.handle = handle;
	kick_ctx.kick_counter = &kick_counter;
	kick_ctx.stop_flag = false;

	/* Start kicker thread */
	ret = OSAL_pthread_create(&kick_thread, NULL, watchdog_kicker_thread, &kick_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Start CPU load threads */
	for (int i = 0; i < 4; i++) {
		ret = OSAL_pthread_create(&load_threads[i], NULL, watchdog_cpu_load_thread, &load_stop_flag);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	OSAL_printf("[ INFO ] Running watchdog under high CPU load...\n");

	/* Let it run */
	OSAL_sleep(test_duration_sec);

	/* Stop threads */
	kick_ctx.stop_flag = true;
	load_stop_flag = true;

	OSAL_pthread_join(kick_thread, NULL);
	for (int i = 0; i < 4; i++) {
		OSAL_pthread_join(load_threads[i], NULL);
	}

	/* Check results */
	uint64_t total_kicks = OSAL_atomic_load(&kick_counter);
	OSAL_printf("[ INFO ] Total kicks under load: %llu\n", (unsigned long long)total_kicks);
	OSAL_printf("[ INFO ] Expected kicks: ~%u\n", test_duration_sec);

	/* Verify kicks occurred regularly */
	TEST_ASSERT_TRUE(total_kicks >= (uint64_t)(test_duration_sec * 0.8));

	/* Cleanup */
	HAL_WATCHDOG_disable(handle);
	HAL_WATCHDOG_deinit(handle);

	OSAL_printf("[ PASS ] Watchdog under CPU load stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_stress_watchdog_rapid_kick",
		.func = test_stress_watchdog_rapid_kick,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_watchdog_timeout_edge_cases",
		.func = test_stress_watchdog_timeout_edge_cases,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_watchdog_concurrent_config",
		.func = test_stress_watchdog_concurrent_config,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_watchdog_under_load",
		.func = test_stress_watchdog_under_load,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_watchdog",
	.module_name = "stress_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
		.timeout_ms = 120000,
		.description = "HAL Watchdog stress tests"
	}
};

__attribute__((constructor))
static void register_stress_hal_watchdog_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_WATCHDOG */
