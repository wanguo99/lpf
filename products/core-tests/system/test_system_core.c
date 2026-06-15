/**
 * @file test_system_core.c
 * @brief 系统测试框架核心实现（仅使用OSAL接口）
 */

#include <test/test_system.h>
#include <test/test_assert.h>
#include "osal.h"

#define MAX_CHECKPOINTS 100

/* 检查点记录 */
typedef struct {
    char name[64];
    bool passed;
    uint64_t timestamp_ms;
} checkpoint_record_t;

/* 系统测试上下文结构 */
struct system_test_context {
    char name[64];
    system_test_type_t type;
    system_test_env_t env;

    /* 环境管理函数 */
    system_env_setup_func_t setup_func;
    system_env_teardown_func_t teardown_func;

    /* 检查点记录 */
    checkpoint_record_t checkpoints[MAX_CHECKPOINTS];
    uint32_t checkpoint_count;
    uint32_t checkpoints_passed;
    uint32_t checkpoints_failed;

    /* 运行状态 */
    uint64_t start_time_ms;
    uint64_t end_time_ms;
    bool test_passed;
};

system_test_context_t* system_test_create(const char *name,
                                           system_test_type_t type) {
    if (!name) {
        return NULL;
    }

    system_test_context_t *ctx = (system_test_context_t*)OSAL_malloc(
        OSAL_sizeof(system_test_context_t));
    if (!ctx) {
        return NULL;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(system_test_context_t));
    OSAL_strncpy(ctx->name, name, OSAL_sizeof(ctx->name) - 1);
    ctx->name[OSAL_sizeof(ctx->name) - 1] = '\0';
    ctx->type = type;

    return ctx;
}

void system_test_destroy(system_test_context_t *ctx) {
    if (ctx) {
        OSAL_free(ctx);
    }
}

void system_test_set_env_funcs(system_test_context_t *ctx,
                                system_env_setup_func_t setup,
                                system_env_teardown_func_t teardown) {
    if (ctx) {
        ctx->setup_func = setup;
        ctx->teardown_func = teardown;
    }
}

int32_t system_test_run(system_test_context_t *ctx,
                        system_test_func_t test_func) {
    if (!ctx || !test_func) {
        return -1;
    }

    ctx->start_time_ms = OSAL_get_tick_count();

    /* 环境初始化 */
    if (ctx->setup_func) {
        OSAL_printf("[ SETUP    ] Initializing test environment...\n");
        int32_t result = ctx->setup_func(&ctx->env);
        if (result != 0) {
            OSAL_printf("[ SETUP FAIL ] Environment setup failed: %d\n", result);
            return result;
        }
        OSAL_printf("[ SETUP OK ] Environment initialized\n");
    }

    /* 运行测试 */
    OSAL_printf("[ RUN      ] %s\n", ctx->name);
    int32_t test_result = test_func(&ctx->env);

    /* 环境清理 */
    if (ctx->teardown_func) {
        OSAL_printf("[ TEARDOWN ] Cleaning up test environment...\n");
        ctx->teardown_func(&ctx->env);
        OSAL_printf("[ TEARDOWN OK ] Environment cleaned up\n");
    }

    ctx->end_time_ms = OSAL_get_tick_count();
    ctx->test_passed = (test_result == 0);

    /* 打印结果 */
    if (ctx->test_passed) {
        OSAL_printf("[ PASS     ] %s (%lu ms)\n",
                   ctx->name,
                   (unsigned long)(ctx->end_time_ms - ctx->start_time_ms));
    } else {
        OSAL_printf("[ FAIL     ] %s (%lu ms)\n",
                   ctx->name,
                   (unsigned long)(ctx->end_time_ms - ctx->start_time_ms));
    }

    return test_result;
}

system_test_env_t* system_test_get_env(system_test_context_t *ctx) {
    return ctx ? &ctx->env : NULL;
}

void system_test_checkpoint(system_test_context_t *ctx,
                            const char *checkpoint_name,
                            bool passed) {
    if (!ctx || !checkpoint_name || ctx->checkpoint_count >= MAX_CHECKPOINTS) {
        return;
    }

    checkpoint_record_t *cp = &ctx->checkpoints[ctx->checkpoint_count++];
    OSAL_strncpy(cp->name, checkpoint_name, OSAL_sizeof(cp->name) - 1);
    cp->name[OSAL_sizeof(cp->name) - 1] = '\0';
    cp->passed = passed;
    cp->timestamp_ms = OSAL_get_tick_count();

    if (passed) {
        ctx->checkpoints_passed++;
        OSAL_printf("[ CHECKPOINT OK ] %s\n", checkpoint_name);
    } else {
        ctx->checkpoints_failed++;
        OSAL_printf("[ CHECKPOINT FAIL ] %s\n", checkpoint_name);
    }
}

void system_test_print_report(system_test_context_t *ctx) {
    if (!ctx) return;

    const char *type_str = "";
    switch (ctx->type) {
        case SYSTEM_TEST_INTEGRATION:
            type_str = "Integration";
            break;
        case SYSTEM_TEST_E2E:
            type_str = "End-to-End";
            break;
        case SYSTEM_TEST_SCENARIO:
            type_str = "Scenario";
            break;
        case SYSTEM_TEST_REGRESSION:
            type_str = "Regression";
            break;
        default:
            type_str = "Unknown";
            break;
    }

    OSAL_printf("\n");
    OSAL_printf("=== System Test Report: %s ===\n", ctx->name);
    OSAL_printf("Type:        %s\n", type_str);
    OSAL_printf("Status:      %s\n", ctx->test_passed ? "PASSED" : "FAILED");
    OSAL_printf("Duration:    %lu ms\n",
               (unsigned long)(ctx->end_time_ms - ctx->start_time_ms));
    OSAL_printf("\n");
    OSAL_printf("Checkpoints:\n");
    OSAL_printf("  Total:     %u\n", ctx->checkpoint_count);
    OSAL_printf("  Passed:    %u\n", ctx->checkpoints_passed);
    OSAL_printf("  Failed:    %u\n", ctx->checkpoints_failed);
    OSAL_printf("\n");

    if (ctx->checkpoint_count > 0) {
        OSAL_printf("Checkpoint Details:\n");
        uint32_t i;

        for (i = 0; i < ctx->checkpoint_count; i++) {
            checkpoint_record_t *cp = &ctx->checkpoints[i];
            const char *status = cp->passed ? "+" : "X";
            OSAL_printf("  [%s] %s\n", status, cp->name);
        }
    }

    OSAL_printf("=====================================\n\n");
}
