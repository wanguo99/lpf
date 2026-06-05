/**
 * @file test_example_simplified.c
 * @brief 使用最简化宏的测试示例（立即可用，推荐方式）
 *
 * 本文件展示如何使用新增的简化宏来编写测试，相比传统方式显著减少样板代码。
 *
 * 代码对比：
 *
 * 【传统方式】需要 ~25 行样板代码：
 *   TEST_MODULE_BEGIN(test_osal_thread, "OSAL")
 *   TEST_CASE_REGISTER(test_thread_create, "")
 *   TEST_CASE_REGISTER(test_thread_join, "")
 *   TEST_CASE_REGISTER(test_thread_detach, "")
 *   TEST_SUITE_METADATA(test_osal_thread, TEST_CATEGORY_UNIT, TEST_TAG_FAST, 200, "OSAL thread tests")
 *   TEST_MODULE_END(test_osal_thread, "OSAL")
 *
 * 【简化方式】只需 ~10 行：
 *   TEST_CASE(test_thread_create) { ... }
 *   TEST_CASE(test_thread_join) { ... }
 *   TEST_CASE(test_thread_detach) { ... }
 *
 *   static const test_case_t osal_thread_cases[] = {
 *       TEST_CASE_ENTRY(test_thread_create),
 *       TEST_CASE_ENTRY(test_thread_join),
 *       TEST_CASE_ENTRY(test_thread_detach),
 *   };
 *
 *   TEST_SUITE_REGISTER(osal_thread, "OSAL", osal_thread_cases,
 *                       TEST_CATEGORY_UNIT, TEST_TAG_FAST, 200, "OSAL thread tests");
 *
 * 优势：
 *   ✓ 减少 60% 的样板代码
 *   ✓ 测试用例数组一目了然
 *   ✓ 添加/删除测试用例只需修改数组
 *   ✓ 元数据集中定义，易于维护
 *   ✓ 无需 linker section，立即可用
 */

#include "test_framework.h"
#include "osal.h"

/* ============================================================================
 * 线程测试用例
 * ============================================================================ */

/* 全局变量：用于线程间通信 */
static volatile int32_t thread_counter = 0;
static OSAL_Mutex_Handle_t test_mutex = NULL;

/* 线程入口函数 */
static void* thread_entry_increment(void *arg)
{
    int32_t iterations = *(int32_t*)arg;

    for (int32_t i = 0; i < iterations; i++) {
        OSAL_Mutex_Lock(test_mutex, OSAL_WAIT_FOREVER);
        thread_counter++;
        OSAL_Mutex_Unlock(test_mutex);
    }

    return NULL;
}

TEST_CASE(test_thread_create)
{
    OSAL_Thread_Config_t config = {
        .name = "test_thread",
        .entry = thread_entry_increment,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_THREAD_PRIORITY_NORMAL
    };

    OSAL_Thread_Handle_t thread = OSAL_Thread_Create(&config);
    TEST_ASSERT_NOT_NULL(thread);

    OSAL_Thread_Delete(thread);
}

TEST_CASE(test_thread_join)
{
    thread_counter = 0;
    test_mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(test_mutex);

    int32_t iterations = 1000;
    OSAL_Thread_Config_t config = {
        .name = "test_thread_join",
        .entry = thread_entry_increment,
        .arg = &iterations,
        .stack_size = 4096,
        .priority = OSAL_THREAD_PRIORITY_NORMAL
    };

    OSAL_Thread_Handle_t thread = OSAL_Thread_Create(&config);
    TEST_ASSERT_NOT_NULL(thread);

    /* 等待线程结束 */
    int32_t ret = OSAL_Thread_Join(thread, NULL);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    /* 验证计数器 */
    TEST_ASSERT_EQUAL(1000, thread_counter);

    OSAL_Mutex_Delete(test_mutex);
}

TEST_CASE(test_thread_detach)
{
    OSAL_Thread_Config_t config = {
        .name = "test_thread_detach",
        .entry = thread_entry_increment,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_THREAD_PRIORITY_NORMAL
    };

    OSAL_Thread_Handle_t thread = OSAL_Thread_Create(&config);
    TEST_ASSERT_NOT_NULL(thread);

    /* 分离线程 */
    int32_t ret = OSAL_Thread_Detach(thread);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    /* 分离后不能再 join */
    ret = OSAL_Thread_Join(thread, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_OK, ret);
}

TEST_CASE(test_thread_multiple)
{
    thread_counter = 0;
    test_mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(test_mutex);

    const int32_t thread_count = 5;
    const int32_t iterations = 200;
    OSAL_Thread_Handle_t threads[thread_count];

    /* 创建多个线程 */
    for (int32_t i = 0; i < thread_count; i++) {
        OSAL_Thread_Config_t config = {
            .name = "test_thread_multi",
            .entry = thread_entry_increment,
            .arg = (void*)&iterations,
            .stack_size = 4096,
            .priority = OSAL_THREAD_PRIORITY_NORMAL
        };
        threads[i] = OSAL_Thread_Create(&config);
        TEST_ASSERT_NOT_NULL(threads[i]);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < thread_count; i++) {
        OSAL_Thread_Join(threads[i], NULL);
    }

    /* 验证计数器（5个线程，每个200次） */
    TEST_ASSERT_EQUAL(thread_count * iterations, thread_counter);

    OSAL_Mutex_Delete(test_mutex);
}

/* ============================================================================
 * 测试用例注册（简化方式）
 * ============================================================================ */

/**
 * 步骤 1：创建测试用例数组
 *
 * 使用 TEST_CASE_ENTRY() 宏可以自动填充 name, func, setup, teardown 字段。
 * 只需列出测试函数名即可。
 */
static const test_case_t example_simplified_cases[] = {
    TEST_CASE_ENTRY(test_thread_create),
    TEST_CASE_ENTRY(test_thread_join),
    TEST_CASE_ENTRY(test_thread_detach),
    TEST_CASE_ENTRY(test_thread_multiple),
};

/**
 * 步骤 2：一行注册测试套件
 *
 * TEST_SUITE_REGISTER() 宏会自动：
 *   - 创建 test_metadata_t 结构
 *   - 创建 test_suite_t 结构
 *   - 注册到测试框架（使用 constructor 属性）
 *
 * 参数：
 *   - suite_id: 套件标识符（建议使用 layer_module 格式，如 osal_thread）
 *   - layer_name: 所属层级（"OSAL", "HAL", "PDL" 等）
 *   - cases_array: 测试用例数组
 *   - category: 测试分类
 *   - tags: 测试标签
 *   - timeout: 超时时间（毫秒）
 *   - description: 描述字符串
 */
TEST_SUITE_REGISTER(example_simplified, "EXAMPLE", example_simplified_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 500,
                    "Simplified test registration example");

/* ============================================================================
 * 备选方案：使用预定义元数据模板
 * ============================================================================
 *
 * 对于标准的单元测试，可以使用预定义的元数据宏：
 *
 *   static const test_metadata_t my_metadata = TEST_METADATA_DEFAULT_UNIT("My tests");
 *   TEST_SUITE_REGISTER_CASES(my_suite, "OSAL", my_cases, my_metadata);
 *
 * 可用的元数据模板：
 *   - TEST_METADATA_DEFAULT_UNIT(desc)      - 单元测试（快速，1s 超时）
 *   - TEST_METADATA_DEFAULT_PERF(desc)      - 性能测试（慢速，5s 超时）
 *   - TEST_METADATA_DEFAULT_STRESS(desc)    - 压力测试（慢速，10s 超时）
 *   - TEST_METADATA_DEFAULT_SYSTEM(desc)    - 系统测试（慢速，5s 超时）
 */

/* ============================================================================
 * 迁移指南
 * ============================================================================
 *
 * 将现有测试从传统方式迁移到简化方式的步骤：
 *
 * 1. 保持 TEST_CASE() 定义不变
 *
 * 2. 删除 TEST_MODULE_BEGIN/END 及其中的 TEST_CASE_REGISTER 调用
 *
 * 3. 创建测试用例数组：
 *    static const test_case_t my_cases[] = {
 *        TEST_CASE_ENTRY(test_1),
 *        TEST_CASE_ENTRY(test_2),
 *        ...
 *    };
 *
 * 4. 将原来的 TEST_SUITE_METADATA() 参数移到 TEST_SUITE_REGISTER() 中：
 *    TEST_SUITE_REGISTER(my_suite, "LAYER", my_cases,
 *                        category, tags, timeout, description);
 *
 * 5. 编译验证
 *
 * 示例对比：
 *
 * 【迁移前】
 *   TEST_MODULE_BEGIN(test_osal_mutex, "OSAL")
 *   TEST_CASE_REGISTER(test_mutex_create, "")
 *   TEST_CASE_REGISTER(test_mutex_lock, "")
 *   TEST_SUITE_METADATA(test_osal_mutex, TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "Mutex tests")
 *   TEST_MODULE_END(test_osal_mutex, "OSAL")
 *
 * 【迁移后】
 *   static const test_case_t osal_mutex_cases[] = {
 *       TEST_CASE_ENTRY(test_mutex_create),
 *       TEST_CASE_ENTRY(test_mutex_lock),
 *   };
 *   TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases,
 *                       TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "Mutex tests");
 */
