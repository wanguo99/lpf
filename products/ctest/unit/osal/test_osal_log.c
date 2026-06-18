#include <test_framework/test_framework.h>
/**
 * @file test_osal_log.c
 * @brief OSAL日志系统单元测试
 */

#include "osal.h"

/* 测试日志文件路径 */
#define TEST_LOG_FILE "/tmp/osal_test.log"

/*===========================================================================
 * 日志初始化测试
 *===========================================================================*/

/* 测试用例: 日志初始化 - 成功 */
static void test_osal_log_init_success(void)
{
    int32_t ret;

    /* 初始化日志系统 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置文件大小和备份数 */
    OSAL_log_set_max_file_size(1024 * 1024); /* 1MB */
    OSAL_log_set_max_files(3);

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/* 测试用例: 日志初始化 - 空路径（仅终端输出） */
static void test_osal_log_init_null_path(void)
{
    int32_t ret;

    /* NULL路径表示只输出到终端 */
    ret = OSAL_log_init(NULL, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 清理 */
    OSAL_log_shutdown();
}

/* 测试用例: 日志初始化 - 不同级别 */
static void test_osal_log_init_different_levels(void)
{
    int32_t ret;

    /* DEBUG级别 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_log_shutdown();

    /* INFO级别 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_log_shutdown();

    /* ERROR级别 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_log_shutdown();

    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 日志写入测试
 *===========================================================================*/

/* 测试用例: 日志写入 - 基本功能 */
static void test_osal_log_write_basic(void)
{
    int32_t ret;

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 写入不同级别的日志 */
    LOG_DEBUG("TEST", "This is a debug message");
    LOG_INFO("TEST", "This is an info message");
    LOG_WARN("TEST", "This is a warning message");
    LOG_ERROR("TEST", "This is an error message");
    LOG_FATAL("TEST", "This is a fatal message");

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/* 测试用例: 日志写入 - 格式化 */
static void test_osal_log_write_formatted(void)
{
    int32_t ret;

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 写入格式化日志 */
    LOG_INFO("TEST", "Integer: %d, String: %s, Hex: 0x%X", 42, "Hello", 0xFF);
    LOG_ERROR("TEST", "Error code: %d, Message: %s", -1, "Failed");

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/* 测试用例: 日志写入 - 长消息 */
static void test_osal_log_write_long_message(void)
{
    int32_t ret;
    char long_msg[512];

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 构造长消息 */
    OSAL_memset(long_msg, 'A', OSAL_sizeof(long_msg) - 1);
    long_msg[OSAL_sizeof(long_msg) - 1] = '\0';

    /* 写入长消息 */
    LOG_INFO("TEST", "%s", long_msg);

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 日志级别测试
 *===========================================================================*/

/* 测试用例: 设置日志级别 */
static void test_osal_log_set_level(void)
{
    int32_t ret;

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置为ERROR级别 */
    OSAL_log_set_level(OS_LOG_LEVEL_ERROR);

    /* DEBUG和INFO不应该输出 */
    LOG_DEBUG("TEST", "This should not appear");
    LOG_INFO("TEST", "This should not appear");

    /* ERROR应该输出 */
    LOG_ERROR("TEST", "This should appear");

    /* 设置为DEBUG级别 */
    OSAL_log_set_level(OS_LOG_LEVEL_DEBUG);

    /* 所有级别都应该输出 */
    LOG_DEBUG("TEST", "Debug message");
    LOG_INFO("TEST", "Info message");
    LOG_ERROR("TEST", "Error message");

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 日志轮转测试
 *===========================================================================*/

/* 测试用例: 日志轮转 - 基本功能 */
static void test_osal_log_rotation_basic(void)
{
    int32_t ret;

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置小文件大小，便于触发轮转 */
    OSAL_log_set_max_file_size(1024); /* 1KB */
    OSAL_log_set_max_files(3);

    /* 写入大量日志，触发轮转 */
    int32_t i;

    for (i = 0; i < 100; i++) {
        LOG_INFO("TEST",
                 "Log message %d: This is a test message to trigger rotation",
                 i);
    }

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
    OSAL_unlink("/tmp/osal_test.log.1");
    OSAL_unlink("/tmp/osal_test.log.2");
}

/*===========================================================================
 * 日志刷新测试
 *===========================================================================*/

/* 测试用例: Printf - 简单打印 */
static void test_osal_printf(void)
{
    int32_t ret;

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 使用Printf（不带日志级别） */
    OSAL_printf("Simple message\n");
    OSAL_printf("Formatted: %d, %s\n", 42, "test");

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 并发测试
 *===========================================================================*/

/* 测试任务函数 */
static volatile bool g_log_test_running = true;

static void *log_test_task(void *arg)
{
    int32_t task_id = *(int32_t *)arg;

    int32_t i;

    for (i = 0; i < 10 && g_log_test_running; i++) {
        LOG_INFO("TEST", "Task %d: Message %d", task_id, i);
        OSAL_msleep(10);
    }
    return NULL;
}

/* 测试用例: 多线程日志写入 */
static void test_osal_log_multithread(void)
{
    int32_t ret;
    osal_thread_t task_ids[3];
    int32_t task_args[3] = { 1, 2, 3 };

    /* 初始化日志 */
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    g_log_test_running = true;

    /* 创建多个任务 */
    int32_t i;

    for (i = 0; i < 3; i++) {
        ret = OSAL_pthread_create(&task_ids[i],
                                  NULL,
                                  log_test_task,
                                  &task_args[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待任务完成 */
    OSAL_msleep(500);

    /* 等待线程退出 */

    for (i = 0; i < 3; i++) {
        OSAL_pthread_join(task_ids[i], NULL);
    }

    /* 清理 */
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: 未初始化时写入日志 */
static void test_osal_log_write_without_init(void)
{
    /* 跳过此测试：当前OSAL日志实现在未初始化时调用LOG_INFO会导致段错误
     * 原因：log_internal_ex函数使用pthread_mutex_lock但没有检查初始化状态
     * TODO: 需要在OSAL日志实现中添加初始化状态检查
     */
    TEST_ASSERT_FALSE(
        true); // OSAL log implementation needs initialization check
}

/* 测试用例: 重复初始化 */
static void test_osal_log_init_twice(void)
{
    /* 跳过此测试：重复调用OSAL_LogInit会导致文件描述符泄漏和状态损坏
     * 原因：OSAL_LogInit不检查是否已初始化，直接打开新文件而不关闭旧文件
     * TODO: 需要在OSAL_LogInit中添加已初始化检查，或先调用shutdown
     */
    TEST_ASSERT_FALSE(true); // Multiple init causes file descriptor leak
}

/* 测试用例: 重复清理 */
static void test_osal_log_shutdown_twice(void)
{
    /* 跳过此测试：重复调用OSAL_LogShutdown后，后续测试可能因状态不一致导致段错误
     * 原因：OSAL_LogShutdown只关闭文件但不清理全局状态，可能影响后续测试
     * TODO: 需要在OSAL日志实现中添加完整的状态重置逻辑
     */
    TEST_ASSERT_FALSE(
        true); // Multiple shutdown may corrupt log state for subsequent tests
}

/*===========================================================================
 * 性能测试
 *===========================================================================*/

/* 测试用例: 日志写入性能 */
static void test_osal_log_performance(void)
{
    int32_t ret;
    uint64_t start_time, end_time;
    const int32_t iterations = 1000;

    // 初始化日志
    ret = OSAL_log_init(TEST_LOG_FILE, OS_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_log_set_max_file_size(10 * 1024 * 1024); // 10MB

    // 测试写入性能
    start_time = OSAL_get_tick_count();
    int32_t i;

    for (i = 0; i < iterations; i++) {
        LOG_INFO("TEST", "Performance test message %d", i);
    }
    end_time = OSAL_get_tick_count();

    // 平均每条日志应该小于1ms
    uint64_t elapsed = end_time - start_time;
    TEST_ASSERT_TRUE(elapsed < (uint64_t)iterations);

    // 清理
    OSAL_log_shutdown();
    OSAL_unlink(TEST_LOG_FILE);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// OSAL日志系统测试
/* 日志初始化 */
/* 日志写入 */
/* 日志级别 */
/* 日志轮转 */
/* Printf */
/* 并发测试 */
/* 边界条件 */
/* 性能测试 */
// /* 已屏蔽 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
    { .name = "test_osal_log_init_success",
      .func = test_osal_log_init_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_init_null_path",
      .func = test_osal_log_init_null_path,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_init_different_levels",
      .func = test_osal_log_init_different_levels,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_write_basic",
      .func = test_osal_log_write_basic,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_write_formatted",
      .func = test_osal_log_write_formatted,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_write_long_message",
      .func = test_osal_log_write_long_message,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_set_level",
      .func = test_osal_log_set_level,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_rotation_basic",
      .func = test_osal_log_rotation_basic,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_printf",
      .func = test_osal_printf,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_multithread",
      .func = test_osal_log_multithread,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_write_without_init",
      .func = test_osal_log_write_without_init,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_init_twice",
      .func = test_osal_log_init_twice,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_shutdown_twice",
      .func = test_osal_log_shutdown_twice,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_log_performance",
      .func = test_osal_log_performance,
      .setup = NULL,
      .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
    .suite_name = "osal_log",
    .module_name = "osal_log",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = { .category = TEST_CATEGORY_UNIT,
                  .tags = TEST_TAG_FAST,
                  .timeout_ms = 100,
                  .description = "OSAL osal_log tests" }
};

/* 测试套件注册函数 */
__attribute__((constructor)) static void register_osal_log_tests(void)
{
    libutest_register_suite(&test_suite);
}
