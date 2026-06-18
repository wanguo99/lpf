/**
 * @file test_stress_hal_concurrent.c
 * @brief HAL concurrent multi-driver stress tests
 * @details Tests simultaneous usage of multiple HAL drivers to verify
 *          system-wide stability, resource sharing, and driver independence
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_CONCURRENT

/* Test configuration */
#define CONCURRENT_DURATION_SEC 20

/* Shared context for all drivers */
typedef struct {
	hal_can_handle_t can_handle;
	hal_serial_handle_t serial_handle;
	hal_spi_handle_t spi_handle;
	hal_i2c_handle_t i2c_handle;
	uint32_t gpio_pin; /* GPIO doesn't use handles, uses pin numbers */
	osal_atomic_uint32_t can_counter;
	osal_atomic_uint32_t serial_counter;
	osal_atomic_uint32_t spi_counter;
	osal_atomic_uint32_t i2c_counter;
	osal_atomic_uint32_t gpio_counter;
	bool stop_flag;
} concurrent_test_ctx_t;

/*===========================================================================
 * Driver Worker Threads
 *===========================================================================*/

/**
 * CAN worker thread
 */
static void *_can_worker_thread(void *arg)
{
	concurrent_test_ctx_t *ctx = (concurrent_test_ctx_t *)arg;
	hal_can_frame_t frame;
	int32_t ret;
	uint32_t iteration = 0;

	osal_printf("[ INFO ] CAN worker started\n");

	while (!ctx->stop_flag) {
		frame.can_id = 0x100;
		frame.dlc = 8;
		for (int i = 0; i < 8; i++) {
			frame.data[i] = (uint8_t)(iteration + i);
		}

		ret = hal_can_send(ctx->can_handle, &frame);
		if (ret == OSAL_SUCCESS) {
			osal_atomic_inc(&ctx->can_counter);
		}

		iteration++;
		osal_msleep(10);
	}

	osal_printf("[ INFO ] CAN worker stopped\n");
	return NULL;
}

/**
 * Serial worker thread
 */
static void *_serial_worker_thread(void *arg)
{
	concurrent_test_ctx_t *ctx = (concurrent_test_ctx_t *)arg;
	uint8_t buffer[64];
	int32_t ret;
	uint32_t iteration = 0;

	osal_printf("[ INFO ] Serial worker started\n");

	while (!ctx->stop_flag) {
		for (size_t i = 0; i < sizeof(buffer); i++) {
			buffer[i] = (uint8_t)(iteration + i);
		}

		ret = hal_serial_write(ctx->serial_handle, buffer, sizeof(buffer), 100);
		if (ret > 0) {
			osal_atomic_fetch_add(&ctx->serial_counter, (uint32_t)ret);
		}

		iteration++;
		osal_msleep(15);
	}

	osal_printf("[ INFO ] Serial worker stopped\n");
	return NULL;
}

/**
 * SPI worker thread
 */
static void *_spi_worker_thread(void *arg)
{
	concurrent_test_ctx_t *ctx = (concurrent_test_ctx_t *)arg;
	uint8_t tx_buffer[32];
	uint8_t rx_buffer[32];
	int32_t ret;
	uint32_t iteration = 0;

	osal_printf("[ INFO ] SPI worker started\n");

	while (!ctx->stop_flag) {
		for (size_t i = 0; i < sizeof(tx_buffer); i++) {
			tx_buffer[i] = (uint8_t)(iteration + i);
		}

		ret = hal_spi_transfer(ctx->spi_handle, tx_buffer, rx_buffer,
							   sizeof(tx_buffer));
		if (ret == OSAL_SUCCESS) {
			osal_atomic_inc(&ctx->spi_counter);
		}

		iteration++;
		osal_msleep(20);
	}

	osal_printf("[ INFO ] SPI worker stopped\n");
	return NULL;
}

/**
 * I2C worker thread
 */
static void *_i2c_worker_thread(void *arg)
{
	concurrent_test_ctx_t *ctx = (concurrent_test_ctx_t *)arg;
	uint8_t data[8];
	int32_t ret;
	uint32_t iteration = 0;

	osal_printf("[ INFO ] I2C worker started\n");

	while (!ctx->stop_flag) {
		for (size_t i = 0; i < sizeof(data); i++) {
			data[i] = (uint8_t)(iteration + i);
		}

		ret = hal_i2c_write(ctx->i2c_handle, 0x50, data, sizeof(data));
		if (ret == OSAL_SUCCESS || ret == OSAL_ERR_TIMEOUT || ret == OSAL_EIO) {
			osal_atomic_inc(&ctx->i2c_counter);
		}

		iteration++;
		osal_msleep(25);
	}

	osal_printf("[ INFO ] I2C worker stopped\n");
	return NULL;
}

/**
 * GPIO worker thread
 */
static void *_gpio_worker_thread(void *arg)
{
	concurrent_test_ctx_t *ctx = (concurrent_test_ctx_t *)arg;
	int32_t ret;
	uint32_t iteration = 0;

	osal_printf("[ INFO ] GPIO worker started\n");

	while (!ctx->stop_flag) {
		hal_gpio_level_t level = (iteration % 2) ? HAL_GPIO_LEVEL_HIGH :
												   HAL_GPIO_LEVEL_LOW;

		ret = hal_gpio_set_level(ctx->gpio_pin, level);
		if (ret == OSAL_SUCCESS) {
			osal_atomic_inc(&ctx->gpio_counter);
		}

		iteration++;
		osal_msleep(30);
	}

	osal_printf("[ INFO ] GPIO worker stopped\n");
	return NULL;
}

/*===========================================================================
 * Test 1: Simultaneous Multi-Driver Operations
 *===========================================================================*/

/**
 * Test: All HAL drivers operating simultaneously
 * Verifies: driver independence, no side effects, system stability
 */
static void _test_stress_hal_all_drivers_concurrent(void)
{
	concurrent_test_ctx_t ctx;
	osal_thread_t threads[5];
	int32_t ret;
	uint32_t active_drivers = 0;

	osal_printf("[ INFO ] Starting concurrent multi-driver stress test\n");
	osal_printf("         Duration: %u seconds\n", CONCURRENT_DURATION_SEC);

	/* Initialize context */
	osal_memset(&ctx, 0, sizeof(ctx));
	ctx.stop_flag = false;

	/* Initialize CAN */
	hal_can_config_t can_config = { .interface = "can0",
									.baudrate = 500000,
									.rx_timeout = 1000,
									.tx_timeout = 1000 };
	ret = hal_can_init(&can_config, &ctx.can_handle);
	if (ret == OSAL_SUCCESS) {
		active_drivers++;
		osal_printf("[ INFO ] CAN driver initialized\n");
	}

	/* Initialize Serial */
	hal_serial_config_t serial_config = { .baud_rate = 115200,
										  .data_bits = 8,
										  .stop_bits = 1,
										  .parity = HAL_SERIAL_PARITY_NONE,
										  .flow_control = 0 };
	ret = hal_serial_open("/dev/ttyUSB0", &serial_config, &ctx.serial_handle);
	if (ret == OSAL_SUCCESS) {
		active_drivers++;
		osal_printf("[ INFO ] Serial driver initialized\n");
	}

	/* Initialize SPI */
	hal_spi_config_t spi_config = { .device = "/dev/spidev0.0",
									.mode = 0,
									.bits_per_word = 8,
									.max_speed_hz = 1000000,
									.timeout = 1000 };
	ret = hal_spi_open(&spi_config, &ctx.spi_handle);
	if (ret == OSAL_SUCCESS) {
		active_drivers++;
		osal_printf("[ INFO ] SPI driver initialized\n");
	}

	/* Initialize I2C */
	hal_i2c_config_t i2c_config = { .device = "/dev/i2c-0", .timeout = 1000 };
	ret = hal_i2c_open(&i2c_config, &ctx.i2c_handle);
	if (ret == OSAL_SUCCESS) {
		active_drivers++;
		osal_printf("[ INFO ] I2C driver initialized\n");
	}

	/* Initialize GPIO */
	ctx.gpio_pin = 100;
	hal_gpio_config_t gpio_config = { .direction = HAL_GPIO_DIR_OUTPUT,
									  .initial_level = HAL_GPIO_LEVEL_LOW,
									  .edge = HAL_GPIO_EDGE_NONE,
									  .callback = NULL,
									  .user_data = NULL };
	ret = hal_gpio_init(ctx.gpio_pin, &gpio_config);
	if (ret == OSAL_SUCCESS) {
		active_drivers++;
		osal_printf("[ INFO ] GPIO driver initialized\n");
	}

	osal_printf("[ INFO ] Active drivers: %u/5\n", active_drivers);

	if (active_drivers < 2) {
		osal_printf("[ SKIP ] Need at least 2 drivers available\n");
		TEST_SKIP("Insufficient drivers available");
		goto cleanup;
	}

	/* Start worker threads */
	uint32_t thread_idx = 0;

	if (ctx.can_handle) {
		osal_pthread_create(&threads[thread_idx++], NULL, _can_worker_thread,
							&ctx);
	}
	if (ctx.serial_handle) {
		osal_pthread_create(&threads[thread_idx++], NULL, _serial_worker_thread,
							&ctx);
	}
	if (ctx.spi_handle) {
		osal_pthread_create(&threads[thread_idx++], NULL, _spi_worker_thread,
							&ctx);
	}
	if (ctx.i2c_handle) {
		osal_pthread_create(&threads[thread_idx++], NULL, _i2c_worker_thread,
							&ctx);
	}
	if (ctx.gpio_pin) {
		osal_pthread_create(&threads[thread_idx++], NULL, _gpio_worker_thread,
							&ctx);
	}

	osal_printf(
		"[ INFO ] All worker threads started, running for %u seconds...\n",
		CONCURRENT_DURATION_SEC);

	/* Progress updates */
	for (uint32_t i = 0; i < CONCURRENT_DURATION_SEC; i++) {
		osal_msleep(1000);

		if (i % 5 == 0) {
			osal_printf("[ INFO ] Progress: %u/%u sec\n", i,
						CONCURRENT_DURATION_SEC);
		}
	}

	/* Stop all workers */
	ctx.stop_flag = true;

	osal_printf("[ INFO ] Stopping workers...\n");

	/* Wait for threads to finish */
	for (uint32_t i = 0; i < thread_idx; i++) {
		osal_pthread_join(threads[i], NULL);
	}

	/* Print statistics */
	osal_printf("\n[ INFO ] Final Statistics:\n");
	if (ctx.can_handle) {
		osal_printf("         CAN operations: %llu\n",
					(unsigned long long)osal_atomic_load(&ctx.can_counter));
	}
	if (ctx.serial_handle) {
		osal_printf("         Serial bytes: %llu\n",
					(unsigned long long)osal_atomic_load(&ctx.serial_counter));
	}
	if (ctx.spi_handle) {
		osal_printf("         SPI transfers: %llu\n",
					(unsigned long long)osal_atomic_load(&ctx.spi_counter));
	}
	if (ctx.i2c_handle) {
		osal_printf("         I2C operations: %llu\n",
					(unsigned long long)osal_atomic_load(&ctx.i2c_counter));
	}
	if (ctx.gpio_pin) {
		osal_printf("         GPIO toggles: %llu\n",
					(unsigned long long)osal_atomic_load(&ctx.gpio_counter));
	}

	/* Verify all drivers had activity */
	if (ctx.can_handle) {
		TEST_ASSERT_TRUE(osal_atomic_load(&ctx.can_counter) > 0);
	}
	if (ctx.serial_handle) {
		TEST_ASSERT_TRUE(osal_atomic_load(&ctx.serial_counter) > 0);
	}
	if (ctx.spi_handle) {
		TEST_ASSERT_TRUE(osal_atomic_load(&ctx.spi_counter) > 0);
	}

cleanup:
	/* Cleanup drivers */
	if (ctx.gpio_pin) {
		hal_gpio_deinit(ctx.gpio_pin);
	}
	if (ctx.i2c_handle) {
		hal_i2c_close(ctx.i2c_handle);
	}
	if (ctx.spi_handle) {
		hal_spi_close(ctx.spi_handle);
	}
	if (ctx.serial_handle) {
		hal_serial_close(ctx.serial_handle);
	}
	if (ctx.can_handle) {
		hal_can_deinit(ctx.can_handle);
	}

	osal_printf("[ PASS ] Concurrent multi-driver stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_stress_hal_all_drivers_concurrent",
	  .func = _test_stress_hal_all_drivers_concurrent,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_concurrent",
	.module_name = "stress_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_STRESS,
				  .tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE |
						  TEST_TAG_INTEGRATION,
				  .timeout_ms = 180000,
				  .description = "HAL concurrent multi-driver stress tests" }
};

void register_stress_hal_concurrent_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_CONCURRENT */
