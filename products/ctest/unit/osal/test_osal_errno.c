#include <test_framework/test_framework.h>
/**
 * @file test_osal_errno.c
 * @brief OSAL错误码操作单元测试
 */

#include "osal.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

static void _test_osal_get_errno(void)
{
	/* Set errno to a known value */
	osal_set_errno(OSAL_EINVAL);

	/* Get errno */
	int32_t err = osal_get_errno();
	TEST_ASSERT_EQUAL(OSAL_EINVAL, err);
}

static void _test_osal_set_errno(void)
{
	/* Set errno to different values */
	osal_set_errno(OSAL_ENOENT);
	TEST_ASSERT_EQUAL(OSAL_ENOENT, osal_get_errno());

	osal_set_errno(OSAL_ENOMEM);
	TEST_ASSERT_EQUAL(OSAL_ENOMEM, osal_get_errno());

	osal_set_errno(0);
	TEST_ASSERT_EQUAL(0, osal_get_errno());
}

static void _test_osal_strerror(void)
{
	/* Test common error codes */
	const char *msg;

	msg = osal_strerror(OSAL_EINVAL);
	TEST_ASSERT_NOT_NULL(msg);
	TEST_ASSERT_TRUE(osal_strlen(msg) > 0);

	msg = osal_strerror(OSAL_ENOENT);
	TEST_ASSERT_NOT_NULL(msg);
	TEST_ASSERT_TRUE(osal_strlen(msg) > 0);

	msg = osal_strerror(OSAL_ENOMEM);
	TEST_ASSERT_NOT_NULL(msg);
	TEST_ASSERT_TRUE(osal_strlen(msg) > 0);
}

static void _test_osal_get_status_name(void)
{
	/* Test OSAL status code names */
	const char *name;

	name = osal_get_status_name(OSAL_SUCCESS);
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_STRING_EQUAL("OSAL_SUCCESS", name);

	name = osal_get_status_name(OSAL_ERR_INVALID_POINTER);
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_STRING_EQUAL("OSAL_ERR_INVALID_POINTER", name);

	name = osal_get_status_name(OSAL_ERR_INVALID_SIZE);
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_STRING_EQUAL("OSAL_ERR_INVALID_SIZE", name);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_osal_get_errno",
	  .func = _test_osal_get_errno,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_set_errno",
	  .func = _test_osal_set_errno,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_strerror",
	  .func = _test_osal_strerror,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_osal_get_status_name",
	  .func = _test_osal_get_status_name,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_errno",
	.module_name = "osal_errno",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "OSAL osal_errno tests" }
};

/* 测试套件注册函数 */
void register_osal_errno_tests(void)
{
	libutest_register_suite(&test_suite);
}
