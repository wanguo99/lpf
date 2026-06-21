# OSAL (操作系统抽象层)

## 概述

OSAL (Operating System Abstraction Layer) 提供 Linux 用户态系统接口封装，覆盖任务管理、进程间通信、网络、文件操作等系统调用。

**设计理念**：
- 用户态库设计，无需显式初始化
- Linux-only 用户态实现
- 线程安全，优雅退出
- 引用计数，死锁检测

## 快速开始

### 编译

```bash
# 在项目根目录加载一个包含 OSAL 的配置
make ubuntu_x86_modules_defconfig
make
```

### 使用示例

```c
#include <osal.h>

static void worker_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown()) {
        LOG_INFO("Worker", "Running...");
        OSAL_TaskDelay(1000);
    }
}

int main(void)
{
    osal_id_t task_id;
    OSAL_TaskCreate(&task_id, "Worker", worker_task, NULL, 
                    8192, 100, 0);
    
    OSAL_TaskDelay(5000);
    OSAL_TaskDelete(task_id);
    return 0;
}
```

## 主要特性

- **零初始化**：静态初始化，无需显式 Init/Teardown
- **Linux-only**：基于 POSIX/Linux libc API 的用户态实现
- **线程安全**：所有接口均为线程安全
- **优雅退出**：避免 pthread_cancel，支持任务优雅关闭
- **引用计数**：消息队列使用引用计数，防止 use-after-free
- **死锁检测**：互斥锁内置 5 秒超时检测
- **日志轮转**：5 级日志系统，自动轮转（10MB/5 个备份）

## 模块组成

### IPC - 进程间通信
- **任务管理** - 任务创建、删除、优雅退出
- **消息队列** - 线程安全的消息队列（引用计数）
- **互斥锁** - 互斥锁（死锁检测）
- **信号量** - 信号量操作
- **条件变量** - 条件变量同步
- **原子操作** - 原子操作封装

### SYS - 系统调用封装
- **时钟** - 系统时钟
- **信号** - 信号处理
- **文件** - 文件 I/O
- **Select** - I/O 多路复用
- **环境变量** - 环境变量操作
- **时间** - 时间操作

### NET - 网络抽象
- **Socket** - Socket 操作
- **Termios** - 串口控制

### LIB - 标准库封装
- **字符串** - 字符串操作
- **堆内存** - 内存管理
- **错误处理** - 37 个标准错误码

### UTIL - 工具类
- **日志** - 5 级日志系统，自动轮转
- **版本** - 版本信息查询

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    OSAL Public API                       │
│              (平台无关的公共接口)                         │
└─────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
┌───────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐
│  IPC模块     │  │  SYS模块    │  │  NET模块    │
│  任务/队列   │  │  文件/信号  │  │  Socket     │
│  互斥锁/原子 │  │  时钟/环境  │  │  Termios    │
└───────┬──────┘  └──────┬──────┘  └──────┬──────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          │
┌─────────────────────────▼─────────────────────────────┐
│              LIB模块 + UTIL模块                        │
│          字符串/内存/错误处理 + 日志/版本              │
└─────────────────────────┬─────────────────────────────┘
                          │
┌─────────────────────────▼─────────────────────────────┐
│         Linux User-Space Implementation Layer           │
│              (实现代码: src/)                           │
└─────────────────────────┬─────────────────────────────┘
                          │
┌─────────────────────────▼─────────────────────────────┐
│              Linux libc / Linux syscalls               │
└───────────────────────────────────────────────────────┘
```

### 关键设计

#### 1. 任务优雅退出

使用 shutdown 标志而非 pthread_cancel：

```c
/* 任务循环检查退出标志 */
while (!OSAL_TaskShouldShutdown()) {
    /* 执行任务逻辑 */
}
/* 清理资源 */
```

**优势**：
- 避免资源泄漏（锁未释放）
- 避免死锁（取消点不确定）
- 简化清理逻辑

#### 2. 队列引用计数

防止多线程环境下的 use-after-free：

```c
/* 获取队列（增加引用计数） */
queue_impl_t* osal_queue_acquire(osal_id_t queue_id)
{
    if (impl->valid) {
        atomic_fetch_add(&impl->ref_count, 1);
        return impl;
    }
    return NULL;
}

/* 释放队列（减少引用计数） */
void osal_queue_release(queue_impl_t *impl)
{
    int old_count = atomic_fetch_sub(&impl->ref_count, 1);
    
    /* 引用计数降为0且已标记无效，则释放资源 */
    if (old_count == 1 && !impl->valid) {
        /* 销毁资源 */
    }
}
```

**删除流程**：
1. 从全局表移除
2. 标记 valid=false
3. 唤醒所有等待线程
4. 释放初始引用
5. 等待所有使用者释放引用
6. 最后一个释放者销毁资源

#### 3. 互斥锁死锁检测

5 秒超时机制：

```c
int32_t osal_task_lock(osal_mutex_t *mutex, uint32_t timeout_ms)
{
    int ret = osal_mutex_timed_lock(mutex, timeout_ms);

    if (ret == OSAL_ETIMEDOUT) {
        return OS_SEM_TIMEOUT;  /* 死锁检测 */
    }

    return OS_SUCCESS;
}
```

#### 4. 静态初始化

无需显式初始化：

```c
/* 全局表静态初始化为0 */
static osal_task_record_t g_osal_task_table[OS_MAX_TASKS] = {0};

/* 互斥锁静态初始化 */
static osal_mutex_t g_task_table_mutex = OSAL_MUTEX_INITIALIZER;

/* 无需 Init 函数，直接使用 */
```

## 源码布局

当前 OSAL 面向 Linux-only 用户态实现，源码按能力域组织：

```
osal/
├── include/           # 接口层（平台无关）
└── src/
    ├── ipc/           # IPC 与同步
    ├── lib/           # libc/工具封装
    ├── net/           # socket/termios/pty
    ├── sys/           # 系统调用封装
    └── util/          # log/version/crc
```
3. 修改 CMakeLists.txt 选择实现
4. 运行测试验证

## 性能指标

- **任务切换延迟**：< 1ms
- **队列操作延迟**：< 100μs
- **互斥锁获取延迟**：< 50μs
- **日志写入延迟**：< 1ms

## 资源限制

```c
#define OS_MAX_TASKS        64   /* 最大任务数 */
#define OS_MAX_QUEUES       64   /* 最大队列数 */
#define OS_MAX_MUTEXES      64   /* 最大互斥锁数 */
#define OS_MAX_API_NAME     20   /* 最大名称长度 */
```

## 参考文档

- [使用指南](docs/USAGE_GUIDE.md) - 完整的 API 参考、类型指南和使用示例
- [编码规范](../docs/CODING_STANDARDS.md) - 项目编码规范
