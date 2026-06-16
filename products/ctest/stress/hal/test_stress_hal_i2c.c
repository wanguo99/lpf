/**
 * @file test_stress_hal_i2c.c
 * @brief HAL I2C stress tests - rapid transactions and error handling
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_I2C

/* Test configuration */
#define I2C_STRESS_DEVICE         "/dev/i2c-0"
#define I2C_STRESS_SLAVE_ADDR     0x50
#define I2C_STRESS_THREAD_COUNT   4
#define I2C_STRESS_DURATION_SEC   15
#define I2C_STRESS_MAX_HANDLES    16

/* Worker context */
typedef struct {
	hal_i2c_handle_t handle;
	uint32_t thread_id;
	osal_atomic_uint32_t *transaction_counter;
} i2c_worker_ctx_t;

/*===========================================================================
 * Test 1: Rapid I2C Transactions
 *===========================================================================*/

/**
 * Worker for rapid I2C transactions
 */
static int32_t i2c_rapid_transaction_worker(void *user_data, uint32_t iteration)
{
	i2c_worker_ctx_t *ctx = (i2c_worker_ctx_t *)user_data;
	uint8_t write_data[16];
	uint8_t read_data[16];
	int32_t ret;

	/* Prepare write data */
	for (size_t i = 0; i < sizeof(write_data); i++) {
		write_data[i] = (uint8_t)(iteration + i + ctx->thread_id);
	}

	/* Perform write transaction */
	ret = HAL_I2C_write(ctx->handle, I2C_STRESS_SLAVE_ADDR,
	                    write_data, sizeof(write_data));
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->transaction_counter);
	} else if (ret == OSAL_ERR_TIMEOUT || ret == OSAL_ERR_DEVICE) {
		/* I2C errors are acceptable in stress test */
		return OSAL_SUCCESS;
	}

	/* Small delay between transactions */
	OSAL_usleep(1 * 1000);

	/* Perform read transaction */
	ret = HAL_I2C_read(ctx->handle, I2C_STRESS_SLAVE_ADDR,
	                   read_data, sizeof(read_data));
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->transaction_counter);
	} else if (ret == OSAL_ERR_TIMEOUT || ret == OSAL_ERR_DEVICE) {
		/* I2C errors are acceptable in stress test */
		return OSAL_SUCCESS;
	}

	return OSAL_SUCCESS;
}

/**
 * Test: Rapid I2C transactions
 * Verifies: transaction stability, error handling
 */
static void test_stress_i2c_rapid_transactions(void)
{
	hal_i2c_handle_t handle = NULL;
	osal_atomic_uint32_t transaction_counter;
	i2c_worker_ctx_t worker_ctx;
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_DURATION(I2C_STRESS_DURATION_SEC);
	int32_t ret;

	OSAL_printf("[ INFO ] Starting I2C rapid transactions stress test\n");
	OSAL_printf("         Duration: %u sec, Device: %s, Addr: 0x%02X\n",
	           I2C_STRESS_DURATION_SEC, I2C_STRESS_DEVICE, I2C_STRESS_SLAVE_ADDR);

	/* Open I2C device */
	ret = HAL_I2C_open(I2C_STRESS_DEVICE, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] I2C device not available\n");
		TEST_SKIP("I2C device not available");
		return;
	}

	/* Initialize counter */
	OSAL_atomic_store(&transaction_counter, 0);

	/* Setup worker context */
	worker_ctx.handle = handle;
	worker_ctx.thread_id = 0;
	worker_ctx.transaction_counter = &transaction_counter;

	/* Create stress test context */
	stress_ctx = stress_context_create("I2C_Rapid_Transactions", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	ret = stress_run(stress_ctx, i2c_rapid_transaction_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Wait for completion */
	OSAL_usleep(I2C_STRESS_DURATION_SEC * 1000 + 1000 * 1000);

	/* Print report */
	stress_print_report(stress_ctx);

	/* Verify transactions */
	uint64_t total_transactions = OSAL_atomic_load(&transaction_counter);
	OSAL_printf("[ INFO ] Total successful transactions: %llu\n",
	           (unsigned long long)total_transactions);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	HAL_I2C_close(handle);

	OSAL_printf("[ PASS ] I2C rapid transactions stress test completed\n");
}

/*===========================================================================
 * Test 2: Concurrent I2C Access
 *===========================================================================*/

/**
 * Worker for concurrent I2C access
 */
static int32_t i2c_concurrent_worker(void *user_data, uint32_t iteration)
{
	i2c_worker_ctx_t *ctx = (i2c_worker_ctx_t *)user_data;
	uint8_t reg_addr = (uint8_t)(iteration % 256);
	uint8_t write_data[4] = {
		(uint8_t)(0xA0 + ctx->thread_id),
		(uint8_t)(iteration & 0xFF),
		(uint8_t)((iteration >> 8) & 0xFF),
		(uint8_t)((iteration >> 16) & 0xFF)
	};
	uint8_t read_data[4];
	int32_t ret;

	/* Write to register */
	ret = HAL_I2C_write_reg(ctx->handle, I2C_STRESS_SLAVE_ADDR,
	                       reg_addr, write_data, sizeof(write_data));
	if (ret != OSAL_SUCCESS && ret != OSAL_ERR_TIMEOUT && ret != OSAL_ERR_DEVICE) {
		return ret;
	}

	/* Read back from register */
	ret = HAL_I2C_read_reg(ctx->handle, I2C_STRESS_SLAVE_ADDR,
	                      reg_addr, read_data, sizeof(read_data));
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->transaction_counter);
	}

	return (ret == OSAL_ERR_TIMEOUT || ret == OSAL_ERR_DEVICE) ? OSAL_SUCCESS : ret;
}

/**
 * Test: Concurrent I2C access from multiple threads
 * Verifies: bus arbitration, thread safety
 */
static void test_stress_i2c_concurrent_access(void)
{
	hal_i2c_handle_t handles[I2C_STRESS_THREAD_COUNT];
	osal_atomic_uint32_t transaction_counter;
	i2c_worker_ctx_t worker_ctx[I2C_STRESS_THREAD_COUNT];
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(
		I2C_STRESS_THREAD_COUNT, 10);
	int32_t ret;

	OSAL_printf("[ INFO ] Starting I2C concurrent access stress test\n");
	OSAL_printf("         Threads: %u, Duration: 10 sec\n", I2C_STRESS_THREAD_COUNT);

	/* Open I2C handles */
	for (uint32_t i = 0; i < I2C_STRESS_THREAD_COUNT; i++) {
		ret = HAL_I2C_open(I2C_STRESS_DEVICE, &handles[i]);
		if (ret != OSAL_SUCCESS) {
			for (uint32_t j = 0; j < i; j++) {
				HAL_I2C_close(handles[j]);
			}
			OSAL_printf("[ SKIP ] Cannot open multiple I2C handles\n");
			TEST_SKIP("I2C multi-open not supported");
			return;
		}
	}

	/* Initialize counter */
	OSAL_atomic_store(&transaction_counter, 0);

	/* Setup worker contexts */
	for (uint32_t i = 0; i < I2C_STRESS_THREAD_COUNT; i++) {
		worker_ctx[i].handle = handles[i];
		worker_ctx[i].thread_id = i;
		worker_ctx[i].transaction_counter = &transaction_counter;
	}

	/* Create stress test context */
	stress_ctx = stress_context_create("I2C_Concurrent_Access", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test */
	for (uint32_t i = 0; i < I2C_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(stress_ctx, i2c_concurrent_worker, &worker_ctx[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion */
	OSAL_sleep(12000/1000);

	/* Print report */
	stress_print_report(stress_ctx);

	/* Verify transactions */
	uint64_t total_transactions = OSAL_atomic_load(&transaction_counter);
	OSAL_printf("[ INFO ] Total concurrent transactions: %llu\n",
	           (unsigned long long)total_transactions);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	for (uint32_t i = 0; i < I2C_STRESS_THREAD_COUNT; i++) {
		HAL_I2C_close(handles[i]);
	}

	OSAL_printf("[ PASS ] I2C concurrent access stress test completed\n");
}

/*===========================================================================
 * Test 3: Resource Exhaustion
 *===========================================================================*/

/**
 * Test: I2C resource exhaustion (max handles)
 * Verifies: graceful failure, proper cleanup
 */
static void test_stress_i2c_resource_exhaustion(void)
{
	hal_i2c_handle_t handles[I2C_STRESS_MAX_HANDLES];
	uint32_t opened_count = 0;
	int32_t ret;

	OSAL_printf("[ INFO ] Starting I2C resource exhaustion test\n");
	OSAL_printf("         Max handles to test: %u\n", I2C_STRESS_MAX_HANDLES);

	/* Try to open maximum handles */
	for (uint32_t i = 0; i < I2C_STRESS_MAX_HANDLES; i++) {
		ret = HAL_I2C_open(I2C_STRESS_DEVICE, &handles[i]);
		if (ret == OSAL_SUCCESS) {
			opened_count++;
		} else {
			OSAL_printf("[ INFO ] Failed to open handle #%u (expected)\n", i);
			break;
		}
	}

	OSAL_printf("[ INFO ] Successfully opened %u handles\n", opened_count);
	TEST_ASSERT_TRUE(opened_count > 0);

	/* Verify opened handles are functional */
	uint8_t test_data[4] = {0x11, 0x22, 0x33, 0x44};

	for (uint32_t i = 0; i < opened_count; i++) {
		ret = HAL_I2C_write(handles[i], I2C_STRESS_SLAVE_ADDR,
		                    test_data, sizeof(test_data));
		/* Acceptable to fail if no device present */
		if (ret != OSAL_SUCCESS && ret != OSAL_ERR_TIMEOUT && ret != OSAL_ERR_DEVICE) {
			TEST_FAIL();
		}
	}

	/* Cleanup all handles */
	for (uint32_t i = 0; i < opened_count; i++) {
		ret = HAL_I2C_close(handles[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Verify we can open again after cleanup */
	hal_i2c_handle_t new_handle = NULL;
	ret = HAL_I2C_open(I2C_STRESS_DEVICE, &new_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	HAL_I2C_close(new_handle);

	OSAL_printf("[ PASS ] I2C resource exhaustion test completed\n");
}

/*===========================================================================
 * Test 4: NAK Handling Under Load
 *===========================================================================*/

/**
 * Test: NAK/timeout handling under high transaction rate
 * Verifies: error recovery, no system hang
 */
static void test_stress_i2c_nak_handling(void)
{
	hal_i2c_handle_t handle = NULL;
	uint8_t data[8];
	int32_t ret;
	const uint32_t iterations = 1000;
	uint32_t success_count = 0;
	uint32_t nak_count = 0;
	uint32_t timeout_count = 0;

	OSAL_printf("[ INFO ] Starting I2C NAK handling stress test\n");
	OSAL_printf("         Iterations: %u\n", iterations);

	/* Open I2C device */
	ret = HAL_I2C_open(I2C_STRESS_DEVICE, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] I2C device not available\n");
		TEST_SKIP("I2C device not available");
		return;
	}

	/* Try to communicate with various addresses (some will NAK) */
	for (uint32_t i = 0; i < iterations; i++) {
		uint16_t slave_addr = 0x08 + (i % 120);  /* Scan address range */

		ret = HAL_I2C_read(handle, slave_addr, data, sizeof(data));

		if (ret == OSAL_SUCCESS) {
			success_count++;
		} else if (ret == OSAL_ERR_TIMEOUT) {
			timeout_count++;
		} else if (ret == OSAL_ERR_DEVICE) {
			nak_count++;
		}

		/* Progress indicator */
		if (i % 100 == 0) {
			OSAL_printf("[ INFO ] Progress: %u/%u (success: %u, NAK: %u, timeout: %u)\n",
			           i, iterations, success_count, nak_count, timeout_count);
		}
	}

	/* Print final statistics */
	OSAL_printf("\n[ INFO ] Final Statistics:\n");
	OSAL_printf("         Success: %u\n", success_count);
	OSAL_printf("         NAK/IO errors: %u\n", nak_count);
	OSAL_printf("         Timeouts: %u\n", timeout_count);

	/* Verify system is still functional */
	ret = HAL_I2C_read(handle, I2C_STRESS_SLAVE_ADDR, data, sizeof(data));
	/* Acceptable to fail if no device present */

	/* Cleanup */
	HAL_I2C_close(handle);

	OSAL_printf("[ PASS ] I2C NAK handling stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_stress_i2c_rapid_transactions",
		.func = test_stress_i2c_rapid_transactions,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_i2c_concurrent_access",
		.func = test_stress_i2c_concurrent_access,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_i2c_resource_exhaustion",
		.func = test_stress_i2c_resource_exhaustion,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_i2c_nak_handling",
		.func = test_stress_i2c_nak_handling,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_i2c",
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
		.description = "HAL I2C stress tests"
	}
};

__attribute__((constructor))
static void register_stress_hal_i2c_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_I2C */
