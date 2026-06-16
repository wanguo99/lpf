/**
 * @file test_system.h
 * @brief 系统测试框架
 *
 * 提供系统级集成测试的基础设施：
 * - 多层集成测试（OSAL+HAL+PDL+PCONFIG+ACONFIG）
 * - 端到端测试（完整业务流程）
 * - 场景测试（真实使用场景模拟）
 * - 环境管理（测试环境搭建和清理）
 */

#ifndef TEST_SYSTEM_H
#define TEST_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

/* 注意：源文件需要包含 osal.h 以使用 OSAL API */

/* 系统测试类型 */
typedef enum {
    SYSTEM_TEST_INTEGRATION,    /* 集成测试 */
    SYSTEM_TEST_E2E,            /* 端到端测试 */
    SYSTEM_TEST_SCENARIO,       /* 场景测试 */
    SYSTEM_TEST_REGRESSION      /* 回归测试 */
} system_test_type_t;

/* 系统测试环境 */
typedef struct {
    bool osal_initialized;
    bool hal_initialized;
    bool pdl_initialized;
    bool pconfig_initialized;
    bool aconfig_initialized;
    void *user_data;
} system_test_env_t;

/* 系统测试上下文 */
typedef struct system_test_context system_test_context_t;

/* 环境初始化函数 */
typedef int32_t (*system_env_setup_func_t)(system_test_env_t *env);

/* 环境清理函数 */
typedef void (*system_env_teardown_func_t)(system_test_env_t *env);

/* 系统测试函数 */
typedef int32_t (*system_test_func_t)(system_test_env_t *env);

/**
 * 创建系统测试上下文
 * @param name 测试名称
 * @param type 测试类型
 * @return 系统测试上下文，失败返回NULL
 */
system_test_context_t* system_test_create(const char *name,
                                           system_test_type_t type);

/**
 * 销毁系统测试上下文
 * @param ctx 系统测试上下文
 */
void system_test_destroy(system_test_context_t *ctx);

/**
 * 设置环境初始化函数
 * @param ctx 系统测试上下文
 * @param setup 初始化函数
 * @param teardown 清理函数
 */
void system_test_set_env_funcs(system_test_context_t *ctx,
                                system_env_setup_func_t setup,
                                system_env_teardown_func_t teardown);

/**
 * 运行系统测试
 * @param ctx 系统测试上下文
 * @param test_func 测试函数
 * @return 0成功，负数失败
 */
int32_t system_test_run(system_test_context_t *ctx,
                        system_test_func_t test_func);

/**
 * 获取测试环境
 * @param ctx 系统测试上下文
 * @return 测试环境指针
 */
system_test_env_t* system_test_get_env(system_test_context_t *ctx);

/**
 * 记录系统测试检查点
 * @param ctx 系统测试上下文
 * @param checkpoint_name 检查点名称
 * @param passed 是否通过
 */
void system_test_checkpoint(system_test_context_t *ctx,
                            const char *checkpoint_name,
                            bool passed);

/**
 * 打印系统测试报告
 * @param ctx 系统测试上下文
 */
void system_test_print_report(system_test_context_t *ctx);

/* 便捷宏 */

/**
 * 系统测试用例定义
 * @param name 测试名称
 */
#define SYSTEM_TEST_CASE(name) \
    static int32_t system_test_##name(system_test_env_t *env)

/**
 * 系统测试环境初始化
 * @param name 环境名称
 */
#define SYSTEM_ENV_SETUP(name) \
    static int32_t system_env_setup_##name(system_test_env_t *env)

/**
 * 系统测试环境清理
 * @param name 环境名称
 */
#define SYSTEM_ENV_TEARDOWN(name) \
    static void system_env_teardown_##name(system_test_env_t *env)

/**
 * 系统测试检查点
 * @param ctx 系统测试上下文
 * @param name 检查点名称
 * @param condition 条件表达式
 */
#define SYSTEM_CHECKPOINT(ctx, name, condition) \
    do { \
        bool _passed = (condition); \
        system_test_checkpoint(ctx, name, _passed); \
        if (!_passed) { \
            OSAL_printf("[ CHECKPOINT FAIL ] %s: %s\n", name, #condition); \
        } \
    } while (0)

/**
 * 系统测试断言：检查点必须全部通过
 * @param ctx 系统测试上下文
 */
#define SYSTEM_ASSERT_ALL_CHECKPOINTS_PASSED(ctx) \
    do { \
        /* 实现由框架提供 */ \
    } while (0)

#endif /* TEST_SYSTEM_H */
