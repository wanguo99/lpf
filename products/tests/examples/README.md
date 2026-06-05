# 测试框架示例

本目录包含 EMS 测试框架的使用示例，展示如何使用简化的 API 编写测试用例。

## 文件说明

| 文件 | 说明 | 推荐度 |
|------|------|--------|
| `test_example_simplified.c` | **推荐**：使用简化宏的标准示例 | ⭐⭐⭐⭐⭐ |
| `test_example_with_fixture.c` | 带 fixture（setup/teardown）的示例 | ⭐⭐⭐⭐ |
| `test_example_simple_auto.c` | 过渡方案：手动数组 + 自动注册 | ⭐⭐⭐ |
| `test_example_minimal.c` | 最小化示例（包含理论设计） | ⭐⭐ |

## 快速开始

### 1. 最简单的测试（推荐方式）

```c
#include "test_framework.h"
#include "osal.h"

/* 定义测试用例 */
TEST_CASE(test_mutex_create)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    TEST_ASSERT_NOT_NULL(mutex);
    OSAL_Mutex_Delete(mutex);
}

TEST_CASE(test_mutex_lock)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    int32_t ret = OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(OSAL_OK, ret);
    OSAL_Mutex_Unlock(mutex);
    OSAL_Mutex_Delete(mutex);
}

/* 创建测试用例数组 */
static const test_case_t osal_mutex_cases[] = {
    TEST_CASE_ENTRY(test_mutex_create),
    TEST_CASE_ENTRY(test_mutex_lock),
};

/* 注册测试套件（一行搞定） */
TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "OSAL mutex operations");
```

**就这么简单！**相比传统方式减少了 60% 的样板代码。

### 2. 带 Fixture 的测试

```c
#include "test_framework.h"
#include "osal.h"

/* 全局测试上下文 */
static OSAL_Mutex_Handle_t g_mutex;
static uint8_t *g_buffer;

/* Setup: 在每个测试前执行 */
static void setup(void)
{
    g_mutex = OSAL_Mutex_Create();
    g_buffer = OSAL_Malloc(1024);
    OSAL_Memset(g_buffer, 0, 1024);
}

/* Teardown: 在每个测试后执行 */
static void teardown(void)
{
    if (g_mutex) OSAL_Mutex_Delete(g_mutex);
    if (g_buffer) OSAL_Free(g_buffer);
}

TEST_CASE(test_with_fixture)
{
    /* setup() 已经初始化好了资源 */
    TEST_ASSERT_NOT_NULL(g_mutex);
    TEST_ASSERT_NOT_NULL(g_buffer);
    
    /* 进行测试 */
    // ...
    
    /* teardown() 会自动清理资源 */
}

/* 使用 fixture 注册 */
static const test_case_t my_cases[] = {
    TEST_CASE_ENTRY_WITH_FIXTURE(test_with_fixture, setup, teardown),
};

TEST_SUITE_REGISTER(my_suite, "OSAL", my_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "Test with fixture");
```

## API 对比

### 传统方式（不推荐）

```c
/* ❌ 需要 ~25 行样板代码 */
TEST_MODULE_BEGIN(test_osal_mutex, "OSAL")
TEST_CASE_REGISTER(test_mutex_create, "")
TEST_CASE_REGISTER(test_mutex_lock, "")
TEST_CASE_REGISTER(test_mutex_unlock, "")
TEST_CASE_REGISTER(test_mutex_delete, "")
TEST_SUITE_METADATA(test_osal_mutex, 
                    TEST_CATEGORY_UNIT, 
                    TEST_TAG_FAST, 
                    100, 
                    "OSAL mutex tests")
TEST_MODULE_END(test_osal_mutex, "OSAL")
```

### 简化方式（推荐）

```c
/* ✅ 只需 ~10 行 */
static const test_case_t osal_mutex_cases[] = {
    TEST_CASE_ENTRY(test_mutex_create),
    TEST_CASE_ENTRY(test_mutex_lock),
    TEST_CASE_ENTRY(test_mutex_unlock),
    TEST_CASE_ENTRY(test_mutex_delete),
};

TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "OSAL mutex tests");
```

## 可用的宏

### 测试用例定义

```c
TEST_CASE(test_name)                    /* 定义测试用例 */
```

### 测试用例数组

```c
TEST_CASE_ENTRY(test_func)              /* 基本测试用例 */
TEST_CASE_ENTRY_WITH_FIXTURE(           /* 带 fixture 的测试用例 */
    test_func, 
    setup_func, 
    teardown_func
)
```

### 测试套件注册

```c
/* 完整注册（推荐） */
TEST_SUITE_REGISTER(
    suite_id,                           /* 套件标识符 */
    "LAYER",                            /* 层级名称 */
    cases_array,                        /* 测试用例数组 */
    TEST_CATEGORY_UNIT,                 /* 测试分类 */
    TEST_TAG_FAST,                      /* 标签 */
    timeout_ms,                         /* 超时时间 */
    "description"                       /* 描述 */
)

/* 分步注册（高级用法） */
TEST_SUITE_REGISTER_CASES(
    suite_id, 
    "LAYER", 
    cases_array, 
    metadata
)
```

### 元数据模板

```c
TEST_METADATA_DEFAULT_UNIT(desc)        /* 单元测试（1s 超时） */
TEST_METADATA_DEFAULT_PERF(desc)        /* 性能测试（5s 超时） */
TEST_METADATA_DEFAULT_STRESS(desc)      /* 压力测试（10s 超时） */
TEST_METADATA_DEFAULT_SYSTEM(desc)      /* 系统测试（5s 超时） */
```

## 测试分类和标签

### 测试分类（category）

- `TEST_CATEGORY_UNIT` - 单元测试（默认）
- `TEST_CATEGORY_PERFORMANCE` - 性能测试
- `TEST_CATEGORY_STRESS` - 压力测试
- `TEST_CATEGORY_SYSTEM` - 系统集成测试

### 测试标签（tags）

- `TEST_TAG_FAST` - 快速测试（< 100ms）
- `TEST_TAG_SLOW` - 慢速测试（> 1s）
- `TEST_TAG_HARDWARE` - 需要硬件支持
- `TEST_TAG_NETWORK` - 需要网络连接

标签可以使用 `|` 组合：

```c
TEST_TAG_SLOW | TEST_TAG_HARDWARE
```

## 常用断言

```c
/* 相等性断言 */
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_NOT_EQUAL(expected, actual)

/* 布尔断言 */
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)

/* 指针断言 */
TEST_ASSERT_NULL(ptr)
TEST_ASSERT_NOT_NULL(ptr)

/* 字符串断言 */
TEST_ASSERT_STR_EQUAL(expected, actual)
TEST_ASSERT_STR_NOT_EQUAL(expected, actual)
```

## 添加新测试的步骤

### 步骤 1：创建测试文件

文件命名规范：`test_<layer>_<module>.c`

例如：`test_osal_mutex.c`、`test_hal_can.c`

### 步骤 2：编写测试用例

```c
#include "test_framework.h"
#include "<被测试的模块>.h"

TEST_CASE(test_feature_1)
{
    /* 测试代码 */
    TEST_ASSERT_EQUAL(expected, actual);
}

TEST_CASE(test_feature_2)
{
    /* 测试代码 */
}
```

### 步骤 3：创建测试用例数组

```c
static const test_case_t my_module_cases[] = {
    TEST_CASE_ENTRY(test_feature_1),
    TEST_CASE_ENTRY(test_feature_2),
};
```

### 步骤 4：注册测试套件

```c
TEST_SUITE_REGISTER(my_module, "LAYER", my_module_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "Description of my module tests");
```

### 步骤 5：添加 Kconfig 配置

在 `products/tests/unit/<layer>/Kconfig` 中添加：

```kconfig
config TEST_<LAYER>_<MODULE>
    bool "Test <layer> <module>"
    depends on CONFIG_<LAYER>
    default y
    help
      Test <layer> <module> functionality.
      Runtime: <100ms
      Hardware: None
```

### 步骤 6：编译和运行

```bash
# 配置（如果新增了 Kconfig 选项）
python3 build.py config tests_x86_full_defconfig

# 编译
python3 build.py build

# 运行测试
./_build/bin/ems-test
```

## 迁移现有测试

将传统的 `TEST_MODULE_BEGIN/END` 方式迁移到简化方式：

### 迁移前

```c
TEST_MODULE_BEGIN(test_osal_mutex, "OSAL")
TEST_CASE_REGISTER(test_mutex_create, "")
TEST_CASE_REGISTER(test_mutex_lock, "")
TEST_SUITE_METADATA(test_osal_mutex, 
                    TEST_CATEGORY_UNIT, 
                    TEST_TAG_FAST, 
                    100, 
                    "Mutex tests")
TEST_MODULE_END(test_osal_mutex, "OSAL")
```

### 迁移后

```c
static const test_case_t osal_mutex_cases[] = {
    TEST_CASE_ENTRY(test_mutex_create),
    TEST_CASE_ENTRY(test_mutex_lock),
};

TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
                    "Mutex tests");
```

### 迁移步骤

1. 保持所有 `TEST_CASE()` 定义不变
2. 删除 `TEST_MODULE_BEGIN` 和 `TEST_MODULE_END` 行
3. 创建 `test_case_t` 数组，使用 `TEST_CASE_ENTRY()` 列出所有测试
4. 将 `TEST_SUITE_METADATA()` 的参数移到 `TEST_SUITE_REGISTER()` 中
5. 编译验证

## 最佳实践

### 1. 测试独立性

每个测试应该能够独立运行，不依赖其他测试的执行顺序。

```c
/* ✅ 好的做法：每个测试创建自己的资源 */
TEST_CASE(test_mutex_lock)
{
    OSAL_Mutex_Handle_t mutex = OSAL_Mutex_Create();
    OSAL_Mutex_Lock(mutex, OSAL_WAIT_FOREVER);
    OSAL_Mutex_Unlock(mutex);
    OSAL_Mutex_Delete(mutex);
}

/* ❌ 不好的做法：依赖全局状态 */
static OSAL_Mutex_Handle_t g_shared_mutex;
TEST_CASE(test_mutex_lock)
{
    OSAL_Mutex_Lock(g_shared_mutex, OSAL_WAIT_FOREVER);  /* 假设已初始化 */
}
```

### 2. 使用 Fixture 管理资源

如果多个测试需要相同的初始化，使用 fixture：

```c
static void setup(void) { /* 初始化 */ }
static void teardown(void) { /* 清理 */ }

static const test_case_t cases[] = {
    TEST_CASE_ENTRY_WITH_FIXTURE(test_1, setup, teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_2, setup, teardown),
};
```

### 3. 清晰的测试命名

测试名称应该清楚地表达测试的内容：

```c
/* ✅ 好的命名 */
TEST_CASE(test_mutex_create_success)
TEST_CASE(test_mutex_lock_timeout)
TEST_CASE(test_mutex_recursive_locking)

/* ❌ 不好的命名 */
TEST_CASE(test_1)
TEST_CASE(test_mutex)
TEST_CASE(test_case_a)
```

### 4. 适当的断言粒度

```c
/* ✅ 好的做法：每个断言验证一个具体的条件 */
TEST_CASE(test_buffer_copy)
{
    uint8_t src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[10] = {0};
    
    OSAL_Memcpy(dst, src, 10);
    
    TEST_ASSERT_EQUAL(1, dst[0]);
    TEST_ASSERT_EQUAL(10, dst[9]);
    TEST_ASSERT_EQUAL(0, OSAL_Memcmp(src, dst, 10));
}

/* ❌ 不好的做法：一个断言验证太多 */
TEST_CASE(test_buffer_copy)
{
    /* ... */
    TEST_ASSERT_TRUE(dst[0] == 1 && dst[9] == 10 && memcmp(src, dst, 10) == 0);
}
```

### 5. 合理的超时设置

根据测试类型设置合理的超时：

```c
/* 单元测试：快速（< 100ms） */
TEST_SUITE_REGISTER(unit_tests, "OSAL", cases,
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "...");

/* 性能测试：中等（1-5s） */
TEST_SUITE_REGISTER(perf_tests, "OSAL", cases,
                    TEST_CATEGORY_PERFORMANCE, TEST_TAG_SLOW, 5000, "...");

/* 压力测试：较长（10s+） */
TEST_SUITE_REGISTER(stress_tests, "OSAL", cases,
                    TEST_CATEGORY_STRESS, TEST_TAG_SLOW, 10000, "...");
```

## 常见问题

### Q: 为什么需要手动创建测试用例数组？

A: 由于 C 预处理器的限制，无法在编译时自动收集 `TEST_CASE()` 定义。未来版本可能通过 linker section 或构建脚本实现完全自动化。当前的手动数组方式已经比传统方式简化了很多。

### Q: 可以混用传统方式和简化方式吗？

A: 可以。两种方式可以共存，但建议逐步迁移到简化方式以获得更好的一致性。

### Q: 如何在测试中打印调试信息？

A: 使用测试框架提供的日志宏：

```c
TEST_LOG_INFO("Informational message");
TEST_LOG_WARNING("Warning message");
TEST_LOG_ERROR("Error message");
```

### Q: 测试失败后会继续运行吗？

A: 默认情况下，单个测试用例失败不会影响其他测试用例的执行。测试框架会继续运行所有测试，并在最后报告统计信息。

## 参考资料

- [测试框架核心文档](../README.md)
- [断言宏列表](../include/test_assert.h)
- [元数据定义](../include/test_metadata.h)
- [Kconfig 配置指南](../../../docs/KCONFIG_GUIDE.md)
