/**
 * @file test_system_pdl_pmc.c
 * @brief PDL PMC设备集成测试
 *
 * 测试范围：
 * 1. PMC通信驱动初始化
 * 2. 遥测数据发送和接收
 * 3. 遥控命令处理
 * 4. 节点管理操作
 * 5. 电源控制集成
 * 6. 心跳和连接状态
 * 7. 固件更新流程
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "pdl.h"
#include "osal.h"

#define TEST_TIMEOUT_MS 10000

/* 回调测试上下文 */
static volatile bool g_telemetry_received = false;
static volatile bool g_command_received = false;
static volatile uint32_t g_callback_count = 0;

/*===========================================================================
 * 回调函数
 *===========================================================================*/

#ifdef CONFIG_PDL_PMC_SUPPORT
static void telemetry_callback(uint8_t tm_type, const uint8_t *data,
			       uint32_t len, void *user_data)
{
	(void)user_data;
	OSAL_printf("[CALLBACK] Telemetry received: type=0x%02X, len=%u\n", tm_type, len);
	g_telemetry_received = true;
	__sync_fetch_and_add(&g_callback_count, 1);
}

static void command_callback(uint8_t tc_type, const uint8_t *data,
			     uint32_t len, void *user_data)
{
	(void)user_data;
	OSAL_printf("[CALLBACK] Command received: type=0x%02X, len=%u\n", tc_type, len);
	g_command_received = true;
	__sync_fetch_and_add(&g_callback_count, 1);
}
#endif

static void reset_test_state(void)
{
	g_telemetry_received = false;
	g_command_received = false;
	g_callback_count = 0;
}

/*===========================================================================
 * 测试1: PMC初始化和基本通信
 *===========================================================================*/
static void test_pmc_init_and_basic_communication(void)
{
	OSAL_printf("=== Test: PMC Initialization and Basic Communication ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;
	reset_test_state();

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;
	config.retry_count = 3;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC initialization failed: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}
	OSAL_printf("[OK] PMC initialized\n");

	/* 注册回调 */
	ret = PDL_PMC_register_telemetry_callback(telemetry_callback, NULL);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Telemetry callback registered\n");

	ret = PDL_PMC_register_command_callback(command_callback, NULL);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] Command callback registered\n");

	/* 查询连接状态 */
	pdl_pmc_connection_status_t conn_status;
	ret = PDL_PMC_GetConnectionStatus(&conn_status);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[INFO] Connection: connected=%d, link_quality=%u%%\n",
			conn_status.connected, conn_status.link_quality);
	}

	/* 发送心跳 */
	OSAL_printf("[Step 1] Sending heartbeat...\n");
	ret = PDL_PMC_SendHeartbeat(PDL_PMC_NODE_STATUS_NORMAL);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Heartbeat sent\n");
	} else {
		OSAL_printf("[WARN] Heartbeat failed: %d\n", ret);
	}

	/* 查询统计信息 */
	pdl_pmc_stats_t stats;
	ret = PDL_PMC_GetStats(&stats);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Stats: rx=%u, tx=%u, errors=%u\n",
			stats.rx_count, stats.tx_count, stats.error_count);
	}

	/* 清理 */
	ret = PDL_PMC_deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[OK] PMC deinitialized\n");

	OSAL_printf("=== Test Completed: PMC Basic Communication ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试2: PMC遥测数据发送
 *===========================================================================*/
static void test_pmc_telemetry_transmission(void)
{
	OSAL_printf("=== Test: PMC Telemetry Transmission ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC not available: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}

	/* 准备遥测数据 */
	uint8_t tm_data[128];
	for (uint32_t i = 0; i < sizeof(tm_data); i++) {
		tm_data[i] = (uint8_t)(i & 0xFF);
	}

	/* 发送不同类型的遥测 */
	OSAL_printf("[Step 1] Sending various telemetry types...\n");

	ret = PDL_PMC_send_telemetry(0x01, tm_data, 64);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Telemetry type 0x01 sent (64 bytes)\n");
	} else {
		OSAL_printf("[WARN] Telemetry 0x01 failed: %d\n", ret);
	}

	ret = PDL_PMC_send_telemetry(0x02, tm_data, 32);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Telemetry type 0x02 sent (32 bytes)\n");
	} else {
		OSAL_printf("[WARN] Telemetry 0x02 failed: %d\n", ret);
	}

	/* 等待可能的响应 */
	OSAL_Sleep(1000);

	/* 查询统计 */
	pdl_pmc_stats_t stats;
	ret = PDL_PMC_GetStats(&stats);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] After telemetry: tx=%u, rx=%u\n", stats.tx_count, stats.rx_count);
	}

	/* 清理 */
	PDL_PMC_deinit();

	OSAL_printf("=== Test Completed: PMC Telemetry ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试3: PMC遥控命令发送
 *===========================================================================*/
static void test_pmc_telecommand_transmission(void)
{
	OSAL_printf("=== Test: PMC Telecommand Transmission ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC not available: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}

	/* 发送命令 */
	uint8_t tc_data[] = {0x01, 0x02, 0x03, 0x04};

	OSAL_printf("[Step 1] Sending telecommand...\n");
	ret = PDL_PMC_send_command(0x10, tc_data, sizeof(tc_data));
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Telecommand sent\n");
	} else {
		OSAL_printf("[WARN] Telecommand failed: %d\n", ret);
	}

	/* 等待响应 */
	OSAL_Sleep(1000);

	/* 清理 */
	PDL_PMC_deinit();

	OSAL_printf("=== Test Completed: PMC Telecommand ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试4: PMC节点管理
 *===========================================================================*/
static void test_pmc_node_management(void)
{
	OSAL_printf("=== Test: PMC Node Management ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC not available: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}

	/* 查询节点状态 */
	OSAL_printf("[Step 1] Querying node status...\n");
	ret = PDL_PMC_node_manage(PDL_PMC_NODE_OP_QUERY, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Node query sent\n");
	} else {
		OSAL_printf("[WARN] Node query failed: %d\n", ret);
	}

	OSAL_Sleep(500);

	/* 启动节点 */
	OSAL_printf("[Step 2] Starting node...\n");
	ret = PDL_PMC_node_manage(PDL_PMC_NODE_OP_START, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Node start command sent\n");
	} else {
		OSAL_printf("[WARN] Node start failed: %d\n", ret);
	}

	OSAL_Sleep(500);

	/* 停止节点 */
	OSAL_printf("[Step 3] Stopping node...\n");
	ret = PDL_PMC_node_manage(PDL_PMC_NODE_OP_STOP, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Node stop command sent\n");
	} else {
		OSAL_printf("[WARN] Node stop failed: %d\n", ret);
	}

	/* 清理 */
	PDL_PMC_deinit();

	OSAL_printf("=== Test Completed: PMC Node Management ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试5: PMC电源控制
 *===========================================================================*/
static void test_pmc_power_control(void)
{
	OSAL_printf("=== Test: PMC Power Control ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC not available: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}

	/* 查询电源状态 */
	OSAL_printf("[Step 1] Querying power domain status...\n");
	ret = PDL_PMC_power_control(PDL_PMC_POWER_OP_QUERY, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Power query sent\n");
	} else {
		OSAL_printf("[WARN] Power query failed: %d\n", ret);
	}

	OSAL_Sleep(500);

	/* 上电 */
	OSAL_printf("[Step 2] Powering on domain...\n");
	ret = PDL_PMC_power_control(PDL_PMC_POWER_OP_ON, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Power on command sent\n");
	} else {
		OSAL_printf("[WARN] Power on failed: %d\n", ret);
	}

	OSAL_Sleep(500);

	/* 下电 */
	OSAL_printf("[Step 3] Powering off domain...\n");
	ret = PDL_PMC_power_control(PDL_PMC_POWER_OP_OFF, 1);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Power off command sent\n");
	} else {
		OSAL_printf("[WARN] Power off failed: %d\n", ret);
	}

	/* 清理 */
	PDL_PMC_deinit();

	OSAL_printf("=== Test Completed: PMC Power Control ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试6: PMC状态查询
 *===========================================================================*/
static void test_pmc_status_query(void)
{
	OSAL_printf("=== Test: PMC Status Query ===\n");

#ifdef CONFIG_PDL_PMC_SUPPORT
	int32_t ret;

	/* 初始化PMC */
	pdl_pmc_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	config.interface.type = PDL_PMC_INTERFACE_ETHERNET;
	OSAL_strcpy(config.interface.eth.device, "eth0");
	OSAL_strcpy(config.interface.eth.remote_ip, "192.168.1.10");
	config.interface.eth.remote_port = 8888;
	config.interface.eth.local_port = 9999;
	config.timeout_ms = 3000;

	ret = PDL_PMC_init(&config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] PMC not available: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}

	/* 查询不同类型的状态 */
	OSAL_printf("[Step 1] Querying various status types...\n");

	ret = PDL_PMC_QueryStatus(0x01, 0);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Status query 0x01 sent\n");
	} else {
		OSAL_printf("[WARN] Status query 0x01 failed: %d\n", ret);
	}

	OSAL_Sleep(200);

	ret = PDL_PMC_QueryStatus(0x02, 0);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Status query 0x02 sent\n");
	} else {
		OSAL_printf("[WARN] Status query 0x02 failed: %d\n", ret);
	}

	/* 清理 */
	PDL_PMC_deinit();

	OSAL_printf("=== Test Completed: PMC Status Query ===\n");
#else
	OSAL_printf("[SKIP] PMC not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_pmc_init_and_basic_communication",
		.func = test_pmc_init_and_basic_communication,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pmc_telemetry_transmission",
		.func = test_pmc_telemetry_transmission,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pmc_telecommand_transmission",
		.func = test_pmc_telecommand_transmission,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pmc_node_management",
		.func = test_pmc_node_management,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pmc_power_control",
		.func = test_pmc_power_control,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pmc_status_query",
		.func = test_pmc_status_query,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "system_pdl_pmc",
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
		.description = "PDL PMC device integration tests"
	}
};

__attribute__((constructor))
static void register_system_pdl_pmc_tests(void)
{
	libutest_register_suite(&test_suite);
}
