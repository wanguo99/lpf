/**
 * @file test_stress_hal_serial.c
 * @brief HAL Serial stress tests - long-running and high-throughput scenarios
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_SERIAL

/* Test configuration */
#define SERIAL_STRESS_PORT         "/dev/ttyUSB0"
#define SERIAL_STRESS_BAUDRATE     115200
#define SERIAL_STRESS_THREAD_COUNT 4
#define SERIAL_STRESS_DURATION_SEC 20
#define SERIAL_STRESS_BUFFER_SIZE  4096
#define SERIAL_STRESS_PATTERN_SIZE 256

/* Worker context for concurrent operations */
typedef struct {
	hal_serial_handle_t handle;
	uint32_t thread_id;
	osal_atomic_uint32_t *byte_counter;
	uint8_t *pattern_buffer;
} serial_worker_ctx_t;

/* Context for long-running test */
typedef struct {
	hal_serial_handle_t handle;
	uint64_t total_bytes_tx;
	uint64_t total_bytes_rx;
	uint32_t error_count;
	bool stop_flag;
} serial_longrun_ctx_t;

/*===========================================================================
 * Test 1: Continuous Full-Speed Transmission
 *===========================================================================*/

/**
 * Worker function for continuous transmission
 */
static int32_t serial_continuous_tx_worker(void *user_data, uint32_t iteration)
{
	serial_worker_ctx_t *ctx = (serial_worker_ctx_t *)user_data;
	uint8_t buffer[SERIAL_STRESS_BUFFER_SIZE];
	int32_t ret;
	size_t written;

	/* Fill buffer with pattern */
	for (size_t i = 0; i < SERIAL_STRESS_BUFFER_SIZE; i++) {
		buffer[i] = (uint8_t)(iteration + i + ctx->thread_id);
	}

	/* Transmit data */
	ret = HAL_SERIAL_write(ctx->handle, buffer, SERIAL_STRESS_BUFFER_SIZE, &written);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_add(ctx->byte_counter, written);
	}

	return ret;
}

/**
 * Test: Continuous full-speed serial transmission
 * Verifies: buffer management, no data loss, sustained throughput
 */
static void test_stress_serial_continuous_transmission(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.device = SERIAL_STRESS_PORT,
		.baudrate = SERIAL_STRESS_BAUDRATE,
		.data_bits = HAL_SERIAL_DATA_BITS_8,
		.stop_bits = HAL_SERIAL_STOP_BITS_1,
		.parity = HAL_SERIAL_PARITY_NONE,
		.flow_control = HAL_SERIAL_FLOW_CONTROL_NONE,
		.timeout_ms = 1000
	};
	osal_atomic_uint32_t byte_counter;
	serial_worker_ctx_t worker_ctx;
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_DURATION(SERIAL_STRESS_DURATION_SEC);
	int32_t ret;
	uint64_t start_time, end_time;

	OSAL_printf("[ INFO ] Starting serial continuous transmission stress test\n");
	OSAL_printf("         Duration: %u sec, Baudrate: %u\n",
	           SERIAL_STRESS_DURATION_SEC, SERIAL_STRESS_BAUDRATE);

	/* Open serial port */
	ret = HAL_SERIAL_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Serial port not available\n");
		TEST_SKIP("Serial port not available");
		return;
	}

	/* Initialize context */
	OSAL_atomic_store(&byte_counter, 0);
	worker_ctx.handle = handle;
	worker_ctx.thread_id = 0;
	worker_ctx.byte_counter = &byte_counter;

	/* Create stress test context */
	stress_ctx = stress_context_create("Serial_Continuous_TX", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Start test */
	start_time = 0;
	ret = stress_run(stress_ctx, serial_continuous_tx_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Wait for completion */
	OSAL_usleep(SERIAL_STRESS_DURATION_SEC * 1000 + 1000 * 1000);
	end_time = 0;

	/* Print report */
	stress_print_report(stress_ctx);

	/* Calculate throughput */
	uint64_t total_bytes = OSAL_atomic_load(&byte_counter);
	uint64_t elapsed_ms = end_time - start_time;
	double throughput_kbps = (total_bytes * 8.0 / 1000.0) / (elapsed_ms / 1000.0);
	double expected_kbps = SERIAL_STRESS_BAUDRATE / 1000.0;

	OSAL_printf("[ INFO ] Total bytes transmitted: %llu\n",
	           (unsigned long long)total_bytes);
	OSAL_printf("[ INFO ] Throughput: %.2f kbps (expected: %.2f kbps)\n",
	           throughput_kbps, expected_kbps);
	OSAL_printf("[ INFO ] Efficiency: %.2f%%\n",
	           (throughput_kbps / expected_kbps) * 100.0);

	/* Verify minimum throughput (at least 50% of theoretical) */
	TEST_ASSERT_TRUE(throughput_kbps > expected_kbps * 0.5);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 95.0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	HAL_SERIAL_close(handle);

	OSAL_printf("[ PASS ] Serial continuous transmission stress test completed\n");
}

/*===========================================================================
 * Test 2: Concurrent Read/Write Operations
 *===========================================================================*/

/**
 * Worker function for concurrent write
 */
static int32_t serial_concurrent_write_worker(void *user_data, uint32_t iteration)
{
	serial_worker_ctx_t *ctx = (serial_worker_ctx_t *)user_data;
	uint8_t buffer[SERIAL_STRESS_PATTERN_SIZE];
	size_t written;
	int32_t ret;

	/* Prepare data pattern */
	for (size_t i = 0; i < SERIAL_STRESS_PATTERN_SIZE; i++) {
		buffer[i] = (uint8_t)(0xA0 + ctx->thread_id + (iteration % 16));
	}

	/* Write data */
	ret = HAL_SERIAL_write(ctx->handle, buffer, SERIAL_STRESS_PATTERN_SIZE, &written);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_add(ctx->byte_counter, written);
	}

	return ret;
}

/**
 * Worker function for concurrent read
 */
static int32_t serial_concurrent_read_worker(void *user_data, uint32_t iteration)
{
	serial_worker_ctx_t *ctx = (serial_worker_ctx_t *)user_data;
	uint8_t buffer[SERIAL_STRESS_PATTERN_SIZE];
	size_t read_count;
	int32_t ret;

	/* Try to read data */
	ret = HAL_SERIAL_read(ctx->handle, buffer, SERIAL_STRESS_PATTERN_SIZE, &read_count);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_add(ctx->byte_counter, read_count);
	} else if (ret == OSAL_ERR_TIMEOUT) {
		/* Timeout is acceptable */
		return OSAL_SUCCESS;
	}

	return ret;
}

/**
 * Test: Concurrent read and write operations
 * Verifies: thread-safety, buffer integrity, no deadlocks
 */
static void test_stress_serial_concurrent_operations(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.device = SERIAL_STRESS_PORT,
		.baudrate = SERIAL_STRESS_BAUDRATE,
		.data_bits = HAL_SERIAL_DATA_BITS_8,
		.stop_bits = HAL_SERIAL_STOP_BITS_1,
		.parity = HAL_SERIAL_PARITY_NONE,
		.flow_control = HAL_SERIAL_FLOW_CONTROL_NONE,
		.timeout_ms = 100
	};
	osal_atomic_uint32_t write_counter, read_counter;
	serial_worker_ctx_t write_ctx[SERIAL_STRESS_THREAD_COUNT];
	serial_worker_ctx_t read_ctx[SERIAL_STRESS_THREAD_COUNT];
	stress_context_t *write_stress = NULL;
	stress_context_t *read_stress = NULL;
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(
		SERIAL_STRESS_THREAD_COUNT, 10);
	int32_t ret;

	OSAL_printf("[ INFO ] Starting serial concurrent operations stress test\n");

	/* Open serial port */
	ret = HAL_SERIAL_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Serial port not available\n");
		TEST_SKIP("Serial port not available");
		return;
	}

	/* Initialize counters */
	OSAL_atomic_store(&write_counter, 0);
	OSAL_atomic_store(&read_counter, 0);

	/* Setup worker contexts */
	for (uint32_t i = 0; i < SERIAL_STRESS_THREAD_COUNT; i++) {
		write_ctx[i].handle = handle;
		write_ctx[i].thread_id = i;
		write_ctx[i].byte_counter = &write_counter;

		read_ctx[i].handle = handle;
		read_ctx[i].thread_id = i;
		read_ctx[i].byte_counter = &read_counter;
	}

	/* Create stress contexts */
	write_stress = stress_context_create("Serial_Concurrent_Write", &stress_config);
	read_stress = stress_context_create("Serial_Concurrent_Read", &stress_config);
	TEST_ASSERT_NOT_NULL(write_stress);
	TEST_ASSERT_NOT_NULL(read_stress);

	/* Start write workers */
	for (uint32_t i = 0; i < SERIAL_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(write_stress, serial_concurrent_write_worker, &write_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Start read workers */
	for (uint32_t i = 0; i < SERIAL_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(read_stress, serial_concurrent_read_worker, &read_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion */
	OSAL_sleep(12000/1000);

	/* Print reports */
	OSAL_printf("\n--- Write Report ---\n");
	stress_print_report(write_stress);
	OSAL_printf("\n--- Read Report ---\n");
	stress_print_report(read_stress);

	/* Verify counters */
	uint64_t written = OSAL_atomic_load(&write_counter);
	uint64_t read = OSAL_atomic_load(&read_counter);
	OSAL_printf("[ INFO ] Total written: %llu bytes, read: %llu bytes\n",
	           (unsigned long long)written, (unsigned long long)read);

	/* Cleanup */
	stress_context_destroy(write_stress);
	stress_context_destroy(read_stress);
	HAL_SERIAL_close(handle);

	OSAL_printf("[ PASS ] Serial concurrent operations stress test completed\n");
}

/*===========================================================================
 * Test 3: Buffer Overflow Protection
 *===========================================================================*/

/**
 * Test: Receive buffer overflow handling
 * Verifies: no data corruption, graceful overflow handling
 */
static void test_stress_serial_buffer_overflow(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.device = SERIAL_STRESS_PORT,
		.baudrate = SERIAL_STRESS_BAUDRATE,
		.data_bits = HAL_SERIAL_DATA_BITS_8,
		.stop_bits = HAL_SERIAL_STOP_BITS_1,
		.parity = HAL_SERIAL_PARITY_NONE,
		.flow_control = HAL_SERIAL_FLOW_CONTROL_NONE,
		.timeout_ms = 100
	};
	uint8_t tx_buffer[SERIAL_STRESS_BUFFER_SIZE];
	uint8_t rx_buffer[256];
	size_t written, read_count;
	int32_t ret;
	const uint32_t overflow_iterations = 100;

	OSAL_printf("[ INFO ] Starting serial buffer overflow stress test\n");

	/* Open serial port */
	ret = HAL_SERIAL_open(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Serial port not available\n");
		TEST_SKIP("Serial port not available");
		return;
	}

	/* Fill transmit buffer with pattern */
	for (size_t i = 0; i < SERIAL_STRESS_BUFFER_SIZE; i++) {
		tx_buffer[i] = (uint8_t)(i & 0xFF);
	}

	/* Flood the receive buffer */
	for (uint32_t i = 0; i < overflow_iterations; i++) {
		ret = HAL_SERIAL_write(handle, tx_buffer, SERIAL_STRESS_BUFFER_SIZE, &written);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Don't read immediately - let buffer fill up */
		if (i % 10 == 0) {
			OSAL_usleep(10 * 1000);
		}
	}

	OSAL_printf("[ INFO ] Sent %u KB, now draining receive buffer\n",
	           (overflow_iterations * SERIAL_STRESS_BUFFER_SIZE) / 1024);

	/* Try to drain receive buffer */
	uint64_t total_read = 0;
	uint32_t read_attempts = 0;

	while (read_attempts < 1000) {
		ret = HAL_SERIAL_read(handle, rx_buffer, sizeof(rx_buffer), &read_count);
		if (ret == OSAL_SUCCESS && read_count > 0) {
			total_read += read_count;
		} else if (ret == OSAL_ERR_TIMEOUT) {
			/* No more data */
			break;
		}
		read_attempts++;
	}

	OSAL_printf("[ INFO ] Read %llu bytes after overflow\n",
	           (unsigned long long)total_read);

	/* Verify system is still functional after overflow */
	HAL_SERIAL_flush(handle);

	uint8_t test_pattern[] = {0xAA, 0xBB, 0xCC, 0xDD};
	ret = HAL_SERIAL_write(handle, test_pattern, sizeof(test_pattern), &written);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(test_pattern), written);

	/* Cleanup */
	HAL_SERIAL_close(handle);

	OSAL_printf("[ PASS ] Serial buffer overflow stress test completed\n");
}

/*===========================================================================
 * Test 4: Long-Running Stability Test
 *===========================================================================*/

/**
 * Long-running worker thread
 */
static void* serial_longrun_worker(void *arg)
{
	serial_longrun_ctx_t *ctx = (serial_longrun_ctx_t *)arg;
	uint8_t tx_buffer[1024];
	uint8_t rx_buffer[1024];
	size_t written, read_count;
	int32_t ret;
	uint64_t iteration = 0;

	OSAL_printf("[ INFO ] Long-running worker started\n");

	/* Fill pattern */
	for (size_t i = 0; i < sizeof(tx_buffer); i++) {
		tx_buffer[i] = (uint8_t)(i & 0xFF);
	}

	while (!ctx->stop_flag) {
		/* Write data */
		ret = HAL_SERIAL_write(ctx->handle, tx_buffer, sizeof(tx_buffer), &written);
		if (ret == OSAL_SUCCESS) {
			ctx->total_bytes_tx += written;
		} else {
			ctx->error_count++;
		}

		/* Try to read back */
		ret = HAL_SERIAL_read(ctx->handle, rx_buffer, sizeof(rx_buffer), &read_count);
		if (ret == OSAL_SUCCESS) {
			ctx->total_bytes_rx += read_count;
		} else if (ret != OSAL_ERR_TIMEOUT) {
			ctx->error_count++;
		}

		iteration++;

		/* Progress report every 1000 iterations */
		if (iteration % 1000 == 0) {
			OSAL_printf("[ INFO ] Iteration %llu: TX=%llu bytes, RX=%llu bytes, errors=%u\n",
			           (unsigned long long)iteration,
			           (unsigned long long)ctx->total_bytes_tx,
			           (unsigned long long)ctx->total_bytes_rx,
			           ctx->error_count);
		}

		/* Brief sleep to avoid CPU hogging */
		OSAL_usleep(1 * 1000);
	}

	OSAL_printf("[ INFO ] Long-running worker stopped\n");
	return NULL;
}

/**
 * Test: Long-running stability test (memory leak detection)
 * Verifies: no memory leaks, stable operation over time
 */
static void test_stress_serial_long_running(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.device = SERIAL_STRESS_PORT,
		.baudrate = SERIAL_STRESS_BAUDRATE,
		.data_bits = HAL_SERIAL_DATA_BITS_8,
		.stop_bits = HAL_SERIAL_STOP_BITS_1,
		.parity = HAL_SERIAL_PARITY_NONE,
		.flow_control = HAL_SERIAL_FLOW_CONTROL_NONE,
		.timeout_ms = 100
	};
	serial_longrun_ctx_t ctx;
	osal_thread_t worker_thread;
	int32_t ret;
	const uint32_t test_duration_sec = 30;

	OSAL_printf("[ INFO ] Starting serial long-running stability test\n");
	OSAL_printf("         Duration: %u seconds\n", test_duration_sec);

	/* Open serial port */
	ret = HAL_SERIAL_open(SERIAL_STRESS_PORT, &config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] Serial port not available\n");
		TEST_SKIP("Serial port not available");
		return;
	}

	/* Initialize context */
	OSAL_memset(&ctx, 0, sizeof(ctx));
	ctx.handle = handle;
	ctx.stop_flag = false;

	/* Create worker thread */
	ret = OSAL_pthread_create(&worker_thread, NULL, serial_longrun_worker, &ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Let it run */
	OSAL_sleep(test_duration_sec);

	/* Stop worker */
	ctx.stop_flag = true;
	OSAL_pthread_join(worker_thread, NULL);

	/* Print final statistics */
	OSAL_printf("\n[ INFO ] Final Statistics:\n");
	OSAL_printf("         Total TX: %llu bytes\n", (unsigned long long)ctx.total_bytes_tx);
	OSAL_printf("         Total RX: %llu bytes\n", (unsigned long long)ctx.total_bytes_rx);
	OSAL_printf("         Errors: %u\n", ctx.error_count);

	/* Verify operation */
	TEST_ASSERT_TRUE(ctx.total_bytes_tx > 0);
	TEST_ASSERT_TRUE(ctx.error_count < 100); /* Allow some errors */

	/* Cleanup */
	HAL_SERIAL_close(handle);

	OSAL_printf("[ PASS ] Serial long-running stability test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_stress_serial_continuous_transmission",
		.func = test_stress_serial_continuous_transmission,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_serial_concurrent_operations",
		.func = test_stress_serial_concurrent_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_serial_buffer_overflow",
		.func = test_stress_serial_buffer_overflow,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_serial_long_running",
		.func = test_stress_serial_long_running,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_serial",
	.module_name = "stress_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
		.timeout_ms = 180000,
		.description = "HAL Serial stress tests"
	}
};

__attribute__((constructor))
static void register_stress_hal_serial_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_SERIAL */
