# Tests (测试框架)

## 模块概述

Tests是ES-Middleware的统一测试框架，提供完整的测试能力，支持单元测试、性能测试、压力测试和系统测试。

**设计理念**：
- 统一测试框架，支持所有层级（OSAL/HAL/PDL/PConfig/AConfig）
- 四种测试类型：单元测试、性能测试、压力测试、系统测试
- 交互式菜单和命令行两种运行模式
- 模块化测试注册机制
- 简洁的测试断言API
- 性能指标采集和统计分析
- 压力测试和稳定性验证
- 多层集成和端到端测试

**测试类型**：
- **单元测试（Unit Test）**：测试单个模块或函数的功能正确性
- **性能测试（Performance Test）**：测量和验证性能指标（延迟、吞吐量、CPU使用率）
- **压力测试（Stress Test）**：验证系统在高负载、长时间运行下的稳定性
- **系统测试（System Test）**：验证多层集成和端到端业务流程

**测试覆盖**：
- OSAL层：线程、队列、互斥锁、信号量、原子操作、时间等
- HAL层：CAN、UART、I2C、SPI、GPIO、Watchdog等硬件驱动
- PDL层：BMC、MCU、Watchdog等外设服务
- PConfig层：平台配置查询
- AConfig层：业务配置验证

## 主要特性

- **统一框架**：所有测试模块使用相同的框架和API
- **交互式菜单**：三级选择（层级→模块→测试用例）
- **命令行模式**：支持批量运行、过滤、列表查询
- **自动注册**：测试模块自动注册到测试运行器
- **详细报告**：测试结果统计、失败详情、执行时间
- **性能分析**：高精度时间测量、统计分析（平均值、百分位数）、性能基准对比
- **压力测试**：并发压力、长时间运行、资源耗尽、边界条件测试
- **系统测试**：多层集成、端到端测试、场景测试、检查点机制

## 快速开始

### 1. 编译测试

```bash
# 在项目根目录编译测试
./build.sh -d           # Debug模式（推荐用于测试）
./build.sh              # Release模式
```

### 2. 运行测试

```bash
# 使用测试脚本（推荐）
./tests/run_tests.sh                    # 运行所有测试
./tests/run_tests.sh -t unit            # 运行单元测试
./tests/run_tests.sh -t performance     # 运行性能测试
./tests/run_tests.sh -t stress          # 运行压力测试
./tests/run_tests.sh -t system          # 运行系统测试
./tests/run_tests.sh -t unit -l OSAL    # 运行OSAL单元测试
./tests/run_tests.sh -t all -r          # 运行所有测试并生成报告

# 或直接运行测试二进制
./build/bin/es-middleware-test                    # 交互式菜单
./build/bin/es-middleware-test --all              # 运行所有测试
./build/bin/es-middleware-test --layer OSAL       # 运行OSAL层测试
```

### 3. 查看测试报告

```bash
# 测试报告保存在 test_reports/ 目录
ls test_reports/
cat test_reports/test_report_20260517_143025.txt
```

## 测试类型详解

### 单元测试（Unit Test）

测试单个模块或函数的功能正确性。

**特点**：
- 快速执行（毫秒级）
- 独立运行（不依赖其他模块）
- 覆盖正常路径和错误路径
- 使用断言验证结果

**示例**：
```bash
./tests/run_tests.sh -t unit -l OSAL
```

### 性能测试（Performance Test）

测量和验证性能指标，包括延迟、吞吐量、CPU使用率等。

**特点**：
- 高精度时间测量（微秒级）
- 统计分析（平均值、标准差、百分位数）
- 性能基准对比
- 性能退化检测

**示例**：
```bash
./tests/run_tests.sh -t performance -l OSAL
```

**输出示例**：
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

### 压力测试（Stress Test）

验证系统在高负载、长时间运行下的稳定性。

**特点**：
- 并发压力测试（多线程）
- 长时间运行测试（小时级）
- 资源耗尽测试
- 边界条件测试
- 稳定性验证

**示例**：
```bash
./tests/run_tests.sh -t stress -l OSAL
```

**输出示例**：
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

### 系统测试（System Test）

验证多层集成和端到端业务流程。

**特点**：
- 多层集成测试（OSAL+HAL+PDL+PConfig+AConfig）
- 端到端测试（完整业务流程）
- 场景测试（真实使用场景）
- 检查点机制（跟踪测试进度）

**示例**：
```bash
./tests/run_tests.sh -t system
```

## 编译说明

### 快速编译

```bash
# 在项目根目录编译测试
./build.sh -d           # Debug模式（推荐用于测试）
./build.sh              # Release模式
```

### 单独编译测试模块

```bash
# 方法1: 使用CMake直接编译
mkdir -p build && cd build
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
make es-middleware-test -j$(nproc)
cd ../..

# 方法2: 在已配置的构建目录中编译
cd build
make es-middleware-test -j$(nproc)
cd ../..

# 方法3: 编译单个测试模块（快速迭代）
cd build
make osal_tests -j$(nproc)    # 仅编译OSAL测试
make hal_tests -j$(nproc)     # 仅编译HAL测试
cd ../..
```

### 支持的编译参数

#### CMake配置参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `BUILD_TESTING` | BOOL | ON | 是否编译测试 |
| `CMAKE_BUILD_TYPE` | STRING | Release | 编译类型：Release/Debug |

#### 编译类型说明

**Debug模式**（推荐用于测试）：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
```
- 优化级别：`-O0`
- 包含调试信息：`-g`
- 适用于测试和调试

**Release模式**：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
```
- 优化级别：`-O2`
- 无调试信息
- 适用于性能测试

### 禁用测试编译

```bash
# 不编译测试模块
cmake ../.. -DBUILD_TESTING=OFF
make -j$(nproc)
```

### 编译输出

```
output/
├── build/
│   └── lib/
│       ├── libtests_core.a    # 测试框架核心库
│       ├── libosal_tests.a    # OSAL测试库
│       ├── libhal_tests.a     # HAL测试库
│       ├── libpcl_tests.a     # PCL测试库
│       └── libpdl_tests.a     # PDL测试库
└── target/
    └── bin/
        ├── es-middleware-test           # 统一测试运行器（主程序）
        ├── osal-test -> es-middleware-test  # OSAL层测试快捷方式
        ├── hal-test -> es-middleware-test   # HAL层测试快捷方式
        ├── pcl-test -> es-middleware-test   # PConfig层测试快捷方式
        └── pdl-test -> es-middleware-test   # PDL层测试快捷方式
```

### 常用编译命令

```bash
# 完整编译流程
./build.sh -d                   # Debug模式编译所有

# 仅编译测试
cd build && make es-middleware-test -j$(nproc) && cd ../..

# 快速迭代单个测试模块
cd build && make osal_tests -j$(nproc) && cd ../..
./build/bin/es-middleware-test -m test_osal_version

# 清理并重新编译
./build.sh -c && ./build.sh -d

# 查看编译日志
cat build.log | grep -A 5 "test"
```

## 运行测试

### Busybox 风格调用（推荐）

测试框架采用 Busybox 风格设计，提供多种调用方式：

```bash
# 方式1: 使用主程序 es-middleware-test（运行所有层级测试）
./build/bin/es-middleware-test -h

# 方式2: 使用层级快捷方式（自动过滤对应层级）
./build/bin/osal-test --list    # 仅列出OSAL测试
./build/bin/hal-test --list     # 仅列出HAL测试
./build/bin/pcl-test --list     # 仅列出PCL测试
./build/bin/pdl-test --list     # 仅列出PDL测试

# 方式3: 使用层级过滤参数
./build/bin/es-middleware-test -L OSAL    # 运行OSAL层测试
./build/bin/es-middleware-test -L HAL     # 运行HAL层测试
```

**快捷方式说明**：
- `osal-test`、`hal-test`、`pcl-test`、`pdl-test` 是指向 `es-middleware-test` 的软链接
- 程序根据调用名称自动识别并过滤对应层级的测试
- 等效于 `es-middleware-test -L <LAYER>` 但更简洁直观

### 交互式菜单

```bash
./build/bin/es-middleware-test -i
# 或
./build/bin/es-middleware-test --interactive

# 也可以使用层级快捷方式进入交互式菜单
./build/bin/osal-test -i    # 仅显示OSAL测试菜单
```

**菜单特点**：
- 三级选择：层级 → 模块 → 测试用例
- 使用序号选择，无需输入完整名称
- 支持单个测试、模块测试、层级测试

**使用示例**：
```
=== ES-Middleware 测试框架 ===
请选择测试层级：
1. OSAL (操作系统抽象层)
2. HAL (硬件抽象层)
3. PConfig (外设配置库)
4. PDL (外设驱动层)
5. 运行所有测试
0. 退出

请输入选项 (0-5): 1

=== OSAL 测试模块 ===
1. test_osal_task (任务管理测试)
2. test_osal_queue (消息队列测试)
3. test_osal_mutex (互斥锁测试)
4. test_osal_signal (信号处理测试)
5. 运行所有OSAL测试
0. 返回上级菜单

请输入选项 (0-5): 1
```

### 命令行模式

```bash
# 运行所有测试
./build/bin/es-middleware-test -a
./build/bin/es-middleware-test --all

# 运行指定层级的测试（方式1：使用参数）
./build/bin/es-middleware-test -L OSAL    # OSAL层测试
./build/bin/es-middleware-test -L HAL     # HAL层测试
./build/bin/es-middleware-test -L PConfig     # PConfig层测试
./build/bin/es-middleware-test -L PDL     # PDL层测试

# 运行指定层级的测试（方式2：使用快捷方式，推荐）
./build/bin/osal-test -a        # 运行所有OSAL测试
./build/bin/hal-test -a         # 运行所有HAL测试
./build/bin/pcl-test -a         # 运行所有PCL测试
./build/bin/pdl-test -a         # 运行所有PDL测试

# 运行指定模块的测试
./build/bin/es-middleware-test -m test_osal_version    # 版本测试
./build/bin/es-middleware-test -m test_osal_atomic     # 原子操作测试
./build/bin/osal-test -m test_osal_version   # 等效于上面，但更简洁

# 列出所有测试
./build/bin/es-middleware-test -l         # 列出所有层级测试
./build/bin/es-middleware-test --list
./build/bin/osal-test --list    # 仅列出OSAL测试
./build/bin/hal-test --list     # 仅列出HAL测试

# 显示帮助信息
./build/bin/es-middleware-test -h
./build/bin/es-middleware-test --help
./build/bin/osal-test -h        # 显示OSAL层专用帮助
```

### 测试输出示例

```
========================================
运行测试模块: test_osal_task
========================================

[RUN ] test_task_create_delete
[PASS] test_task_create_delete (12 ms)

[RUN ] test_task_delay
[PASS] test_task_delay (105 ms)

[RUN ] test_task_priority
[PASS] test_task_priority (8 ms)

========================================
测试结果统计
========================================
总测试数: 3
通过: 3
失败: 0
跳过: 0
总耗时: 125 ms
========================================
```

## 模块结构

```
tests/
├── include/                    # 测试框架头文件
│   ├── test_framework.h        # 测试框架宏定义
│   ├── test_runner.h           # 测试运行器接口
│   ├── test_registry.h         # 测试注册接口
│   └── tests_core.h            # 测试核心接口
├── core/                       # 测试框架核心
│   ├── main.c                  # Busybox风格主入口
│   ├── test_runner.c           # 测试运行器实现
│   ├── test_registry.c         # 测试注册实现
│   └── test_menu.c             # 交互式菜单实现
├── unit/                       # 单元测试
│   ├── osal/                   # OSAL层单元测试
│   │   ├── test_osal_version.c
│   │   ├── test_osal_atomic.c
│   │   ├── test_osal_string.c
│   │   └── ...
│   ├── hal/                    # HAL层单元测试
│   │   ├── test_hal_can.c
│   │   ├── test_hal_serial.c
│   │   └── ...
│   ├── pcl/                    # PConfig层单元测试
│   │   └── test_pcl_api.c
│   └── pdl/                    # PDL层单元测试
│       ├── test_pdl_bmc.c
│       └── ...
├── system/                     # 系统测试（待添加）
│   └── (跨层级集成测试)
├── stress/                     # 压力测试（待添加）
│   └── (性能和稳定性测试)
├── docs/                       # 测试文档
│   ├── OPTIMIZATION_PLAN.md    # 优化计划
│   └── README.md               # 详细文档
└── CMakeLists.txt              # 统一测试构建配置
```

## 测试覆盖

### OSAL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_osal_version | 3 | 版本信息查询 |
| test_osal_atomic | 3 | 原子操作（加、减、交换） |
| test_osal_string | 6 | 字符串操作（长度、比较、复制、拼接、查找） |
| test_osal_time | 4 | 时间获取和转换 |
| test_osal_heap | 5 | 内存分配和释放 |
| test_osal_log | 4 | 日志系统 |
| test_osal_errno | 3 | 错误码处理 |
| test_osal_env | 3 | 环境变量操作 |
| test_osal_file | 7 | 文件操作 |
| test_osal_clock | 3 | 时钟操作 |
| test_osal_mutex | 5 | 互斥锁 |
| test_osal_semaphore | 5 | 信号量 |
| test_osal_cond | 3 | 条件变量 |
| test_osal_signal | 3 | 信号处理 |
| test_osal_stress | 3 | 压力测试 |

### HAL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_hal_can | 3 | CAN初始化、发送、接收 |
| test_hal_serial | 3 | 串口初始化、读写 |
| test_hal_i2c | 3 | I2C初始化、读写 |
| test_hal_spi | 3 | SPI初始化、读写 |

**注意**：HAL测试需要实际硬件设备（can0、/dev/ttyS0等）

### PConfig层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_pcl_api | 3 | 配置查询、外设枚举 |

### PDL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_pdl_bmc | 3 | BMC通信 |
| test_pdl_bmc_protocol | 3 | BMC协议 |
| test_pdl_mcu | 3 | MCU通信 |
| test_pdl_satellite | 3 | 卫星通信 |

## 开发指南

### 添加新的测试模块

**⚡ 快速开始：** 查看 [如何添加测试完整指南](examples/HOWTO_ADD_TEST.md)

### 简化流程（推荐）

新的测试系统支持**自动发现**和**简化注册**，大幅减少样板代码。

#### 1. 创建测试文件

文件命名：`test_<layer>_<module>.c`（例如：`test_osal_timer.c`）

```c
#include "test_framework.h"
#include "osal.h"

/* 测试用例 */
TEST_CASE(test_timer_create_success)
{
    osal_timer_t *timer = NULL;
    int32_t ret = OSAL_TimerCreate(&timer, "test_timer");
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_TimerDelete(timer);
}

TEST_CASE(test_timer_start_stop)
{
    osal_timer_t *timer = NULL;
    OSAL_TimerCreate(&timer, "test_timer");
    
    int32_t ret = OSAL_TimerStart(timer, 1000, 0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    
    OSAL_TimerStop(timer);
    OSAL_TimerDelete(timer);
}

/* 创建测试用例数组 */
static const test_case_t osal_timer_cases[] = {
    TEST_CASE_ENTRY(test_timer_create_success),
    TEST_CASE_ENTRY(test_timer_start_stop),
};

/* 注册测试套件（一行搞定！） */
TEST_SUITE_REGISTER(osal_timer, "OSAL", osal_timer_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "OSAL timer operations");
```

#### 2. 添加 Config.in 配置

在 `products/tests/unit/osal/Kconfig` 中添加：

```kconfig
config TEST_OSAL_TIMER
    bool "Test OSAL timer"
    depends on CONFIG_OSAL
    default y
    help
      Test OSAL timer operations (create, start, stop, delete).
      Runtime: <100ms
      Hardware: None
```

**重要：** 配置名必须与文件名对应：
- 文件：`test_osal_timer.c`
- 配置：`CONFIG_TEST_OSAL_TIMER`

#### 3. 编译并运行（无需修改 CMakeLists.txt！）

```bash
# 配置
make tests_x86_full_defconfig_defconfig

# 编译（CMake 自动发现新测试）
make

# 运行测试
./_build/bin/es-middleware-test -m osal_timer
```

### 新系统优势

✅ **自动发现**：CMake 自动扫描测试文件，无需手动编辑 CMakeLists.txt  
✅ **简化注册**：相比传统方式减少 60% 样板代码  
✅ **元数据支持**：支持分类、标签、超时、描述  
✅ **灵活过滤**：按分类、标签、层级过滤测试  
✅ **细粒度控制**：每个测试文件独立的 Kconfig 选项

### 详细文档

- **[如何添加测试完整指南](examples/HOWTO_ADD_TEST.md)** - 详细步骤和故障排除
- **[测试模板](examples/test_template.c)** - 可复制的模板文件
- **[简化示例](examples/test_example_simplified.c)** - 推荐的编写方式
- **[元数据示例](examples/test_example_metadata.c)** - 元数据使用示例
- **[Fixture 示例](examples/test_example_with_fixture.c)** - Setup/Teardown 使用

### 测试框架API

#### 测试模块注册

```c
/* 定义测试模块 */
TEST_MODULE_BEGIN(module_name, "模块描述")
    TEST_CASE_REGISTER(test_case1, "测试用例1描述")
    TEST_CASE_REGISTER(test_case2, "测试用例2描述")
TEST_MODULE_END()
```

#### 测试用例定义

```c
/* 定义测试用例 */
TEST_CASE(test_case_name)
{
    /* 测试代码 */
    TEST_ASSERT_EQUAL(expected, actual);
}
```

#### 断言宏

```c
/* 相等断言 */
TEST_ASSERT_EQUAL(expected, actual)

/* 不等断言 */
TEST_ASSERT_NOT_EQUAL(expected, actual)

/* 真值断言 */
TEST_ASSERT_TRUE(condition)

/* 假值断言 */
TEST_ASSERT_FALSE(condition)

/* 空指针断言 */
TEST_ASSERT_NULL(pointer)

/* 非空指针断言 */
TEST_ASSERT_NOT_NULL(pointer)

/* 字符串相等断言 */
TEST_ASSERT_STR_EQUAL(expected, actual)

/* 内存相等断言 */
TEST_ASSERT_MEM_EQUAL(expected, actual, size)
```

### 测试文件命名规范

**规则**：`test_<layer>_<component>.c`

**示例**：
- `unit/osal/test_osal_version.c` - OSAL版本测试
- `unit/osal/test_osal_atomic.c` - OSAL原子操作测试
- `unit/hal/test_hal_can.c` - HAL CAN测试
- `unit/pcl/test_pcl_api.c` - PConfig API测试
- `unit/pdl/test_pdl_bmc.c` - PDL BMC测试

**目录结构**：
- `unit/` - 单元测试（测试单个模块或函数）
- `system/` - 系统测试（测试跨层级集成）
- `stress/` - 压力测试（测试性能和稳定性）

**详细规范**：参考 [编码规范](../docs/CODING_STANDARDS.md)

### 编码规范（重要）

**必须遵守**：
- ✅ 使用测试框架宏：`TEST_MODULE_BEGIN/END`, `TEST_CASE`, `TEST_ASSERT_*`
- ✅ 使用OSAL日志：`LOG_INFO()`, `LOG_ERROR()`
- ❌ 禁止使用：`printf()`, `fprintf()`
- ✅ 测试用例必须独立，不依赖执行顺序
- ✅ 测试后清理资源（任务、队列、互斥锁等）

**示例**：
```c
/* ✅ 正确 */
TEST_CASE(test_example)
{
    osal_id_t task_id;
    int32 ret = OSAL_TaskCreate(&task_id, "test", task_func, NULL, 
                                 8192, OSAL_PRIORITY_C(100), 0);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    /* 清理资源 */
    OSAL_TaskDelete(task_id);
}

/* ❌ 错误 */
TEST_CASE(test_example)
{
    osal_id_t task_id;
    OSAL_TaskCreate(&task_id, "test", task_func, NULL, 
                    8192, OSAL_PRIORITY_C(100), 0);
    // 未检查返回值
    // 未清理资源
}
```

## 测试最佳实践

### 1. 测试独立性

每个测试用例必须独立，不依赖其他测试的执行结果：

```c
/* ✅ 正确 - 每个测试独立创建资源 */
TEST_CASE(test_queue_send)
{
    osal_id_t queue_id;
    OSAL_QueueCreate(&queue_id, "test_queue", 10, OSAL_SIZEOF(uint32), 0);
    
    uint32 data = 123;
    int32 ret = OSAL_QueuePut(queue_id, &data, OSAL_SIZEOF(data), 1000);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    OSAL_QueueDelete(queue_id);
}

/* ❌ 错误 - 依赖全局变量 */
static osal_id_t g_queue_id;  // 全局变量

TEST_CASE(test_queue_send)
{
    uint32 data = 123;
    OSAL_QueuePut(g_queue_id, &data, OSAL_SIZEOF(data), 1000);  // 依赖外部状态
}
```

### 2. 资源清理

测试后必须清理所有创建的资源：

```c
TEST_CASE(test_with_cleanup)
{
    osal_id_t task_id, queue_id, mutex_id;
    
    /* 创建资源 */
    OSAL_TaskCreate(&task_id, "test", task_func, NULL, 8192, 100, 0);
    OSAL_QueueCreate(&queue_id, "test_queue", 10, OSAL_SIZEOF(uint32), 0);
    OSAL_MutexCreate(&mutex_id, "test_mutex", 0);
    
    /* 测试逻辑 */
    // ...
    
    /* 清理资源（即使测试失败也要清理） */
    OSAL_TaskDelete(task_id);
    OSAL_QueueDelete(queue_id);
    OSAL_MutexDelete(mutex_id);
}
```

### 3. 超时设置

涉及阻塞操作的测试必须设置合理的超时：

```c
/* ✅ 正确 - 设置超时 */
TEST_CASE(test_queue_receive)
{
    osal_id_t queue_id;
    OSAL_QueueCreate(&queue_id, "test_queue", 10, OSAL_SIZEOF(uint32), 0);
    
    uint32 data;
    int32 ret = OSAL_QueueGet(queue_id, &data, OSAL_SIZEOF(data), 100);  // 100ms超时
    TEST_ASSERT_EQUAL(OS_QUEUE_TIMEOUT, ret);
    
    OSAL_QueueDelete(queue_id);
}

/* ❌ 错误 - 无超时，可能永久阻塞 */
TEST_CASE(test_queue_receive)
{
    osal_id_t queue_id;
    OSAL_QueueCreate(&queue_id, "test_queue", 10, OSAL_SIZEOF(uint32), 0);
    
    uint32 data;
    OSAL_QueueGet(queue_id, &data, sizeof(data), OSAL_WAIT_FOREVER);  // 危险
}
```

### 4. 错误处理

测试必须验证错误情况：

```c
TEST_CASE(test_error_handling)
{
    /* 测试空指针 */
    int32 ret = OSAL_TaskCreate(NULL, "test", task_func, NULL, 8192, 100, 0);
    TEST_ASSERT_EQUAL(OS_INVALID_POINTER, ret);
    
    /* 测试无效参数 */
    osal_id_t task_id;
    ret = OSAL_TaskCreate(&task_id, NULL, task_func, NULL, 8192, 100, 0);
    TEST_ASSERT_EQUAL(OS_INVALID_POINTER, ret);
}
```

## 常见问题

**Q: 编译时提示找不到测试框架头文件？**
```bash
# 确保BUILD_TESTING=ON
cmake ../.. -DBUILD_TESTING=ON
```

**Q: 测试运行时段错误（Segmentation fault）？**
```bash
# 使用Debug模式编译，使用GDB调试
./build.sh -d
gdb --args ./build/bin/es-middleware-test -m test_osal_task
(gdb) run
(gdb) bt
```

**Q: HAL测试失败（CAN/串口设备不存在）？**
```bash
# HAL测试需要实际硬件设备，可以创建虚拟设备
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

**Q: 如何跳过硬件相关测试？**
```bash
# 只运行OSAL层测试（不需要硬件）
./build/bin/es-middleware-test -L OSAL
# 或使用快捷方式
./build/bin/osal-test -a
```

**Q: 如何添加测试超时？**
```c
/* 在测试用例中使用OSAL延时和超时机制 */
TEST_CASE(test_with_timeout)
{
    uint32 start_time = OSAL_GetTimeMs();
    
    /* 执行测试 */
    do_something();
    
    uint32 elapsed = OSAL_GetTimeMs() - start_time;
    TEST_ASSERT_TRUE(elapsed < 1000);  // 验证在1秒内完成
}
```

## 设计原则

1. **统一框架**：所有层级使用相同的测试框架和API
2. **自动注册**：测试模块自动注册，无需手动维护列表
3. **独立性**：每个测试用例独立，不依赖执行顺序
4. **资源管理**：测试后必须清理所有资源
5. **错误覆盖**：测试正常路径和错误路径

## 参考文档

- [测试框架详细文档](docs/README.md)
- [测试用例编写指南](docs/TEST_GUIDE.md)
- [编码规范](../docs/CODING_STANDARDS.md)
- [OSAL层文档](../osal/README.md)
- [HAL层文档](../hal/README.md)
- [PCL文档](../pcl/README.md)
- [PDL文档](../pdl/README.md)
