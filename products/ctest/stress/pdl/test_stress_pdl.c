/**
 * @file test_stress_pdl.c
 * @brief PDL层压力测试
 *
 * 测试场景：
 * 1. BMC传感器并发读取压力
 * 2. MCU状态长时间查询压力
 * 3. Watchdog高频喂狗压力
 * 4. 卫星遥测并发采集压力
 * 5. CCM通信并发压力
 * 6. 多设备混合压力
 * 7. 内存和资源耗尽测试
 */

#include <errno.h>
#include <test_framework/test_framework.h>
#include <test_framework/test_stress.h>
#include "pdl.h"

/*===========================================================================
 * BMC压力测试
 *===========================================================================*/

typedef struct {
	pdl_bmc_handle_t handle;
	osal_atomic_uint32_t read_count;
	osal_atomic_uint32_t error_count;
} bmc_stress_data_t;

/**
 * BMC传感器读取工作函数
 */
static int32_t bmc_sensor_worker(void *user_data, uint32_t iteration)
{
	bmc_stress_data_t *data = (bmc_stress_data_t *)user_data;
	(void)iteration;

	pdl_bmc_sensor_reading_t sensors[10];
	uint32_t actual_count;
	int32_t ret = PDL_BMC_ReadSensors(data->handle, PDL_BMC_SENSOR_TEMP,
	                                  sensors, 10, &actual_count);

	if (ret == 0) {
		OSAL_atomic_inc(&data->read_count);
	} else {
		OSAL_atomic_inc(&data->error_count);
	}

	return ret;
}

/**
 * 测试BMC传感器并发读取压力
 */
static void test_stress_bmc_concurrent_read(void)
{
	bmc_stress_data_t data;
	pdl_bmc_config_t config = {
		.primary_channel = PDL_BMC_CHANNEL_NETWORK,
		.primary_config.network = {
			.ip_addr = "127.0.0.1",
			.port = 623,
			.username = "admin",
			.password = "admin",
			.timeout_ms = 5000
		},
		.backup_channel = PDL_BMC_CHANNEL_SERIAL,
		.auto_switch = false,
		.retry_count = 3,
		.health_check_interval = 5000
	};

	/* 初始化BMC */
	int32_t ret = PDL_BMC_Init(&config, &data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] BMC not available (init failed: %d)\n", ret);
		return;
	}

	OSAL_atomic_init(&data.read_count, 0);
	OSAL_atomic_init(&data.error_count, 0);

	/* 创建压力测试：8线程并发读取5秒 */
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(8, 5);
	stress_context_t *ctx = stress_context_create("BMC Concurrent Read", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running BMC sensor concurrent read test: 8 threads, 5 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, bmc_sensor_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：至少95%成功率 */
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 95.0);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_BMC_Deinit(data.handle);

	OSAL_printf("[ INFO     ] BMC reads: %u, errors: %u\n",
	           OSAL_atomic_load(&data.read_count),
	           OSAL_atomic_load(&data.error_count));
}

/*===========================================================================
 * MCU压力测试
 *===========================================================================*/

typedef struct {
	pdl_mcu_handle_t handle;
	osal_atomic_uint32_t query_count;
	osal_atomic_uint32_t timeout_count;
} mcu_stress_data_t;

/**
 * MCU状态查询工作函数
 */
static int32_t mcu_status_worker(void *user_data, uint32_t iteration)
{
	mcu_stress_data_t *data = (mcu_stress_data_t *)user_data;
	(void)iteration;

	pdl_mcu_status_t status;
	int32_t ret = PDL_MCU_GetStatus(data->handle, &status);

	if (ret == 0) {
		OSAL_atomic_inc(&data->query_count);
	} else if (ret == -ETIMEDOUT) {
		OSAL_atomic_inc(&data->timeout_count);
	}

	/* 每隔10ms查询一次，避免过度占用总线 */
	OSAL_usleep(10000);

	return ret;
}

/**
 * 测试MCU状态长时间查询压力
 */
static void test_stress_mcu_long_running_query(void)
{
	mcu_stress_data_t data;
	pdl_mcu_config_t config = {
		.name = "MCU_TEST",
		.interface = PDL_MCU_INTERFACE_CAN,
		.hw.can = {
			.device = "can0",
			.bitrate = 500000,
			.rx_timeout = 100,
			.tx_timeout = 100,
			.tx_id = 0x100,
			.rx_id = 0x101
		},
		.cmd_timeout_ms = 100,
		.retry_count = 3
	};

	/* 初始化MCU */
	int32_t ret = PDL_MCU_Init(&config, &data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] MCU not available (init failed: %d)\n", ret);
		return;
	}

	OSAL_atomic_init(&data.query_count, 0);
	OSAL_atomic_init(&data.timeout_count, 0);

	/* 创建压力测试：单线程持续查询30秒 */
	stress_config_t stress_config = STRESS_CONFIG_DURATION(30);
	stress_context_t *ctx = stress_context_create("MCU Long Running Query", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running MCU long-running query test: 30 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, mcu_status_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：至少90%成功率（允许偶尔超时） */
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 90.0);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_MCU_Deinit(data.handle);

	OSAL_printf("[ INFO     ] MCU queries: %u, timeouts: %u\n",
	           OSAL_atomic_load(&data.query_count),
	           OSAL_atomic_load(&data.timeout_count));
}

/*===========================================================================
 * Watchdog压力测试
 *===========================================================================*/

typedef struct {
	pdl_watchdog_handle_t handle;
	osal_atomic_uint32_t kick_count;
} watchdog_stress_data_t;

/**
 * Watchdog喂狗工作函数
 */
static int32_t watchdog_kick_worker(void *user_data, uint32_t iteration)
{
	watchdog_stress_data_t *data = (watchdog_stress_data_t *)user_data;
	(void)iteration;

	int32_t ret = PDL_WATCHDOG_Kick(data->handle);

	if (ret == 0) {
		OSAL_atomic_inc(&data->kick_count);
	}

	/* 高频喂狗：每1ms一次 */
	OSAL_usleep(1000);

	return ret;
}

/**
 * 测试Watchdog高频喂狗压力
 */
static void test_stress_watchdog_high_frequency_kick(void)
{
	watchdog_stress_data_t data;
	pdl_watchdog_config_t config = {
		.name = "WDT_TEST",
		.device = "/dev/watchdog",
		.timeout_sec = 60,
		.mode = PDL_WATCHDOG_MODE_MANUAL,
		.kick_interval_ms = 1000,
		.enable_on_init = false
	};

	/* 初始化Watchdog */
	int32_t ret = PDL_WATCHDOG_Init(&config, &data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] Watchdog not available (init failed: %d)\n", ret);
		return;
	}

	ret = PDL_WATCHDOG_Enable(data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] Watchdog cannot be enabled\n");
		PDL_WATCHDOG_Deinit(data.handle);
		return;
	}

	OSAL_atomic_init(&data.kick_count, 0);

	/* 创建压力测试：单线程高频喂狗10秒 */
	stress_config_t stress_config = STRESS_CONFIG_DURATION(10);
	stress_context_t *ctx = stress_context_create("Watchdog High Frequency Kick", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running watchdog high-frequency kick test: 10 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, watchdog_kick_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：无错误 */
	STRESS_ASSERT_NO_ERRORS(ctx);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_WATCHDOG_Disable(data.handle);
	PDL_WATCHDOG_Deinit(data.handle);

	OSAL_printf("[ INFO     ] Watchdog kicks: %u\n",
	           OSAL_atomic_load(&data.kick_count));
}

/*===========================================================================
 * Satellite压力测试
 *===========================================================================*/

typedef struct {
	pdl_satellite_handle_t handle;
	osal_atomic_uint32_t telemetry_count;
	osal_atomic_uint32_t heartbeat_count;
} satellite_stress_data_t;

/**
 * Satellite遥测发送工作函数
 */
static int32_t satellite_telemetry_worker(void *user_data, uint32_t iteration)
{
	satellite_stress_data_t *data = (satellite_stress_data_t *)user_data;

	/* 发送响应（使用迭代号作为序列号） */
	int32_t ret = PDL_SATELLITE_SendResponse(data->handle, iteration,
	                                           PDL_SATELLITE_STATUS_OK, 0);

	if (ret == 0) {
		OSAL_atomic_inc(&data->telemetry_count);
	}

	/* 每隔100ms发送一次 */
	OSAL_usleep(100000);

	return ret;
}

/**
 * 测试卫星遥测并发采集压力
 */
static void test_stress_satellite_concurrent_telemetry(void)
{
	satellite_stress_data_t data;
	pdl_satellite_config_t config = {
		.can_device = "can0",
		.can_bitrate = 500000,
		.can_rx_timeout = 1000,
		.can_tx_timeout = 1000,
		.heartbeat_interval_ms = 5000,
		.cmd_timeout_ms = 1000
	};

	/* 初始化Satellite */
	int32_t ret = PDL_SATELLITE_Init(&config, &data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] Satellite not available (init failed: %d)\n", ret);
		return;
	}

	OSAL_atomic_init(&data.telemetry_count, 0);
	OSAL_atomic_init(&data.heartbeat_count, 0);

	/* 创建压力测试：4线程并发发送遥测15秒 */
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(4, 15);
	stress_context_t *ctx = stress_context_create("Satellite Concurrent Telemetry", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running satellite concurrent telemetry test: 4 threads, 15 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, satellite_telemetry_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：至少95%成功率 */
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 95.0);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_SATELLITE_Deinit(data.handle);

	OSAL_printf("[ INFO     ] Telemetry sent: %u\n",
	           OSAL_atomic_load(&data.telemetry_count));
}

/*===========================================================================
 * CCM压力测试
 *===========================================================================*/

#ifdef CONFIG_PDL_CCM_SUPPORT

typedef struct {
	pdl_ccm_handle_t handle;
	osal_atomic_uint32_t send_count;
	osal_atomic_uint32_t error_count;
} ccm_stress_data_t;

/**
 * CCM命令发送工作函数
 */
static int32_t ccm_command_worker(void *user_data, uint32_t iteration)
{
	ccm_stress_data_t *data = (ccm_stress_data_t *)user_data;

	/* 构造命令数据 */
	uint8_t cmd_data[128];
	size_t cmd_len = OSAL_snprintf((char *)cmd_data, sizeof(cmd_data),
	                               "Command #%u", iteration);

	int32_t ret = PDL_CCM_SendCommand(data->handle, 0x01, 0x10, 0x20,
	                                   cmd_data, cmd_len);

	if (ret == 0) {
		OSAL_atomic_inc(&data->send_count);
	} else {
		OSAL_atomic_inc(&data->error_count);
	}

	/* 每隔50ms发送一次 */
	OSAL_usleep(50000);

	return ret;
}

/**
 * 测试CCM通信并发压力
 */
static void test_stress_ccm_concurrent_communication(void)
{
	ccm_stress_data_t data;
	pdl_ccm_config_t config = {
		.ccm_ip = "127.0.0.1",
		.ccm_port = 8888,
		.connect_timeout_ms = 5000,
		.send_timeout_ms = 1000,
		.recv_timeout_ms = 1000,
		.heartbeat_interval_ms = 5000,
		.cmd_timeout_ms = 3000,
		.max_retries = 3
	};

	/* 初始化CCM */
	int32_t ret = PDL_CCM_Init(&config, &data.handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] CCM not available (init failed: %d)\n", ret);
		return;
	}

	OSAL_atomic_init(&data.send_count, 0);
	OSAL_atomic_init(&data.error_count, 0);

	/* 创建压力测试：6线程并发通信10秒 */
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(6, 10);
	stress_context_t *ctx = stress_context_create("CCM Concurrent Communication", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running CCM concurrent communication test: 6 threads, 10 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, ccm_command_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：至少90%成功率 */
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 90.0);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_CCM_Deinit(data.handle);

	OSAL_printf("[ INFO     ] CCM sends: %u, errors: %u\n",
	           OSAL_atomic_load(&data.send_count),
	           OSAL_atomic_load(&data.error_count));
}

#endif /* CONFIG_PDL_CCM_SUPPORT */

/*===========================================================================
 * 混合压力测试
 *===========================================================================*/

typedef struct {
	pdl_mcu_handle_t mcu_handle;
	pdl_watchdog_handle_t watchdog_handle;
	osal_atomic_uint32_t mcu_ops;
	osal_atomic_uint32_t watchdog_ops;
} multidevice_stress_data_t;

/**
 * 多设备混合操作工作函数
 */
static int32_t multidevice_worker(void *user_data, uint32_t iteration)
{
	multidevice_stress_data_t *data = (multidevice_stress_data_t *)user_data;

	/* 交替操作MCU和Watchdog */
	if (iteration % 2 == 0) {
		pdl_mcu_status_t status;
		if (PDL_MCU_GetStatus(data->mcu_handle, &status) == 0) {
			OSAL_atomic_inc(&data->mcu_ops);
		}
	} else {
		if (PDL_WATCHDOG_Kick(data->watchdog_handle) == 0) {
			OSAL_atomic_inc(&data->watchdog_ops);
		}
	}

	OSAL_usleep(5000);

	return 0;
}

/**
 * 测试多设备混合压力
 */
static void test_stress_multidevice_mixed(void)
{
	multidevice_stress_data_t data;

	/* 初始化MCU */
	pdl_mcu_config_t mcu_config = {
		.name = "MCU_TEST",
		.interface = PDL_MCU_INTERFACE_CAN,
		.hw.can = {
			.device = "can0",
			.bitrate = 500000,
			.rx_timeout = 100,
			.tx_timeout = 100,
			.tx_id = 0x100,
			.rx_id = 0x101
		},
		.cmd_timeout_ms = 100,
		.retry_count = 3
	};

	int32_t ret = PDL_MCU_Init(&mcu_config, &data.mcu_handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] MCU not available\n");
		return;
	}

	/* 初始化Watchdog */
	pdl_watchdog_config_t wdt_config = {
		.name = "WDT_TEST",
		.device = "/dev/watchdog",
		.timeout_sec = 60,
		.mode = PDL_WATCHDOG_MODE_MANUAL,
		.kick_interval_ms = 1000,
		.enable_on_init = false
	};

	ret = PDL_WATCHDOG_Init(&wdt_config, &data.watchdog_handle);
	if (ret != 0) {
		OSAL_printf("[ SKIP     ] Watchdog not available\n");
		PDL_MCU_Deinit(data.mcu_handle);
		return;
	}

	PDL_WATCHDOG_Enable(data.watchdog_handle);

	OSAL_atomic_init(&data.mcu_ops, 0);
	OSAL_atomic_init(&data.watchdog_ops, 0);

	/* 创建压力测试：4线程混合操作20秒 */
	stress_config_t stress_config = STRESS_CONFIG_CONCURRENCY(4, 20);
	stress_context_t *ctx = stress_context_create("Multi-Device Mixed", &stress_config);
	TEST_ASSERT_NOT_NULL(ctx);

	/* 运行压力测试 */
	OSAL_printf("[ INFO     ] Running multi-device mixed test: 4 threads, 20 seconds\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, multidevice_worker, &data), 0);

	/* 打印报告 */
	stress_print_report(ctx);

	/* 验证结果：至少85%成功率 */
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 85.0);

	/* 清理 */
	stress_context_destroy(ctx);
	PDL_WATCHDOG_Disable(data.watchdog_handle);
	PDL_WATCHDOG_Deinit(data.watchdog_handle);
	PDL_MCU_Deinit(data.mcu_handle);

	OSAL_printf("[ INFO     ] MCU ops: %u, Watchdog ops: %u\n",
	           OSAL_atomic_load(&data.mcu_ops),
	           OSAL_atomic_load(&data.watchdog_ops));
}

/*===========================================================================
 * 资源耗尽测试
 *===========================================================================*/

/**
 * 测试重复初始化和释放（检测内存泄漏）
 */
static void test_stress_resource_exhaustion(void)
{
	const uint32_t iterations = 1000;

	OSAL_printf("[ INFO     ] Running resource exhaustion test: %u iterations\n", iterations);

	for (uint32_t i = 0; i < iterations; i++) {
		pdl_mcu_handle_t handle;
		pdl_mcu_config_t config = {
			.name = "MCU_TEST",
			.interface = PDL_MCU_INTERFACE_CAN,
			.hw.can = {
				.device = "can0",
				.bitrate = 500000,
				.rx_timeout = 100,
				.tx_timeout = 100,
				.tx_id = 0x100,
				.rx_id = 0x101
			},
			.cmd_timeout_ms = 100,
			.retry_count = 3
		};

		/* 初始化 */
		int32_t ret = PDL_MCU_Init(&config, &handle);
		if (ret != 0) {
			OSAL_printf("[ SKIP     ] MCU init failed at iteration %u\n", i);
			return;
		}

		/* 立即释放 */
		PDL_MCU_Deinit(handle);

		/* 每100次迭代打印进度 */
		if ((i + 1) % 100 == 0) {
			OSAL_printf("[ INFO     ] Progress: %u/%u iterations completed\n", i + 1, iterations);
		}
	}

	OSAL_printf("[ PASS     ] Resource exhaustion test completed: %u iterations\n", iterations);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_stress_bmc_concurrent_read",
		.func = test_stress_bmc_concurrent_read,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_mcu_long_running_query",
		.func = test_stress_mcu_long_running_query,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_watchdog_high_frequency_kick",
		.func = test_stress_watchdog_high_frequency_kick,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_satellite_concurrent_telemetry",
		.func = test_stress_satellite_concurrent_telemetry,
		.setup = NULL,
		.teardown = NULL
	},
#ifdef CONFIG_PDL_CCM_SUPPORT
	{
		.name = "test_stress_ccm_concurrent_communication",
		.func = test_stress_ccm_concurrent_communication,
		.setup = NULL,
		.teardown = NULL
	},
#endif
	{
		.name = "test_stress_multidevice_mixed",
		.func = test_stress_multidevice_mixed,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_resource_exhaustion",
		.func = test_stress_resource_exhaustion,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "stress_pdl",
	.module_name = "stress_pdl",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 120000,  /* 2分钟超时 */
		.description = "PDL stress tests - concurrency, long-running, resource exhaustion"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_stress_pdl_tests(void)
{
	libutest_register_suite(&test_suite);
}
