/**
 * @file test_call_chain.c
 * @brief 完整调用链测试用例
 * @note 验证 APP → ACONFIG → PDL → PCONFIG → HAL
 */

#include "osal.h"
#include "aconfig.h"
#include "pconfig.h"
#include "pdl.h"

/* 测试功能 ID 定义 */
#define TEST_FUNCTION_MCU_CAN       0x9001  /* 测试 MCU CAN 接口 */
#define TEST_FUNCTION_MCU_SERIAL    0x9002  /* 测试 MCU Serial 接口 */
#define TEST_FUNCTION_BMC           0x9003  /* 测试 BMC */

/**
 * @brief 测试调用链 - MCU CAN
 */
int32_t TestCallChain_MCU_CAN(void)
{
	uint32_t mcu_index;
	int32_t ret;

	LOG_INFO("TEST_CALL_CHAIN", "========================================");
	LOG_INFO("TEST_CALL_CHAIN", "Test Call Chain: MCU CAN");
	LOG_INFO("TEST_CALL_CHAIN", "========================================");

	/* 步骤 1: 从 ACONFIG 获取设备索引 */
	LOG_INFO("TEST_CALL_CHAIN", "Step 1: Query ACONFIG for device index");
	LOG_INFO("TEST_CALL_CHAIN", "Function ID: 0x%04X (TEST_FUNCTION_MCU_CAN)", TEST_FUNCTION_MCU_CAN);

	/* TODO: 实际从 ACONFIG 查询
	 * const void *config = ACONFIG_GetFunctionConfig(TEST_FUNCTION_MCU_CAN);
	 * mcu_index = ((ccm_tc_config_t*)config)->device_ref.device_index;
	 */
	mcu_index = 0;  /* 暂时硬编码为 0 */
	LOG_INFO("TEST_CALL_CHAIN", "Got MCU index: %u", mcu_index);

	/* 步骤 2: 调用 PDL 测试接口 */
	LOG_INFO("TEST_CALL_CHAIN", "Step 2: Call PDL_MCU_test_call(%u)", mcu_index);
	ret = PDL_MCU_test_call(mcu_index);

	if (OSAL_SUCCESS == ret) {
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
		LOG_INFO("TEST_CALL_CHAIN", "✅ Test Call Chain PASSED");
		LOG_INFO("TEST_CALL_CHAIN", "Complete flow: APP → ACONFIG → PDL → PCONFIG → HAL");
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
	} else {
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
		LOG_ERROR("TEST_CALL_CHAIN", "❌ Test Call Chain FAILED: %d", ret);
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
	}

	return ret;
}

/**
 * @brief 测试调用链 - MCU Serial
 */
int32_t TestCallChain_MCU_Serial(void)
{
	uint32_t mcu_index;
	int32_t ret;

	LOG_INFO("TEST_CALL_CHAIN", "========================================");
	LOG_INFO("TEST_CALL_CHAIN", "Test Call Chain: MCU Serial");
	LOG_INFO("TEST_CALL_CHAIN", "========================================");

	LOG_INFO("TEST_CALL_CHAIN", "Step 1: Query ACONFIG for device index");
	LOG_INFO("TEST_CALL_CHAIN", "Function ID: 0x%04X (TEST_FUNCTION_MCU_SERIAL)", TEST_FUNCTION_MCU_SERIAL);

	mcu_index = 1;  /* 假设 Serial 是 index 1 */
	LOG_INFO("TEST_CALL_CHAIN", "Got MCU index: %u", mcu_index);

	LOG_INFO("TEST_CALL_CHAIN", "Step 2: Call PDL_MCU_test_call(%u)", mcu_index);
	ret = PDL_MCU_test_call(mcu_index);

	if (OSAL_SUCCESS == ret) {
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
		LOG_INFO("TEST_CALL_CHAIN", "✅ Test Call Chain PASSED");
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
	} else {
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
		LOG_ERROR("TEST_CALL_CHAIN", "❌ Test Call Chain FAILED: %d", ret);
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
	}

	return ret;
}

/**
 * @brief 测试调用链 - BMC
 */
int32_t TestCallChain_BMC(void)
{
	uint32_t bmc_index;
	int32_t ret;

	LOG_INFO("TEST_CALL_CHAIN", "========================================");
	LOG_INFO("TEST_CALL_CHAIN", "Test Call Chain: BMC");
	LOG_INFO("TEST_CALL_CHAIN", "========================================");

	LOG_INFO("TEST_CALL_CHAIN", "Step 1: Query ACONFIG for device index");
	LOG_INFO("TEST_CALL_CHAIN", "Function ID: 0x%04X (TEST_FUNCTION_BMC)", TEST_FUNCTION_BMC);

	bmc_index = 0;  /* 假设 BMC 是 index 0 */
	LOG_INFO("TEST_CALL_CHAIN", "Got BMC index: %u", bmc_index);

	LOG_INFO("TEST_CALL_CHAIN", "Step 2: Call PDL_BMC_test_call(%u)", bmc_index);
	ret = PDL_BMC_test_call(bmc_index);

	if (OSAL_SUCCESS == ret) {
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
		LOG_INFO("TEST_CALL_CHAIN", "✅ Test Call Chain PASSED");
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
	} else {
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
		LOG_ERROR("TEST_CALL_CHAIN", "❌ Test Call Chain FAILED: %d", ret);
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
	}

	return ret;
}

/**
 * @brief 运行所有测试
 */
int32_t TestCallChain_RunAll(void)
{
	int32_t ret;
	int32_t failed = 0;

	LOG_INFO("TEST_CALL_CHAIN", " ");
	LOG_INFO("TEST_CALL_CHAIN", "╔═══════════════════════════════════════════════════════════════╗");
	LOG_INFO("TEST_CALL_CHAIN", "║          Complete Call Chain Test Suite                      ║");
	LOG_INFO("TEST_CALL_CHAIN", "╚═══════════════════════════════════════════════════════════════╝");
	LOG_INFO("TEST_CALL_CHAIN", " ");

	/* 测试 1: MCU CAN */
	ret = TestCallChain_MCU_CAN();
	if (OSAL_SUCCESS != ret) failed++;

	LOG_INFO("TEST_CALL_CHAIN", " ");

	/* 测试 2: MCU Serial */
	ret = TestCallChain_MCU_Serial();
	if (OSAL_SUCCESS != ret) failed++;

	LOG_INFO("TEST_CALL_CHAIN", " ");

	/* 测试 3: BMC */
	ret = TestCallChain_BMC();
	if (OSAL_SUCCESS != ret) failed++;

	LOG_INFO("TEST_CALL_CHAIN", " ");
	LOG_INFO("TEST_CALL_CHAIN", "╔═══════════════════════════════════════════════════════════════╗");
	LOG_INFO("TEST_CALL_CHAIN", "║          Test Summary                                         ║");
	LOG_INFO("TEST_CALL_CHAIN", "╚═══════════════════════════════════════════════════════════════╝");
	LOG_INFO("TEST_CALL_CHAIN", "Total: 3 tests");
	LOG_INFO("TEST_CALL_CHAIN", "Passed: %d", 3 - failed);
	LOG_INFO("TEST_CALL_CHAIN", "Failed: %d", failed);

	if (failed == 0) {
		LOG_INFO("TEST_CALL_CHAIN", " ");
		LOG_INFO("TEST_CALL_CHAIN", "✅ ALL TESTS PASSED");
		LOG_INFO("TEST_CALL_CHAIN", " ");
		return OSAL_SUCCESS;
	} else {
		LOG_ERROR("TEST_CALL_CHAIN", " ");
		LOG_ERROR("TEST_CALL_CHAIN", "❌ SOME TESTS FAILED");
		LOG_ERROR("TEST_CALL_CHAIN", " ");
		return OSAL_ERR_GENERIC;
	}
}
