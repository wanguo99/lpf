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

#### 2.2.4 遥控命令处理流程

**纯A53架构下的遥控处理**：

```text
卫星平台 ──CAN命令──> Telecommand进程（CPU0，实时）
                          │
                          ├─ 1. CAN接收（<20μs）
                          │    • 从CAN总线接收命令帧
                          │    • 解析命令ID和参数
                          │
                          ├─ 2. ACL配置查询（<5μs）
                          │    • 查询遥控命令配置（O(1)数组索引）
                          │    • 获取目标设备类型和逻辑索引
                          │
                          ├─ 3. PDL设备操作（<1ms）
                          │    • 根据设备类型调用PDL接口
                          │    • BMC: Redfish/IPMI命令
                          │    • MCU: CAN命令
                          │    • FPGA: 寄存器操作
                          │
                          ├─ 4. 失效相关遥测（<10μs）
                          │    • 查询失效映射表
                          │    • 标记受影响的遥测为STALE
                          │
                          └─ 5. CAN应答（<20μs）
                               • 构造应答帧
                               • 发送到卫星平台
                               
总延迟：<1.5ms（满足2ms要求）
```

**遥控命令分类**：

| 命令类型 | 处理方式 | 延迟 | 示例 |
|---------|---------|------|------|
| **快速遥控** | Telecommand进程直接处理 | <1ms | GPIO控制、MCU CAN命令 |
| **慢速遥控** | Telecommand进程调用PDL | <100ms | BMC Redfish命令、固件升级 |

**关键代码示例**：

```c
// Telecommand进程：遥控命令处理
int32_t handle_telecommand(pmc_tc_function_t cmd_type, uint32_t param) {
    // 1. 查询ACL配置（O(1)）
    const acl_tc_config_t *cfg = ACL_GetTcConfig(cmd_type);
    if (!cfg || !cfg->enabled) {
        return OSAL_ERR_INVALID_PARAM;
    }
    
    // 2. 根据设备类型调用PDL
    int32_t ret = OSAL_ERR_NOT_IMPLEMENTED;
    
    switch (cfg->device_type) {
        case ACL_DEVICE_BMC:
            // 调用BMC PDL接口
            ret = PDL_BMC_ExecuteCommand(cfg->logic_index, cmd_type, param);
            break;
            
        case ACL_DEVICE_MCU:
            // 调用MCU PDL接口
            ret = PDL_MCU_ExecuteCommand(cfg->logic_index, cmd_type, param);
            break;
            
        case ACL_DEVICE_FPGA:
            // 调用FPGA PDL接口
            ret = PDL_FPGA_ExecuteCommand(cfg->logic_index, cmd_type, param);
            break;
            
        default:
            return OSAL_ERR_INVALID_PARAM;
    }
    
    // 3. 失效相关遥测
    if (ret == OSAL_SUCCESS) {
        ACL_InvalidateAffectedTelemetry(cmd_type);
    }
    
    return ret;
}
```

**实时性保证**：

1. **实时调度**：SCHED_FIFO优先级99，抢占所有非实时任务
2. **CPU绑定**：独占CPU0，避免进程迁移开销
3. **内存锁定**：mlockall防止页错误
4. **确定性路径**：所有操作都是O(1)或有界延迟

---

#### 2.2.5 遥测数据处理流程

**纯A53架构下的遥测处理（统一缓存模型）**：

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    Telemetry进程（CPU1，后台）                       │
│                                                                      │
│  周期性采集循环（100ms周期）：                                        │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ for each 遥测配置项:                                        │    │
│  │   1. 检查更新周期（是否需要采集）                            │    │
│  │   2. 调用PDL接口采集数据                                    │    │
│  │      • BMC: Redfish/IPMI查询                               │    │
│  │      • MCU: CAN查询                                        │    │
│  │      • FPGA: 寄存器读取                                    │    │
│  │   3. 写入共享内存缓存                                       │    │
│  │      • 更新数据                                            │    │
│  │      • 更新时间戳                                          │    │
│  │      • 计算新鲜度（FRESH/STALE/INVALID）                   │    │
│  │   4. 异常检测                                              │    │
│  │      • 温度超限                                            │    │
│  │      • 电压异常                                            │    │
│  │      • 设备离线                                            │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                      │
└──────────────────────────┬───────────────────────────────────────────┘
                           │ 写入
                           ↓
              ┌────────────────────────────┐
              │   POSIX共享内存（4MB）      │
              │  • 遥测缓存条目数组         │
              │  • pthread_rwlock保护      │
              │  • 时间戳 + 新鲜度标志      │
              └────────────────────────────┘
                           │ 读取（<50μs）
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                  Telecommand进程（CPU0，实时）                        │
│                                                                      │
│  遥测请求处理：                                                       │
│  1. 从共享内存读取缓存数据（<50μs）                                   │
│  2. 检查新鲜度标志                                                   │
│     • FRESH: 数据新鲜，直接返回                                      │
│     • STALE: 数据过期但可用，返回并标记                               │
│     • INVALID: 数据无效，返回错误码                                  │
│  3. 构造遥测应答帧                                                   │
│  4. CAN发送到卫星平台（<20μs）                                       │
│                                                                      │
│  总延迟：<100μs（远小于2ms要求）                                      │
└─────────────────────────────────────────────────────────────────────┘
```

**遥测数据新鲜度计算**：

```c
// 新鲜度计算逻辑
tm_freshness_t calculate_freshness(uint64_t timestamp_us, uint32_t validity_ms) {
    uint64_t now_us = OSAL_GetTimeUs();
    uint64_t age_ms = (now_us - timestamp_us) / 1000;
    
    if (age_ms > validity_ms * 2) {
        return TM_STATUS_INVALID;  // 超过2倍有效期，数据无效
    } else if (age_ms > validity_ms) {
        return TM_STATUS_STALE;    // 超过有效期，数据过期但可用
    } else {
        return TM_STATUS_FRESH;    // 在有效期内，数据新鲜
    }
}
```

**遥测缓存访问示例**：

```c
// Telemetry进程：写入缓存
int32_t update_telemetry_cache(pmc_tm_function_t tm_id, const uint8_t *data, uint32_t size) {
    // 1. 采集数据（调用PDL）
    const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_id);
    uint8_t tm_data[256];
    uint32_t tm_size = 0;
    
    int32_t ret = collect_telemetry_from_device(cfg, tm_data, &tm_size);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    // 2. 写入共享内存缓存
    return ACL_TelemetryCache_Write(tm_id, tm_data, tm_size);
}

// Telecommand进程：读取缓存（<50μs）
int32_t handle_telemetry_request(pmc_tm_function_t tm_id, telemetry_response_t *response) {
    // 从缓存读取（快速，无阻塞）
    int32_t ret = ACL_TelemetryCache_Read(tm_id, response);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    // 检查新鲜度
    if (response->freshness == TM_STATUS_INVALID) {
        LOG_WARN("TC", "遥测数据无效: %d", tm_id);
        return OSAL_ERR_TIMEOUT;
    }
    
    if (response->freshness == TM_STATUS_STALE) {
        LOG_DEBUG("TC", "遥测数据过期: %d, age=%ums", tm_id, response->age_ms);
        // 仍然返回数据，但卫星平台知道数据已过期
    }
    
    return OSAL_SUCCESS;
}
```

**遥测周期配置**：

| 遥测类型 | 更新周期 | 有效期 | 新鲜度判断 | 示例 |
|---------|---------|-------|-----------|------|
| **快遥** | 100ms | 500ms | >500ms=STALE, >1s=INVALID | 电源状态、MCU状态 |
| **慢遥** | 2s | 4s | >4s=STALE, >8s=INVALID | 温度、电压、风扇转速 |

---

#### 2.2.6 故障隔离与自恢复

**进程隔离设计**：

```text
┌─────────────────────────────────────────────────────────────────────┐
│                         Supervisor进程（CPU2）                        │
│                                                                      │
│  职责：                                                               │
│  • 监控所有进程健康状态（心跳检测）                                    │
│  • 检测进程崩溃（通过信号或心跳超时）                                  │
│  • 自动重启崩溃的进程                                                 │
│  • 记录故障日志                                                       │
│  • 系统级故障恢复（重启整个系统）                                      │
│                                                                      │
└──────────────────────────┬───────────────────────────────────────────┘
                           │ 监控
                           ↓
        ┌──────────────────────────────────────────┐
        │                                          │
        ↓                                          ↓
┌──────────────────┐                    ┌──────────────────┐
│ Telecommand进程  │                    │ Telemetry进程    │
│ （CPU0，实时）   │                    │ （CPU1，后台）   │
│                  │                    │                  │
│ 故障影响：       │                    │ 故障影响：       │
│ • 无法处理遥控   │                    │ • 缓存不更新     │
│ • 无法应答遥测   │                    │ • 数据变STALE    │
│                  │                    │                  │
│ 恢复策略：       │                    │ 恢复策略：       │
│ • Supervisor重启 │                    │ • Supervisor重启 │
│ • <1秒恢复服务   │                    │ • <5秒恢复采集   │
└──────────────────┘                    └──────────────────┘
```

**故障检测机制**：

1. **心跳检测**：
   - 每个进程定期更新心跳时间戳（共享内存）
   - Supervisor进程检测心跳超时（2秒）
   - 超时则判定进程卡死，执行重启

2. **信号捕获**：
   - 捕获SIGSEGV、SIGABRT等崩溃信号
   - 记录崩溃日志（堆栈、寄存器）
   - 通知Supervisor进程

3. **看门狗**：
   - 硬件看门狗（5秒超时）
   - Supervisor进程定期喂狗
   - 系统级死锁保护

**自恢复策略**：

| 故障类型 | 检测方式 | 恢复策略 | 恢复时间 |
|---------|---------|---------|---------|
| **进程崩溃** | 信号捕获 | Supervisor重启进程 | <1秒 |
| **进程卡死** | 心跳超时 | Supervisor杀死并重启 | <2秒 |
| **Supervisor崩溃** | 硬件看门狗 | 系统复位 | <5秒 |
| **系统死锁** | 硬件看门狗 | 系统复位 | <5秒 |

**恢复代码示例**：

```c
// Supervisor进程：监控和自动重启
void supervisor_monitor_loop(void) {
    while (running) {
        // 1. 检查Telecommand进程心跳
        if (check_process_heartbeat("telecommand", 2000) == TIMEOUT) {
            LOG_ERROR("SUPERVISOR", "Telecommand进程心跳超时，重启中...");
            restart_process("telecommand");
        }
        
        // 2. 检查Telemetry进程心跳
        if (check_process_heartbeat("telemetry", 2000) == TIMEOUT) {
            LOG_ERROR("SUPERVISOR", "Telemetry进程心跳超时，重启中...");
            restart_process("telemetry");
        }
        
        // 3. 喂硬件看门狗
        HAL_Watchdog_Feed();
        
        // 4. 休眠500ms
        OSAL_msleep(500);
    }
}

// 进程重启函数
int32_t restart_process(const char *process_name) {
    // 1. 杀死旧进程
    pid_t pid = get_process_pid(process_name);
    if (pid > 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    
    // 2. 启动新进程
    pid = fork();
    if (pid == 0) {
        // 子进程：执行目标程序
        execl(process_name, process_name, NULL);
        exit(1);
    }
    
    // 3. 记录重启日志
    LOG_INFO("SUPERVISOR", "进程 %s 已重启，PID=%d", process_name, pid);
    
    return OSAL_SUCCESS;
}
```

---

#### 2.2.7 资源分配

**CPU分配**：

| CPU核心 | 进程 | 调度策略 | 优先级 | 职责 |
|--------|------|---------|-------|------|
| **CPU0** | Telecommand | SCHED_FIFO | 99 | 实时遥控处理，<1.5ms响应 |
| **CPU1** | Telemetry | SCHED_OTHER | 0 | 后台遥测采集，100ms周期 |
| **CPU2** | Supervisor | SCHED_OTHER | 0 | 进程监控和自动重启 |
| **CPU3** | Logger | SCHED_OTHER | 0 | 日志收集和持久化 |

**内存分配**：

| 内存区域 | 大小 | 用途 | 访问方式 |
|---------|------|------|---------|
| **共享内存** | 4MB | 遥测缓存 | POSIX shm_open + mmap |
| **进程堆栈** | 8MB/进程 | 进程私有内存 | malloc |
| **代码段** | ~2MB | 可执行文件 | 只读映射 |

**实时性配置**：

```bash
# 1. CPU隔离（可选）
# 在内核启动参数中添加：
isolcpus=0,1

# 2. 中断亲和性（可选）
# 将CAN中断绑定到CPU0：
echo 1 > /proc/irq/<CAN_IRQ>/smp_affinity

# 3. 实时调度配置
# 在应用代码中设置：
struct sched_param param = { .sched_priority = 99 };
sched_setscheduler(0, SCHED_FIFO, &param);

# 4. CPU亲和性
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);

# 5. 内存锁定
mlockall(MCL_CURRENT | MCL_FUTURE);
```

---

#### 2.2.8 ACL配置加载机制

**配置加载流程**：

```text
系统启动
    ↓
ACL_Init()
    ↓
1. 加载遥控配置表（g_pmc_tc_configs）
   • 编译时静态链接
   • O(1)数组索引访问
    ↓
2. 加载遥测配置表（g_pmc_tm_configs）
   • 编译时静态链接
   • O(1)数组索引访问
    ↓
3. 加载失效映射表（g_invalidation_map）
   • 编译时静态链接
   • 遥控命令 → 失效遥测列表
    ↓
4. 初始化遥测缓存
   • 创建POSIX共享内存
   • 初始化pthread_rwlock
   • 所有条目标记为INVALID
    ↓
配置加载完成
```

**配置选择机制**：

```cmake
# CMakeLists.txt中选择配置
set(ACL_CONFIG "H200_100P" CACHE STRING "ACL configuration")

if(ACL_CONFIG STREQUAL "H200_100P")
    list(APPEND ACL_SOURCES
        config/H200_100P/acl_h200_100p.c
        config/H200_100P/acl_h200_100p_invalidation.c
    )
elseif(ACL_CONFIG STREQUAL "demo")
    list(APPEND ACL_SOURCES
        config/demo/acl_demo_v1.c
        config/demo/acl_demo_v1_invalidation.c
    )
endif()
```

**配置示例**：

```c
// acl_h200_100p.c
const acl_tc_config_t g_pmc_tc_configs[] = {
    // 服务器电源控制 → BMC
    { TC_SERVER_POWER_ON,    ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_OFF,   ACL_DEVICE_BMC, 0, true },
    
    // MCU控制 → MCU[0]
    { TC_MCU_RESET,          ACL_DEVICE_MCU, 0, true },
    
    // ... 更多配置
};

const acl_tm_config_t g_pmc_tm_configs[] = {
    // 服务器遥测 → BMC，1秒更新，2秒有效期
    { TM_SERVER_CPU_TEMP,    ACL_DEVICE_BMC, 0, 2000, 1000, true },
    { TM_SERVER_BOARD_TEMP,  ACL_DEVICE_BMC, 0, 2000, 1000, true },
    
    // MCU遥测 → MCU，2秒更新，4秒有效期
    { TM_MCU_STATUS,         ACL_DEVICE_MCU, 0, 4000, 2000, true },
    
    // ... 更多配置
};
```

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
  - TI Linux平台实现（系统调用封装）
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
  - 启动流程（纯A53多进程启动）
  - 开发阶段划分
  - 里程碑与交付物
  - 风险评估与应对
  - 详细设计：[3-2-implementation_plan.md](./3-2-implementation_plan.md)

---

## 4. 总结

### 4.1 方案核心特点

1. ✅ **满足2ms硬实时**


- **FreeRTOS调度**：确定性实时响应，抖动±20μs

- **TCM零等待**：关键代码和数据在TCM，零等待访问

- **实测延迟**：<1.32ms（远小于2ms）

2. ✅ **航空级可靠性**


- **进程级故障隔离**（A53侧MMU硬件保护）


- **三级故障恢复**（任务级 → 核心级 → 系统级）


3. ✅ **多核异构优化**

- **A53侧**：4核负载均衡，处理复杂业务（Redfish/IPMI/固件升级/日志）

- **Telecommand进程：实时遥控处理，SCHED_FIFO保证确定性

- **核间通信**：RPMsg消息传递（<100μs）+ 共享内存（<10μs）+ 硬件Mailbox（<5μs）

- **负载分离：Telecommand进程CPU0独占，Telemetry进程CPU1独占

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


- ✅ 符合航天标准（NASA/ESA要求硬件级隔离）

- ⚖️ 略高资源开销（~39MB vs 30MB）

相比v3.1多进程：


- ✅ 确定性保证（FreeRTOS调度±20μs vs Linux调度±50μs）


- ✅ 独立Logger进程（vs 手动实现）


### 4.3 适用场景

PMC方案最适合：

- ✅ 航天级应用（卫星、深空探测）

- ✅ 硬实时要求（<2ms响应）


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
