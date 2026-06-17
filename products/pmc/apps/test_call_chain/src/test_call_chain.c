/**
 * @file test_call_chain.c
 * @brief 完整调用链测试用例
 * @note 验证 APP -> PMC_ACONFIG -> PDL -> PCONFIG -> HAL
 */

#include "osal.h"
#include "pdl.h"
#include "pmc_aconfig.h"
#include "pmc_tc_functions.h"

static int32_t run_mcu_tc_call(uint32_t function_id, const char *name)
{
	const pmc_tc_config_t *tc_config;
	pdl_mcu_handle_t handle = NULL;
	int32_t ret;

	LOG_INFO("TEST_CALL_CHAIN", "========================================");
	LOG_INFO("TEST_CALL_CHAIN", "Test Call Chain: %s", name);
	LOG_INFO("TEST_CALL_CHAIN", "========================================");

	LOG_INFO("TEST_CALL_CHAIN", "Step 1: Query PMC_ACONFIG for function 0x%04X", function_id);
	tc_config = PMC_ACONFIG_GetTcConfig(function_id);
	if (NULL == tc_config || !tc_config->enabled) {
		LOG_ERROR("TEST_CALL_CHAIN", "No enabled TC config for function 0x%04X", function_id);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	if (PMC_DEVICE_MCU != tc_config->device.type) {
		LOG_ERROR("TEST_CALL_CHAIN", "Function 0x%04X is not mapped to MCU: type=%u",
			  function_id, tc_config->device.type);
		return OSAL_ERR_NOT_SUPPORTED;
	}

	LOG_INFO("TEST_CALL_CHAIN", "Got MCU index: %u", tc_config->device.index);

	LOG_INFO("TEST_CALL_CHAIN", "Step 2: Call PDL_MCU_init(%u)", tc_config->device.index);
	ret = PDL_MCU_init(tc_config->device.index, &handle);
	if (OSAL_SUCCESS == ret) {
		(void)PDL_MCU_deinit(handle);
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
		LOG_INFO("TEST_CALL_CHAIN", "Test Call Chain PASSED");
		LOG_INFO("TEST_CALL_CHAIN", "Complete flow: APP -> PMC_ACONFIG -> PDL -> PCONFIG -> HAL");
		LOG_INFO("TEST_CALL_CHAIN", "========================================");
	} else {
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
		LOG_ERROR("TEST_CALL_CHAIN", "Test Call Chain FAILED: %d", ret);
		LOG_ERROR("TEST_CALL_CHAIN", "========================================");
	}

	return ret;
}

int32_t TestCallChain_MCU_CAN(void)
{
	return run_mcu_tc_call(PMC_TC_MCU_RESET, "MCU CAN");
}

int32_t TestCallChain_MCU_Serial(void)
{
	return run_mcu_tc_call(PMC_TC_POWER_OFF, "MCU Serial");
}

int32_t TestCallChain_BMC(void)
{
	return run_mcu_tc_call(PMC_TC_POWER_ON, "Power Control MCU");
}

int32_t TestCallChain_RunAll(void)
{
	int32_t ret;
	int32_t failed = 0;

	LOG_INFO("TEST_CALL_CHAIN", " ");
	LOG_INFO("TEST_CALL_CHAIN", "Complete Call Chain Test Suite");
	LOG_INFO("TEST_CALL_CHAIN", " ");

	ret = TestCallChain_MCU_CAN();
	if (OSAL_SUCCESS != ret) {
		failed++;
	}

	LOG_INFO("TEST_CALL_CHAIN", " ");

	ret = TestCallChain_MCU_Serial();
	if (OSAL_SUCCESS != ret) {
		failed++;
	}

	LOG_INFO("TEST_CALL_CHAIN", " ");

	ret = TestCallChain_BMC();
	if (OSAL_SUCCESS != ret) {
		failed++;
	}

	LOG_INFO("TEST_CALL_CHAIN", " ");
	LOG_INFO("TEST_CALL_CHAIN", "Test Summary");
	LOG_INFO("TEST_CALL_CHAIN", "Total: 3 tests");
	LOG_INFO("TEST_CALL_CHAIN", "Passed: %d", 3 - failed);
	LOG_INFO("TEST_CALL_CHAIN", "Failed: %d", failed);

	if (0 == failed) {
		LOG_INFO("TEST_CALL_CHAIN", "ALL TESTS PASSED");
		return OSAL_SUCCESS;
	}

	LOG_ERROR("TEST_CALL_CHAIN", "SOME TESTS FAILED");
	return OSAL_ERR_GENERIC;
}
