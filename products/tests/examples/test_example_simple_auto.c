/**
 * @file test_example_simple_auto.c
 * @brief 简化自动注册示例（无需 linker section，立即可用）
 *
 * 本文件演示如何使用简化的宏来减少样板代码，同时不依赖 linker section 特性。
 *
 * 相比传统方式的改进：
 *   传统方式（需要 ~20 行样板代码）：
 *     TEST_MODULE_BEGIN(test_osal_mutex, "OSAL")
 *     TEST_CASE_REGISTER(test_mutex_create, "")
 *     TEST_CASE_REGISTER(test_mutex_lock, "")
 *     ...
 *     TEST_SUITE_METADATA(test_osal_mutex, ...)
 *     TEST_MODULE_END(test_osal_mutex, "OSAL")
 *
 *   简化方式（只需 1 行）：
 *     TEST_CASE(test_mutex_create) { ... }
 *     TEST_CASE(test_mutex_lock) { ... }
 *     ...
 *     TEST_SUITE_REGISTER_CASES(suite_id, "LAYER", cases, metadata)
 *
 * 这个版本作为过渡方案，在 linker section 完全实现之前提供简化的体验。
 */

#include "test_framework.h"
#include "osal.h"

/* ============================================================================
 * 测试用例定义
 * ============================================================================ */

TEST_CASE(test_mutex_create)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(mutex);
    OSAL_Mutex_Delete(mutex);
}

TEST_CASE(test_mutex_lock_unlock)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(mutex);

    int32_t ret = OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    ret = OSAL_Mutex_Unlock(mutex);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    OSAL_Mutex_Delete(mutex);
}

TEST_CASE(test_mutex_timeout)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(mutex);

    /* 在另一个上下文中锁定 mutex，这里测试超时 */
    OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);

    /* 尝试获取已锁定的 mutex，应该超时 */
    int32_t ret = OSAL_Mutex_Lock(mutex, 100);  /* 100ms 超时 */
    TEST_ASSERT_NOT_EQUAL(OSAL_OK, ret);

    OSAL_Mutex_Unlock(mutex);
    OSAL_Mutex_Delete(mutex);
}

TEST_CASE(test_mutex_recursive)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(mutex);

    /* 测试递归锁定 */
    int32_t ret = OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    ret = OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    /* 需要相同次数的解锁 */
    OSAL_Mutex_Unlock(mutex);
    OSAL_Mutex_Unlock(mutex);

    OSAL_Mutex_Delete(mutex);
}

/* ============================================================================
 * 手动构建测试用例数组（过渡方案）
 * ============================================================================
 *
 * 在完全自动化之前，我们需要手动列出测试用例。
 * 但相比传统方式，这已经简化了很多（不需要 BEGIN/END 宏包围）。
 */

static const test_case_t example_simple_auto_cases[] = {
    { .name = "test_mutex_create",       .func = test_mutex_create,       .setup = NULL, .teardown = NULL },
    { .name = "test_mutex_lock_unlock",  .func = test_mutex_lock_unlock,  .setup = NULL, .teardown = NULL },
    { .name = "test_mutex_timeout",      .func = test_mutex_timeout,      .setup = NULL, .teardown = NULL },
    { .name = "test_mutex_recursive",    .func = test_mutex_recursive,    .setup = NULL, .teardown = NULL },
};

/* 定义元数据 */
static const test_metadata_t example_simple_auto_metadata = {
    .category = TEST_CATEGORY_UNIT,
    .tags = TEST_TAG_FAST,
    .timeout_ms = 500,
    .description = "Simplified auto-registration example (transition version)"
};

/* 注册测试套件 */
static const test_suite_t example_simple_auto_suite = {
    .suite_name = "example_simple_auto",
    .module_name = "example_simple_auto",
    .layer_name = "EXAMPLE",
    .cases = example_simple_auto_cases,
    .case_count = sizeof(example_simple_auto_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = example_simple_auto_metadata
};

__attribute__((constructor))
static void register_example_simple_auto(void)
{
    libutest_register_suite(&example_simple_auto_suite);
}

/* ============================================================================
 * 进一步简化的宏（可以添加到 test_framework.h）
 * ============================================================================
 *
 * 为了进一步减少样板代码，可以定义以下辅助宏：
 *
 * #define TEST_SUITE_REGISTER_CASES(suite_id, layer, cases_array, meta) \
 *     static const test_suite_t suite_id##_suite = { \
 *         .suite_name = #suite_id, \
 *         .module_name = #suite_id, \
 *         .layer_name = layer, \
 *         .cases = cases_array, \
 *         .case_count = sizeof(cases_array) / sizeof(test_case_t), \
 *         .suite_setup = NULL, \
 *         .suite_teardown = NULL, \
 *         .metadata = meta \
 *     }; \
 *     __attribute__((constructor)) \
 *     static void register_##suite_id(void) { \
 *         libutest_register_suite(&suite_id##_suite); \
 *     }
 *
 * 使用示例：
 *   TEST_SUITE_REGISTER_CASES(example_simple_auto, "EXAMPLE",
 *                             example_simple_auto_cases,
 *                             example_simple_auto_metadata);
 *
 * 这样只需要：
 *   1. 定义 TEST_CASE() 函数
 *   2. 手动列出测试用例数组
 *   3. 调用 TEST_SUITE_REGISTER_CASES() 一次
 */

/* ============================================================================
 * 使用建议
 * ============================================================================
 *
 * 1. 当前推荐方式（立即可用）：
 *    - 使用 TEST_CASE() 定义测试
 *    - 手动创建 test_case_t 数组
 *    - 使用 TEST_SUITE_REGISTER_CASES() 宏（需要添加到 test_framework.h）
 *
 * 2. 未来完全自动化版本（需要 linker section 支持）：
 *    - 使用 TEST_CASE_AUTO(suite_id, test_name) 定义测试
 *    - 调用 TEST_SUITE_AUTO_REGISTER() 自动收集所有测试
 *
 * 3. 迁移路径：
 *    - 第一步：将现有测试迁移到简化的宏系统（本文件演示的方式）
 *    - 第二步：配置 linker script 支持 section 边界符号
 *    - 第三步：迁移到完全自动化的 TEST_CASE_AUTO() 宏
 */
