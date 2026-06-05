/**
 * @file test_example_minimal.c
 * @brief 最小化测试用例示例（使用新的简化 API）
 *
 * 本文件演示如何使用新的 TEST_SUITE_AUTO_REGISTER 宏来简化测试编写：
 *   1. 只需包含 test_framework.h 和被测模块头文件
 *   2. 使用 TEST_CASE(name) 定义测试用例
 *   3. 调用 TEST_SUITE_AUTO_REGISTER 一次性注册所有测试
 *
 * 不再需要：
 *   - TEST_MODULE_BEGIN/END 样板代码
 *   - 手动 TEST_CASE_REF() 引用每个测试
 *   - 维护测试用例列表
 *
 * 编译配置：
 *   需要在 Kconfig 中启用对应的测试选项（例如 CONFIG_TEST_EXAMPLE_MINIMAL）
 */

#include "test_framework.h"
#include "osal.h"  /* 被测试的模块 */

/* ============================================================================
 * 测试用例定义 - 传统方式（仍然支持）
 * ============================================================================
 * 如果不使用 linker section 自动收集，可以继续使用传统的显式注册方式。
 */

/**
 * 测试用例 1：基本断言
 */
TEST_CASE(test_basic_assertions)
{
    /* 基本相等性断言 */
    TEST_ASSERT_EQUAL(1, 1);
    TEST_ASSERT_EQUAL(0, 0);

    /* 布尔断言 */
    TEST_ASSERT_TRUE(1 == 1);
    TEST_ASSERT_FALSE(1 == 2);

    /* 指针断言 */
    int value = 42;
    TEST_ASSERT_NOT_NULL(&value);
    TEST_ASSERT_NULL(NULL);
}

/**
 * 测试用例 2：字符串操作
 */
TEST_CASE(test_string_operations)
{
    const char *str1 = "hello";
    const char *str2 = "hello";
    const char *str3 = "world";

    /* 字符串相等性 */
    TEST_ASSERT_STR_EQUAL(str1, str2);
    TEST_ASSERT_STR_NOT_EQUAL(str1, str3);

    /* 字符串长度 */
    TEST_ASSERT_EQUAL(5, OSAL_Strlen(str1));
    TEST_ASSERT_EQUAL(5, OSAL_Strlen(str3));
}

/**
 * 测试用例 3：数值比较
 */
TEST_CASE(test_numeric_comparisons)
{
    int32_t a = 10;
    int32_t b = 20;
    int32_t c = 10;

    /* 数值比较 */
    TEST_ASSERT_EQUAL(a, c);
    TEST_ASSERT_NOT_EQUAL(a, b);

    /* 范围检查 */
    TEST_ASSERT_TRUE(a < b);
    TEST_ASSERT_TRUE(b > a);
    TEST_ASSERT_TRUE(a <= c);
    TEST_ASSERT_TRUE(a >= c);
}

/**
 * 测试用例 4：内存操作
 */
TEST_CASE(test_memory_operations)
{
    /* 分配内存 */
    void *ptr = OSAL_Malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 初始化内存 */
    OSAL_Memset(ptr, 0xAA, 100);

    /* 验证内存内容 */
    uint8_t *bytes = (uint8_t *)ptr;
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL(0xAA, bytes[i]);
    }

    /* 释放内存 */
    OSAL_Free(ptr);
}

/**
 * 测试用例 5：错误处理
 */
TEST_CASE(test_error_handling)
{
    /* 测试无效参数 */
    int32_t ret = OSAL_Init(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_OK, ret);

    /* 测试正常初始化 */
    osal_config_t config = { .max_threads = 10 };
    ret = OSAL_Init(&config);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);
}

/* ============================================================================
 * 自动注册测试套件
 * ============================================================================
 *
 * 使用 TEST_SUITE_AUTO_REGISTER 宏自动注册所有测试用例。
 *
 * 参数说明：
 *   - suite_id: 测试套件标识符（建议使用文件名去掉 test_ 前缀和 .c 后缀）
 *               例如：test_example_minimal.c -> example_minimal
 *   - layer_name: 所属层级（"OSAL", "HAL", "PDL", "PRL", "PCONFIG", "ACONFIG", "EXAMPLE"）
 *   - category: 测试分类
 *               - TEST_CATEGORY_UNIT: 单元测试
 *               - TEST_CATEGORY_PERFORMANCE: 性能测试
 *               - TEST_CATEGORY_STRESS: 压力测试
 *               - TEST_CATEGORY_SYSTEM: 系统测试
 *   - tags: 测试标签（可以使用 | 组合多个标签）
 *           - TEST_TAG_FAST: 快速测试（< 100ms）
 *           - TEST_TAG_SLOW: 慢速测试（> 1s）
 *           - TEST_TAG_HARDWARE: 需要硬件支持
 *           - TEST_TAG_NETWORK: 需要网络连接
 *   - timeout: 超时时间（毫秒，0 表示无限制）
 *   - description: 测试套件描述
 *
 * 注意：
 *   由于 linker section 的实现复杂性，当前版本的 TEST_SUITE_AUTO_REGISTER
 *   会在 linker section 不可用时自动回退到空套件。为了完全支持自动收集，
 *   需要：
 *     1. 使用 TEST_CASE_AUTO(suite_id, test_name) 替代 TEST_CASE(test_name)
 *     2. 配置 linker script 定义 section 边界符号
 *
 *   当前示例使用传统方式（需要显式 TEST_CASE_REF），作为过渡方案。
 */
TEST_SUITE_AUTO_REGISTER(example_minimal,           /* suite_id */
                         "EXAMPLE",                  /* layer_name */
                         TEST_CATEGORY_UNIT,         /* category */
                         TEST_TAG_FAST,              /* tags */
                         100,                        /* timeout_ms */
                         "Minimal test example demonstrating simplified API");

/* ============================================================================
 * 简化版自动注册（使用默认配置）
 * ============================================================================
 *
 * 如果不需要自定义元数据，可以使用 TEST_SUITE_AUTO_REGISTER_SIMPLE：
 *
 *   TEST_SUITE_AUTO_REGISTER_SIMPLE(example_minimal, "EXAMPLE");
 *
 * 这将使用默认配置：
 *   - category: TEST_CATEGORY_UNIT
 *   - tags: TEST_TAG_FAST
 *   - timeout: 1000ms
 *   - description: 自动生成
 */

/* ============================================================================
 * 完全自动化版本（需要 linker section 支持）
 * ============================================================================
 *
 * 未来版本将支持完全自动化的测试用例收集：
 *
 *   TEST_CASE_AUTO(example_minimal, test_basic_assertions) { ... }
 *   TEST_CASE_AUTO(example_minimal, test_string_operations) { ... }
 *   ...
 *   TEST_SUITE_AUTO_REGISTER(example_minimal, "EXAMPLE", ...);
 *
 * 这将完全消除手动注册的需要。
 *
 * 实现需求：
 *   1. 修改 linker script (ld) 添加：
 *      .test_cases : {
 *          __start_test_cases_example_minimal = .;
 *          KEEP(*(.test_cases.example_minimal))
 *          __stop_test_cases_example_minimal = .;
 *      }
 *
 *   2. 或者在运行时使用 dlsym() 查找 section 边界符号
 *
 *   3. 或者使用构建脚本扫描源文件，自动生成注册代码
 */
