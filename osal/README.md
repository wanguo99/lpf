# OSAL (操作系统抽象层)

## 模块概述

OSAL (Operating System Abstraction Layer) 提供跨平台的操作系统抽象接口，封装任务管理、进程间通信、网络、文件操作等系统调用。

**设计理念**：
- 用户态库设计，无需显式初始化
- 跨平台操作系统抽象层设计
- 支持POSIX平台（Linux/Unix）
- 为RTOS移植预留接口

## 编译说明

### 快速开始

```bash
# 在项目根目录编译整个项目（包含OSAL）
./build.sh              # Release模式
./build.sh -d           # Debug模式
```

### 单独编译OSAL模块

```bash
# 方法1: 使用CMake直接编译
mkdir -p output/build && cd output/build
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make osal -j$(nproc)
cd ../..

# 方法2: 在已配置的构建目录中编译
cd output/build
make osal -j$(nproc)
cd ../..
```

### 支持的编译参数

#### CMake配置参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `CMAKE_BUILD_TYPE` | STRING | Release | 编译类型：Release/Debug |
| `PLATFORM` | STRING | native | 目标平台：native/generic-linux |
| `CMAKE_C_COMPILER` | STRING | gcc | C编译器路径 |
| `BUILD_TESTING` | BOOL | ON | 是否编译测试 |

#### 编译类型说明

**Release模式**（默认）：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Release
```
- 优化级别：`-O2`
- 无调试信息
- 适用于生产环境

**Debug模式**：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
```
- 优化级别：`-O0`
- 包含调试信息：`-g`
- 适用于开发调试

#### 平台配置

**本地编译（默认）**：
```bash
cmake ../.. -DPLATFORM=native
```
- 使用系统默认编译器
- 适用于x86_64/ARM本地开发

**交叉编译（通用Linux）**：
```bash
cmake ../.. \
    -DPLATFORM=generic-linux \
    -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc
```
- 使用指定的交叉编译工具链
- 适用于嵌入式Linux目标

### 配置编译参数

#### 方法1: 命令行传参（推荐）

```bash
cd output/build
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPLATFORM=native \
    -DBUILD_TESTING=ON
make osal -j$(nproc)
```

#### 方法2: 使用ccmake交互式配置

```bash
cd output/build
ccmake ../..
# 按 'c' 配置，按 'g' 生成
make osal -j$(nproc)
```

#### 方法3: 修改CMakeCache.txt

```bash
cd output/build
# 编辑 CMakeCache.txt
vim CMakeCache.txt
make osal -j$(nproc)
```

### 编译输出

```
output/
├── build/
│   └── lib/
│       └── libosal.a          # OSAL静态库
└── target/
    └── bin/
        └── unit-test          # 测试程序（包含OSAL测试）
```

### 常用编译命令

```bash
# 完整编译流程
./build.sh -d                   # Debug模式编译所有

# 仅编译OSAL库
cd output/build && make osal -j$(nproc) && cd ../..

# 清理并重新编译
./build.sh -c                   # 清理
./build.sh                      # 重新编译

# 查看编译日志
cat output/build.log

# 查看编译选项
cd output/build && cmake -L ../.. && cd ../..
```

## 主要特性

- **用户态库设计**：无需显式初始化，使用静态初始化机制
- **跨平台支持**：POSIX实现，易于移植到RTOS（FreeRTOS、VxWorks等）
- **线程安全**：所有接口均为线程安全
- **优雅关闭**：任务管理支持优雅退出，避免资源泄漏
- **引用计数**：消息队列使用引用计数，防止use-after-free
- **死锁检测**：互斥锁内置5秒超时和死锁检测
- **日志轮转**：5级日志系统，支持自动轮转（10MB/5个备份）

## 模块组成

### IPC - 进程间通信
- **任务管理** (`osal_task.h`) - 任务创建、删除、优雅退出
- **消息队列** (`osal_queue.h`) - 线程安全的消息队列（引用计数）
- **互斥锁** (`osal_mutex.h`) - 互斥锁（死锁检测）
- **原子操作** (`osal_atomic.h`) - 原子操作封装

### SYS - 系统调用封装
- **时钟** (`osal_clock.h`) - 系统时钟
- **信号** (`osal_signal.h`) - 信号处理
- **文件** (`osal_file.h`) - 文件I/O
- **Select** (`osal_select.h`) - I/O多路复用
- **环境变量** (`osal_env.h`) - 环境变量操作
- **时间** (`osal_time.h`) - 时间操作

### NET - 网络抽象
- **Socket** (`osal_socket.h`) - Socket操作
- **Termios** (`osal_termios.h`) - 串口控制

### LIB - 标准库封装
- **字符串** (`osal_string.h`) - 字符串操作
- **堆内存** (`osal_heap.h`) - 内存管理
- **错误处理** (`osal_errno.h`) - 错误码和错误信息（37个标准错误码）

### UTIL - 工具类
- **日志** (`osal_log.h`) - 日志系统（5级别，自动轮转）
- **版本** - 版本信息查询

## 模块结构

```
osal/
├── include/                    # 公共接口头文件
│   ├── osal.h                  # 总头文件
│   ├── osal_types.h            # 类型定义
│   ├── sys/                    # 系统调用封装
│   ├── ipc/                    # 进程间通信
│   ├── net/                    # 网络抽象
│   ├── lib/                    # 标准库封装
│   └── util/                   # 工具函数
└── src/posix/                  # POSIX实现
    ├── ipc/                    # 任务、队列、互斥锁
    ├── sys/                    # 时钟、信号、文件
    ├── net/                    # Socket封装
    ├── lib/                    # 内存、字符串
    └── util/                   # 日志、版本
```

## 依赖关系

**OSAL依赖**：
- 系统库：`pthread`, `rt`
- 无其他模块依赖（基础层）

**被依赖**：
- HAL层：硬件抽象层
- PDL层：外设驱动层
- Apps层：应用层
- Tests层：测试框架

## 使用示例

### 在其他模块中使用OSAL

**CMakeLists.txt配置**：
```cmake
# 链接OSAL接口库（获取头文件路径）
target_link_libraries(your_module PUBLIC ems::osal_public_api)

# 链接OSAL实现库（运行时链接）
target_link_libraries(your_module PRIVATE ems::osal)
```

**代码中使用**：
```c
#include <osal.h>

int main(void)
{
    /* 无需显式初始化，直接使用OSAL接口 */
    
    /* 创建任务 */
    osal_id_t task_id;
    OSAL_TaskCreate(&task_id, "MyTask", task_entry, NULL, 
                    8192, OSAL_PRIORITY_C(100), 0);
    
    /* 日志输出 */
    LOG_INFO("MAIN", "Application started");
    
    /* 延时 */
    OSAL_TaskDelay(1000);
    
    return 0;
}
```

## 测试

```bash
# 编译测试
./build.sh -d

# 运行OSAL测试
./output/target/bin/unit-test -L OSAL      # 运行所有OSAL测试
./output/target/bin/unit-test -m test_osal_task   # 运行任务测试
./output/target/bin/unit-test -i           # 交互式菜单
```

## 开发指南

### 添加新的OSAL接口

1. 在 `include/` 对应子目录添加接口声明
2. 在 `src/posix/` 对应子目录添加实现
3. 在 `CMakeLists.txt` 的 `OSAL_SOURCES` 中添加源文件
4. 在 `tests/src/osal/` 添加单元测试

### 移植到新平台

1. 创建新的平台目录：`src/<platform>/`
2. 实现OSAL接口（参考 `src/posix/` 实现）
3. 修改 `CMakeLists.txt` 支持新平台
4. 运行测试验证移植

## 常见问题

**Q: 编译时找不到pthread库？**
```bash
# 确保安装了pthread开发库
sudo apt-get install libpthread-stubs0-dev
```

**Q: 如何查看OSAL版本？**
```c
const char *version = OS_GetVersionString();
OSAL_Printf("OSAL Version: %s\n", version);
```

**Q: 如何启用更多编译警告？**
```bash
# 修改 osal/CMakeLists.txt
target_compile_options(osal PRIVATE
    -Wall -Wextra -Wpedantic
)
```

## 设计原则

1. **平台无关性**：OSAL是唯一允许包含系统头文件的层
2. **接口稳定性**：API保持向后兼容
3. **资源管理**：所有资源必须显式释放
4. **错误处理**：所有函数返回int32状态码
5. **线程安全**：所有接口均为线程安全

## 参考文档

- [OSAL详细文档](docs/README.md)
- [架构设计](docs/ARCHITECTURE.md)
- [API参考](docs/API_REFERENCE.md)
- [使用指南](docs/USAGE_GUIDE.md)
- [编码规范](../docs/CODING_STANDARDS.md)
## 参考资料

- POSIX标准文档
- Linux系统编程手册
- 嵌入式系统设计模式
