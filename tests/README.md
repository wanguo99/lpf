# Tests (测试框架)

## 模块概述

Tests是EMS的统一测试框架，提供跨层级的单元测试和集成测试能力。

**设计理念**：
- 统一测试框架，支持所有层级（OSAL/HAL/PCL/PDL）
- 交互式菜单和命令行两种运行模式
- 模块化测试注册机制
- 简洁的测试断言API

**测试覆盖**：
- OSAL层：任务、队列、互斥锁、信号等
- HAL层：CAN、串口等硬件驱动
- PCL层：硬件配置查询
- PDL层：外设服务

## 主要特性

- **统一框架**：所有测试模块使用相同的框架和API
- **交互式菜单**：三级选择（层级→模块→测试用例）
- **命令行模式**：支持批量运行、过滤、列表查询
- **自动注册**：测试模块自动注册到测试运行器
- **详细报告**：测试结果统计、失败详情、执行时间

## 编译说明

### 快速开始

```bash
# 在项目根目录编译测试
./build.sh -d           # Debug模式（推荐用于测试）
./build.sh              # Release模式
```

### 单独编译测试模块

```bash
# 方法1: 使用CMake直接编译
mkdir -p output/build && cd output/build
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
make unit-test -j$(nproc)
cd ../..

# 方法2: 在已配置的构建目录中编译
cd output/build
make unit-test -j$(nproc)
cd ../..

# 方法3: 编译单个测试模块（快速迭代）
cd output/build
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
        └── unit-test          # 统一测试运行器
```

### 常用编译命令

```bash
# 完整编译流程
./build.sh -d                   # Debug模式编译所有

# 仅编译测试
cd output/build && make unit-test -j$(nproc) && cd ../..

# 快速迭代单个测试模块
cd output/build && make osal_tests -j$(nproc) && cd ../..
./output/target/bin/unit-test -m test_osal_task

# 清理并重新编译
./build.sh -c && ./build.sh -d

# 查看编译日志
cat output/build.log | grep -A 5 "test"
```

## 运行测试

### 交互式菜单（推荐）

```bash
./output/target/bin/unit-test -i
# 或
./output/target/bin/unit-test --interactive
```

**菜单特点**：
- 三级选择：层级 → 模块 → 测试用例
- 使用序号选择，无需输入完整名称
- 支持单个测试、模块测试、层级测试

**使用示例**：
```
=== EMS 测试框架 ===
请选择测试层级：
1. OSAL (操作系统抽象层)
2. HAL (硬件抽象层)
3. PCL (外设配置库)
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
./output/target/bin/unit-test -a
./output/target/bin/unit-test --all

# 运行指定层级的测试
./output/target/bin/unit-test -L OSAL    # OSAL层测试
./output/target/bin/unit-test -L HAL     # HAL层测试
./output/target/bin/unit-test -L PCL     # PCL层测试
./output/target/bin/unit-test -L PDL     # PDL层测试

# 运行指定模块的测试
./output/target/bin/unit-test -m test_osal_task    # 任务测试
./output/target/bin/unit-test -m test_osal_queue   # 队列测试
./output/target/bin/unit-test -m test_hal_can      # CAN测试

# 列出所有测试
./output/target/bin/unit-test -l
./output/target/bin/unit-test --list

# 显示帮助信息
./output/target/bin/unit-test -h
./output/target/bin/unit-test --help
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
│   ├── main.c                  # 测试入口
│   ├── test_runner.c           # 测试运行器实现
│   ├── test_registry.c         # 测试注册实现
│   └── test_menu.c             # 交互式菜单实现
├── osal/                       # OSAL层测试
│   ├── test_osal_task.c        # 任务管理测试
│   ├── test_osal_queue.c       # 消息队列测试
│   ├── test_osal_mutex.c       # 互斥锁测试
│   └── test_osal_signal.c      # 信号处理测试
├── hal/                        # HAL层测试
│   └── test_hal_can.c          # CAN驱动测试
├── pcl/                        # PCL层测试
│   └── test_pcl_api.c          # PCL API测试
├── pdl/                        # PDL层测试（占位）
│   └── (待添加)
└── CMakeLists.txt              # 统一测试构建配置
```

## 测试覆盖

### OSAL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_osal_task | 15+ | 任务创建、删除、延时、优先级、退出 |
| test_osal_queue | 12+ | 队列创建、删除、发送、接收、超时 |
| test_osal_mutex | 10+ | 互斥锁创建、删除、加锁、解锁、死锁检测 |
| test_osal_signal | 8+ | 信号注册、触发、处理、优雅退出 |

### HAL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_hal_can | 3+ | CAN初始化、发送、接收 |

**注意**：HAL测试需要实际硬件设备（can0、/dev/ttyS0等）

### PCL层测试

| 模块 | 测试用例数 | 覆盖功能 |
|------|-----------|---------|
| test_pcl_api | 5+ | 配置查询、外设枚举 |

### PDL层测试

PDL层测试待添加（占位符）

## 开发指南

### 添加新的测试模块

#### 1. 创建测试文件

在对应层级目录创建测试文件（如 `tests/osal/test_osal_timer.c`）：

```c
#include "test_framework.h"
#include <osal.h>

/* 测试用例1 */
TEST_CASE(test_timer_create)
{
    osal_id_t timer_id;
    int32 ret = OSAL_TimerCreate(&timer_id, "test_timer", NULL, NULL);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    OSAL_TimerDelete(timer_id);
}

/* 测试用例2 */
TEST_CASE(test_timer_set)
{
    osal_id_t timer_id;
    OSAL_TimerCreate(&timer_id, "test_timer", NULL, NULL);
    
    int32 ret = OSAL_TimerSet(timer_id, 1000, 1000);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    OSAL_TimerDelete(timer_id);
}

/* 注册测试模块 */
TEST_MODULE_BEGIN(test_osal_timer, "定时器测试")
    TEST_CASE_REGISTER(test_timer_create, "定时器创建")
    TEST_CASE_REGISTER(test_timer_set, "定时器设置")
TEST_MODULE_END()
```

#### 2. 在CMakeLists.txt中添加源文件

编辑 `tests/CMakeLists.txt`：

```cmake
# OSAL Tests
add_library(osal_tests STATIC
    osal/test_osal_task.c
    osal/test_osal_queue.c
    osal/test_osal_mutex.c
    osal/test_osal_signal.c
    osal/test_osal_timer.c      # 添加新测试文件
)
```

#### 3. 重新编译并运行

```bash
./build.sh -d
./output/target/bin/unit-test -m test_osal_timer
```

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

**规则**：`test_<module>_<component>.c`

**示例**：
- `test_osal_task.c` - OSAL任务测试
- `test_osal_queue.c` - OSAL队列测试
- `test_hal_can.c` - HAL CAN测试
- `test_pcl_api.c` - PCL API测试

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
    OSAL_QueueCreate(&queue_id, "test_queue", 10, sizeof(uint32), 0);
    
    uint32 data = 123;
    int32 ret = OSAL_QueuePut(queue_id, &data, sizeof(data), 1000);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    OSAL_QueueDelete(queue_id);
}

/* ❌ 错误 - 依赖全局变量 */
static osal_id_t g_queue_id;  // 全局变量

TEST_CASE(test_queue_send)
{
    uint32 data = 123;
    OSAL_QueuePut(g_queue_id, &data, sizeof(data), 1000);  // 依赖外部状态
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
    OSAL_QueueCreate(&queue_id, "test_queue", 10, sizeof(uint32), 0);
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
    OSAL_QueueCreate(&queue_id, "test_queue", 10, sizeof(uint32), 0);
    
    uint32 data;
    int32 ret = OSAL_QueueGet(queue_id, &data, sizeof(data), 100);  // 100ms超时
    TEST_ASSERT_EQUAL(OS_QUEUE_TIMEOUT, ret);
    
    OSAL_QueueDelete(queue_id);
}

/* ❌ 错误 - 无超时，可能永久阻塞 */
TEST_CASE(test_queue_receive)
{
    osal_id_t queue_id;
    OSAL_QueueCreate(&queue_id, "test_queue", 10, sizeof(uint32), 0);
    
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
gdb --args ./output/target/bin/unit-test -m test_osal_task
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
./output/target/bin/unit-test -L OSAL
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
