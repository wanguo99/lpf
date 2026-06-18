#include <test_framework/test_framework.h>
/**
 * @file test_osal_env.c
 * @brief OSAL环境变量操作单元测试
 */

#include "osal.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

static void _test_osal_getenv_existing(void)
{
	/* Get an existing environment variable */
	char *value = osal_getenv("PATH");
	TEST_ASSERT_NOT_NULL(value);
	TEST_ASSERT_TRUE(osal_strlen(value) > 0);
}

static void _test_osal_getenv_nonexistent(void)
{
	/* Get a non-existent environment variable */
	char *value = osal_getenv("OSAL_NONEXISTENT_VAR_12345");
	TEST_ASSERT_NULL(value);
}

static void _test_osal_setenv_new_variable(void)
{
	/* Set a new environment variable */
	int32_t ret = osal_setenv("OSAL_TEST_VAR", "test_value", 1);
	TEST_ASSERT_EQUAL(0, ret);

	/* Verify it was set */
	char *value = osal_getenv("OSAL_TEST_VAR");
	TEST_ASSERT_NOT_NULL(value);
	TEST_ASSERT_STRING_EQUAL("test_value", value);

	/* Clean up */
	osal_unsetenv("OSAL_TEST_VAR");
}

static void _test_osal_setenv_overwrite(void)
{
	/* Set initial value */
	osal_setenv("OSAL_TEST_VAR2", "initial", 1);

	/* Overwrite with new value */
	int32_t ret = osal_setenv("OSAL_TEST_VAR2", "updated", 1);
	TEST_ASSERT_EQUAL(0, ret);

	/* Verify it was updated */
	char *value = osal_getenv("OSAL_TEST_VAR2");
	TEST_ASSERT_NOT_NULL(value);
	TEST_ASSERT_STRING_EQUAL("updated", value);

	/* Clean up */
	osal_unsetenv("OSAL_TEST_VAR2");
}

static void _test_osal_setenv_no_overwrite(void)
{
	/* Set initial value */
	osal_setenv("OSAL_TEST_VAR3", "initial", 1);

	/* Try to set without overwrite flag */
	int32_t ret = osal_setenv("OSAL_TEST_VAR3", "should_not_change", 0);
	TEST_ASSERT_EQUAL(0, ret);

	/* Verify original value is preserved */
	char *value = osal_getenv("OSAL_TEST_VAR3");
	TEST_ASSERT_NOT_NULL(value);
	TEST_ASSERT_STRING_EQUAL("initial", value);

	/* Clean up */
	osal_unsetenv("OSAL_TEST_VAR3");
}

static void _test_osal_unsetenv_existing(void)
{
	/* Set a variable */
	osal_setenv("OSAL_TEST_VAR4", "temp_value", 1);

	/* Unset it */
	int32_t ret = osal_unsetenv("OSAL_TEST_VAR4");
	TEST_ASSERT_EQUAL(0, ret);

	/* Verify it's gone */
	char *value = osal_getenv("OSAL_TEST_VAR4");
	TEST_ASSERT_NULL(value);
}

static void _test_osal_unsetenv_nonexistent(void)
{
	/* Unset a non-existent variable (should succeed) */
	int32_t ret = osal_unsetenv("OSAL_NONEXISTENT_VAR_67890");
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_osal_env_empty_value(void)
{
	/* Set variable with empty value */
	int32_t ret = osal_setenv("OSAL_TEST_VAR5", "", 1);
	TEST_ASSERT_EQUAL(0, ret);

	/* Verify it exists but is empty */
	char *value = osal_getenv("OSAL_TEST_VAR5");
	TEST_ASSERT_NOT_NULL(value);
	TEST_ASSERT_STRING_EQUAL("", value);

	/* Clean up */
	osal_unsetenv("OSAL_TEST_VAR5");
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_osal_getenv_existing",
	  .func = _test_osal_getenv_existing,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_getenv_nonexistent",
	  .func = _test_osal_getenv_nonexistent,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_setenv_new_variable",
	  .func = _test_osal_setenv_new_variable,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_setenv_overwrite",
	  .func = _test_osal_setenv_overwrite,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_setenv_no_overwrite",
	  .func = _test_osal_setenv_no_overwrite,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_unsetenv_existing",
	  .func = _test_osal_unsetenv_existing,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_unsetenv_nonexistent",
	  .func = _test_osal_unsetenv_nonexistent,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_env_empty_value",
	  .func = _test_osal_env_empty_value,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_env",
	.module_name = "osal_env",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "OSAL osal_env tests" }
};

/* 测试套件注册函数 */
void register_osal_env_tests(void)
{
	libutest_register_suite(&test_suite);
}
