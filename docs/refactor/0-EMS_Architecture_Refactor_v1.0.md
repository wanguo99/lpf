# PMC架构设计方案

## 摘要

​		针对PMC（Payload Management Controller）的航天级可靠性和实时需求，采用**纯A53四核架构**：充分利用Linux生态和多核能力，通过实时调度和CPU隔离实现确定性响应。该方案通过统一缓存模型和进程隔离，在满足<2ms实时应答的同时，提供完整的故障隔离和可靠性保证。

**核心特点**：

- ✅ 满足2ms实时应答（Telecommand进程<1.5ms，确定性保证）

- ✅ 航空级可靠性（进程隔离、CPU绑定、故障恢复）

- ✅ **纯A53四核架构（CPU0遥控/CPU1遥测/CPU2监控/CPU3日志）**

- ✅ **统一缓存模型（所有遥测从共享内存读取，<50μs）**

- ✅ **数据新鲜度保证（FRESH/STALE/INVALID标记）**

- ✅ 完整日志收集（运行日志、崩溃日志、状态日志）

- ✅ 严格6层架构完全解耦（APP / ACL / PDL / PCL / HAL / OSAL）

- ✅ PDL独立外设设计（每种外设独立API，无统一抽象接口）

**关键设计亮点**：

- **纯A53架构**：四核A53（Linux），CPU0实时遥控（SCHED_FIFO优先级99），CPU1后台遥测（SCHED_OTHER），CPU2/3预留
- **实时保证**：SCHED_FIFO实时调度 + CPU亲和性绑定 + mlockall内存锁定，Telecommand进程<1.5ms响应
- **统一缓存模型**：所有遥测数据存储在POSIX共享内存，Telemetry进程周期采集，Telecommand进程快速读取
- **进程隔离**：独立进程空间，故障不传播，Supervisor进程监控和自动重启
- **确定性执行**：实时调度策略，CPU隔离（isolcpus），中断亲和性配置
- **数据新鲜度标记**：FRESH（新鲜）/STALE（过期但可用）/INVALID（无效），卫星平台可判断数据可信度
- **PDL独立外设服务**：Satellite/BMC/MCU各自独立设计，ACL层配置指定具体外设类型，避免统一接口陷阱

---

## 1. 背景与需求

### 1.1 PMC产品定位

- 卫星与载荷服务器之间的桥接管理板（类似BMC）

- 管理服务器载荷与卫星平台的通信

- 服务器载荷的电源管理

### 1.2 核心业务功能

1. **遥控**：接收卫星平台命令 → 解析 → 向载荷服务器/MCU/FPGA下发

2. **遥测**：从载荷服务器/MCU/FPGA收集数据 → 打包 → 向卫星平台发送

3. **健康管理**：监测载荷服务器状态、电源控制、复位控制

4. **固件升级**：管理板固件校验恢复、主备分区升级、FPGA固件升级

5. **日志收集**：收集服务器状态、外设信息、进程日志、崩溃日志

### 1.3 技术细节

- 与卫星平台：CAN通信（可能扩展以太网、UART）

- 与载荷服务器：BMC-Redfish/IPMI（可能支持无BMC服务器的自定义协议）

- 与MCU：CAN通信（可能扩展I2C、UART、SPI）

- 电源管理：当前通过MCU，可能改为直连电源管理芯片

### 1.4 关键需求

1. **航空级稳定性**：设备必须极度稳定

2. **自恢复能力**：太空环境无人管理，出问题需自恢复

3. **实时性**：遥控遥测命令2ms内应答

4. **统一缓存模型**：
 - 所有遥测数据存储在共享内存缓存
 - Telemetry进程周期性采集并更新缓存
 - Telecommand进程从缓存快速读取（<50μs）
 - 通过时间戳 + 有效期 + 新鲜度标志体现数据质量

5. **遥测周期**：
 - 快遥：100ms周期（如电源状态）
 - 慢遥：2秒周期（如温度）
 - 可能同时或先后收到快遥和慢遥

6. **硬件平台**：TI AM6254四核A53 SoC
 - **ARM Cortex-A53**：4核应用处理器（最高1.4GHz），运行Linux
   - CPU0：实时遥控进程（SCHED_FIFO优先级99，mlockall）
   - CPU1：后台遥测进程（SCHED_OTHER，周期采集）
   - CPU2：监控进程（预留，健康管理）
   - CPU3：日志进程（预留，日志收集）
 - **核心分工（关键）**：
   - **CPU0（Telecommand进程）**：
     - CAN接收和遥控命令处理（<1ms）
     - 从共享内存缓存读取遥测（<50μs）
     - 2ms内应答
     - SCHED_FIFO优先级99，mlockall锁定内存
   - **CPU1（Telemetry进程）**：
     - 后台遥测采集（100ms周期）
     - 更新共享内存缓存
     - 更新时间戳和新鲜度标记
     - SCHED_OTHER普通调度
   - **CPU2/3（预留）**：
     - Supervisor进程（监控和自动重启）
     - Logger进程（日志收集和持久化）
 - **实时性保证**：
   - 实时调度：SCHED_FIFO优先级99
   - CPU亲和性：绑定到专用CPU核心
   - 内存锁定：mlockall防止页错误
   - CPU隔离：isolcpus内核参数（可选）
   - 中断亲和性：CAN中断绑定到CPU0（可选）

---

## 2. 架构设计原则

### 2.1 软件架构

#### 2.1.1 架构分层

```tex
┌─────────────────────────────────────────────────────────────────────────────┐
│                          OSAL运行环境                                        │
│  • 屏蔽硬件平台差异（Linux、32位/64位、系统调用封装）                          │
│  • 提供统一接口（进程/线程/IPC/内存/时间/文件/网络）                             │
│  • 平台实现（POSIX）                                                          │
│  • 多核支持（CPU亲和性、实时调度、内存锁定）                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                     所有模块运行在OSAL之上                                     │
│                                                                              │
│  ┌──────────────────────────────────┐      ┌──────────────────────────────┐  │
│  │         APP（应用层）             │      │      ACL（应用配置层）         │  │
│  │  • 2个核心进程（Telecommand/      │ ◄─── │   • 业务功能枚举               │  │
│  │    Telemetry）                    │ 读取 │   • 业务数据结构               │  │
│  │  • CPU绑定：关键进程独占CPU        │ 配置 │   • 设备映射配置（O(1)查找）    │  │
│  │  • 实时调度：SCHED_FIFO保证确定性  │      │   • 遥测缓存管理               │  │
│  └────────────┬─────────────────────┘      └──────────────────────────────┘  │
│               │ 直接调用PDL API                                               │
│               ↓                                                              │
│  ┌──────────────────────────────────┐    ┌──────────────────────────────┐    │
│  │         PDL（外设驱动层）         │    │      PCL（外设配置层）         │    │
│  │  • 独立外设服务（Satellite/      │◄───┤  • 硬件配置数据                 │    │
│  │    BMC/MCU各自独立设计）         │读取│  • 平台配置（vendor/chip/       │    │
│  │  • 协议实现（Redfish/IPMI/CAN）  │配置│    product）                   │    │
│  │  • 双通道冗余、自动切换           │    │  • 纯数据结构，无业务逻辑        │    │
│  └────────────┬─────────────────────┘    └──────────────────────────────┘    │
│               │ 直接调用HAL API                                               │
│               ↓                                                              │
│  ┌──────────────────────────────────┐                                        │
│  │         HAL（硬件抽象层）          │                                        │
│  │  • 硬件驱动接口（CAN/UART/I2C/     │                                        │
│  │    SPI/GPIO/Watchdog）            │                                        │
│  │  • 平台实现（Linux驱动）           │                                        │
│  └──────────────────────────────────┘                                        │
│                                                                              │
│  【关键原则】                                                                 │
│  • 所有模块都调用OSAL接口，不直接访问标准库或系统调用，确保高度可移植性             │
│  • APP读取ACL配置后直接调用PDL，不经过ACL                                       │
│  • PDL读取PCL配置后直接调用HAL，不经过PCL                                      │
│  • 多核优化：关键进程独占CPU，实时调度保证确定性，CPU隔离避免干扰                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.1.2 完全解耦原则

**核心规则**：
1. ✅ **OSAL运行环境**：所有模块运行在OSAL之上，只调用OSAL接口，不直接访问标准库或系统调用，确保高度可移植性
2. ✅ **纵向依赖**：APP → PDL → HAL（每层直接调用下层API）
3. ✅ **横向配置**：APP读取ACL配置，PDL读取PCL配置（配置层不参与调用链）
4. ✅ **接口隔离**：每层只暴露必要的公开接口，内部实现完全隐藏
5. ✅ **无跨层依赖**：严禁跨层直接访问（如APP直接调用HAL）
6. ✅ **无common**：所有代码都有明确归属，不存在"公共组件"

**数据结构归属原则**：
- **通用IPC机制** → OSAL层（日志环形缓冲区、心跳表）
- **业务数据结构** → ACL层（遥测缓存、状态快照）
- **应用级设计** → APP层（共享内存布局）

**示例**：
```c
// ✅ 正确：APP层读取ACL配置，然后直接调用PDL
#include "acl_telemetry_config.h"
#include "pdl_bmc.h"
acl_tm_config_t *cfg = ACL_GetTelemetryConfig(TM_CPU_TEMP);
if (cfg->device_type == PDL_DEVICE_TYPE_BMC) {
    PDL_BMC_ReadSensors(cfg->device_index, &data);
}

// ❌ 错误：APP层直接访问HAL层（跨层依赖）
#include "hal_can.h"  // 禁止！
HAL_CAN_Send(...);

// ✅ 正确：所有模块都调用OSAL接口
#include "osal_thread.h"
#include "osal_mutex.h"
osal_thread_create(&thread, thread_func, NULL);
osal_mutex_lock(&mutex);

// ❌ 错误：直接调用系统调用或标准库
pthread_create(...);  // 禁止！应该调用osal_thread_create()
malloc(...);          // 禁止！应该调用osal_malloc()
```

#### 2.1.3 职责清晰原则

| 层 | 职责 | 禁止事项 |
|---|------|---------|
| **APP** | 业务流程、进程管理、共享内存布局 | 不能直接调用HAL、不能直接访问系统调用 |
| **ACL** | 业务配置、设备映射、业务数据结构 | 不能包含业务逻辑、不参与调用链 |
| **PDL** | 独立外设服务、协议实现、通道管理 | 不能包含硬件寄存器操作、不设计统一外设接口 |
| **PCL** | 硬件配置数据 | 不能包含业务逻辑、函数实现、不参与调用链 |
| **HAL** | 硬件驱动接口，调用OS驱动或自写驱动 | 不能包含业务逻辑 |
| **OSAL** | 系统调用封装、通用IPC机制 | 不能包含业务逻辑、硬件操作 |

**架构关系说明**：
- ✅ **纵向依赖**：APP → PDL → HAL->OS_Drivers/Reg_Operations（每层直接调用下层API）
- ✅ **横向配置**：APP读取ACL配置，PDL读取PCL配置（配置层是数据源，不参与调用链）
- ✅ **OSAL基础**：所有模块都运行在OSAL之上，只调用OSAL接口，不直接访问系统调用

**PDL层设计原则**：
- ✅ **独立外设服务**：每种外设（Satellite/BMC/MCU）完全独立设计，各自暴露专属API
- ✅ **无统一抽象**：不强行抽象成统一的"设备"接口，避免最小公分母或最大公约数陷阱
- ✅ **ACL层配置**：通过ACL层配置表指定具体外设类型和索引（device_type + device_index）
- ✅ **接口贴合特性**：每种外设的API完全贴合其功能特性，无冗余参数和类型判断

**示例**：
```c
// ✅ PDL层：每种外设独立设计
// pdl_bmc.h
int PDL_BMC_ReadSensors(uint32_t bmc_index, bmc_sensor_data_t *data);
int PDL_BMC_PowerControl(uint32_t bmc_index, bmc_power_cmd_t cmd);

// pdl_mcu.h
int PDL_MCU_SendCommand(uint32_t mcu_index, mcu_cmd_t cmd, uint32_t param);
int PDL_MCU_GetWatchdogStatus(uint32_t mcu_index, mcu_wdt_status_t *status);

// pdl_satellite.h
int PDL_Satellite_SendTelecommand(uint32_t sat_index, can_frame_t *frame);

// ✅ ACL层：配置表指定具体外设
typedef struct {
    tm_id_t tm_id;
    pdl_device_type_t device_type;  // BMC/MCU/SATELLITE
    uint32_t device_index;
    // ...
} acl_tm_config_t;

// 配置示例
{
    .tm_id = TM_CPU_TEMP,
    .device_type = PDL_DEVICE_TYPE_BMC,
    .device_index = 0
}
```

---

### 2.2 纯A53四核架构设计

#### 2.2.1 核心分工

**CPU0（Telecommand进程）- 实时遥控**：
```
CPU0职责（总延迟<1.5ms）：
├─ CAN接收和遥控命令处理（<1ms）
├─ 从共享内存缓存读取遥测（<50μs）
├─ 2ms内应答
├─ SCHED_FIFO优先级99
├─ mlockall锁定内存
└─ 绑定到CPU0（独占）
```

**CPU1（Telemetry进程）- 后台遥测**：
```
CPU1职责（周期性采集）：
├─ 后台遥测采集（100ms周期）
├─ 更新共享内存缓存
├─ 更新时间戳和新鲜度标记
├─ SCHED_OTHER普通调度
└─ 绑定到CPU1
```

**CPU2/3（预留）**：
```
预留用途：
├─ Supervisor进程（监控和自动重启）
├─ Logger进程（日志收集和持久化）
└─ 其他后台服务
```

**设计原则**：
- ✅ **实时路径（CPU0）**：CAN接收 → 缓存读取 → 应答发送，全程<1.5ms
- ✅ **确定性保证**：SCHED_FIFO实时调度 + CPU亲和性 + mlockall内存锁定
- ✅ **进程隔离**：独立进程空间，故障不传播
- ✅ **统一缓存模型**：所有遥测从共享内存读取，无实时查询延迟
- ✅ **CPU隔离**：关键进程独占CPU核心，避免调度干扰

#### 2.2.2 软件模块部署图

**6层架构在纯A53四核环境下的部署**：

```text
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         TI AM6254 四核A53 SoC                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌─────────────────────────────────────┐  ┌─────────────────────────────────┐  │
│  │   CPU0 (Linux, SCHED_FIFO)          │  │   CPU1 (Linux, SCHED_OTHER)     │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ APP层 (Telecommand进程)             │  │ APP层 (Telemetry进程)           │  │
│  │ ├─ 实时遥控处理                     │  │ ├─ 后台遥测采集                 │  │
│  │ ├─ CAN命令接收                      │  │ ├─ 周期性设备查询               │  │
│  │ ├─ 缓存遥测读取                     │  │ ├─ 共享内存缓存更新             │  │
│  │ ├─ 2ms内应答                        │  │ └─ 新鲜度标记更新               │  │
│  │ ├─ SCHED_FIFO优先级99               │  │                                 │  │
│  │ └─ mlockall内存锁定                 │  │                                 │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ ACL层 (运行时加载)                  │  │ ACL层 (运行时加载)              │  │
│  │ ├─ 配置表（H200_100P/demo）         │  │ ├─ 配置表                       │  │
│  │ ├─ 遥控/遥测枚举                    │  │ ├─ 遥测缓存管理                 │  │
│  │ └─ 失效映射                         │  │ └─ 新鲜度计算                   │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ PDL层 (完整模块)                    │  │ PDL层 (完整模块)                │  │
│  │ ├─ PDL_Satellite (CAN协议)          │  │ ├─ PDL_BMC (Redfish/IPMI)       │  │
│  │ └─ PDL_MCU (CAN协议)                │  │ ├─ PDL_MCU (完整功能)           │  │
│  │                                     │  │ └─ PDL_Satellite (管理接口)     │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ PCL层 (运行时加载)                  │  │ PCL层 (运行时加载)              │  │
│  │ └─ 硬件配置 (DDR)                   │  │ └─ 硬件配置 (DDR)               │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ HAL层 (Linux驱动)                   │  │ HAL层 (Linux驱动)               │  │
│  │ ├─ CAN (SocketCAN)                  │  │ ├─ CAN (SocketCAN)              │  │
│  │ ├─ GPIO (sysfs)                     │  │ ├─ Ethernet (Linux驱动)         │  │
│  │ └─ Watchdog (Linux驱动)             │  │ ├─ UART (Linux驱动)             │  │
│  │                                     │  │ └─ I2C/SPI (Linux驱动)          │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ OSAL层 (POSIX实现)                  │  │ OSAL层 (POSIX实现)              │  │
│  │ ├─ 线程管理 (pthread)               │  │ ├─ 线程管理 (pthread)           │  │
│  │ ├─ 实时调度 (sched_setscheduler)    │  │ ├─ 共享内存 (shm_open/mmap)     │  │
│  │ ├─ CPU亲和性 (sched_setaffinity)    │  │ ├─ 互斥锁 (pthread_mutex)       │  │
│  │ └─ 内存锁定 (mlockall)              │  │ └─ 信号处理 (sigaction)         │  │
│  └─────────────────────────────────────┘  └─────────────────────────────────┘  │
│                                  │                      │                       │
│                                  └──────────┬───────────┘                       │
│                                             ▼                                   │
│                          ┌──────────────────────────────────┐                  │
│                          │      进程间通信 (IPC)             │                  │
│                          │  • POSIX共享内存 (DDR, 4MB)       │                  │
│                          │  • pthread_rwlock (并发保护)      │                  │
│                          │  • 遥测缓存 (统一缓存模型)         │                  │
│                          └──────────────────────────────────┘                  │
│                                                                                 │
│  ┌─────────────────────────────────────┐  ┌─────────────────────────────────┐  │
│  │   CPU2 (预留)                       │  │   CPU3 (预留)                   │  │
│  ├─────────────────────────────────────┤  ├─────────────────────────────────┤  │
│  │ • Supervisor进程                    │  │ • Logger进程                    │  │
│  │ • 健康监控                          │  │ • 日志收集                      │  │
│  │ • 自动重启                          │  │ • 崩溃分析                      │  │
│  └─────────────────────────────────────┘  └─────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**关键设计决策**：

| 层 | CPU0（实时） | CPU1（后台） | 说明 |
|---|-------|-------|------|
| **APP** | Telecommand进程 | Telemetry进程 | CPU0实时遥控，CPU1后台采集 |
| **ACL** | 配置表 + 失效映射 | 配置表 + 缓存管理 | 统一配置，缓存管理在ACL层 |
| **PDL** | Satellite/MCU（CAN） | BMC/MCU（完整） | CPU0处理CAN，CPU1处理网络 |
| **PCL** | 硬件配置 | 硬件配置 | 两侧配置保持一致 |
| **HAL** | Linux驱动 | Linux驱动 | 统一使用Linux驱动 |
| **OSAL** | POSIX实现 | POSIX实现 | 统一POSIX接口 |

#### 2.2.3 进程间通信机制

**通信方式选择**：

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    TI AM6254 四核A53 SoC                                  │
│                                                                          │
│  ┌──────────────────────────────────┐  ┌──────────────────────────────┐ │
│  │   CPU0 (实时遥控)                 │  │   CPU1 (后台遥测)             │ │
│  │   Telecommand进程                │  │   Telemetry进程              │ │
│  │   SCHED_FIFO优先级99             │  │   SCHED_OTHER                │ │
│  │                                  │  │                              │ │
│  │  ┌────────────────────────────┐  │  │  ┌────────────────────────┐  │ │
│  │  │ 实时遥控处理               │  │  │  │ 后台遥测采集           │  │ │
│  │  │ • CAN命令接收              │  │  │  │ • 周期性设备查询       │  │ │
│  │  │ • 缓存遥测读取(<50μs)      │  │  │  │ • 缓存更新             │  │ │
│  │  │ • 2ms内应答                │  │  │  │ • 新鲜度标记           │  │ │
│  │  └────────────────────────────┘  │  │  └────────────────────────┘  │ │
│  │                                  │  │                              │ │
│  └──────────────┬───────────────────┘  └──────────────┬───────────────┘ │
│                 │                                      │                │
│                 │         读取（<50μs）                │ 写入（<10μs）  │
│                 └──────────────┬───────────────────────┘                │
│                                │                                        │
│         ┌──────────────────────▼──────────────────────────┐            │
│         │         POSIX共享内存（遥测缓存）                │            │
│         │  • shm_open + mmap (DDR, 4MB)                   │            │
│         │  • pthread_rwlock (并发保护)                    │            │
│         │  • 时间戳 + 有效期 + 新鲜度标志                 │            │
│         └──────────────────────────────────────────────────┘            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

**通信机制详解**：

| 通信方向 | 机制 | 延迟 | 用途 | 数据类型 |
|---------|------|------|------|---------|
| **Telemetry → Telecommand** | POSIX共享内存写入 | <10μs | 遥测数据缓存更新 | 大块数据（高频） |
| **Telecommand → Telemetry** | POSIX共享内存读取 | <50μs | 遥测数据快速读取 | 大块数据（高频） |
| **并发保护** | pthread_rwlock | <1μs | 读写锁保护 | 同步原语 |

**共享内存详细布局**：

```c
/************************************************
 * 共享内存布局（DDR，4MB，物理地址连续）
 * 基地址：通过shm_open动态分配
 * 访问方式：mmap映射到进程地址空间
 ************************************************/

#define ACL_TM_CACHE_SHM_NAME    "/acl_tm_cache"
#define ACL_TM_CACHE_SIZE        (4 * 1024 * 1024)  // 4MB

typedef struct {
    /************************************************
     * 遥测缓存条目（每个遥测项一个条目）
     ************************************************/
    struct {
        uint8_t data[ACL_TM_MAX_DATA_SIZE];  // 遥测数据（最大256字节）
        uint32_t size;                        // 数据大小
        uint64_t timestamp_us;                // 采集时间戳（微秒）
        uint32_t validity_ms;                 // 有效期（毫秒）
        tm_freshness_t freshness;             // 新鲜度标记
        pthread_rwlock_t lock;                // 读写锁
        uint8_t padding[32];                  // 缓存行对齐
    } entries[TM_FUNC_MAX] __attribute__((aligned(64)));
    
    /************************************************
     * 全局元数据
     ************************************************/
    uint32_t magic;                           // 魔数校验（0x544D4341）
    uint32_t version;                         // 版本号
    uint64_t init_timestamp_us;               // 初始化时间戳
    
} acl_tm_cache_t;
     * 区域4：日志环形缓冲区（1MB）
     * R5F写，Logger进程(A53)读
     ************************************************/
    struct {
        _Atomic uint32_t write_pos;             // 写入位置（环形）
        _Atomic uint32_t read_pos;              // 读取位置（环形）
        uint32_t entry_size;                    // 每条日志大小（256字节）
        uint32_t max_entries;                   // 最大条目数（4096）
        
        // 日志条目数组
        struct {
            uint64_t timestamp_ns;              // 时间戳
            uint8_t  level;                     // 日志级别（DEBUG/INFO/WARN/ERROR）
            uint8_t  module;                    // 模块ID
            uint16_t line;                      // 行号
            char     message[248];              // 日志消息
        } entries[4096];
    } log_ring_buffer __attribute__((aligned(64)));
    
    /************************************************
     * 区域5：RPMsg通信缓冲区（256KB）
     * A53写，R5F读（控制命令）
     ************************************************/
    struct {
        _Atomic uint32_t write_pos;             // 写入位置
        _Atomic uint32_t read_pos;              // 读取位置
        uint32_t max_messages;                  // 最大消息数（256）
        
        // 消息队列
        struct {
            uint32_t msg_type;                  // 消息类型（固件升级/配置更新/日志请求）
            uint32_t msg_len;                   // 消息长度
            uint8_t  payload[1016];             // 消息负载（1KB对齐）
        } messages[256];
    } rpmsg_buffer __attribute__((aligned(64)));
    
    /************************************************
     * 区域6：遥控命令失效映射表（128KB）
     * 编译时生成，R5F和A53共享只读
     ************************************************/
    struct {
        uint32_t version;                       // 映射表版本
        uint32_t entry_count;                   // 条目数量
        
        // 失效映射条目（遥控命令 → 失效的遥测ID列表）
        struct {
            uint16_t tc_id;                     // 遥控命令ID
            uint16_t invalidated_tm_count;      // 失效的遥测数量
            uint16_t invalidated_tm_ids[32];    // 失效的遥测ID列表
        } entries[1024];
    } tc_invalidation_map __attribute__((aligned(64)));
    
} ipc_shared_memory_t;

// 编译时检查共享内存大小
_Static_assert(sizeof(ipc_shared_memory_t) <= IPC_SHM_SIZE, 
               "Shared memory layout exceeds 4MB");
```

**共享内存访问协议**：

```c
/************************************************
 * 遥测缓存访问协议（双缓冲无锁读写）
 ************************************************/

// A53侧写入（Telemetry进程）
void telemetry_cache_write(ipc_shared_memory_t *shm, telemetry_entry_t *entries, uint32_t count) {
    // 1. 获取非活动缓冲区索引
    uint32_t active = atomic_load(&shm->telemetry_cache.active_buffer);
    uint32_t inactive = 1 - active;
    
    // 2. 写入非活动缓冲区（无锁，不影响R5F读取）
    shm->telemetry_cache.buffers[inactive].entry_count = count;
    shm->telemetry_cache.buffers[inactive].timestamp_ns = get_time_ns();
    memcpy(shm->telemetry_cache.buffers[inactive].entries, entries, 
           count * sizeof(telemetry_entry_t));
    
    // 3. 计算CRC32
    shm->telemetry_cache.buffers[inactive].crc32 = 
        crc32(&shm->telemetry_cache.buffers[inactive], 
              sizeof(shm->telemetry_cache.buffers[inactive]) - 4);
    
    // 4. 原子切换活动缓冲区
    atomic_store(&shm->telemetry_cache.active_buffer, inactive);
    atomic_fetch_add(&shm->telemetry_cache.write_sequence, 1);
    
    // 5. 通过Mailbox通知R5F（可选，如果R5F在等待）
    mailbox_send_interrupt(MAILBOX_R5F, MAILBOX_MSG_TM_UPDATED);
}

// R5F侧读取（CAN_RX_Task）
int32_t telemetry_cache_read(ipc_shared_memory_t *shm, uint16_t tm_id, uint32_t *value) {
    // 1. 获取当前活动缓冲区
    uint32_t active = atomic_load(&shm->telemetry_cache.active_buffer);
    uint32_t seq_before = atomic_load(&shm->telemetry_cache.write_sequence);
    
    // 2. 读取数据（无锁，A53可能在写另一个缓冲区）
    uint32_t count = shm->telemetry_cache.buffers[active].entry_count;
    for (uint32_t i = 0; i < count; i++) {
        if (shm->telemetry_cache.buffers[active].entries[i].tm_id == tm_id) {
            *value = shm->telemetry_cache.buffers[active].entries[i].value;
            
            // 3. 检查是否在读取期间发生了切换
            uint32_t seq_after = atomic_load(&shm->telemetry_cache.write_sequence);
            if (seq_before != seq_after) {
                // 发生了切换，重新读取
                return telemetry_cache_read(shm, tm_id, value);
            }
            
            return OS_SUCCESS;
        }
    }
    
    return OS_ERROR;  // 未找到
}

/************************************************
 * 日志环形缓冲区访问协议（无锁SPSC）
 ************************************************/

// R5F侧写入（所有任务）
void log_write(ipc_shared_memory_t *shm, uint8_t level, uint8_t module, 
               uint16_t line, const char *message) {
    // 1. 原子获取写入位置
    uint32_t write_pos = atomic_fetch_add(&shm->log_ring_buffer.write_pos, 1);
    uint32_t index = write_pos % shm->log_ring_buffer.max_entries;
    
    // 2. 写入日志条目（无锁，单生产者）
    shm->log_ring_buffer.entries[index].timestamp_ns = get_time_ns();
    shm->log_ring_buffer.entries[index].level = level;
    shm->log_ring_buffer.entries[index].module = module;
    shm->log_ring_buffer.entries[index].line = line;
    strncpy(shm->log_ring_buffer.entries[index].message, message, 247);
    shm->log_ring_buffer.entries[index].message[247] = '\0';
    
    // 3. 内存屏障（确保写入完成后再更新write_pos）
    atomic_thread_fence(memory_order_release);
}

// A53侧读取（Logger进程）
int32_t log_read(ipc_shared_memory_t *shm, log_entry_t *entry) {
    // 1. 检查是否有新日志
    uint32_t read_pos = atomic_load(&shm->log_ring_buffer.read_pos);
    uint32_t write_pos = atomic_load(&shm->log_ring_buffer.write_pos);
    
    if (read_pos == write_pos) {
        return OS_ERROR;  // 无新日志
    }
    
    // 2. 读取日志条目
    uint32_t index = read_pos % shm->log_ring_buffer.max_entries;
    memcpy(entry, &shm->log_ring_buffer.entries[index], sizeof(log_entry_t));
    
    // 3. 更新读取位置
    atomic_store(&shm->log_ring_buffer.read_pos, read_pos + 1);
    
    return OS_SUCCESS;
}
```

#### 2.2.4 遥控命令处理流程

**命令分类与处理策略**：

| 命令类型 | 处理核心 | 处理路径 | 延迟目标 | 示例 |
|---------|---------|---------|---------|------|
| **简单遥控** | R5F独立处理 | R5F本地执行 | <1.3ms | GPIO控制、CAN发送到MCU |
| **复杂遥控** | R5F转发A53 | R5F→A53→执行→R5F | <100ms | BMC电源控制、固件升级 |
| **实时遥测** | R5F查询 | R5F读共享内存 | <800μs | 从缓存读取CPU温度 |
| **慢速遥测** | R5F转发A53 | R5F→A53→查询→R5F | <100ms | BMC实时传感器读取 |

**完整数据流图**：

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                        遥控命令处理流程                                       │
└─────────────────────────────────────────────────────────────────────────────┘

卫星平台 ──CAN帧──> R5F (CAN中断，<10μs)
                     │
                     ├─ CAN_RX_Task (优先级255)
                     │   ├─ 接收CAN帧 (<50μs)
                     │   ├─ CRC校验 (<20μs)
                     │   ├─ 协议解析 (<100μs)
                     │   └─ 命令分类 (<30μs)
                     │
                     ├──────────┬──────────┬──────────┐
                     │          │          │          │
                     ▼          ▼          ▼          ▼
              ┌──────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐
              │简单遥控  │ │实时遥测 │ │复杂遥控 │ │慢速遥测 │
              └──────────┘ └─────────┘ └─────────┘ └─────────┘
                   │            │           │           │
                   │            │           │           │
        ┌──────────▼────────┐   │           │           │
        │ Telecontrol_Task  │   │           │           │
        │ (优先级180)       │   │           │           │
        │ • 查ACL配置       │   │           │           │
        │ • 调PDL接口       │   │           │           │
        │ • GPIO/CAN操作    │   │           │           │
        │ • 延迟<500μs      │   │           │           │
        └──────────┬────────┘   │           │           │
                   │            │           │           │
                   │    ┌───────▼────────┐  │           │
                   │    │ Realtime_TM_   │  │           │
                   │    │ Task (优先级200)│  │           │
                   │    │ • 读共享内存    │  │           │
                   │    │ • 查遥测缓存    │  │           │
                   │    │ • 延迟<800μs    │  │           │
                   │    └───────┬────────┘  │           │
                   │            │           │           │
                   │            │    ┌──────▼──────┐    │
                   │            │    │ IPC_Handler │    │
                   │            │    │ _Task       │    │
                   │            │    │ (优先级100) │    │
                   │            │    │ • RPMsg发送 │    │
                   │            │    │ • 等待应答  │    │
                   │            │    │ • 延迟<100ms│    │
                   │            │    └──────┬──────┘    │
                   │            │           │           │
                   │            │           ▼           ▼
                   │            │      ┌─────────────────────┐
                   │            │      │   A53 Linux         │
                   │            │      │ • Telemetry进程     │
                   │            │      │ • 调用PDL_BMC       │
                   │            │      │ • 网络请求          │
                   │            │      │ • 返回结果          │
                   │            │      └─────────┬───────────┘
                   │            │                │
                   │            │                ▼
                   │            │      ┌─────────────────────┐
                   │            │      │ 共享内存/RPMsg      │
                   │            │      └─────────┬───────────┘
                   │            │                │
                   ├────────────┴────────────────┘
                   │
                   ▼
            ┌──────────────┐
            │ 封装CAN应答  │
            │ • 结果打包   │
            │ • CRC计算    │
            │ • 延迟<100μs │
            └──────┬───────┘
                   │
                   ▼
            ┌──────────────┐
            │ CAN发送      │
            │ • 硬件FIFO   │
            │ • 延迟<50μs  │
            └──────┬───────┘
                   │
                   ▼
              卫星平台 (收到应答)

┌─────────────────────────────────────────────────────────────────────────────┐
│                        时间预算分析                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│ 简单遥控（R5F独立处理）：                                                    │
│   CAN中断(10μs) + 接收(50μs) + 解析(100μs) + 分类(30μs) +                  │
│   执行(500μs) + 封装(100μs) + 发送(50μs) = 840μs < 1.3ms ✓                 │
│                                                                             │
│ 实时遥测（R5F读共享内存）：                                                  │
│   CAN中断(10μs) + 接收(50μs) + 解析(100μs) + 分类(30μs) +                  │
│   查询(800μs) + 封装(100μs) + 发送(50μs) = 1140μs < 1.3ms ✓                │
│                                                                             │
│ 复杂遥控（R5F转发A53）：                                                     │
│   R5F处理(200μs) + RPMsg(100μs) + A53处理(50ms) + 返回(100μs) +            │
│   R5F应答(200μs) = ~51ms < 100ms ✓                                         │
└─────────────────────────────────────────────────────────────────────────────┘
```

**命令分类逻辑**：

```c
/************************************************
 * R5F侧命令分类（CAN_RX_Task）
 ************************************************/

typedef enum {
    CMD_CLASS_SIMPLE_TC,      // 简单遥控（R5F独立处理）
    CMD_CLASS_REALTIME_TM,    // 实时遥测（R5F读共享内存）
    CMD_CLASS_COMPLEX_TC,     // 复杂遥控（转发A53）
    CMD_CLASS_SLOW_TM         // 慢速遥测（转发A53）
} command_class_t;

command_class_t classify_command(uint16_t cmd_id) {
    // 查询ACL配置表（编译时生成，存储在TCM）
    acl_tc_config_t *cfg = acl_get_tc_config(cmd_id);
    
    if (cfg->device_type == ACL_DEVICE_MCU && cfg->interface == ACL_IF_CAN) {
        // MCU通过CAN控制，R5F可以直接处理
        return CMD_CLASS_SIMPLE_TC;
    }
    
    if (cfg->device_type == ACL_DEVICE_SATELLITE && cfg->cmd_type == ACL_CMD_TM) {
        // 遥测命令
        if (cfg->tm_type == TM_TYPE_CACHED) {
            // 缓存型遥测，R5F从共享内存读取
            return CMD_CLASS_REALTIME_TM;
        } else {
            // 实时型遥测，需要A53实时查询
            return CMD_CLASS_SLOW_TM;
        }
    }
    
    if (cfg->device_type == ACL_DEVICE_BMC) {
        // BMC需要网络通信，必须转发A53
        return CMD_CLASS_COMPLEX_TC;
    }
    
    // 默认转发A53
    return CMD_CLASS_COMPLEX_TC;
}
```

#### 2.2.5 遥测数据处理流程

**遥测分类与采集策略**：

| 遥测类型 | 采集核心 | 采集周期 | 存储位置 | 应答延迟 | 示例 |
|---------|---------|---------|---------|---------|------|
| **缓存型遥测** | A53后台采集 | 100ms-1s | 共享内存 | <800μs | CPU温度、内存使用率 |
| **实时型遥测** | A53实时查询 | 按需 | 临时缓冲 | <100ms | 服务器开关机状态 |
| **快遥** | A53后台采集 | 1s | 共享内存 | <800μs | 关键传感器 |
| **慢遥** | A53后台采集 | 2s | 共享内存 | <800μs | 非关键传感器 |

**完整数据流图**：

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                        遥测数据采集与应答流程                                 │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                    A53侧：后台遥测采集（Telemetry进程）                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ Cache_Collector_Thread (1秒周期)                                     │  │
│  │ ├─ 遍历ACL配置表（缓存型遥测列表）                                    │  │
│  │ ├─ 调用PDL接口采集数据                                                │  │
│  │ │   ├─ PDL_BMC_ReadSensors() → Redfish/IPMI                          │  │
│  │ │   ├─ PDL_MCU_GetStatus() → CAN                                     │  │
│  │ │   └─ PDL_Satellite_GetHealth() → 本地状态                          │  │
│  │ ├─ 更新遥测缓存（双缓冲写入）                                         │  │
│  │ │   ├─ 写入非活动缓冲区                                               │  │
│  │ │   ├─ 计算CRC32                                                     │  │
│  │ │   └─ 原子切换活动缓冲区                                             │  │
│  │ └─ 写入共享内存（R5F可读）                                            │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ Health_Monitor_Thread (5秒周期)                                      │  │
│  │ ├─ 检查外设连接状态                                                   │  │
│  │ ├─ 检查通信错误计数                                                   │  │
│  │ ├─ 检查R5F心跳                                                        │  │
│  │ └─ 更新健康状态到共享内存                                             │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ Status_Snapshot_Thread (10秒周期)                                    │  │
│  │ ├─ 收集完整系统状态                                                   │  │
│  │ ├─ 序列化为JSON格式                                                   │  │
│  │ └─ 写入共享内存（用于日志归档）                                       │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │   共享内存（DDR）      │
                        │ • 遥测缓存（双缓冲）   │
                        │ • 状态快照            │
                        │ • 心跳表              │
                        └───────────┬───────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    R5F侧：遥测应答（Realtime_TM_Task）                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  卫星平台 ──遥测请求(CAN)──> R5F (CAN中断)                                  │
│                               │                                             │
│                               ▼                                             │
│                    ┌──────────────────────┐                                │
│                    │ CAN_RX_Task          │                                │
│                    │ • 解析遥测请求       │                                │
│                    │ • 提取遥测ID         │                                │
│                    │ • 唤醒Realtime_TM    │                                │
│                    └──────────┬───────────┘                                │
│                               │                                             │
│                               ▼                                             │
│                    ┌──────────────────────┐                                │
│                    │ Realtime_TM_Task     │                                │
│                    │ (优先级200)          │                                │
│                    └──────────┬───────────┘                                │
│                               │                                             │
│                    ┌──────────▼───────────┐                                │
│                    │ 查询ACL配置          │                                │
│                    │ • 遥测类型？         │                                │
│                    └──────────┬───────────┘                                │
│                               │                                             │
│                ┌──────────────┴──────────────┐                             │
│                ▼                             ▼                             │
│     ┌──────────────────┐          ┌──────────────────┐                    │
│     │ 缓存型遥测       │          │ 实时型遥测       │                    │
│     │ • 读共享内存     │          │ • RPMsg请求A53   │                    │
│     │ • 查遥测缓存     │          │ • 等待应答       │                    │
│     │ • 检查新鲜度     │          │ • 超时降级到缓存 │                    │
│     │ • 延迟<10μs      │          │ • 延迟<100ms     │                    │
│     └──────────┬───────┘          └──────────┬───────┘                    │
│                │                             │                             │
│                └──────────────┬──────────────┘                             │
│                               ▼                                             │
│                    ┌──────────────────────┐                                │
│                    │ 封装CAN应答          │                                │
│                    │ • 遥测值             │                                │
│                    │ • 新鲜度标记         │                                │
│                    │ • 时间戳             │                                │
│                    │ • CRC校验            │                                │
│                    └──────────┬───────────┘                                │
│                               │                                             │
│                               ▼                                             │
│                    ┌──────────────────────┐                                │
│                    │ CAN发送              │                                │
│                    └──────────┬───────────┘                                │
│                               │                                             │
│                               ▼                                             │
│                          卫星平台 (收到遥测数据)                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**数据新鲜度管理**：

```c
/************************************************
 * 遥测数据新鲜度标记
 ************************************************/

typedef enum {
    TM_FRESH = 0,      // 新鲜数据：采集时间在有效期内
    TM_STALE = 1,      // 过期数据：受遥控命令影响，等待更新
    TM_INVALID = 2     // 无效数据：从未采集过或采集失败
} tm_freshness_t;

/************************************************
 * 遥控命令失效遥测数据（R5F侧）
 ************************************************/

void invalidate_telemetry_by_tc(ipc_shared_memory_t *shm, uint16_t tc_id) {
    // 1. 查询失效映射表（编译时生成，存储在共享内存）
    tc_invalidation_entry_t *entry = find_invalidation_entry(
        &shm->tc_invalidation_map, tc_id);
    
    if (entry == NULL) {
        return;  // 该遥控命令不影响任何遥测
    }
    
    // 2. 标记相关遥测为STALE
    for (uint32_t i = 0; i < entry->invalidated_tm_count; i++) {
        uint16_t tm_id = entry->invalidated_tm_ids[i];
        
        // 在遥测缓存中查找并标记
        uint32_t active = atomic_load(&shm->telemetry_cache.active_buffer);
        for (uint32_t j = 0; j < shm->telemetry_cache.buffers[active].entry_count; j++) {
            if (shm->telemetry_cache.buffers[active].entries[j].tm_id == tm_id) {
                shm->telemetry_cache.buffers[active].entries[j].freshness = TM_STALE;
            }
        }
    }
    
    // 3. 通知A53重新采集（通过Mailbox）
    mailbox_send_interrupt(MAILBOX_A53, MAILBOX_MSG_TM_INVALIDATED);
}

/************************************************
 * A53侧重新采集（Telemetry进程）
 ************************************************/

void handle_tm_invalidation(ipc_shared_memory_t *shm) {
    // 1. 扫描遥测缓存，找到所有STALE条目
    uint32_t active = atomic_load(&shm->telemetry_cache.active_buffer);
    for (uint32_t i = 0; i < shm->telemetry_cache.buffers[active].entry_count; i++) {
        if (shm->telemetry_cache.buffers[active].entries[i].freshness == TM_STALE) {
            uint16_t tm_id = shm->telemetry_cache.buffers[active].entries[i].tm_id;
            
            // 2. 立即重新采集
            uint32_t new_value;
            int32_t ret = collect_telemetry(tm_id, &new_value);
            
            if (ret == OS_SUCCESS) {
                // 3. 更新缓存并标记为FRESH
                shm->telemetry_cache.buffers[active].entries[i].value = new_value;
                shm->telemetry_cache.buffers[active].entries[i].freshness = TM_FRESH;
                shm->telemetry_cache.buffers[active].entries[i].timestamp_ns = get_time_ns();
            }
        }
    }
}
```

**R5F硬实时路径（总计<1.3ms）**：

```text
卫星CAN命令 → CAN中断触发（0μs）
    ↓
中断处理程序（10μs）：保存上下文 + 唤醒CAN_RX_Task
    ↓
CAN_RX_Task调度（5μs）：FreeRTOS调度
    ↓
命令解析（200μs）：协议解析 + 校验
    ↓
命令分类（50μs）：
    ├─ 实时型遥测 → Realtime_TM_Task（800μs）
    │   ├─ 从物理设备读取最新数据
    │   └─ 封装应答
    └─ 遥控命令 → Telecontrol_Task（500μs）
        ├─ 调用PDL接口控制外设
        └─ 封装应答
    ↓
应答封装（100μs）
    ↓
CAN发送（100μs）
    ↓
总计：1265μs（<1.3ms）

抖动控制：±20μs（FreeRTOS调度抖动）
最坏情况：1.3ms + 0.02ms = 1.32ms < 2ms ✓
```

**关键优化**：
1. **TCM零等待**：R5F代码和关键数据存储在TCM（256KB），零等待访问
2. **中断优先级**：CAN中断最高优先级（255），不可被抢占
3. **FreeRTOS调度**：可预测调度，抖动<20μs
4. **预分配**：所有缓冲区启动时预分配，运行时零malloc
5. **零拷贝**：CAN数据直接在TCM中处理，无需额外拷贝

#### 2.2.4 故障隔离（安全岛设计）

**R5F作为安全岛**：

```text
R5F安全岛特性：
├─ 独立电源域（可选）
├─ 独立时钟源
├─ 独立看门狗
├─ 独立CAN控制器
├─ A53崩溃时，R5F仍可维持遥控遥测
└─ R5F崩溃时，A53可通过RemoteProc复位R5F
```

**故障恢复机制**：

| 故障场景 | 检测机制 | 恢复措施 | 恢复时间 |
|---------|---------|---------|---------|
| **R5F崩溃** | A53监控R5F心跳（共享内存） | A53通过RemoteProc复位R5F | <1秒 |
| **A53崩溃** | R5F监控A53心跳（共享内存） | R5F独立运行，维持遥控遥测 | 无需恢复 |
| **R5F任务死锁** | Watchdog_Task监控其他任务心跳 | 硬件看门狗复位整个系统 | <5秒 |
| **核间通信故障** | RPMsg超时检测 | 降级到本地缓存模式 | 立即 |

#### 2.2.5 资源分配

**内存分配**：

| 区域 | 大小 | 用途 | 访问权限 |
|------|------|------|---------|
| **R5F TCM** | 256KB | R5F代码+关键数据，零等待 | R5F独占 |
| **R5F DDR** | 8MB | 遥测缓存、日志缓冲区 | R5F独占 |
| **共享内存** | 4MB | 核间通信（遥测、心跳、日志） | R5F写，A53读 |
| **A53 DDR** | 512MB | Linux系统+应用 | A53独占 |

**外设分配**：

| 外设 | 分配 | 访问方式 | 优先级 |
|------|------|---------|--------|
| **CAN0** | R5F独占 | 直接寄存器操作 | 硬实时 |
| **CAN1** | R5F优先，A53备用 | 仲裁访问 | 硬实时 |
| **硬件看门狗** | R5F独占 | 直接寄存器操作 | 最高 |
| **Mailbox** | 共享 | 核间中断 | 高 |
| **Ethernet** | A53独占 | Linux驱动 | 低 |
| **UART0** | A53独占 | Linux驱动（调试串口） | 低 |
| **eMMC/SD** | A53独占 | Linux驱动（文件系统） | 低 |
| **I2C/SPI** | R5F优先，A53备用 | 仲裁访问 | 中 |
| **GPIO** | 共享 | 仲裁访问（电源控制） | 中 |

**CPU分配（A53侧）**：

| 进程 | 核心 | 调度策略 | 优先级 | 职责 |
|------|------|---------|--------|------|
| **Supervisor** | CPU1 | SCHED_FIFO | 50 | 监控R5F心跳、进程监控、故障恢复 |
| **Telemetry** | CPU3 | SCHED_FIFO | 70 | 后台遥测采集、通过共享内存同步到R5F |
| **Firmware** | CPU0 | SCHED_OTHER | 0 | 固件升级、通过RPMsg通知R5F |
| **Logger** | CPU0 | SCHED_OTHER | 0 | 日志收集、通过RPMsg请求R5F日志 |

#### 2.2.8 R5F任务架构设计

**R5F侧任务列表（FreeRTOS）**：

| 任务名称 | 优先级 | 栈大小 | 周期/触发 | 职责 | 时间预算 |
|---------|--------|--------|----------|------|---------|
| **CAN_RX_Task** | 255（最高） | 2KB | CAN中断触发 | CAN接收、命令解析、分发 | <200μs |
| **Watchdog_Task** | 250 | 1KB | 100ms周期 | 喂硬件看门狗、监控任务心跳 | <50μs |
| **Realtime_TM_Task** | 200 | 2KB | 命令触发 | 实时遥测查询、共享内存读取 | <800μs |
| **Telecontrol_Task** | 180 | 2KB | 命令触发 | 简单遥控执行、PDL调用 | <500μs |
| **IPC_Handler_Task** | 100 | 2KB | RPMsg触发 | 处理A53命令、复杂遥控转发 | <100ms |
| **Heartbeat_Task** | 50 | 1KB | 1秒周期 | 更新心跳、监控A53、状态上报 | <100μs |

**任务间通信机制**：

```c
/************************************************
 * R5F任务间通信（FreeRTOS队列）
 ************************************************/

// 命令队列（CAN_RX_Task → 其他任务）
typedef struct {
    QueueHandle_t realtime_tm_queue;    // 实时遥测命令队列（深度32）
    QueueHandle_t telecontrol_queue;    // 遥控命令队列（深度32）
    QueueHandle_t ipc_queue;            // IPC命令队列（深度16）
} r5f_task_queues_t;

// 命令消息结构
typedef struct {
    uint16_t cmd_id;                    // 命令ID
    uint32_t seq_num;                   // 序列号
    uint8_t  payload[64];               // 命令负载
    uint64_t timestamp_ns;              // 接收时间戳
} r5f_command_msg_t;

/************************************************
 * R5F任务实现示例
 ************************************************/

// CAN_RX_Task：CAN接收和命令分发
void CAN_RX_Task(void *arg) {
    r5f_task_queues_t *queues = (r5f_task_queues_t *)arg;
    can_frame_t frame;
    
    while (1) {
        // 1. 等待CAN中断信号（阻塞）
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        uint64_t start_time = get_time_ns();
        
        // 2. 从CAN硬件FIFO读取帧
        if (HAL_CAN_Receive(CAN0, &frame, 0) == OS_SUCCESS) {
            // 3. CRC校验
            if (verify_crc(&frame)) {
                // 4. 协议解析
                r5f_command_msg_t cmd;
                parse_can_frame(&frame, &cmd);
                
                // 5. 命令分类和分发
                command_class_t class = classify_command(cmd.cmd_id);
                
                switch (class) {
                    case CMD_CLASS_REALTIME_TM:
                        xQueueSend(queues->realtime_tm_queue, &cmd, 0);
                        break;
                    case CMD_CLASS_SIMPLE_TC:
                        xQueueSend(queues->telecontrol_queue, &cmd, 0);
                        break;
                    case CMD_CLASS_COMPLEX_TC:
                    case CMD_CLASS_SLOW_TM:
                        xQueueSend(queues->ipc_queue, &cmd, 0);
                        break;
                }
                
                // 6. 性能统计
                uint64_t latency_us = (get_time_ns() - start_time) / 1000;
                update_realtime_stats(latency_us);
            }
        }
    }
}

// Realtime_TM_Task：实时遥测处理
void Realtime_TM_Task(void *arg) {
    r5f_task_queues_t *queues = (r5f_task_queues_t *)arg;
    r5f_command_msg_t cmd;
    ipc_shared_memory_t *shm = get_shared_memory();
    
    while (1) {
        // 1. 等待命令（阻塞）
        if (xQueueReceive(queues->realtime_tm_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            uint64_t start_time = get_time_ns();
            
            // 2. 从共享内存读取遥测缓存
            uint32_t tm_value;
            int32_t ret = telemetry_cache_read(shm, cmd.cmd_id, &tm_value);
            
            // 3. 封装CAN应答
            can_frame_t response;
            build_tm_response(&response, cmd.seq_num, tm_value, ret);
            
            // 4. 发送CAN应答
            HAL_CAN_Send(CAN0, &response, 100);
            
            // 5. 性能统计
            uint64_t latency_us = (get_time_ns() - start_time) / 1000;
            update_realtime_stats(latency_us);
        }
    }
}

// Watchdog_Task：看门狗和任务监控
void Watchdog_Task(void *arg) {
    static uint64_t task_heartbeats[6] = {0};
    
    while (1) {
        // 1. 喂硬件看门狗
        HAL_Watchdog_Feed();
        
        // 2. 检查各任务心跳（通过任务通知计数）
        for (int i = 0; i < 6; i++) {
            uint64_t current_hb = get_task_heartbeat(i);
            if (current_hb == task_heartbeats[i]) {
                // 任务心跳未更新，可能死锁
                LOG_ERROR("WDT", "Task %d heartbeat timeout", i);
                // 不喂狗，触发硬件看门狗复位
                return;
            }
            task_heartbeats[i] = current_hb;
        }
        
        // 3. 延迟100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Heartbeat_Task：心跳和状态上报
void Heartbeat_Task(void *arg) {
    ipc_shared_memory_t *shm = get_shared_memory();
    
    while (1) {
        // 1. 更新R5F心跳到共享内存
        atomic_store(&shm->heartbeat.r5f_heartbeat_ns, get_time_ns());
        
        // 2. 更新R5F任务状态位图
        uint32_t task_status = get_all_task_status();
        atomic_store(&shm->heartbeat.r5f_task_status, task_status);
        
        // 3. 检查A53心跳
        uint64_t a53_hb = atomic_load(&shm->heartbeat.a53_heartbeat_ns);
        if (get_time_ns() - a53_hb > 5000000000ULL) {  // 5秒超时
            LOG_ERROR("R5F", "A53 heartbeat timeout");
            // 进入降级模式
            enable_r5f_degraded_mode();
        }
        
        // 4. 延迟1秒
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**R5F启动流程**：

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                        R5F启动流程                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. Bootloader阶段（A53 U-Boot）                                             │
│     ├─ 加载Linux内核到DDR                                                    │
│     ├─ 加载R5F固件到DDR（/lib/firmware/r5f_firmware.elf）                    │
│     └─ 启动Linux内核                                                         │
│                                                                             │
│  2. Linux启动阶段（A53）                                                     │
│     ├─ 加载RemoteProc驱动                                                    │
│     ├─ 加载RPMsg驱动                                                         │
│     ├─ 初始化共享内存（/dev/shm/pmc_ipc）                                    │
│     └─ 通过RemoteProc启动R5F                                                 │
│        echo "start" > /sys/class/remoteproc/remoteproc0/state               │
│                                                                             │
│  3. R5F固件加载阶段（RemoteProc驱动）                                        │
│     ├─ 从DDR拷贝固件到TCM（代码段）                                          │
│     ├─ 初始化TCM数据段                                                       │
│     ├─ 配置MPU（TCM可执行，DDR可读写）                                       │
│     ├─ 配置中断向量表                                                        │
│     └─ 释放R5F复位，开始执行                                                 │
│                                                                             │
│  4. R5F初始化阶段（FreeRTOS）                                                │
│     ├─ 初始化FreeRTOS内核                                                    │
│     ├─ 初始化OSAL层（FreeRTOS实现）                                          │
│     ├─ 初始化HAL层（CAN/GPIO/Watchdog）                                      │
│     ├─ 加载ACL配置（从TCM读取，编译时链接）                                  │
│     ├─ 初始化PDL层（Satellite/MCU服务）                                      │
│     ├─ 映射共享内存（DDR，0xA0000000）                                       │
│     ├─ 初始化RPMsg通信                                                       │
│     └─ 创建6个任务                                                           │
│                                                                             │
│  5. R5F运行阶段                                                              │
│     ├─ 所有任务进入就绪状态                                                  │
│     ├─ FreeRTOS调度器开始运行                                                │
│     ├─ Heartbeat_Task发送心跳到A53                                           │
│     ├─ CAN_RX_Task等待CAN中断                                                │
│     └─ 系统进入正常运行状态                                                  │
│                                                                             │
│  总启动时间：<500ms                                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.2.9 ACL配置加载机制

**ACL配置在多核环境下的部署策略**：

| 配置项 | R5F侧 | A53侧 | 同步机制 |
|--------|-------|-------|---------|
| **遥控命令映射** | 编译时链接到TCM | 运行时从文件加载 | 编译时保证一致 |
| **遥测数据映射** | 编译时链接到TCM | 运行时从文件加载 | 编译时保证一致 |
| **遥控失效映射** | 编译时链接到共享内存 | 只读访问共享内存 | 编译时生成 |
| **设备配置** | 简化版（只包含CAN设备） | 完整版（所有设备） | 各自独立 |

**R5F侧ACL配置加载**：

```c
/************************************************
 * R5F侧ACL配置（编译时生成，链接到TCM）
 ************************************************/

// ACL配置表（编译时生成，存储在TCM）
typedef struct {
    uint32_t version;                           // 配置版本号
    uint32_t tc_count;                          // 遥控命令数量
    uint32_t tm_count;                          // 遥测数据数量
    
    // 遥控命令配置表（数组索引 = 命令ID）
    struct {
        uint16_t cmd_id;                        // 命令ID
        uint8_t  device_type;                   // 设备类型（MCU/SATELLITE）
        uint8_t  device_index;                  // 设备索引
        uint8_t  interface_type;                // 接口类型（CAN/GPIO）
        uint8_t  cmd_class;                     // 命令分类（简单/复杂）
        uint16_t timeout_ms;                    // 超时时间
    } tc_config[256];
    
    // 遥测数据配置表（数组索引 = 遥测ID）
    struct {
        uint16_t tm_id;                         // 遥测ID
        uint8_t  tm_type;                       // 遥测类型（缓存/实时）
        uint8_t  data_type;                     // 数据类型（uint8/uint16/uint32）
        uint16_t cache_index;                   // 缓存索引（在共享内存中的位置）
        uint16_t timeout_ms;                    // 超时时间
    } tm_config[512];
    
} r5f_acl_config_t __attribute__((section(".tcm_data")));

// 全局ACL配置实例（链接到TCM）
r5f_acl_config_t g_r5f_acl_config __attribute__((section(".tcm_data"))) = {
    .version = 0x00010000,  // v1.0
    .tc_count = 32,
    .tm_count = 128,
    
    // 遥控命令配置（编译时生成）
    .tc_config = {
        {.cmd_id = TC_SERVER_POWER_ON, .device_type = ACL_DEVICE_BMC, 
         .device_index = 0, .cmd_class = CMD_CLASS_COMPLEX_TC, .timeout_ms = 5000},
        {.cmd_id = TC_MCU_RESET, .device_type = ACL_DEVICE_MCU, 
         .device_index = 0, .cmd_class = CMD_CLASS_SIMPLE_TC, .timeout_ms = 1000},
        // ... 更多配置
    },
    
    // 遥测数据配置（编译时生成）
    .tm_config = {
        {.tm_id = TM_CPU_TEMP, .tm_type = TM_TYPE_CACHED, 
         .data_type = DATA_TYPE_UINT16, .cache_index = 0, .timeout_ms = 100},
        {.tm_id = TM_SERVER_POWER_STATE, .tm_type = TM_TYPE_REALTIME, 
         .data_type = DATA_TYPE_UINT8, .cache_index = 1, .timeout_ms = 5000},
        // ... 更多配置
    }
};

// R5F侧ACL查询接口（O(1)查找）
acl_tc_config_t* acl_get_tc_config(uint16_t cmd_id) {
    if (cmd_id < g_r5f_acl_config.tc_count) {
        return &g_r5f_acl_config.tc_config[cmd_id];
    }
    return NULL;
}

acl_tm_config_t* acl_get_tm_config(uint16_t tm_id) {
    if (tm_id < g_r5f_acl_config.tm_count) {
        return &g_r5f_acl_config.tm_config[tm_id];
    }
    return NULL;
}
```

**A53侧ACL配置加载**：

```c
/************************************************
 * A53侧ACL配置（运行时从文件加载）
 ************************************************/

// A53侧ACL配置文件路径
#define ACL_CONFIG_FILE  "/etc/pmc/acl_pmc_v1.conf"

// A53侧ACL配置加载
int32_t acl_load_config_a53(const char *config_file) {
    FILE *fp = fopen(config_file, "r");
    if (fp == NULL) {
        LOG_ERROR("ACL", "Failed to open config file: %s", config_file);
        return OS_ERROR;
    }
    
    // 1. 读取配置文件（JSON格式）
    char buffer[64 * 1024];
    size_t len = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);
    
    // 2. 解析JSON配置
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        LOG_ERROR("ACL", "Failed to parse config file");
        return OS_ERROR;
    }
    
    // 3. 加载遥控命令配置
    cJSON *tc_array = cJSON_GetObjectItem(root, "telecommands");
    int tc_count = cJSON_GetArraySize(tc_array);
    for (int i = 0; i < tc_count; i++) {
        cJSON *tc = cJSON_GetArrayItem(tc_array, i);
        // 解析并存储配置
        // ...
    }
    
    // 4. 加载遥测数据配置
    cJSON *tm_array = cJSON_GetObjectItem(root, "telemetry");
    int tm_count = cJSON_GetArraySize(tm_array);
    for (int i = 0; i < tm_count; i++) {
        cJSON *tm = cJSON_GetArrayItem(tm_array, i);
        // 解析并存储配置
        // ...
    }
    
    cJSON_Delete(root);
    
    LOG_INFO("ACL", "Loaded %d TC configs and %d TM configs", tc_count, tm_count);
    return OS_SUCCESS;
}
```

**配置一致性保证**：

```bash
#!/bin/bash
# 配置生成脚本（编译时执行）

# 1. 从JSON配置生成R5F的C代码（编译时链接）
python3 tools/acl_gen_r5f.py \
    --input config/acl_pmc_v1.json \
    --output r5f/acl/acl_config_generated.c

# 2. 拷贝JSON配置到A53文件系统（运行时加载）
cp config/acl_pmc_v1.json buildroot/overlay/etc/pmc/acl_pmc_v1.conf

# 3. 生成遥控失效映射表（编译时链接到共享内存）
python3 tools/acl_gen_invalidation.py \
    --input config/acl_pmc_v1.json \
    --output shared/acl_invalidation_map.c

# 4. 校验R5F和A53配置一致性
python3 tools/acl_verify_consistency.py \
    --r5f r5f/acl/acl_config_generated.c \
    --a53 config/acl_pmc_v1.json
```

---

### 2.3. 关键文件清单

#### 2.3.1 整体目录结构

```
EMS/
├── osal/                                    # 操作系统抽象层
│   ├── include/
│   │   ├── sys/
│   │   │   ├── osal_process_mgmt.h         # 进程管理接口（fork/exec/wait/kill）
│   │   │   └── osal_sched.h                # 实时调度接口（SCHED_FIFO/CPU亲和性/内存锁定）
│   │   ├── ipc/
│   │   │   ├── osal_shm.h                  # 共享内存接口（shm_open/mmap）
│   │   │   ├── osal_rpmsg.h                # RPMsg接口（核间消息传递）
│   │   │   ├── osal_mailbox.h              # Mailbox接口（核间中断）
│   │   │   ├── osal_log_ring_buffer.h      # 日志环形缓冲区（通用IPC机制，4096条目）
│   │   │   └── osal_heartbeat_table.h      # 心跳表（通用IPC机制，原子时间戳）
│   │   └── multicore/
│   │       ├── osal_multicore.h            # 多核抽象接口（CPU亲和性、实时调度）
│   │       └── osal_remoteproc.h           # RemoteProc接口（R5F启动/停止/复位）
│   └── src/
│       ├── posix/                          # Linux实现（A53侧）
│       │   ├── osal_process_mgmt.c         # 进程管理实现
│       │   ├── osal_sched.c                # 实时调度实现
│       │   ├── osal_shm.c                  # 共享内存实现
│       │   ├── osal_rpmsg.c                # RPMsg实现（/dev/rpmsg_*）
│       │   ├── osal_mailbox.c              # Mailbox实现（/dev/mailbox_*）
│       │   ├── osal_log_ring_buffer.c      # 日志环形缓冲区实现
│       │   ├── osal_heartbeat_table.c      # 心跳表实现
│       │   └── osal_remoteproc.c           # RemoteProc实现（/sys/class/remoteproc/）
│       └── freertos/                       # FreeRTOS实现（R5F侧）
│           ├── osal_task.c                 # 任务管理实现
│           ├── osal_queue.c                # 队列实现
│           ├── osal_mutex.c                # 互斥锁实现
│           ├── osal_rpmsg.c                # RPMsg实现（TI IPC库）
│           ├── osal_mailbox.c              # Mailbox实现（TI IPC库）
│           └── osal_shm.c                  # 共享内存实现（DDR映射）
│
├── hal/                                     # 硬件抽象层（现有）
│   ├── include/
│   │   ├── hal_gpio.h                      # GPIO抽象接口
│   │   ├── hal_uart.h                      # UART抽象接口
│   │   ├── hal_i2c.h                       # I2C抽象接口
│   │   ├── hal_spi.h                       # SPI抽象接口
│   │   ├── hal_can.h                       # CAN抽象接口
│   │   └── hal_watchdog.h                  # 看门狗抽象接口
│   └── src/
│       ├── linux/                          # Linux平台实现（A53侧）
│       │   ├── hal_gpio_linux.c
│       │   ├── hal_uart_linux.c
│       │   ├── hal_i2c_linux.c
│       │   ├── hal_spi_linux.c
│       │   ├── hal_can_linux.c
│       │   └── hal_watchdog_linux.c
│       └── ti_r5f/                         # TI R5F平台实现（R5F侧）
│           ├── hal_gpio_r5f.c              # GPIO寄存器操作
│           ├── hal_uart_r5f.c              # UART寄存器操作
│           ├── hal_i2c_r5f.c               # I2C寄存器操作
│           ├── hal_spi_r5f.c               # SPI寄存器操作
│           ├── hal_can_r5f.c               # CAN寄存器操作（MCAN）
│           └── hal_watchdog_r5f.c          # 看门狗寄存器操作
│
├── pcl/                                     # 外设配置层（现有）
│   ├── include/
│   │   ├── api/
│   │   │   └── pcl_api.h                   # PCL统一查询接口
│   │   ├── internal/
│   │   │   ├── pcl.h                       # PCL内部接口
│   │   │   ├── pcl_common.h                # PCL通用类型定义
│   │   │   └── pcl_board.h                 # 板级配置接口
│   │   └── peripheral/
│   │       ├── pcl_mcu.h                   # MCU外设配置结构
│   │       ├── pcl_bmc.h                   # BMC外设配置结构
│   │       ├── pcl_satellite.h             # Satellite外设配置结构
│   │       ├── pcl_sensor.h                # 传感器配置结构
│   │       ├── pcl_storage.h               # 存储配置结构
│   │       └── pcl_hardware_interface.h    # 硬件接口配置（CAN/UART/I2C/SPI）
│   ├── src/
│   │   ├── pcl_api.c                       # PCL查询接口实现
│   │   └── pcl_register.c                  # PCL注册机制实现
│   └── platform/
│       ├── ti/
│       │   └── am6254/
│       │       ├── carrier_board_v1/
│       │       │   └── ti_am6254_carrier_board_v1.c    # TI AM6254载板v1硬件配置
│       │       └── carrier_board_v2/
│       │           └── ti_am6254_carrier_board_v2.c    # TI AM6254载板v2硬件配置
│       └── vendor_demo/
│           └── platform_demo/
│               └── vendor_demo_platform_demo_v1.c      # Demo平台硬件配置
│
├── pdl/                                     # 外设驱动层（现有）
│   ├── include/
│   │   ├── pdl_mcu.h                       # MCU驱动接口（独立）
│   │   ├── pdl_bmc.h                       # BMC驱动接口（独立）
│   │   ├── pdl_satellite.h                 # Satellite驱动接口（独立）
│   │   └── pdl_watchdog.h                  # 看门狗驱动接口
│   └── src/
│       ├── pdl_mcu/                        # MCU驱动实现（独立模块）
│       │   ├── pdl_mcu.c                   # MCU核心模块（对外API）
│       │   ├── pdl_mcu_protocol.c          # MCU协议处理模块
│       │   ├── pdl_mcu_can.c               # MCU CAN通信模块
│       │   ├── pdl_mcu_serial.c            # MCU串口通信模块
│       │   └── pdl_mcu_internal.h          # MCU内部头文件
│       ├── pdl_bmc/                        # BMC驱动实现（独立模块）
│       │   ├── pdl_bmc.c                   # BMC核心模块（对外API）
│       │   ├── pdl_bmc_redfish.c           # Redfish协议实现
│       │   ├── pdl_bmc_ipmi.c              # IPMI协议实现
│       │   ├── pdl_bmc_transport.c         # BMC传输层（网络/串口）
│       │   └── pdl_bmc_internal.h          # BMC内部头文件
│       ├── pdl_satellite/                  # Satellite驱动实现（独立模块）
│       │   ├── pdl_satellite.c             # Satellite核心模块（对外API）
│       │   ├── pdl_satellite_can.c         # Satellite CAN通信模块
│       │   └── pdl_satellite_internal.h    # Satellite内部头文件
│       └── pdl_watchdog.c                  # 看门狗驱动实现
│
├── acl/                                     # 应用配置层（新增）
│   ├── include/
│   │   ├── pmc_acl_types.h                 # PMC业务功能枚举（遥控/遥测/健康管理）
│   │   ├── acl_config.h                    # ACL配置结构（设备映射+数据类型）
│   │   ├── acl_telemetry_cache.h           # 遥测缓冲区结构（业务数据，双缓冲无锁读写）
│   │   └── acl_status_snapshot.h           # 状态快照结构（业务数据，服务器+外设状态）
│   ├── src/
│   │   ├── acl_core.c                      # ACL核心实现（O(1)查找表）
│   │   ├── acl_telemetry_cache.c           # 遥测缓存管理实现
│   │   └── acl_status_snapshot.c           # 状态快照管理实现
│   └── config/
│       └── pmc_v1/
│           ├── acl_pmc_v1.c                # PMC v1.0配置（命名规范：acl_{project}_{product}_{version}.c）
│           └── acl_pmc_v1_invalidation.c   # 遥控命令失效映射
│
├── apps/                                     # 应用层（新增）
│   ├── supervisor/
│   │   ├── supervisor.c                    # Supervisor进程（<1000行，心跳检测+进程重启）
│   │   └── pmc_shm_layout.h                # PMC共享内存布局定义（应用级设计决策）
│   │
│   ├── telecommand/
│   │   ├── telecommand.c                   # Telecommand进程主程序（CAN接收+遥控处理）
│   │   ├── can_rx_handler.c                # CAN接收处理线程（2ms应答关键路径）
│   │   ├── realtime_tm_handler.c           # 实时遥测处理线程（PDL查询<500μs）
│   │   └── telecontrol_executor.c          # 遥控执行线程（异步执行）
│   │
│   ├── telemetry/
│   │   ├── telemetry.c                     # Telemetry进程主程序（后台采集）
│   │   ├── cache_collector.c               # 缓存采集线程（1秒周期，双缓冲写入）
│   │   ├── health_monitor.c                # 健康监控线程（5秒周期，异常检测）
│   │   └── status_snapshot.c               # 状态快照线程（10秒周期，JSON格式）
│   │
│   ├── firmware/
│   │   ├── firmware.c                      # Firmware进程主程序（固件升级）
│   │   ├── upgrade_control.c               # 升级控制线程（A/B分区管理）
│   │   └── verify_thread.c                 # 校验线程（CRC32+签名验证）
│   │
│   └── logger/
│       ├── logger.c                        # Logger进程主程序（日志收集）
│       ├── log_collector.c                 # 日志收集线程（共享内存环形缓冲区）
│       ├── status_archiver.c               # 状态归档线程（JSON压缩存储）
│       ├── crash_analyzer.c                # 崩溃分析线程（coredump+backtrace）
│       └── log_rotator.c                   # 日志轮转线程（按大小/时间轮转）
│
│
└── tests/                                   # 测试目录
    ├── unit/                                # 单元测试
    │   ├── acl/
    │   │   └── test_acl_lookup.c           # ACL查找测试（O(1)性能验证）
    │   ├── osal/
    │   │   ├── test_osal_process.c         # 进程管理测试（fork/exec/wait）
    │   │   ├── test_osal_shm.c             # 共享内存测试（双缓冲/原子操作）
    │   │   └── test_osal_sched.c           # 实时调度测试（SCHED_FIFO/优先级）
    │   ├── pdl/
    │   │   ├── test_pdl_mcu.c              # MCU驱动测试
    │   │   ├── test_pdl_bmc.c              # BMC驱动测试
    │   │   └── test_pdl_satellite.c        # Satellite驱动测试
    │   └── pcl/
    │       └── test_pcl_api.c              # PCL查询接口测试
    │
    ├── integration/                         # 集成测试
    │   ├── test_2ms_latency.c              # 2ms延迟测试（关键路径验证）
    │   ├── test_process_crash.c            # 进程崩溃测试（三级恢复机制）
    │   └── test_seu_simulation.c           # SEU模拟测试（数据损坏+恢复）
    │
    └── stress/                              # 压力测试
        ├── test_high_frequency.c           # 高频压力测试（每秒100条命令）
        └── test_long_term.c                # 长期稳定性测试（7×24小时）
```

#### 2.3.2 关键文件说明

- OSAL层扩展（进程间通信 + 多核支持）

| 文件                     | 行数估算 | 说明                                        |
| ------------------------ | -------- | ------------------------------------------- |
| `osal_process_mgmt.h`    | ~150行   | 进程管理接口：fork/exec/wait/kill/getpid    |
| `osal_sched.h`           | ~100行   | 实时调度接口：SCHED_FIFO/CPU亲和性/内存锁定 |
| `osal_shm.h`             | ~120行   | 共享内存接口：shm_open/mmap/munmap          |
| `osal_multicore.h`       | ~150行   | 多核抽象接口：CPU亲和性、实时调度           |
| `osal_log_ring_buffer.h` | ~150行   | 日志环形缓冲区接口（通用IPC机制）           |
| `osal_heartbeat_table.h` | ~100行   | 心跳表接口（通用IPC机制）                   |
| `osal_process_mgmt.c`    | ~300行   | 进程管理实现（POSIX封装）                   |
| `osal_sched.c`           | ~200行   | 实时调度实现（sched_setscheduler/mlockall） |
| `osal_shm.c`             | ~250行   | 共享内存实现（mmap/原子操作）               |
| `osal_log_ring_buffer.c` | ~200行   | 日志环形缓冲区实现（无锁读写）              |
| `osal_heartbeat_table.c` | ~150行   | 心跳表实现（原子操作）                      |

- HAL层（现有，5个头文件 + 5个实现文件）

| 文件            | 行数估算  | 说明                                |
| --------------- | --------- | ----------------------------------- |
| `hal_gpio.h`    | ~100行    | GPIO抽象接口（输入/输出/中断）      |
| `hal_uart.h`    | ~120行    | UART抽象接口（配置/收发/流控）      |
| `hal_i2c.h`     | ~100行    | I2C抽象接口（主模式/从模式）        |
| `hal_spi.h`     | ~100行    | SPI抽象接口（主模式/从模式）        |
| `hal_can.h`     | ~150行    | CAN抽象接口（标准帧/扩展帧/过滤）   |
| `hal_*_linux.c` | ~200行/个 | Linux平台实现（5个文件，共~1000行） |

- PCL层（现有，10个头文件 + 2个实现文件 + 平台配置）

| 文件                                      | 行数估算  | 说明                              |
| ----------------------------------------- | --------- | --------------------------------- |
| `pcl_api.h`                               | ~150行    | PCL统一查询接口（按逻辑索引查询） |
| `pcl_common.h`                            | ~200行    | PCL通用类型定义（设备配置结构）   |
| `pcl_mcu.h`                               | ~150行    | MCU外设配置结构                   |
| `pcl_bmc.h`                               | ~200行    | BMC外设配置结构（Redfish/IPMI）   |
| `pcl_satellite.h`                         | ~150行    | Satellite外设配置结构             |
| `pcl_sensor.h`                            | ~100行    | 传感器配置结构                    |
| `pcl_storage.h`                           | ~100行    | 存储配置结构                      |
| `pcl_hardware_interface.h`                | ~200行    | 硬件接口配置（CAN/UART/I2C/SPI）  |
| `pcl_api.c`                               | ~400行    | PCL查询接口实现                   |
| `pcl_register.c`                          | ~300行    | PCL注册机制实现                   |
| `{platform}_{chip}_{project}_{version}.c` | ~500行/个 | 平台硬件配置（按命名规范）        |

- PDL层（现有，4个头文件 + 13个实现文件）

| 文件                  | 行数估算 | 说明                                    |
| --------------------- | -------- | --------------------------------------- |
| `pdl_mcu.h`           | ~300行   | MCU驱动接口（电源/复位/看门狗/传感器）  |
| `pdl_bmc.h`           | ~400行   | BMC驱动接口（Redfish/IPMI/电源/传感器） |
| `pdl_satellite.h`     | ~250行   | Satellite驱动接口（遥控/遥测/心跳）     |
| `pdl_watchdog.h`      | ~150行   | 看门狗驱动接口                          |
| `pdl_mcu.c`           | ~600行   | MCU核心模块（对外API）                  |
| `pdl_mcu_protocol.c`  | ~500行   | MCU协议处理模块                         |
| `pdl_mcu_can.c`       | ~400行   | MCU CAN通信模块                         |
| `pdl_mcu_serial.c`    | ~350行   | MCU串口通信模块                         |
| `pdl_bmc.c`           | ~700行   | BMC核心模块（对外API）                  |
| `pdl_bmc_redfish.c`   | ~800行   | Redfish协议实现                         |
| `pdl_bmc_ipmi.c`      | ~600行   | IPMI协议实现                            |
| `pdl_bmc_transport.c` | ~500行   | BMC传输层（网络/串口）                  |
| `pdl_satellite.c`     | ~500行   | Satellite核心模块（对外API）            |
| `pdl_satellite_can.c` | ~400行   | Satellite CAN通信模块                   |
| `pdl_watchdog.c`      | ~300行   | 看门狗驱动实现                          |

- ACL层（新增，4个头文件 + 4个实现文件 + 配置文件）

| 文件                     | 行数估算 | 说明                                       |
| ------------------------ | -------- | ------------------------------------------ |
| `pmc_acl_types.h`        | ~200行   | PMC业务功能枚举（遥控/遥测/健康管理）      |
| `acl_config.h`           | ~150行   | ACL配置结构（设备映射+数据类型）           |
| `acl_telemetry_cache.h`  | ~200行   | 遥测缓冲区结构（业务数据，双缓冲无锁读写） |
| `acl_status_snapshot.h`  | ~150行   | 状态快照结构（业务数据，服务器+外设状态）  |
| `acl_core.c`                 | ~300行   | ACL核心实现（O(1)查找表初始化）            |
| `acl_telemetry_cache.c`      | ~250行   | 遥测缓存管理实现                           |
| `acl_status_snapshot.c`      | ~200行   | 状态快照管理实现                           |
| `acl_pmc_v1.c`               | ~500行   | PMC v1.0配置（BMC/MCU/FPGA映射表）         |
| `acl_pmc_v1_invalidation.c`  | ~200行   | 遥控命令失效映射                           |

- 应用层（5个Linux进程）

| 进程/线程       | 文件                   | 行数估算 | 说明                                  |
| --------------- | ---------------------- | -------- | ------------------------------------- |
| **Supervisor**  | `supervisor.c`         | ~800行   | 监控进程，心跳检测+进程重启+故障恢复  |
|                 | `pmc_shm_layout.h`     | ~150行   | PMC共享内存布局定义（应用级设计决策） |
| **Telecommand** | `telecommand.c`        | ~600行   | 遥控进程主程序，CAN接收+遥控处理      |
|                 | `can_rx_handler.c`     | ~500行   | CAN接收处理线程，2ms应答关键路径      |
|                 | `realtime_tm_handler.c`| ~400行   | 实时遥测处理线程，PDL查询<500μs       |
|                 | `telecontrol_executor.c`| ~450行  | 遥控执行线程，异步执行                |
| **Telemetry**   | `telemetry.c`          | ~350行   | 后台采集进程主程序                    |
|                 | `cache_collector.c`    | ~450行   | 缓存采集线程，1秒周期双缓冲写入       |
|                 | `health_monitor.c`     | ~400行   | 健康监控线程，5秒周期异常检测         |
|                 | `status_snapshot.c`    | ~300行   | 状态快照线程，10秒周期JSON格式        |
| **Firmware**    | `firmware.c`           | ~350行   | 固件升级进程主程序                    |
|                 | `upgrade_control.c`    | ~600行   | 升级控制线程，A/B分区管理             |
|                 | `verify_thread.c`      | ~300行   | 校验线程，CRC32+签名验证              |
| **Logger**      | `logger.c`             | ~300行   | 日志收集进程主程序                    |
|                 | `log_collector.c`      | ~400行   | 日志收集线程，共享内存环形缓冲区      |
|                 | `status_archiver.c`    | ~350行   | 状态归档线程，JSON压缩存储            |
|                 | `crash_analyzer.c`     | ~450行   | 崩溃分析线程，coredump+backtrace      |
|                 | `log_rotator.c`        | ~250行   | 日志轮转线程，按大小/时间轮转         |

- 测试文件（多核和实时性测试）

| 文件                          | 行数估算 | 说明                              |
| ----------------------------- | -------- | --------------------------------- |
| `test_acl_lookup.c`           | ~300行   | ACL查找测试，验证O(1)性能         |
| `test_osal_process.c`         | ~400行   | 进程管理测试，fork/exec/wait      |
| `test_osal_shm.c`             | ~450行   | 共享内存测试，mmap/munmap         |
| `test_osal_sched.c`           | ~350行   | 实时调度测试，SCHED_FIFO/优先级   |
| `test_osal_multicore.c`       | ~400行   | 多核测试，CPU亲和性、核心隔离     |
| `test_osal_log_ring_buffer.c` | ~350行   | 日志环形缓冲区测试，无锁读写      |
| `test_osal_heartbeat_table.c` | ~300行   | 心跳表测试，原子操作              |
| `test_pdl_mcu.c`              | ~400行   | MCU驱动测试                       |
| `test_pdl_bmc.c`              | ~450行   | BMC驱动测试                       |
| `test_pdl_satellite.c`        | ~400行   | Satellite驱动测试                 |
| `test_pcl_api.c`              | ~350行   | PCL查询接口测试                   |
| `test_2ms_latency.c`          | ~600行   | 2ms延迟测试，验证实时路径         |
| `test_process_crash.c`        | ~450行   | 进程崩溃测试，三级恢复机制        |
| `test_seu_simulation.c`       | ~500行   | SEU模拟测试，数据损坏+恢复        |
| `test_high_frequency.c`       | ~550行   | 高频压力测试，每秒100条命令       |
| `test_long_term.c`            | ~600行   | 长期稳定性测试，7×24小时          |
| `test_acl_telemetry_cache.c`  | ~400行   | 遥测缓存测试，FRESH/STALE/INVALID |
| `test_acl_status_snapshot.c`  | ~300行   | 状态快照测试，JSON序列化          |

#### 2.3.3 代码量统计

| 模块           | 文件数 | 总行数估算    | 说明                                                 |
| -------------- | ------ | ------------- | ---------------------------------------------------- |
| **OSAL层扩展** | 10     | ~2,020行      | 进程管理+实时调度+共享内存+日志缓冲区+心跳表（新增） |
| **HAL层**      | 10     | ~1,570行      | GPIO/UART/I2C/SPI/CAN抽象（现有）                    |
| **PCL层**      | 13+    | ~2,550行+     | 外设配置+平台配置（现有，需扩展）                    |
| **PDL层**      | 17     | ~6,400行      | MCU/BMC/Satellite驱动（现有）                        |
| **ACL层**      | 9      | ~2,000行      | 业务功能映射+遥测缓存+状态快照（新增）               |
| **应用层**     | 18     | ~7,350行      | 5个进程+18个线程+共享内存布局（新增）                |
| **测试代码**   | 18     | ~7,050行      | 单元+集成+压力测试（新增）                           |
| **新增代码**   | **55** | **~18,420行** | 本次架构优化新增代码                                 |
| **现有代码**   | **40** | **~10,520行** | 现有OSAL/HAL/PCL/PDL层                               |
| **总计**       | **95** | **~28,940行** | 完整PMC系统代码量                                    |
| **应用层**     | 18     | ~6,900行      | 4个进程+15个线程+共享内存布局（新增）                |
| **测试代码**   | 15     | ~5,650行      | 单元+集成+压力测试（新增）                           |
| **新增代码**   | **52** | **~16,570行** | 本次架构优化新增代码                                 |
| **现有代码**   | **40** | **~10,520行** | 现有OSAL/HAL/PCL/PDL层                               |
| **总计**       | **92** | **~27,090行** | 完整PMC系统代码量                                    |



---




## 3. 详细设计文档索引

本架构方案的详细设计内容已拆分为独立文档，便于阅读和维护。各层详细设计文档如下：

### 3.1 核心层设计

- **OSAL层** - 操作系统抽象层
  - 进程管理接口（fork/exec/wait/kill）
  - 实时调度接口（SCHED_FIFO/CPU亲和性/内存锁定）
  - 共享内存接口（shm_open/mmap）
  - 核间通信接口（RPMsg/Mailbox）
  - 通用IPC机制（日志环形缓冲区、心跳表）
  - 多核抽象接口（CPU亲和性、RemoteProc）
  - 详细设计：[1-1-osal_design.md](./1-1-osal_design.md)

- **HAL层** - 硬件抽象层
  - 硬件驱动接口（CAN/UART/I2C/SPI/GPIO/Watchdog）
  - Linux平台实现（A53侧）
  - TI R5F平台实现（R5F侧，寄存器操作）
  - 平台移植指南
  - 详细设计：[1-2-hal_design.md](./1-2-hal_design.md)

- **PCL层** - 外设配置层
  - 硬件配置数据结构
  - 平台配置组织（vendor/chip/product）
  - PCL统一查询接口
  - 配置注册机制
  - 详细设计：[1-3-pcl_design.md](./1-3-pcl_design.md)

- **PDL层** - 外设驱动层
  - 独立外设服务（Satellite/BMC/MCU）
  - 协议实现（Redfish/IPMI/CAN）
  - 双通道冗余与自动切换
  - 看门狗驱动
  - 详细设计：[1-4-pdl_design.md](./1-4-pdl_design.md)

- **ACL层** - 应用配置层
  - PMC业务功能枚举（遥控/遥测/健康管理）
  - 设备映射配置（O(1)查找）
  - 遥测缓存结构（双缓冲无锁读写）
  - 状态快照结构（服务器+外设状态）
  - 遥控命令失效映射
  - 详细设计：[1-5-acl_design.md](./1-5-acl_design.md)

- **APP层** - 应用层
  - 5个进程架构（Supervisor/Telecommand/Telemetry/Firmware/Logger）
  - 进程间通信机制（共享内存/心跳监控）
  - 实时调度策略（SCHED_FIFO/CPU绑定）
  - 故障恢复机制（进程重启/降级运行）
  - 详细设计：[1-6-app_design.md](./1-6-app_design.md)

### 3.2 专项设计

- **抗辐射设计**
  - TMR三模冗余机制
  - SEU单粒子翻转检测与恢复
  - 关键数据保护策略
  - 内存纠错码（ECC）
  - 详细设计：[2-1-radiation_hardening.md](./2-1-radiation_hardening.md)

- **容错与恢复机制**
  - 三级故障恢复机制
  - 进程崩溃检测与重启
  - 核间通信故障处理
  - 看门狗机制
  - 故障隔离（安全岛设计）
  - 详细设计：[2-2-fault_tolerance.md](./2-2-fault_tolerance.md)

- **性能优化**
  - 2ms硬实时路径优化
  - 多核CPU绑定与调度策略
  - 内存优化（TCM/DDR分配）
  - 零拷贝技术
  - 性能分析与测试结果
  - 详细设计：[2-3-performance_optimization.md](./2-3-performance_optimization.md)

### 3.3 实施与测试

- **测试策略**
  - 单元测试（OSAL/HAL/PCL/PDL/ACL/APP）
  - 集成测试（2ms延迟、进程崩溃、SEU模拟）
  - 压力测试（高频、长期稳定性）
  - 测试覆盖率要求
  - 详细设计：[3-1-testing_strategy.md](./3-1-testing_strategy.md)

- **实施计划**
  - 启动流程（R5F/A53双核启动）
  - 开发阶段划分
  - 里程碑与交付物
  - 风险评估与应对
  - 详细设计：[3-2-implementation_plan.md](./3-2-implementation_plan.md)

---

## 4. 总结

### 4.1 方案核心特点

1. ✅ **满足2ms硬实时**

- **R5F硬实时路径**：关键路径全部在R5F完成，无Linux调度干扰

- **FreeRTOS调度**：确定性实时响应，抖动±20μs

- **TCM零等待**：关键代码和数据在TCM，零等待访问

- **实测延迟**：<1.32ms（远小于2ms）

2. ✅ **航空级可靠性**

- **硬件级故障隔离**（R5F独立运行，A53崩溃不影响R5F）

- **进程级故障隔离**（A53侧MMU硬件保护）

- **抗辐射**（R5F独立电源域 + ECC内存 + 硬件隔离）

- **三级故障恢复**（任务级 → 核心级 → 系统级）

- **降级运行**（R5F独立运行模式 + A53降级模式）

3. ✅ **多核异构优化**

- **A53侧**：4核负载均衡，处理复杂业务（Redfish/IPMI/固件升级/日志）

- **R5F侧**：双核实时处理，保证硬实时（CAN接收/遥控遥测应答）

- **核间通信**：RPMsg消息传递（<100μs）+ 共享内存（<10μs）+ 硬件Mailbox（<5μs）

- **负载分离**：A53负载<22%，R5F负载<15%，互不干扰

4. ✅ **业务解耦**

- ACL层（业务功能 → 设备映射）

- O(1)查找（直接索引）

- 遥测分类（缓存型/实时型）

- 支持多平台配置

5. ✅ **严格6层架构**

- 完全解耦（APP/ACL/PDL/PCL/HAL/OSAL）

- 单向依赖（每层只访问下层公开API）

- 无common/目录（所有代码明确归属）

- 职责清晰（每层职责明确，禁止事项清晰）

6. ✅ **PDL独立外设设计**

- 每种外设独立API（Satellite/BMC/MCU各自独立）

- 无统一抽象接口（避免最小公分母/最大公约数陷阱）

- ACL层配置指定外设类型（device_type + device_index）

- 接口贴合特性（无冗余参数和类型判断）

7. ✅ **完整日志**

- 独立Logger进程

- 运行日志（ERROR/WARN/INFO/DEBUG）

- 状态日志（JSON格式，10秒快照）

- 崩溃日志（coredump + backtrace）

- 日志轮转（压缩、归档、自动清理）

### 4.2 与v3.0/v3.1的差异

相比v3.0单进程：

- ✅ 更可靠（硬件级隔离 vs 线程隔离）

- ✅ 更抗辐射（R5F独立电源域 vs 共享内存）

- ✅ 符合航天标准（NASA/ESA要求硬件级隔离）

- ⚖️ 略高资源开销（~39MB vs 30MB）

相比v3.1多进程：

- ✅ 更实时（R5F硬实时<1.32ms vs Linux软实时<1.24ms）

- ✅ 确定性保证（FreeRTOS调度±20μs vs Linux调度±50μs）

- ✅ 故障隔离更强（R5F独立运行 vs 进程隔离）

- ✅ 独立Logger进程（vs 手动实现）

- ⚖️ 需要异构SoC（A53+R5F vs 纯A53）

### 4.3 适用场景

PMC方案最适合：

- ✅ 航天级应用（卫星、深空探测）

- ✅ 硬实时要求（<2ms响应）

- ✅ 多核异构SoC平台（A53+R5F）

- ✅ 长期无人值守

- ✅ 极高可靠性要求

不适合场景：

- ❌ 地面测试系统（可用v3.0单进程）

- ❌ 资源极度受限（<128MB内存）

- ❌ 无实时性要求（可用v3.0单进程）

- ❌ 纯A53单核平台（可用v3.1多进程）

---

## 5. 启动流程

### 5.1 系统启动序列

1. Bootloader启动
 ├── 检查启动标志
    ├── 选择启动分区（A或B）
    └── 加载Linux内核

2. Linux内核启动
 ├── 初始化硬件
    ├── 挂载根文件系统
    └── 启动init进程

3. Init进程启动PMC
 ├── 加载配置文件（/etc/pmc/pmc.conf）
    ├── 创建共享内存（/dev/shm/pmc_shm）
    ├── 启动Supervisor进程
    └── Supervisor启动子进程

4. Supervisor启动子进程
 ├── 启动Telecommand进程（最高优先级）
    ├── 启动Telemetry进程
    ├── 启动Logger进程
    ├── 启动Firmware进程
    └── 开始心跳监控

5. 各进程初始化
 ├── Telecommand进程：
    │   ├── 加载ACL配置
    │   ├── 初始化PDL服务（Satellite/BMC/MCU）
    │   ├── 设置实时调度（SCHED_FIFO, priority=99）
    │   ├── 绑定CPU0
    │   ├── 锁定内存
    │   └── 启动CAN接收线程
    │
    ├── Telemetry进程：
    │   ├── 加载ACL配置
    │   ├── 初始化PDL服务
    │   ├── 映射共享内存（遥测缓冲区）
    │   └── 启动采集线程
    │
    ├── Logger进程：
    │   ├── 映射共享内存（日志环形缓冲区）
    │   ├── 创建日志目录
    │   └── 启动日志收集线程
    │
    └── Firmware进程：
	 ├── 检查启动分区
	 ├── 标记启动成功
	 └── 进入空闲状态

6. 系统就绪
 ├── 向卫星平台发送心跳
    ├── 开始接收遥控遥测命令
    └── 后台采集遥测数据

### 5.2 启动脚本

``` shell
#!/bin/bash

# /etc/init.d/pmc

# PMC启动脚本

PMC_CONF="/etc/pmc/pmc.conf"
PMC_SUPERVISOR="/usr/bin/pmc_supervisor"
PMC_PID_FILE="/var/run/pmc_supervisor.pid"
PMC_LOG_DIR="/var/log/pmc"

start() {
  echo "Starting PMC."

  # 1. 检查配置文件

  if [ ! -f "$PMC_CONF" ]; then
	  echo "Error: Config file not found: $PMC_CONF"
	  exit 1
  fi

  # 2. 创建日志目录

  mkdir -p "$PMC_LOG_DIR"
  mkdir -p "$PMC_LOG_DIR/archive"
  mkdir -p "$PMC_LOG_DIR/crash"

  # 3. 清理旧的共享内存

  rm -f /dev/shm/pmc_*

  # 4. 设置CAN接口

  ip link set can0 type can bitrate 500000
  ip link set can0 up

  # 5. 启动Supervisor进程

  $PMC_SUPERVISOR --config $PMC_CONF --daemon

  # 6. 等待启动完成

  sleep 2

  # 7. 检查进程状态

  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  if ps -p $PID > /dev/null; then
		  echo "PMC started successfully (PID: $PID)"
	  else
		  echo "Error: PMC failed to start"
		  exit 1
	  fi
  else
	  echo "Error: PID file not found"
	  exit 1
  fi
}

stop() {
  echo "Stopping PMC."

  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  kill -TERM $PID

	  # 等待进程退出

	  for i in {110}; do
		  if ! ps -p $PID > /dev/null; then
			  echo "PMC stopped"
			  rm -f $PMC_PID_FILE
			  return 0
		  fi
		  sleep 1
	  done

	  # 强制杀死

	  kill -KILL $PID
	  rm -f $PMC_PID_FILE
	  echo "PMC force stopped"
  else
	  echo "PMC is not running"
  fi
}

status() {
  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  if ps -p $PID > /dev/null; then
		  echo "PMC is running (PID: $PID)"

		  # 显示子进程状态

		  echo ""
		  echo "Child processes:"
		  ps -eo pid,comm,stat,pri,nice | grep -E "pmc_|PID"
	  else
		  echo "PMC is not running (stale PID file)"
	  fi
  else
	  echo "PMC is not running"
  fi
}

case "$1" in
  start)
	  start
	  ;;
  stop)
	  stop
	  ;;
  restart)
	  stop
	  start
	  ;;
  status)
	  status
	  ;;
  *)
	  echo "Usage: $0 {start|stop|restart|status}"
	  exit 1
	  ;;
esac

exit 0
```

---

## 6. 调试与故障排查

### 6.1 调试工具

实时性分析：
```shell
# 1. 查看进程调度策略

chrt -p <pid>

# 2. 查看CPU亲和性

taskset -p <pid>

# 3. 测量延迟（cyclictest）

cyclictest -p 99 -t 1 -n -i 1000 -l 100000

# 4. 查看中断延迟

cat /proc/interrupts

# 5. 跟踪系统调用（strace）

strace -p <pid> -T -tt
```

性能分析：
``` shell
# 1. CPU占用（top）

top -H -p <pid>

# 2. 内存占用（pmap）

pmap -x <pid>

# 3. 系统调用统计（strace）

strace -c -p <pid>

# 4. 性能分析（perf）

perf record -p <pid> -g
perf report

# 5. 火焰图

perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

日志分析：

# 1. 查看实时日志

tail -f /var/log/pmc/telecommand.log

# 2. 搜索错误日志

grep ERROR /var/log/pmc/*.log

# 3. 统计错误次数

grep ERROR /var/log/pmc/*.log | wc -l

# 4. 查看崩溃日志

ls -lh /var/log/pmc/crash/

# 5. 分析状态日志（JSON）

jq '.server.power_state' /var/log/pmc/status.log
```

### 6.2 常见问题排查

问题1：2ms应答超时
症状：遥控遥测命令应答时间>2ms

排查步骤：

1. 检查Telecommand进程调度策略
``` shell
 chrt -p $(pgrep pmc_telecommand)
 # 预期：SCHED_FIFO, priority=99
```

2. 检查CPU亲和性
``` shell
 taskset -p $(pgrep pmc_telecommand)
 # 预期：affinity mask: 1（绑定CPU0）
```

3. 检查内存锁定
``` shell
 cat /proc/$(pgrep pmc_telecommand)/status | grep VmLck
 # 预期：VmLck > 0
```

4. 检查中断延迟
``` shell
 cyclictest -p 99 -t 1 -n -i 1000 -l 10000
 # 预期：Max latency < 100μs
```

5. 检查CAN总线状态
``` shell
 ip -s link show can0
 # 检查错误计数
```

解决方案：

- 使用PREEMPT_RT内核

- 禁用不必要的中断

- 优化PDL层代码

问题2：进程频繁崩溃
症状：Telemetry进程每隔几分钟崩溃一次

排查步骤：

1. 查看崩溃日志
``` shell
 cat /var/log/pmc/crash/crash_*.log
 # 查看崩溃原因（SIGSEGV/SIGABRT）
```

2. 分析coredump
``` shell
 gdb /usr/bin/pmc_telemetry /var/log/pmc/crash/core.1234
    (gdb) bt
 # 查看调用栈
```

3. 检查内存泄漏
``` shell
 valgrind --leak-check=full /usr/bin/pmc_telemetry
```

4. 检查共享内存
``` shell
 ls -lh /dev/shm/pmc_*
 # 检查共享内存是否损坏
```

解决方案：

- 修复段错误（NULL指针、数组越界）

- 修复内存泄漏

- 增加错误处理

问题3：日志占满Flash
症状：Flash空间不足，系统无法写入日志

排查步骤：

1. 检查Flash使用情况
``` shell
 df -h /var/log/pmc
 # 查看剩余空间
```

2. 检查日志文件大小
``` shell
 du -sh /var/log/pmc/*
 # 查找大文件
```

3. 检查日志轮转
``` shell
 ls -lh /var/log/pmc/archive/
 # 检查是否正常轮转
```

解决方案：

- 手动清理旧日志
``` shell
rm -f /var/log/pmc/archive/*.log.gz
```

- 调整日志轮转策略
``` shell

# 修改/etc/pmc/pmc.conf

max_log_size_mb = 5
max_archive_days = 3

- 重启Logger进程
kill -HUP $(pgrep pmc_logger)
```

---



---

## 7. 附录

### 7.1 术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| PMC | Payload Management Controller | 载荷管理控制器 |
| BMC | Baseboard Management Controller | 基板管理控制器 |
| MCU | Microcontroller Unit | 微控制器 |
| FPGA | Field-Programmable Gate Array | 现场可编程门阵列 |
| Telecommand | Telecommand | 遥控，卫星平台向载荷发送的控制命令 |
| Telemetry | Telemetry | 遥测，载荷向卫星平台上报的状态数据 |
| ACL | Application Configuration Layer | 应用配置层 |
| PDL | Peripheral Driver Layer | 外设驱动层 |
| PCL | Peripheral Configuration Layer | 外设配置层 |
| HAL | Hardware Abstraction Layer | 硬件抽象层 |
| OSAL | Operating System Abstraction Layer | 操作系统抽象层 |
| SEU | Single Event Upset | 单粒子翻转 |
| SEL | Single Event Latchup | 单粒子闩锁 |
| IPC | Inter-Process Communication | 进程间通信 |
| SCHED_FIFO | First-In-First-Out Scheduling | 先进先出调度 |
| PREEMPT_RT | Real-Time Preemption | 实时抢占 |

### 7.2 参考文档

1. EMS架构文档

- docs/ARCHITECTURE.md

- docs/CODING_STANDARDS.md


2. Refactor方案

- docs/refactor/EMS_Architecture_Refactor_v3.0_SingleProcess.md

- docs/refactor/EMS_Architecture_Refactor_v3.1_MultiProcess_Final.md

3. Linux实时编程

- Linux PREEMPT_RT Documentation

- POSIX Real-Time Extensions

- Shared Memory Programming Guide

4. 航天软件标准

- NASA Software Safety Guidebook

- ESA Software Engineering Standards

- DO-178C (航空软件标准)

### 7.3 联系方式

技术支持：

- 邮箱：guohaoprc@163.com

- 文档：https://github.com/wanguo99/EMS/tree/master/docs

- 问题跟踪：https://github.com/wanguo99/EMS/issues

---
验证清单

在实施完成后，请确认以下检查项：

功能验证

- 遥控命令2ms内应答（100%成功率）

- 快遥1秒周期正常工作

- 慢遥2秒周期正常工作

- 快遥慢遥可同时进行

- 缓存型遥测返回正确数据

- 实时型遥测返回最新数据

- 固件升级成功（主备分区切换）

- 日志正常收集和轮转

性能验证

- 遥控命令延迟<500μs（平均）

- 缓存型遥测延迟<200μs（平均）

- 实时型遥测延迟<800μs（平均）

- CPU占用<60%（正常负载）

- 内存占用<50MB

- Flash占用<200MB

可靠性验证

- Telemetry进程崩溃后1秒内恢复

- Telecommand进程崩溃后1秒内恢复

- 连续崩溃5次触发系统复位

- 硬件看门狗10秒超时触发复位

- SEU模拟测试通过

- 72小时稳定性测试通过

文档验证

- 代码注释完整

- API文档完整

- 用户手册完整

- 测试报告完整

- 部署文档完整

---
方案版本：v1.0
最后更新：2026-05-17
作者：wanguo
