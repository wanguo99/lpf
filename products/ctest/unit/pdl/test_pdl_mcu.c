/**
 * @file test_pdl_mcu.c
 * @brief PDL MCU外设驱动单元测试
 *
 * 注意：这些测试需要在 PCONFIG 中预先配置 MCU 设备
 */

#include <test_framework/test_framework.h>
#include "pconfig/pconfig.h"
#include "osal.h"
#include "pdl.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: MCU驱动初始化 - 有效索引 */
static void _test_pdl_mcu_init_valid_index(void)
{
	pdl_mcu_handle_t handle = NULL;

	/* 使用索引 0 初始化（假设 PCONFIG 中已配置） */
	int32_t ret = pdl_mcu_init(0, &handle);

	/* 如果配置存在且启用，应该成功 */
	if (ret == OSAL_SUCCESS) {
		TEST_ASSERT_NOT_NULL(handle);
		pdl_mcu_deinit(handle);
	} else {
		/* 配置不存在或未启用，跳过测试 */
		TEST_MESSAGE("SKIPPED: MCU index 0 not configured in PCONFIG");
	}
}

/* 测试用例: MCU驱动初始化 - 空句柄指针 */
static void _test_pdl_mcu_init_null_handle(void)
{
	int32_t ret = pdl_mcu_init(0, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: MCU驱动初始化 - 无效索引 */
static void _test_pdl_mcu_init_invalid_index(void)
{
	pdl_mcu_handle_t handle = NULL;

	/* 使用一个很大的无效索引 */
	int32_t ret = pdl_mcu_init(999, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: MCU驱动清理 */
static void _test_pdl_mcu_deinit(void)
{
	pdl_mcu_handle_t handle = NULL;

	int32_t ret = pdl_mcu_init(0, &handle);
	if (ret == OSAL_SUCCESS) {
		ret = pdl_mcu_deinit(handle);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	} else {
		TEST_MESSAGE("SKIPPED: MCU index 0 not configured");
	}
}

/* 测试用例: 清理空句柄 */
static void _test_pdl_mcu_deinit_null_handle(void)
{
	int32_t ret = pdl_mcu_deinit(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 版本信息测试
 *===========================================================================*/

/* 测试用例: 获取版本信息 - 空句柄 */
static void _test_pdl_mcu_get_version_null_handle(void)
{
	pdl_mcu_version_t version;
	int32_t ret = pdl_mcu_get_version(NULL, &version);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取版本信息 - 空输出指针 */
static void _test_pdl_mcu_get_version_null_output(void)
{
	pdl_mcu_handle_t handle = NULL;

	int32_t ret = pdl_mcu_init(0, &handle);
	if (ret == OSAL_SUCCESS) {
		ret = pdl_mcu_get_version(handle, NULL);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
		pdl_mcu_deinit(handle);
	} else {
		TEST_MESSAGE("SKIPPED: MCU index 0 not configured");
	}
}

/*===========================================================================
 * 命令发送测试
 *===========================================================================*/

/* 测试用例: 发送命令 - 空句柄 */
static void _test_pdl_mcu_send_command_null_handle(void)
{
	uint8_t cmd_data[] = { 0x01, 0x02, 0x03 };
	uint8_t resp_data[32];
	uint32_t resp_len;

	int32_t ret = pdl_mcu_send_command(NULL, 0x01, cmd_data, sizeof(cmd_data),
									   resp_data, sizeof(resp_data), &resp_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 发送命令 - 空响应缓冲区 */
static void _test_pdl_mcu_send_command_null_response(void)
{
	pdl_mcu_handle_t handle = NULL;
	uint8_t cmd_data[] = { 0x01, 0x02, 0x03 };
	uint32_t resp_len;

	int32_t ret = pdl_mcu_init(0, &handle);
	if (ret == OSAL_SUCCESS) {
		ret = pdl_mcu_send_command(handle, 0x01, cmd_data, sizeof(cmd_data),
								   NULL, 32, &resp_len);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
		pdl_mcu_deinit(handle);
	} else {
		TEST_MESSAGE("SKIPPED: MCU index 0 not configured");
	}
}

/*===========================================================================
 * 状态查询测试
 *===========================================================================*/

/* 测试用例: 获取状态 - 空句柄 */
static void _test_pdl_mcu_get_status_null_handle(void)
{
	pdl_mcu_status_t status;
	int32_t ret = pdl_mcu_get_status(NULL, &status);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取状态 - 空输出指针 */
static void _test_pdl_mcu_get_status_null_output(void)
{
	pdl_mcu_handle_t handle = NULL;

	int32_t ret = pdl_mcu_init(0, &handle);
	if (ret == OSAL_SUCCESS) {
		ret = pdl_mcu_get_status(handle, NULL);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
		pdl_mcu_deinit(handle);
	} else {
		TEST_MESSAGE("SKIPPED: MCU index 0 not configured");
	}
}

/*===========================================================================
 * 测试套件入口
 *===========================================================================*/

void test_pdl_mcu(void)
{
	_test_pdl_mcu_init_valid_index();
	_test_pdl_mcu_init_null_handle();
	_test_pdl_mcu_init_invalid_index();
	_test_pdl_mcu_deinit();
	_test_pdl_mcu_deinit_null_handle();

	_test_pdl_mcu_get_version_null_handle();
	_test_pdl_mcu_get_version_null_output();

	_test_pdl_mcu_send_command_null_handle();
	_test_pdl_mcu_send_command_null_response();

	_test_pdl_mcu_get_status_null_handle();
	_test_pdl_mcu_get_status_null_output();
}

static const test_case_t test_cases[] = {
	{ .name = "test_pdl_mcu",
	  .func = test_pdl_mcu,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "pdl_mcu",
	.module_name = "pdl_mcu",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_HARDWARE,
				  .timeout_ms = 5000,
				  .description = "PDL MCU tests" }
};

void register_pdl_mcu_tests(void)
{
	libutest_register_suite(&test_suite);
}
