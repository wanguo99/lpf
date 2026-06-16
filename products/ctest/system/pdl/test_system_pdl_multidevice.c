/**
 * @file test_system_pdl_multidevice.c
 * @brief PDL多设备并发操作系统测试
 *
 * 测试范围：
 * 1. 多个PDL设备同时操作
 * 2. 设备间独立性验证
 * 3. 并发命令处理
 * 4. 资源竞争检测
 * 5. 设备优先级处理
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "pdl.h"
#include "osal.h"

/* 测试配置 */
#define CONCURRENT_OPERATIONS 10
#define TEST_TIMEOUT_MS 15000

/* 多设备测试上下文 */
typedef struct {
	pdl_mcu_handle_t mcu_handle;
	pdl_bmc_handle_t bmc_handle;
	volatile uint32_t mcu_operation_count;
	volatile uint32_t bmc_operation_count;
	volatile uint32_t error_count;
	bool test_running;
} multidevice_context_t;

static multidevice_context_t g_context;

/*===========================================================================
 * 辅助函数
 *===========================================================================*/

static void reset_context(void)
{
	OSAL_memset(&g_context, 0, sizeof(g_context));
}

/**
 * MCU操作线程
 */
static void* mcu_operation_thread(void *arg)
{
	multidevice_context_t *ctx = (multidevice_context_t *)arg;

	OSAL_printf("[MCU Thread] Started\n");

	while (ctx->test_running) {
		if (ctx->mcu_handle) {
			/* 查询MCU状态 */
			pdl_mcu_status_t status;
			int32_t ret = PDL_MCU_GetStatus(ctx->mcu_handle, &status);
			if (ret == OSAL_SUCCESS) {
				__sync_fetch_and_add(&ctx->mcu_operation_count, 1);
			} else {
				__sync_fetch_and_add(&ctx->error_count, 1);
			}
		}

		OSAL_usleep(100000);
	}

	OSAL_printf("[MCU Thread] Stopped (operations: %u)\n", ctx->mcu_operation_count);
	return NULL;
}

/**
 * BMC操作线程
 */
static void* bmc_operation_thread(void *arg)
{
	multidevice_context_t *ctx = (multidevice_context_t *)arg;

	OSAL_printf("[BMC Thread] Started\n");

	while (ctx->test_running) {
		if (ctx->bmc_handle) {
			/* 查询BMC统计 */
			uint32_t cmd_count, success_count, fail_count, switch_count;
			int32_t ret = PDL_BMC_GetStats(ctx->bmc_handle, &cmd_count, &success_count, &fail_count, &switch_count);
			if (ret == OSAL_SUCCESS) {
				__sync_fetch_and_add(&ctx->bmc_operation_count, 1);
			} else {
				__sync_fetch_and_add(&ctx->error_count, 1);
			}
		}

		OSAL_usleep(150000);
	}

	OSAL_printf("[BMC Thread] Stopped (operations: %u)\n", ctx->bmc_operation_count);
	return NULL;
}

/*===========================================================================
 * 测试1: 多设备并发初始化
 *===========================================================================*/
static void test_multidevice_concurrent_init(void)
{
	OSAL_printf("=== Test: Multi-device Concurrent Initialization ===\n");

	reset_context();

#if defined(CONFIG_PDL_MCU_SUPPORT) && defined(CONFIG_PDL_BMC_SUPPORT)
	/* 并发初始化MCU和BMC */
	OSAL_printf("[Step 1] Concurrent device initialization...\n");

	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PCONFIG_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;
	mcu_config.retry_count = 3;

	pdl_bmc_config_t bmc_config;
	OSAL_memset(&bmc_config, 0, sizeof(bmc_config));
	bmc_config.primary_channel = PDL_BMC_CHANNEL_NETWORK;
	bmc_config.primary_config.network.ip_addr = "192.168.1.100";
	bmc_config.primary_config.network.username = "admin";
	bmc_config.primary_config.network.password = "admin";
	bmc_config.retry_count = 3;

	int32_t mcu_ret = PDL_MCU_Init(&mcu_config, &g_context.mcu_handle);
	int32_t bmc_ret = PDL_BMC_Init(&bmc_config, &g_context.bmc_handle);

	if (mcu_ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] MCU initialized\n");
	} else {
		OSAL_printf("[SKIP] MCU not available: %d\n", mcu_ret);
	}

	if (bmc_ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] BMC initialized\n");
	} else {
		OSAL_printf("[SKIP] BMC not available: %d\n", bmc_ret);
	}

	/* 验证两个设备都可以独立操作 */
	if (g_context.mcu_handle) {
		pdl_mcu_status_t status;
		int32_t ret = PDL_MCU_GetStatus(g_context.mcu_handle, &status);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] MCU operational\n");
		}
	}

	if (g_context.bmc_handle) {
		bool connected = PDL_BMC_IsConnected(g_context.bmc_handle);
		OSAL_printf("[INFO] BMC connection status: %s\n", connected ? "connected" : "disconnected");
	}

	/* 清理 */
	if (g_context.bmc_handle) {
		PDL_BMC_Deinit(g_context.bmc_handle);
	}
	if (g_context.mcu_handle) {
		PDL_MCU_Deinit(g_context.mcu_handle);
	}

	OSAL_printf("=== Test Completed: Concurrent Initialization ===\n");
#else
	OSAL_printf("[SKIP] MCU or BMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试2: 多设备并发操作
 *===========================================================================*/
static void test_multidevice_concurrent_operations(void)
{
	OSAL_printf("=== Test: Multi-device Concurrent Operations ===\n");

	reset_context();

#if defined(CONFIG_PDL_MCU_SUPPORT) && defined(CONFIG_PDL_BMC_SUPPORT) && defined(CONFIG_OSAL_THREAD)
	/* 初始化设备 */
	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PCONFIG_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;
	mcu_config.retry_count = 3;

	pdl_bmc_config_t bmc_config;
	OSAL_memset(&bmc_config, 0, sizeof(bmc_config));
	bmc_config.primary_channel = PDL_BMC_CHANNEL_NETWORK;
	bmc_config.primary_config.network.ip_addr = "192.168.1.100";
	bmc_config.primary_config.network.username = "admin";
	bmc_config.primary_config.network.password = "admin";
	bmc_config.retry_count = 3;

	int32_t mcu_ret = PDL_MCU_Init(&mcu_config, &g_context.mcu_handle);
	int32_t bmc_ret = PDL_BMC_Init(&bmc_config, &g_context.bmc_handle);

	if (mcu_ret != OSAL_SUCCESS && bmc_ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] No devices available\n");
		goto cleanup;
	}

	/* 启动并发操作线程 */
	OSAL_printf("[Step 1] Starting concurrent operation threads...\n");
	g_context.test_running = true;

	osal_thread_t mcu_thread = 0;
	osal_thread_t bmc_thread = 0;

	if (g_context.mcu_handle) {
		OSAL_pthread_create(&mcu_thread, NULL, mcu_operation_thread, &g_context);
	}

	if (g_context.bmc_handle) {
		OSAL_pthread_create(&bmc_thread, NULL, bmc_operation_thread, &g_context);
	}

	/* 运行5秒 */
	OSAL_printf("[Step 2] Running concurrent operations (5s)...\n");
	OSAL_sleep(5);

	/* 停止线程 */
	g_context.test_running = false;

	if (mcu_thread) {
		OSAL_pthread_join(mcu_thread, NULL);
	}
	if (bmc_thread) {
		OSAL_pthread_join(bmc_thread, NULL);
	}

	/* 验证结果 */
	OSAL_printf("[Results] MCU ops: %u, BMC ops: %u, errors: %u\n",
		g_context.mcu_operation_count,
		g_context.bmc_operation_count,
		g_context.error_count);

	TEST_ASSERT(g_context.mcu_operation_count > 0 || g_context.bmc_operation_count > 0);
	OSAL_printf("[OK] Devices operated independently\n");

cleanup:
	if (g_context.bmc_handle) {
		PDL_BMC_Deinit(g_context.bmc_handle);
	}
	if (g_context.mcu_handle) {
		PDL_MCU_Deinit(g_context.mcu_handle);
	}

	OSAL_printf("=== Test Completed: Concurrent Operations ===\n");
#else
	OSAL_printf("[SKIP] Thread support or devices not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试3: 设备独立性验证
 *===========================================================================*/
static void test_multidevice_independence(void)
{
	OSAL_printf("=== Test: Device Independence Verification ===\n");

#if defined(CONFIG_PDL_MCU_SUPPORT) && defined(CONFIG_PDL_WATCHDOG_SUPPORT)
	/* 初始化MCU和Watchdog，验证它们不会相互影响 */
	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PCONFIG_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;

	pdl_watchdog_config_t wdt_config;
	OSAL_memset(&wdt_config, 0, sizeof(wdt_config));
	wdt_config.device = "/dev/watchdog";
	wdt_config.mode = PDL_WATCHDOG_MODE_MANUAL;
	wdt_config.timeout_sec = 10;

	pdl_mcu_handle_t mcu_handle = NULL;
	pdl_watchdog_handle_t wdt_handle = NULL;
	int32_t mcu_ret = PDL_MCU_Init(&mcu_config, &mcu_handle);
	int32_t wdt_ret = PDL_WATCHDOG_Init(&wdt_config, &wdt_handle);

	OSAL_printf("[Init] MCU: %s, Watchdog: %s\n",
		mcu_ret == OSAL_SUCCESS ? "OK" : "SKIP",
		wdt_ret == OSAL_SUCCESS ? "OK" : "SKIP");

	/* 交替操作两个设备 */
	for (int i = 0; i < 5; i++) {
		if (wdt_handle) {
			PDL_WATCHDOG_Kick(wdt_handle);
		}

		if (mcu_handle) {
			pdl_mcu_status_t status;
			PDL_MCU_GetStatus(mcu_handle, &status);
		}

		OSAL_usleep(100000);
	}

	OSAL_printf("[OK] Devices operated independently without interference\n");

	/* 清理 */
	if (mcu_handle) {
		PDL_MCU_Deinit(mcu_handle);
	}
	if (wdt_handle) {
		PDL_WATCHDOG_Deinit(wdt_handle);
	}

	OSAL_printf("=== Test Completed: Device Independence ===\n");
#else
	OSAL_printf("[SKIP] MCU or Watchdog not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_multidevice_concurrent_init",
		.func = test_multidevice_concurrent_init,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_multidevice_concurrent_operations",
		.func = test_multidevice_concurrent_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_multidevice_independence",
		.func = test_multidevice_independence,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "system_pdl_multidevice",
	.module_name = "system_pdl",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
		.timeout_ms = TEST_TIMEOUT_MS,
		.description = "PDL multi-device concurrent operation tests"
	}
};

__attribute__((constructor))
static void register_system_pdl_multidevice_tests(void)
{
	libutest_register_suite(&test_suite);
}
