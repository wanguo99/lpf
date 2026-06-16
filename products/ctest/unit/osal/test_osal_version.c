/**
 * @file test_osal_version.c
 * @brief OSAL版本信息单元测试
 */

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

static void test_osal_get_version_string(void)
{
    const char *version = OSAL_get_version_string();

    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE(OSAL_strlen(version) > 0);

    /* Version string should contain "OSAL" */
    TEST_ASSERT_TRUE(OSAL_strstr(version, "OSAL") != NULL);
}

static void test_osal_version_format(void)
{
    const char *version = OSAL_get_version_string();

    /* Version should contain version number pattern */
    bool has_v = (OSAL_strstr(version, "v") != NULL || OSAL_strstr(version, "V") != NULL);
    bool has_dot = false;

    /* Check for dot manually */
    for (const char *p = version; *p != '\0'; p++) {
        if (*p == '.') {
            has_dot = true;
            break;
        }
    }

    TEST_ASSERT_TRUE(has_v || has_dot);
}

static void test_osal_version_consistency(void)
{
    /* Multiple calls should return the same string */
    const char *v1 = OSAL_get_version_string();
    const char *v2 = OSAL_get_version_string();

    TEST_ASSERT_STRING_EQUAL(v1, v2);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_osal_get_version_string",
		.func = test_osal_get_version_string,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_version_format",
		.func = test_osal_version_format,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_version_consistency",
		.func = test_osal_version_consistency,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_version",
	.module_name = "osal_version",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "OSAL osal_version tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_osal_version_tests(void)
{
	libutest_register_suite(&test_suite);
}
