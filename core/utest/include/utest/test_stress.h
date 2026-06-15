/**
 * @file test_stress.h
 * @brief 压力测试框架
 *
 * 提供压力测试的基础设施：
 * - 并发压力测试（多线程）
 * - 长时间运行测试
 * - 资源耗尽测试
 * - 边界条件测试
 * - 稳定性验证
 */

#ifndef TEST_STRESS_H
#define TEST_STRESS_H

#include "osal.h"

/* 压力测试类型 */
typedef enum {
    STRESS_TYPE_CONCURRENCY,    /* 并发压力 */
    STRESS_TYPE_DURATION,       /* 长时间运行 */
    STRESS_TYPE_RESOURCE,       /* 资源耗尽 */
    STRESS_TYPE_BOUNDARY,       /* 边界条件 */
    STRESS_TYPE_MIXED           /* 混合压力 */
} stress_test_type_t;

/* 压力测试配置 */
typedef struct {
    stress_test_type_t type;
    uint32_t thread_count;      /* 并发线程数 */
    uint32_t duration_sec;      /* 运行时长（秒） */
    uint32_t iterations;        /* 迭代次数（0表示无限） */
    uint32_t ramp_up_sec;       /* 启动时间（秒） */
    bool stop_on_error;         /* 遇到错误时停止 */
} stress_config_t;

/* 压力测试统计 */
typedef struct {
    uint64_t total_operations;  /* 总操作数 */
    uint64_t successful_ops;    /* 成功操作数 */
    uint64_t failed_ops;        /* 失败操作数 */
    uint64_t timeout_ops;       /* 超时操作数 */
    double ops_per_sec;         /* 每秒操作数 */
    double avg_latency_us;      /* 平均延迟（微秒） */
    double max_latency_us;      /* 最大延迟（微秒） */
    uint32_t error_count;       /* 错误计数 */
    uint64_t elapsed_time_ms;   /* 运行时间（毫秒） */
} stress_stats_t;

/* 压力测试上下文 */
typedef struct stress_context stress_context_t;

/* 压力测试工作函数 */
typedef int32_t (*stress_worker_func_t)(void *user_data, uint32_t iteration);

/**
 * 创建压力测试上下文
 * @param name 测试名称
 * @param config 测试配置
 * @return 压力测试上下文，失败返回NULL
 */
stress_context_t* stress_context_create(const char *name,
                                         const stress_config_t *config);

/**
 * 销毁压力测试上下文
 * @param ctx 压力测试上下文
 */
void stress_context_destroy(stress_context_t *ctx);

/**
 * 运行压力测试
 * @param ctx 压力测试上下文
 * @param worker 工作函数
 * @param user_data 用户数据
 * @return 0成功，负数失败
 */
int32_t stress_run(stress_context_t *ctx,
                   stress_worker_func_t worker,
                   void *user_data);

/**
 * 停止压力测试
 * @param ctx 压力测试上下文
 */
void stress_stop(stress_context_t *ctx);

/**
 * 获取压力测试统计
 * @param ctx 压力测试上下文
 * @param stats 输出统计数据
 * @return 0成功，负数失败
 */
int32_t stress_get_stats(stress_context_t *ctx, stress_stats_t *stats);

/**
 * 打印压力测试报告
 * @param ctx 压力测试上下文
 */
void stress_print_report(stress_context_t *ctx);

/**
 * 记录压力测试错误
 * @param ctx 压力测试上下文
 * @param error_msg 错误消息
 */
void stress_record_error(stress_context_t *ctx, const char *error_msg);

/**
 * 检查是否应该停止
 * @param ctx 压力测试上下文
 * @return true应该停止，false继续运行
 */
bool stress_should_stop(stress_context_t *ctx);

/* 便捷宏 */

/**
 * 压力测试用例定义
 * @param name 测试名称
 */
#define STRESS_TEST_CASE(name) \
    static void stress_test_##name(void)

/**
 * 并发压力测试配置
 * @param threads 线程数
 * @param duration 运行时长（秒）
 */
#define STRESS_CONFIG_CONCURRENCY(threads, duration) \
    { \
        .type = STRESS_TYPE_CONCURRENCY, \
        .thread_count = (threads), \
        .duration_sec = (duration), \
        .iterations = 0, \
        .ramp_up_sec = 1, \
        .stop_on_error = false \
    }

/**
 * 长时间运行测试配置
 * @param duration 运行时长（秒）
 */
#define STRESS_CONFIG_DURATION(duration) \
    { \
        .type = STRESS_TYPE_DURATION, \
        .thread_count = 1, \
        .duration_sec = (duration), \
        .iterations = 0, \
        .ramp_up_sec = 0, \
        .stop_on_error = false \
    }

/**
 * 迭代次数测试配置
 * @param iters 迭代次数
 */
#define STRESS_CONFIG_ITERATIONS(iters) \
    { \
        .type = STRESS_TYPE_BOUNDARY, \
        .thread_count = 1, \
        .duration_sec = 0, \
        .iterations = (iters), \
        .ramp_up_sec = 0, \
        .stop_on_error = true \
    }

/**
 * 压力测试断言：成功率必须大于阈值
 * @param ctx 压力测试上下文
 * @param threshold_percent 阈值百分比（0-100）
 */
#define STRESS_ASSERT_SUCCESS_RATE_GT(ctx, threshold_percent) \
    do { \
        stress_stats_t stats; \
        stress_get_stats(ctx, &stats); \
        double success_rate = (stats.total_operations > 0) ? \
            (100.0 * stats.successful_ops / stats.total_operations) : 0.0; \
        if (success_rate < (threshold_percent)) { \
            OSAL_printf("[ STRESS FAIL ] Success rate %.2f%% < threshold %.2f%%\n", \
                       success_rate, (double)(threshold_percent)); \
            TEST_FAIL(); \
        } \
    } while (0)

/**
 * 压力测试断言：无错误
 * @param ctx 压力测试上下文
 */
#define STRESS_ASSERT_NO_ERRORS(ctx) \
    do { \
        stress_stats_t stats; \
        stress_get_stats(ctx, &stats); \
        if (stats.error_count > 0) { \
            OSAL_printf("[ STRESS FAIL ] Found %u errors\n", stats.error_count); \
            TEST_FAIL(); \
        } \
    } while (0)

#endif /* TEST_STRESS_H */
