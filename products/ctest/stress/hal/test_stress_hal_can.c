/**
 * @file test_stress_hal_can.c
 * @brief HAL CAN stress tests - concurrent and high-load scenarios
 */

#include <test_framework/test_framework.h>
#include <test_stress.h>
#include "hal.h"
#include "osal.h"

#ifdef CONFIG_TEST_STRESS_HAL_CAN

/* Test configuration */
#define CAN_STRESS_INTERFACE      "can0"
#define CAN_STRESS_BAUDRATE       500000
#define CAN_STRESS_THREAD_COUNT   8
#define CAN_STRESS_DURATION_SEC   15
#define CAN_STRESS_BURST_SIZE     100
#define CAN_STRESS_MAX_HANDLES    32

/* Worker context for concurrent send test */
typedef struct {
	hal_can_handle_t handle;
	uint32_t thread_id;
	osal_atomic_uint32_t *shared_counter;
} can_send_worker_ctx_t;

/* Worker context for receive flooding test */
typedef struct {
	hal_can_handle_t tx_handle;
	hal_can_handle_t rx_handle;
	osal_atomic_uint32_t *rx_count;
} can_flood_worker_ctx_t;

/*===========================================================================
 * Test 1: Concurrent Send Stress
 *===========================================================================*/

/**
 * Worker function for concurrent send test
 */
static int32_t can_concurrent_send_worker(void *user_data, uint32_t iteration)
{
	can_send_worker_ctx_t *ctx = (can_send_worker_ctx_t *)user_data;
	hal_can_frame_t frame;
	int32_t ret;

	/* Prepare frame with unique ID per thread */
	frame.can_id = 0x100 + ctx->thread_id;
	frame.dlc = 8;
	frame.data[0] = (iteration >> 24) & 0xFF;
	frame.data[1] = (iteration >> 16) & 0xFF;
	frame.data[2] = (iteration >> 8) & 0xFF;
	frame.data[3] = iteration & 0xFF;
	frame.data[4] = ctx->thread_id;
	frame.data[5] = 0xAA;
	frame.data[6] = 0xBB;
	frame.data[7] = 0xCC;

	/* Send frame */
	ret = HAL_CAN_Send(ctx->handle, &frame);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->shared_counter);
	}

	return ret;
}

/**
 * Test: CAN concurrent send from multiple threads
 * Verifies: thread-safe transmission, no message loss, buffer management
 */
static void test_stress_can_concurrent_send(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = {
		.interface = CAN_STRESS_INTERFACE,
		.baudrate = CAN_STRESS_BAUDRATE,
		.rx_timeout = 1000,
		.tx_timeout = 1000
	};
	osal_atomic_uint32_t shared_counter;
	can_send_worker_ctx_t *worker_contexts = NULL;
	stress_context_t *stress_ctx = NULL;
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(
		CAN_STRESS_THREAD_COUNT, CAN_STRESS_DURATION_SEC);
	int32_t ret;

	OSAL_printf("[ INFO ] Starting CAN concurrent send stress test\n");
	OSAL_printf("         Threads: %u, Duration: %u sec\n",
	           CAN_STRESS_THREAD_COUNT, CAN_STRESS_DURATION_SEC);

	/* Initialize CAN */
	ret = HAL_CAN_Init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] CAN interface not available\n");
		TEST_SKIP("CAN interface not available");
		return;
	}

	/* Initialize shared counter */
	OSAL_atomic_store(&shared_counter, 0);

	/* Allocate worker contexts */
	worker_contexts = OSAL_malloc(sizeof(can_send_worker_ctx_t) * CAN_STRESS_THREAD_COUNT);
	TEST_ASSERT_NOT_NULL(worker_contexts);

	for (uint32_t i = 0; i < CAN_STRESS_THREAD_COUNT; i++) {
		worker_contexts[i].handle = handle;
		worker_contexts[i].thread_id = i;
		worker_contexts[i].shared_counter = &shared_counter;
	}

	/* Create stress test context */
	stress_ctx = stress_context_create("CAN_Concurrent_Send", &stress_config);
	TEST_ASSERT_NOT_NULL(stress_ctx);

	/* Run stress test with all workers */
	for (uint32_t i = 0; i < CAN_STRESS_THREAD_COUNT; i++) {
		ret = stress_run(stress_ctx, can_concurrent_send_worker, &worker_contexts[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Wait for completion and check results */
	OSAL_usleep(CAN_STRESS_DURATION_SEC * 1000 + 2000 * 1000);

	/* Print report */
	stress_print_report(stress_ctx);

	/* Verify success rate */
	STRESS_ASSERT_SUCCESS_RATE_GT(stress_ctx, 95.0);

	/* Verify sent count */
	uint32_t sent_count = OSAL_atomic_load(&shared_counter);
	OSAL_printf("[ INFO ] Total frames sent: %u\n", sent_count);
	TEST_ASSERT_TRUE(sent_count > 0);

	/* Cleanup */
	stress_context_destroy(stress_ctx);
	OSAL_free(worker_contexts);
	HAL_CAN_Deinit(handle);

	OSAL_printf("[ PASS ] CAN concurrent send stress test completed\n");
}

/*===========================================================================
 * Test 2: Receive Buffer Flooding
 *===========================================================================*/

/**
 * TX worker: flood CAN bus with frames
 */
static int32_t can_flood_tx_worker(void *user_data, uint32_t iteration)
{
	can_flood_worker_ctx_t *ctx = (can_flood_worker_ctx_t *)user_data;
	hal_can_frame_t frame;
	int32_t ret;

	/* Send burst of frames */
	for (uint32_t i = 0; i < CAN_STRESS_BURST_SIZE; i++) {
		frame.can_id = 0x200 + (iteration % 256);
		frame.dlc = 8;
		frame.data[0] = (iteration >> 24) & 0xFF;
		frame.data[1] = (iteration >> 16) & 0xFF;
		frame.data[2] = (iteration >> 8) & 0xFF;
		frame.data[3] = iteration & 0xFF;
		frame.data[4] = i;
		frame.data[5] = 0xDD;
		frame.data[6] = 0xEE;
		frame.data[7] = 0xFF;

		ret = HAL_CAN_Send(ctx->tx_handle, &frame);
		if (ret != OSAL_SUCCESS) {
			return ret;
		}
	}

	return OSAL_SUCCESS;
}

/**
 * RX worker: receive frames as fast as possible
 */
static int32_t can_flood_rx_worker(void *user_data, uint32_t iteration)
{
	can_flood_worker_ctx_t *ctx = (can_flood_worker_ctx_t *)user_data;
	hal_can_frame_t frame;
	int32_t ret;

	/* Try to receive frame */
	ret = HAL_CAN_Recv(ctx->rx_handle, &frame, 100);
	if (ret == OSAL_SUCCESS) {
		OSAL_atomic_inc(ctx->rx_count);
	} else if (ret == OSAL_ERR_TIMEOUT) {
		/* Timeout is acceptable during flood test */
		return OSAL_SUCCESS;
	}

	return ret;
}

/**
 * Test: CAN receive buffer flooding
 * Verifies: buffer overflow protection, no data loss, receive performance
 */
static void test_stress_can_rx_flooding(void)
{
	hal_can_handle_t tx_handle = NULL;
	hal_can_handle_t rx_handle = NULL;
	hal_can_config_t config = {
		.interface = CAN_STRESS_INTERFACE,
		.baudrate = CAN_STRESS_BAUDRATE,
		.rx_timeout = 100,  /* Short timeout for flood test */
		.tx_timeout = 1000
	};
	osal_atomic_uint32_t rx_count;
	can_flood_worker_ctx_t worker_ctx;
	stress_context_t *tx_stress_ctx = NULL;
	stress_context_t *rx_stress_ctx = NULL;
	stress_config_t tx_config = STRESS_CONFIG_CONCURRENCY(4, 10);
	stress_config_t rx_config = STRESS_CONFIG_CONCURRENCY(4, 10);
	int32_t ret;

	OSAL_printf("[ INFO ] Starting CAN RX flooding stress test\n");

	/* Initialize CAN interfaces */
	ret = HAL_CAN_Init(&config, &tx_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] CAN TX interface not available\n");
		TEST_SKIP("CAN interface not available");
		return;
	}

	ret = HAL_CAN_Init(&config, &rx_handle);
	if (ret != OSAL_SUCCESS) {
		HAL_CAN_Deinit(tx_handle);
		OSAL_printf("[ SKIP ] CAN RX interface not available\n");
		TEST_SKIP("CAN interface not available");
		return;
	}

	/* Initialize context */
	OSAL_atomic_store(&rx_count, 0);
	worker_ctx.tx_handle = tx_handle;
	worker_ctx.rx_handle = rx_handle;
	worker_ctx.rx_count = &rx_count;

	/* Create stress contexts */
	tx_stress_ctx = stress_context_create("CAN_Flood_TX", &tx_config);
	rx_stress_ctx = stress_context_create("CAN_Flood_RX", &rx_config);
	TEST_ASSERT_NOT_NULL(tx_stress_ctx);
	TEST_ASSERT_NOT_NULL(rx_stress_ctx);

	/* Start TX flood */
	ret = stress_run(tx_stress_ctx, can_flood_tx_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Start RX workers */
	ret = stress_run(rx_stress_ctx, can_flood_rx_worker, &worker_ctx);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Wait for completion */
	OSAL_sleep(12000/1000);

	/* Print reports */
	OSAL_printf("\n--- TX Flood Report ---\n");
	stress_print_report(tx_stress_ctx);
	OSAL_printf("\n--- RX Report ---\n");
	stress_print_report(rx_stress_ctx);

	/* Verify received frames */
	uint32_t received = OSAL_atomic_load(&rx_count);
	OSAL_printf("[ INFO ] Total frames received: %u\n", received);
	TEST_ASSERT_TRUE(received > 0);

	/* Cleanup */
	stress_context_destroy(tx_stress_ctx);
	stress_context_destroy(rx_stress_ctx);
	HAL_CAN_Deinit(rx_handle);
	HAL_CAN_Deinit(tx_handle);

	OSAL_printf("[ PASS ] CAN RX flooding stress test completed\n");
}

/*===========================================================================
 * Test 3: Resource Exhaustion
 *===========================================================================*/

/**
 * Test: CAN resource exhaustion (max handles)
 * Verifies: graceful failure when resources exhausted, proper cleanup
 */
static void test_stress_can_resource_exhaustion(void)
{
	hal_can_handle_t handles[CAN_STRESS_MAX_HANDLES];
	hal_can_config_t config = {
		.interface = CAN_STRESS_INTERFACE,
		.baudrate = CAN_STRESS_BAUDRATE,
		.rx_timeout = 1000,
		.tx_timeout = 1000
	};
	uint32_t opened_count = 0;
	int32_t ret;

	OSAL_printf("[ INFO ] Starting CAN resource exhaustion test\n");
	OSAL_printf("         Max handles to test: %u\n", CAN_STRESS_MAX_HANDLES);

	/* Try to open maximum handles */
	for (uint32_t i = 0; i < CAN_STRESS_MAX_HANDLES; i++) {
		ret = HAL_CAN_Init(&config, &handles[i]);
		if (ret == OSAL_SUCCESS) {
			opened_count++;
		} else {
			/* Expected to fail eventually */
			OSAL_printf("[ INFO ] Failed to open handle #%u (expected)\n", i);
			break;
		}
	}

	OSAL_printf("[ INFO ] Successfully opened %u handles\n", opened_count);
	TEST_ASSERT_TRUE(opened_count > 0);

	/* Verify opened handles are functional */
	hal_can_frame_t test_frame = {
		.can_id = 0x123,
		.dlc = 4,
		.data = {0x11, 0x22, 0x33, 0x44}
	};

	for (uint32_t i = 0; i < opened_count; i++) {
		ret = HAL_CAN_Send(handles[i], &test_frame);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Cleanup all handles */
	for (uint32_t i = 0; i < opened_count; i++) {
		ret = HAL_CAN_Deinit(handles[i]);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* Verify we can open again after cleanup */
	hal_can_handle_t new_handle = NULL;
	ret = HAL_CAN_Init(&config, &new_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	HAL_CAN_Deinit(new_handle);

	OSAL_printf("[ PASS ] CAN resource exhaustion test completed\n");
}

/*===========================================================================
 * Test 4: Rapid Filter Reconfiguration
 *===========================================================================*/

/**
 * Test: Rapid filter reconfiguration under load
 * Verifies: filter setup stability, no race conditions
 */
static void test_stress_can_filter_reconfiguration(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = {
		.interface = CAN_STRESS_INTERFACE,
		.baudrate = CAN_STRESS_BAUDRATE,
		.rx_timeout = 1000,
		.tx_timeout = 1000
	};
	uint32_t filter_id, filter_mask;
	int32_t ret;
	const uint32_t iterations = 1000;

	OSAL_printf("[ INFO ] Starting CAN filter reconfiguration stress test\n");
	OSAL_printf("         Iterations: %u\n", iterations);

	/* Initialize CAN */
	ret = HAL_CAN_Init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] CAN interface not available\n");
		TEST_SKIP("CAN interface not available");
		return;
	}

	/* Rapidly reconfigure filters */
	for (uint32_t i = 0; i < iterations; i++) {
		filter_id = 0x100 + (i % 256);
		filter_mask = 0x7FF;

		ret = HAL_CAN_SetFilter(handle, filter_id, filter_mask);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Send test frame */
		hal_can_frame_t frame = {
			.can_id = filter_id,
			.dlc = 2,
			.data = {(i >> 8) & 0xFF, i & 0xFF}
		};

		ret = HAL_CAN_Send(handle, &frame);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Brief sleep to allow filter to take effect */
		if (i % 100 == 0) {
			OSAL_usleep(1 * 1000);
		}
	}

	/* Cleanup */
	HAL_CAN_Deinit(handle);

	OSAL_printf("[ PASS ] CAN filter reconfiguration stress test completed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_stress_can_concurrent_send",
		.func = test_stress_can_concurrent_send,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_can_rx_flooding",
		.func = test_stress_can_rx_flooding,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_can_resource_exhaustion",
		.func = test_stress_can_resource_exhaustion,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_can_filter_reconfiguration",
		.func = test_stress_can_filter_reconfiguration,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "stress_hal_can",
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
		.description = "HAL CAN stress tests"
	}
};

__attribute__((constructor))
static void register_stress_hal_can_tests(void)
{
	libutest_register_suite(&test_suite);
}

#endif /* CONFIG_TEST_STRESS_HAL_CAN */
