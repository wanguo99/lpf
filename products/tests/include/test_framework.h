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
#include "test_mock.h"
#include "test_metadata.h"

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
 * 定义测试套件元数据
 * 必须在 TEST_MODULE_END 之前调用
 * @param module_id 模块标识符（必须与 TEST_MODULE_BEGIN 中的相同）
 * @param category 测试分类（TEST_CATEGORY_UNIT/PERFORMANCE/STRESS/SYSTEM）
 * @param tags 标签位掩码（TEST_TAG_FAST | TEST_TAG_HARDWARE 等）
 * @param timeout 超时时间（毫秒，0表示无限制）
 * @param desc 测试描述字符串
 *
 * 使用示例：
 *   TEST_MODULE_BEGIN(test_osal_mutex, "OSAL")
 *   TEST_CASE_REGISTER(test_mutex_create, "")
 *   TEST_SUITE_METADATA(test_osal_mutex, TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "OSAL mutex operations")
 *   TEST_MODULE_END(test_osal_mutex, "OSAL")
 */
#define TEST_SUITE_METADATA(module_id, category, tags, timeout, desc) \
    static const test_metadata_t module_id##_metadata = { \
        .category = (category), \
        .tags = (tags), \
        .timeout_ms = (timeout), \
        .description = (desc) \
    };

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

/* ============================================================================
 * 自动注册宏 - 简化测试用例编写
 * ============================================================================
 *
 * 新的简化 API 消除了 TEST_MODULE_BEGIN/END 的样板代码。
 * 开发者只需：
 *   1. 编写 TEST_CASE(test_name) { ... }
 *   2. 调用 TEST_SUITE_AUTO_REGISTER() 自动注册所有测试
 *
 * 示例：
 *   #include "test_framework.h"
 *   #include "osal.h"
 *
 *   TEST_CASE(test_mutex_create) { TEST_ASSERT_NOT_NULL(osal_mutex_create()); }
 *   TEST_CASE(test_mutex_lock) { ... }
 *
 *   TEST_SUITE_AUTO_REGISTER(OSAL, TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
 *                            "OSAL mutex operations");
 *
 * 宏自动从 __FILE__ 推导 suite 名称（test_osal_mutex.c -> osal_mutex）
 */

/**
 * 自动测试用例注册宏（使用 linker section）
 *
 * 此宏将测试用例信息放入特殊的 ELF section，允许运行时自动发现所有测试。
 *
 * 实现原理：
 *   1. 使用 __attribute__((section(".test_cases.SUITE_NAME")))
 *      将测试用例放入特定的 linker section
 *   2. 在 TEST_SUITE_AUTO_REGISTER 中，通过 extern 声明获取 section 的起始和结束地址
 *   3. 运行时遍历该 section 收集所有测试用例
 *
 * 注意：此宏需要 GCC/Clang 支持，并且依赖 linker script 定义 section 边界符号。
 *       如果平台不支持，将回退到传统的显式注册方式。
 */
#define TEST_CASE_AUTO(suite_name, test_name) \
    static void test_name(void); \
    __attribute__((used, section(".test_cases." #suite_name))) \
    static const auto_test_case_t auto_case_##test_name = { \
        .section_name = #suite_name, \
        .case_info = { \
            .name = #test_name, \
            .func = test_name, \
            .setup = NULL, \
            .teardown = NULL \
        } \
    }; \
    static void test_name(void)

/**
 * 从文件名推导 suite 名称的宏
 *
 * 推导规则：
 *   test_osal_mutex.c -> osal_mutex
 *   test_hal_can.c    -> hal_can
 *   test_pdl_mcu.c    -> pdl_mcu
 *
 * 实现限制：
 *   由于 C 预处理器无法进行字符串操作，我们需要开发者手动提供 suite_id。
 *   未来可以考虑使用外部工具（如 Python 脚本）自动生成带 suite_id 的代码。
 */
#define TEST_SUITE_DERIVE_NAME(file_name) file_name

/**
 * 自动注册测试套件（简化版）
 *
 * 此宏自动收集当前编译单元中的所有 TEST_CASE() 并注册为一个 suite。
 *
 * @param suite_id 套件标识符（必须手动提供，例如 osal_mutex）
 * @param layer_name 层级名称（"OSAL", "HAL", "PDL", "PRL", "PCONFIG", "ACONFIG"）
 * @param category 测试分类（TEST_CATEGORY_UNIT/PERFORMANCE/STRESS/SYSTEM）
 * @param tags 标签位掩码（TEST_TAG_FAST | TEST_TAG_HARDWARE 等）
 * @param timeout 超时时间（毫秒，0表示无限制）
 * @param desc 测试描述字符串
 *
 * 使用示例：
 *   // 文件：test_osal_mutex.c
 *   TEST_CASE(test_mutex_create) { ... }
 *   TEST_CASE(test_mutex_lock) { ... }
 *   TEST_CASE(test_mutex_unlock) { ... }
 *
 *   TEST_SUITE_AUTO_REGISTER(osal_mutex, "OSAL",
 *                            TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
 *                            "OSAL mutex operations");
 *
 * 注意：
 *   由于 C 预处理器限制，suite_id 必须手动提供。
 *   建议遵循命名约定：test_<layer>_<module>.c -> suite_id = <layer>_<module>
 */
#define TEST_SUITE_AUTO_REGISTER(suite_id, layer_name, category, tags, timeout, desc) \
    /* 声明 linker section 边界（由 linker script 提供） */ \
    extern auto_test_case_t __start_test_cases_##suite_id __attribute__((weak)); \
    extern auto_test_case_t __stop_test_cases_##suite_id __attribute__((weak)); \
    \
    /* 定义静态测试用例数组（作为 fallback，如果 linker section 不可用） */ \
    static const test_case_t suite_id##_cases_fallback[] = { \
        { .name = NULL, .func = NULL, .setup = NULL, .teardown = NULL } \
    }; \
    \
    /* 定义测试套件元数据 */ \
    static const test_metadata_t suite_id##_metadata = { \
        .category = (category), \
        .tags = (tags), \
        .timeout_ms = (timeout), \
        .description = (desc) \
    }; \
    \
    /* 运行时收集测试用例（从 linker section） */ \
    __attribute__((constructor)) \
    static void auto_register_##suite_id(void) { \
        /* 检查 linker section 是否可用 */ \
        if (&__start_test_cases_##suite_id == NULL || \
            &__stop_test_cases_##suite_id == NULL || \
            &__start_test_cases_##suite_id >= &__stop_test_cases_##suite_id) { \
            /* Linker section 不可用，使用 fallback */ \
            static const test_suite_t suite_id##_suite_fallback = { \
                .suite_name = #suite_id, \
                .module_name = #suite_id, \
                .layer_name = layer_name, \
                .cases = suite_id##_cases_fallback, \
                .case_count = 0, \
                .suite_setup = NULL, \
                .suite_teardown = NULL, \
                .metadata = suite_id##_metadata \
            }; \
            libutest_register_suite(&suite_id##_suite_fallback); \
        } else { \
            /* 计算 section 中的测试用例数量 */ \
            size_t case_count = ((char*)&__stop_test_cases_##suite_id - \
                                 (char*)&__start_test_cases_##suite_id) / \
                                sizeof(auto_test_case_t); \
            \
            /* 分配测试用例数组（注意：这里使用静态数组避免动态分配） */ \
            static test_case_t suite_id##_cases_collected[64]; /* 假设最多64个测试用例 */ \
            if (case_count > 64) case_count = 64; \
            \
            /* 从 section 复制测试用例信息 */ \
            for (size_t i = 0; i < case_count; i++) { \
                auto_test_case_t *auto_case = &__start_test_cases_##suite_id + i; \
                suite_id##_cases_collected[i] = auto_case->case_info; \
            } \
            \
            /* 定义并注册测试套件 */ \
            static const test_suite_t suite_id##_suite = { \
                .suite_name = #suite_id, \
                .module_name = #suite_id, \
                .layer_name = layer_name, \
                .cases = suite_id##_cases_collected, \
                .case_count = case_count, \
                .suite_setup = NULL, \
                .suite_teardown = NULL, \
                .metadata = suite_id##_metadata \
            }; \
            libutest_register_suite(&suite_id##_suite); \
        } \
    }

/**
 * 简化版自动注册（使用默认元数据）
 *
 * 适用于快速编写测试，使用默认配置：
 *   - category: TEST_CATEGORY_UNIT
 *   - tags: TEST_TAG_FAST
 *   - timeout: 1000ms
 *   - description: 从 suite_id 生成
 */
#define TEST_SUITE_AUTO_REGISTER_SIMPLE(suite_id, layer_name) \
    TEST_SUITE_AUTO_REGISTER(suite_id, layer_name, \
                             TEST_CATEGORY_UNIT, TEST_TAG_FAST, 1000, \
                             "Test suite: " #suite_id)

/* ============================================================================
 * 过渡方案宏 - 立即可用的简化注册
 * ============================================================================
 *
 * 在完全自动化（linker section）实现之前，提供简化的注册方式。
 * 相比传统的 TEST_MODULE_BEGIN/END，减少了大量样板代码。
 *
 * 使用步骤：
 *   1. 定义 TEST_CASE() 函数
 *   2. 创建 test_case_t 数组（手动列出测试）
 *   3. 创建 test_metadata_t 结构
 *   4. 调用 TEST_SUITE_REGISTER_CASES() 一次性注册
 */

/**
 * 手动注册测试用例数组（过渡方案）
 *
 * 此宏用于手动注册测试用例数组，适用于不依赖 linker section 的场景。
 *
 * @param suite_id 套件标识符
 * @param layer_str 层级名称
 * @param cases_array 测试用例数组（test_case_t[]）
 * @param meta 测试元数据（test_metadata_t）
 *
 * 使用示例：
 *   static const test_case_t my_suite_cases[] = {
 *       { .name = "test_1", .func = test_1, .setup = NULL, .teardown = NULL },
 *       { .name = "test_2", .func = test_2, .setup = NULL, .teardown = NULL },
 *   };
 *
 *   static const test_metadata_t my_suite_metadata = {
 *       .category = TEST_CATEGORY_UNIT,
 *       .tags = TEST_TAG_FAST,
 *       .timeout_ms = 100,
 *       .description = "My test suite"
 *   };
 *
 *   TEST_SUITE_REGISTER_CASES(my_suite, "OSAL", my_suite_cases, my_suite_metadata);
 */
#define TEST_SUITE_REGISTER_CASES(suite_id, layer_str, cases_array, meta) \
    static const test_suite_t suite_id##_suite = { \
        .suite_name = #suite_id, \
        .module_name = #suite_id, \
        .layer_name = layer_str, \
        .cases = cases_array, \
        .case_count = sizeof(cases_array) / sizeof(test_case_t), \
        .suite_setup = NULL, \
        .suite_teardown = NULL, \
        .metadata = meta \
    }; \
    __attribute__((constructor)) \
    static void register_##suite_id(void) { \
        libutest_register_suite(&suite_id##_suite); \
    }

/**
 * 简化版测试用例数组元素定义
 *
 * 减少手动创建 test_case_t 数组时的重复代码。
 *
 * 使用示例：
 *   static const test_case_t my_cases[] = {
 *       TEST_CASE_ENTRY(test_1),
 *       TEST_CASE_ENTRY(test_2),
 *       TEST_CASE_ENTRY(test_3),
 *   };
 */
#define TEST_CASE_ENTRY(test_func) \
    { .name = #test_func, .func = test_func, .setup = NULL, .teardown = NULL }

/**
 * 带 fixture 的测试用例数组元素定义
 *
 * 使用示例：
 *   static const test_case_t my_cases[] = {
 *       TEST_CASE_ENTRY_WITH_FIXTURE(test_1, setup_func, teardown_func),
 *       TEST_CASE_ENTRY_WITH_FIXTURE(test_2, setup_func, teardown_func),
 *   };
 */
#define TEST_CASE_ENTRY_WITH_FIXTURE(test_func, setup_func, teardown_func) \
    { .name = #test_func, .func = test_func, .setup = setup_func, .teardown = teardown_func }

/**
 * 定义默认元数据
 *
 * 快速创建具有默认设置的元数据结构。
 *
 * 使用示例：
 *   TEST_METADATA_DEFAULT_UNIT("My test description");
 */
#define TEST_METADATA_DEFAULT_UNIT(desc) \
    { \
        .category = TEST_CATEGORY_UNIT, \
        .tags = TEST_TAG_FAST, \
        .timeout_ms = 1000, \
        .description = desc \
    }

#define TEST_METADATA_DEFAULT_PERF(desc) \
    { \
        .category = TEST_CATEGORY_PERFORMANCE, \
        .tags = TEST_TAG_SLOW, \
        .timeout_ms = 5000, \
        .description = desc \
    }

#define TEST_METADATA_DEFAULT_STRESS(desc) \
    { \
        .category = TEST_CATEGORY_STRESS, \
        .tags = TEST_TAG_SLOW, \
        .timeout_ms = 10000, \
        .description = desc \
    }

#define TEST_METADATA_DEFAULT_SYSTEM(desc) \
    { \
        .category = TEST_CATEGORY_SYSTEM, \
        .tags = TEST_TAG_SLOW, \
        .timeout_ms = 5000, \
        .description = desc \
    }

/**
 * 完整的简化注册宏（包含元数据内联定义）
 *
 * 这是最简化的形式，适用于快速添加测试。
 *
 * @param suite_id 套件标识符
 * @param layer_str 层级名称
 * @param cases_array 测试用例数组
 * @param cat 测试分类
 * @param tag_bits 标签位掩码
 * @param timeout_val 超时时间（毫秒）
 * @param desc 描述字符串
 *
 * 使用示例：
 *   static const test_case_t my_cases[] = {
 *       TEST_CASE_ENTRY(test_1),
 *       TEST_CASE_ENTRY(test_2),
 *   };
 *
 *   TEST_SUITE_REGISTER(my_suite, "OSAL", my_cases,
 *                       TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
 *                       "My test suite");
 */
#define TEST_SUITE_REGISTER(suite_id, layer_str, cases_array, cat, tag_bits, timeout_val, desc) \
    static const test_metadata_t suite_id##_metadata = { \
        .category = (cat), \
        .tags = (tag_bits), \
        .timeout_ms = (timeout_val), \
        .description = (desc) \
    }; \
    TEST_SUITE_REGISTER_CASES(suite_id, layer_str, cases_array, suite_id##_metadata)

/* 测试输出格式化宏 */
#define TEST_LOG_INFO(msg)     OSAL_Printf("[ INFO     ] %s\n", msg)
#define TEST_LOG_WARNING(msg)  OSAL_Printf("[ WARNING  ] %s\n", msg)
#define TEST_LOG_ERROR(msg)    OSAL_Printf("[ ERROR    ] %s\n", msg)

#endif /* TEST_FRAMEWORK_H */
