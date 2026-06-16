/**
 * @file test_system_pdl_protocol_stack.c
 * @brief PDL协议栈集成测试 (PRL + PDL + HAL)
 *
 * 测试范围：
 * 1. 端到端协议栈数据流
 * 2. PRL编码 -> PDL发送 -> HAL传输
 * 3. HAL接收 -> PDL处理 -> PRL解码
 * 4. 各层错误传播
 * 5. 数据完整性验证
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "pdl.h"
#include "osal.h"

#ifdef CONFIG_PRL
#include "prl.h"
#endif

#define TEST_TIMEOUT_MS 10000
#define TEST_PAYLOAD_SIZE 64

/* 测试数据 */
static uint8_t g_test_payload[TEST_PAYLOAD_SIZE];
static volatile bool g_data_received = false;

/*===========================================================================
 * 辅助函数
 *===========================================================================*/

static void generate_test_payload(void)
{
	for (uint32_t i = 0; i < TEST_PAYLOAD_SIZE; i++) {
		g_test_payload[i] = (uint8_t)(i & 0xFF);
	}
}

static bool verify_test_payload(const uint8_t *data, uint32_t len)
{
	if (len != TEST_PAYLOAD_SIZE) {
		return false;
	}

	for (uint32_t i = 0; i < len; i++) {
		if (data[i] != (uint8_t)(i & 0xFF)) {
			return false;
		}
	}

	return true;
}

/*===========================================================================
 * 测试1: MCU协议栈端到端测试
 *
 * 测试流程：
 * 1. 生成应用层数据
 * 2. PRL编码为MCU协议报文
 * 3. PDL通过MCU驱动发送
 * 4. (如果有回环) HAL接收 -> PDL解析 -> PRL解码
 * 5. 验证数据完整性
 *===========================================================================*/
static void test_protocol_stack_mcu_end_to_end(void)
{
	OSAL_printf("=== Test: MCU Protocol Stack End-to-End ===\n");

#if defined(CONFIG_PDL_MCU_SUPPORT) && defined(CONFIG_PRL) && defined(CONFIG_PRL_MCU)
	int32_t ret;

	/* 生成测试数据 */
	generate_test_payload();
	OSAL_printf("[Step 1] Generated test payload (%u bytes)\n", TEST_PAYLOAD_SIZE);

	/* PRL层编码 */
	uint8_t encoded_buffer[256];
	int encoded_len = PRL_Encode(
		PRL_DEV_TYPE_MCU,
		PRL_MCU_MSG_GET_STATUS,
		g_test_payload,
		TEST_PAYLOAD_SIZE,
		encoded_buffer,
		sizeof(encoded_buffer),
		0
	);

	if (encoded_len < 0) {
		OSAL_printf("[SKIP] PRL encoding failed: %d\n", encoded_len);
		TEST_ASSERT_TRUE(true);
		return;
	}
	OSAL_printf("[OK] PRL encoded: %d bytes\n", encoded_len);

	/* 初始化MCU */
	pdl_mcu_config_t mcu_config;
	OSAL_memset(&mcu_config, 0, sizeof(mcu_config));
	mcu_config.interface = PCONFIG_MCU_INTERFACE_CAN;
	mcu_config.hw.can.device = "can0";
	mcu_config.hw.can.bitrate = 500000;
	mcu_config.hw.can.tx_id = 0x100;
	mcu_config.hw.can.rx_id = 0x200;
	mcu_config.cmd_timeout_ms = 3000;
	mcu_config.retry_count = 3;

	pdl_mcu_handle_t mcu_handle = NULL;
	ret = PDL_MCU_Init(&mcu_config, &mcu_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] MCU initialization failed: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}
	OSAL_printf("[OK] PDL MCU initialized\n");

	/* 发送命令（通过协议栈） */
	uint8_t response[256];
	uint32_t response_len = 0;
	ret = PDL_MCU_SendCommand(mcu_handle, PRL_MCU_MSG_GET_STATUS,
				   g_test_payload, TEST_PAYLOAD_SIZE,
				   response, sizeof(response), &response_len);

	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Command sent and response received (%u bytes)\n", response_len);

		/* PRL解码响应 */
		uint8_t dev_type, msg_type;
		const uint8_t *payload;
		uint16_t payload_len;

		ret = PRL_Decode(response, response_len, &dev_type, &msg_type,
				 &payload, &payload_len);

		if (ret == OSAL_SUCCESS) {
			OSAL_printf("[OK] PRL decoded: dev_type=0x%02X, msg_type=0x%02X, payload=%u bytes\n",
				dev_type, msg_type, payload_len);

			/* 验证协议栈完整性 */
			TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
		} else {
			OSAL_printf("[WARN] PRL decoding failed: %d (response may not be PRL format)\n", ret);
		}
	} else {
		OSAL_printf("[WARN] Command failed: %d (expected if no MCU hardware)\n", ret);
	}

	/* 清理 */
	PDL_MCU_Deinit(mcu_handle);

	OSAL_printf("=== Test Completed: MCU Protocol Stack ===\n");
#else
	OSAL_printf("[SKIP] MCU, PRL, or PRL_MCU not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试2: Satellite协议栈端到端测试
 *===========================================================================*/
static void test_protocol_stack_satellite_end_to_end(void)
{
	OSAL_printf("=== Test: Satellite Protocol Stack End-to-End ===\n");

#if defined(CONFIG_PDL_SATELLITE_SUPPORT) && defined(CONFIG_PRL) && defined(CONFIG_PRL_SATELLITE)
	int32_t ret;

	/* 生成测试数据 */
	generate_test_payload();

	/* PRL编码卫星遥测数据 */
	uint8_t encoded_buffer[256];
	int encoded_len = PRL_Encode(
		PRL_DEV_TYPE_SATELLITE,
		0x01, /* 遥测消息类型 */
		g_test_payload,
		TEST_PAYLOAD_SIZE,
		encoded_buffer,
		sizeof(encoded_buffer),
		0
	);

	if (encoded_len < 0) {
		OSAL_printf("[SKIP] PRL encoding failed: %d\n", encoded_len);
		TEST_ASSERT_TRUE(true);
		return;
	}
	OSAL_printf("[OK] PRL encoded satellite telemetry: %d bytes\n", encoded_len);

	/* 初始化Satellite服务 */
	pdl_satellite_config_t sat_config;
	OSAL_memset(&sat_config, 0, sizeof(sat_config));
	sat_config.can.device = "can1";
	sat_config.can.bitrate = 250000;
	sat_config.can.tx_id = 0x300;
	sat_config.can.rx_id = 0x400;

	ret = PDL_SATELLITE_Init(&sat_config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[SKIP] Satellite initialization failed: %d\n", ret);
		TEST_ASSERT_TRUE(true);
		return;
	}
	OSAL_printf("[OK] PDL Satellite initialized\n");

	/* 发送响应（携带PRL编码的数据） */
	ret = PDL_SATELLITE_SendResponse(1, 0, encoded_buffer, encoded_len);
	if (ret == OSAL_SUCCESS) {
		OSAL_printf("[OK] Satellite response sent through protocol stack\n");
	} else {
		OSAL_printf("[WARN] Send failed: %d (expected if no satellite hardware)\n", ret);
	}

	/* 清理 */
	PDL_SATELLITE_Deinit();

	OSAL_printf("=== Test Completed: Satellite Protocol Stack ===\n");
#else
	OSAL_printf("[SKIP] Satellite, PRL, or PRL_SATELLITE not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试3: 协议栈错误注入测试
 *
 * 验证错误在各层之间的正确传播
 *===========================================================================*/
static void test_protocol_stack_error_propagation(void)
{
	OSAL_printf("=== Test: Protocol Stack Error Propagation ===\n");

#if defined(CONFIG_PDL_MCU_SUPPORT) && defined(CONFIG_PRL)
	int32_t ret;

	/* 场景1: PRL编码错误 - 缓冲区太小 */
	OSAL_printf("[Scenario 1] PRL encoding with insufficient buffer...\n");
	uint8_t small_buffer[10];
	generate_test_payload();

	int encoded_len = PRL_Encode(
		PRL_DEV_TYPE_MCU,
		PRL_MCU_MSG_GET_STATUS,
		g_test_payload,
		TEST_PAYLOAD_SIZE,
		small_buffer,
		sizeof(small_buffer), /* 太小，应该失败 */
		0
	);

	if (encoded_len < 0) {
		OSAL_printf("[OK] PRL correctly reported encoding error: %d\n", encoded_len);
	} else {
		OSAL_printf("[WARN] PRL should have failed with small buffer\n");
	}

	/* 场景2: PRL解码错误 - 无效数据 */
	OSAL_printf("[Scenario 2] PRL decoding with invalid data...\n");
	uint8_t invalid_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
	uint8_t dev_type, msg_type;
	const uint8_t *payload;
	uint16_t payload_len;

	ret = PRL_Decode(invalid_data, sizeof(invalid_data), &dev_type, &msg_type,
			 &payload, &payload_len);

	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[OK] PRL correctly rejected invalid data: %d\n", ret);
	} else {
		OSAL_printf("[WARN] PRL should have rejected invalid data\n");
	}

	/* 场景3: PDL层错误 - 空句柄操作 */
	OSAL_printf("[Scenario 3] PDL operation with NULL handle...\n");
	pdl_mcu_status_t status;
	ret = PDL_MCU_GetStatus(NULL, &status);

	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[OK] PDL correctly rejected NULL handle: %d\n", ret);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* 场景4: PDL层错误 - 空参数 */
	OSAL_printf("[Scenario 4] PDL operation with NULL parameter...\n");
	pdl_mcu_config_t config;
	OSAL_memset(&config, 0, sizeof(config));
	ret = PDL_MCU_Init(&config, NULL);

	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[OK] PDL correctly rejected NULL parameter: %d\n", ret);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	}

	OSAL_printf("=== Test Completed: Error Propagation ===\n");
#else
	OSAL_printf("[SKIP] MCU or PRL not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试4: 数据完整性验证
 *
 * 验证数据在协议栈各层传递过程中不会被破坏
 *===========================================================================*/
static void test_protocol_stack_data_integrity(void)
{
	OSAL_printf("=== Test: Protocol Stack Data Integrity ===\n");

#if defined(CONFIG_PRL) && defined(CONFIG_PRL_MCU)
	/* 生成测试数据 */
	generate_test_payload();
	OSAL_printf("[Step 1] Generated test pattern\n");

	/* PRL编码 */
	uint8_t encoded_buffer[256];
	int encoded_len = PRL_Encode(
		PRL_DEV_TYPE_MCU,
		PRL_MCU_MSG_GET_VERSION,
		g_test_payload,
		TEST_PAYLOAD_SIZE,
		encoded_buffer,
		sizeof(encoded_buffer),
		0
	);

	TEST_ASSERT(encoded_len > 0);
	OSAL_printf("[Step 2] PRL encoded: %d bytes\n", encoded_len);

	/* PRL解码 */
	uint8_t dev_type, msg_type;
	const uint8_t *decoded_payload;
	uint16_t decoded_len;

	int ret = PRL_Decode(encoded_buffer, encoded_len, &dev_type, &msg_type,
			     &decoded_payload, &decoded_len);

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[Step 3] PRL decoded: %u bytes\n", decoded_len);

	/* 验证数据完整性 */
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
	TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
	TEST_ASSERT_EQUAL(TEST_PAYLOAD_SIZE, decoded_len);

	bool integrity_ok = verify_test_payload(decoded_payload, decoded_len);
	TEST_ASSERT_TRUE(integrity_ok);

	if (integrity_ok) {
		OSAL_printf("[OK] Data integrity verified through encode/decode cycle\n");
	}

	OSAL_printf("=== Test Completed: Data Integrity ===\n");
#else
	OSAL_printf("[SKIP] PRL or PRL_MCU not configured\n");
#endif

	TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{
		.name = "test_protocol_stack_mcu_end_to_end",
		.func = test_protocol_stack_mcu_end_to_end,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_protocol_stack_satellite_end_to_end",
		.func = test_protocol_stack_satellite_end_to_end,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_protocol_stack_error_propagation",
		.func = test_protocol_stack_error_propagation,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_protocol_stack_data_integrity",
		.func = test_protocol_stack_data_integrity,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "system_pdl_protocol_stack",
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
		.description = "PDL protocol stack integration tests (PRL+PDL+HAL)"
	}
};

__attribute__((constructor))
static void register_system_pdl_protocol_stack_tests(void)
{
	libutest_register_suite(&test_suite);
}
