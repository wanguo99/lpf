# EMS 桥接板软件架构优化方案 v3.1（多进程架构 - 最终版）

**文档版本**：v3.1（多进程架构最终版）  
**编制日期**：2026年5月  
**修订日期**：2026年5月16日  
**适用平台**：Linux + PREEMPT_RT（TI AM62x / Xilinx Zynq MPSoC）  
**核心架构**：多进程 + Supervisor + OSAL IPC + ACL 配置层 + 进程级故障隔离  
**适用环境**：太空环境（辐射、无法维修、极高可靠性要求）  
**对应仓库**：https://github.com/wanguo99/EMS.git

**重要更新（v3.1）**：
- ✅ IPC机制完全集成到OSAL层
- ✅ 提供高级IPC抽象接口（消息队列、遥测池、心跳表）
- ✅ 业务层代码大幅简化

---

## ⚠️ 重要说明：为什么选择多进程架构

**太空环境的特殊约束**：
1. **无法物理接触** - 发射后无法维修或重启硬件
2. **辐射环境** - 单粒子翻转（SEU）、单粒子闩锁（SEL）
3. **极端可靠性** - 任务失败代价巨大（数亿美元 + 科学数据丢失）
4. **长期运行** - 可能需要连续运行数年

**单进程多线程的致命缺陷**：
- ❌ 任何线程的内存错误导致整个进程崩溃
- ❌ SEU 翻转共享内存，所有线程同时受影响
- ❌ 无法实现真正的故障隔离
- ❌ 一个线程死锁可能拖垮整个系统

**多进程架构的优势**：
- ✅ 进程级故障隔离（一个进程崩溃不影响其他进程）
- ✅ 辐射容错（SEU 只影响单个进程的地址空间）
- ✅ 自动恢复（Supervisor 检测并重启崩溃的进程）
- ✅ 降级运行（部分功能失效时系统仍可运行）
- ✅ 符合 NASA/ESA 航天软件标准

**性能代价可接受**：
- IPC 延迟：5-10 μs（vs 单进程的 50-100 ns）
- 对于秒级遥测周期，5μs 延迟完全可忽略
- **可靠性 >> 性能**（这是太空应用的铁律）

---

## 1. 系统概述

### 1.1 架构演进

本方案（v3.1）是针对太空环境的多进程架构最终版，相比 v3.0 单进程多线程方案和 v2.0 多进程方案：

**v3.0 单进程多线程方案的问题（太空环境）**：

1. **故障隔离不足**：任何线程的段错误、空指针访问都会导致整个进程崩溃
2. **辐射敏感**：单粒子翻转（SEU）影响共享内存，所有线程同时受影响
3. **无法真正恢复**：线程崩溃后，进程地址空间已损坏，重启线程无意义
4. **不符合航天标准**：NASA/ESA 要求进程级隔离

**v2.0 多进程方案的问题**：

1. 缺少 ACL 层：APP 与硬件强耦合
2. IPC 设计不完善：未考虑进程崩溃时的资源清理
3. Supervisor 设计过于复杂：未遵循最小化原则

**v3.1 多进程方案的核心思想**：

- **多进程架构**：每个关键功能独立进程，完全隔离
- **Supervisor 进程**：最小化、高可靠的监控进程（<500行代码）
- **进程级故障隔离**：一个进程崩溃不影响其他进程
- **自动恢复**：Supervisor 检测进程崩溃并自动重启
- **降级运行**：部分功能失效时，系统仍可运行核心功能
- **保留 ACL 配置层**：业务功能与硬件完全解耦（每个进程独立加载）
- **OSAL IPC集成**：IPC机制完全集成到OSAL层，提供统一的高级抽象接口
  - 基础层：POSIX消息队列、共享内存、信号量的1:1封装
  - 抽象层：消息队列、遥测池、心跳表的高级接口
- **业务层简化**：业务进程直接使用OSAL IPC接口，无需重复实现IPC逻辑

**ACL 与 PCL 的职责划分（保持不变）**：

| 层次 | 职责 | 数据内容 | 示例 |
|------|------|----------|------|
| **ACL** | 业务功能到设备的映射 | 功能枚举 → (设备类型, 逻辑索引) | `CPU_TEMP → (BMC, 1)` |
| **PCL** | 硬件配置存储 | 逻辑索引 → 具体硬件配置 | `BMC[1] → {Redfish, 192.168.1.100:443}` |

**为什么保留 ACL 层？**

1. **业务与硬件完全分离**：每个进程只关心业务功能，不关心硬件实现
2. **进程独立配置**：每个进程独立加载 ACL 配置，互不影响
3. **类型安全**：枚举提供编译期类型检查
4. **O(1) 查找性能**：使用枚举值作为数组索引

**ACL 的核心价值：业务语义与硬件实现的完全解耦**

```
APP 层：      "我需要获取 CPU 温度"
              ↓
ACL 层：      CPU_TEMP → (BMC, logic_index=1)
              ↓
PDL 层：      调用 PDL_BMC_ReadSensor(bmc_handle[1], SENSOR_CPU_TEMP, &temp)
              ↓
PCL 层：      BMC[1] → {protocol=Redfish, ip="192.168.1.100", port=443}
              ↓
HAL 层：      通过 Redfish 协议读取传感器
```

**关键点**：每个进程独立运行，进程崩溃不影响其他进程，Supervisor 自动重启。

### 1.2 系统架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Supervisor Process (监控进程)                     │
│  • 最小化设计（<500行代码）                                          │
│  • 监控所有子进程（心跳检测）                                        │
│  • 进程崩溃自动重启                                                  │
│  • 故障计数和降级策略                                                │
│  • 喂硬件看门狗                                                      │
│  • 记录故障到持久存储                                                │
└────────┬──────────────┬──────────────┬──────────────────────────────┘
         │              │              │
    ┌────▼────────┐ ┌───▼─────────┐ ┌─▼──────────┐
    │ sat_comm    │ │   server    │ │ local_dev  │
    │ Process     │ │   Process   │ │  Process   │
    │ (高优先级)  │ │  (低优先级) │ │ (中优先级) │
    │             │ │             │ │            │
    │ • 1553B/SpW │ │ • Redfish   │ │ • I2C/SPI  │
    │ • 命令路由  │ │ • 电源控制  │ │ • MCU/CPLD │
    │ • 遥测打包  │ │ • 传感器    │ │ • 看门狗   │
    │             │ │             │ │            │
    │ 独立地址空间│ │ 独立地址空间│ │独立地址空间│
    └───┬─────────┘ └──────┬──────┘ └───┬────────┘
        │                  │             │
        │   ┌──────────────▼─────────────▼──────────┐
        │   │  Shared Memory (共享内存遥测池)       │
        │   │  • 双缓冲设计                          │
        │   │  • 信号量保护                          │
        │   │  • 进程崩溃不影响共享内存              │
        │   └────────────────────────────────────────┘
        │
        └──────────── POSIX Message Queues (消息队列) ──────────────────┤
                     • 内核保证可靠性                                    │
                     • 进程崩溃不丢失消息                                │
                                                                         │
  ┌────────────────────────────────────────────────────────────────────┘
  │
  │  ┌────────────────────────────────────────────────────────────────┐
  │  │                    ACL (应用配置层)                             │
  │  │  每个进程独立加载 ACL 配置                                      │
  │  │  业务功能枚举 → (设备类型, 逻辑索引, 启用标志)                  │
  │  │  • 类型安全的枚举定义                                           │
  │  │  • O(1) 查找性能                                                │
  │  └──────────────────────────┬─────────────────────────────────────┘
  │                             │
  │  ┌──────────────────────────▼─────────────────────────────────────┐
  │  │                   PDL (外设驱动层)                              │
  │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
  │  │  │  Satellite   │  │     BMC      │  │     MCU      │         │
  │  │  │   Service    │  │   Service    │  │   Service    │         │
  │  │  │  (完全独立)  │  │  (完全独立)  │  │  (完全独立)  │         │
  │  │  └──────────────┘  └──────────────┘  └──────────────┘         │
  │  └──────────────────────────┬─────────────────────────────────────┘
  │                             │
  │  ┌──────────────────────────▼─────────────────────────────────────┐
  │  │                   PCL (外设配置层)                              │
  │  │  硬件配置数组 (逻辑索引 → 硬件参数)                             │
  │  │  • MCU 配置数组:  [0]=主MCU, [1]=CPLD, [2]=看门狗              │
  │  │  • BMC 配置数组:  [0]=服务器BMC                                │
  │  │  • Satellite 配置: [0]=卫星平台CAN接口                         │
  │  └──────────────────────────┬─────────────────────────────────────┘
  │                             │
  │  ┌──────────────────────────▼─────────────────────────────────────┐
  │  │                   HAL (硬件抽象层)                              │
  │  │  CAN │ UART │ I2C │ SPI │ Ethernet │ GPIO                      │
  │  └──────────────────────────┬─────────────────────────────────────┘
  │                             │
  │  ┌──────────────────────────▼─────────────────────────────────────┐
  │  │                   OSAL (操作系统抽象层)                         │
  │  │  Process │ Shm │ Mq │ Thread │ Mutex │ Semaphore │ Signal     │
  │  └────────────────────────────────────────────────────────────────┘
  │
  └──────────────────────────────────────────────────────────────────────
                             │
                    ┌────────▼────────┐
                    │ Hardware Watchdog│
                    │  (硬件看门狗)    │
                    │  • 最后防线      │
                    │  • 5-10秒超时   │
                    └──────────────────┘
```

**架构说明**：

- **Supervisor 进程**：最小化、高可靠，监控所有子进程
- **业务进程**：sat_comm（高）、local_dev（中）、server（低）
- **进程隔离**：每个进程独立地址空间，崩溃不影响其他进程
- **IPC 机制**：
  - POSIX 消息队列：用于命令传递（sat_comm → server/local_dev）
  - 共享内存 + 信号量：用于遥测数据收集（server/local_dev → sat_comm）
- **配置层次**：
  - ACL：业务功能映射（每个进程独立加载）
  - PCL：硬件配置存储
- **驱动层次**：
  - PDL：三个完全独立的服务（Satellite/BMC/MCU）
  - HAL：硬件接口驱动
  - OSAL：操作系统抽象（新增进程管理接口）
- **故障恢复**：
  - 进程级：Supervisor 重启崩溃的进程
  - 系统级：硬件看门狗复位整个系统

**关键设计原则**：

1. **最小化 Supervisor**：<500行代码，降低自身故障概率
2. **进程完全隔离**：独立地址空间，SEU 只影响单个进程
3. **内核保证 IPC**：使用 POSIX 标准 IPC，内核保证可靠性
4. **三级故障恢复**：进程重启 → 降级运行 → 硬件复位
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │              Watchdog Thread (看门狗线程)                       │ │
│  │  • 监控所有业务线程心跳                                         │ │
│  │  • 检测线程挂死并重启                                           │ │
│  │  • 记录故障次数，触发系统复位                                   │ │
│  └───────────┬──────────────┬──────────────┬──────────────────────┘ │
│              │              │              │                         │
│  ┌───────────▼────┐  ┌──────▼──────┐  ┌───▼──────────┐             │
│  │  sat_comm      │  │   server    │  │  local_dev   │             │
│  │  Thread        │  │   Thread    │  │  Thread      │             │
│  │  (高优先级)    │  │  (低优先级) │  │  (中优先级)  │             │
│  │                │  │             │  │              │             │
│  │  • 1553B/SpW   │  │  • Redfish  │  │  • I2C/SPI   │             │
│  │  • 命令路由    │  │  • 电源控制 │  │  • MCU/CPLD  │             │
│  │  • 遥测打包    │  │  • 传感器   │  │  • 看门狗    │             │
│  └───┬────────────┘  └──────┬──────┘  └───┬──────────┘             │
│      │                      │             │                         │
│      │   ┌──────────────────▼─────────────▼──────────┐             │
│      │   │    Telemetry Pool (双缓冲遥测池)          │             │
│      │   │    • 无锁双缓冲设计                        │             │
│      │   │    • 原子操作保证一致性                    │             │
│      │   └────────────────────────────────────────────┘             │
│      │                                                               │
│      └──────────── SPSC Queues (无锁命令队列) ──────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    ACL (应用配置层)                             │ │
│  │  业务功能枚举 → (设备类型, 逻辑索引, 启用标志)                  │ │
│  │  • 类型安全的枚举定义                                           │ │
│  │  • 编译期类型检查                                               │ │
│  └──────────────────────────┬─────────────────────────────────────┘ │
│                             │                                        │
│  ┌──────────────────────────▼─────────────────────────────────────┐ │
│  │                   PDL (外设驱动层)                              │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │ │
│  │  │  Satellite   │  │     BMC      │  │     MCU      │         │ │
│  │  │   Service    │  │   Service    │  │   Service    │         │ │
│  │  │  (完全独立)  │  │  (完全独立)  │  │  (完全独立)  │         │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘         │ │
│  └──────────────────────────┬─────────────────────────────────────┘ │
│                             │                                        │
│  ┌──────────────────────────▼─────────────────────────────────────┐ │
│  │                   PCL (外设配置层)                              │ │
│  │  硬件配置数组 (逻辑索引 → 硬件参数)                             │ │
│  │  • MCU 配置数组:  [0]=主MCU, [1]=CPLD, [2]=看门狗              │ │
│  │  • BMC 配置数组:  [0]=服务器BMC                                │ │
│  │  • Satellite 配置: [0]=卫星平台CAN接口                         │ │
│  └──────────────────────────┬─────────────────────────────────────┘ │
│                             │                                        │
│  ┌──────────────────────────▼─────────────────────────────────────┐ │
│  │                   HAL (硬件抽象层)                              │ │
│  │  CAN │ UART │ I2C │ SPI │ Ethernet │ GPIO                      │ │
│  └──────────────────────────┬─────────────────────────────────────┘ │
│                             │                                        │
│  ┌──────────────────────────▼─────────────────────────────────────┐ │
│  │                   OSAL (操作系统抽象层)                         │ │
│  │  Thread │ Mutex │ Semaphore │ Socket │ Time │ Signal           │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

**架构说明**：

- **看门狗线程**：最高优先级，监控所有业务线程的心跳
- **业务线程**：sat_comm（高）、local_dev（中）、server（低）
- **IPC 机制**：
  - SPSC 无锁队列：用于命令传递（sat_comm → server/local_dev）
  - 双缓冲遥测池：用于遥测数据收集（server/local_dev → sat_comm）
- **配置层次**：
  - ACL：业务功能映射（枚举 → 设备索引）
  - PCL：硬件配置存储（索引 → 硬件参数）
- **驱动层次**：
  - PDL：三个完全独立的服务（Satellite/BMC/MCU）
  - HAL：硬件接口驱动
  - OSAL：操作系统抽象

---

## 2. Supervisor 进程设计（监控进程）

### 2.1 Supervisor 核心职责

**设计原则**：最小化、高可靠（<500行代码）

Supervisor 是整个系统的监护者，负责：
1. 启动所有子进程
2. 监控子进程健康状态（心跳检测）
3. 检测进程崩溃并自动重启
4. 故障计数和降级策略
5. 喂硬件看门狗
6. 记录故障到持久存储

**为什么要最小化？**
- 代码越少，bug 越少
- Supervisor 自身崩溃会导致整个系统失效
- 遵循 NASA 航天软件标准（关键组件最小化）

### 2.2 进程信息结构

```c
/************************************************************************
 * supervisor/include/supervisor.h
 * Supervisor 进程核心数据结构
 ************************************************************************/

#include "osal.h"

/**
 * @brief 进程状态
 */
typedef enum {
    PROC_STATE_STOPPED = 0,    /* 已停止 */
    PROC_STATE_STARTING,       /* 启动中 */
    PROC_STATE_RUNNING,        /* 运行中 */
    PROC_STATE_CRASHED,        /* 已崩溃 */
    PROC_STATE_RESTARTING,     /* 重启中 */
    PROC_STATE_DISABLED        /* 已禁用（降级运行）*/
} process_state_t;

/**
 * @brief 进程信息
 */
typedef struct {
    const char *name;          /* 进程名称 */
    const char *exec_path;     /* 可执行文件路径 */
    pid_t       pid;           /* 进程ID */
    process_state_t state;     /* 进程状态 */
    
    /* 心跳监控 */
    time_t      last_heartbeat;    /* 最后心跳时间 */
    uint32_t    heartbeat_timeout; /* 心跳超时时间（秒）*/
    
    /* 故障统计 */
    uint32_t    crash_count;       /* 崩溃次数 */
    uint32_t    restart_count;     /* 重启次数 */
    time_t      last_crash_time;   /* 最后崩溃时间 */
    
    /* 重启策略 */
    uint32_t    max_restart_count; /* 最大重启次数 */
    uint32_t    restart_window_sec;/* 重启窗口时间（秒）*/
    
    /* 优先级 */
    int32_t     priority;          /* 进程优先级 */
    bool        critical;          /* 是否关键进程 */
} process_info_t;

/**
 * @brief 系统运行模式
 */
typedef enum {
    SYS_MODE_NORMAL = 0,       /* 正常模式 */
    SYS_MODE_DEGRADED,         /* 降级模式（部分功能失效）*/
    SYS_MODE_SAFE              /* 安全模式（仅核心功能）*/
} system_mode_t;
```

### 2.3 Supervisor 主循环

```c
/************************************************************************
 * supervisor/src/supervisor_main.c
 * Supervisor 进程主程序
 ************************************************************************/

#include "supervisor.h"
#include <signal.h>
#include <sys/wait.h>

/* 全局进程表 */
static process_info_t g_processes[] = {
    {
        .name = "sat_comm",
        .exec_path = "/opt/ems/bin/sat_comm_process",
        .pid = 0,
        .state = PROC_STATE_STOPPED,
        .heartbeat_timeout = 5,      /* 5秒心跳超时 */
        .max_restart_count = 5,      /* 最多重启5次 */
        .restart_window_sec = 300,   /* 5分钟窗口 */
        .priority = 90,              /* 高优先级 */
        .critical = true             /* 关键进程 */
    },
    {
        .name = "server",
        .exec_path = "/opt/ems/bin/server_process",
        .pid = 0,
        .state = PROC_STATE_STOPPED,
        .heartbeat_timeout = 10,
        .max_restart_count = 3,
        .restart_window_sec = 300,
        .priority = 50,
        .critical = false
    },
    {
        .name = "local_dev",
        .exec_path = "/opt/ems/bin/local_dev_process",
        .pid = 0,
        .state = PROC_STATE_STOPPED,
        .heartbeat_timeout = 5,
        .max_restart_count = 5,
        .restart_window_sec = 300,
        .priority = 70,
        .critical = true
    }
};

#define NUM_PROCESSES (sizeof(g_processes) / sizeof(g_processes[0]))

/* 系统运行模式 */
static system_mode_t g_system_mode = SYS_MODE_NORMAL;

/* 运行标志 */
static volatile bool g_running = true;

/**
 * @brief 信号处理函数
 */
static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        LOG_INFO("SUPERVISOR", "Received shutdown signal");
        g_running = false;
    } else if (sig == SIGCHLD) {
        /* 子进程退出信号，在主循环中处理 */
    }
}

/**
 * @brief 启动子进程
 */
static int32_t start_process(process_info_t *proc)
{
    pid_t pid = fork();
    
    if (pid < 0) {
        LOG_ERROR("SUPERVISOR", "Failed to fork process %s", proc->name);
        return OSAL_ERR_GENERIC;
    }
    
    if (pid == 0) {
        /* 子进程 */
        
        /* 设置进程优先级 */
        struct sched_param param;
        param.sched_priority = proc->priority;
        if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
            LOG_WARN("SUPERVISOR", "Failed to set priority for %s", proc->name);
        }
        
        /* 执行子进程程序 */
        execl(proc->exec_path, proc->name, NULL);
        
        /* 如果 execl 返回，说明执行失败 */
        LOG_FATAL("SUPERVISOR", "Failed to exec %s: %s", 
                  proc->name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* 父进程 */
    proc->pid = pid;
    proc->state = PROC_STATE_RUNNING;
    proc->last_heartbeat = time(NULL);
    
    LOG_INFO("SUPERVISOR", "Started process %s (PID=%d)", proc->name, pid);
    
    return OSAL_SUCCESS;
}

/**
 * @brief 停止子进程
 */
static int32_t stop_process(process_info_t *proc)
{
    if (proc->pid <= 0) {
        return OSAL_SUCCESS;
    }
    
    LOG_INFO("SUPERVISOR", "Stopping process %s (PID=%d)", proc->name, proc->pid);
    
    /* 发送 SIGTERM 信号 */
    kill(proc->pid, SIGTERM);
    
    /* 等待进程退出（最多5秒）*/
    for (int i = 0; i < 50; i++) {
        int status;
        pid_t result = waitpid(proc->pid, &status, WNOHANG);
        
        if (result == proc->pid) {
            /* 进程已退出 */
            proc->pid = 0;
            proc->state = PROC_STATE_STOPPED;
            LOG_INFO("SUPERVISOR", "Process %s stopped gracefully", proc->name);
            return OSAL_SUCCESS;
        }
        
        usleep(100000);  /* 100ms */
    }
    
    /* 超时，强制杀死 */
    LOG_WARN("SUPERVISOR", "Process %s did not stop, sending SIGKILL", proc->name);
    kill(proc->pid, SIGKILL);
    waitpid(proc->pid, NULL, 0);
    
    proc->pid = 0;
    proc->state = PROC_STATE_STOPPED;
    
    return OSAL_SUCCESS;
}

/**
 * @brief 检查进程是否存活
 */
static bool is_process_alive(process_info_t *proc)
{
    if (proc->pid <= 0) {
        return false;
    }
    
    /* 发送信号0检查进程是否存在 */
    if (kill(proc->pid, 0) == 0) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查进程心跳
 */
static bool check_heartbeat(process_info_t *proc)
{
    time_t now = time(NULL);
    time_t elapsed = now - proc->last_heartbeat;
    
    if (elapsed > proc->heartbeat_timeout) {
        LOG_ERROR("SUPERVISOR", "Process %s heartbeat timeout (%ld sec)", 
                  proc->name, elapsed);
        return false;
    }
    
    return true;
}

/**
 * @brief 处理进程崩溃
 */
static void handle_process_crash(process_info_t *proc)
{
    time_t now = time(NULL);
    
    proc->state = PROC_STATE_CRASHED;
    proc->crash_count++;
    proc->last_crash_time = now;
    
    LOG_ERROR("SUPERVISOR", "Process %s crashed (crash_count=%u)", 
              proc->name, proc->crash_count);
    
    /* 记录故障到持久存储 */
    log_fault_to_storage(proc->name, proc->crash_count, now);
    
    /* 检查是否超过重启次数限制 */
    if (proc->crash_count >= proc->max_restart_count) {
        LOG_FATAL("SUPERVISOR", "Process %s exceeded max restart count, disabling", 
                  proc->name);
        proc->state = PROC_STATE_DISABLED;
        
        /* 如果是关键进程，进入降级模式 */
        if (proc->critical) {
            g_system_mode = SYS_MODE_DEGRADED;
            LOG_WARN("SUPERVISOR", "Entering DEGRADED mode due to critical process failure");
        }
        
        return;
    }
    
    /* 重启进程 */
    LOG_INFO("SUPERVISOR", "Restarting process %s (attempt %u/%u)", 
             proc->name, proc->restart_count + 1, proc->max_restart_count);
    
    proc->state = PROC_STATE_RESTARTING;
    proc->restart_count++;
    
    /* 延迟1秒后重启 */
    sleep(1);
    
    if (start_process(proc) == OSAL_SUCCESS) {
        LOG_INFO("SUPERVISOR", "Process %s restarted successfully", proc->name);
    } else {
        LOG_ERROR("SUPERVISOR", "Failed to restart process %s", proc->name);
        proc->state = PROC_STATE_DISABLED;
    }
}

/**
 * @brief 喂硬件看门狗
 */
static void feed_hardware_watchdog(void)
{
    /* 通过 OSAL 接口喂硬件看门狗 */
    /* 实际实现取决于硬件平台 */
    static int fd = -1;
    
    if (fd < 0) {
        fd = open("/dev/watchdog", O_WRONLY);
        if (fd < 0) {
            LOG_ERROR("SUPERVISOR", "Failed to open hardware watchdog");
            return;
        }
    }
    
    /* 写入任意数据喂狗 */
    write(fd, "1", 1);
}

/**
 * @brief Supervisor 主函数
 */
int main(int argc, char *argv[])
{
    int32_t ret;
    
    LOG_INFO("SUPERVISOR", "EMS Supervisor starting...");
    
    /* 注册信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, signal_handler);
    
    /* 启动所有子进程 */
    for (uint32_t i = 0; i < NUM_PROCESSES; i++) {
        ret = start_process(&g_processes[i]);
        if (ret != OSAL_SUCCESS) {
            LOG_ERROR("SUPERVISOR", "Failed to start process %s", g_processes[i].name);
        }
    }
    
    LOG_INFO("SUPERVISOR", "All processes started, entering monitoring loop");
    
    /* 主监控循环 */
    while (g_running) {
        /* 1. 检查所有进程状态 */
        for (uint32_t i = 0; i < NUM_PROCESSES; i++) {
            process_info_t *proc = &g_processes[i];
            
            if (proc->state == PROC_STATE_DISABLED) {
                continue;  /* 已禁用的进程跳过 */
            }
            
            /* 检查进程是否存活 */
            if (!is_process_alive(proc)) {
                LOG_ERROR("SUPERVISOR", "Process %s is dead", proc->name);
                handle_process_crash(proc);
                continue;
            }
            
            /* 检查心跳（TODO: 通过共享内存读取心跳时间戳）*/
            if (!check_heartbeat(proc)) {
                LOG_ERROR("SUPERVISOR", "Process %s heartbeat timeout", proc->name);
                /* 杀死并重启进程 */
                stop_process(proc);
                handle_process_crash(proc);
            }
        }
        
        /* 2. 处理 SIGCHLD 信号（子进程退出）*/
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            /* 找到对应的进程 */
            for (uint32_t i = 0; i < NUM_PROCESSES; i++) {
                if (g_processes[i].pid == pid) {
                    LOG_ERROR("SUPERVISOR", "Process %s (PID=%d) exited with status %d", 
                              g_processes[i].name, pid, WEXITSTATUS(status));
                    g_processes[i].pid = 0;
                    handle_process_crash(&g_processes[i]);
                    break;
                }
            }
        }
        
        /* 3. 喂硬件看门狗 */
        feed_hardware_watchdog();
        
        /* 4. 检查系统模式 */
        if (g_system_mode == SYS_MODE_DEGRADED) {
            /* 降级模式：检查是否可以恢复到正常模式 */
            bool all_critical_running = true;
            for (uint32_t i = 0; i < NUM_PROCESSES; i++) {
                if (g_processes[i].critical && 
                    g_processes[i].state != PROC_STATE_RUNNING) {
                    all_critical_running = false;
                    break;
                }
            }
            
            if (all_critical_running) {
                g_system_mode = SYS_MODE_NORMAL;
                LOG_INFO("SUPERVISOR", "Recovered to NORMAL mode");
            }
        }
        
        /* 5. 休眠1秒 */
        sleep(1);
    }
    
    /* 关闭所有子进程 */
    LOG_INFO("SUPERVISOR", "Shutting down all processes...");
    for (uint32_t i = 0; i < NUM_PROCESSES; i++) {
        stop_process(&g_processes[i]);
    }
    
    LOG_INFO("SUPERVISOR", "Supervisor exiting");
    
    return 0;
}
```

### 2.4 故障恢复策略

**三级故障恢复机制**：

```
Level 1: 进程级重启
  ↓ (重启失败或超过次数限制)
Level 2: 降级运行
  ↓ (Supervisor 崩溃)
Level 3: 硬件看门狗复位整个系统
```

**重启策略**：
- 单次崩溃：立即重启
- 5分钟内崩溃5次：禁用进程，进入降级模式
- 关键进程禁用：系统进入降级模式
- Supervisor 崩溃：硬件看门狗复位系统

**降级运行模式**：
- 正常模式：所有进程运行
- 降级模式：关键进程运行，非关键进程禁用
- 安全模式：仅 sat_comm 进程运行（最小功能）

---

## 3. ACL 层设计（应用配置层）

### 2.1 核心数据结构

```c
/************************************************************************
 * acl/include/acl_types.h
 * ACL 核心类型定义
 ************************************************************************/

/**
 * @brief 业务功能枚举（编译期类型检查）
 * 
 * 设计原则：
 * - 枚举名称表达业务语义，不涉及具体硬件实现
 * - APP 只需要知道"我要获取 CPU 温度"，不需要知道通过 BMC 还是 MCU
 * - 同一功能在不同项目中可以映射到不同设备
 */
typedef enum {
    /* 卫星通信功能 */
    ACL_FUNC_SAT_SEND_TELEMETRY = 0,    /* 发送遥测数据 */
    ACL_FUNC_SAT_RECV_COMMAND,          /* 接收遥控指令 */
    ACL_FUNC_SAT_SEND_HEARTBEAT,        /* 发送心跳 */

    /* 服务器管理功能 */
    ACL_FUNC_SERVER_POWER_ON,           /* 服务器开机 */
    ACL_FUNC_SERVER_POWER_OFF,          /* 服务器关机 */
    ACL_FUNC_SERVER_RESET,              /* 服务器复位 */
    
    /* 传感器读取功能（业务语义，不指定设备） */
    ACL_FUNC_READ_CPU_TEMP,             /* 读取 CPU 温度（可能来自 BMC 或 MCU） */
    ACL_FUNC_READ_BOARD_TEMP,           /* 读取板卡温度 */
    ACL_FUNC_READ_VOLTAGE_12V,          /* 读取 12V 电压 */
    ACL_FUNC_READ_VOLTAGE_5V,           /* 读取 5V 电压 */
    ACL_FUNC_READ_FAN_SPEED,            /* 读取风扇转速 */
    
    /* 本地控制功能 */
    ACL_FUNC_WRITE_CPLD_REG,            /* 写 CPLD 寄存器 */
    ACL_FUNC_READ_CPLD_STATUS,          /* 读 CPLD 状态 */
    ACL_FUNC_FEED_WATCHDOG,             /* 喂看门狗 */
    ACL_FUNC_TRIGGER_SYSTEM_RESET,      /* 触发系统复位 */

    ACL_FUNC_MAX  /* 边界检查 */
} acl_function_t;

/**
 * @brief 设备类型枚举（与 PCL 对齐）
 */
typedef enum {
    ACL_DEVICE_SATELLITE = 0,
    ACL_DEVICE_BMC,
    ACL_DEVICE_MCU,
    ACL_DEVICE_CPLD,
    ACL_DEVICE_MAX
} acl_device_type_t;

/**
 * @brief 功能配置项（ACL 核心映射）
 */
typedef struct {
    acl_function_t    function;      /* 业务功能枚举 */
    acl_device_type_t device_type;   /* 设备类型 */
    uint32_t          logic_index;   /* 逻辑索引（对应 PCL 数组索引）*/
    bool              enabled;       /* 是否启用 */
} acl_function_config_t;

/**
 * @brief ACL 配置表
 */
typedef struct {
    const acl_function_config_t *configs;  /* 配置数组 */
    uint32_t                     count;    /* 配置项数量 */
} acl_config_t;
```

### 2.2 ACL 接口

```c
/************************************************************************
 * acl/include/acl.h
 * ACL 公共接口
 ************************************************************************/

/**
 * @brief 初始化 ACL 层
 *
 * @param[in] config ACL 配置表
 * @return OSAL_SUCCESS 成功
 */
int32_t ACL_Init(const acl_config_t *config);

/**
 * @brief 根据业务功能获取设备索引
 *
 * @param[in]  function     业务功能枚举
 * @param[out] device_type  设备类型
 * @param[out] logic_index  逻辑索引
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_FOUND 功能未配置
 */
int32_t ACL_GetDeviceIndex(acl_function_t function,
                           acl_device_type_t *device_type,
                           uint32_t *logic_index);

/**
 * @brief 检查功能是否启用
 *
 * @param[in] function 业务功能枚举
 * @return true 启用
 * @return false 禁用或未配置
 */
bool ACL_IsFunctionEnabled(acl_function_t function);

/**
 * @brief 获取功能名称（调试用）
 *
 * @param[in] function 业务功能枚举
 * @return 功能名称字符串
 */
const char* ACL_GetFunctionName(acl_function_t function);
```

### 2.3 ACL 配置示例

**项目 A 的 ACL 配置**（CPU 温度通过 BMC 获取）：

```c
/************************************************************************
 * acl/config/ti_am6254_carrier_board_v1/acl_config.c
 * 项目 A 的业务功能映射配置
 ************************************************************************/

static const acl_function_config_t g_acl_configs[] = {
    /* 卫星通信功能 → Satellite 设备 */
    { ACL_FUNC_SAT_SEND_TELEMETRY,    ACL_DEVICE_SATELLITE, 0, true  },
    { ACL_FUNC_SAT_RECV_COMMAND,      ACL_DEVICE_SATELLITE, 0, true  },
    { ACL_FUNC_SAT_SEND_HEARTBEAT,    ACL_DEVICE_SATELLITE, 0, true  },
    
    /* 服务器管理功能 → BMC 设备 */
    { ACL_FUNC_SERVER_POWER_ON,       ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_SERVER_POWER_OFF,      ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_SERVER_RESET,          ACL_DEVICE_BMC,       0, true  },
    
    /* 传感器读取功能 → BMC 设备（项目 A 通过 BMC 获取） */
    { ACL_FUNC_READ_CPU_TEMP,         ACL_DEVICE_BMC,       0, true  },  // BMC[0]
    { ACL_FUNC_READ_BOARD_TEMP,       ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_READ_VOLTAGE_12V,      ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_READ_FAN_SPEED,        ACL_DEVICE_BMC,       0, true  },
    
    /* 本地控制功能 → MCU/CPLD 设备 */
    { ACL_FUNC_WRITE_CPLD_REG,        ACL_DEVICE_CPLD,      0, true  },
    { ACL_FUNC_READ_CPLD_STATUS,      ACL_DEVICE_CPLD,      0, true  },
    { ACL_FUNC_FEED_WATCHDOG,         ACL_DEVICE_MCU,       1, true  },  // MCU[1] 是看门狗
    { ACL_FUNC_TRIGGER_SYSTEM_RESET,  ACL_DEVICE_MCU,       0, true  },
};

const acl_config_t g_acl_config = {
    .configs = g_acl_configs,
    .count   = sizeof(g_acl_configs) / sizeof(g_acl_configs[0])
};
```

**项目 B 的 ACL 配置**（CPU 温度通过 MCU 获取）：

```c
/************************************************************************
 * acl/config/ti_am6254_carrier_board_v2/acl_config.c
 * 项目 B 的业务功能映射配置（不同的硬件实现）
 ************************************************************************/

static const acl_function_config_t g_acl_configs[] = {
    /* 卫星通信功能 → Satellite 设备 */
    { ACL_FUNC_SAT_SEND_TELEMETRY,    ACL_DEVICE_SATELLITE, 0, true  },
    { ACL_FUNC_SAT_RECV_COMMAND,      ACL_DEVICE_SATELLITE, 0, true  },
    { ACL_FUNC_SAT_SEND_HEARTBEAT,    ACL_DEVICE_SATELLITE, 0, true  },
    
    /* 服务器管理功能 → BMC 设备 */
    { ACL_FUNC_SERVER_POWER_ON,       ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_SERVER_POWER_OFF,      ACL_DEVICE_BMC,       0, true  },
    { ACL_FUNC_SERVER_RESET,          ACL_DEVICE_BMC,       0, true  },
    
    /* 传感器读取功能 → MCU 设备（项目 B 通过 MCU 获取） */
    { ACL_FUNC_READ_CPU_TEMP,         ACL_DEVICE_MCU,       0, true  },  // MCU[0]
    { ACL_FUNC_READ_BOARD_TEMP,       ACL_DEVICE_MCU,       0, true  },
    { ACL_FUNC_READ_VOLTAGE_12V,      ACL_DEVICE_MCU,       0, true  },
    { ACL_FUNC_READ_FAN_SPEED,        ACL_DEVICE_MCU,       0, false },  // 项目 B 无风扇
    
    /* 本地控制功能 → MCU/CPLD 设备 */
    { ACL_FUNC_WRITE_CPLD_REG,        ACL_DEVICE_CPLD,      0, true  },
    { ACL_FUNC_READ_CPLD_STATUS,      ACL_DEVICE_CPLD,      0, true  },
    { ACL_FUNC_FEED_WATCHDOG,         ACL_DEVICE_MCU,       1, true  },
    { ACL_FUNC_TRIGGER_SYSTEM_RESET,  ACL_DEVICE_MCU,       0, true  },
};

const acl_config_t g_acl_config = {
    .configs = g_acl_configs,
    .count   = sizeof(g_acl_configs) / sizeof(g_acl_configs[0])
};
```

**关键点**：
- 同一业务功能 `ACL_FUNC_READ_CPU_TEMP` 在项目 A 映射到 `BMC[0]`，在项目 B 映射到 `MCU[0]`
- APP 代码完全不需要修改，只需要更换 ACL 配置文件
- PDL 根据 ACL 返回的设备类型和索引，调用对应的驱动接口

### 2.4 ACL 实现

```c
/************************************************************************
 * acl/src/acl.c
 * ACL 核心实现
 ************************************************************************/

#include "acl.h"
#include "osal.h"
#include <string.h>

/* 全局 ACL 查找表（O(1) 直接索引） */
static acl_function_config_t g_acl_lookup[ACL_FUNC_MAX];
static bool g_acl_initialized = false;

/**
 * @brief 初始化 ACL 层
 * 
 * 性能优化：构建查找表，使用枚举值作为数组索引，实现 O(1) 查找
 */
int32_t ACL_Init(const acl_config_t *config)
{
    if (config == NULL || config->configs == NULL || config->count == 0) {
        return OSAL_ERR_INVALID_PARAM;
    }
    
    /* 清空查找表 */
    memset(g_acl_lookup, 0, sizeof(g_acl_lookup));
    
    /* 构建查找表：使用枚举值作为数组索引 */
    for (uint32_t i = 0; i < config->count; i++) {
        acl_function_t func = config->configs[i].function;
        
        if (func >= ACL_FUNC_MAX) {
            LOG_ERROR("ACL", "Invalid function enum value: %d", func);
            continue;
        }
        
        /* 直接索引存储，O(1) 查找 */
        g_acl_lookup[func] = config->configs[i];
    }
    
    g_acl_initialized = true;
    
    LOG_INFO("ACL", "ACL initialized with %u function mappings (O(1) lookup)", config->count);
    return OSAL_SUCCESS;
}

/**
 * @brief 根据业务功能获取设备索引
 * 
 * 性能：O(1) 直接索引，无需遍历
 */
int32_t ACL_GetDeviceIndex(acl_function_t function,
                           acl_device_type_t *device_type,
                           uint32_t *logic_index)
{
    if (!g_acl_initialized) {
        return OSAL_ERR_NOT_INITIALIZED;
    }
    
    if (function >= ACL_FUNC_MAX || device_type == NULL || logic_index == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }
    
    /* O(1) 直接索引查找 */
    const acl_function_config_t *cfg = &g_acl_lookup[function];
    
    /* 检查是否已配置（function 字段为 0 表示未配置） */
    if (cfg->function != function) {
        return OSAL_ERR_NOT_FOUND;
    }
    
    /* 检查是否启用 */
    if (!cfg->enabled) {
        return OSAL_ERR_DISABLED;
    }
    
    *device_type = cfg->device_type;
    *logic_index = cfg->logic_index;
    return OSAL_SUCCESS;
}

/**
 * @brief 检查功能是否启用
 * 
 * 性能：O(1) 直接索引
 */
bool ACL_IsFunctionEnabled(acl_function_t function)
{
    if (!g_acl_initialized || function >= ACL_FUNC_MAX) {
        return false;
    }
    
    /* O(1) 直接索引查找 */
    const acl_function_config_t *cfg = &g_acl_lookup[function];
    
    /* 检查是否已配置 */
    if (cfg->function != function) {
        return false;
    }
    
    return cfg->enabled;
}

/**
 * @brief 获取功能名称（调试用）
 */
const char* ACL_GetFunctionName(acl_function_t function)
{
    static const char *names[] = {
        [ACL_FUNC_SAT_SEND_TELEMETRY]    = "SAT_SEND_TELEMETRY",
        [ACL_FUNC_SAT_RECV_COMMAND]      = "SAT_RECV_COMMAND",
        [ACL_FUNC_SAT_SEND_HEARTBEAT]    = "SAT_SEND_HEARTBEAT",
        [ACL_FUNC_SERVER_POWER_ON]       = "SERVER_POWER_ON",
        [ACL_FUNC_SERVER_POWER_OFF]      = "SERVER_POWER_OFF",
        [ACL_FUNC_SERVER_RESET]          = "SERVER_RESET",
        [ACL_FUNC_READ_CPU_TEMP]         = "READ_CPU_TEMP",
        [ACL_FUNC_READ_BOARD_TEMP]       = "READ_BOARD_TEMP",
        [ACL_FUNC_READ_VOLTAGE_12V]      = "READ_VOLTAGE_12V",
        [ACL_FUNC_READ_VOLTAGE_5V]       = "READ_VOLTAGE_5V",
        [ACL_FUNC_READ_FAN_SPEED]        = "READ_FAN_SPEED",
        [ACL_FUNC_WRITE_CPLD_REG]        = "WRITE_CPLD_REG",
        [ACL_FUNC_READ_CPLD_STATUS]      = "READ_CPLD_STATUS",
        [ACL_FUNC_FEED_WATCHDOG]         = "FEED_WATCHDOG",
        [ACL_FUNC_TRIGGER_SYSTEM_RESET]  = "TRIGGER_SYSTEM_RESET",
    };
    
    if (function < ACL_FUNC_MAX) {
        return names[function];
    }
    
    return "UNKNOWN";
}
```

---

## 3. PCL 层设计（外设配置层）

**说明**：PCL 层负责存储硬件配置，ACL 通过逻辑索引查询 PCL 获取具体硬件参数。

**重要**：PCL 中的 `pcl_app.h` 和 APP 配置功能将被移除，由 ACL 层完全替代。

**架构优化**：统一 PCL 和 PDL 的配置结构，避免类型转换开销。

### 3.1 PCL 统一配置结构

```c
/************************************************************************
 * pcl/include/peripheral/pcl_common.h
 * PCL 通用配置类型（与 PDL 接口对齐）
 ************************************************************************/

/**
 * @brief 硬件接口类型
 */
typedef enum {
    PCL_HW_INTERFACE_CAN = 0,
    PCL_HW_INTERFACE_UART,
    PCL_HW_INTERFACE_I2C,
    PCL_HW_INTERFACE_SPI,
    PCL_HW_INTERFACE_ETHERNET,
    PCL_HW_INTERFACE_MAX
} pcl_hw_interface_type_t;

/**
 * @brief 统一设备配置结构（PDL 直接使用）
 * 
 * 设计原则：
 * - PCL 和 PDL 使用相同的配置结构，避免类型转换
 * - 所有设备类型（MCU/BMC/Satellite）共享此结构
 * - 通过 interface_type 区分不同的硬件接口
 */
typedef struct {
    const char *name;             /* 设备名称 */
    const char *description;      /* 描述信息 */
    bool        enabled;          /* 是否启用 */

    /* 硬件通信接口配置 */
    pcl_hw_interface_type_t interface_type;
    union {
        pcl_can_cfg_t       can;
        pcl_uart_cfg_t      uart;
        pcl_i2c_cfg_t       i2c;
        pcl_spi_cfg_t       spi;
        pcl_ethernet_cfg_t  ethernet;
    } interface;

    /* 通用设备配置 */
    uint32_t cmd_timeout_ms;      /* 命令超时时间 */
    uint32_t retry_count;         /* 重试次数 */
    
    /* GPIO控制（可选） */
    pcl_gpio_config_t *reset_gpio;
    pcl_gpio_config_t *irq_gpio;
} pcl_device_config_t;
```

### 3.2 PCL 设备特定配置

```c
/************************************************************************
 * pcl/include/peripheral/pcl_mcu.h
 * MCU 硬件配置类型
 ************************************************************************/

/**
 * @brief MCU外设配置（基于统一配置结构）
 */
typedef struct {
    pcl_device_config_t base;     /* 基础配置 */
    
    /* MCU特定配置 */
    bool     enable_crc;          /* 是否启用CRC校验 */
    uint32_t watchdog_timeout_ms; /* 看门狗超时时间 */
} pcl_mcu_cfg_t;
```

```c
/************************************************************************
 * pcl/include/peripheral/pcl_bmc.h
 * BMC 硬件配置类型
 ************************************************************************/

/**
 * @brief BMC协议类型
 */
typedef enum {
    PCL_BMC_PROTOCOL_IPMI = 0,
    PCL_BMC_PROTOCOL_REDFISH,
    PCL_BMC_PROTOCOL_MAX
} pcl_bmc_protocol_t;

/**
 * @brief BMC外设配置（基于统一配置结构）
 */
typedef struct {
    pcl_device_config_t base;     /* 基础配置 */
    
    /* BMC特定配置 */
    pcl_bmc_protocol_t protocol;  /* 协议类型 */
    
    /* 网络配置（IPMI over LAN / Redfish） */
    struct {
        const char *ip_addr;
        uint16_t    port;
        const char *username;
        const char *password;
    } network;
    
    /* 串口配置（IPMI over Serial，备份通道） */
    struct {
        const char *device;
        uint32_t    baudrate;
    } serial;
    
    bool auto_failover;           /* 自动故障切换 */
    uint32_t failover_threshold;  /* 故障切换阈值 */
} pcl_bmc_cfg_t;
```

```c
/************************************************************************
 * pcl/include/peripheral/pcl_satellite.h
 * Satellite 硬件配置类型
 ************************************************************************/

/**
 * @brief Satellite外设配置（基于统一配置结构）
 */
typedef struct {
    pcl_device_config_t base;     /* 基础配置 */
    
    /* Satellite特定配置 */
    uint32_t telemetry_interval_ms;  /* 遥测发送间隔 */
    uint32_t heartbeat_interval_ms;  /* 心跳间隔 */
    bool     enable_encryption;      /* 是否启用加密 */
} pcl_satellite_cfg_t;
```

**关键优化**：
1. **统一基础结构** - 所有设备配置都基于 `pcl_device_config_t`
2. **PDL 直接使用** - PDL 接口直接接受 `pcl_device_config_t*`，无需类型转换
3. **扩展性好** - 设备特定配置通过继承方式扩展

    pcl_gpio_config_t *power_gpio;
    pcl_gpio_config_t *reset_gpio;
} pcl_bmc_cfg_t;
```

```c
/************************************************************************
 * pcl/include/peripheral/pcl_satellite.h
 * Satellite 硬件配置类型
 ************************************************************************/

/**
 * @brief Satellite外设配置
 */
typedef struct {
    const char *name;
    const char *description;
    bool        enabled;

    /* 硬件通信接口配置 */
    pcl_hw_interface_type_t interface_type;
    union {
        pcl_can_cfg_t can;
        /* 未来可扩展 SpaceWire, 1553B */
    } interface_cfg;

    uint32_t heartbeat_interval_ms;
    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
} pcl_satellite_cfg_t;
```

### 3.2 PCL 配置文件示例

```c
/************************************************************************
 * pcl/platform/ti/am6254/carrier_board_v1/hw_config.c
 * TI AM6254 桥接板硬件配置
 ************************************************************************/

/* MCU 设备配置数组（索引 = logic_index）*/
static pcl_mcu_cfg_t g_mcu_configs[] = {
    /* logic_index = 0: 主 MCU（温度传感器）*/
    {
        .name        = "MCU_MAIN",
        .description = "Main MCU for temperature sensing",
        .enabled     = true,
        .interface_type = PCL_HW_INTERFACE_I2C,
        .interface_cfg.i2c = {
            .bus_path    = "/dev/i2c-1",
            .device_addr = 0x48,
            .speed_hz    = 100000
        },
        .cmd_timeout_ms = 1000,
        .retry_count    = 3,
        .enable_crc     = true
    },
    /* logic_index = 1: CPLD 控制器 */
    {
        .name        = "CPLD_CTRL",
        .description = "CPLD Controller",
        .enabled     = true,
        .interface_type = PCL_HW_INTERFACE_I2C,
        .interface_cfg.i2c = {
            .bus_path    = "/dev/i2c-2",
            .device_addr = 0x50,
            .speed_hz    = 100000
        },
        .cmd_timeout_ms = 500,
        .retry_count    = 3,
        .enable_crc     = false
    },
    /* logic_index = 2: 看门狗 MCU */
    {
        .name        = "WATCHDOG_MCU",
        .description = "Watchdog MCU",
        .enabled     = true,
        .interface_type = PCL_HW_INTERFACE_I2C,
        .interface_cfg.i2c = {
            .bus_path    = "/dev/i2c-1",
            .device_addr = 0x60,
            .speed_hz    = 100000
        },
        .cmd_timeout_ms = 2000,
        .retry_count    = 5,
        .enable_crc     = true
    }
};

/* BMC 设备配置数组 */
static pcl_bmc_cfg_t g_bmc_configs[] = {
    /* logic_index = 0: 主服务器 BMC */
    {
        .name       = "SERVER_BMC",
        .description = "Server BMC via Redfish",
        .enabled    = true,
        .primary_channel = {
            .protocol = PCL_BMC_PROTOCOL_REDFISH,
            .cfg.redfish = {
                .base_url  = "https://192.168.1.100",
                .username  = "admin",
                .password  = "admin123",
                .use_https = true
            }
        },
        .cmd_timeout_ms = 5000,
        .retry_count    = 3,
        .failover_threshold = 3
    }
};

/* Satellite 设备配置数组 */
static pcl_satellite_cfg_t g_satellite_configs[] = {
    /* logic_index = 0: 卫星平台 CAN 接口 */
    {
        .name        = "SAT_PLATFORM",
        .description = "Satellite Platform CAN Interface",
        .enabled     = true,
        .interface_type = PCL_HW_INTERFACE_CAN,
        .interface_cfg.can = {
            .device      = "can0",
            .bitrate     = 500000,
            .sample_point = 875
        },
        .heartbeat_interval_ms = 1000,
        .cmd_timeout_ms        = 3000,
        .retry_count           = 3
    }
};
```

### 3.3 PCL 接口

```c
/************************************************************************
 * pcl/include/pcl.h
 * PCL 公共接口
 ************************************************************************/

/**
 * @brief 初始化 PCL 层
 *
 * @param[in] config 平台配置
 * @return OSAL_SUCCESS 成功
 */
int32_t PCL_Init(const pcl_platform_config_t *config);

/**
 * @brief 根据逻辑索引获取 MCU 配置
 *
 * @param[in] logic_index 逻辑索引
 * @return MCU 配置指针，失败返回 NULL
 */
const pcl_mcu_cfg_t* PCL_GetMcuConfig(uint32_t logic_index);

/**
 * @brief 根据逻辑索引获取 BMC 配置
 */
const pcl_bmc_cfg_t* PCL_GetBmcConfig(uint32_t logic_index);

/**
 * @brief 根据逻辑索引获取 Satellite 配置
 */
const pcl_satellite_cfg_t* PCL_GetSatelliteConfig(uint32_t logic_index);
```
    const char *name;
    const char *description;
    bool        enabled;

    /* 主通道配置 */
    struct {
        pcl_bmc_protocol_t protocol;
        union {
            pcl_bmc_ipmi_lan_cfg_t  ipmi_lan;
            pcl_bmc_redfish_cfg_t   redfish;
        } cfg;
    } primary_channel;

    /* 备份通道配置 */
    struct {
        pcl_bmc_protocol_t protocol;
        pcl_bmc_ipmi_serial_cfg_t cfg;
    } backup_channel;

    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
    uint32_t failover_threshold;

    pcl_gpio_config_t *power_gpio;
    pcl_gpio_config_t *reset_gpio;
} pcl_bmc_cfg_t;

/************************************************************************
* pcl/include/peripheral/pcl_satellite.h
* Satellite 硬件配置类型（已存在）
************************************************************************/

/**
* @brief Satellite外设配置
*/
typedef struct {
    const char *name;
    const char *description;
    bool        enabled;

    /* 硬件通信接口配置 */
    pcl_hw_interface_type_t interface_type;
    union {
        pcl_can_cfg_t can;
        /* 未来可扩展 SpaceWire, 1553B */
    } interface_cfg;

    uint32_t heartbeat_interval_ms;
    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
} pcl_satellite_cfg_t;

### 3.3 PCL 配置文件示例

```c
/************************************************************************
 * pcl/platform/ti/am6254/carrier_board_v1/hw_config.c
 * TI AM6254 桥接板硬件配置
 ************************************************************************/

/* MCU 设备配置数组（索引 = logic_index）*/
static pcl_mcu_cfg_t g_mcu_configs[] = {
    /* logic_index = 0: 主 MCU（温度传感器）*/
    {
        .base = {
            .name        = "MCU_MAIN",
            .description = "Main MCU for temperature sensing",
            .enabled     = true,
            .interface_type = PCL_HW_INTERFACE_I2C,
            .interface.i2c = {
                .bus_path    = "/dev/i2c-1",
                .device_addr = 0x48,
                .speed_hz    = 100000
            },
            .cmd_timeout_ms = 1000,
            .retry_count    = 3,
            .reset_gpio     = NULL,
            .irq_gpio       = NULL
        },
        .enable_crc     = true,
        .watchdog_timeout_ms = 0  /* 非看门狗设备 */
    },
    /* logic_index = 1: 看门狗 MCU */
    {
        .base = {
            .name        = "WATCHDOG_MCU",
            .description = "Watchdog MCU",
            .enabled     = true,
            .interface_type = PCL_HW_INTERFACE_I2C,
            .interface.i2c = {
                .bus_path    = "/dev/i2c-1",
                .device_addr = 0x60,
                .speed_hz    = 100000
            },
            .cmd_timeout_ms = 2000,
            .retry_count    = 5,
            .reset_gpio     = NULL,
            .irq_gpio       = NULL
        },
        .enable_crc     = true,
        .watchdog_timeout_ms = 30000  /* 30秒超时 */
    }
};

/* BMC 设备配置数组 */
static pcl_bmc_cfg_t g_bmc_configs[] = {
    /* logic_index = 0: 主服务器 BMC */
    {
        .base = {
            .name        = "SERVER_BMC",
            .description = "Server BMC via Redfish",
            .enabled     = true,
            .interface_type = PCL_HW_INTERFACE_ETHERNET,
            .interface.ethernet = {
                .ip_addr = "192.168.1.100",
                .port    = 443
            },
            .cmd_timeout_ms = 5000,
            .retry_count    = 3,
            .reset_gpio     = NULL,
            .irq_gpio       = NULL
        },
        .protocol = PCL_BMC_PROTOCOL_REDFISH,
        .network = {
            .ip_addr  = "192.168.1.100",
            .port     = 443,
            .username = "admin",
            .password = "admin123"
        },
        .serial = {
            .device   = "/dev/ttyS1",
            .baudrate = 115200
        },
        .auto_failover = true,
        .failover_threshold = 3
    }
};

/* Satellite 设备配置数组 */
static pcl_satellite_cfg_t g_satellite_configs[] = {
    /* logic_index = 0: 卫星平台 CAN 接口 */
    {
        .base = {
            .name        = "SAT_PLATFORM",
            .description = "Satellite Platform CAN Interface",
            .enabled     = true,
            .interface_type = PCL_HW_INTERFACE_CAN,
            .interface.can = {
                .device      = "can0",
                .bitrate     = 500000,
                .sample_point = 875
            },
            .cmd_timeout_ms = 3000,
            .retry_count    = 3,
            .reset_gpio     = NULL,
            .irq_gpio       = NULL
        },
        .telemetry_interval_ms = 1000,
        .heartbeat_interval_ms = 1000,
        .enable_encryption     = false
    }
};

/* PCL 查询接口实现 */
const pcl_device_config_t* PCL_GetMCUConfig(uint32_t logic_index)
{
    if (logic_index >= sizeof(g_mcu_configs) / sizeof(g_mcu_configs[0])) {
        return NULL;
    }
    return &g_mcu_configs[logic_index].base;
}

const pcl_device_config_t* PCL_GetBMCConfig(uint32_t logic_index)
{
    if (logic_index >= sizeof(g_bmc_configs) / sizeof(g_bmc_configs[0])) {
        return NULL;
    }
    return &g_bmc_configs[logic_index].base;
}

const pcl_device_config_t* PCL_GetSatelliteConfig(uint32_t logic_index)
{
    if (logic_index >= sizeof(g_satellite_configs) / sizeof(g_satellite_configs[0])) {
        return NULL;
    }
    return &g_satellite_configs[logic_index].base;
}
```

**关键改进**：
1. 使用统一的 `pcl_device_config_t` 基础结构
2. PCL 查询接口返回基础配置指针，PDL 可直接使用
3. 避免了类型转换和数据拷贝

---

## 4. PDL 层设计（外设驱动层）

**说明**：PDL 层提供三个完全独立的服务（Satellite/BMC/MCU），每个服务有自己的接口和实现。

**架构优化**：PDL 接口直接使用 PCL 的统一配置结构，避免类型转换。

### 4.1 PDL 接口设计

```c
/************************************************************************
 * pdl/include/pdl_mcu.h
 * MCU 驱动接口（直接使用 PCL 配置结构）
 ************************************************************************/

/**
 * @brief 初始化 MCU 服务
 *
 * @param[in] config PCL 配置结构（直接使用，无需转换）
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_MCU_Init(const pcl_device_config_t *config,
                     mcu_handle_t *handle);

/**
 * @brief 反初始化 MCU 服务
 */
int32_t PDL_MCU_Deinit(mcu_handle_t handle);

/**
 * @brief 读取传感器数据
 */
int32_t PDL_MCU_ReadSensor(mcu_handle_t handle,
                          uint32_t sensor_id,
                          float *value);

/**
 * @brief 写入寄存器
 */
int32_t PDL_MCU_WriteRegister(mcu_handle_t handle,
                             uint32_t reg_addr,
                             uint32_t value);

/**
 * @brief 喂看门狗
 */
int32_t PDL_MCU_FeedWatchdog(mcu_handle_t handle);
```

```c
/************************************************************************
 * pdl/include/pdl_bmc.h
 * BMC 驱动接口（直接使用 PCL 配置结构）
 ************************************************************************/

/**
 * @brief 初始化 BMC 服务
 *
 * @param[in] config PCL 配置结构（直接使用，无需转换）
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_Init(const pcl_device_config_t *config,
                     bmc_handle_t *handle);

/**
 * @brief 电源控制
 */
int32_t PDL_BMC_PowerOn(bmc_handle_t handle);
int32_t PDL_BMC_PowerOff(bmc_handle_t handle);
int32_t PDL_BMC_PowerReset(bmc_handle_t handle);

/**
 * @brief 读取传感器
 */
int32_t PDL_BMC_ReadSensors(bmc_handle_t handle,
                           bmc_sensor_type_t type,
                           bmc_sensor_reading_t *readings,
                           uint32_t max_count,
                           uint32_t *actual_count);
```

```c
/************************************************************************
 * pdl/include/pdl_satellite.h
 * Satellite 驱动接口（直接使用 PCL 配置结构）
 ************************************************************************/

/**
 * @brief 初始化 Satellite 服务
 *
 * @param[in] config PCL 配置结构（直接使用，无需转换）
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_Init(const pcl_device_config_t *config,
                          satellite_handle_t *handle);

/**
 * @brief 发送遥测数据
 */
int32_t PDL_Satellite_SendTelemetry(satellite_handle_t handle,
                                   const telemetry_packet_t *packet);

/**
 * @brief 接收遥控指令
 */
int32_t PDL_Satellite_ReceiveCommand(satellite_handle_t handle,
                                    command_packet_t *packet,
                                    uint32_t timeout_ms);
```

**关键改进**：
1. PDL 接口直接接受 `const pcl_device_config_t*`
2. 无需类型转换，避免数据拷贝
3. 配置结构在 PCL 和 PDL 之间完全对齐

### 4.1 PDL 设计原则

**关键原则**：Satellite/BMC/MCU 是完全独立的服务，不做统一抽象。

- **PDL_Satellite**：卫星平台通信服务（CAN 协议、命令解析、心跳）
- **PDL_BMC**：服务器 BMC 管理服务（Redfish/IPMI、电源控制、传感器）
- **PDL_MCU**：本地 MCU 控制服务（I2C/SPI 通信、寄存器读写）

每个服务有独立的接口、独立的状态机、独立的业务逻辑。

### 4.2 PDL 使用 ACL + PCL 的方式

```c
/************************************************************************
 * pdl/src/pdl_bmc/pdl_bmc_core.c
 * BMC 服务实现（示例）
 ************************************************************************/

int32_t PDL_BMC_Init(bmc_service_handle_t *handle)
{
    /* 1. 从 ACL 获取逻辑索引 */
    acl_device_type_t device_type;
    uint32_t logic_index;
    int32_t ret = ACL_GetDeviceIndex(ACL_FUNC_SERVER_POWER_ON,
                                     &device_type, &logic_index);
    if (ret != OSAL_SUCCESS || device_type != ACL_DEVICE_BMC) {
        LOG_ERROR("PDL_BMC", "Failed to get BMC device index");
        return OSAL_ERR_NOT_FOUND;
    }

    /* 2. 从 PCL 获取硬件配置 */
    const pcl_bmc_cfg_t *hw_config = PCL_GetBmcConfig(logic_index);
    if (hw_config == NULL) {
        LOG_ERROR("PDL_BMC", "BMC config not found for index %u", logic_index);
        return OSAL_ERR_NOT_FOUND;
    }

    /* 3. 使用硬件配置初始化 BMC 连接 */
    LOG_INFO("PDL_BMC", "Connecting to BMC: %s", hw_config->name);
    
    if (hw_config->primary_channel.protocol == PCL_BMC_PROTOCOL_REDFISH) {
        const pcl_bmc_redfish_cfg_t *redfish = &hw_config->primary_channel.cfg.redfish;
        LOG_INFO("PDL_BMC", "Using Redfish at %s", redfish->base_url);
        /* 初始化 Redfish 客户端 */
    }

    return OSAL_SUCCESS;
}

int32_t PDL_BMC_PowerOn(bmc_service_handle_t handle)
{
    /* 业务逻辑：发送 Redfish 电源控制命令 */
    return OSAL_SUCCESS;
}
```

**说明**：
- PDL 初始化时使用 ACL 枚举（如 `ACL_FUNC_SERVER_POWER_ON`）查询设备索引
- 获取到 `logic_index` 后，通过 PCL 接口获取具体硬件配置
- 这样实现了业务功能与硬件配置的完全解耦

---

## 5. APP 层使用示例

### 5.1 sat_comm 线程（卫星通信）

```c
/************************************************************************
 * apps/sat_comm/sat_comm_thread.c
 * 卫星通信线程
 ************************************************************************/

/* 全局运行标志 */
static volatile bool g_running = true;

static void *sat_comm_thread_entry(void *arg)
{
    satellite_service_handle_t sat_handle;

    /* 初始化卫星服务（PDL 内部查询 ACL + PCL）*/
    int32_t ret = PDL_Satellite_Init(&sat_handle);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to init satellite service");
        return NULL;
    }

    /* 注册命令回调 */
    PDL_Satellite_RegisterCallback(sat_handle, command_handler, NULL);

    /* 主循环 */
    while (g_running) {
        /* 1. 接收卫星命令（PDL 内部处理）*/

        /* 2. 打包遥测数据 */
        telemetry_packet_t tm_pkt;
        collect_telemetry(&tm_pkt);

        /* 3. 发送遥测（使用 ACL 功能枚举检查）*/
        if (ACL_IsFunctionEnabled(ACL_FUNC_SAT_SEND_TELEMETRY)) {
            PDL_Satellite_SendTelemetry(sat_handle, &tm_pkt);
        }

        /* 4. 更新心跳 */
        watchdog_update_heartbeat(THREAD_ID_SAT_COMM);

        OSAL_msleep(100);
    }

    PDL_Satellite_Deinit(sat_handle);
    return NULL;
}


### 5.2 server 线程（服务器管理）

```c
/************************************************************************
 * apps/server/server_thread.c
 * 服务器管理线程
 ************************************************************************/

static void *server_thread_entry(void *arg)
{
    bmc_service_handle_t bmc_handle;

    /* 初始化 BMC 服务 */
    int32_t ret = PDL_BMC_Init(&bmc_handle);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SERVER", "Failed to init BMC service");
        return NULL;
    }

    /* 主循环 */
    while (g_running) {
        /* 1. 从命令队列接收命令 */
        server_cmd_t cmd;
        if (queue_receive(g_server_cmd_queue, &cmd, 100) == OSAL_SUCCESS) {
            handle_server_command(&cmd, bmc_handle);
        }

        /* 2. 周期性采集传感器数据（使用 ACL 检查功能是否启用）*/
        if (ACL_IsFunctionEnabled(ACL_FUNC_SERVER_READ_CPU_TEMP)) {
            float cpu_temp;
            ret = PDL_BMC_ReadSensor(bmc_handle, SENSOR_CPU_TEMP, &cpu_temp);
            if (ret == OSAL_SUCCESS) {
                telemetry_pool_write(TM_SLOT_SERVER_CPU_TEMP, &cpu_temp);
            }
        }

        if (ACL_IsFunctionEnabled(ACL_FUNC_SERVER_READ_VOLTAGE)) {
            float voltage;
            ret = PDL_BMC_ReadSensor(bmc_handle, SENSOR_VOLTAGE, &voltage);
            if (ret == OSAL_SUCCESS) {
                telemetry_pool_write(TM_SLOT_SERVER_VOLTAGE, &voltage);
            }
        }

        /* 3. 更新心跳 */
        watchdog_update_heartbeat(THREAD_ID_SERVER);

        OSAL_msleep(1000);
    }

    PDL_BMC_Deinit(bmc_handle);
    return NULL;
}

static void handle_server_command(const server_cmd_t *cmd,
                                  bmc_service_handle_t bmc_handle)
{
    switch (cmd->type) {
        case CMD_POWER_ON:
            if (ACL_IsFunctionEnabled(ACL_FUNC_SERVER_POWER_ON)) {
                PDL_BMC_PowerOn(bmc_handle);
            }
            break;

        case CMD_POWER_OFF:
            if (ACL_IsFunctionEnabled(ACL_FUNC_SERVER_POWER_OFF)) {
                PDL_BMC_PowerOff(bmc_handle);
            }
            break;

        default:
            LOG_WARN("SERVER", "Unknown command type: %d", cmd->type);
            break;
    }
}
```

### 5.3 local_dev 线程（本地外设）

```c
/************************************************************************
 * apps/local_dev/local_dev_thread.c
 * 本地外设管理线程
 ************************************************************************/

static void *local_dev_thread_entry(void *arg)
{
    mcu_service_handle_t mcu_handle;

    /* 初始化 MCU 服务 */
    int32_t ret = PDL_MCU_Init(&mcu_handle);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LOCAL_DEV", "Failed to init MCU service");
        return NULL;
    }

    /* 主循环 */
    while (g_running) {
        /* 1. 从命令队列接收命令 */
        local_cmd_t cmd;
        if (queue_receive(g_local_cmd_queue, &cmd, 100) == OSAL_SUCCESS) {
            handle_local_command(&cmd, mcu_handle);
        }

        /* 2. 周期性采集本地传感器（使用 ACL 检查功能是否启用）*/
        if (ACL_IsFunctionEnabled(ACL_FUNC_LOCAL_READ_MCU_TEMP)) {
            float mcu_temp;
            ret = PDL_MCU_ReadTemperature(mcu_handle, &mcu_temp);
            if (ret == OSAL_SUCCESS) {
                telemetry_pool_write(TM_SLOT_MCU_TEMP, &mcu_temp);
            }
        }

        if (ACL_IsFunctionEnabled(ACL_FUNC_LOCAL_READ_CPLD_STATUS)) {
            uint32_t cpld_status;
            ret = PDL_MCU_ReadRegister(mcu_handle, CPLD_STATUS_REG, &cpld_status);
            if (ret == OSAL_SUCCESS) {
                telemetry_pool_write(TM_SLOT_CPLD_STATUS, &cpld_status);
            }
        }

        /* 3. 喂狗（使用 ACL 检查功能是否启用）*/
        if (ACL_IsFunctionEnabled(ACL_FUNC_LOCAL_FEED_WATCHDOG)) {
            PDL_MCU_FeedWatchdog(mcu_handle);
        }

        /* 4. 更新心跳 */
        watchdog_update_heartbeat(THREAD_ID_LOCAL_DEV);

        OSAL_msleep(500);
    }

    PDL_MCU_Deinit(mcu_handle);
    return NULL;
}

static void handle_local_command(const local_cmd_t *cmd,
                                mcu_service_handle_t mcu_handle)
{
    switch (cmd->type) {
        case CMD_WRITE_CPLD_REG:
            if (ACL_IsFunctionEnabled(ACL_FUNC_LOCAL_WRITE_CPLD_REG)) {
                PDL_MCU_WriteRegister(mcu_handle, cmd->reg_addr, cmd->value);
            }
            break;

        default:
            LOG_WARN("LOCAL_DEV", "Unknown command type: %d", cmd->type);
            break;
    }
}
```
    return NULL;
}

static void handle_local_command(const local_cmd_t *cmd,
                                mcu_service_handle_t mcu_handle)
{
    switch (cmd->type) {
        case CMD_WRITE_CPLD_REG:
            PDL_MCU_WriteRegister(mcu_handle, cmd->reg_addr, cmd->value);
            break;

        default:
            LOG_WARN("LOCAL_DEV", "Unknown command type: %d", cmd->type);
            break;
    }
}

---
6. 线程模型与看门狗设计

6.1 线程架构

/************************************************************************
* apps/main.c
* 主程序入口
************************************************************************/

/* 线程 ID 定义 */
typedef enum {
    THREAD_ID_SAT_COMM = 0,
    THREAD_ID_SERVER,
    THREAD_ID_LOCAL_DEV,
    THREAD_ID_WATCHDOG,
    THREAD_ID_MAX
} thread_id_t;

/* 线程配置 */
typedef struct {
    const char *name;
    osal_thread_func_t entry;
    uint32_t heartbeat_timeout_ms;
} thread_config_t;

static const thread_config_t g_thread_configs[THREAD_ID_MAX] = {
    {
        .name                = "sat_comm",
        .entry               = sat_comm_thread_entry,
        .heartbeat_timeout_ms = 2000
    },
    {
        .name                = "server",
        .entry               = server_thread_entry,
        .heartbeat_timeout_ms = 5000
    },
    {
        .name                = "local_dev",
        .entry               = local_dev_thread_entry,
        .heartbeat_timeout_ms = 3000
    },
    {
        .name                = "watchdog",
        .entry               = watchdog_thread_entry,
        .heartbeat_timeout_ms = 0   /* 看门狗自己不需要监控 */
    }
};

/* 全局运行标志 */
static volatile bool g_running = true;

/* 线程句柄 */
static osal_thread_t g_thread_ids[THREAD_ID_MAX];

/* 信号处理函数 */
static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT || sig == OS_SIGNAL_TERM) {
        LOG_INFO("MAIN", "Received signal %d, shutting down...", sig);
        g_running = false;
    }
}

int main(int argc, char *argv[])
{
    int32_t ret;

    LOG_INFO("MAIN", "EMS Bridge Application Starting...");

    /* 1. 注册信号处理 */
    OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
    OSAL_SignalRegister(OS_SIGNAL_TERM, signal_handler);

    /* 2. 初始化 PCL */
    ret = PCL_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "Failed to init PCL");
        return -1;
    }

    /* 3. 初始化 IPC 资源 */
    ipc_init();

    /* 4. 创建所有业务线程 */
    for (uint32_t i = 0; i < THREAD_ID_MAX; i++) {
        const thread_config_t *cfg = &g_thread_configs[i];
        ret = OSAL_ThreadCreate(&g_thread_ids[i], cfg->entry, NULL);
        if (ret != OSAL_SUCCESS) {
            LOG_ERROR("MAIN", "Failed to create thread %s", cfg->name);
            return -1;
        }
        LOG_INFO("MAIN", "Thread %s created", cfg->name);
    }

    /* 5. 主线程等待退出信号 */
    while (g_running) {
        OSAL_msleep(1000);
    }

    LOG_INFO("MAIN", "Shutting down...");

    /* 6. 等待所有线程退出 */
    for (uint32_t i = 0; i < THREAD_ID_MAX; i++) {
        OSAL_ThreadJoin(g_thread_ids[i]);
        LOG_INFO("MAIN", "Thread %s exited", g_thread_configs[i].name);
    }

    /* 7. 清理资源 */
    ipc_cleanup();

    LOG_INFO("MAIN", "Application exited");
    return 0;
}

6.2 看门狗线程设计

/************************************************************************
* apps/watchdog/watchdog_thread.c
* 看门狗线程（线程级故障隔离）
************************************************************************/

/* 心跳记录 */
typedef struct {
    _Atomic uint64_t last_heartbeat_ms;  /* 最后心跳时间戳 */
    uint32_t         timeout_ms;         /* 超时阈值 */
    uint32_t         failure_count;      /* 连续失败次数 */
    bool             enabled;            /* 是否启用监控 */
} heartbeat_record_t;

static heartbeat_record_t g_heartbeat_records[THREAD_ID_MAX];

/* 故障处理策略 */
#define MAX_RESTART_ATTEMPTS  3
#define SYSTEM_RESET_THRESHOLD 5

static uint32_t g_total_failure_count = 0;

void watchdog_update_heartbeat(thread_id_t thread_id)
{
    if (thread_id >= THREAD_ID_MAX) {
        return;
    }

    uint64_t now_ms = OSAL_GetTimeMs();
    atomic_store(&g_heartbeat_records[thread_id].last_heartbeat_ms, now_ms);
}

static void *watchdog_thread_entry(void *arg)
{
    /* 初始化心跳记录 */
    for (uint32_t i = 0; i < THREAD_ID_MAX; i++) {
        if (i == THREAD_ID_WATCHDOG) {
            g_heartbeat_records[i].enabled = false;
            continue;
        }

        g_heartbeat_records[i].timeout_ms = g_thread_configs[i].heartbeat_timeout_ms;
        g_heartbeat_records[i].failure_count = 0;
        g_heartbeat_records[i].enabled = true;

        uint64_t now_ms = OSAL_GetTimeMs();
        atomic_store(&g_heartbeat_records[i].last_heartbeat_ms, now_ms);
    }

    LOG_INFO("WATCHDOG", "Watchdog thread started");

    /* 主循环 */
    while (g_running) {
        uint64_t now_ms = OSAL_GetTimeMs();

        for (uint32_t i = 0; i < THREAD_ID_MAX; i++) {
            if (!g_heartbeat_records[i].enabled) {
                continue;
            }

            uint64_t last_hb = atomic_load(&g_heartbeat_records[i].last_heartbeat_ms);
            uint64_t elapsed_ms = now_ms - last_hb;

            if (elapsed_ms > g_heartbeat_records[i].timeout_ms) {
                /* 检测到线程挂死 */
                LOG_ERROR("WATCHDOG", "Thread %s timeout (elapsed: %llu ms)",
                         g_thread_configs[i].name, (unsigned long long)elapsed_ms);

                g_heartbeat_records[i].failure_count++;
                g_total_failure_count++;

                /* 故障处理 */
                if (g_heartbeat_records[i].failure_count <= MAX_RESTART_ATTEMPTS) {
                    /* 尝试重启线程 */
                    LOG_WARN("WATCHDOG", "Restarting thread %s (attempt %u/%u)",
                            g_thread_configs[i].name,
                            g_heartbeat_records[i].failure_count,
                            MAX_RESTART_ATTEMPTS);

                    restart_thread(i);

                    /* 重置心跳时间戳 */
                    atomic_store(&g_heartbeat_records[i].last_heartbeat_ms, now_ms);
                } else {
                    /* 超过重启次数，标记为永久失败 */
                    LOG_ERROR("WATCHDOG", "Thread %s failed permanently, disabling monitoring",
                             g_thread_configs[i].name);
                    g_heartbeat_records[i].enabled = false;
                }

                /* 检查是否需要系统复位 */
                if (g_total_failure_count >= SYSTEM_RESET_THRESHOLD) {
                    LOG_FATAL("WATCHDOG", "Too many failures (%u), triggering system reset",
                             g_total_failure_count);
                    trigger_system_reset();
                }
            }
        }

        /* 看门狗检查周期 */
        OSAL_msleep(500);
    }

    LOG_INFO("WATCHDOG", "Watchdog thread exiting");
    return NULL;
}

static void restart_thread(thread_id_t thread_id)
{
    /* 线程重启的正确做法：
     * 1. 设置退出标志，通知线程优雅退出
     * 2. 等待线程自行退出（带超时）
     * 3. 如果超时仍未退出，记录错误并触发系统复位
     * 4. 清理线程资源
     * 5. 重新创建线程
     * 
     * 注意：不要使用 pthread_cancel()，它不安全！
     */
    
    thread_context_t *ctx = &g_thread_contexts[thread_id];
    
    LOG_WARN("WATCHDOG", "Attempting to restart thread %d", thread_id);
    
    /* 1. 设置退出标志 */
    ctx->should_exit = true;
    
    /* 2. 等待线程自行退出（最多10秒）*/
    int32_t wait_count = 0;
    const int32_t max_wait = 100;  /* 10秒 */
    
    while (wait_count < max_wait) {
        if (ctx->thread == 0) {
            /* 线程已退出 */
            break;
        }
        OSAL_msleep(100);
        wait_count++;
    }
    
    /* 3. 检查线程是否成功退出 */
    if (ctx->thread != 0) {
        /* 线程未能优雅退出，可能持有锁或资源 */
        LOG_FATAL("WATCHDOG", "Thread %d failed to exit gracefully, system reset required", thread_id);
        
        /* 记录故障并触发系统复位 */
        g_total_failure_count++;
        trigger_system_reset();
        return;
    }
    
    /* 4. 清理线程资源 */
    ctx->failure_count = 0;
    ctx->should_exit = false;
    
    /* 5. 重新创建线程 */
    int32_t ret = OSAL_ThreadCreate(&ctx->thread, ctx->entry_func, ctx->arg);
    if (ret != OSAL_SUCCESS) {
        LOG_FATAL("WATCHDOG", "Failed to recreate thread %d: %d", thread_id, ret);
        trigger_system_reset();
        return;
    }
    
    LOG_INFO("WATCHDOG", "Thread %d restarted successfully", thread_id);
}

static void trigger_system_reset(void)
{
    /* 1. 记录致命错误到持久存储 */
    log_fatal_error("WATCHDOG_SYSTEM_RESET", g_total_failure_count);

    /* 2. 停止硬件看门狗喂狗，让硬件看门狗超时复位系统 */
    LOG_FATAL("WATCHDOG", "Stopping hardware watchdog feed, system will reset");
    
    /* 3. 等待硬件看门狗超时（通常 5-10 秒）*/
    OSAL_msleep(10000);

    /* 4. 如果硬件看门狗不可用，使用软件复位 */
    LOG_FATAL("WATCHDOG", "Hardware watchdog failed, using software reset");
    
    /* 注意：直接调用 reboot() 需要 root 权限，应该封装在 OSAL 层 */
    /* OSAL_SystemReboot(); */
}

---
7. IPC 机制设计

7.1 命令队列（SPSC 无锁队列）

/************************************************************************
* ipc/include/spsc_queue.h
* 单生产者单消费者无锁队列
************************************************************************/

typedef struct {
  void            *buffer;        /* 环形缓冲区 */
  uint32_t         elem_size;     /* 元素大小 */
  uint32_t         capacity;      /* 容量（2的幂）*/
  _Atomic uint32_t write_idx;     /* 写索引 */
  _Atomic uint32_t read_idx;      /* 读索引 */
} spsc_queue_t;

/**
* @brief 创建 SPSC 队列
*
* @param[out] queue      队列句柄
* @param[in]  elem_size  元素大小
* @param[in]  capacity   容量（必须是2的幂）
* @return OSAL_SUCCESS 成功
*/
int32_t SPSC_QueueCreate(spsc_queue_t **queue, uint32_t elem_size, uint32_t capacity);

/**
* @brief 入队（非阻塞）
*
* @param[in] queue 队列句柄
* @param[in] elem  元素指针
* @return OSAL_SUCCESS 成功
* @return OSAL_ERR_QUEUE_FULL 队列满
*/
int32_t SPSC_QueuePush(spsc_queue_t *queue, const void *elem);

/**
* @brief 出队（非阻塞）
*
* @param[in]  queue 队列句柄
* @param[out] elem  元素指针
* @return OSAL_SUCCESS 成功
* @return OSAL_ERR_QUEUE_EMPTY 队列空
*/
int32_t SPSC_QueuePop(spsc_queue_t *queue, void *elem);

/**
* @brief 出队（阻塞，带超时）
*
* @param[in]  queue      队列句柄
* @param[out] elem       元素指针
* @param[in]  timeout_ms 超时时间（毫秒）
* @return OSAL_SUCCESS 成功
* @return OSAL_ERR_TIMEOUT 超时
*/
int32_t SPSC_QueuePopTimeout(spsc_queue_t *queue, void *elem, uint32_t timeout_ms);

7.2 遥测数据池（双缓冲）

/************************************************************************
* ipc/include/telemetry_pool.h
* 遥测数据池（双缓冲 + 原子索引）
************************************************************************/

/* 遥测槽位定义 */
typedef enum {
  TM_SLOT_SERVER_CPU_TEMP = 0,
  TM_SLOT_SERVER_VOLTAGE,
  TM_SLOT_SERVER_FAN_SPEED,
  TM_SLOT_MCU_TEMP,
  TM_SLOT_CPLD_STATUS,
  TM_SLOT_MAX
} tm_slot_t;

/* 遥测槽位结构（双缓冲）*/
typedef struct {
  uint8_t          buffer_a[256];  /* 缓冲区 A */
  uint8_t          buffer_b[256];  /* 缓冲区 B */
  _Atomic uint32_t active_idx;     /* 当前活跃缓冲区索引（0=A, 1=B）*/
  _Atomic uint64_t timestamp_ms;   /* 最后更新时间戳 */
  uint32_t         data_size;      /* 数据大小 */
} tm_slot_data_t;

/**
* @brief 初始化遥测池
*
* @return OSAL_SUCCESS 成功
*/
int32_t TelemetryPool_Init(void);

/**
* @brief 写入遥测数据（生产者）
*
* @param[in] slot 槽位 ID
* @param[in] data 数据指针
* @param[in] size 数据大小
* @return OSAL_SUCCESS 成功
*/
int32_t TelemetryPool_Write(tm_slot_t slot, const void *data, uint32_t size);

/**
* @brief 读取遥测数据（消费者）
*
* @param[in]  slot      槽位 ID
* @param[out] data      数据缓冲区
* @param[in]  buf_size  缓冲区大小
* @param[out] timestamp 时间戳（可选）
* @return OSAL_SUCCESS 成功
*/
int32_t TelemetryPool_Read(tm_slot_t slot, void *data, uint32_t buf_size,
						 uint64_t *timestamp);

7.3 IPC 实现示例

/************************************************************************
* ipc/src/spsc_queue.c
* SPSC 队列实现
************************************************************************/

int32_t SPSC_QueuePush(spsc_queue_t *queue, const void *elem)
{
  uint32_t write_idx = atomic_load_explicit(&queue->write_idx, memory_order_relaxed);
  uint32_t read_idx = atomic_load_explicit(&queue->read_idx, memory_order_acquire);

  /* 检查队列是否满 */
  uint32_t next_write = (write_idx + 1) & (queue->capacity - 1);
  if (next_write == read_idx) {
	  return OSAL_ERR_QUEUE_FULL;
  }

  /* 写入数据 */
  uint8_t *dst = (uint8_t*)queue->buffer + (write_idx * queue->elem_size);
  OSAL_Memcpy(dst, elem, queue->elem_size);

  /* 更新写索引 */
  atomic_store_explicit(&queue->write_idx, next_write, memory_order_release);

  return OSAL_SUCCESS;
}

int32_t SPSC_QueuePop(spsc_queue_t *queue, void *elem)
{
  uint32_t read_idx = atomic_load_explicit(&queue->read_idx, memory_order_relaxed);
  uint32_t write_idx = atomic_load_explicit(&queue->write_idx, memory_order_acquire);

  /* 检查队列是否空 */
  if (read_idx == write_idx) {
	  return OSAL_ERR_QUEUE_EMPTY;
  }

  /* 读取数据 */
  uint8_t *src = (uint8_t*)queue->buffer + (read_idx * queue->elem_size);
  OSAL_Memcpy(elem, src, queue->elem_size);

  /* 更新读索引 */
  uint32_t next_read = (read_idx + 1) & (queue->capacity - 1);
  atomic_store_explicit(&queue->read_idx, next_read, memory_order_release);

  return OSAL_SUCCESS;
}

/************************************************************************
* ipc/src/telemetry_pool.c
* 遥测池实现
************************************************************************/

static tm_slot_data_t g_tm_slots[TM_SLOT_MAX];

int32_t TelemetryPool_Write(tm_slot_t slot, const void *data, uint32_t size)
{
  if (slot >= TM_SLOT_MAX || size > 256) {
	  return OSAL_ERR_INVALID_PARAM;
  }

  tm_slot_data_t *tm_slot = &g_tm_slots[slot];

  /* 获取当前活跃缓冲区索引 */
  uint32_t active = atomic_load_explicit(&tm_slot->active_idx, memory_order_relaxed);

  /* 写入非活跃缓冲区 */
  uint8_t *buffer = (active == 0) ? tm_slot->buffer_b : tm_slot->buffer_a;
  OSAL_Memcpy(buffer, data, size);
  tm_slot->data_size = size;

  /* 切换活跃缓冲区 */
  uint32_t new_active = 1 - active;
  atomic_store_explicit(&tm_slot->active_idx, new_active, memory_order_release);

  /* 更新时间戳 */
  uint64_t now_ms = OSAL_GetTimeMs();
  atomic_store_explicit(&tm_slot->timestamp_ms, now_ms, memory_order_release);

  return OSAL_SUCCESS;
}

int32_t TelemetryPool_Read(tm_slot_t slot, void *data, uint32_t buf_size,
						 uint64_t *timestamp)
{
  if (slot >= TM_SLOT_MAX) {
	  return OSAL_ERR_INVALID_PARAM;
  }

  tm_slot_data_t *tm_slot = &g_tm_slots[slot];

  /* 获取当前活跃缓冲区索引 */
  uint32_t active = atomic_load_explicit(&tm_slot->active_idx, memory_order_acquire);

  /* 读取活跃缓冲区 */
  uint8_t *buffer = (active == 0) ? tm_slot->buffer_a : tm_slot->buffer_b;
  uint32_t size = (tm_slot->data_size < buf_size) ? tm_slot->data_size : buf_size;
  OSAL_Memcpy(data, buffer, size);

  /* 读取时间戳 */
  if (timestamp != NULL) {
	  *timestamp = atomic_load_explicit(&tm_slot->timestamp_ms, memory_order_acquire);
  }

  return OSAL_SUCCESS;
}

---

## 6. 线程设计

### 6.1 线程模型

本方案采用**单进程多线程**架构，所有业务逻辑运行在同一进程的不同线程中。

**线程列表**：

| 线程名称 | 优先级 | 调度策略 | 主要职责 |
|---------|--------|---------|---------|
| `watchdog_thread` | 最高 (99) | SCHED_FIFO | 监控所有业务线程心跳，检测挂死并重启 |
| `sat_comm_thread` | 高 (80) | SCHED_FIFO | 卫星通信、命令路由、遥测打包 |
| `local_dev_thread` | 中 (60) | SCHED_FIFO | 本地外设控制（MCU/CPLD/FPGA） |
| `server_thread` | 低 (40) | SCHED_FIFO | 服务器 BMC 管理、电源控制 |

**优先级设计原则**：
1. 看门狗线程优先级最高，确保故障检测不被阻塞
2. 卫星通信线程优先级高，保证指令响应及时
3. 本地外设线程中等优先级，平衡实时性和资源占用
4. 服务器管理线程优先级低，允许被高优先级任务抢占

### 6.2 线程生命周期管理

**线程创建**：

```c
/* main.c - 主函数 */
int main(int argc, char *argv[])
{
    int32_t ret;
    
    /* 1. 初始化 OSAL */
    ret = OSAL_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "OSAL initialization failed");
        return -1;
    }
    
    /* 2. 初始化 ACL 配置层 */
    ret = ACL_Init(NULL);  /* 使用默认配置 */
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "ACL initialization failed");
        return -1;
    }
    
    /* 3. 初始化 IPC 资源 */
    ret = IPC_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "IPC initialization failed");
        return -1;
    }
    
    /* 4. 创建业务线程 */
    osal_thread_t sat_comm_tid, server_tid, local_dev_tid;
    
    ret = OSAL_ThreadCreate(&sat_comm_tid, sat_comm_thread_entry, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "Failed to create sat_comm thread");
        return -1;
    }
    
    ret = OSAL_ThreadCreate(&server_tid, server_thread_entry, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "Failed to create server thread");
        return -1;
    }
    
    ret = OSAL_ThreadCreate(&local_dev_tid, local_dev_thread_entry, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "Failed to create local_dev thread");
        return -1;
    }
    
    /* 5. 创建看门狗线程（最后创建，确保所有业务线程已启动） */
    osal_thread_t watchdog_tid;
    ret = OSAL_ThreadCreate(&watchdog_tid, watchdog_thread_entry, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "Failed to create watchdog thread");
        return -1;
    }
    
    /* 6. 注册信号处理（SIGINT/SIGTERM） */
    OSAL_SignalRegister(SIGINT, signal_handler);
    OSAL_SignalRegister(SIGTERM, signal_handler);
    
    /* 7. 等待所有线程退出 */
    OSAL_ThreadJoin(watchdog_tid);
    OSAL_ThreadJoin(sat_comm_tid);
    OSAL_ThreadJoin(server_tid);
    OSAL_ThreadJoin(local_dev_tid);
    
    /* 8. 清理资源 */
    IPC_Deinit();
    ACL_Deinit();
    
    LOG_INFO("MAIN", "Application exited gracefully");
    return 0;
}
```

**线程退出控制**：

```c
/* 全局退出标志 */
static volatile bool g_running = true;

/* 信号处理函数 */
static void signal_handler(int signum)
{
    LOG_INFO("MAIN", "Received signal %d, shutting down...", signum);
    g_running = false;
}

/* 业务线程标准模板 */
static void *sat_comm_thread_entry(void *arg)
{
    int32_t ret;
    
    /* 线程初始化 */
    ret = sat_comm_init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Thread initialization failed");
        return NULL;
    }
    
    LOG_INFO("SAT_COMM", "Thread started");
    
    /* 主循环 */
    while (g_running) {
        /* 处理业务逻辑 */
        sat_comm_process();
        
        /* 更新心跳 */
        heartbeat_update(THREAD_ID_SAT_COMM);
        
        /* 短暂延时，避免 CPU 占用过高 */
        OSAL_msleep(10);
    }
    
    /* 线程清理 */
    sat_comm_deinit();
    
    LOG_INFO("SAT_COMM", "Thread exited");
    return NULL;
}
```

### 6.3 线程优先级设置

**使用 POSIX 实时调度策略**：

```c
#include <pthread.h>
#include <sched.h>

/* 设置线程优先级（在 OSAL_ThreadCreate 内部实现） */
static int32_t set_thread_priority(pthread_t thread, int priority)
{
    struct sched_param param;
    param.sched_priority = priority;
    
    int ret = pthread_setschedparam(thread, SCHED_FIFO, &param);
    if (ret != 0) {
        LOG_ERROR("OSAL", "Failed to set thread priority: %d", ret);
        return OSAL_ERR_GENERIC;
    }
    
    return OSAL_SUCCESS;
}
```

**优先级配置表**：

```c
/* thread_config.h */
typedef enum {
    THREAD_PRIORITY_WATCHDOG   = 99,  /* 最高优先级 */
    THREAD_PRIORITY_SAT_COMM   = 80,  /* 高优先级 */
    THREAD_PRIORITY_LOCAL_DEV  = 60,  /* 中优先级 */
    THREAD_PRIORITY_SERVER     = 40   /* 低优先级 */
} thread_priority_t;
```

### 6.4 看门狗线程设计

**核心职责**：
1. 周期性检查所有业务线程的心跳时间戳
2. 检测到线程挂死时，记录日志并触发线程重启
3. 连续故障超过阈值时，触发系统复位

**心跳机制**：

```c
/* watchdog.h */
typedef enum {
    THREAD_ID_SAT_COMM = 0,
    THREAD_ID_SERVER,
    THREAD_ID_LOCAL_DEV,
    THREAD_ID_MAX
} thread_id_t;

typedef struct {
    uint64_t last_heartbeat_ms;  /* 最后心跳时间戳 */
    uint32_t timeout_ms;         /* 超时阈值 */
    uint32_t fault_count;        /* 连续故障次数 */
    bool     enabled;            /* 是否启用监控 */
} watchdog_entry_t;

/* 全局心跳表 */
static watchdog_entry_t g_watchdog_table[THREAD_ID_MAX] = {
    [THREAD_ID_SAT_COMM]  = { .timeout_ms = 2000, .enabled = true },
    [THREAD_ID_SERVER]    = { .timeout_ms = 5000, .enabled = true },
    [THREAD_ID_LOCAL_DEV] = { .timeout_ms = 3000, .enabled = true }
};

/* 业务线程更新心跳 */
void heartbeat_update(thread_id_t thread_id)
{
    if (thread_id >= THREAD_ID_MAX) {
        return;
    }
    
    uint64_t now_ms = OSAL_GetTimeMs();
    g_watchdog_table[thread_id].last_heartbeat_ms = now_ms;
}

/* 看门狗线程主循环 */
static void *watchdog_thread_entry(void *arg)
{
    LOG_INFO("WATCHDOG", "Watchdog thread started");
    
    while (g_running) {
        uint64_t now_ms = OSAL_GetTimeMs();
        
        for (uint32_t i = 0; i < THREAD_ID_MAX; i++) {
            watchdog_entry_t *entry = &g_watchdog_table[i];
            
            if (!entry->enabled) {
                continue;
            }
            
            /* 检查心跳超时 */
            uint64_t elapsed_ms = now_ms - entry->last_heartbeat_ms;
            if (elapsed_ms > entry->timeout_ms) {
                entry->fault_count++;
                
                LOG_ERROR("WATCHDOG", "Thread %u timeout (elapsed=%llu ms, fault_count=%u)",
                         i, elapsed_ms, entry->fault_count);
                
                /* 连续故障超过 3 次，触发系统复位 */
                if (entry->fault_count >= 3) {
                    LOG_FATAL("WATCHDOG", "Thread %u failed 3 times, triggering system reset", i);
                    system_reset();  /* 触发硬件复位 */
                }
                
                /* 尝试重启线程（简化实现，实际需要更复杂的重启逻辑） */
                thread_restart(i);
            } else {
                /* 心跳正常，清除故障计数 */
                entry->fault_count = 0;
            }
        }
        
        /* 看门狗检查周期：1 秒 */
        OSAL_msleep(1000);
    }
    
    LOG_INFO("WATCHDOG", "Watchdog thread exited");
    return NULL;
}
```

**线程重启机制**：

```c
/* 线程重启（简化实现） */
static void thread_restart(thread_id_t thread_id)
{
    LOG_WARN("WATCHDOG", "Restarting thread %u", thread_id);
    
    /* 实际实现需要：
     * 1. 设置线程退出标志
     * 2. 等待线程退出（超时强制终止）
     * 3. 清理线程资源
     * 4. 重新创建线程
     * 5. 重置心跳时间戳
     */
    
    /* 这里仅重置心跳时间戳，避免连续触发 */
    g_watchdog_table[thread_id].last_heartbeat_ms = OSAL_GetTimeMs();
}
```

### 6.5 线程故障隔离

**隔离机制**：
1. **地址空间共享**：所有线程共享同一地址空间，但通过编程规范避免越界访问
2. **资源独占**：每个线程独占特定硬件资源（如 sat_comm 独占 CAN 总线）
3. **看门狗监控**：通过心跳机制检测线程挂死，实现软件级故障隔离
4. **系统级保护**：硬件看门狗作为最后防线，防止整个进程挂死

**与多进程方案的对比**：

| 隔离维度 | 多进程方案 | 单进程多线程方案 |
|---------|-----------|----------------|
| 内存隔离 | MMU 硬件隔离 | 编程规范隔离 |
| 故障检测 | Supervisor 监控 | 看门狗线程监控 |
| 故障恢复 | 进程重启 | 线程重启 |
| 资源开销 | 大（独立地址空间） | 小（共享地址空间） |
| 调试难度 | 高（多进程 GDB） | 低（单进程 GDB） |

---

## 7. OSAL IPC 设计（集成到操作系统抽象层）

### 7.1 设计理念

**为什么将IPC集成到OSAL层？**

IPC（进程间通信）是操作系统提供的核心功能，应该由OSAL层统一封装，理由如下：

1. **符合OSAL定位**：OSAL负责封装所有操作系统API，包括进程管理和IPC
2. **避免代码重复**：每个业务进程都需要IPC，集成到OSAL避免重复实现
3. **统一接口**：提供一致的IPC接口，简化业务层代码
4. **便于移植**：未来移植到其他RTOS时，只需实现OSAL IPC接口

**分层设计**：

```
OSAL IPC 层次结构：
┌─────────────────────────────────────┐
│  业务层（sat_comm/server/local_dev）│
│  使用高级IPC抽象接口                 │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  OSAL IPC 抽象层（层2）              │
│  • OSAL_IPC_CreateMsgQueue()        │
│  • OSAL_IPC_CreateTelemetryPool()   │
│  • OSAL_IPC_CreateHeartbeatTable()  │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  OSAL IPC 基础层（层1）              │
│  • OSAL_MqOpen/Send/Receive         │
│  • OSAL_ShmOpen/Mmap/Munmap         │
│  • OSAL_SemOpen/Wait/Post           │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  POSIX IPC（Linux内核）              │
│  • mq_open/mq_send/mq_receive       │
│  • shm_open/mmap/munmap             │
│  • sem_open/sem_wait/sem_post       │
└─────────────────────────────────────┘
```

### 7.2 OSAL IPC 目录结构

```
osal/
├── include/
│   ├── ipc/                        # IPC接口（新增）
│   │   ├── osal_mqueue.h           # POSIX消息队列（层1）
│   │   ├── osal_shm.h              # 共享内存（层1）
│   │   ├── osal_sem.h              # POSIX信号量（层1）
│   │   └── osal_ipc.h              # 高级IPC抽象（层2）
│   ├── sys/
│   │   ├── osal_process.h          # 进程管理
│   │   ├── osal_thread.h           # 线程管理
│   │   └── ...
│   └── osal.h                      # 主头文件（包含所有）
└── src/
    └── posix/
        ├── ipc/                    # IPC实现（新增）
        │   ├── osal_mqueue.c       # 消息队列实现
        │   ├── osal_shm.c          # 共享内存实现
        │   ├── osal_sem.c          # 信号量实现
        │   └── osal_ipc.c          # 高级抽象实现
        └── sys/
            ├── osal_process.c      # 进程管理实现
            └── ...
```

### 7.3 层1：基础IPC接口（POSIX封装）

#### 7.3.1 POSIX 消息队列

```c
/************************************************************************
 * osal/include/ipc/osal_mqueue.h
 * POSIX消息队列封装（1:1映射）
 ************************************************************************/

/**
 * @brief 打开/创建POSIX消息队列
 * 
 * @param[in]  name     队列名称（如"/ems_sat_to_server"）
 * @param[in]  flags    标志（O_CREAT | O_RDONLY | O_WRONLY）
 * @param[in]  mode     权限（如0666）
 * @param[in]  maxmsg   最大消息数
 * @param[in]  msgsize  消息大小
 * @param[out] mqd      消息队列描述符
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_MqOpen(const char *name, int32_t flags, uint32_t mode,
                    uint32_t maxmsg, uint32_t msgsize, int32_t *mqd);

/**
 * @brief 发送消息到队列
 * 
 * @param[in] mqd      消息队列描述符
 * @param[in] msg      消息数据
 * @param[in] msglen   消息长度
 * @param[in] prio     优先级（0-31，数字越大优先级越高）
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_MqSend(int32_t mqd, const void *msg, uint32_t msglen, uint32_t prio);

/**
 * @brief 从队列接收消息
 * 
 * @param[in]  mqd         消息队列描述符
 * @param[out] msg         消息缓冲区
 * @param[in]  msglen      缓冲区大小
 * @param[out] prio        接收到的消息优先级
 * @param[in]  timeout_ms  超时时间（-1=阻塞，0=非阻塞，>0=超时）
 * @return 接收到的字节数，失败返回负数
 */
int32_t OSAL_MqReceive(int32_t mqd, void *msg, uint32_t msglen,
                       uint32_t *prio, int32_t timeout_ms);

/**
 * @brief 关闭消息队列
 */
int32_t OSAL_MqClose(int32_t mqd);

/**
 * @brief 删除消息队列
 */
int32_t OSAL_MqUnlink(const char *name);
```

#### 7.3.2 共享内存

```c
/************************************************************************
 * osal/include/ipc/osal_shm.h
 * 共享内存封装（1:1映射）
 ************************************************************************/

/**
 * @brief 打开/创建共享内存对象
 * 
 * @param[in]  name  共享内存名称（如"/ems_telemetry"）
 * @param[in]  flags 标志（O_CREAT | O_RDWR）
 * @param[in]  mode  权限（如0666）
 * @param[out] fd    文件描述符
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_ShmOpen(const char *name, int32_t flags, uint32_t mode, int32_t *fd);

/**
 * @brief 映射共享内存到进程地址空间
 * 
 * @param[out] addr   映射后的地址
 * @param[in]  length 映射长度
 * @param[in]  prot   保护标志（PROT_READ | PROT_WRITE）
 * @param[in]  flags  映射标志（MAP_SHARED）
 * @param[in]  fd     共享内存文件描述符
 * @param[in]  offset 偏移量
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_Mmap(void **addr, uint32_t length, int32_t prot,
                  int32_t flags, int32_t fd, uint32_t offset);

/**
 * @brief 解除共享内存映射
 */
int32_t OSAL_Munmap(void *addr, uint32_t length);

/**
 * @brief 删除共享内存对象
 */
int32_t OSAL_ShmUnlink(const char *name);
```

#### 7.3.3 POSIX 信号量

```c
/************************************************************************
 * osal/include/ipc/osal_sem.h
 * POSIX信号量封装（1:1映射）
 ************************************************************************/

/**
 * @brief 打开/创建POSIX信号量
 * 
 * @param[in]  name  信号量名称（如"/ems_tm_sem"）
 * @param[in]  flags 标志（O_CREAT）
 * @param[in]  mode  权限（如0666）
 * @param[in]  value 初始值
 * @param[out] sem   信号量指针
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_SemOpen(const char *name, int32_t flags, uint32_t mode,
                     uint32_t value, void **sem);

/**
 * @brief 等待信号量（P操作）
 * 
 * @param[in] sem        信号量指针
 * @param[in] timeout_ms 超时时间（-1=阻塞，>0=超时）
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_SemWait(void *sem, int32_t timeout_ms);

/**
 * @brief 释放信号量（V操作）
 */
int32_t OSAL_SemPost(void *sem);

/**
 * @brief 关闭信号量
 */
int32_t OSAL_SemClose(void *sem);

/**
 * @brief 删除信号量
 */
int32_t OSAL_SemUnlink(const char *name);
```

### 7.4 层2：高级IPC抽象

#### 7.4.1 统一消息格式

```c
/************************************************************************
 * osal/include/ipc/osal_ipc.h
 * 高级IPC抽象接口
 ************************************************************************/

/**
 * @brief IPC消息结构（统一格式，256字节）
 */
typedef struct {
    uint16_t msg_type;      /* 消息类型 */
    uint16_t cmd_code;      /* 命令码 */
    uint32_t seq_num;       /* 序列号 */
    uint32_t data_len;      /* 数据长度 */
    uint8_t  data[240];     /* 数据负载 */
} osal_ipc_msg_t;

/**
 * @brief 消息类型枚举
 */
typedef enum {
    OSAL_IPC_MSG_COMMAND = 1,    /* 命令消息 */
    OSAL_IPC_MSG_RESPONSE,       /* 响应消息 */
    OSAL_IPC_MSG_EVENT,          /* 事件消息 */
    OSAL_IPC_MSG_HEARTBEAT       /* 心跳消息 */
} osal_ipc_msg_type_t;
```

#### 7.4.2 消息队列抽象

```c
/**
 * @brief 创建IPC消息队列
 * 
 * @param[in]  name     队列名称（如"sat_to_server"，自动添加"/ems_"前缀）
 * @param[in]  max_msgs 最大消息数（建议10-20）
 * @param[out] queue_id 队列ID
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_CreateMsgQueue(const char *name, uint32_t max_msgs, osal_id_t *queue_id);

/**
 * @brief 打开已存在的消息队列
 */
int32_t OSAL_IPC_OpenMsgQueue(const char *name, bool readonly, osal_id_t *queue_id);

/**
 * @brief 发送IPC消息
 * 
 * @param[in] queue_id 队列ID
 * @param[in] msg      消息结构
 * @param[in] prio     优先级（0=普通，1=高）
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_SendMsg(osal_id_t queue_id, const osal_ipc_msg_t *msg, uint32_t prio);

/**
 * @brief 接收IPC消息
 * 
 * @param[in]  queue_id   队列ID
 * @param[out] msg        消息缓冲区
 * @param[in]  timeout_ms 超时时间（-1=阻塞，0=非阻塞，>0=超时）
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_ReceiveMsg(osal_id_t queue_id, osal_ipc_msg_t *msg, int32_t timeout_ms);

/**
 * @brief 关闭消息队列
 */
int32_t OSAL_IPC_CloseMsgQueue(osal_id_t queue_id);

/**
 * @brief 删除消息队列
 */
int32_t OSAL_IPC_DeleteMsgQueue(const char *name);
```

#### 7.4.3 遥测池抽象（双缓冲共享内存）

```c
/**
 * @brief 遥测槽位结构（双缓冲设计）
 */
typedef struct {
    uint8_t  buffer_a[256];           /* 缓冲区 A */
    uint8_t  buffer_b[256];           /* 缓冲区 B */
    _Atomic uint32_t active_idx;      /* 活跃缓冲区索引（0=A, 1=B） */
    _Atomic uint64_t timestamp_ms;    /* 数据时间戳 */
    uint32_t data_size;               /* 数据大小 */
} osal_telemetry_slot_t;

/**
 * @brief 创建遥测共享内存池
 * 
 * @param[in]  name      池名称（如"telemetry"，自动添加"/ems_"前缀）
 * @param[in]  num_slots 槽位数量
 * @param[out] pool      池指针
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_CreateTelemetryPool(const char *name, uint32_t num_slots, void **pool);

/**
 * @brief 打开已存在的遥测池
 */
int32_t OSAL_IPC_OpenTelemetryPool(const char *name, uint32_t num_slots, void **pool);

/**
 * @brief 写入遥测数据（无锁，双缓冲）
 * 
 * @param[in] pool    池指针
 * @param[in] slot_id 槽位ID
 * @param[in] data    数据
 * @param[in] size    数据大小
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_WriteTelemetry(void *pool, uint32_t slot_id,
                                 const void *data, uint32_t size);

/**
 * @brief 读取遥测数据（无锁，双缓冲）
 * 
 * @param[in]  pool      池指针
 * @param[in]  slot_id   槽位ID
 * @param[out] data      数据缓冲区
 * @param[in]  buf_size  缓冲区大小
 * @param[out] timestamp 数据时间戳
 * @return 读取的字节数，失败返回负数
 */
int32_t OSAL_IPC_ReadTelemetry(void *pool, uint32_t slot_id,
                                void *data, uint32_t buf_size, uint64_t *timestamp);

/**
 * @brief 关闭遥测池
 */
int32_t OSAL_IPC_CloseTelemetryPool(void *pool, uint32_t num_slots);

/**
 * @brief 删除遥测池
 */
int32_t OSAL_IPC_DeleteTelemetryPool(const char *name);
```

#### 7.4.4 心跳表抽象

```c
/**
 * @brief 心跳表项结构
 */
typedef struct {
    _Atomic uint64_t last_heartbeat_ms;  /* 最后心跳时间 */
    char process_name[32];                /* 进程名称 */
    uint32_t timeout_ms;                  /* 超时阈值 */
} osal_heartbeat_entry_t;

/**
 * @brief 创建心跳共享内存表
 * 
 * @param[in]  name          表名称（如"heartbeat"）
 * @param[in]  max_processes 最大进程数
 * @param[out] table         表指针
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_CreateHeartbeatTable(const char *name, uint32_t max_processes, void **table);

/**
 * @brief 打开已存在的心跳表
 */
int32_t OSAL_IPC_OpenHeartbeatTable(const char *name, uint32_t max_processes, void **table);

/**
 * @brief 注册进程到心跳表
 * 
 * @param[in] table        表指针
 * @param[in] process_name 进程名称
 * @param[in] timeout_ms   心跳超时时间
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_RegisterHeartbeat(void *table, const char *process_name, uint32_t timeout_ms);

/**
 * @brief 更新心跳（业务进程调用）
 * 
 * @param[in] table        表指针
 * @param[in] process_name 进程名称
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_IPC_UpdateHeartbeat(void *table, const char *process_name);

/**
 * @brief 读取心跳（Supervisor调用）
 * 
 * @param[in] table        表指针
 * @param[in] process_name 进程名称
 * @return 最后心跳时间戳（毫秒），0表示未注册
 */
uint64_t OSAL_IPC_ReadHeartbeat(void *table, const char *process_name);

/**
 * @brief 关闭心跳表
 */
int32_t OSAL_IPC_CloseHeartbeatTable(void *table, uint32_t max_processes);

/**
 * @brief 删除心跳表
 */
int32_t OSAL_IPC_DeleteHeartbeatTable(const char *name);
```

### 7.5 业务层使用示例

#### 7.5.1 sat_comm进程初始化

```c
/************************************************************************
 * sat_comm_process/src/ipc_init.c
 * 使用OSAL IPC抽象接口
 ************************************************************************/

#include "osal.h"

static osal_id_t g_mq_to_server;
static osal_id_t g_mq_to_local;
static osal_id_t g_mq_from_server;
static osal_id_t g_mq_from_local;
static void *g_telemetry_pool;
static void *g_heartbeat_table;

int32_t sat_comm_ipc_init(void)
{
    int32_t ret;
    
    /* 1. 创建消息队列（发送） */
    ret = OSAL_IPC_CreateMsgQueue("sat_to_server", 10, &g_mq_to_server);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to create queue to server");
        return ret;
    }
    
    ret = OSAL_IPC_CreateMsgQueue("sat_to_local", 10, &g_mq_to_local);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to create queue to local");
        return ret;
    }
    
    /* 2. 打开消息队列（接收） */
    ret = OSAL_IPC_OpenMsgQueue("server_to_sat", true, &g_mq_from_server);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to open queue from server");
        return ret;
    }
    
    ret = OSAL_IPC_OpenMsgQueue("local_to_sat", true, &g_mq_from_local);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to open queue from local");
        return ret;
    }
    
    /* 3. 打开遥测池（只读） */
    ret = OSAL_IPC_OpenTelemetryPool("telemetry", TM_SLOT_MAX, &g_telemetry_pool);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to open telemetry pool");
        return ret;
    }
    
    /* 4. 打开心跳表 */
    ret = OSAL_IPC_OpenHeartbeatTable("heartbeat", 4, &g_heartbeat_table);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to open heartbeat table");
        return ret;
    }
    
    /* 5. 注册自己的心跳 */
    ret = OSAL_IPC_RegisterHeartbeat(g_heartbeat_table, "sat_comm", 5000);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SAT_COMM", "Failed to register heartbeat");
        return ret;
    }
    
    LOG_INFO("SAT_COMM", "IPC initialized successfully");
    return OSAL_SUCCESS;
}
```

#### 7.5.2 发送命令

```c
int32_t sat_comm_send_server_command(uint16_t cmd_code, const void *data, uint32_t len)
{
    osal_ipc_msg_t msg;
    
    /* 填充消息 */
    msg.msg_type = OSAL_IPC_MSG_COMMAND;
    msg.cmd_code = cmd_code;
    msg.seq_num = g_seq_num++;
    msg.data_len = len;
    
    if (len > sizeof(msg.data)) {
        return OSAL_ERR_INVALID_PARAM;
    }
    
    OSAL_Memcpy(msg.data, data, len);
    
    /* 发送消息（使用OSAL封装） */
    return OSAL_IPC_SendMsg(g_mq_to_server, &msg, 0);
}
```

#### 7.5.3 接收命令

```c
void server_process_commands(void)
{
    osal_ipc_msg_t msg;
    int32_t ret;
    
    while (g_running) {
        /* 接收消息（1秒超时） */
        ret = OSAL_IPC_ReceiveMsg(g_mq_from_sat, &msg, 1000);
        
        if (ret == OSAL_SUCCESS) {
            /* 处理命令 */
            server_handle_command(&msg);
        } else if (ret == OSAL_ERR_TIMEOUT) {
            /* 超时，继续循环 */
            continue;
        } else {
            LOG_ERROR("SERVER", "Failed to receive message: %d", ret);
            break;
        }
        
        /* 更新心跳 */
        OSAL_IPC_UpdateHeartbeat(g_heartbeat_table, "server");
    }
}
```

#### 7.5.4 写入遥测

```c
void server_update_telemetry(void)
{
    uint8_t tm_data[256];
    uint32_t offset = 0;
    
    /* 打包遥测数据 */
    // ... 填充 tm_data ...
    
    /* 写入遥测池（使用OSAL封装，无锁） */
    OSAL_IPC_WriteTelemetry(g_telemetry_pool, TM_SLOT_SERVER_STATUS, tm_data, offset);
}
```

#### 7.5.5 读取遥测

```c
void sat_comm_collect_telemetry(void)
{
    uint8_t tm_data[256];
    uint64_t timestamp;
    int32_t ret;
    
    /* 读取server遥测 */
    ret = OSAL_IPC_ReadTelemetry(g_telemetry_pool, TM_SLOT_SERVER_STATUS,
                                 tm_data, sizeof(tm_data), &timestamp);
    if (ret > 0) {
        /* 打包到卫星遥测帧 */
        sat_comm_pack_telemetry(tm_data, ret, timestamp);
    }
    
    /* 读取local_dev遥测 */
    ret = OSAL_IPC_ReadTelemetry(g_telemetry_pool, TM_SLOT_LOCAL_STATUS,
                                 tm_data, sizeof(tm_data), &timestamp);
    if (ret > 0) {
        sat_comm_pack_telemetry(tm_data, ret, timestamp);
    }
}
```

### 7.6 性能特性

**IPC延迟对比**：

| IPC机制 | 延迟 | 吞吐量 | 可靠性 | 适用场景 |
|---------|------|--------|--------|---------|
| POSIX消息队列 | 5-10 μs | ~200K ops/s | ✅ 内核保证 | 命令传递 |
| 共享内存（双缓冲） | 1-2 μs | ~1M ops/s | ✅ 原子操作 | 遥测数据 |
| 共享内存（心跳表） | <1 μs | ~10M ops/s | ✅ 原子操作 | 健康监控 |

**性能优势**：
- ✅ 消息队列：内核保证可靠性，进程崩溃不丢消息
- ✅ 共享内存：无系统调用开销，接近内存访问速度
- ✅ 双缓冲设计：无锁读写，避免锁竞争

**对秒级遥测周期的影响**：
- 遥测周期：1秒 = 1,000,000 μs
- IPC延迟：5-10 μs
- 延迟占比：0.001%（完全可忽略）

---
## 8. 目录结构

### 8.1 ACL 目录结构（新增）

```
acl/
├── include/
│   ├── acl.h                    # ACL统一接口
│   └── acl_types.h              # ACL类型定义（枚举）
├── src/
│   └── acl.c                    # ACL统一实现
└── config/
    ├── ti_am6254_carrier_board_v1/  # 项目A的业务配置
    │   ├── acl_config.c
    │   └── acl_config.h
    ├── ti_am6254_carrier_board_v2/  # 项目B的业务配置
    │   └── acl_config.c
    └── vendor_demo_board/           # Demo项目配置
        └── acl_config.c
```

**设计说明**：
- 项目级组织：ACL 是业务配置，按项目组织
- 配置文件：每个项目一个配置文件，定义业务功能到设备的映射
- 命名规范：`{platform}_{project}/acl_config.c`

### 8.2 PCL 目录结构（现有，简化）

```
pcl/
├── include/
│   ├── pcl.h                    # PCL统一接口
│   ├── pcl_types.h              # PCL类型定义
│   ├── pcl_mcu.h                # MCU配置结构
│   ├── pcl_bmc.h                # BMC配置结构
│   └── pcl_satellite.h          # Satellite配置结构
├── src/
│   └── pcl.c                    # PCL统一实现
└── platform/
    ├── ti/am6254/
    │   ├── carrier_board_v1/    # 项目配置
    │   │   └── hw_config.c      # 硬件配置
    │   └── carrier_board_v2/
    │       └── hw_config.c
    └── vendor_demo/
        └── demo_board/
            └── hw_config.c
```

**设计说明**：
- 三层组织：vendor/chip/project
- 只存储硬件配置（IP、总线、地址等）
- 通过逻辑索引访问

### 8.3 PDL 目录结构（现有）

```
pdl/
├── include/
│   ├── pdl_mcu.h                # MCU驱动接口（独立）
│   ├── pdl_bmc.h                # BMC驱动接口（独立）
│   ├── pdl_satellite.h          # Satellite驱动接口（独立）
│   └── pdl_watchdog.h           # 看门狗接口
├── src/
│   ├── pdl_mcu/                 # MCU驱动实现（独立）
│   │   ├── pdl_mcu_core.c       # 核心模块（对外API）
│   │   ├── pdl_mcu_protocol.c   # 协议模块
│   │   ├── pdl_mcu_i2c.c        # I2C通信模块
│   │   ├── pdl_mcu_spi.c        # SPI通信模块
│   │   ├── pdl_mcu_uart.c       # UART通信模块
│   │   ├── pdl_mcu_can.c        # CAN通信模块
│   │   └── pdl_mcu_internal.h   # 内部头文件
│   ├── pdl_bmc/                 # BMC驱动（独立）
│   │   ├── pdl_bmc_core.c       # 核心模块
│   │   ├── pdl_bmc_protocol.c   # 协议模块
│   │   ├── pdl_bmc_redfish.c    # Redfish通信模块
│   │   ├── pdl_bmc_ipmi.c       # IPMI通信模块
│   │   └── pdl_bmc_internal.h   # 内部头文件
│   └── pdl_satellite/           # Satellite驱动（独立）
│       ├── pdl_satellite_core.c     # 核心模块
│       ├── pdl_satellite_protocol.c # 协议模块
│       ├── pdl_satellite_can.c      # CAN通信模块
│       └── pdl_satellite_internal.h # 内部头文件
└── README.md
```

**设计说明**：
- 完全独立：MCU/BMC/Satellite 三个服务完全独立
- 模块化划分：core（API）+ protocol（协议）+ 通信模块
- 体现"不做统一抽象"的设计原则

### 8.4 IPC 目录结构（新增）

```
ipc/
├── include/
│   ├── spsc_queue.h             # 无锁队列
│   └── telemetry_pool.h         # 遥测池
└── src/
    ├── spsc_queue.c
    └── telemetry_pool.c
```

### 8.5 应用层目录结构（新增）

```
apps/
├── main.c                       # 主程序入口
├── sat_comm/                    # 卫星通信线程
│   └── sat_comm_thread.c
├── server/                      # 服务器管理线程
│   └── server_thread.c
├── local_dev/                   # 本地外设线程
│   └── local_dev_thread.c
└── watchdog/                    # 看门狗线程
    └── watchdog_thread.c
```

### 8.6 完整项目结构

```
ems_bridge/
├── acl/                         # ACL 应用配置层（新增）
├── pcl/                         # PCL 外设配置层（已存在）
├── pdl/                         # PDL 外设驱动层（已存在）
├── hal/                         # HAL 硬件抽象层（已存在）
├── osal/                        # OSAL 操作系统抽象层（已存在）
├── ipc/                         # IPC 通信机制（新增）
├── apps/                        # 应用层（新增桥接板应用）
├── tests/                       # 测试代码（已存在）
├── docs/                        # 文档（已存在）
├── tools/                       # 工具脚本（已存在）
├── CMakeLists.txt
└── README.md
```

---

## 9. 实施计划

| 阶段 | 内容 | 周期 | 产出 |
|------|------|------|------|
| **阶段 0** | **原型验证（新增）** | **1 周** | |
| 0.1 | 实现最小线程框架（4个线程） | 2 天 | 基本线程创建和退出 |
| 0.2 | 实现简单IPC（基于互斥锁） | 2 天 | 命令队列和遥测池 |
| 0.3 | 验证线程间通信可行性 | 1 天 | 原型验证报告 |
| **阶段 1** | **OSAL 层扩展（前置工作）** | **2-3 周** | |
| 1.1 | 增加时间戳接口（OSAL_GetTimeMs） | 0.5 天 | osal_clock.h 扩展 |
| 1.2 | 增加初始化接口（OSAL_Init/Cleanup） | 0.5 天 | osal.h 扩展 |
| 1.3 | 设计线程属性结构（osal_thread_attr_t） | 1 天 | osal_thread.h 扩展 |
| 1.4 | 实现 OSAL_ThreadCreateEx（支持优先级） | 2 天 | osal_thread.c |
| 1.5 | 实现 OSAL_ThreadSetPriority/SetSchedPolicy | 2 天 | osal_thread.c |
| 1.6 | 多平台测试（x86_64/ARM32/ARM64/RISC-V） | 3 天 | 平台兼容性验证 |
| 1.7 | 权限检查和降级策略 | 1 天 | 实时优先级权限处理 |
| 1.8 | OSAL 扩展接口单元测试 | 2 天 | 测试通过 |
| **阶段 2** | **ACL 层开发** | **1 周** | |
| 2.1 | 定义业务功能枚举（acl_function_t） | 1 天 | acl_types.h |
| 2.2 | 实现 ACL 核心接口（O(1)查找优化） | 2 天 | acl.c（直接索引） |
| 2.3 | 编写 ACL 配置文件（项目 A 和项目 B） | 1 天 | acl_config.c |
| 2.4 | ACL 单元测试 | 1 天 | 测试通过 |
| **阶段 3** | **PCL 层重构** | **1.5 周** | |
| 3.1 | 移除 PCL 中的 APP 配置功能（pcl_app.h） | 1 天 | 删除冗余代码 |
| 3.2 | 设计统一配置结构（pcl_device_config_t） | 1 天 | pcl_common.h |
| 3.3 | 重构 PCL 配置结构（与 PDL 接口对齐） | 2 天 | pcl_mcu.h, pcl_bmc.h, pcl_satellite.h |
| 3.4 | 实现 PCL 查询接口（PCL_GetMCU/BMC/Satellite） | 2 天 | pcl_api.c |
| 3.5 | 编写 PCL 配置文件（TI AM6254 项目 A） | 1 天 | hw_config.c |
| 3.6 | PCL 单元测试 | 1 天 | 测试通过 |
| **阶段 4** | **PDL 层适配** | **1 周** | |
| 4.1 | 修改 PDL 接口签名（接受 pcl_device_config_t*） | 1 天 | pdl_mcu.h, pdl_bmc.h, pdl_satellite.h |
| 4.2 | 适配 PDL_MCU 实现（使用统一配置） | 2 天 | pdl_mcu/ 目录 |
| 4.3 | 适配 PDL_BMC 实现（使用统一配置） | 2 天 | pdl_bmc/ 目录 |
| 4.4 | 适配 PDL_Satellite 实现（使用统一配置） | 1 天 | pdl_satellite/ 目录 |
| 4.5 | PDL 集成测试 | 1 天 | 测试通过 |
| **阶段 5** | **IPC 机制开发** | **2 周** | |
| 5.1 | 实现基于互斥锁的 SPSC 队列（先简单实现） | 2 天 | spsc_queue.c（v1） |
| 5.2 | 实现基于互斥锁的双缓冲遥测池 | 2 天 | telemetry_pool.c（v1） |
| 5.3 | 实现心跳表（原子操作） | 1 天 | heartbeat.c |
| 5.4 | IPC 功能测试 | 1 天 | 功能验证 |
| 5.5 | IPC 性能测试（确定是否需要无锁优化） | 1 天 | 性能基准报告 |
| 5.6 | 如需要，优化为无锁实现（缓存行对齐） | 3 天 | spsc_queue.c（v2） |
| 5.7 | 无锁版本性能测试 | 1 天 | 性能对比报告 |
| **阶段 6** | **应用层开发** | **2 周** | |
| 6.1 | 实现 sat_comm 线程（命令路由、遥测打包） | 3 天 | sat_comm_thread.c |
| 6.2 | 实现 server 线程（BMC 管理、传感器采集） | 3 天 | server_thread.c |
| 6.3 | 实现 local_dev 线程（MCU 控制、看门狗喂狗） | 3 天 | local_dev_thread.c |
| 6.4 | 实现看门狗线程（心跳监控、优雅重启） | 2 天 | watchdog_thread.c |
| 6.5 | 实现主程序（线程创建、信号处理） | 2 天 | main.c |
| **阶段 7** | **集成测试与验证** | **2 周** | |
| 7.1 | 功能测试（命令路由、遥测打包、设备控制） | 2 天 | 功能测试报告 |
| 7.2 | 性能测试（IPC 延迟、线程切换、CPU 占用） | 2 天 | 性能测试报告 |
| 7.3 | 故障注入测试（线程挂死、硬件故障） | 2 天 | 故障测试报告 |
| 7.4 | 实时性分析（WCET、优先级反转检测） | 1 天 | 实时性分析报告 |
| 7.5 | 代码审查（MISRA C 检查、静态分析） | 1 天 | 代码质量报告 |
| 7.6 | 修复测试发现的问题 | 4 天 | 问题修复报告 |
| 7.7 | 回归测试 | 2 天 | 最终验证报告 |
| **阶段 8** | **文档和培训（可选）** | **1 周** | |
| 8.1 | 编写用户手册和开发指南 | 3 天 | 文档交付 |
| 8.2 | 团队培训和知识转移 | 2 天 | 培训完成 |
| **总计** | | **12.5-13.5 周** | |

**说明**：
- **阶段 0 为新增项**：原型验证，快速验证架构可行性（1周）
- **阶段 1 周期调整**：OSAL 扩展从 1 周调整为 2-3 周（考虑多平台测试）
- **阶段 4 为优化项**：PDL 不重构，只适配新的配置结构（从 2.5 周降为 1 周）
- **阶段 5 采用渐进式**：先实现互斥锁版本，性能测试后再决定是否优化为无锁
- **阶段 7 增加时间**：集成测试从 1.5 周增加到 2 周，包含更多测试和修复时间
- **总周期**：12.5-13.5 周（更现实的估算）

**关键路径**：
1. 原型验证（1周）→ 确认架构可行性
2. OSAL 扩展（2-3周）→ 基础设施
3. ACL + PCL + PDL（3.5周）→ 配置和驱动层
4. IPC + 应用层（4周）→ 核心功能
5. 集成测试（2周）→ 质量保证

**风险缓解**：
1. **原型验证阶段**：快速发现架构问题，避免后期返工
2. **渐进式 IPC 开发**：先简单后优化，避免过早优化
3. **充足的测试时间**：2周集成测试 + 4天问题修复
4. **多平台测试**：OSAL 扩展阶段包含多平台验证

**说明**：
- **阶段 0 为必须项**：OSAL 扩展是后续开发的基础，必须先完成
- **阶段 7 为可选项**：进程接口扩展用于支持未来可能的多进程架构需求
- **总周期**：不含阶段 7 为 10.5 周，含阶段 7 为 11.5 周
- **关键路径**：OSAL 扩展 → ACL 开发 → PCL 重构 → PDL 重构 → 应用层开发

**重要变更**：
1. 新增阶段 0：OSAL 层扩展（时间戳、线程优先级等必需接口）
2. 阶段 2 增加 PCL 重构任务：移除 APP 配置功能，与 PDL 接口对齐
3. 阶段 3 增加 PDL 接口重新设计任务：确保与 PCL 配置结构兼容
4. 阶段 4 增加 IPC 优化任务：缓存行对齐、内存屏障
5. 阶段 6 增加问题修复时间：预留 3 天处理测试发现的问题

---

## 10. 关键设计决策总结

### 10.1 为什么选择单进程多线程？

**v2.0 多进程方案的问题：**
1. 与现有 OSAL 接口不匹配（OSAL 提供线程接口，不提供进程接口）
2. IPC 复杂度高（消息队列、共享内存、Supervisor）
3. 资源开销大（独立地址空间）
4. 调试困难（多进程日志、故障定位）

**v3.0 单进程多线程的优势：**
1. 使用 OSAL 提供的线程接口，架构一致
2. IPC 简单高效（无锁队列、双缓冲）
3. 资源开销小（共享地址空间）
4. 调试简单（统一日志、单进程调试）
5. 通过看门狗实现线程级故障隔离

### 10.2 为什么需要独立的 ACL 层？

**问题**：APP 直接调用 PDL 接口，与硬件实现强耦合
- 硬件变更（如更换 BMC IP）需要修改 APP 代码
- 功能启用/禁用需要修改 APP 逻辑
- 不同项目需要维护多个 APP 版本

**解决方案**：ACL 层实现业务功能与硬件完全解耦
- 业务功能用枚举定义（编译期类型检查）
- 映射关系在配置文件中定义（运行时可变）
- APP 只关心业务功能，不关心硬件实现

**为什么不使用 PCL 的 pcl_app.h？**

虽然 PCL 已有 `pcl_app.h`，但它不适合航天级应用：

| 特性 | PCL APP 配置 | ACL 层 |
|------|-------------|--------|
| 功能标识 | 字符串 | 枚举（类型安全） |
| 查找性能 | 字符串比较（O(n)） | 直接索引（O(1)） |
| 编译检查 | 无 | 有（拼写错误立即发现） |
| IDE 支持 | 无自动补全 | 有自动补全 |
| 业务语义 | 通用 APP 配置 | 专用业务功能映射 |
| 适用场景 | 通用嵌入式应用 | 航天级实时系统 |

**结论**：新增独立的 ACL 层，使用枚举提供类型安全和 O(1) 性能。

### 10.3 ACL 层的性能优化

**传统实现（O(n) 线性查找）**：
```c
for (uint32_t i = 0; i < config->count; i++) {
    if (config->configs[i].function == function) {
        // 找到了
    }
}
```

**优化实现（O(1) 直接索引）**：
```c
// 使用枚举值作为数组索引
static acl_function_config_t g_acl_lookup[ACL_FUNC_MAX];

// 初始化时构建查找表
for (uint32_t i = 0; i < config->count; i++) {
    acl_function_t func = config->configs[i].function;
    g_acl_lookup[func] = config->configs[i];
}

// O(1) 查找
const acl_function_config_t *cfg = &g_acl_lookup[function];
```

**性能对比**：
- 线性查找：O(n)，平均 10-20 次比较
- 直接索引：O(1)，单次数组访问
- 性能提升：10-20倍

### 10.4 PCL 与 PDL 的接口对齐

**问题**：配置结构不匹配，需要类型转换
```c
// PCL 返回 pcl_mcu_cfg_t
// PDL 需要 mcu_config_t
// 需要转换层，增加复杂度
```

**解决方案**：统一配置结构
```c
// PCL 定义统一的基础配置
typedef struct {
    const char *name;
    bool enabled;
    pcl_hw_interface_type_t interface_type;
    union { ... } interface;
    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
} pcl_device_config_t;

// PDL 直接使用
int32_t PDL_MCU_Init(const pcl_device_config_t *config, ...);
```

**优势**：
1. 无需类型转换
2. 避免数据拷贝
3. 配置结构在 PCL 和 PDL 之间完全对齐

### 10.5 为什么 PDL 不做统一抽象？

**错误做法**：将 Satellite/BMC/MCU 抽象成统一接口
```c
// ❌ 错误：强行统一抽象
Device_SendCommand(device_id, cmd);
Device_ReadSensor(device_id, sensor_id, &value);
```

**正确做法**：保持 PDL 服务完全独立
```c
// ✅ 正确：独立接口
PDL_Satellite_SendCommand(sat_handle, cmd);
PDL_BMC_ReadSensor(bmc_handle, sensor_id, &value);
PDL_MCU_WriteRegister(mcu_handle, reg_addr, value);
```

**原因**：
1. 业务逻辑完全不同（卫星协议 vs BMC 管理 vs MCU 控制）
2. 接口语义不同（命令格式、参数类型、返回值）
3. 强行统一会导致接口臃肿、语义模糊、难以维护

### 10.6 看门狗线程重启的正确做法

**错误做法**：使用 pthread_cancel()
```c
// ❌ 危险：可能导致资源泄漏
pthread_cancel(thread);
pthread_create(&thread, ...);
```

**正确做法**：优雅退出 + 超时保护
```c
// ✅ 正确：
// 1. 设置退出标志
ctx->should_exit = true;

// 2. 等待线程自行退出（带超时）
for (int i = 0; i < 100 && ctx->thread != 0; i++) {
    OSAL_msleep(100);
}

// 3. 如果超时仍未退出，触发系统复位
if (ctx->thread != 0) {
    LOG_FATAL("Thread failed to exit, system reset required");
    trigger_system_reset();
}

// 4. 重新创建线程
OSAL_ThreadCreate(&ctx->thread, ...);
```

**原因**：
1. pthread_cancel() 不安全，可能导致锁未释放、文件未关闭
2. 线程应该自行清理资源后退出
3. 如果线程无法退出，说明系统已经不可恢复，应该复位

### 10.7 IPC 的渐进式优化策略

**阶段 1：先实现互斥锁版本**
```c
typedef struct {
    pthread_mutex_t lock;
    command_t queue[QUEUE_SIZE];
    uint32_t head, tail;
} command_queue_t;
```

**阶段 2：性能测试**
- 测试 IPC 延迟和吞吐量
- 确定是否是性能瓶颈

**阶段 3：如需要，优化为无锁版本**
```c
typedef struct {
    alignas(64) atomic_uint head;
    alignas(64) atomic_uint tail;
    void *buffer[QUEUE_SIZE];
} spsc_queue_t;
```

**原因**：
1. 避免过早优化（Premature optimization is the root of all evil）
2. 互斥锁版本更容易调试
3. 只有性能测试证明必要时，才进行无锁优化

### 10.8 实施计划的关键改进

**原方案问题**：
- 工期估算过于乐观（10.5周）
- 缺少原型验证阶段
- OSAL 扩展工作量被低估
- PDL 重构风险过高

**优化方案**：
1. **新增原型验证阶段**（1周）：快速验证架构可行性
2. **OSAL 扩展周期调整**（2-3周）：包含多平台测试
3. **PDL 改为适配而非重构**（1周）：降低风险
4. **IPC 采用渐进式开发**（2周）：先简单后优化
5. **增加集成测试时间**（2周）：包含充足的问题修复时间

**总周期**：12.5-13.5周（更现实的估算）
```


---

## 11. 与 v2.0 方案的对比

| 维度 | v2.0 多进程方案 | v3.0 单进程多线程方案 |
|------|----------------|---------------------|
| 架构一致性 | 与 OSAL 接口不匹配（需要进程接口） | 使用 OSAL 提供的线程接口 |
| 故障隔离 | 进程级隔离（MMU 保护） | 线程级隔离（看门狗监控） |
| IPC 复杂度 | 高（消息队列 + 共享内存 + Supervisor） | 低（无锁队列 + 双缓冲） |
| 资源开销 | 大（独立地址空间） | 小（共享地址空间） |
| 调试难度 | 高（多进程日志、GDB 多进程） | 低（单进程日志、GDB 单进程） |
| 实时性 | 进程切换开销大 | 线程切换开销小 |
| 代码复用 | 需要重写 IPC 和监护逻辑 | 直接使用 OSAL 线程接口 |
| 应用解耦 | 无（APP 直接调用 PDL） | 有（ACL 层完全解耦） |
| 配置管理 | 硬编码 + Parameter Block | ACL + PCL 分层配置 |
| 新增层次 | 无 | ACL 层（新增） |
| 类型安全 | 无 | 有（枚举类型检查） |
| 可维护性 | 低（复杂度高） | 高（架构清晰） |
| 实施周期 | 8 周 | 10.5-11.5 周 |

---

## 12. 总结

### 12.1 核心改进

本方案（v3.0）相比 v2.0 多进程方案，实现了以下关键改进：

1. **架构简化**：单进程多线程 vs 多进程
   - 使用 OSAL 提供的线程接口，架构一致
   - IPC 从 POSIX 消息队列（5-10μs）优化为 SPSC 无锁队列（50-100ns）
   - 性能提升 50-100 倍

2. **业务与硬件解耦**：新增 ACL 配置层
   - 业务功能枚举（编译期类型检查）
   - O(1) 直接索引查找（性能优化）
   - 支持不同项目的硬件变体

3. **配置结构统一**：PCL 与 PDL 接口对齐
   - 统一的 `pcl_device_config_t` 基础结构
   - PDL 直接使用 PCL 配置，无需类型转换
   - 避免数据拷贝和转换开销

4. **线程级故障隔离**：看门狗监控 + 优雅重启
   - 心跳监控（原子操作，无锁）
   - 优雅退出机制（避免 pthread_cancel）
   - 超时保护 + 系统复位

5. **渐进式实施**：降低风险
   - 原型验证阶段（1周）
   - IPC 先简单后优化（互斥锁 → 无锁）
   - PDL 适配而非重构（降低风险）
   - 充足的测试时间（2周集成测试）

6. **实时性能优化**：
   - SPSC 无锁队列：50-100ns 延迟
   - 双缓冲遥测池：无锁并发访问
   - 缓存行对齐：避免伪共享
   - 线程优先级：SCHED_FIFO 实时调度

### 12.2 技术亮点

**ACL 层的 O(1) 查找优化**：
```c
// 使用枚举值作为数组索引，实现 O(1) 查找
static acl_function_config_t g_acl_lookup[ACL_FUNC_MAX];
const acl_function_config_t *cfg = &g_acl_lookup[function];
```

**PCL 与 PDL 的零拷贝配置传递**：
```c
// PCL 返回基础配置指针
const pcl_device_config_t* PCL_GetMCUConfig(uint32_t logic_index);

// PDL 直接使用，无需转换
int32_t PDL_MCU_Init(const pcl_device_config_t *config, ...);
```

**看门狗的优雅重启机制**：
```c
// 1. 设置退出标志
ctx->should_exit = true;

// 2. 等待线程自行退出（带超时）
// 3. 如果超时，触发系统复位
// 4. 重新创建线程
```

**无锁 IPC 的性能优化**：
```c
// 缓存行对齐，避免伪共享
typedef struct {
    alignas(64) atomic_uint head;  // 生产者独占缓存行
    alignas(64) atomic_uint tail;  // 消费者独占缓存行
    void *buffer[QUEUE_SIZE];
} spsc_queue_t;
```

### 12.3 实施建议

**阶段划分**：
1. **原型验证**（1周）：快速验证架构可行性
2. **基础设施**（2-3周）：OSAL 扩展
3. **配置层**（2.5周）：ACL + PCL + PDL 适配
4. **核心功能**（4周）：IPC + 应用层
5. **质量保证**（2周）：集成测试

**总工期**：12.5-13.5周

**关键里程碑**：
- 第 1 周：原型验证通过
- 第 4 周：OSAL 扩展完成
- 第 6.5 周：配置层完成
- 第 10.5 周：核心功能完成
- 第 12.5 周：集成测试完成

**风险控制**：
1. 原型验证阶段快速发现问题
2. 渐进式 IPC 开发避免过早优化
3. PDL 适配而非重构降低风险
4. 充足的测试和修复时间

### 12.4 与 v2.0 方案的对比

| 维度 | v2.0 多进程方案 | v3.0 单进程多线程方案 |
|------|----------------|---------------------|
| **架构复杂度** | 高（Supervisor + 4进程） | 中（1进程 + 4线程） |
| **IPC 延迟** | 5-10 μs（消息队列） | 50-100 ns（无锁队列） |
| **IPC 吞吐量** | ~200K ops/s | ~10M ops/s |
| **内存开销** | 高（独立地址空间） | 低（共享地址空间） |
| **故障隔离** | 进程级（强隔离） | 线程级（轻量隔离） |
| **调试难度** | 高（多进程调试） | 低（单进程调试） |
| **OSAL 兼容性** | 差（需要进程接口） | 好（使用线程接口） |
| **配置管理** | 无 ACL 层 | 有 ACL 层（业务解耦） |
| **实施周期** | 未评估 | 12.5-13.5 周 |
| **实时性能** | 中 | 高（SCHED_FIFO + 无锁） |

**结论**：v3.0 方案在性能、复杂度、可维护性上全面优于 v2.0 方案。

### 12.5 后续工作

**短期（3个月内）**：
1. 完成原型验证
2. 完成 OSAL 扩展和多平台测试
3. 完成 ACL/PCL/PDL 配置层开发
4. 完成核心功能开发和集成测试

**中期（6个月内）**：
1. 性能优化和调优
2. 完整的功能测试和压力测试
3. MISRA C 合规性检查
4. 文档完善和团队培训

**长期（1年内）**：
1. 多项目适配验证
2. 硬件变体支持
3. 故障注入测试和可靠性验证
4. 生产环境部署和监控

---

**文档版本**：v3.1（优化版）  
**最后更新**：2026年5月16日  
**主要改进**：
- ACL 层实现优化为 O(1) 查找
- PCL 与 PDL 配置结构统一
- 看门狗线程重启机制完善
- 实施计划调整为 12.5-13.5 周
- 新增原型验证阶段
- IPC 采用渐进式开发策略

**审查状态**：✅ 技术审查通过  
**建议等级**：🟢 **推荐实施**

本方案在保持现有 EMS 架构一致性的前提下，通过以下优化实现了高可靠、高实时、易维护的卫星桥接板软件架构：

1. **单进程多线程**：使用 OSAL 提供的线程接口，避免多进程复杂度
2. **ACL 配置层**：业务功能与硬件完全解耦，APP 和 PDL 可由不同团队维护
3. **PCL 硬件配置**：移除 APP 配置功能，专注于硬件配置存储
4. **PDL 独立服务**：Satellite/BMC/MCU 完全独立，不做统一抽象
5. **线程级故障隔离**：看门狗监控心跳，实现故障检测和系统保护
6. **无锁 IPC**：SPSC 队列 + 双缓冲遥测池，确定性实时通信

**关键改进点**：

1. **OSAL 层扩展（阶段 0）**：
   - 增加 `OSAL_GetTimeMs()` - 64 位毫秒时间戳
   - 增加 `OSAL_Init()/Cleanup()` - 初始化和清理接口
   - 扩展线程接口 - 支持优先级和调度策略设置
   - 增加 `OSAL_ThreadCreateEx()` - 创建线程时指定属性
   - 增加 `OSAL_ThreadSetPriority()` - 动态设置线程优先级

2. **ACL 层设计（阶段 1）**：
   - 使用枚举定义业务功能（如 `ACL_FUNC_READ_CPU_TEMP`）
   - 业务功能映射到设备类型和逻辑索引（如 `CPU_TEMP → BMC[1]`）
   - APP 只关心业务功能，不关心硬件实现（BMC 还是 MCU）
   - 不同项目可以有不同的映射配置，APP 代码无需修改

3. **PCL 层重构（阶段 2）**：
   - 移除 `pcl_app.h` 和 APP 配置功能（由 ACL 替代）
   - 重新设计配置结构，与 PDL 接口对齐
   - 专注于硬件配置存储（如 `BMC[1] → {Redfish, 192.168.1.100:443}`）

4. **PDL 层重构（阶段 3）**：
   - 重新设计接口和数据类型，与 PCL 配置结构兼容
   - PDL 初始化时直接接受 PCL 配置指针，无需类型转换
   - 通过 ACL 查询逻辑索引，再通过 PCL 获取硬件配置

5. **IPC 优化（阶段 4）**：
   - SPSC 队列增加缓存行对齐，避免伪共享
   - 双缓冲遥测池增加内存屏障，确保多核一致性
   - 心跳表使用原子操作，无锁更新

6. **线程设计（阶段 5）**：
   - 使用 `OSAL_ThreadCreateEx()` 创建线程并设置优先级
   - 使用 `OSAL_GetTimeMs()` 获取时间戳
   - 使用全局标志 `g_running` 控制线程退出
   - 看门狗线程监控心跳，检测故障但不强制重启（依赖硬件看门狗）

**实施周期**：
- **不含 OSAL 进程接口扩展**：10.5 周
- **含 OSAL 进程接口扩展**：11.5 周

**关键路径**：
```
OSAL 扩展 (1周) → ACL 开发 (1周) → PCL 重构 (1.5周) → 
PDL 重构 (2.5周) → IPC 开发 (1周) → 应用层开发 (2周) → 
集成测试 (1.5周)
```

**OSAL 进程接口扩展（可选）**：

虽然本方案采用单进程多线程架构，但为了支持未来可能的多进程需求（如隔离度要求更高的场景），建议在 OSAL 层增加进程接口封装：

```c
/* osal/include/sys/osal_process.h - 进程接口（可选扩展）*/

/**
 * @brief 创建子进程
 * @return 父进程返回子进程ID，子进程返回0，失败返回-1
 */
int32_t OSAL_ProcessCreate(osal_pid_t *pid);

/**
 * @brief 等待子进程退出
 */
int32_t OSAL_ProcessWait(osal_pid_t pid, int32_t *status);

/**
 * @brief 进程间共享内存
 */
int32_t OSAL_ShmCreate(const char *name, size_t size, void **addr);
int32_t OSAL_ShmOpen(const char *name, void **addr);
int32_t OSAL_ShmClose(void *addr);

/**
 * @brief 进程间消息队列
 */
int32_t OSAL_MqCreate(const char *name, uint32_t depth, uint32_t msg_size);
int32_t OSAL_MqSend(const char *name, const void *msg, uint32_t size);
int32_t OSAL_MqReceive(const char *name, void *msg, uint32_t size);
```

这些接口可以为未来的架构演进提供支持，但当前方案不依赖这些接口。

**ACL 层的核心价值**：

1. **业务与硬件完全分离**：
   - APP 层：只关心"获取 CPU 温度"，不关心通过 BMC 还是 MCU
   - PDL 层：只关心"读取 BMC[1] 的传感器"，不关心这是给谁用的
   - 两者通过 ACL 完全解耦，可由不同团队独立维护

2. **类型安全**：
   - 枚举提供编译期类型检查，避免字符串拼写错误
   - IDE 自动补全和重构支持

3. **性能优化**：
   - 数组索引查找（O(n)，n 很小）比字符串比较快
   - 适合实时系统

4. **配置灵活性**：
   - 同一业务功能在不同项目中可以映射到不同设备
   - 更换硬件实现时，只需修改 ACL 配置，APP 代码无需改动

**示例**：
```
项目 A：ACL_FUNC_READ_CPU_TEMP → BMC[0] → Redfish 协议
项目 B：ACL_FUNC_READ_CPU_TEMP → MCU[0] → CAN 总线协议
APP 代码完全相同，只需更换 ACL 配置文件
```

这是一个务实、可落地、易维护的架构设计，完全符合航天级嵌入式系统的高可靠性要求，且与现有 EMS 代码库兼容。通过 ACL 层实现了业务与硬件的完全解耦，为后续的团队协作和硬件变更提供了良好的支持。

---

**文档结束**
