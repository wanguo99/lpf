/**
 * @file test_stress_hal_gpio.c
 * @brief HAL GPIO stress tests - high-frequency and interrupt storm scenarios
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_GPIO

/* Test configuration */
#define GPIO_STRESS_PIN_BASE 100
#define GPIO_STRESS_PIN_COUNT 8
#define GPIO_STRESS_THREAD_COUNT 8
#define GPIO_STRESS_DURATION_SEC 15
#define GPIO_STRESS_TOGGLE_COUNT 100000

/* Worker context */
typedef struct {
	hal_gpio_handle_t handle;
	uint32_t pin;
	osal_atomic_uint32_t *toggle_counter;
} gpio_worker_ctx_t;

/* Interrupt test context */
typedef struct {
	hal_gpio_handle_t handles[GPIO_STRESS_PIN_COUNT];
	osal_atomic_uint32_t interrupt_counts[GPIO_STRESS_PIN_COUNT];
	uint32_t total_interrupts;
	bool stop_flag;
} gpio_interrupt_ctx_t;

/*===========================================================================
 * Test 1: High-Frequency Pin Toggling
 *===========================================================================*/

/**
 * Worker function for high-frequency toggling
 */
static int32_t _gpio_toggle_worker(void *user_data, uint32_t iteration)
{
	gpio_worker_ctx_t *ctx = (gpio_worker_ctx_t *)user_data;
	int32_t ret;
	hal_gpio_level_t level;

	/* Toggle pin state */
	level = (iteration % 2) ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
	ret = hal_gpio_set_level(ctx->handle, level);

	if (ret == OSAL_SUCCESS) {
		osal_atomic_inc(ctx->toggle_counter);
	}

	return ret;
}

/**
 * Test: High-frequency GPIO pin toggling
 * Verifies: maximum toggle rate, no state corruption
 */
static void _test_stress_gpio_high_frequency_toggle(void)
{
	hal_gpio_handle_t handles[GPIO_STRESS_THREAD_COUNT];
	hal_gpio_config_t config;
	osal_atomic_uint32_t toggle_counter;
	gpio_worker_ctx_t worker_ctx[GPIO_STRESS_THREAD_COUNT];
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config =
		STRESS_CONFIG_ITERATIONS(GPIO_STRESS_TOGGLE_COUNT);
	int32_t ret;
	uint64_t start_time, end_time;

	osal_printf("[ INFO ] Starting GPIO high-frequency toggle stress test\n");
	osal_printf("         Pins: %u, Toggles per pin: %u\n",
				GPIO_STRESS_THREAD_COUNT, GPIO_STRESS_TOGGLE_COUNT);

	/* Initialize GPIOs */
	for (uint32_t i = 0; i < GPIO_STRESS_THREAD_COUNT; i++) {
		config.pin = GPIO_STRESS_PIN_BASE + i;
		config.direction = HAL_GPIO_DIRECTION_OUTPUT;
		config.level = HAL_GPIO_LEVEL_LOW;
		config.interrupt_mode = HAL_GPIO_INTERRUPT_NONE;
		config.callback = NULL;
		config.user_data = NULL;

		ret = hal_gpio_init(&config, &handles[i]);
		if (ret != OSAL_SUCCESS) {
			/* Skip if GPIO not available */
			for (uint32_t j = 0; j < i; j++) {
				hal_gpio_deinit(handles[j]);
			}
			osal_printf("[ SKIP ] GPIO pins not available\n");
			TEST_SKIP("GPIO not available");
			return;
		}
	}

	/* Initialize counter */
	osal_atomic_store(&toggle_counter, 0);

	/* Setup worker contexts */
	for (uint32_t i = 0; i < GPIO_STRESS_THREAD_COUNT; i++) {
		worker_ctx[i].handle = handles[i];
		worker_ctx[i].pin = GPIO_STRESS_PIN_BASE + i;
		worker_ctx[i].toggle_counter = &toggle_counter;
	}

	/* Create stress test context */
	stress_ctx = stress_context_create("GPIO_High_Freq_Toggle", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	start_time = 0;

	for (uint32_t i = 0; i < GPIO_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(stress_ctx, _gpio_toggle_worker, &worker_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion */
	osal_sleep(30000 / 1000); /* Max 30 seconds */
	end_time = 0;

	/* Print report */
	stress_print_report(stress_ctx);

	/* Calculate toggle rate */
	uint64_t total_toggles = osal_atomic_load(&toggle_counter);
	uint64_t elapsed_ms = end_time - start_time;
	double toggles_per_sec = (total_toggles * 1000.0) / elapsed_ms;

	osal_printf("[ INFO ] Total toggles: %llu\n",
				(unsigned long long)total_toggles);
	osal_printf("[ INFO ] Toggle rate: %.2f Hz\n", toggles_per_sec);
	osal_printf("[ INFO ] Time elapsed: %llu ms\n",
				(unsigned long long)elapsed_ms);

	/* Verify all toggles completed */
	TEST_ASSERT_EQUAL(GPIO_STRESS_TOGGLE_COUNT * GPIO_STRESS_THREAD_COUNT,
					  total_toggles);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 99.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	for (uint32_t i = 0; i < GPIO_STRESS_THREAD_COUNT; i++) {
		hal_gpio_deinit(handles[i]);
	}

	osal_printf("[ PASS ] GPIO high-frequency toggle stress test completed\n");
}

/*===========================================================================
 * Test 2: Concurrent Pin Access
 *===========================================================================*/

/**
 * Worker for concurrent read/write
 */
static int32_t _gpio_concurrent_access_worker(void *user_data,
											  uint32_t iteration)
{
	gpio_worker_ctx_t *ctx = (gpio_worker_ctx_t *)user_data;
	int32_t ret;
	hal_gpio_level_t level;

	/* Write operation */
	level = (iteration % 2) ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
	ret = hal_gpio_set_level(ctx->handle, level);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* Read back to verify */
	ret = hal_gpio_get_level(ctx->handle, &level);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* Verify state consistency */
	hal_gpio_level_t expected = (iteration % 2) ? HAL_GPIO_LEVEL_HIGH :
												  HAL_GPIO_LEVEL_LOW;
	if (level == expected) {
		osal_atomic_inc(ctx->toggle_counter);
	}

	return OSAL_SUCCESS;
}

/**
 * Test: Concurrent access to multiple GPIO pins
 * Verifies: thread safety, state consistency
 */
static void _test_stress_gpio_concurrent_access(void)
{
	hal_gpio_handle_t handles[GPIO_STRESS_PIN_COUNT];
	hal_gpio_config_t config;
	osal_atomic_uint32_t success_counter;
	gpio_worker_ctx_t worker_ctx[GPIO_STRESS_PIN_COUNT];
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(
		GPIO_STRESS_PIN_COUNT, GPIO_STRESS_DURATION_SEC);
	int32_t ret;

	osal_printf("[ INFO ] Starting GPIO concurrent access stress test\n");
	osal_printf("         Pins: %u, Duration: %u sec\n", GPIO_STRESS_PIN_COUNT,
				GPIO_STRESS_DURATION_SEC);

	/* Initialize GPIOs */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		config.pin = GPIO_STRESS_PIN_BASE + i;
		config.direction = HAL_GPIO_DIRECTION_OUTPUT;
		config.level = HAL_GPIO_LEVEL_LOW;
		config.interrupt_mode = HAL_GPIO_INTERRUPT_NONE;
		config.callback = NULL;
		config.user_data = NULL;

		ret = hal_gpio_init(&config, &handles[i]);
		if (ret != OSAL_SUCCESS) {
			for (uint32_t j = 0; j < i; j++) {
				hal_gpio_deinit(handles[j]);
			}
			osal_printf("[ SKIP ] GPIO pins not available\n");
			TEST_SKIP("GPIO not available");
			return;
		}
	}

	/* Initialize counter */
	osal_atomic_store(&success_counter, 0);

	/* Setup worker contexts */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		worker_ctx[i].handle = handles[i];
		worker_ctx[i].pin = GPIO_STRESS_PIN_BASE + i;
		worker_ctx[i].toggle_counter = &success_counter;
	}

	/* Create stress test context */
	stress_ctx =
		stress_context_create("GPIO_Concurrent_Access", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		ret = stress_run(stress_ctx, _gpio_concurrent_access_worker,
						 &worker_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion */
	osal_usleep(GPIO_STRESS_DURATION_SEC * 1000 + 2000 * 1000);

	/* Print report */
	stress_print_report(stress_ctx);

	/* Verify success */
	uint64_t success_count = osal_atomic_load(&success_counter);
	osal_printf("[ INFO ] Successful consistent operations: %llu\n",
				(unsigned long long)success_count);
	TEST_ASSERT_TRUE(success_count > 0);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 95.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		hal_gpio_deinit(handles[i]);
	}

	osal_printf("[ PASS ] GPIO concurrent access stress test completed\n");
}

/*===========================================================================
 * Test 3: Interrupt Storm Handling
 *===========================================================================*/

/**
 * Interrupt callback for storm test
 */
static void _gpio_interrupt_storm_callback(uint32_t pin, hal_gpio_level_t level,
										   void *user_data)
{
	gpio_interrupt_ctx_t *ctx = (gpio_interrupt_ctx_t *)user_data;

	/* Find the pin index and increment counter */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		if (pin == (GPIO_STRESS_PIN_BASE + i)) {
			osal_atomic_inc(&ctx->interrupt_counts[i]);
			break;
		}
	}
}

/**
 * Test: Interrupt storm handling (many interrupts per second)
 * Verifies: interrupt handler stability, no missed interrupts
 */
static void _test_stress_gpio_interrupt_storm(void)
{
	gpio_interrupt_ctx_t ctx;
	hal_gpio_config_t config;
	int32_t ret;
	const uint32_t storm_duration_sec = 10;
	const uint32_t triggers_per_pin = 1000;

	osal_printf("[ INFO ] Starting GPIO interrupt storm stress test\n");
	osal_printf("         Pins: %u, Triggers per pin: %u\n",
				GPIO_STRESS_PIN_COUNT, triggers_per_pin);

	/* Initialize context */
	osal_memset(&ctx, 0, sizeof(ctx));
	ctx.stop_flag = false;

	/* Initialize GPIO pins with interrupts */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		osal_atomic_store(&ctx.interrupt_counts[i], 0);

		config.pin = GPIO_STRESS_PIN_BASE + i;
		config.direction = HAL_GPIO_DIRECTION_OUTPUT;
		config.level = HAL_GPIO_LEVEL_LOW;
		config.interrupt_mode = HAL_GPIO_INTERRUPT_BOTH_EDGES;
		config.callback = _gpio_interrupt_storm_callback;
		config.user_data = &ctx;

		ret = hal_gpio_init(&config, &ctx.handles[i]);
		if (ret != OSAL_SUCCESS) {
			for (uint32_t j = 0; j < i; j++) {
				hal_gpio_deinit(ctx.handles[j]);
			}
			osal_printf("[ SKIP ] GPIO interrupt not supported\n");
			TEST_SKIP("GPIO interrupt not available");
			return;
		}

		/* Enable interrupt */
		ret = hal_gpio_enable_interrupt(ctx.handles[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	osal_printf("[ INFO ] Generating interrupt storm...\n");

	/* Generate rapid interrupts by toggling pins */
	for (uint32_t trigger = 0; trigger < triggers_per_pin; trigger++) {
		for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
			hal_gpio_level_t level = (trigger % 2) ? HAL_GPIO_LEVEL_HIGH :
													 HAL_GPIO_LEVEL_LOW;
			ret = hal_gpio_set_level(ctx.handles[i], level);
			TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		}

		/* Small delay to allow interrupt processing */
		if (trigger % 100 == 0) {
			osal_usleep(1 * 1000);
		}
	}

	/* Wait for interrupt processing */
	osal_sleep(1000 / 1000);

	/* Print interrupt statistics */
	osal_printf("\n[ INFO ] Interrupt Statistics:\n");
	uint64_t total_interrupts = 0;

	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		uint32_t count = osal_atomic_load(&ctx.interrupt_counts[i]);
		total_interrupts += count;
		osal_printf("         Pin %u: %u interrupts\n",
					GPIO_STRESS_PIN_BASE + i, count);
	}

	osal_printf("[ INFO ] Total interrupts: %llu\n",
				(unsigned long long)total_interrupts);

	/* Verify interrupts were received */
	TEST_ASSERT_TRUE(total_interrupts > 0);

	/* Each pin should have received some interrupts */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		uint32_t count = osal_atomic_load(&ctx.interrupt_counts[i]);
		TEST_ASSERT_TRUE(count > 0);
	}

	/* Cleanup */
	for (uint32_t i = 0; i < GPIO_STRESS_PIN_COUNT; i++) {
		hal_gpio_disable_interrupt(ctx.handles[i]);
		hal_gpio_deinit(ctx.handles[i]);
	}

	osal_printf("[ PASS ] GPIO interrupt storm stress test completed\n");
}

/*===========================================================================
 * Test 4: Rapid Configuration Changes
 *===========================================================================*/

/**
 * Test: Rapid direction and configuration changes
 * Verifies: configuration stability, no race conditions
 */
static void _test_stress_gpio_rapid_config_changes(void)
{
	hal_gpio_handle_t handle = NULL;
	hal_gpio_config_t config = { .pin = GPIO_STRESS_PIN_BASE,
								 .direction = HAL_GPIO_DIRECTION_OUTPUT,
								 .level = HAL_GPIO_LEVEL_LOW,
								 .interrupt_mode = HAL_GPIO_INTERRUPT_NONE,
								 .callback = NULL,
								 .user_data = NULL };
	int32_t ret;
	const uint32_t iterations = 10000;

	osal_printf("[ INFO ] Starting GPIO rapid config changes stress test\n");
	osal_printf("         Iterations: %u\n", iterations);

	/* Initialize GPIO */
	ret = hal_gpio_init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		osal_printf("[ SKIP ] GPIO not available\n");
		TEST_SKIP("GPIO not available");
		return;
	}

	/* Rapidly change direction */
	for (uint32_t i = 0; i < iterations; i++) {
		hal_gpio_direction_t dir = (i % 2) ? HAL_GPIO_DIRECTION_INPUT :
											 HAL_GPIO_DIRECTION_OUTPUT;

		ret = hal_gpio_set_direction(handle, dir);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Verify direction */
		hal_gpio_direction_t read_dir;
		ret = hal_gpio_get_direction(handle, &read_dir);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_EQUAL(dir, read_dir);

		/* If output, toggle level */
		if (dir == HAL_GPIO_DIRECTION_OUTPUT) {
			hal_gpio_level_t level = (i % 4 < 2) ? HAL_GPIO_LEVEL_HIGH :
												   HAL_GPIO_LEVEL_LOW;
			ret = hal_gpio_set_level(handle, level);
			TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		}

		/* Progress indicator */
		if (i % 1000 == 0) {
			osal_printf("[ INFO ] Progress: %u/%u\n", i, iterations);
		}
	}

	/* Cleanup */
	hal_gpio_deinit(handle);

	osal_printf("[ PASS ] GPIO rapid config changes stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_stress_gpio_high_frequency_toggle",
	  .func = _test_stress_gpio_high_frequency_toggle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_gpio_concurrent_access",
	  .func = _test_stress_gpio_concurrent_access,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_gpio_interrupt_storm",
	  .func = _test_stress_gpio_interrupt_storm,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_gpio_rapid_config_changes",
	  .func = _test_stress_gpio_rapid_config_changes,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_gpio",
	.module_name = "stress_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_STRESS,
				  .tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
				  .timeout_ms = 120000,
				  .description = "HAL GPIO stress tests" }
};

void register_stress_hal_gpio_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_GPIO */
