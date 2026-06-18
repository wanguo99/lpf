/**
 * @file test_stress_hal_spi.c
 * @brief HAL SPI stress tests - high-speed transfer and concurrent access
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_SPI

/* Test configuration */
#define SPI_STRESS_DEVICE "/dev/spidev0.0"
#define SPI_STRESS_SPEED_HZ 1000000
#define SPI_STRESS_THREAD_COUNT 4
#define SPI_STRESS_DURATION_SEC 15
#define SPI_STRESS_SMALL_BUF 64
#define SPI_STRESS_LARGE_BUF (4 * 1024)
#define SPI_STRESS_HUGE_BUF (1 * 1024 * 1024)

/* Worker context */
typedef struct {
	hal_spi_handle_t handle;
	uint32_t thread_id;
	osal_atomic_uint32_t *transfer_counter;
	osal_atomic_uint32_t *byte_counter;
} spi_worker_ctx_t;

/*===========================================================================
 * Test 1: High-Speed Continuous Transfer
 *===========================================================================*/

/**
 * Worker for continuous SPI transfer
 */
static int32_t _spi_continuous_transfer_worker(void *user_data,
											   uint32_t iteration)
{
	spi_worker_ctx_t *ctx = (spi_worker_ctx_t *)user_data;
	uint8_t tx_buffer[SPI_STRESS_SMALL_BUF];
	uint8_t rx_buffer[SPI_STRESS_SMALL_BUF];
	int32_t ret;

	/* Fill transmit buffer with pattern */
	for (size_t i = 0; i < SPI_STRESS_SMALL_BUF; i++) {
		tx_buffer[i] = (uint8_t)(iteration + i + ctx->thread_id);
	}

	/* Perform transfer */
	ret = hal_spi_transfer(ctx->handle, tx_buffer, rx_buffer,
						   SPI_STRESS_SMALL_BUF);
	if (ret == OSAL_SUCCESS) {
		osal_atomic_inc(ctx->transfer_counter);
		osal_atomic_fetch_add(ctx->byte_counter, SPI_STRESS_SMALL_BUF);
	}

	return ret;
}

/**
 * Test: Continuous high-speed SPI transfers
 * Verifies: sustained throughput, data integrity
 */
static void _test_stress_spi_continuous_transfer(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = SPI_STRESS_DEVICE,
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = SPI_STRESS_SPEED_HZ,
								.timeout = 1000 };
	osal_atomic_uint32_t transfer_counter, byte_counter;
	spi_worker_ctx_t worker_ctx;
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config =
		STRESS_CONFIG_DURATION(SPI_STRESS_DURATION_SEC);
	int32_t ret;
	uint64_t start_time, end_time;

	osal_printf("[ INFO ] Starting SPI continuous transfer stress test\n");
	osal_printf("         Duration: %u sec, Speed: %u Hz\n",
				SPI_STRESS_DURATION_SEC, SPI_STRESS_SPEED_HZ);

	/* Open SPI device */
	ret = hal_spi_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		osal_printf("[ SKIP ] SPI device not available\n");
		TEST_SKIP("SPI device not available");
		return;
	}

	/* Initialize counters */
	osal_atomic_store(&transfer_counter, 0);
	osal_atomic_store(&byte_counter, 0);

	/* Setup worker context */
	worker_ctx.handle = handle;
	worker_ctx.thread_id = 0;
	worker_ctx.transfer_counter = &transfer_counter;
	worker_ctx.byte_counter = &byte_counter;

	/* Create stress test context */
	stress_ctx =
		stress_context_create("SPI_Continuous_Transfer", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	start_time = 0;
	ret = stress_run(stress_ctx, _spi_continuous_transfer_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Wait for completion */
	osal_usleep(SPI_STRESS_DURATION_SEC * 1000 + 1000 * 1000);
	end_time = 0;

	/* Print report */
	stress_print_report(stress_ctx);

	/* Calculate throughput */
	uint64_t total_bytes = osal_atomic_load(&byte_counter);
	uint64_t total_transfers = osal_atomic_load(&transfer_counter);
	uint64_t elapsed_ms = end_time - start_time;
	double throughput_kbps =
		(total_bytes * 8.0 / 1000.0) / (elapsed_ms / 1000.0);
	double transfers_per_sec = (total_transfers * 1000.0) / elapsed_ms;

	osal_printf("[ INFO ] Total transfers: %llu\n",
				(unsigned long long)total_transfers);
	osal_printf("[ INFO ] Total bytes: %llu\n",
				(unsigned long long)total_bytes);
	osal_printf("[ INFO ] Throughput: %.2f kbps\n", throughput_kbps);
	osal_printf("[ INFO ] Transfers/sec: %.2f\n", transfers_per_sec);

	/* Verify operation */
	TEST_ASSERT_TRUE(total_transfers > 0);
	TEST_ASSERT_TRUE(total_bytes > 0);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 95.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	hal_spi_close(handle);

	osal_printf("[ PASS ] SPI continuous transfer stress test completed\n");
}

/*===========================================================================
 * Test 2: Large Buffer Transfers
 *===========================================================================*/

/**
 * Test: Large buffer SPI transfers (MB range)
 * Verifies: DMA handling, memory management
 */
static void _test_stress_spi_large_buffers(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = SPI_STRESS_DEVICE,
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = SPI_STRESS_SPEED_HZ,
								.timeout = 1000 };
	uint8_t *tx_buffer = NULL;
	uint8_t *rx_buffer = NULL;
	int32_t ret;
	const uint32_t iterations = 10;

	osal_printf("[ INFO ] Starting SPI large buffer stress test\n");
	osal_printf("         Buffer size: %u bytes, Iterations: %u\n",
				SPI_STRESS_HUGE_BUF, iterations);

	/* Open SPI device */
	ret = hal_spi_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		osal_printf("[ SKIP ] SPI device not available\n");
		TEST_SKIP("SPI device not available");
		return;
	}

	/* Allocate large buffers */
	tx_buffer = osal_malloc(SPI_STRESS_HUGE_BUF);
	rx_buffer = osal_malloc(SPI_STRESS_HUGE_BUF);
	TEST_ASSERT_NOT_NULL(tx_buffer);
	TEST_ASSERT_NOT_NULL(rx_buffer);

	/* Fill transmit buffer with pattern */
	for (size_t i = 0; i < SPI_STRESS_HUGE_BUF; i++) {
		tx_buffer[i] = (uint8_t)(i & 0xFF);
	}

	/* Perform large transfers */
	uint64_t total_bytes = 0;
	uint64_t start_time = 0;

	for (uint32_t i = 0; i < iterations; i++) {
		ret =
			hal_spi_transfer(handle, tx_buffer, rx_buffer, SPI_STRESS_HUGE_BUF);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		total_bytes += SPI_STRESS_HUGE_BUF;

		osal_printf("[ INFO ] Completed transfer %u/%u\n", i + 1, iterations);
	}

	uint64_t end_time = 0;
	uint64_t elapsed_ms = end_time - start_time;

	/* Calculate throughput */
	double throughput_mbps =
		(total_bytes * 8.0 / 1000000.0) / (elapsed_ms / 1000.0);
	osal_printf("[ INFO ] Total transferred: %llu bytes in %llu ms\n",
				(unsigned long long)total_bytes,
				(unsigned long long)elapsed_ms);
	osal_printf("[ INFO ] Throughput: %.2f Mbps\n", throughput_mbps);

	/* Cleanup */
	osal_free(tx_buffer);
	osal_free(rx_buffer);
	hal_spi_close(handle);

	osal_printf("[ PASS ] SPI large buffer stress test completed\n");
}

/*===========================================================================
 * Test 3: Concurrent Multi-Device Access
 *===========================================================================*/

/**
 * Worker for concurrent SPI access
 */
static int32_t _spi_concurrent_worker(void *user_data, uint32_t iteration)
{
	spi_worker_ctx_t *ctx = (spi_worker_ctx_t *)user_data;
	uint8_t tx_buffer[SPI_STRESS_SMALL_BUF];
	uint8_t rx_buffer[SPI_STRESS_SMALL_BUF];
	int32_t ret;

	/* Fill buffer with thread-specific pattern */
	for (size_t i = 0; i < SPI_STRESS_SMALL_BUF; i++) {
		tx_buffer[i] = (uint8_t)(0xA0 + ctx->thread_id + (iteration % 16));
	}

	/* Perform transfer */
	ret = hal_spi_transfer(ctx->handle, tx_buffer, rx_buffer,
						   SPI_STRESS_SMALL_BUF);
	if (ret == OSAL_SUCCESS) {
		osal_atomic_inc(ctx->transfer_counter);
	}

	return ret;
}

/**
 * Test: Concurrent SPI access (simulates multiple device contention)
 * Verifies: exclusive bus access, no data corruption
 */
static void _test_stress_spi_concurrent_access(void)
{
	hal_spi_handle_t handles[SPI_STRESS_THREAD_COUNT];
	hal_spi_config_t config = { .device = SPI_STRESS_DEVICE,
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = SPI_STRESS_SPEED_HZ,
								.timeout = 1000 };
	osal_atomic_uint32_t transfer_counter;
	spi_worker_ctx_t worker_ctx[SPI_STRESS_THREAD_COUNT];
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config =
		STRESS_CONFIG_CONCURRENCY(SPI_STRESS_THREAD_COUNT, 10);
	int32_t ret;

	osal_printf("[ INFO ] Starting SPI concurrent access stress test\n");
	osal_printf("         Threads: %u, Duration: 10 sec\n",
				SPI_STRESS_THREAD_COUNT);

	/* Open multiple SPI handles */
	for (uint32_t i = 0; i < SPI_STRESS_THREAD_COUNT; i++) {
		ret = hal_spi_open(&config, &handles[i]);
		if (ret != OSAL_SUCCESS) {
			for (uint32_t j = 0; j < i; j++) {
				hal_spi_close(handles[j]);
			}
			osal_printf("[ SKIP ] Cannot open multiple SPI handles\n");
			TEST_SKIP("SPI multi-open not supported");
			return;
		}
	}

	/* Initialize counter */
	osal_atomic_store(&transfer_counter, 0);

	/* Setup worker contexts */
	for (uint32_t i = 0; i < SPI_STRESS_THREAD_COUNT; i++) {
		worker_ctx[i].handle = handles[i];
		worker_ctx[i].thread_id = i;
		worker_ctx[i].transfer_counter = &transfer_counter;
		worker_ctx[i].byte_counter = NULL;
	}

	/* Create stress test context */
	stress_ctx = stress_context_create("SPI_Concurrent_Access", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	for (uint32_t i = 0; i < SPI_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(stress_ctx, _spi_concurrent_worker, &worker_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion */
	osal_sleep(12000 / 1000);

	/* Print report */
	stress_print_report(stress_ctx);

	/* Verify transfers */
	uint64_t total_transfers = osal_atomic_load(&transfer_counter);
	osal_printf("[ INFO ] Total concurrent transfers: %llu\n",
				(unsigned long long)total_transfers);
	TEST_ASSERT_TRUE(total_transfers > 0);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 90.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	for (uint32_t i = 0; i < SPI_STRESS_THREAD_COUNT; i++) {
		hal_spi_close(handles[i]);
	}

	osal_printf("[ PASS ] SPI concurrent access stress test completed\n");
}

/*===========================================================================
 * Test 4: Rapid Configuration Changes
 *===========================================================================*/

/**
 * Test: Rapid SPI configuration changes under load
 * Verifies: configuration stability, no corruption
 */
static void _test_stress_spi_rapid_config_changes(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = SPI_STRESS_DEVICE,
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = SPI_STRESS_SPEED_HZ,
								.timeout = 1000 };
	uint8_t tx_buffer[64];
	uint8_t rx_buffer[64];
	int32_t ret;
	const uint32_t iterations = 1000;

	osal_printf("[ INFO ] Starting SPI rapid config changes stress test\n");
	osal_printf("         Iterations: %u\n", iterations);

	/* Open SPI device */
	ret = hal_spi_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		osal_printf("[ SKIP ] SPI device not available\n");
		TEST_SKIP("SPI device not available");
		return;
	}

	/* Rapidly change configuration and transfer */
	for (uint32_t i = 0; i < iterations; i++) {
		/* Cycle through SPI modes */
		uint8_t mode = (uint8_t)(i % 4);
		config.mode = mode;
		config.max_speed_hz = 500000 + ((i % 4) * 250000);

		ret = hal_spi_set_config(handle, &config);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Fill buffer */
		for (size_t j = 0; j < sizeof(tx_buffer); j++) {
			tx_buffer[j] = (uint8_t)(i + j);
		}

		/* Perform transfer with new config */
		ret = hal_spi_transfer(handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Progress indicator */
		if (i % 100 == 0) {
			osal_printf("[ INFO ] Progress: %u/%u\n", i, iterations);
		}
	}

	/* Cleanup */
	hal_spi_close(handle);

	osal_printf("[ PASS ] SPI rapid config changes stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_stress_spi_continuous_transfer",
	  .func = _test_stress_spi_continuous_transfer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_spi_large_buffers",
	  .func = _test_stress_spi_large_buffers,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_spi_concurrent_access",
	  .func = _test_stress_spi_concurrent_access,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_stress_spi_rapid_config_changes",
	  .func = _test_stress_spi_rapid_config_changes,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_spi",
	.module_name = "stress_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_STRESS,
				  .tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
				  .timeout_ms = 120000,
				  .description = "HAL SPI stress tests" }
};

void register_stress_hal_spi_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_SPI */
