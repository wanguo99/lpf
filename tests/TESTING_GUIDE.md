# 测试框架完整指南

## 概述

EMS测试框架提供四种测试类型，覆盖从单元测试到系统集成的完整测试需求：

- **单元测试（Unit Test）**：测试单个模块或函数的功能正确性
- **性能测试（Performance Test）**：测量和验证性能指标（延迟、吞吐量）
- **压力测试（Stress Test）**：验证系统在高负载、长时间运行下的稳定性
- **系统测试（System Test）**：验证多层集成和端到端业务流程

## 目录结构

```
tests/
├── include/                    # 测试框架头文件
│   ├── test_framework.h       # 统一测试框架
│   ├── test_assert.h          # 断言宏
│   ├── test_registry.h        # 测试注册
│   ├── test_performance.h     # 性能测试框架
│   ├── test_stress.h          # 压力测试框架
│   └── test_system.h          # 系统测试框架
├── core/                       # 测试框架核心实现
├── unit/                       # 单元测试
│   ├── osal/                  # OSAL层单元测试
│   ├── hal/                   # HAL层单元测试
│   ├── pdl/                   # PDL层单元测试
│   └── pcl/                   # PCL层单元测试
├── performance/                # 性能测试
│   ├── test_perf_core.c       # 性能测试框架实现
│   └── osal/                  # OSAL层性能测试
├── stress/                     # 压力测试
│   ├── test_stress_core.c     # 压力测试框架实现
│   └── osal/                  # OSAL层压力测试
├── system/                     # 系统测试
│   ├── test_system_core.c     # 系统测试框架实现
│   └── integration/           # 集成测试
├── run_tests.sh               # 测试运行脚本
└── README.md                  # 测试文档
```

## 快速开始

### 1. 编译测试

```bash
# 编译所有测试（Debug模式）
./build.sh -d

# 或者单独编译测试
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
make ems-test -j$(nproc)
```

### 2. 运行测试

```bash
# 运行所有测试
./tests/run_tests.sh

# 运行单元测试
./tests/run_tests.sh -t unit

# 运行性能测试
./tests/run_tests.sh -t performance

# 运行压力测试
./tests/run_tests.sh -t stress

# 运行系统测试
./tests/run_tests.sh -t system

# 运行特定层的测试
./tests/run_tests.sh -t unit -l OSAL

# 生成测试报告
./tests/run_tests.sh -t all -r
```

### 3. 交互式运行

```bash
# 启动交互式菜单
./build/bin/ems-test

# 选择层级 -> 选择模块 -> 选择测试用例
```

## 单元测试

### 编写单元测试

```c
#include "test_framework.h"
#include "osal.h"

/**
 * 测试用例：测试互斥锁创建和销毁
 */
TEST_CASE(test_mutex_create_destroy) {
    osal_mutex_t *mutex = NULL;

    /* 创建互斥锁 */
    TEST_ASSERT_EQUAL(OSAL_MutexCreate(&mutex), 0);
    TEST_ASSERT_NOT_NULL(mutex);

    /* 销毁互斥锁 */
    TEST_ASSERT_EQUAL(OSAL_MutexDestroy(mutex), 0);
}

/**
 * 测试用例：测试互斥锁加锁解锁
 */
TEST_CASE(test_mutex_lock_unlock) {
    osal_mutex_t *mutex = NULL;

    TEST_ASSERT_EQUAL(OSAL_MutexCreate(&mutex), 0);

    /* 加锁 */
    TEST_ASSERT_EQUAL(OSAL_MutexLock(mutex, OSAL_WAIT_FOREVER), 0);

    /* 解锁 */
    TEST_ASSERT_EQUAL(OSAL_MutexUnlock(mutex), 0);

    OSAL_MutexDestroy(mutex);
}

/* 注册测试模块 */
TEST_MODULE_BEGIN(mutex_tests, "OSAL")
    TEST_CASE_REGISTER(test_mutex_create_destroy, "Mutex create and destroy")
    TEST_CASE_REGISTER(test_mutex_lock_unlock, "Mutex lock and unlock")
TEST_MODULE_END(mutex_tests, "OSAL")
```

### 常用断言宏

```c
TEST_ASSERT(condition)                    // 断言条件为真
TEST_ASSERT_EQUAL(expected, actual)       // 断言相等
TEST_ASSERT_NOT_EQUAL(expected, actual)   // 断言不相等
TEST_ASSERT_NULL(ptr)                     // 断言指针为NULL
TEST_ASSERT_NOT_NULL(ptr)                 // 断言指针不为NULL
TEST_ASSERT_TRUE(condition)               // 断言为真
TEST_ASSERT_FALSE(condition)              // 断言为假
TEST_FAIL()                               // 直接失败
```

## 性能测试

### 编写性能测试

```c
#include "test_framework.h"
#include "test_performance.h"
#include "osal.h"

/* 定义性能基准 */
static const perf_baseline_t mutex_baseline = {
    .name = "OSAL_MutexLock",
    .type = PERF_METRIC_LATENCY,
    .baseline_value = 5.0,      /* 基准：5微秒 */
    .tolerance_percent = 50.0   /* 容差：50% */
};

TEST_CASE(test_perf_mutex) {
    const uint32_t iterations = 10000;
    osal_mutex_t *mutex = NULL;

    OSAL_MutexCreate(&mutex);

    /* 创建性能测量上下文 */
    perf_context_t *ctx = perf_context_create("OSAL_MutexLock",
                                               PERF_METRIC_LATENCY,
                                               iterations);

    /* 性能测试 */
    for (uint32_t i = 0; i < iterations; i++) {
        perf_begin(ctx);
        OSAL_MutexLock(mutex, OSAL_WAIT_FOREVER);
        OSAL_MutexUnlock(mutex);
        perf_end(ctx);
    }

    /* 打印统计 */
    perf_print_stats(ctx);

    /* 与基准对比 */
    perf_result_t result;
    perf_compare_baseline(ctx, &mutex_baseline, &result);
    perf_print_result(&result);

    /* 性能断言 */
    PERF_ASSERT_LATENCY_LT(ctx, 20);  /* 延迟必须小于20微秒 */

    /* 导出数据 */
    perf_export_csv(ctx, "mutex_perf.csv");
    perf_export_json(ctx, "mutex_perf.json");

    /* 清理 */
    perf_context_destroy(ctx);
    OSAL_MutexDestroy(mutex);
}

TEST_MODULE_BEGIN(perf_osal, "PERFORMANCE")
    TEST_CASE_REGISTER(test_perf_mutex, "Mutex performance test")
TEST_MODULE_END(perf_osal, "PERFORMANCE")
```

### 性能指标类型

- `PERF_METRIC_LATENCY`：延迟（微秒）
- `PERF_METRIC_THROUGHPUT`：吞吐量（ops/s）
- `PERF_METRIC_CPU_USAGE`：CPU使用率（%）
- `PERF_METRIC_MEMORY_USAGE`：内存使用（字节）
- `PERF_METRIC_CUSTOM`：自定义指标

### 性能统计输出

```
=== Performance Statistics: OSAL_MutexLock ===
Samples:     10000
Mean:        4.23 us
Std Dev:     1.15 us
Min:         2.10 us
Max:         18.50 us
Median(p50): 4.00 us
p95:         6.80 us
p99:         9.20 us
p99.9:       15.30 us
=====================================
```

## 压力测试

### 编写压力测试

```c
#include "test_framework.h"
#include "test_stress.h"
#include "osal.h"

/* 压力测试工作函数 */
static int32_t mutex_worker(void *user_data, uint32_t iteration) {
    osal_mutex_t *mutex = (osal_mutex_t*)user_data;

    /* 加锁 */
    int32_t ret = OSAL_MutexLock(mutex, OSAL_WAIT_FOREVER);
    if (ret != 0) {
        return ret;
    }

    /* 模拟工作 */
    OSAL_Sleep(1);

    /* 解锁 */
    OSAL_MutexUnlock(mutex);

    return 0;
}

TEST_CASE(test_stress_mutex_concurrency) {
    osal_mutex_t *mutex = NULL;
    const uint32_t thread_count = 10;
    const uint32_t duration_sec = 5;

    OSAL_MutexCreate(&mutex);

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Mutex Concurrency", &config);

    /* 运行压力测试 */
    OSAL_Printf("Running mutex concurrency test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    stress_run(ctx, mutex_worker, mutex);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 清理 */
    stress_context_destroy(ctx);
    OSAL_MutexDestroy(mutex);
}

TEST_MODULE_BEGIN(stress_osal, "STRESS")
    TEST_CASE_REGISTER(test_stress_mutex_concurrency, "Mutex concurrency stress")
TEST_MODULE_END(stress_osal, "STRESS")
```

### 压力测试配置宏

```c
/* 并发压力测试 */
STRESS_CONFIG_CONCURRENCY(threads, duration_sec)

/* 长时间运行测试 */
STRESS_CONFIG_DURATION(duration_sec)

/* 迭代次数测试 */
STRESS_CONFIG_ITERATIONS(iterations)
```

### 压力测试报告

```
=== Stress Test Report: Mutex Concurrency ===
Configuration:
  Threads:      10
  Duration:     5 sec
  Iterations:   0

Results:
  Total Ops:    50000
  Successful:   50000 (100.00%)
  Failed:       0
  Timeout:      0
  Errors:       0

Performance:
  Throughput:   10000.00 ops/s
  Avg Latency:  100.50 us
  Max Latency:  250.30 us
  Elapsed Time: 5000 ms
=====================================
```

## 系统测试

### 编写系统测试

```c
#include "test_framework.h"
#include "test_system.h"
#include "osal.h"
#include "hal.h"
#include "pdl.h"

/* 环境初始化 */
SYSTEM_ENV_SETUP(full_stack) {
    OSAL_Printf("Initializing full stack environment\n");

    if (OSAL_Init() != 0) return -1;
    env->osal_initialized = true;

    if (HAL_Init() != 0) return -1;
    env->hal_initialized = true;

    if (PDL_Init() != 0) return -1;
    env->pdl_initialized = true;

    return 0;
}

/* 环境清理 */
SYSTEM_ENV_TEARDOWN(full_stack) {
    if (env->pdl_initialized) PDL_Deinit();
    if (env->hal_initialized) HAL_Deinit();
    if (env->osal_initialized) OSAL_Deinit();
}

/* 系统测试用例 */
SYSTEM_TEST_CASE(osal_hal_integration) {
    /* 检查点1：OSAL初始化 */
    SYSTEM_CHECKPOINT(NULL, "OSAL initialized", env->osal_initialized);

    /* 检查点2：HAL初始化 */
    SYSTEM_CHECKPOINT(NULL, "HAL initialized", env->hal_initialized);

    /* 检查点3：GPIO操作 */
    hal_gpio_handle_t gpio = NULL;
    int32_t ret = HAL_GPIO_Open(0, &gpio);
    SYSTEM_CHECKPOINT(NULL, "GPIO open", ret == 0);

    if (ret == 0) {
        HAL_GPIO_SetDirection(gpio, HAL_GPIO_DIR_OUTPUT);
        HAL_GPIO_Write(gpio, 1);
        HAL_GPIO_Close(gpio);
    }

    return 0;
}

/* 运行系统测试 */
TEST_CASE(test_system_integration) {
    system_test_context_t *ctx;

    ctx = system_test_create("OSAL+HAL Integration", SYSTEM_TEST_INTEGRATION);
    system_test_set_env_funcs(ctx,
                              system_env_setup_full_stack,
                              system_env_teardown_full_stack);
    system_test_run(ctx, system_test_osal_hal_integration);
    system_test_print_report(ctx);
    system_test_destroy(ctx);
}

TEST_MODULE_BEGIN(system_integration, "SYSTEM")
    TEST_CASE_REGISTER(test_system_integration, "System integration test")
TEST_MODULE_END(system_integration, "SYSTEM")
```

### 系统测试类型

- `SYSTEM_TEST_INTEGRATION`：集成测试
- `SYSTEM_TEST_E2E`：端到端测试
- `SYSTEM_TEST_SCENARIO`：场景测试
- `SYSTEM_TEST_REGRESSION`：回归测试

## 测试报告

### 生成测试报告

```bash
# 生成文本报告
./tests/run_tests.sh -t all -r

# 报告保存在 test_reports/ 目录
ls test_reports/
# test_report_20260517_143025.txt
```

### 导出性能数据

```c
/* 导出CSV格式 */
perf_export_csv(ctx, "performance_data.csv");

/* 导出JSON格式 */
perf_export_json(ctx, "performance_data.json");
```

## 最佳实践

### 1. 单元测试

- 每个函数至少一个测试用例
- 测试正常路径和错误路径
- 使用有意义的测试名称
- 保持测试独立性（不依赖其他测试）

### 2. 性能测试

- 使用足够的样本数（至少1000次）
- 预热系统（前几次迭代可能较慢）
- 设置合理的性能基准
- 定期运行性能测试，监控性能退化

### 3. 压力测试

- 逐步增加负载（使用ramp_up_sec）
- 监控资源使用（CPU、内存）
- 测试边界条件（最大并发、最长运行时间）
- 验证错误恢复机制

### 4. 系统测试

- 测试真实业务场景
- 验证多层交互
- 检查资源清理
- 使用检查点跟踪测试进度

## 持续集成

### CI配置示例

```yaml
# .github/workflows/test.yml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: ./build.sh -d
      - name: Run Unit Tests
        run: ./tests/run_tests.sh -t unit -r
      - name: Run Performance Tests
        run: ./tests/run_tests.sh -t performance -r
      - name: Upload Reports
        uses: actions/upload-artifact@v2
        with:
          name: test-reports
          path: test_reports/
```

## 故障排查

### 测试失败

1. 查看详细日志：`./tests/run_tests.sh -t unit -v`
2. 运行单个测试：`./build/bin/ems-test --test <test_name>`
3. 使用调试器：`gdb ./build/bin/ems-test`

### 性能测试不稳定

1. 关闭其他应用程序
2. 固定CPU频率：`sudo cpupower frequency-set -g performance`
3. 增加样本数量
4. 使用更宽的容差范围

### 压力测试超时

1. 检查死锁
2. 增加超时时间
3. 减少并发线程数
4. 检查资源泄漏

## 参考资料

- [测试框架API文档](./include/test_framework.h)
- [性能测试API文档](./include/test_performance.h)
- [压力测试API文档](./include/test_stress.h)
- [系统测试API文档](./include/test_system.h)
