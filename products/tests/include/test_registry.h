/**
 * @file test_registry.h
 * @brief 测试注册宏
 *
 * 提供自动测试注册宏，使用GCC constructor属性实现。
 * 测试用例在程序启动时自动注册，无需手动维护注册列表。
 */

#ifndef TEST_REGISTRY_H
#define TEST_REGISTRY_H

#include "test_core.h"

/* Define a test case */
#define TEST_CASE(name) \
    static void name(void)

/* Define a test case with fixtures */
#define TEST_CASE_WITH_FIXTURE(name, setup_func, teardown_func) \
    static void name(void)

/* Begin test suite definition */
#define TEST_SUITE_BEGIN(suite_id, module, layer) \
    static const test_case_t suite_id##_cases[] = {

/* Add test case to suite */
#define TEST_CASE_REF(test_name) \
    { \
        .name = #test_name, \
        .func = test_name, \
        .setup = NULL, \
        .teardown = NULL \
    },

/* End test suite definition and auto-register */
#define TEST_SUITE_END(suite_id, module, layer) \
    }; \
    static const test_suite_t suite_id##_suite = { \
        .suite_name = #suite_id, \
        .module_name = module, \
        .layer_name = layer, \
        .cases = suite_id##_cases, \
        .case_count = sizeof(suite_id##_cases) / sizeof(test_case_t), \
        .suite_setup = NULL, \
        .suite_teardown = NULL, \
        .metadata = TEST_METADATA_DEFAULT \
    }; \
    __attribute__((constructor)) \
    static void register_##suite_id(void) { \
        libutest_register_suite(&suite_id##_suite); \
    }

/* End test suite definition with metadata and auto-register */
#define TEST_SUITE_END_WITH_METADATA(suite_id, module, layer, meta) \
    }; \
    static const test_suite_t suite_id##_suite = { \
        .suite_name = #suite_id, \
        .module_name = module, \
        .layer_name = layer, \
        .cases = suite_id##_cases, \
        .case_count = sizeof(suite_id##_cases) / sizeof(test_case_t), \
        .suite_setup = NULL, \
        .suite_teardown = NULL, \
        .metadata = meta \
    }; \
    __attribute__((constructor)) \
    static void register_##suite_id(void) { \
        libutest_register_suite(&suite_id##_suite); \
    }

/* Simplified macros for common use cases */
#define TEST_MODULE_BEGIN(module_id, layer_name) \
    TEST_SUITE_BEGIN(module_id, #module_id, layer_name)

#define TEST_MODULE_END(module_id, layer_name) \
    TEST_SUITE_END(module_id, #module_id, layer_name)

/* Suite with setup/teardown */
#define TEST_SUITE_END_WITH_FIXTURE(suite_id, module, layer, setup, teardown) \
    }; \
    static const test_suite_t suite_id##_suite = { \
        .suite_name = #suite_id, \
        .module_name = module, \
        .layer_name = layer, \
        .cases = suite_id##_cases, \
        .case_count = sizeof(suite_id##_cases) / sizeof(test_case_t), \
        .suite_setup = setup, \
        .suite_teardown = teardown, \
        .metadata = TEST_METADATA_DEFAULT \
    }; \
    __attribute__((constructor)) \
    static void register_##suite_id(void) { \
        libutest_register_suite(&suite_id##_suite); \
    }

/* Suite with setup/teardown and metadata */
#define TEST_SUITE_END_WITH_FIXTURE_AND_METADATA(suite_id, module, layer, setup, teardown, meta) \
    }; \
    static const test_suite_t suite_id##_suite = { \
        .suite_name = #suite_id, \
        .module_name = module, \
        .layer_name = layer, \
        .cases = suite_id##_cases, \
        .case_count = sizeof(suite_id##_cases) / sizeof(test_case_t), \
        .suite_setup = setup, \
        .suite_teardown = teardown, \
        .metadata = meta \
    }; \
    __attribute__((constructor)) \
    static void register_##suite_id(void) { \
        libutest_register_suite(&suite_id##_suite); \
    }

#endif /* TEST_REGISTRY_H */
