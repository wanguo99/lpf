/**
 * @file test_perf_core.c
 * @brief 性能测试框架核心实现（仅使用OSAL接口）
 */

#include <test_framework/test_performance.h>
#include <test_framework/test_assert.h>
#include "osal.h"

/* 性能测量上下文结构 */
struct perf_context {
    char name[64];
    perf_metric_type_t metric_type;
    uint32_t max_samples;
    uint32_t sample_count;
    double *samples;            /* 样本数组 */
    uint64_t start_time_us;     /* 开始时间（微秒） */
    bool measuring;             /* 是否正在测量 */
};

/**
 * 简单的平方根实现（牛顿迭代法）
 */
static double simple_sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;

    double guess = x / 2.0;
    double epsilon = 0.00001;

    int32_t i;


    for (i = 0; i < 50; i++) {
        double next_guess = (guess + x / guess) / 2.0;
        if ((next_guess - guess) < epsilon && (next_guess - guess) > -epsilon) {
            break;
        }
        guess = next_guess;
    }

    return guess;
}

/**
 * 简单的冒泡排序（用于百分位数计算）
 */
static void simple_sort(double *arr, uint32_t count) {
    uint32_t i;

    for (i = 0; i < count - 1; i++) {
        uint32_t j;

        for (j = 0; j < count - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                double temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/**
 * 计算百分位数
 */
static double calculate_percentile(double *sorted_samples, uint32_t count, double percentile) {
    if (count == 0) return 0.0;

    double index = (percentile / 100.0) * (count - 1);
    uint32_t lower = (uint32_t)index;
    uint32_t upper = lower + 1;

    if (upper >= count) {
        return sorted_samples[count - 1];
    }

    double weight = index - lower;
    return sorted_samples[lower] * (1.0 - weight) + sorted_samples[upper] * weight;
}

perf_context_t* perf_context_create(const char *name,
                                     perf_metric_type_t metric_type,
                                     uint32_t max_samples) {
    if (!name || max_samples == 0) {
        return NULL;
    }

    perf_context_t *ctx = (perf_context_t*)OSAL_malloc(OSAL_sizeof(perf_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->samples = (double*)OSAL_malloc(OSAL_sizeof(double) * max_samples);
    if (!ctx->samples) {
        OSAL_free(ctx);
        return NULL;
    }

    OSAL_strncpy(ctx->name, name, OSAL_sizeof(ctx->name) - 1);
    ctx->name[OSAL_sizeof(ctx->name) - 1] = '\0';
    ctx->metric_type = metric_type;
    ctx->max_samples = max_samples;
    ctx->sample_count = 0;
    ctx->start_time_us = 0;
    ctx->measuring = false;

    return ctx;
}

void perf_context_destroy(perf_context_t *ctx) {
    if (ctx) {
        if (ctx->samples) {
            OSAL_free(ctx->samples);
        }
        OSAL_free(ctx);
    }
}

void perf_begin(perf_context_t *ctx) {
    if (!ctx) return;

    ctx->start_time_us = OSAL_get_monotonic_time();
    ctx->measuring = true;
}

void perf_end(perf_context_t *ctx) {
    if (!ctx || !ctx->measuring) return;

    uint64_t end_time_us = OSAL_get_monotonic_time();
    double elapsed_us = (double)(end_time_us - ctx->start_time_us);

    perf_record(ctx, elapsed_us);
    ctx->measuring = false;
}

void perf_record(perf_context_t *ctx, double value) {
    if (!ctx || ctx->sample_count >= ctx->max_samples) {
        return;
    }

    ctx->samples[ctx->sample_count++] = value;
}

int32_t perf_calculate_stats(perf_context_t *ctx, perf_stats_t *stats) {
    if (!ctx || !stats || ctx->sample_count == 0) {
        return -1;
    }

    OSAL_memset(stats, 0, OSAL_sizeof(perf_stats_t));

    /* 计算基本统计量 */
    stats->count = ctx->sample_count;
    stats->min = ctx->samples[0];
    stats->max = ctx->samples[0];
    stats->sum = 0.0;

    uint32_t i;


    for (i = 0; i < ctx->sample_count; i++) {
        double value = ctx->samples[i];
        stats->sum += value;
        if (value < stats->min) stats->min = value;
        if (value > stats->max) stats->max = value;
    }

    stats->mean = stats->sum / stats->count;

    /* 计算方差和标准差 */
    double variance_sum = 0.0;

    for (i = 0; i < ctx->sample_count; i++) {
        double diff = ctx->samples[i] - stats->mean;
        variance_sum += diff * diff;
    }
    stats->variance = variance_sum / stats->count;
    stats->stddev = simple_sqrt(stats->variance);

    /* 计算百分位数（需要排序） */
    double *sorted = (double*)OSAL_malloc(OSAL_sizeof(double) * ctx->sample_count);
    if (sorted) {
        OSAL_memcpy(sorted, ctx->samples, OSAL_sizeof(double) * ctx->sample_count);
        simple_sort(sorted, ctx->sample_count);

        stats->p50 = calculate_percentile(sorted, ctx->sample_count, 50.0);
        stats->p95 = calculate_percentile(sorted, ctx->sample_count, 95.0);
        stats->p99 = calculate_percentile(sorted, ctx->sample_count, 99.0);
        stats->p999 = calculate_percentile(sorted, ctx->sample_count, 99.9);

        OSAL_free(sorted);
    }

    return 0;
}

int32_t perf_compare_baseline(perf_context_t *ctx,
                               const perf_baseline_t *baseline,
                               perf_result_t *result) {
    if (!ctx || !baseline || !result) {
        return -1;
    }

    OSAL_memset(result, 0, OSAL_sizeof(perf_result_t));

    result->test_name = ctx->name;
    result->metric_type = ctx->metric_type;

    if (perf_calculate_stats(ctx, &result->stats) != 0) {
        return -1;
    }

    /* 计算与基准的差异 */
    double diff = result->stats.mean - baseline->baseline_value;
    result->baseline_diff_percent = (diff / baseline->baseline_value) * 100.0;

    /* 判断是否通过基准测试 */
    double abs_diff_percent = (result->baseline_diff_percent > 0) ?
        result->baseline_diff_percent : -result->baseline_diff_percent;
    result->baseline_passed = (abs_diff_percent <= baseline->tolerance_percent);

    return 0;
}

void perf_print_stats(perf_context_t *ctx) {
    if (!ctx) return;

    perf_stats_t stats;
    if (perf_calculate_stats(ctx, &stats) != 0) {
        OSAL_printf("[ PERF ERROR ] Failed to calculate statistics\n");
        return;
    }

    const char *unit = "";
    switch (ctx->metric_type) {
        case PERF_METRIC_LATENCY:
            unit = "us";
            break;
        case PERF_METRIC_THROUGHPUT:
            unit = "ops/s";
            break;
        case PERF_METRIC_CPU_USAGE:
            unit = "%";
            break;
        case PERF_METRIC_MEMORY_USAGE:
            unit = "bytes";
            break;
        default:
            unit = "";
            break;
    }

    OSAL_printf("\n");
    OSAL_printf("=== Performance Statistics: %s ===\n", ctx->name);
    OSAL_printf("Samples:     %lu\n", (unsigned long)stats.count);
    OSAL_printf("Mean:        %.2f %s\n", stats.mean, unit);
    OSAL_printf("Std Dev:     %.2f %s\n", stats.stddev, unit);
    OSAL_printf("Min:         %.2f %s\n", stats.min, unit);
    OSAL_printf("Max:         %.2f %s\n", stats.max, unit);
    OSAL_printf("Median(p50): %.2f %s\n", stats.p50, unit);
    OSAL_printf("p95:         %.2f %s\n", stats.p95, unit);
    OSAL_printf("p99:         %.2f %s\n", stats.p99, unit);
    OSAL_printf("p99.9:       %.2f %s\n", stats.p999, unit);
    OSAL_printf("=====================================\n\n");
}

void perf_print_result(const perf_result_t *result) {
    if (!result) return;

    const char *status = result->baseline_passed ? "PASS" : "FAIL";
    const char *symbol = result->baseline_passed ? "+" : "X";

    OSAL_printf("\n");
    OSAL_printf("=== Performance Test Result ===\n");
    OSAL_printf("Test:        %s\n", result->test_name);
    OSAL_printf("Status:      [%s] %s\n", symbol, status);
    OSAL_printf("Mean:        %.2f\n", result->stats.mean);
    OSAL_printf("Baseline:    %.2f%%\n", result->baseline_diff_percent);
    OSAL_printf("===============================\n\n");
}

int32_t perf_export_csv(perf_context_t *ctx, const char *filename) {
    if (!ctx || !filename) {
        return -1;
    }

    /* CSV导出功能需要文件操作，暂不实现 */
    OSAL_printf("[ INFO ] CSV export not implemented (requires file I/O)\n");
    return -1;
}

int32_t perf_export_json(perf_context_t *ctx, const char *filename) {
    if (!ctx || !filename) {
        return -1;
    }

    /* JSON导出功能需要文件操作，暂不实现 */
    OSAL_printf("[ INFO ] JSON export not implemented (requires file I/O)\n");
    return -1;
}

