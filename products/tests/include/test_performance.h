/**
 * @file test_performance.h
 * @brief 性能测试框架
 *
 * 提供性能测试的基础设施：
 * - 高精度时间测量
 * - 性能指标采集（延迟、吞吐量、CPU使用率）
 * - 统计分析（平均值、最小值、最大值、标准差、百分位数）
 * - 性能基准对比
 * - 性能报告生成
 */

#ifndef TEST_PERFORMANCE_H
#define TEST_PERFORMANCE_H

#include "osal.h"
#include <stdbool.h>

/* 性能指标类型 */
typedef enum {
    PERF_METRIC_LATENCY,        /* 延迟（微秒） */
    PERF_METRIC_THROUGHPUT,     /* 吞吐量（ops/s） */
    PERF_METRIC_CPU_USAGE,      /* CPU使用率（%） */
    PERF_METRIC_MEMORY_USAGE,   /* 内存使用（字节） */
    PERF_METRIC_CUSTOM          /* 自定义指标 */
} perf_metric_type_t;

/* 性能统计数据 */
typedef struct {
    uint64_t count;             /* 样本数量 */
    double sum;                 /* 总和 */
    double min;                 /* 最小值 */
    double max;                 /* 最大值 */
    double mean;                /* 平均值 */
    double variance;            /* 方差 */
    double stddev;              /* 标准差 */
    double p50;                 /* 50th百分位数（中位数） */
    double p95;                 /* 95th百分位数 */
    double p99;                 /* 99th百分位数 */
    double p999;                /* 99.9th百分位数 */
} perf_stats_t;

/* 性能测量上下文 */
typedef struct perf_context perf_context_t;

/* 性能基准数据 */
typedef struct {
    const char *name;           /* 基准名称 */
    perf_metric_type_t type;    /* 指标类型 */
    double baseline_value;      /* 基准值 */
    double tolerance_percent;   /* 容差百分比 */
} perf_baseline_t;

/* 性能测试结果 */
typedef struct {
    const char *test_name;
    perf_metric_type_t metric_type;
    perf_stats_t stats;
    bool baseline_passed;       /* 是否通过基准测试 */
    double baseline_diff_percent; /* 与基准的差异百分比 */
} perf_result_t;

/**
 * 创建性能测量上下文
 * @param name 测试名称
 * @param metric_type 指标类型
 * @param max_samples 最大样本数（用于百分位数计算）
 * @return 性能上下文指针，失败返回NULL
 */
perf_context_t* perf_context_create(const char *name,
                                     perf_metric_type_t metric_type,
                                     uint32_t max_samples);

/**
 * 销毁性能测量上下文
 * @param ctx 性能上下文
 */
void perf_context_destroy(perf_context_t *ctx);

/**
 * 开始性能测量
 * @param ctx 性能上下文
 */
void perf_begin(perf_context_t *ctx);

/**
 * 结束性能测量并记录样本
 * @param ctx 性能上下文
 */
void perf_end(perf_context_t *ctx);

/**
 * 记录自定义性能样本
 * @param ctx 性能上下文
 * @param value 样本值
 */
void perf_record(perf_context_t *ctx, double value);

/**
 * 计算性能统计
 * @param ctx 性能上下文
 * @param stats 输出统计数据
 * @return 0成功，负数失败
 */
int32_t perf_calculate_stats(perf_context_t *ctx, perf_stats_t *stats);

/**
 * 与基准对比
 * @param ctx 性能上下文
 * @param baseline 基准数据
 * @param result 输出测试结果
 * @return 0成功，负数失败
 */
int32_t perf_compare_baseline(perf_context_t *ctx,
                               const perf_baseline_t *baseline,
                               perf_result_t *result);

/**
 * 打印性能统计报告
 * @param ctx 性能上下文
 */
void perf_print_stats(perf_context_t *ctx);

/**
 * 打印性能对比报告
 * @param result 测试结果
 */
void perf_print_result(const perf_result_t *result);

/**
 * 导出性能数据到CSV
 * @param ctx 性能上下文
 * @param filename 文件名
 * @return 0成功，负数失败
 */
int32_t perf_export_csv(perf_context_t *ctx, const char *filename);

/**
 * 导出性能报告到JSON
 * @param ctx 性能上下文
 * @param filename 文件名
 * @return 0成功，负数失败
 */
int32_t perf_export_json(perf_context_t *ctx, const char *filename);

/* 便捷宏 */

/**
 * 性能测试用例定义
 * @param name 测试名称
 */
#define PERF_TEST_CASE(name) \
    static void perf_test_##name(void)

/**
 * 性能测量块
 * @param ctx 性能上下文
 */
#define PERF_MEASURE(ctx) \
    for (perf_begin(ctx); ctx != NULL; perf_end(ctx), ctx = NULL)

/**
 * 性能断言：延迟必须小于阈值
 * @param ctx 性能上下文
 * @param threshold_us 阈值（微秒）
 */
#define PERF_ASSERT_LATENCY_LT(ctx, threshold_us) \
    do { \
        perf_stats_t stats; \
        perf_calculate_stats(ctx, &stats); \
        if (stats.mean > (threshold_us)) { \
            OSAL_Printf("[ PERF FAIL ] Latency %.2f us > threshold %lu us\n", \
                       stats.mean, (unsigned long)(threshold_us)); \
            TEST_FAIL(); \
        } \
    } while (0)

/**
 * 性能断言：吞吐量必须大于阈值
 * @param ctx 性能上下文
 * @param threshold_ops 阈值（ops/s）
 */
#define PERF_ASSERT_THROUGHPUT_GT(ctx, threshold_ops) \
    do { \
        perf_stats_t stats; \
        perf_calculate_stats(ctx, &stats); \
        if (stats.mean < (threshold_ops)) { \
            OSAL_Printf("[ PERF FAIL ] Throughput %.2f ops/s < threshold %lu ops/s\n", \
                       stats.mean, (unsigned long)(threshold_ops)); \
            TEST_FAIL(); \
        } \
    } while (0)

#endif /* TEST_PERFORMANCE_H */
