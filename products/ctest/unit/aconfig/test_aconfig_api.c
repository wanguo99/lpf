/**
 * @file test_aconfig_api.c
 * @brief ACONFIG API 单元测试
 *
 * 测试通用配置框架的只读查询 API 功能
 */

#include <test_framework/test_framework.h>
#include "osal.h"
#include "aconfig.h"

static void _test_aconfig_get_table(void)
{
	const aconfig_config_table_t *table = aconfig_get_table();

	TEST_ASSERT_NOT_NULL(table);
	TEST_ASSERT_EQUAL_STRING("ctest_aconfig", table->name);
	TEST_ASSERT_NULL(table->function_map);
	TEST_ASSERT_NULL(table->user_data);
}

static const test_case_t test_cases[] = {
	{ .name = "test_aconfig_get_table",
	  .func = _test_aconfig_get_table,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "aconfig_api",
	.module_name = "aconfig",
	.layer_name = "UNIT",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "ACONFIG API unit tests" }
};

void register_aconfig_api_tests(void)
{
	libutest_register_suite(&test_suite);
}
