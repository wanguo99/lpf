/**
 * @file test_stress_core.c
 * @brief 压力测试框架核心实现（仅使用OSAL接口）
 */

#include "test_stress.h"
#include "test_assert.h"
#include "osal.h"

#define MAX_ERROR_MESSAGES 100

/* 压力测试上下文结构 */
struct stress_context {
    char name[64];
    stress_config_t config;

    /* 运行状态 */
    bool running;
    bool should_stop;
    uint64_t start_time_ms;

    /* 统计数据 */
    osal_atomic_uint64_t total_operations;
    osal_atomic_uint64_t successful_ops;
    osal_atomic_uint64_t failed_ops;
    osal_atomic_uint64_t timeout_ops;
    osal_atomic_uint32_t error_count;

    /* 延迟统计 */
    double total_latency_us;
    double max_latency_us;
    osal_mutex_t *stats_mutex;

    /* 错误记录 */
    char error_messages[MAX_ERROR_MESSAGES][128];
    uint32_t error_msg_count;
    osal_mutex_t *error_mutex;
};

/* 工作线程参数 */
typedef struct {
    stress_context_t *ctx;
    stress_worker_func_t worker;
    void *user_data;
    uint32_t thread_id;
} worker_thread_args_t;

/**
 * 工作线程函数
 */
static void* worker_thread_func(void *arg) {
    worker_thread_args_t *args = (worker_thread_args_t*)arg;
    stress_context_t *ctx = args->ctx;
    uint32_t iteration = 0;

    while (!stress_should_stop(ctx)) {
        uint64_t start_us = OSAL_GetMonotonicTime();

        /* 执行工作函数 */
        int32_t result = args->worker(args->user_data, iteration);

        uint64_t end_us = OSAL_GetMonotonicTime();
        double latency_us = (double)(end_us - start_us);

        /* 更新统计 */
        OSAL_AtomicInc64(&ctx->total_operations);

        if (result == 0) {
            OSAL_AtomicInc64(&ctx->successful_ops);
        } else if (result == -OSAL_ERR_TIMEOUT) {
            OSAL_AtomicInc64(&ctx->timeout_ops);
        } else {
            OSAL_AtomicInc64(&ctx->failed_ops);
            if (ctx->config.stop_on_error) {
                ctx->should_stop = true;
            }
        }

        /* 更新延迟统计 */
        OSAL_MutexLock(ctx->stats_mutex);
        ctx->total_latency_us += latency_us;
        if (latency_us > ctx->max_latency_us) {
            ctx->max_latency_us = latency_us;
        }
        OSAL_MutexUnlock(ctx->stats_mutex);

        iteration++;

        /* 检查迭代次数限制 */
        if (ctx->config.iterations > 0 && iteration >= ctx->config.iterations) {
            break;
        }
    }

    return NULL;
}

stress_context_t* stress_context_create(const char *name,
                                         const stress_config_t *config) {
    if (!name || !config) {
        return NULL;
    }

    stress_context_t *ctx = (stress_context_t*)OSAL_Malloc(sizeof(stress_context_t));
    if (!ctx) {
        return NULL;
    }

    OSAL_memset(ctx, 0, sizeof(stress_context_t));
    OSAL_strncpy(ctx->name, name, sizeof(ctx->name) - 1);
    ctx->name[sizeof(ctx->name) - 1] = '\0';
    ctx->config = *config;

    /* 初始化原子变量 */
    OSAL_AtomicInit64(&ctx->total_operations, 0);
    OSAL_AtomicInit64(&ctx->successful_ops, 0);
    OSAL_AtomicInit64(&ctx->failed_ops, 0);
    OSAL_AtomicInit64(&ctx->timeout_ops, 0);
    OSAL_AtomicInit32(&ctx->error_count, 0);

    /* 创建互斥锁 */
    if (OSAL_MutexCreate(&ctx->stats_mutex) != 0) {
        OSAL_Free(ctx);
        return NULL;
    }

    if (OSAL_MutexCreate(&ctx->error_mutex) != 0) {
        OSAL_MutexDestroy(ctx->stats_mutex);
        OSAL_Free(ctx);
        return NULL;
    }

    return ctx;
}

void stress_context_destroy(stress_context_t *ctx) {
    if (ctx) {
        if (ctx->stats_mutex) {
            OSAL_MutexDestroy(ctx->stats_mutex);
        }
        if (ctx->error_mutex) {
            OSAL_MutexDestroy(ctx->error_mutex);
        }
        OSAL_Free(ctx);
    }
}

int32_t stress_run(stress_context_t *ctx,
                   stress_worker_func_t worker,
                   void *user_data) {
    if (!ctx || !worker) {
        return -1;
    }

    ctx->running = true;
    ctx->should_stop = false;
    ctx->start_time_ms = OSAL_GetTime();

    /* 创建工作线程 */
    osal_thread_t *threads = (osal_thread_t*)OSAL_Malloc(
        sizeof(osal_thread_t) * ctx->config.thread_count);
    worker_thread_args_t *args = (worker_thread_args_t*)OSAL_Malloc(
        sizeof(worker_thread_args_t) * ctx->config.thread_count);

    if (!threads || !args) {
        if (threads) OSAL_Free(threads);
        if (args) OSAL_Free(args);
        return -1;
    }

    /* 启动工作线程 */
    for (uint32_t i = 0; i < ctx->config.thread_count; i++) {
        args[i].ctx = ctx;
        args[i].worker = worker;
        args[i].user_data = user_data;
        args[i].thread_id = i;

        char thread_name[32];
        OSAL_snprintf(thread_name, sizeof(thread_name), "stress_worker_%u", i);

        if (OSAL_ThreadCreate(&threads[i], thread_name,
                              worker_thread_func, &args[i],
                              OSAL_PRIORITY_NORMAL, 0) != 0) {
            /* 创建失败，停止已启动的线程 */
            ctx->should_stop = true;
            for (uint32_t j = 0; j < i; j++) {
                OSAL_ThreadJoin(threads[j]);
            }
            OSAL_Free(threads);
            OSAL_Free(args);
            return -1;
        }

        /* Ramp-up延迟 */
        if (ctx->config.ramp_up_sec > 0 && i < ctx->config.thread_count - 1) {
            uint32_t delay_ms = (ctx->config.ramp_up_sec * 1000) / ctx->config.thread_count;
            OSAL_sleep(delay_ms);
        }
    }

    /* 等待运行时长或迭代完成 */
    if (ctx->config.duration_sec > 0) {
        uint32_t elapsed_sec = 0;
        while (elapsed_sec < ctx->config.duration_sec && !ctx->should_stop) {
            OSAL_sleep(1000);
            elapsed_sec++;
        }
        ctx->should_stop = true;
    }

    /* 等待所有线程结束 */
    for (uint32_t i = 0; i < ctx->config.thread_count; i++) {
        OSAL_ThreadJoin(threads[i]);
    }

    ctx->running = false;

    OSAL_Free(threads);
    OSAL_Free(args);

    return 0;
}

void stress_stop(stress_context_t *ctx) {
    if (ctx) {
        ctx->should_stop = true;
    }
}

int32_t stress_get_stats(stress_context_t *ctx, stress_stats_t *stats) {
    if (!ctx || !stats) {
        return -1;
    }

    OSAL_memset(stats, 0, sizeof(stress_stats_t));

    stats->total_operations = OSAL_AtomicLoad64(&ctx->total_operations);
    stats->successful_ops = OSAL_AtomicLoad64(&ctx->successful_ops);
    stats->failed_ops = OSAL_AtomicLoad64(&ctx->failed_ops);
    stats->timeout_ops = OSAL_AtomicLoad64(&ctx->timeout_ops);
    stats->error_count = OSAL_AtomicLoad32(&ctx->error_count);

    uint64_t elapsed_ms = OSAL_GetTime() - ctx->start_time_ms;
    stats->elapsed_time_ms = elapsed_ms;

    if (elapsed_ms > 0) {
        stats->ops_per_sec = (double)stats->total_operations / (elapsed_ms / 1000.0);
    }

    OSAL_MutexLock(ctx->stats_mutex);
    if (stats->total_operations > 0) {
        stats->avg_latency_us = ctx->total_latency_us / stats->total_operations;
    }
    stats->max_latency_us = ctx->max_latency_us;
    OSAL_MutexUnlock(ctx->stats_mutex);

    return 0;
}

void stress_print_report(stress_context_t *ctx) {
    if (!ctx) return;

    stress_stats_t stats;
    if (stress_get_stats(ctx, &stats) != 0) {
        OSAL_Printf("[ STRESS ERROR ] Failed to get statistics\n");
        return;
    }

    double success_rate = (stats.total_operations > 0) ?
        (100.0 * stats.successful_ops / stats.total_operations) : 0.0;

    OSAL_Printf("\n");
    OSAL_Printf("=== Stress Test Report: %s ===\n", ctx->name);
    OSAL_Printf("Configuration:\n");
    OSAL_Printf("  Threads:      %u\n", ctx->config.thread_count);
    OSAL_Printf("  Duration:     %u sec\n", ctx->config.duration_sec);
    OSAL_Printf("  Iterations:   %u\n", ctx->config.iterations);
    OSAL_Printf("\n");
    OSAL_Printf("Results:\n");
    OSAL_Printf("  Total Ops:    %lu\n", (unsigned long)stats.total_operations);
    OSAL_Printf("  Successful:   %lu (%.2f%%)\n",
               (unsigned long)stats.successful_ops, success_rate);
    OSAL_Printf("  Failed:       %lu\n", (unsigned long)stats.failed_ops);
    OSAL_Printf("  Timeout:      %lu\n", (unsigned long)stats.timeout_ops);
    OSAL_Printf("  Errors:       %u\n", stats.error_count);
    OSAL_Printf("\n");
    OSAL_Printf("Performance:\n");
    OSAL_Printf("  Throughput:   %.2f ops/s\n", stats.ops_per_sec);
    OSAL_Printf("  Avg Latency:  %.2f us\n", stats.avg_latency_us);
    OSAL_Printf("  Max Latency:  %.2f us\n", stats.max_latency_us);
    OSAL_Printf("  Elapsed Time: %lu ms\n", (unsigned long)stats.elapsed_time_ms);
    OSAL_Printf("=====================================\n\n");
}

void stress_record_error(stress_context_t *ctx, const char *error_msg) {
    if (!ctx || !error_msg) return;

    OSAL_AtomicInc32(&ctx->error_count);

    OSAL_MutexLock(ctx->error_mutex);
    if (ctx->error_msg_count < MAX_ERROR_MESSAGES) {
        OSAL_strncpy(ctx->error_messages[ctx->error_msg_count], error_msg, 127);
        ctx->error_messages[ctx->error_msg_count][127] = '\0';
        ctx->error_msg_count++;
    }
    OSAL_MutexUnlock(ctx->error_mutex);
}

bool stress_should_stop(stress_context_t *ctx) {
    return ctx ? ctx->should_stop : true;
}
