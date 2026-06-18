/**
 * @file test_pconfig_api.c
 * @brief PCONFIG API unit tests
 */

#include <test_framework/test_framework.h>
#include "osal.h"
#include "pconfig/pconfig.h"

static void _test_pconfig_get_board(void)
{
	const pconfig_platform_config_t *cfg = pconfig_get_board();
	TEST_ASSERT_NOT_NULL(cfg);
	TEST_ASSERT_EQUAL(0, osal_strcmp(cfg->platform_name, "generic"));
	TEST_ASSERT_EQUAL(0, osal_strcmp(cfg->product_name, "middleware"));
}

static void _test_pconfig_find_by_name(void)
{
	const pconfig_platform_config_t *cfg =
		pconfig_find("ti", "framework", NULL);
	TEST_ASSERT_NOT_NULL(cfg);
	TEST_ASSERT_EQUAL(0, osal_strcmp(cfg->chip_name, "am625"));
}

static void _test_pconfig_find_alt_config(void)
{
	const pconfig_platform_config_t *cfg =
		pconfig_find("ti", "framework_alt", NULL);
	TEST_ASSERT_NOT_NULL(cfg);
	TEST_ASSERT_EQUAL(0, osal_strcmp(cfg->product_name, "framework_alt"));
}

static void _test_pconfig_find_missing_config(void)
{
	const pconfig_platform_config_t *cfg =
		pconfig_find("missing", "framework", NULL);
	TEST_ASSERT_NULL(cfg);
}

static void _test_pconfig_hw_get_mcu_by_id(void)
{
	const pconfig_platform_config_t *platform =
		pconfig_find("ti", "framework", NULL);
	const pconfig_mcu_entry_t *mcu = pconfig_hw_get_mcu(platform, 0);
	TEST_ASSERT_NOT_NULL(mcu);
	TEST_ASSERT_EQUAL(0, osal_strcmp(mcu->config.name, "mcu0"));
}

static void _test_pconfig_hw_get_mcu_second_entry(void)
{
	const pconfig_platform_config_t *platform =
		pconfig_find("ti", "framework", NULL);
	const pconfig_mcu_entry_t *mcu = pconfig_hw_get_mcu(platform, 1);
	TEST_ASSERT_NOT_NULL(mcu);
	TEST_ASSERT_EQUAL(0, osal_strcmp(mcu->config.name, "mcu1"));
}

static void _test_pconfig_validate_success(void)
{
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pconfig_validate(pconfig_get_board()));
}

static void _test_pconfig_list(void)
{
	const pconfig_platform_config_t *configs[10];
	uint32_t count = 10;

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pconfig_list(configs, &count));
	TEST_ASSERT_EQUAL(3u, count);
	TEST_ASSERT_NOT_NULL(configs[0]);
	TEST_ASSERT_NOT_NULL(configs[1]);
	TEST_ASSERT_NOT_NULL(configs[2]);
}

static const test_case_t test_cases[] = {
	{ .name = "test_pconfig_get_board",
	  .func = _test_pconfig_get_board,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_find_by_name",
	  .func = _test_pconfig_find_by_name,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_find_alt_config",
	  .func = _test_pconfig_find_alt_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_find_missing_config",
	  .func = _test_pconfig_find_missing_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_hw_get_mcu_by_id",
	  .func = _test_pconfig_hw_get_mcu_by_id,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_hw_get_mcu_second_entry",
	  .func = _test_pconfig_hw_get_mcu_second_entry,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_validate_success",
	  .func = _test_pconfig_validate_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pconfig_list",
	  .func = _test_pconfig_list,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "pconfig_api",
	.module_name = "pconfig",
	.layer_name = "UNIT",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "PCONFIG API unit tests" }
};

void register_pconfig_api_tests(void)
{
	libutest_register_suite(&test_suite);
}
