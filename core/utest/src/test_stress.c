/**
 * @file test_stress.c
 * @brief 压力测试框架实现
 */

#include "test_stress.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 压力测试上下文结构 */
struct stress_context {
    char name[128];
    stress_config_t config;
    stress_stats_t stats;
    volatile bool should_stop;
    osal_mutex_t stats_mutex;
    uint64_t start_time_us;
};

/**
 * 获取当前时间（微秒）
 */
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/**
 * 创建压力测试上下文
 */
stress_context_t* stress_context_create(const char *name,
                                         const stress_config_t *config) {
    if (!name || !config) {
        return NULL;
    }

    stress_context_t *ctx = (stress_context_t*)calloc(1, sizeof(stress_context_t));
    if (!ctx) {
        return NULL;
    }

    strncpy(ctx->name, name, sizeof(ctx->name) - 1);
    ctx->config = *config;
    ctx->should_stop = false;

    if (OSAL_pthread_mutex_init(&ctx->stats_mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }

    return ctx;
}

/**
 * 销毁压力测试上下文
 */
void stress_context_destroy(stress_context_t *ctx) {
    if (!ctx) {
        return;
    }

    OSAL_pthread_mutex_destroy(&ctx->stats_mutex);
    free(ctx);
}

/* 线程工作数据 */
typedef struct {
    stress_context_t *ctx;
    stress_worker_func_t worker;
    void *user_data;
    uint32_t thread_id;
} worker_thread_data_t;

/**
 * 工作线程函数
 */
static void* worker_thread(void *arg) {
    worker_thread_data_t *data = (worker_thread_data_t*)arg;
    stress_context_t *ctx = data->ctx;
    uint32_t iteration = 0;

    while (!ctx->should_stop) {
        /* 检查迭代次数限制 */
        if (ctx->config.iterations > 0 && iteration >= ctx->config.iterations) {
            break;
        }

        /* 检查时长限制 */
        if (ctx->config.duration_sec > 0) {
            uint64_t elapsed_ms = (get_time_us() - ctx->start_time_us) / 1000;
            if (elapsed_ms >= (uint64_t)ctx->config.duration_sec * 1000) {
                break;
            }
        }

        /* 执行工作函数 */
        uint64_t start = get_time_us();
        int32_t result = data->worker(data->user_data, iteration);
        uint64_t end = get_time_us();
        uint64_t latency = end - start;

        /* 更新统计 */
        OSAL_pthread_mutex_lock(&ctx->stats_mutex);
        ctx->stats.total_operations++;
        if (result == 0) {
            ctx->stats.successful_ops++;
        } else {
            ctx->stats.failed_ops++;
            ctx->stats.error_count++;
        }

        /* 更新延迟统计 */
        if (latency > ctx->stats.max_latency_us) {
            ctx->stats.max_latency_us = (double)latency;
        }

        /* 更新平均延迟（增量计算） */
        uint64_t n = ctx->stats.total_operations;
        ctx->stats.avg_latency_us =
            (ctx->stats.avg_latency_us * (n - 1) + latency) / n;

        OSAL_pthread_mutex_unlock(&ctx->stats_mutex);

        /* 检查是否遇错停止 */
        if (ctx->config.stop_on_error && result != 0) {
            ctx->should_stop = true;
            break;
        }

        iteration++;
    }

    return NULL;
}

/**
 * 运行压力测试
 */
int32_t stress_run(stress_context_t *ctx,
                   stress_worker_func_t worker,
                   void *user_data) {
    if (!ctx || !worker) {
        return -1;
    }

    /* 重置统计和状态 */
    memset(&ctx->stats, 0, sizeof(ctx->stats));
    ctx->should_stop = false;
    ctx->start_time_us = get_time_us();

    /* 创建工作线程 */
    uint32_t thread_count = ctx->config.thread_count;
    if (thread_count == 0) {
        thread_count = 1;
    }

    osal_thread_t *threads = (osal_thread_t*)calloc(thread_count, sizeof(osal_thread_t));
    worker_thread_data_t *thread_data =
        (worker_thread_data_t*)calloc(thread_count, sizeof(worker_thread_data_t));

    if (!threads || !thread_data) {
        free(threads);
        free(thread_data);
        return -1;
    }

    /* 启动线程 */
    for (uint32_t i = 0; i < thread_count; i++) {
        thread_data[i].ctx = ctx;
        thread_data[i].worker = worker;
        thread_data[i].user_data = user_data;
        thread_data[i].thread_id = i;

        if (OSAL_pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            /* 启动失败，停止已启动的线程 */
            ctx->should_stop = true;
            for (uint32_t j = 0; j < i; j++) {
                OSAL_pthread_join(threads[j], NULL);
            }
            free(threads);
            free(thread_data);
            return -1;
        }

        /* Ramp-up延迟 */
        if (ctx->config.ramp_up_sec > 0 && i < thread_count - 1) {
            uint32_t delay_ms = (ctx->config.ramp_up_sec * 1000) / thread_count;
            struct timespec ts = {
                .tv_sec = delay_ms / 1000,
                .tv_nsec = (delay_ms % 1000) * 1000000
            };
            nanosleep(&ts, NULL);
        }
    }

    /* 等待所有线程完成 */
    for (uint32_t i = 0; i < thread_count; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 计算最终统计 */
    uint64_t elapsed_us = get_time_us() - ctx->start_time_us;
    ctx->stats.elapsed_time_ms = elapsed_us / 1000;

    if (elapsed_us > 0) {
        ctx->stats.ops_per_sec =
            (double)ctx->stats.total_operations * 1000000.0 / elapsed_us;
    }

    free(threads);
    free(thread_data);

    return 0;
}

/**
 * 停止压力测试
 */
void stress_stop(stress_context_t *ctx) {
    if (ctx) {
        ctx->should_stop = true;
    }
}

/**
 * 获取压力测试统计
 */
int32_t stress_get_stats(stress_context_t *ctx, stress_stats_t *stats) {
    if (!ctx || !stats) {
        return -1;
    }

    OSAL_pthread_mutex_lock(&ctx->stats_mutex);
    *stats = ctx->stats;
    OSAL_pthread_mutex_unlock(&ctx->stats_mutex);

    return 0;
}

/**
 * 打印压力测试报告
 */
void stress_print_report(stress_context_t *ctx) {
    if (!ctx) {
        return;
    }

    stress_stats_t stats;
    stress_get_stats(ctx, &stats);

    double success_rate = (stats.total_operations > 0) ?
        (100.0 * stats.successful_ops / stats.total_operations) : 0.0;

    OSAL_printf("\n");
    OSAL_printf("=== Stress Test Report: %s ===\n", ctx->name);
    OSAL_printf("Configuration:\n");
    OSAL_printf("  Type: %d\n", ctx->config.type);
    OSAL_printf("  Threads: %u\n", ctx->config.thread_count);
    OSAL_printf("  Duration: %u sec\n", ctx->config.duration_sec);
    OSAL_printf("  Iterations: %u\n", ctx->config.iterations);
    OSAL_printf("\n");
    OSAL_printf("Results:\n");
    OSAL_printf("  Total operations: %llu\n", (unsigned long long)stats.total_operations);
    OSAL_printf("  Successful: %llu (%.2f%%)\n",
               (unsigned long long)stats.successful_ops, success_rate);
    OSAL_printf("  Failed: %llu\n", (unsigned long long)stats.failed_ops);
    OSAL_printf("  Errors: %u\n", stats.error_count);
    OSAL_printf("  Elapsed time: %llu ms\n", (unsigned long long)stats.elapsed_time_ms);
    OSAL_printf("  Operations/sec: %.2f\n", stats.ops_per_sec);
    OSAL_printf("  Avg latency: %.2f us\n", stats.avg_latency_us);
    OSAL_printf("  Max latency: %.2f us\n", stats.max_latency_us);
    OSAL_printf("====================================\n");
    OSAL_printf("\n");
}

/**
 * 记录压力测试错误
 */
void stress_record_error(stress_context_t *ctx, const char *error_msg) {
    if (!ctx) {
        return;
    }

    OSAL_pthread_mutex_lock(&ctx->stats_mutex);
    ctx->stats.error_count++;
    OSAL_pthread_mutex_unlock(&ctx->stats_mutex);

    if (error_msg) {
        OSAL_printf("[ STRESS ERROR ] %s\n", error_msg);
    }
}

/**
 * 检查是否应该停止
 */
bool stress_should_stop(stress_context_t *ctx) {
    return ctx ? ctx->should_stop : true;
}
