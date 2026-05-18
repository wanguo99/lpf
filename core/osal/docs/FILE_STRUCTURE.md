# OSAL 文件结构

## 目录组织

OSAL 按功能模块划分为 5 个子目录，头文件和源文件严格对应：

```
osal/
├── include/              # 公共头文件（平台无关）
│   ├── ipc/             # 进程间通信
│   ├── sys/             # 系统调用封装
│   ├── net/             # 网络通信
│   ├── lib/             # 标准库封装
│   ├── util/            # 工具模块
│   ├── osal.h           # 主头文件（包含所有子模块）
│   ├── osal_types.h     # 类型定义
│   └── osal_platform.h  # 平台检测
│
└── src/
    └── posix/           # POSIX 平台实现
        ├── ipc/         # 进程间通信实现
        ├── sys/         # 系统调用实现
        ├── net/         # 网络通信实现
        ├── lib/         # 标准库实现
        └── util/        # 工具模块实现
```

## 功能模块详解

### 1. IPC - 进程间通信（Inter-Process Communication）

**功能**：提供线程同步和原子操作原语

| 头文件 | 源文件 | 功能 |
|--------|--------|------|
| `ipc/osal_mutex.h` | `posix/ipc/osal_mutex.c` | 互斥锁（Mutex） |
| `ipc/osal_semaphore.h` | `posix/ipc/osal_semaphore.c` | 信号量（Semaphore） |
| `ipc/osal_cond.h` | `posix/ipc/osal_cond.c` | 条件变量（Condition Variable） |
| `ipc/osal_atomic.h` | `posix/ipc/osal_atomic.c` | 原子操作（Atomic Operations） |

**典型用途**：
- 多线程资源保护
- 生产者-消费者模式
- 无锁数据结构

### 2. SYS - 系统调用封装（System Calls）

**功能**：封装 POSIX 系统调用，提供平台无关接口

| 头文件 | 源文件 | 功能 |
|--------|--------|------|
| `sys/osal_thread.h` | `posix/sys/osal_thread.c` | 线程管理（Thread） |
| `sys/osal_process.h` | `posix/sys/osal_process.c` | 进程管理（Process） |
| `sys/osal_time.h` | `posix/sys/osal_time.c` | 时间操作（Time） |
| `sys/osal_clock.h` | `posix/sys/osal_clock.c` | 时钟操作（Clock） |
| `sys/osal_signal.h` | `posix/sys/osal_signal.c` | 信号处理（Signal） |
| `sys/osal_file.h` | `posix/sys/osal_file.c` | 文件操作（File I/O） |
| `sys/osal_select.h` | `posix/sys/osal_select.c` | I/O 多路复用（Select/Poll） |
| `sys/osal_env.h` | `posix/sys/osal_env.c` | 环境变量（Environment） |

**典型用途**：
- 任务调度和管理
- 定时器和延时
- 文件读写
- 信号处理

### 3. NET - 网络通信（Network）

**功能**：封装网络和串口通信接口

| 头文件 | 源文件 | 功能 |
|--------|--------|------|
| `net/osal_socket.h` | `posix/net/osal_socket.c` | Socket 网络编程 |
| `net/osal_termios.h` | `posix/net/osal_termios.c` | 串口终端控制（Termios） |

**典型用途**：
- TCP/UDP 通信
- CAN 总线通信（使用 SocketCAN）
- 串口配置和通信

### 4. LIB - 标准库封装（Standard Library）

**功能**：封装 C 标准库函数，确保类型安全和平台一致性

| 头文件 | 源文件 | 功能 |
|--------|--------|------|
| `lib/osal_string.h` | `posix/lib/osal_string.c` | 字符串操作（String） |
| `lib/osal_stdio.h` | `posix/lib/osal_stdio.c` | 标准输入输出（Stdio） |
| `lib/osal_heap.h` | `posix/lib/osal_heap.c` | 内存管理（Heap） |
| `lib/osal_errno.h` | `posix/lib/osal_errno.c` | 错误码（Errno） |
| `lib/osal_error.h` | `posix/lib/osal_error.c` | 错误处理（Error） |

**典型用途**：
- 字符串处理（`OSAL_Strlen`, `OSAL_Strcpy`, `OSAL_Memcpy`）
- 格式化输出（`OSAL_Printf`, `OSAL_Snprintf`）
- 动态内存分配（`OSAL_Malloc`, `OSAL_Free`）
- 错误码转换

### 5. UTIL - 工具模块（Utilities）

**功能**：提供日志、版本等辅助功能

| 头文件 | 源文件 | 功能 |
|--------|--------|------|
| `util/osal_log.h` | `posix/util/osal_log.c` | 日志系统（Logging） |
| `util/osal_version.h` | `posix/util/osal_version.c` | 版本信息（Version） |

**典型用途**：
- 分级日志输出（`LOG_INFO`, `LOG_ERROR`, `LOG_DEBUG`）
- 版本查询（`OSAL_GetVersionString`）

## 核心头文件

### osal.h - 主头文件

包含所有子模块头文件，上层代码只需 `#include <osal.h>` 即可使用全部功能。

```c
#include <osal.h>

int main(void)
{
    /* 所有 OSAL API 都可用 */
    osal_id_t mutex;
    OSAL_MutexCreate(&mutex, "test", 0);
    
    LOG_INFO("APP", "OSAL version: %s", OSAL_GetVersionString());
    
    return 0;
}
```

### osal_types.h - 类型定义

定义平台无关的固定大小类型：

```c
/* 固定大小整数 */
int8, int16, int32, int64
uint8, uint16, uint32, uint64

/* 布尔类型 */
bool (true/false)

/* 平台相关大小类型 */
osal_size_t   /* 32位系统=uint32, 64位系统=uint64 */
osal_ssize_t  /* 32位系统=int32, 64位系统=int64 */

/* OSAL 对象 ID */
osal_id_t     /* uint32 */
```

### osal_platform.h - 平台检测

自动检测操作系统和架构：

```c
/* 操作系统检测 */
OSAL_OS_LINUX, OSAL_OS_WINDOWS, OSAL_OS_MACOS, OSAL_OS_FREERTOS

/* 架构检测 */
OSAL_ARCH_X86_64, OSAL_ARCH_ARM32, OSAL_ARCH_ARM64, OSAL_ARCH_RISCV64

/* 字节序检测 */
OSAL_LITTLE_ENDIAN, OSAL_BIG_ENDIAN
```

## 设计原则

### 1. 头文件和源文件严格对应

每个头文件都有对应的源文件，目录结构完全一致：

```
include/ipc/osal_mutex.h  ←→  src/posix/ipc/osal_mutex.c
include/sys/osal_thread.h ←→  src/posix/sys/osal_thread.c
include/net/osal_socket.h ←→  src/posix/net/osal_socket.c
```

### 2. 功能分组清晰

- **IPC**：线程同步原语
- **SYS**：系统调用（进程、线程、时间、文件、信号）
- **NET**：网络和串口通信
- **LIB**：标准库函数封装
- **UTIL**：辅助工具（日志、版本）

### 3. 平台实现隔离

- `include/` 目录：平台无关的接口声明
- `src/posix/` 目录：POSIX 平台实现
- 未来可扩展：`src/freertos/`, `src/vxworks/`, `src/zephyr/`

### 4. 单一职责

每个模块只负责一个功能领域，避免交叉依赖：

- `osal_socket.c` 只处理 Socket 操作
- `osal_termios.c` 只处理串口配置
- `osal_thread.c` 只处理线程管理

## 添加新模块

### 步骤 1：确定功能分类

根据功能选择合适的子目录：

- 线程同步 → `ipc/`
- 系统调用 → `sys/`
- 网络通信 → `net/`
- 标准库 → `lib/`
- 工具辅助 → `util/`

### 步骤 2：创建头文件

在 `include/<category>/` 创建头文件：

```c
/* include/sys/osal_timer.h */
#ifndef OSAL_TIMER_H
#define OSAL_TIMER_H

#include "osal_types.h"

int32_t OSAL_TimerCreate(osal_id_t *timer_id, const char *name);
int32_t OSAL_TimerDelete(osal_id_t timer_id);

#endif
```

### 步骤 3：创建源文件

在 `src/posix/<category>/` 创建源文件：

```c
/* src/posix/sys/osal_timer.c */
#include "sys/osal_timer.h"
#include <time.h>

int32_t OSAL_TimerCreate(osal_id_t *timer_id, const char *name)
{
    /* POSIX 实现 */
    return 0;
}
```

### 步骤 4：更新 CMakeLists.txt

在 `osal/CMakeLists.txt` 的对应分组添加源文件：

```cmake
# SYS - 系统调用封装
${OSAL_SRC_DIR}/sys/osal_clock.c
${OSAL_SRC_DIR}/sys/osal_timer.c    # 新增
```

### 步骤 5：更新 osal.h

在 `include/osal.h` 的对应分组添加头文件：

```c
/* SYS - 系统调用封装 */
#include "sys/osal_clock.h"
#include "sys/osal_timer.h"    /* 新增 */
```

## 文件统计

```
总计：42 个文件

头文件（include/）：21 个
├── ipc/      4 个
├── sys/      8 个
├── net/      2 个
├── lib/      5 个
├── util/     2 个
└── 根目录    3 个（osal.h, osal_types.h, osal_platform.h）

源文件（src/posix/）：21 个
├── ipc/      4 个
├── sys/      8 个
├── net/      2 个
├── lib/      5 个
└── util/     2 个
```

## 命名规范

### 头文件

```
osal_<module>.h
```

示例：`osal_mutex.h`, `osal_socket.h`, `osal_thread.h`

### 源文件

```
osal_<module>.c
```

示例：`osal_mutex.c`, `osal_socket.c`, `osal_thread.c`

### API 函数

```
OSAL_<Module><Action>()
```

示例：
- `OSAL_MutexCreate()`
- `OSAL_ThreadCreate()`
- `OSAL_SocketBind()`

### 内部函数（static）

```
osal_<module>_<action>()
```

示例：
- `osal_mutex_validate()`
- `osal_thread_cleanup()`

## 依赖关系

```
上层模块（HAL/PDL/Apps）
    ↓
osal.h（主头文件）
    ↓
各子模块头文件（ipc/sys/net/lib/util）
    ↓
osal_types.h（类型定义）
    ↓
系统头文件（<pthread.h>, <unistd.h>, 等）
```

**关键规则**：
- 只有 OSAL 层可以包含系统头文件
- 上层模块只能包含 `<osal.h>` 或子模块头文件
- 禁止上层直接使用系统调用

## 跨平台扩展

当前支持 POSIX 平台，未来可扩展：

```
src/
├── posix/       # Linux/macOS/Unix
├── freertos/    # FreeRTOS（计划中）
├── vxworks/     # VxWorks（计划中）
└── zephyr/      # Zephyr RTOS（计划中）
```

添加新平台时：
1. 创建 `src/<platform>/` 目录
2. 实现所有模块的平台版本
3. 在 CMakeLists.txt 添加平台选择逻辑
4. 头文件保持不变（平台无关）
