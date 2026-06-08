# EMS 核心模块对比

本文档提供 EMS 各核心模块的快速对比参考。

## 模块概览

| 模块 | 全称 | 层次 | 职责 | 内存占用 |
|------|------|------|------|----------|
| OSAL | Operating System Abstraction Layer | 最底层 | 操作系统抽象 | 5-50KB |
| HAL | Hardware Abstraction Layer | 硬件层 | 硬件驱动封装 | 5-50KB |
| PConfig | Platform Configuration | 配置层 | 硬件配置管理 | 10-20KB |
| PRL | Protocol Layer | 协议层 | 统一设备协议 | 15-30KB |
| PDL | Peripheral Driver Layer | 驱动层 | 外设驱动管理 | 8-40KB |
| AConfig | Application Configuration | 配置层 | 业务配置映射 | 20-30KB |

## 依赖关系

```
应用层 (Products)
    ↓ 依赖
应用配置层 (AConfig)
    ↓ 依赖
外设驱动层 (PDL) ←─── 查询配置 ←─── 平台配置层 (PConfig)
    ↓ 依赖                ↓ 使用
协议层 (PRL)           (独立模块)
    ↓ 依赖
硬件抽象层 (HAL)
    ↓ 依赖
操作系统抽象层 (OSAL)
```

**关键点**：
- PConfig 和 PRL 是独立模块
- PDL 查询 PConfig 获取硬件配置
- PDL 使用 PRL 进行协议编解码
- PDL 通过 HAL 进行数据传输（不经过 PRL）

## 模块详细对比

### OSAL vs HAL

| 特性 | OSAL | HAL |
|------|------|-----|
| 抽象对象 | 操作系统 API | 硬件驱动 |
| 平台相关性 | 操作系统相关 (Linux/RTOS) | 硬件平台相关 |
| 实现位置 | `src/posix/`, `src/freertos/` | `src/linux/`, `src/rtems/` |
| 调用层次 | 被所有模块调用 | 仅被 PDL 调用 |
| 示例 API | `OSAL_TaskCreate()`, `OSAL_MutexLock()` | `HAL_CAN_Send()`, `HAL_UART_Open()` |
| 是否包含硬件操作 | 否 | 是（唯一允许直接操作硬件的层） |

### PConfig vs AConfig

| 特性 | PConfig | AConfig |
|------|---------|---------|
| 配置对象 | 硬件设备 | 业务功能 |
| 配置格式 | 设备树风格 | 枚举映射表 |
| 配置内容 | MCU/BMC/Satellite 硬件接口 | TC/TM 业务功能映射 |
| 查询性能 | O(n) 遍历查找 | O(1) 数组索引 |
| 使用者 | PDL 层 | 应用层 |
| 示例 | `PCONFIG_GetMCUConfig("mcu0")` | `ACONFIG_GetTcConfig(TC_POWER_ON)` |
| 配置来源 | 平台配置文件 | 产品配置文件 |

### PRL vs PDL

| 特性 | PRL | PDL |
|------|-----|-----|
| 职责 | 协议编解码 | 外设驱动管理 |
| 调用者 | 仅被 PDL 调用 | 被 AConfig/应用层调用 |
| 传输数据 | 否（仅处理协议） | 是（通过 HAL 传输） |
| 协议格式 | 统一的 20 字节协议头 | 无（调用 PRL） |
| 平台相关性 | 完全平台无关 | 完全平台无关 |
| 示例 API | `PRL_Encode()`, `PRL_Decode()` | `PDL_MCU_GetVersion()`, `PDL_BMC_PowerOn()` |

## 设计模式对比

### 抽象层模式

**OSAL** 和 **HAL** 使用经典的抽象层模式：

```
统一接口层 (include/)
    ↓
平台实现层 (src/linux/, src/posix/)
    ↓
操作系统/硬件
```

### 配置驱动模式

**PConfig** 和 **AConfig** 使用配置驱动模式：

```
运行时查询接口
    ↓
配置数据表 (静态数据)
    ↓
配置源文件 (C 文件)
```

### 协议处理模式

**PRL** 使用协议处理器模式：

```
编码器 (Encoder)  ←→  协议格式定义  ←→  解码器 (Decoder)
                         ↓
                   零拷贝设计
```

### 驱动管理模式

**PDL** 使用驱动管理器模式：

```
外设管理接口
    ↓
协议处理 (PRL) + 配置查询 (PConfig) + 传输层 (HAL)
    ↓
外设设备
```

## 关键特性对比

### 线程安全

| 模块 | 线程安全性 | 实现方式 |
|------|-----------|----------|
| OSAL | 完全线程安全 | 互斥锁保护 |
| HAL | 部分线程安全 | 句柄隔离 |
| PConfig | 完全线程安全 | 只读数据 |
| PRL | 完全线程安全 | 无状态设计 |
| PDL | 部分线程安全 | 句柄隔离 |
| AConfig | 完全线程安全 | 只读数据（初始化后） |

### 性能特性

| 模块 | 关键操作 | 性能指标 |
|------|----------|----------|
| OSAL | 任务切换 | < 1ms |
| OSAL | 队列操作 | < 100μs |
| OSAL | 互斥锁 | < 50μs |
| HAL | CAN 发送 | < 500μs |
| HAL | UART 传输 | 取决于波特率 |
| PConfig | 设备查询 | < 1μs (缓存) |
| PRL | 编解码 | < 10μs |
| PDL | MCU 通信 | < 5ms (含协议) |
| AConfig | 配置查询 | ~5ns (O(1)) |

### 内存特性

| 模块 | 静态内存 | 动态内存 | 栈使用 |
|------|----------|----------|--------|
| OSAL | 中等 (全局表) | 按需分配 | 中等 |
| HAL | 小 | 按需分配 | 小 |
| PConfig | 大 (配置数据) | 无 | 极小 |
| PRL | 小 (查表) | 无 | 中等 (协议缓冲) |
| PDL | 中等 | 按需分配 | 中等 |
| AConfig | 大 (查找表) | 无 | 极小 |

## 使用场景

### 何时直接使用 OSAL？

- 需要创建线程/任务
- 需要进程间通信（队列、信号量）
- 需要文件操作或网络 Socket
- 需要跨平台的系统调用

**示例**：
```c
osal_id_t task_id;
OSAL_TaskCreate(&task_id, "Worker", worker_func, NULL, 8192, 100, 0);
```

### 何时直接使用 HAL？

通常**不直接使用**，应该通过 PDL 调用。

特殊情况：
- PDL 不支持的特殊硬件
- 性能关键路径（跳过 PDL 层）

### 何时使用 PConfig？

**仅在 PDL 层使用**，用于查询硬件配置：

```c
const pconfig_mcu_t *cfg = PCONFIG_GetMCUConfig("mcu0");
// 根据 cfg 初始化 HAL
```

### 何时使用 PRL？

**仅在 PDL 层使用**，用于协议编解码：

```c
// PDL 内部
uint8_t buf[PRL_MAX_PACKET_SIZE];
int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                     payload, payload_len, buf, sizeof(buf), 0);
HAL_CAN_Send(handle, buf, len);  // 通过 HAL 发送
```

### 何时使用 PDL？

**应用层或 AConfig 层使用**，用于设备操作：

```c
// 应用层
pdl_mcu_version_t version;
PDL_MCU_GetVersion(handle, &version);
```

### 何时使用 AConfig？

**应用层使用**，用于查询业务配置：

```c
// 应用层
const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(TC_POWER_ON);
if (cfg && cfg->enabled) {
    // 调用 PDL
    PDL_BMC_PowerOn(cfg->logic_index);
}
```

## 常见误区

### ❌ 错误做法

1. **在应用层直接调用 HAL**
   ```c
   // ❌ 错误：跳过了 PDL 层
   HAL_CAN_Send(handle, data, len);
   ```

2. **在 PDL 中直接调用 OSAL 系统调用**
   ```c
   // ❌ 错误：应该通过 HAL 层
   int fd = OSAL_open("/dev/can0", O_RDWR);
   ```

3. **数据通过 PRL 传输**
   ```c
   // ❌ 错误：PRL 不负责传输
   PRL_Send(data, len);  // PRL 没有 Send 函数！
   ```

4. **在 Core 模块中依赖 Products**
   ```c
   // ❌ 错误：Core 不能依赖 Products
   #include "products/ccm/ccm.h"
   ```

### ✅ 正确做法

1. **应用层通过 PDL 操作设备**
   ```c
   // ✅ 正确
   PDL_MCU_SendCommand(handle, cmd_id, data, len);
   ```

2. **PDL 通过 HAL 访问硬件**
   ```c
   // ✅ 正确
   HAL_CAN_Init(&cfg, &hal_handle);
   HAL_CAN_Send(hal_handle, data, len);
   ```

3. **PDL 使用 PRL 编解码，使用 HAL 传输**
   ```c
   // ✅ 正确
   PRL_Encode(..., buf, ...);  // 编码
   HAL_CAN_Send(handle, buf, len);  // 传输
   ```

4. **业务代码通过 AConfig 查询配置**
   ```c
   // ✅ 正确
   const aconfig_tc_config_t *cfg = ACONFIG_GetTcConfig(cmd);
   PDL_XXX_DoSomething(cfg->logic_index, ...);
   ```

## 扩展指南

### 添加新的 OSAL 接口

1. 在 `osal/include/` 添加接口声明
2. 在 `osal/src/posix/` 添加 POSIX 实现
3. 在 `osal/src/freertos/` 添加 FreeRTOS 实现（可选）
4. 更新 `osal/API.md` 文档

### 添加新的 HAL 驱动

1. 在 `hal/include/` 添加驱动接口（如 `hal_gpio.h`）
2. 在 `hal/src/linux/` 添加 Linux 实现
3. 在 PConfig 添加硬件配置类型
4. 更新 Kconfig 添加配置选项

### 添加新的设备协议

1. 在 `prl/include/` 添加设备类型和消息定义（如 `prl_newdev.h`）
2. 在 `prl/src/` 实现编解码函数
3. 在 PDL 中实现对应的设备驱动
4. 更新 Kconfig 添加协议选项

### 添加新的外设驱动

1. 在 `pdl/include/` 添加外设接口（如 `pdl_sensor.h`）
2. 在 `pdl/src/` 实现驱动逻辑
3. 使用 PConfig 查询硬件配置
4. 使用 PRL 进行协议编解码
5. 使用 HAL 进行数据传输

---

**最后更新**：2026-06-08  
**维护者**：wanguo
