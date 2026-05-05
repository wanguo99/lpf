# Tests 模块优化计划书

## 1. 概述

本文档描述了 EMS 项目 Tests 模块的优化方案，主要目标是：
1. 统一可执行文件命名为 `ems-test`
2. 采用 Busybox 风格的软链接架构，简化测试调用方式
3. 明确区分单元测试、系统测试和压力测试

## 2. 当前架构分析

### 2.1 现状
- **可执行文件名**: `unit-test`
- **测试组织**: 基于 Unity 测试框架，使用 `-s`, `-L`, `-m` 等参数指定模块
- **测试分类**: 未明确区分测试类型
- **调用方式**: `./unit-test -s osal_read` 或 `./unit-test -L` 列出所有测试

### 2.2 存在的问题
1. 可执行文件名不够直观，未体现项目名称
2. 参数形式复杂，需要记忆多个选项
3. 缺乏对不同测试类型的明确分类和管理
4. 无法通过软链接快速调用特定模块的测试

## 3. 优化方案

### 3.1 可执行文件重命名

**目标**: 将 `unit-test` 重命名为 `ems-test`

**实施步骤**:
1. 修改 `tests/CMakeLists.txt`，将可执行文件名从 `unit-test` 改为 `ems-test`
2. 更新所有文档中的引用
3. 更新 CI/CD 脚本中的测试命令

**影响范围**:
- `tests/CMakeLists.txt`
- `tests/README.md`
- `.github/workflows/` (如果存在)
- 开发者文档

### 3.2 Busybox 风格架构设计

#### 3.2.1 Busybox 原理回顾

Busybox 通过以下机制实现多命令共享单一可执行文件：
1. **argv[0] 检测**: 根据程序被调用时的名称（argv[0]）决定执行哪个命令
2. **软链接**: 创建多个软链接指向同一个可执行文件
3. **命令分发**: 内部维护命令表，根据 argv[0] 分发到对应的处理函数

#### 3.2.2 测试框架架构设计

**核心思想**: 
- `ems-test` 作为主可执行文件
- 为每个测试模块创建软链接（如 `osal-test`, `hal-test`, `pdl-test`）
- 根据 argv[0] 自动识别要测试的模块

**软链接命名规范**:
```
osal-test -> ems-test    # OSAL 层测试
hal-test  -> ems-test    # HAL 层测试
pdl-test  -> ems-test    # PDL 层测试
app-test  -> ems-test    # 应用层测试
```

**命令行接口设计**:

```bash
# 方式 1: 通过软链接调用
./osal-test --suite osal_read    # 测试 osal_read 接口
./osal-test --all                # 测试 OSAL 所有接口
./hal-test --suite gpio_init     # 测试 gpio_init 接口

# 方式 2: 直接调用主程序
./ems-test --module osal --suite osal_read
./ems-test --module hal --all

# 方式 3: 列出可用测试
./osal-test --list               # 列出 OSAL 所有测试用例
./ems-test --list-modules        # 列出所有模块
```

**参数简化**:
- `--suite` / `-s`: 指定测试套件（替代原 `-s`）
- `--all` / `-a`: 运行所有测试（替代原 `-L`）
- `--list` / `-l`: 列出可用测试
- `--module` / `-m`: 指定模块（仅在直接调用 ems-test 时需要）
- `--verbose` / `-v`: 详细输出
- `--type` / `-t`: 指定测试类型（unit/system/stress）

#### 3.2.3 代码结构设计

```c
// tests/core/main.c

#include <string.h>
#include <libgen.h>

// 模块枚举
typedef enum {
    MODULE_UNKNOWN = 0,
    MODULE_OSAL,
    MODULE_HAL,
    MODULE_PDL,
    MODULE_APP
} TestModule;

// 根据 argv[0] 识别模块
TestModule detect_module_from_argv0(const char *argv0) {
    char *prog_name = basename((char *)argv0);
    
    if (strcmp(prog_name, "osal-test") == 0) return MODULE_OSAL;
    if (strcmp(prog_name, "hal-test") == 0) return MODULE_HAL;
    if (strcmp(prog_name, "pdl-test") == 0) return MODULE_PDL;
    if (strcmp(prog_name, "app-test") == 0) return MODULE_APP;
    
    return MODULE_UNKNOWN;
}

// 主入口
int main(int argc, char *argv[]) {
    TestModule module = detect_module_from_argv0(argv[0]);
    
    // 如果通过 ems-test 直接调用，需要从参数中获取模块
    if (module == MODULE_UNKNOWN) {
        module = parse_module_from_args(argc, argv);
    }
    
    // 解析其他参数
    TestConfig config = parse_test_args(argc, argv);
    
    // 根据模块分发测试
    switch (module) {
        case MODULE_OSAL:
            return run_osal_tests(&config);
        case MODULE_HAL:
            return run_hal_tests(&config);
        case MODULE_PDL:
            return run_pdl_tests(&config);
        case MODULE_APP:
            return run_app_tests(&config);
        default:
            print_usage();
            return 1;
    }
}
```

### 3.3 测试分类体系

#### 3.3.1 测试类型定义

**单元测试 (Unit Test)**:
- **定义**: 测试单个函数或模块的功能正确性
- **特点**: 
  - 快速执行（毫秒级）
  - 无外部依赖（或使用 mock）
  - 覆盖边界条件和异常情况
- **示例**: 
  - `osal_read()` 函数的参数验证
  - `osal_mutex_lock()` 的基本功能测试

**系统测试 (System Test)**:
- **定义**: 测试多个模块协同工作的集成场景
- **特点**:
  - 涉及真实的系统资源（文件、网络、进程）
  - 执行时间较长（秒级）
  - 验证端到端的功能流程
- **示例**:
  - 多线程并发读写文件
  - 进程间通信完整流程
  - 网络 Socket 通信测试

**压力测试 (Stress Test)**:
- **定义**: 测试系统在高负载下的稳定性和性能
- **特点**:
  - 长时间运行（分钟到小时级）
  - 高并发、大数据量
  - 监控资源使用（内存、CPU、文件描述符）
- **示例**:
  - 1000 次循环创建/销毁线程
  - 10000 次文件读写操作
  - 内存泄漏检测

#### 3.3.2 目录结构设计

```
tests/
├── unit/                    # 单元测试
│   ├── osal/
│   │   ├── test_osal_read.c
│   │   ├── test_osal_mutex.c
│   │   └── ...
│   ├── hal/
│   └── pdl/
│
├── system/                  # 系统测试
│   ├── osal/
│   │   ├── test_ipc_integration.c
│   │   ├── test_multithread.c
│   │   └── ...
│   ├── hal/
│   └── pdl/
│
├── stress/                  # 压力测试
│   ├── osal/
│   │   ├── test_thread_stress.c
│   │   ├── test_memory_leak.c
│   │   └── ...
│   ├── hal/
│   └── pdl/
│
├── core/                    # 测试框架核心
│   ├── main.c              # 主入口（Busybox 风格）
│   ├── test_runner.c       # 测试运行器
│   ├── test_config.c       # 配置解析
│   └── test_report.c       # 测试报告生成
│
├── fixtures/                # 测试夹具和辅助工具
│   ├── mock/               # Mock 对象
│   └── utils/              # 测试工具函数
│
├── docs/                    # 文档
│   ├── OPTIMIZATION_PLAN.md
│   ├── TEST_GUIDE.md
│   └── API_REFERENCE.md
│
└── CMakeLists.txt          # 构建配置
```

#### 3.3.3 测试命名规范

**文件命名**:
- 单元测试: `test_<module>_<function>.c`
- 系统测试: `test_<scenario>_integration.c`
- 压力测试: `test_<module>_stress.c`

**测试用例命名**:
- 单元测试: `test_<function>_<condition>`
  - 例: `test_osal_read_null_buffer`, `test_osal_read_zero_size`
- 系统测试: `test_<scenario>_<aspect>`
  - 例: `test_multithread_file_access`, `test_ipc_message_passing`
- 压力测试: `test_<module>_<load_type>`
  - 例: `test_thread_creation_1000_times`, `test_memory_allocation_stress`

### 3.4 CMake 构建配置

```cmake
# tests/CMakeLists.txt

# 设置可执行文件名
set(TEST_EXECUTABLE_NAME "ems-test")

# 收集不同类型的测试源文件
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/**/*.c")
file(GLOB_RECURSE SYSTEM_TEST_SOURCES "system/**/*.c")
file(GLOB_RECURSE STRESS_TEST_SOURCES "stress/**/*.c")
file(GLOB CORE_SOURCES "core/*.c")

# 创建可执行文件
add_executable(${TEST_EXECUTABLE_NAME}
    ${CORE_SOURCES}
    ${UNIT_TEST_SOURCES}
    ${SYSTEM_TEST_SOURCES}
    ${STRESS_TEST_SOURCES}
)

# 链接依赖库
target_link_libraries(${TEST_EXECUTABLE_NAME}
    osal
    unity
    pthread
)

# 安装可执行文件
install(TARGETS ${TEST_EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
)

# 创建软链接
install(CODE "
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${TEST_EXECUTABLE_NAME}
        \${CMAKE_INSTALL_PREFIX}/bin/osal-test
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${TEST_EXECUTABLE_NAME}
        \${CMAKE_INSTALL_PREFIX}/bin/hal-test
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${TEST_EXECUTABLE_NAME}
        \${CMAKE_INSTALL_PREFIX}/bin/pdl-test
    )
")
```

## 4. 实施计划

### 4.1 阶段划分

**阶段 1: 准备阶段（1-2 天）**
- [ ] 创建新的目录结构（unit/system/stress）
- [ ] 设计测试框架核心代码结构
- [ ] 编写参数解析和模块分发逻辑
- [ ] 更新 CMakeLists.txt 配置

**阶段 2: 迁移阶段（3-5 天）**
- [ ] 将现有测试用例分类到 unit/system/stress 目录
- [ ] 重构测试用例，符合新的命名规范
- [ ] 实现 Busybox 风格的主入口函数
- [ ] 创建软链接安装脚本

**阶段 3: 验证阶段（1-2 天）**
- [ ] 编译新的 ems-test 可执行文件
- [ ] 验证软链接调用功能
- [ ] 运行所有测试用例，确保功能正常
- [ ] 性能测试，确保无明显性能退化

**阶段 4: 文档和清理（1 天）**
- [ ] 更新 README.md 和使用文档
- [ ] 编写测试编写指南
- [ ] 清理旧的测试代码和配置
- [ ] 更新 CI/CD 脚本

### 4.2 时间表

| 阶段 | 任务 | 预计时间 | 负责人 |
|------|------|----------|--------|
| 阶段 1 | 目录结构和框架设计 | 1-2 天 | 开发团队 |
| 阶段 2 | 测试用例迁移和重构 | 3-5 天 | 开发团队 |
| 阶段 3 | 验证和测试 | 1-2 天 | QA 团队 |
| 阶段 4 | 文档和清理 | 1 天 | 开发团队 |
| **总计** | | **6-10 天** | |

### 4.3 风险评估

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|----------|
| 现有测试用例迁移出错 | 高 | 中 | 保留旧代码备份，逐步迁移并验证 |
| 软链接在某些平台不支持 | 中 | 低 | 提供 --module 参数作为备选方案 |
| 性能退化 | 中 | 低 | 在阶段 3 进行性能基准测试 |
| 文档更新不及时 | 低 | 中 | 将文档更新纳入阶段 4 的必须任务 |

## 5. 兼容性考虑

### 5.1 向后兼容

为了平滑过渡，建议在初期保留对旧参数的支持：

```bash
# 新方式（推荐）
./osal-test --suite osal_read

# 旧方式（兼容）
./ems-test -s osal_read -m osal
```

在代码中添加兼容层：
```c
// 检测旧参数格式
if (has_old_style_args(argc, argv)) {
    fprintf(stderr, "Warning: Old-style arguments are deprecated. "
                    "Please use new syntax.\n");
    config = parse_old_style_args(argc, argv);
} else {
    config = parse_new_style_args(argc, argv);
}
```

### 5.2 迁移指南

为开发者提供迁移指南：

**旧命令 → 新命令映射**:
```
./unit-test -L                    → ./ems-test --list-modules
./unit-test -s osal_read          → ./osal-test --suite osal_read
./unit-test -m osal               → ./osal-test --all
./unit-test -s osal_read -v       → ./osal-test --suite osal_read --verbose
```

## 6. 预期收益

### 6.1 用户体验改进
- **更直观的命名**: `ems-test` 明确表明这是 EMS 项目的测试工具
- **更简洁的调用**: 通过软链接，无需记忆复杂的参数组合
- **更清晰的分类**: 单元测试、系统测试、压力测试一目了然

### 6.2 开发效率提升
- **快速定位**: 通过 `osal-test`, `hal-test` 等软链接快速运行特定模块测试
- **灵活扩展**: 新增模块时，只需添加新的软链接和测试目录
- **统一管理**: 所有测试共享同一个可执行文件，便于维护

### 6.3 测试质量提升
- **明确的测试分类**: 有助于制定不同的测试策略和覆盖率目标
- **更好的组织**: 目录结构清晰，便于查找和维护测试用例
- **压力测试支持**: 专门的压力测试目录，便于进行长时间稳定性测试

## 7. 后续优化方向

### 7.1 短期优化（1-3 个月）
- 添加测试覆盖率统计功能
- 集成 Valgrind 进行内存泄漏检测
- 支持测试结果导出（JSON/XML 格式）

### 7.2 中期优化（3-6 个月）
- 实现并行测试执行，提升测试速度
- 添加测试依赖管理，支持测试用例间的依赖关系
- 集成持续集成（CI）系统，自动运行测试

### 7.3 长期优化（6-12 个月）
- 开发图形化测试报告界面
- 支持远程测试执行和结果收集
- 实现测试用例自动生成工具

## 8. 参考资料

### 8.1 Busybox 相关
- [Busybox 官方文档](https://busybox.net/)
- [Busybox 源码分析](https://github.com/mirror/busybox)

### 8.2 测试框架
- [Unity 测试框架](https://github.com/ThrowTheSwitch/Unity)
- [Google Test 文档](https://google.github.io/googletest/)

### 8.3 测试最佳实践
- [单元测试最佳实践](https://martinfowler.com/bliki/UnitTest.html)
- [测试金字塔理论](https://martinfowler.com/articles/practical-test-pyramid.html)

## 9. 附录

### 9.1 示例：完整的测试调用流程

```bash
# 1. 编译测试
cd build
cmake --preset linux-debug
cmake --build . --target ems-test

# 2. 安装（创建软链接）
cmake --install . --prefix ../install

# 3. 运行测试
cd ../install/bin

# 列出所有模块
./ems-test --list-modules

# 列出 OSAL 所有测试
./osal-test --list

# 运行 OSAL 单元测试
./osal-test --type unit --all

# 运行特定测试套件
./osal-test --suite osal_read --verbose

# 运行系统测试
./osal-test --type system --all

# 运行压力测试（长时间）
./osal-test --type stress --suite thread_stress
```

### 9.2 示例：测试用例编写模板

```c
// tests/unit/osal/test_osal_read.c

#include "unity.h"
#include "osal.h"

void setUp(void) {
    // 每个测试前的初始化
}

void tearDown(void) {
    // 每个测试后的清理
}

// 测试用例 1: 空指针检查
void test_osal_read_null_buffer(void) {
    int fd = 0;
    ssize_t result = OSAL_Read(fd, NULL, 100);
    TEST_ASSERT_EQUAL(-1, result);
}

// 测试用例 2: 零长度读取
void test_osal_read_zero_size(void) {
    int fd = 0;
    char buffer[100];
    ssize_t result = OSAL_Read(fd, buffer, 0);
    TEST_ASSERT_EQUAL(0, result);
}

// 测试用例 3: 正常读取
void test_osal_read_normal(void) {
    // 创建临时文件
    int fd = open("/tmp/test_file", O_RDWR | O_CREAT, 0644);
    TEST_ASSERT_NOT_EQUAL(-1, fd);
    
    // 写入测试数据
    const char *test_data = "Hello, World!";
    write(fd, test_data, strlen(test_data));
    lseek(fd, 0, SEEK_SET);
    
    // 读取并验证
    char buffer[100] = {0};
    ssize_t result = OSAL_Read(fd, buffer, strlen(test_data));
    TEST_ASSERT_EQUAL(strlen(test_data), result);
    TEST_ASSERT_EQUAL_STRING(test_data, buffer);
    
    // 清理
    close(fd);
    unlink("/tmp/test_file");
}

// 主函数（由测试框架调用）
int run_osal_read_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_osal_read_null_buffer);
    RUN_TEST(test_osal_read_zero_size);
    RUN_TEST(test_osal_read_normal);
    return UNITY_END();
}
```

---

**文档版本**: v1.0  
**创建日期**: 2026-05-XX  
**最后更新**: 2026-05-XX  
**维护者**: EMS 开发团队
