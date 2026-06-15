/**
 * @file test_registry.h
 * @brief 测试注册接口 - 函数指针数组方式
 *
 * 提供纯函数指针数组的测试注册机制，不使用任何宏定义。
 * 测试用例通过显式的函数调用进行注册。
 */

#ifndef TEST_REGISTRY_H
#define TEST_REGISTRY_H

#include "test_core.h"

/**
 * 创建测试用例
 *
 * @param name 测试名称
 * @param func 测试函数指针
 * @param setup 初始化函数指针（可选，NULL表示不需要）
 * @param teardown 清理函数指针（可选，NULL表示不需要）
 * @return 测试用例结构体
 */
static inline test_case_t test_case_create(const char *name,
                                           test_func_t func,
                                           fixture_func_t setup,
                                           fixture_func_t teardown)
{
	test_case_t test_case = {
		.name = name,
		.func = func,
		.setup = setup,
		.teardown = teardown
	};
	return test_case;
}

/**
 * 创建测试套件
 *
 * @param suite_name 套件名称
 * @param module_name 模块名称
 * @param layer_name 层级名称
 * @param cases 测试用例数组
 * @param case_count 测试用例数量
 * @param suite_setup 套件初始化函数（可选）
 * @param suite_teardown 套件清理函数（可选）
 * @param metadata 测试元数据
 * @return 测试套件结构体
 */
static inline test_suite_t test_suite_create(const char *suite_name,
                                             const char *module_name,
                                             const char *layer_name,
                                             const test_case_t *cases,
                                             uint32_t case_count,
                                             fixture_func_t suite_setup,
                                             fixture_func_t suite_teardown,
                                             test_metadata_t metadata)
{
	test_suite_t suite = {
		.suite_name = suite_name,
		.module_name = module_name,
		.layer_name = layer_name,
		.cases = cases,
		.case_count = case_count,
		.suite_setup = suite_setup,
		.suite_teardown = suite_teardown,
		.metadata = metadata
	};
	return suite;
}

/**
 * 创建测试元数据
 *
 * @param category 测试分类
 * @param tags 标签位掩码
 * @param timeout_ms 超时时间（毫秒）
 * @param description 描述字符串
 * @return 测试元数据结构体
 */
static inline test_metadata_t test_metadata_create(test_category_t category,
                                                    uint32_t tags,
                                                    uint32_t timeout_ms,
                                                    const char *description)
{
	test_metadata_t metadata = {
		.category = category,
		.tags = tags,
		.timeout_ms = timeout_ms,
		.description = description
	};
	return metadata;
}

/* 内部函数声明 */
const test_suite_t** test_get_all_suites(uint32_t *count);
const test_suite_t* test_find_suite(const char *name);
uint32_t test_get_suites_by_layer(const char *layer_name,
                                   const test_suite_t **suites,
                                   uint32_t max_suites);
uint32_t test_get_suites_by_module(const char *module_name,
                                    const test_suite_t **suites,
                                    uint32_t max_suites);
uint32_t test_get_filtered_suites(const test_filter_t *filter,
                                   const test_suite_t **suites,
                                   uint32_t max_suites);
uint32_t test_get_suites_by_layer_filtered(const char *layer_name,
                                            const test_filter_t *filter,
                                            const test_suite_t **suites,
                                            uint32_t max_suites);
uint32_t test_get_suites_by_module_filtered(const char *module_name,
                                             const test_filter_t *filter,
                                             const test_suite_t **suites,
                                             uint32_t max_suites);
uint32_t test_get_suites_by_category(test_category_t category,
                                      const test_suite_t **suites,
                                      uint32_t max_suites);
uint32_t test_get_suites_by_tags(uint32_t tags,
                                  const test_suite_t **suites,
                                  uint32_t max_suites);
uint32_t test_get_layers(const char **layers, uint32_t max_layers);
uint32_t test_get_modules(const char **modules, uint32_t max_modules);

#endif /* TEST_REGISTRY_H */
