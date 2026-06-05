/**
 * @file test_example_with_fixture.c
 * @brief 带 fixture（setup/teardown）的简化测试示例
 *
 * 本文件展示如何在简化的测试框架中使用 fixture（测试夹具）。
 *
 * Fixture 的作用：
 *   - Setup: 在每个测试用例运行前执行，用于初始化测试环境
 *   - Teardown: 在每个测试用例运行后执行，用于清理资源
 *
 * 使用场景：
 *   ✓ 需要初始化硬件设备
 *   ✓ 需要分配/释放资源（内存、文件句柄等）
 *   ✓ 需要设置测试环境（配置、状态等）
 *   ✓ 确保每个测试用例在干净的环境中运行
 */

#include "test_framework.h"
#include "osal.h"

/* ============================================================================
 * 测试 Fixture 数据
 * ============================================================================ */

/**
 * 测试上下文数据结构
 *
 * 用于在 setup/teardown 和测试用例之间共享数据。
 */
typedef struct {
    OSAL_Mutex_Handle_t mutex;
    OSAL_Thread_Handle_t thread;
    uint8_t *buffer;
    size_t buffer_size;
    volatile int32_t counter;
} test_fixture_t;

/* 全局测试上下文 */
static test_fixture_t g_fixture;

/* ============================================================================
 * Fixture 函数定义
 * ============================================================================ */

/**
 * Setup 函数：在每个测试用例运行前执行
 *
 * 初始化测试所需的资源和环境。
 */
static void fixture_setup(void)
{
    /* 初始化测试数据 */
    OSAL_Memset(&g_fixture, 0, sizeof(test_fixture_t));

    /* 创建 mutex */
    g_fixture.mutex = OSAL_Mutex_Create();
    if (g_fixture.mutex == NULL) {
        TEST_LOG_ERROR("Failed to create mutex in setup");
        return;
    }

    /* 分配缓冲区 */
    g_fixture.buffer_size = 1024;
    g_fixture.buffer = (uint8_t *)OSAL_Malloc(g_fixture.buffer_size);
    if (g_fixture.buffer == NULL) {
        TEST_LOG_ERROR("Failed to allocate buffer in setup");
        OSAL_Mutex_Delete(g_fixture.mutex);
        return;
    }

    /* 初始化缓冲区 */
    OSAL_Memset(g_fixture.buffer, 0, g_fixture.buffer_size);

    /* 重置计数器 */
    g_fixture.counter = 0;

    TEST_LOG_INFO("Fixture setup completed");
}

/**
 * Teardown 函数：在每个测试用例运行后执行
 *
 * 清理测试分配的资源，确保不影响后续测试。
 */
static void fixture_teardown(void)
{
    /* 清理线程（如果创建了） */
    if (g_fixture.thread != NULL) {
        OSAL_Thread_Delete(g_fixture.thread);
        g_fixture.thread = NULL;
    }

    /* 清理 mutex */
    if (g_fixture.mutex != NULL) {
        OSAL_Mutex_Delete(g_fixture.mutex);
        g_fixture.mutex = NULL;
    }

    /* 释放缓冲区 */
    if (g_fixture.buffer != NULL) {
        OSAL_Free(g_fixture.buffer);
        g_fixture.buffer = NULL;
    }

    TEST_LOG_INFO("Fixture teardown completed");
}

/* ============================================================================
 * 测试用例（使用 fixture）
 * ============================================================================ */

TEST_CASE(test_mutex_with_fixture)
{
    /* fixture_setup() 已经创建了 mutex */
    TEST_ASSERT_NOT_NULL(g_fixture.mutex);

    /* 测试 mutex 操作 */
    int32_t ret = OSAL_Mutex_Lock(g_fixture.mutex, OSAL_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    ret = OSAL_Mutex_Unlock(g_fixture.mutex);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);

    /* teardown 会自动清理 mutex */
}

TEST_CASE(test_buffer_operations)
{
    /* fixture_setup() 已经分配了 buffer */
    TEST_ASSERT_NOT_NULL(g_fixture.buffer);
    TEST_ASSERT_EQUAL(1024, g_fixture.buffer_size);

    /* 写入数据 */
    for (size_t i = 0; i < g_fixture.buffer_size; i++) {
        g_fixture.buffer[i] = (uint8_t)(i & 0xFF);
    }

    /* 验证数据 */
    for (size_t i = 0; i < g_fixture.buffer_size; i++) {
        TEST_ASSERT_EQUAL((uint8_t)(i & 0xFF), g_fixture.buffer[i]);
    }

    /* teardown 会自动释放 buffer */
}

/* 线程入口函数：用于测试 */
static void* thread_entry_counter(void *arg)
{
    (void)arg;

    for (int i = 0; i < 100; i++) {
        OSAL_Mutex_Lock(g_fixture.mutex, OSAL_WAIT_FOREVER);
        g_fixture.counter++;
        OSAL_Mutex_Unlock(g_fixture.mutex);
        OSAL_Sleep(1);  /* 模拟工作 */
    }

    return NULL;
}

TEST_CASE(test_thread_with_fixture)
{
    /* 使用 fixture 提供的 mutex 和 counter */
    TEST_ASSERT_NOT_NULL(g_fixture.mutex);
    TEST_ASSERT_EQUAL(0, g_fixture.counter);

    /* 创建线程 */
    OSAL_Thread_Config_t config = {
        .name = "test_thread",
        .entry = thread_entry_counter,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_THREAD_PRIORITY_NORMAL
    };

    g_fixture.thread = OSAL_Thread_Create(&config);
    TEST_ASSERT_NOT_NULL(g_fixture.thread);

    /* 等待线程完成 */
    OSAL_Thread_Join(g_fixture.thread, NULL);

    /* 验证计数器 */
    TEST_ASSERT_EQUAL(100, g_fixture.counter);

    /* teardown 会清理线程（如果还在运行） */
    g_fixture.thread = NULL;  /* 已经 join，不需要 delete */
}

TEST_CASE(test_multiple_resources)
{
    /* 所有资源都已准备好 */
    TEST_ASSERT_NOT_NULL(g_fixture.mutex);
    TEST_ASSERT_NOT_NULL(g_fixture.buffer);
    TEST_ASSERT_EQUAL(1024, g_fixture.buffer_size);
    TEST_ASSERT_EQUAL(0, g_fixture.counter);

    /* 使用所有资源进行测试 */
    OSAL_Mutex_Lock(g_fixture.mutex, OSAL_WAIT_FOREVER);
    OSAL_Memset(g_fixture.buffer, 0xAA, g_fixture.buffer_size);
    g_fixture.counter = 42;
    OSAL_Mutex_Unlock(g_fixture.mutex);

    /* 验证 */
    TEST_ASSERT_EQUAL(0xAA, g_fixture.buffer[0]);
    TEST_ASSERT_EQUAL(0xAA, g_fixture.buffer[1023]);
    TEST_ASSERT_EQUAL(42, g_fixture.counter);
}

/* ============================================================================
 * 测试用例注册（带 fixture）
 * ============================================================================ */

/**
 * 使用 TEST_CASE_ENTRY_WITH_FIXTURE() 宏注册带 fixture 的测试用例
 *
 * 每个测试用例都会关联 setup 和 teardown 函数。
 * 测试框架会在运行每个测试前后自动调用这些函数。
 */
static const test_case_t example_fixture_cases[] = {
    TEST_CASE_ENTRY_WITH_FIXTURE(test_mutex_with_fixture, fixture_setup, fixture_teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_buffer_operations, fixture_setup, fixture_teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_thread_with_fixture, fixture_setup, fixture_teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_multiple_resources, fixture_setup, fixture_teardown),
};

/* 注册测试套件 */
TEST_SUITE_REGISTER(example_fixture, "EXAMPLE", example_fixture_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 1000,
                    "Test fixture (setup/teardown) example");

/* ============================================================================
 * Suite-level Fixture（套件级 fixture）
 * ============================================================================
 *
 * 除了 case-level fixture（每个测试用例前后执行），还可以使用 suite-level fixture：
 *   - suite_setup: 在整个测试套件运行前执行一次
 *   - suite_teardown: 在整个测试套件运行后执行一次
 *
 * 适用场景：
 *   - 初始化昂贵的资源（数据库连接、硬件初始化等）
 *   - 在多个测试用例之间共享资源
 *   - 减少重复的初始化开销
 *
 * 使用方法：
 *   需要使用底层的 TEST_SUITE_END_WITH_FIXTURE() 宏，或者扩展
 *   TEST_SUITE_REGISTER() 宏来支持 suite-level fixture。
 *
 * 示例（需要手动构建 test_suite_t）：
 *
 *   static void suite_setup(void) {
 *       // 初始化共享资源
 *   }
 *
 *   static void suite_teardown(void) {
 *       // 清理共享资源
 *   }
 *
 *   static const test_suite_t my_suite = {
 *       .suite_name = "my_suite",
 *       .module_name = "my_suite",
 *       .layer_name = "OSAL",
 *       .cases = my_cases,
 *       .case_count = sizeof(my_cases) / sizeof(test_case_t),
 *       .suite_setup = suite_setup,
 *       .suite_teardown = suite_teardown,
 *       .metadata = my_metadata
 *   };
 *
 *   __attribute__((constructor))
 *   static void register_my_suite(void) {
 *       libutest_register_suite(&my_suite);
 *   }
 */

/* ============================================================================
 * Fixture 最佳实践
 * ============================================================================
 *
 * 1. **保持独立性**
 *    - 每个测试用例应该独立运行
 *    - 不要依赖其他测试的执行顺序或状态
 *    - Setup 应该创建完全独立的环境
 *
 * 2. **确保清理**
 *    - Teardown 必须清理所有分配的资源
 *    - 即使测试失败，也要保证资源被释放
 *    - 避免资源泄漏影响后续测试
 *
 * 3. **错误处理**
 *    - Setup 失败应该跳过测试用例
 *    - Teardown 应该容忍 NULL 指针（资源可能未创建）
 *    - 记录详细的错误信息便于调试
 *
 * 4. **性能考虑**
 *    - 如果 setup/teardown 开销大，考虑使用 suite-level fixture
 *    - 在性能测试中，setup/teardown 时间不应计入测试时间
 *
 * 5. **代码复用**
 *    - 多个测试套件可以共享相同的 fixture 函数
 *    - 将通用的 fixture 逻辑抽取为辅助函数
 */
