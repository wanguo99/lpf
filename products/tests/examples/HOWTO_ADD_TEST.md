# 如何添加测试 (How to Add a Test)

本指南提供添加新测试的完整步骤说明，从创建文件到运行测试。

## 📋 目录

1. [快速开始](#快速开始)
2. [详细步骤](#详细步骤)
3. [命名约定](#命名约定)
4. [最佳实践](#最佳实践)
5. [故障排除](#故障排除)
6. [示例参考](#示例参考)

---

## 快速开始

**5 分钟添加一个测试：**

```bash
# 1. 创建测试文件
vim products/tests/unit/osal/test_osal_timer.c

# 2. 添加 Kconfig 配置
vim products/tests/unit/osal/Kconfig

# 3. 编译并运行
python3 build.py config tests_x86_full_defconfig
python3 build.py build
./_build/bin/ems-test -m test_osal_timer
```

---

## 详细步骤

### 步骤 1: 确定测试位置

根据测试类型和模块选择目录：

```
products/tests/
├── unit/           # 单元测试（测试单个函数/模块）
│   ├── osal/      # OSAL 层测试
│   ├── hal/       # HAL 层测试
│   ├── pdl/       # PDL 层测试
│   ├── prl/       # PRL 层测试
│   ├── pconfig/   # PCONFIG 层测试
│   └── aconfig/   # ACONFIG 层测试
├── performance/    # 性能测试（测试性能指标）
│   ├── osal/
│   ├── hal/
│   └── pdl/
├── stress/         # 压力测试（高负载、并发）
│   ├── osal/
│   ├── hal/
│   └── pdl/
└── system/         # 系统测试（集成测试）
    ├── osal/
    ├── hal/
    ├── pdl/
    └── integration/
```

**选择规则：**
- **Unit**: 测试单个 API 或函数（隔离的、快速的）
- **Performance**: 测试性能指标（吞吐量、延迟、CPU 使用率）
- **Stress**: 测试稳定性（并发、长时间运行、资源耗尽）
- **System**: 测试多模块集成（跨层、端到端）

### 步骤 2: 创建测试文件

**文件命名规范：** `test_<layer>_<module>.c`

**示例：** `products/tests/unit/osal/test_osal_timer.c`

```c
/**
 * @file test_osal_timer.c
 * @brief OSAL Timer Unit Tests
 *
 * Test Coverage:
 * - Timer creation and deletion
 * - Timer start and stop
 * - Timer callback execution
 * - Error handling (null pointers, invalid parameters)
 */

#include "test_framework.h"
#include "osal.h"

/* 测试用例 1: 定时器创建成功 */
TEST_CASE(test_timer_create_success)
{
    osal_timer_t *timer = NULL;
    int32_t ret = OSAL_TimerCreate(&timer, "test_timer");
    
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(timer);
    
    OSAL_TimerDelete(timer);
}

/* 测试用例 2: 定时器创建失败 - 空指针 */
TEST_CASE(test_timer_create_null_pointer)
{
    int32_t ret = OSAL_TimerCreate(NULL, "test_timer");
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例 3: 定时器启动和停止 */
TEST_CASE(test_timer_start_stop)
{
    osal_timer_t *timer = NULL;
    OSAL_TimerCreate(&timer, "test_timer");
    
    int32_t ret = OSAL_TimerStart(timer, 1000, 0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    
    ret = OSAL_TimerStop(timer);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    
    OSAL_TimerDelete(timer);
}

/* 创建测试用例数组 */
static const test_case_t osal_timer_cases[] = {
    TEST_CASE_ENTRY(test_timer_create_success),
    TEST_CASE_ENTRY(test_timer_create_null_pointer),
    TEST_CASE_ENTRY(test_timer_start_stop),
};

/* 注册测试套件 */
TEST_SUITE_REGISTER(osal_timer, "OSAL", osal_timer_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "OSAL timer operations (create, start, stop, delete)");
```

**关键要素：**
1. 包含 `test_framework.h` 和被测试模块的头文件
2. 使用 `TEST_CASE(test_name)` 定义测试用例
3. 使用 `TEST_ASSERT_*` 宏进行断言
4. 创建 `test_case_t` 数组列出所有测试
5. 使用 `TEST_SUITE_REGISTER` 注册测试套件

### 步骤 3: 添加 Kconfig 配置

编辑对应层级的 Kconfig 文件，添加新测试的配置选项。

**文件：** `products/tests/unit/osal/Kconfig`

```kconfig
config TEST_OSAL_TIMER
    bool "Test OSAL timer"
    depends on CONFIG_OSAL
    default y
    help
      Test OSAL timer operations (create, start, stop, delete).
      
      Test Coverage:
      - Timer creation and deletion
      - Timer start and stop
      - Timer callback execution
      - Error handling (null pointers, invalid parameters)
      
      Runtime: <100ms
      Hardware: None
      Dependencies: CONFIG_OSAL
```

**Kconfig 命名规则：**
- 配置选项名称：`CONFIG_TEST_<LAYER>_<MODULE>`
- 必须与文件名对应：`test_osal_timer.c` → `CONFIG_TEST_OSAL_TIMER`
- 使用大写和下划线

**帮助文本内容：**
- 简短描述（一行）
- 测试覆盖范围
- 预期运行时间
- 硬件依赖
- 配置依赖

### 步骤 4: CMake 自动发现（无需手动修改）

新的测试系统使用自动发现机制，**无需手动编辑 CMakeLists.txt**。

CMake 会自动：
1. 扫描测试目录下的所有 `test_*.c` 文件
2. 从文件名推导 Kconfig 选项名（`test_osal_timer.c` → `CONFIG_TEST_OSAL_TIMER`）
3. 检查 Kconfig 选项是否启用
4. 自动添加到编译目标

**如何验证自动发现：**

```bash
# 启用详细输出查看发现的测试文件
python3 build.py build --verbose 2>&1 | grep "Discovered test"
```

**如果自动发现失败：**
- 检查文件命名是否符合规范：`test_<layer>_<module>.c`
- 检查 Kconfig 选项是否正确：`CONFIG_TEST_<LAYER>_<MODULE>`
- 检查文件是否在正确的目录下

### 步骤 5: 配置并编译

```bash
# 加载包含新测试的配置
python3 build.py config tests_x86_full_defconfig

# 或者使用 menuconfig 手动启用
python3 build.py menuconfig
# 导航到: Tests -> Unit Tests -> OSAL Tests -> Test OSAL timer

# 编译项目
python3 build.py build

# 检查编译输出
ls -lh _build/bin/ems-test
```

**常见编译问题：**
- **找不到头文件**：检查 `#include` 路径是否正确
- **链接错误**：检查被测试模块是否已编译（例如 `CONFIG_OSAL=y`）
- **Kconfig 选项未定义**：检查 Kconfig 文件语法和 `source` 语句

### 步骤 6: 运行测试

```bash
# 运行新添加的测试套件
./_build/bin/ems-test -m osal_timer

# 或者运行整个层级的测试
./_build/bin/ems-test -L OSAL

# 或者使用交互式菜单
./_build/bin/ems-test -i
```

**预期输出：**

```
========================================
Running test suite: osal_timer
========================================

[RUN ] test_timer_create_success
[PASS] test_timer_create_success (5 ms)

[RUN ] test_timer_create_null_pointer
[PASS] test_timer_create_null_pointer (1 ms)

[RUN ] test_timer_start_stop
[PASS] test_timer_start_stop (12 ms)

========================================
Test Results Summary
========================================
Total:  3
Passed: 3
Failed: 0
Time:   18 ms
========================================
```

---

## 命名约定

### 文件命名

**格式：** `test_<layer>_<module>.c`

**示例：**
```
test_osal_mutex.c       # OSAL 互斥锁测试
test_osal_thread.c      # OSAL 线程测试
test_hal_can.c          # HAL CAN 测试
test_pdl_mcu.c          # PDL MCU 测试
test_prl_common.c       # PRL 通用协议测试
```

**规则：**
- 全小写
- 使用下划线分隔
- 以 `test_` 开头
- 层级名称在前（osal, hal, pdl, prl, pconfig, aconfig）
- 模块名称在后

### Kconfig 选项命名

**格式：** `CONFIG_TEST_<LAYER>_<MODULE>`

**示例：**
```
CONFIG_TEST_OSAL_MUTEX
CONFIG_TEST_OSAL_THREAD
CONFIG_TEST_HAL_CAN
CONFIG_TEST_PDL_MCU
CONFIG_TEST_PRL_COMMON
```

**规则：**
- 全大写
- 使用下划线分隔
- 以 `CONFIG_TEST_` 开头
- 必须与文件名对应

**对应关系：**
```
test_osal_mutex.c    → CONFIG_TEST_OSAL_MUTEX
test_hal_can.c       → CONFIG_TEST_HAL_CAN
test_pdl_mcu.c       → CONFIG_TEST_PDL_MCU
```

### 测试套件 ID 命名

**格式：** `<layer>_<module>`

**示例：**
```c
TEST_SUITE_REGISTER(osal_mutex, "OSAL", ...)
TEST_SUITE_REGISTER(hal_can, "HAL", ...)
TEST_SUITE_REGISTER(pdl_mcu, "PDL", ...)
```

**规则：**
- 全小写
- 使用下划线分隔
- 与文件名保持一致（去掉 `test_` 前缀）

### 测试用例命名

**格式：** `test_<feature>_<scenario>`

**示例：**
```c
TEST_CASE(test_mutex_create_success)
TEST_CASE(test_mutex_lock_timeout)
TEST_CASE(test_thread_create_null_pointer)
```

**规则：**
- 全小写
- 使用下划线分隔
- 以 `test_` 开头
- 名称应该清楚表达测试的内容
- 包含场景描述（success, fail, timeout, null_pointer 等）

---

## 最佳实践

### 1. 测试独立性

**✅ 好的做法：**
```c
TEST_CASE(test_mutex_lock)
{
    /* 每个测试创建自己的资源 */
    osal_mutex_t *mutex = NULL;
    OSAL_MutexCreate(&mutex);
    
    int32_t ret = OSAL_MutexLock(mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    
    /* 清理资源 */
    OSAL_MutexUnlock(mutex);
    OSAL_MutexDelete(mutex);
}
```

**❌ 不好的做法：**
```c
static osal_mutex_t *g_mutex;  /* 全局变量 */

TEST_CASE(test_mutex_lock)
{
    /* 依赖外部初始化的全局变量 */
    int32_t ret = OSAL_MutexLock(g_mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}
```

### 2. 资源清理

**始终清理测试中创建的资源：**

```c
TEST_CASE(test_with_resources)
{
    osal_mutex_t *mutex = NULL;
    osal_thread_t *thread = NULL;
    uint8_t *buffer = NULL;
    
    /* 创建资源 */
    OSAL_MutexCreate(&mutex);
    OSAL_ThreadCreate(&thread, thread_func, NULL);
    buffer = OSAL_Malloc(1024);
    
    /* 执行测试 */
    /* ... */
    
    /* 清理资源（即使测试失败也要清理） */
    if (mutex) OSAL_MutexDelete(mutex);
    if (thread) OSAL_ThreadDelete(thread);
    if (buffer) OSAL_Free(buffer);
}
```

### 3. 使用 Fixture 管理共享资源

如果多个测试用例需要相同的初始化：

```c
/* 全局测试上下文 */
static osal_mutex_t *g_test_mutex;
static uint8_t *g_test_buffer;

/* Setup: 在每个测试前执行 */
static void setup(void)
{
    OSAL_MutexCreate(&g_test_mutex);
    g_test_buffer = OSAL_Malloc(1024);
}

/* Teardown: 在每个测试后执行 */
static void teardown(void)
{
    if (g_test_mutex) OSAL_MutexDelete(g_test_mutex);
    if (g_test_buffer) OSAL_Free(g_test_buffer);
}

/* 测试用例使用 fixture */
static const test_case_t cases[] = {
    TEST_CASE_ENTRY_WITH_FIXTURE(test_1, setup, teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_2, setup, teardown),
};
```

### 4. 错误路径测试

**不要只测试成功路径，也要测试错误处理：**

```c
/* 成功路径 */
TEST_CASE(test_mutex_create_success)
{
    osal_mutex_t *mutex = NULL;
    int32_t ret = OSAL_MutexCreate(&mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    OSAL_MutexDelete(mutex);
}

/* 错误路径 1: 空指针 */
TEST_CASE(test_mutex_create_null_pointer)
{
    int32_t ret = OSAL_MutexCreate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 错误路径 2: 资源耗尽（如果适用） */
TEST_CASE(test_mutex_create_out_of_memory)
{
    /* 模拟内存不足的情况 */
    /* ... */
}
```

### 5. 断言粒度

**使用多个小断言，而不是一个大断言：**

**✅ 好的做法：**
```c
TEST_CASE(test_buffer_copy)
{
    uint8_t src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[10] = {0};
    
    int32_t ret = OSAL_Memcpy(dst, src, 10);
    
    /* 分别验证每个条件 */
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(1, dst[0]);
    TEST_ASSERT_EQUAL(10, dst[9]);
    TEST_ASSERT_EQUAL(0, OSAL_Memcmp(src, dst, 10));
}
```

**❌ 不好的做法：**
```c
TEST_CASE(test_buffer_copy)
{
    /* ... */
    /* 一个断言验证多个条件 */
    TEST_ASSERT_TRUE(ret == OSAL_SUCCESS && 
                     dst[0] == 1 && 
                     dst[9] == 10 && 
                     OSAL_Memcmp(src, dst, 10) == 0);
}
```

### 6. 测试元数据

**合理设置测试元数据：**

```c
/* 单元测试：快速（< 100ms） */
TEST_SUITE_REGISTER(osal_mutex, "OSAL", cases,
                    TEST_CATEGORY_UNIT,     /* 单元测试 */
                    TEST_TAG_FAST,           /* 快速 */
                    100,                     /* 100ms 超时 */
                    "OSAL mutex operations");

/* 性能测试：中等（1-5s） */
TEST_SUITE_REGISTER(osal_mutex_perf, "OSAL", cases,
                    TEST_CATEGORY_PERFORMANCE,  /* 性能测试 */
                    TEST_TAG_SLOW,              /* 慢速 */
                    5000,                       /* 5s 超时 */
                    "OSAL mutex performance");

/* 硬件测试：需要硬件 */
TEST_SUITE_REGISTER(hal_can, "HAL", cases,
                    TEST_CATEGORY_UNIT,
                    TEST_TAG_FAST | TEST_TAG_HARDWARE,  /* 组合标签 */
                    100,
                    "HAL CAN driver");
```

### 7. 清晰的测试描述

**测试描述应该清楚说明测试内容：**

```c
/* ✅ 好的描述 */
TEST_SUITE_REGISTER(osal_mutex, "OSAL", cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "OSAL mutex operations (create, lock, unlock, destroy)");

/* ❌ 不好的描述 */
TEST_SUITE_REGISTER(osal_mutex, "OSAL", cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "Mutex tests");
```

---

## 故障排除

### 问题 1: 编译时找不到测试文件

**错误信息：**
```
CMake Warning: Test file test_osal_timer.c found but CONFIG_TEST_OSAL_TIMER not defined
```

**解决方案：**
1. 检查 Kconfig 文件中是否定义了对应的配置选项
2. 检查配置选项名称是否正确（必须与文件名匹配）
3. 检查 Kconfig 文件是否被 `source` 引入
4. 重新运行配置：`python3 build.py config tests_x86_full_defconfig`

### 问题 2: 链接错误（undefined reference）

**错误信息：**
```
undefined reference to `OSAL_MutexCreate'
```

**解决方案：**
1. 检查被测试模块是否已启用（例如 `CONFIG_OSAL=y`）
2. 检查测试的依赖关系是否正确
3. 在 Kconfig 中添加 `depends on CONFIG_OSAL`

### 问题 3: 测试未出现在菜单中

**解决方案：**
1. 检查 Kconfig 选项是否启用：`grep CONFIG_TEST_OSAL_TIMER .config`
2. 检查文件是否被编译：`ls -l _build/products/tests/unit/osal/test_osal_timer.c.o`
3. 重新编译：`python3 build.py build --clean`

### 问题 4: 测试运行失败

**错误信息：**
```
[FAIL] test_mutex_create_success (assertion failed at line 25)
```

**解决方案：**
1. 检查断言条件是否正确
2. 添加调试输出：`TEST_LOG_INFO("mutex = %p", mutex);`
3. 使用 GDB 调试：`gdb --args ./_build/bin/ems-test -m osal_timer`
4. 检查资源是否正确初始化和清理

### 问题 5: Kconfig 选项未生效

**解决方案：**
1. 删除旧的配置：`rm .config`
2. 重新加载配置：`python3 build.py config tests_x86_full_defconfig`
3. 清理编译：`python3 build.py clean`
4. 重新编译：`python3 build.py build`

### 问题 6: 自动发现不工作

**解决方案：**
1. 检查文件命名是否符合规范：`test_<layer>_<module>.c`
2. 检查 CMake 缓存：`rm -rf _build/CMakeCache.txt`
3. 启用详细输出：`python3 build.py build --verbose`
4. 检查 TestDiscovery.cmake 是否正确包含

---

## 示例参考

### 完整示例

查看以下文件作为参考：

1. **最小示例**
   - `products/tests/examples/test_example_minimal.c`
   - 展示最简单的测试结构

2. **简化注册示例**
   - `products/tests/examples/test_example_simplified.c`
   - 使用 `TEST_SUITE_REGISTER` 宏

3. **带 Fixture 示例**
   - `products/tests/examples/test_example_with_fixture.c`
   - 展示 setup/teardown 的使用

4. **元数据示例**
   - `products/tests/examples/test_example_metadata.c`
   - 展示如何设置测试元数据

5. **实际测试示例**
   - `products/tests/unit/osal/test_osal_mutex.c`
   - 完整的 OSAL 互斥锁测试

### 模板文件

使用 `products/tests/examples/test_template.c` 作为起点：

```bash
# 复制模板到新位置
cp products/tests/examples/test_template.c \
   products/tests/unit/osal/test_osal_timer.c

# 编辑文件，替换以下内容：
# - MODULE_NAME → timer
# - LAYER → OSAL
# - 添加实际测试用例
```

---

## 常用命令参考

```bash
# 配置
python3 build.py config tests_x86_full_defconfig
python3 build.py menuconfig

# 编译
python3 build.py build
python3 build.py build --verbose

# 清理
python3 build.py clean      # 清理编译产物
python3 build.py distclean  # 完全清理

# 运行测试
./_build/bin/ems-test                    # 交互式菜单
./_build/bin/ems-test -a                 # 运行所有测试
./_build/bin/ems-test -L OSAL            # 运行 OSAL 层测试
./_build/bin/ems-test -m osal_mutex      # 运行特定测试套件
./_build/bin/ems-test -l                 # 列出所有测试

# 调试
gdb --args ./_build/bin/ems-test -m osal_timer
./_build/bin/ems-test -m osal_timer --verbose
```

---

## 进一步阅读

- [测试框架 README](../README.md) - 测试框架概述
- [示例目录 README](./README.md) - 所有示例的说明
- [test_template.c](./test_template.c) - 可复制的模板文件
- [test_example_minimal.c](./test_example_minimal.c) - 最小化示例
- [test_example_simplified.c](./test_example_simplified.c) - 推荐的简化方式
- [命名规范](../../../docs/NAMING_CONVENTIONS.md) - 项目命名规范
- [编码规范](../../../docs/CODING_STANDARDS.md) - 项目编码规范

---

## 获取帮助

如果遇到问题：

1. 查看本指南的[故障排除](#故障排除)部分
2. 参考[示例文件](#示例参考)
3. 检查现有测试的实现（如 `test_osal_mutex.c`）
4. 查看构建日志：`cat _build/build.log`
5. 使用详细模式编译：`python3 build.py build --verbose`

---

**祝你测试愉快！** 🎉
