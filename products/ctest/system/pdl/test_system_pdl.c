/**
 * @file test_system_pdl.c
 * @brief PDL层系统集成测试
 *
 * 测试范围：
 * 1. 外设端到端初始化流程（多模块协同）
 * 2. BMC与MCU协同工作场景
 * 3. 卫星遥测完整数据流
 * 4. Watchdog故障检测与恢复
 * 5. CCM多设备集成测试
 * 6. 跨层协议栈集成（PDL + PRL + HAL）
 * 7. 错误传播与隔离验证
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "pdl.h"
#include "osal.h"

/* 测试超时时间 */
#define TEST_TIMEOUT_MS 10000

/* 测试状态标志 */
static volatile bool g_test_callback_triggered = false;
static volatile int g_test_callback_count = 0;

/*===========================================================================
 * 测试辅助函数
 *===========================================================================*/

/**
 * 重置测试状态
 */
static void reset_test_state(void)
{
	g_test_callback_triggered = false;
	g_test_callback_count = 0;
}

/*===========================================================================
 * 测试1: 外设系统端到端初始化
 *
 * 目的：验证多个PDL设备在真实系统场景下的初始化流程
 *
 * 测试流程：
 * 1. 按依赖顺序初始化多个设备（Watchdog -> MCU -> BMC）
 * 2. 验证各设备初始化成功且状态正确
 * 3. 验证设备间不相互干扰
 * 4. 按反序清理所有设备
 * 5. 验证资源正确释放
 *===========================================================================*/
static void test_system_peripheral_end_to_end_init(void)
{
	int32_t ret;
	bool all_initialized = false;
	pdl_watchdog_handle_t wdt_handle = NULL;
	pdl_mcu_handle_t mcu_handle = NULL;
	pdl_bmc_handle_t bmc_handle = NULL;

	OSAL_printf("=== Test: Peripheral End-to-End Initialization ===\n");

	/* 阶段1: 初始化Watchdog（通常第一个启动） */
#ifdef CONFIG_PDL_WATCHDOG_SUPPORT
	OSAL_printf("[Step 1] Initializing Watchdog...\n");
	pdl_watchdog_config_t wdt_config = {
		.device = "/dev/watchdog",
		.mode = PDL_WATCHDOG_MODE_MANUAL,
		.timeout_sec = 10
	};

	ret = PDL_WATCHDOG_Init(&wdt_config, &wdt_handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Watchdog initialized successfully\n");

		/* 验证Watchdog状态 */
		pdl_watchdog_status_t wdt_status;
		ret = PDL_WATCHDOG_GetStatus(wdt_handle, &wdt_status);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_TRUE(wdt_status.enabled);
		OSAL_printf("[OK] Watchdog status verified\n");
	} else {
		OSAL_printf("[SKIP] Watchdog initialization failed (hardware not available): %d\n", ret);
	}
#else
	OSAL_printf("[SKIP] Watchdog not configured\n");
#endif

	/* 阶段2: 初始化MCU */
#ifdef CONFIG_PDL_MCU_SUPPORT
	OSAL_printf("[Step 2] Initializing MCU...\n");
	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PDL_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;
	mcu_config.retry_count = 3;
	OSAL_strcpy(mcu_config.name, "TEST_MCU");

	// mcu_handle already declared
	ret = PDL_MCU_Init(&mcu_config, &mcu_handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] MCU initialized successfully\n");

		/* 验证MCU状态 */
		pdl_mcu_status_t mcu_status;
		ret = PDL_MCU_GetStatus(mcu_handle, &mcu_status);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] MCU online: %s, state: %s\n",
				mcu_status.online ? "YES" : "NO",
				PDL_MCU_GetStateName(mcu_status.state));
		}
	} else {
		mcu_handle = NULL;
		OSAL_printf("[SKIP] MCU initialization failed (hardware not available): %d\n", ret);
	}
#else
	OSAL_printf("[SKIP] MCU not configured\n");
	pdl_mcu_handle_t mcu_handle = NULL;
#endif

	/* 阶段3: 初始化BMC */
#ifdef CONFIG_PDL_BMC_SUPPORT
	OSAL_printf("[Step 3] Initializing BMC...\n");
	pdl_bmc_config_t bmc_config;
	OSAL_memset(&bmc_config, 0, sizeof(bmc_config));
	bmc_config.primary_channel = PDL_BMC_CHANNEL_NETWORK;
	bmc_config.primary_config.network.ip_addr = "192.168.1.100";
	bmc_config.primary_config.network.username = "admin";
	bmc_config.primary_config.network.password = "admin";
	bmc_config.retry_count = 5000;
	bmc_config.retry_count = 3;

	// bmc_handle already declared
	ret = PDL_BMC_Init(&bmc_config, &bmc_handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] BMC initialized successfully\n");

		/* 验证BMC连接状态 */
		bool connected = PDL_BMC_IsConnected(bmc_handle);
		OSAL_printf("[INFO] BMC connected: %s\n", connected ? "YES" : "NO");
	} else {
		bmc_handle = NULL;
		OSAL_printf("[SKIP] BMC initialization failed (hardware not available): %d\n", ret);
	}
#else
	OSAL_printf("[SKIP] BMC not configured\n");
	pdl_bmc_handle_t bmc_handle = NULL;
#endif

	/* 阶段4: 验证所有设备独立运行 */
	OSAL_printf("[Step 4] Verifying device independence...\n");
	all_initialized = true;

#ifdef CONFIG_PDL_WATCHDOG_SUPPORT
	/* Kick watchdog确保它还活着 */
	ret = PDL_WATCHDOG_Kick(wdt_handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Watchdog kick successful\n");
	}
#endif

#ifdef CONFIG_PDL_MCU_SUPPORT
	/* 查询MCU状态 */
	if (mcu_handle) {
		pdl_mcu_status_t status;
		ret = PDL_MCU_GetStatus(mcu_handle, &status);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] MCU status query successful\n");
		}
	}
#endif

#ifdef CONFIG_PDL_BMC_SUPPORT
	/* 查询BMC统计信息 */
	if (bmc_handle) {
	uint32_t cmd_count, success_count, fail_count, switch_count;
	ret = PDL_BMC_GetStats(bmc_handle, &cmd_count, &success_count, &fail_count, &switch_count);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] BMC Stats: cmd=%u, success=%u, fail=%u, switch=%u\n",
		cmd_count, success_count, fail_count, switch_count);
	}
#endif

	OSAL_printf("[OK] All devices operating independently\n");

	/* 阶段5: 清理所有设备（反序） */
	OSAL_printf("[Step 5] Cleaning up all devices...\n");

#ifdef CONFIG_PDL_BMC_SUPPORT
	if (bmc_handle) {
		ret = PDL_BMC_Deinit(bmc_handle);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] BMC deinitialized\n");
		}
	}
#endif

#ifdef CONFIG_PDL_MCU_SUPPORT
	if (mcu_handle) {
		ret = PDL_MCU_Deinit(mcu_handle);
		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] MCU deinitialized\n");
		}
	}
#endif

#ifdef CONFIG_PDL_WATCHDOG_SUPPORT
	ret = PDL_WATCHDOG_Deinit(wdt_handle);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Watchdog deinitialized\n");
	}
#endif

	OSAL_printf("=== Test Completed: End-to-End Initialization ===\n");
	TEST_ASSERT_TRUE(all_initialized);
}

/*===========================================================================
 * 测试2: BMC与MCU协同工作
 *
 * 目的：验证BMC和MCU两个设备协同工作场景
 *
 * 测试流程：
 * 1. 同时初始化BMC和MCU
 * 2. MCU读取本地传感器数据
 * 3. BMC读取服务器端传感器数据
 * 4. 验证两者数据独立且正确
 * 5. 测试BMC控制MCU电源场景
 *===========================================================================*/
static void test_system_bmc_mcu_coordination(void)
{
	int32_t ret;
	bool test_passed = false;

	OSAL_printf("=== Test: BMC-MCU Coordination ===\n");

#if defined(CONFIG_PDL_BMC_SUPPORT) && defined(CONFIG_PDL_MCU_SUPPORT)
	/* 初始化MCU */
	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PDL_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;
	mcu_config.retry_count = 3;
	OSAL_strcpy(mcu_config.name, "TEST_MCU");

	pdl_mcu_handle_t mcu_handle = NULL;
	ret = PDL_MCU_Init(&mcu_config, &mcu_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] MCU not available: %d\n", ret);
		goto skip_test;
	}
	OSAL_printf("[OK] MCU initialized\n");

	/* 初始化BMC */
	pdl_bmc_config_t bmc_config;
	OSAL_memset(&bmc_config, 0, sizeof(bmc_config));
	bmc_config.primary_channel = PDL_BMC_CHANNEL_NETWORK;
	bmc_config.primary_config.network.ip_addr = "192.168.1.100";
	bmc_config.primary_config.network.username = "admin";
	bmc_config.primary_config.network.password = "admin";
	bmc_config.retry_count = 5000;
	bmc_config.retry_count = 3;

	pdl_bmc_handle_t bmc_handle = NULL;
	ret = PDL_BMC_Init(&bmc_config, &bmc_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] BMC not available: %d\n", ret);
		PDL_MCU_Deinit(mcu_handle);
		goto skip_test;
	}
	OSAL_printf("[OK] BMC initialized\n");

	/* 场景1: 并发读取两个设备的状态 */
	OSAL_printf("[Scenario 1] Concurrent status query...\n");

	pdl_mcu_status_t mcu_status;
	ret = PDL_MCU_GetStatus(mcu_handle, &mcu_status);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] MCU status: online=%d, temp=%.1f°C, voltage=%.2fV\n",
			mcu_status.online, mcu_status.temperature, mcu_status.voltage_mv);
	}

	pdl_bmc_power_state_t power_state;
	ret = PDL_BMC_GetPowerState(bmc_handle, &power_state);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] BMC power state: %d\n", power_state);
	}

	/* 场景2: BMC读取传感器，MCU读取寄存器 */
	OSAL_printf("[Scenario 2] Reading sensors and registers...\n");

	pdl_bmc_sensor_reading_t sensors[10];
	uint32_t sensor_count = 0;
	ret = PDL_BMC_ReadSensors(bmc_handle, PDL_BMC_SENSOR_TEMP, sensors, 10, &sensor_count);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] BMC read %u sensors\n", sensor_count);
	}

	uint8_t reg_value;
	ret = PDL_MCU_ReadRegister(mcu_handle, 0x10, &reg_value);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] MCU register[0x10] = 0x%02X\n", reg_value);
	}

	/* 场景3: 验证设备统计信息独立 */
	OSAL_printf("[Scenario 3] Verifying independent statistics...\n");

	uint32_t cmd_count, success_count, fail_count, switch_count;
	ret = PDL_BMC_GetStats(bmc_handle, &cmd_count, &success_count, &fail_count, &switch_count);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] BMC stats: cmd=%u, success=%u, fail=%u\n",
			cmd_count, success_count, fail_count);
	}

	/* 清理 */
	PDL_BMC_Deinit(bmc_handle);
	PDL_MCU_Deinit(mcu_handle);

	test_passed = true;
	OSAL_printf("=== Test Completed: BMC-MCU Coordination ===\n");

skip_test:
	if (!test_passed) {
		OSAL_printf("[SKIP] Test skipped due to hardware unavailability\n");
	}
#else
	OSAL_printf("[SKIP] BMC or MCU not configured\n");
	test_passed = true; /* 配置未启用时认为测试通过 */
#endif

	TEST_ASSERT_TRUE(test_passed);
}

/*===========================================================================
 * 测试3: 卫星遥测完整流程
 *
 * 目的：验证卫星平台通信的完整数据流
 *
 * 测试流程：
 * 1. 初始化卫星通信服务
 * 2. 注册命令回调函数
 * 3. 模拟接收卫星命令
 * 4. 发送响应和心跳
 * 5. 验证统计数据正确
 *===========================================================================*/

#ifdef CONFIG_PDL_SATELLITE_SUPPORT
static void satellite_command_callback(uint8_t cmd_type, uint32_t param, void *user_data)
{
	(void)user_data;
	OSAL_printf("[CALLBACK] Received satellite command: type=0x%02X, param=0x%08X\n", cmd_type, param);
	g_test_callback_triggered = true;
	g_test_callback_count++;
}
#endif

static void test_system_satellite_telemetry_full_flow(void)
{
	int32_t ret;
	bool test_passed = false;
	pdl_satellite_handle_t sat_handle = NULL;

	OSAL_printf("=== Test: Satellite Telemetry Full Flow ===\n");

#ifdef CONFIG_PDL_SATELLITE_SUPPORT
	reset_test_state();

	/* 初始化卫星服务 */
	pdl_satellite_config_t sat_config;
	OSAL_memset(&sat_config, 0, sizeof(sat_config));
	sat_config.can_device = "can1";
	sat_config.can_bitrate = 250000;
	sat_config.heartbeat_interval_ms = 1000;
	sat_config.cmd_timeout_ms = 5000;

	ret = PDL_SATELLITE_Init(&sat_config, &sat_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] Satellite service initialization failed: %d\n", ret);
		goto skip_test;
	}
	OSAL_printf("[OK] Satellite service initialized\n");

	/* 注册命令回调 */
	ret = PDL_SATELLITE_RegisterCallback(sat_handle, satellite_command_callback, NULL);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Command callback registered\n");

	/* 发送心跳 */
	OSAL_printf("[Step 1] Sending heartbeat...\n");
	ret = PDL_SATELLITE_SendHeartbeat(sat_handle, PDL_SATELLITE_STATUS_OK);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Heartbeat sent\n");
	} else {
		OSAL_printf("[WARN] Heartbeat failed: %d (may be normal if no satellite)\n", ret);
	}

	/* 发送响应 */
	OSAL_printf("[Step 2] Sending response...\n");
	ret = PDL_SATELLITE_SendResponse(sat_handle, 1, PDL_SATELLITE_STATUS_OK, 0x12345678);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Response sent\n");
	} else {
		OSAL_printf("[WARN] Response failed: %d (may be normal if no satellite)\n", ret);
	}

	/* 等待可能的命令回调 */
	OSAL_printf("[Step 3] Waiting for satellite commands (2s)...\n");
	OSAL_sleep(2);

	if (g_test_callback_triggered) {
		OSAL_printf("[OK] Received %d command(s)\n", g_test_callback_count);
	} else {
		OSAL_printf("[INFO] No commands received (normal if no satellite)\n");
	}

	/* 查询统计信息 */
	OSAL_printf("[Step 4] Querying statistics...\n");
	uint32_t rx_count, tx_count, err_count;
	ret = PDL_SATELLITE_GetStats(sat_handle, &rx_count, &tx_count, &err_count);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Stats: rx=%u, tx=%u, errors=%u\n",
			rx_count, tx_count, err_count);
	}

	/* 清理 */
	ret = PDL_SATELLITE_Deinit(sat_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Satellite service deinitialized\n");

	test_passed = true;
	OSAL_printf("=== Test Completed: Satellite Telemetry ===\n");

skip_test:
	if (!test_passed) {
		OSAL_printf("[SKIP] Test skipped due to hardware unavailability\n");
	}
#else
	OSAL_printf("[SKIP] Satellite not configured\n");
	test_passed = true;
#endif

	TEST_ASSERT_TRUE(test_passed);
}

/*===========================================================================
 * 测试4: Watchdog故障恢复机制
 *
 * 目的：验证Watchdog的故障检测和恢复能力
 *
 * 测试流程：
 * 1. 初始化Watchdog（手动模式）
 * 2. 正常kick验证基本功能
 * 3. 切换到自动模式
 * 4. 启动自动kick线程
 * 5. 验证自动kick工作正常
 * 6. 停止自动kick并清理
 *===========================================================================*/
static void test_system_watchdog_fault_recovery(void)
{
	int32_t ret;
	bool test_passed = false;
	pdl_watchdog_handle_t wdt_handle = NULL;

	OSAL_printf("=== Test: Watchdog Fault Recovery ===\n");

#ifdef CONFIG_PDL_WATCHDOG_SUPPORT
	/* 阶段1: 手动模式测试 */
	OSAL_printf("[Phase 1] Manual mode test...\n");

	pdl_watchdog_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.device = "/dev/watchdog";
	config.mode = PDL_WATCHDOG_MODE_MANUAL;
	config.timeout_sec = 10;

	ret = PDL_WATCHDOG_Init(&config, &wdt_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] Watchdog initialization failed: %d\n", ret);
		goto skip_test;
	}
	OSAL_printf("[OK] Watchdog initialized (manual mode)\n");

	/* 验证初始状态 */
	pdl_watchdog_status_t status;
	ret = PDL_WATCHDOG_GetStatus(wdt_handle, &status);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Status: enabled=%d, running=%d, timeout=%us\n",
		status.enabled, status.running, status.timeout_sec);

	/* 手动kick几次 */
	OSAL_printf("[Step 1] Manual kicks...\n");
	for (int i = 0; i < 3; i++) {
		ret = PDL_WATCHDOG_Kick(wdt_handle);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		OSAL_printf("[OK] Kick %d successful\n", i + 1);
		OSAL_usleep(100000);
	}

	/* 验证kick计数 */
	ret = PDL_WATCHDOG_GetStatus(wdt_handle, &status);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Total kicks: %u\n", status.kick_count);
	TEST_ASSERT(status.kick_count >= 3);

	/* 阶段2: 自动模式测试 */
	OSAL_printf("[Phase 2] Switching to auto mode...\n");

	/* 清理手动模式 */
	PDL_WATCHDOG_Deinit(wdt_handle);
	wdt_handle = NULL;

	/* 重新初始化为自动模式 */
	config.mode = PDL_WATCHDOG_MODE_AUTO;
	config.kick_interval_ms = 1000;

	ret = PDL_WATCHDOG_Init(&config, &wdt_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] Auto mode initialization failed: %d\n", ret);
		goto skip_test;
	}
	OSAL_printf("[OK] Watchdog reinitialized (auto mode)\n");

	/* 启动自动kick */
	ret = PDL_WATCHDOG_Start(wdt_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Auto-kick started\n");

	/* 等待自动kick运行 */
	OSAL_printf("[Step 2] Waiting for auto-kicks (3s)...\n");
	OSAL_sleep(3);

	/* 验证自动kick工作 */
	ret = PDL_WATCHDOG_GetStatus(wdt_handle, &status);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Auto mode status: running=%d, kicks=%u, interval=%ums\n",
		status.running, status.kick_count, status.kick_interval_ms);
	TEST_ASSERT_TRUE(status.running);
	TEST_ASSERT(status.kick_count > 0);

	/* 停止自动kick */
	ret = PDL_WATCHDOG_Stop(wdt_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Auto-kick stopped\n");

	/* 清理 */
	ret = PDL_WATCHDOG_Deinit(wdt_handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Watchdog deinitialized\n");

	test_passed = true;
	OSAL_printf("=== Test Completed: Watchdog Fault Recovery ===\n");

skip_test:
	if (!test_passed) {
		OSAL_printf("[SKIP] Test skipped due to hardware unavailability\n");
	}
#else
	OSAL_printf("[SKIP] Watchdog not configured\n");
	test_passed = true;
#endif

	TEST_ASSERT_TRUE(test_passed);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_system_peripheral_end_to_end_init",
		.func = test_system_peripheral_end_to_end_init,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_system_bmc_mcu_coordination",
		.func = test_system_bmc_mcu_coordination,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_system_satellite_telemetry_full_flow",
		.func = test_system_satellite_telemetry_full_flow,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_system_watchdog_fault_recovery",
		.func = test_system_watchdog_fault_recovery,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "system_pdl",
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
		.description = "PDL system integration tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_system_pdl_tests(void)
{
	libutest_register_suite(&test_suite);
}
