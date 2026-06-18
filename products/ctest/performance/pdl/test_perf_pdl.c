/**
 * @file test_perf_pdl.c
 * @brief PDL层性能测试
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_performance.h>
#include "pdl.h"

/**
 * 测试MCU状态查询性能
 */
static void _test_perf_mcu_status_query(void)
{
	// TODO: 实现MCU状态查询性能测试
	osal_printf(
		"[ INFO ] MCU status query performance test - not implemented yet\n");
}

/* 注册性能测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_perf_mcu_status_query",
	  .func = _test_perf_mcu_status_query,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "perf_pdl",
	.module_name = "perf_pdl",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_PERFORMANCE,
				  .tags = TEST_TAG_SLOW,
				  .timeout_ms = 5000,
				  .description = "PDL perf_pdl tests" }
};

/* 测试套件注册函数 */
void register_perf_pdl_tests(void)
{
	libutest_register_suite(&test_suite);
}
