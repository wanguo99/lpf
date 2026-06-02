/**
 * @file test_framework.h
 * @brief 统一测试框架头文件
 *
 * 这是测试用例唯一需要包含的头文件，整合了所有测试框架功能。
 * 使用方式：
 *   #include "test_framework.h"
 *   #include <osal.h>  // 或其他被测试模块的头文件
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* 包含所有测试框架组件 */
#include "test_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "test_parameterized.h"

/* 简化的测试模块注册宏 */

/**
 * 开始定义测试模块
 * @param module_id 模块标识符（用作变量名）
 * @param layer_name 层级名称："OSAL", "HAL", "PCL", "PDL"
 */
#define TEST_MODULE_BEGIN(module_id, layer_name) \
    TEST_SUITE_BEGIN(module_id, #module_id, layer_name)

/**
 * 注册测试用例到模块
 * @param test_func 测试函数名
 * @param description 测试描述（可选，用于文档）
 */
#define TEST_CASE_REGISTER(test_func, description) \
    TEST_CASE_REF(test_func)

/**
 * 结束测试模块定义
 * @param module_id 模块标识符（必须与 TEST_MODULE_BEGIN 中的相同）
 * @param layer_name 层级名称（必须与 TEST_MODULE_BEGIN 中的相同）
 */
#define TEST_MODULE_END(module_id, layer_name) \
    TEST_SUITE_END(module_id, #module_id, layer_name)

/**
 * 带夹具的测试模块定义
 * @param module_id 模块标识符
 * @param layer_name 层级名称
 * @param setup_func 初始化函数
 * @param teardown_func 清理函数
 */
#define TEST_MODULE_BEGIN_WITH_FIXTURE(module_id, layer_name, setup_func, teardown_func) \
    static const test_case_t module_id##_cases[] = {

#define TEST_MODULE_END_WITH_FIXTURE(module_id, layer_name, setup_func, teardown_func) \
    }; \
    static const test_suite_t module_id##_suite = { \
        .suite_name = #module_id, \
        .module_name = #module_id, \
        .layer_name = layer_name, \
        .cases = module_id##_cases, \
        .case_count = sizeof(module_id##_cases) / sizeof(test_case_t), \
        .suite_setup = setup_func, \
        .suite_teardown = teardown_func \
    }; \
    __attribute__((constructor)) \
    static void register_##module_id(void) { \
        libutest_register_suite(&module_id##_suite); \
    }

/* 测试输出格式化宏 */
#define TEST_LOG_INFO(msg)     OSAL_Printf("[ INFO     ] %s\n", msg)
#define TEST_LOG_WARNING(msg)  OSAL_Printf("[ WARNING  ] %s\n", msg)
#define TEST_LOG_ERROR(msg)    OSAL_Printf("[ ERROR    ] %s\n", msg)

#endif /* TEST_FRAMEWORK_H */
