/**
 * @file test_log_refactor.c
 * @brief 测试重构后的 osal_log 日志系统
 *
 * 验证：
 * 1. 统一的日志函数 OSAL_LogEmit
 * 2. 编译时级别过滤
 * 3. 运行时级别过滤
 * 4. 日志宏正常工作
 * 5. _ONCE 宏防止重复日志
 */

#include "osal/osal.h"
#include <stdio.h>

int main(void)
{
    printf("=== OSAL Log Refactor Test ===\n\n");

    /* 初始化日志系统 */
    OSAL_LogInit(NULL, OS_LOG_LEVEL_DEBUG);

    /* 测试所有日志级别 */
    printf("1. Testing all log levels:\n");
    LOG_DEBUG("TEST", "This is a DEBUG message: %d", 1);
    LOG_INFO("TEST", "This is an INFO message: %d", 2);
    LOG_WARN("TEST", "This is a WARN message: %d", 3);
    LOG_ERROR("TEST", "This is an ERROR message: %d", 4);
    LOG_FATAL("TEST", "This is a FATAL message: %d", 5);

    /* 测试运行时级别过滤 */
    printf("\n2. Testing runtime level filtering (set to WARN):\n");
    OSAL_LogSetLevel(OS_LOG_LEVEL_WARN);
    LOG_DEBUG("TEST", "DEBUG should NOT appear");
    LOG_INFO("TEST", "INFO should NOT appear");
    LOG_WARN("TEST", "WARN should appear");
    LOG_ERROR("TEST", "ERROR should appear");

    /* 测试 _ONCE 宏 */
    printf("\n3. Testing LOG_ERROR_ONCE (should only print once):\n");
    for (int i = 0; i < 5; i++) {
        LOG_ERROR_ONCE("TEST", "This error should only appear once, iteration %d", i);
    }

    printf("\n4. Testing LOG_WARN_ONCE (should only print once):\n");
    for (int i = 0; i < 3; i++) {
        LOG_WARN_ONCE("TEST", "This warning should only appear once, iteration %d", i);
    }

    /* 恢复到 DEBUG 级别 */
    printf("\n5. Reset to DEBUG level:\n");
    OSAL_LogSetLevel(OS_LOG_LEVEL_DEBUG);
    LOG_DEBUG("TEST", "DEBUG is back!");
    LOG_INFO("TEST", "INFO is back!");

    /* 关闭日志系统 */
    OSAL_LogShutdown();

    printf("\n=== Test Complete ===\n");
    return 0;
}
