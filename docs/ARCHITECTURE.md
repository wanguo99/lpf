# EMS 架构概述

本文档详细介绍 EMS SDK 的系统架构、模块设计和依赖关系。

---

## 📊 系统架构

EMS 采用分层架构设计，从下到上分为 5 层：

```mermaid
graph TB
    subgraph "应用层"
        Apps[Applications<br/>应用程序]
    end
    
    subgraph "应用控制层"
        ACL[ACL<br/>Application Control Layer<br/>遥测遥控/配置管理]
    end
    
    subgraph "协议数据层"
        PDL[PDL<br/>Protocol Data Layer<br/>卫星/BMC/MCU协议]
    end
    
    subgraph "协议控制层"
        PCL[PCL<br/>Protocol Control Layer<br/>协议注册/调度]
    end
    
    subgraph "硬件抽象层"
        HAL[HAL<br/>Hardware Abstraction Layer<br/>CAN/UART/I2C/SPI/GPIO]
    end
    
    subgraph "操作系统抽象层"
        OSAL[OSAL<br/>OS Abstraction Layer<br/>线程/内存/文件/网络]
    end
    
    subgraph "底层"
        OS[Operating System<br/>Linux/RTOS/Bare-metal]
        HW[Hardware<br/>TI AM62x/Zynq MPSoC]
    end
    
    Apps --> ACL
    ACL --> PDL
    PDL --> PCL
    PCL --> HAL
    HAL --> OSAL
    OSAL --> OS
    HAL --> HW
    
    style Apps fill:#e1f5ff
    style ACL fill:#b3e5fc
    style PDL fill:#81d4fa
    style PCL fill:#4fc3f7
    style HAL fill:#29b6f6
    style OSAL fill:#039be5
    style OS fill:#01579b,color:#fff
    style HW fill:#01579b,color:#fff
```

### 架构特点

- **分层清晰**: 每层职责明确，接口规范
- **向下依赖**: 上层依赖下层，下层不依赖上层
- **可移植性**: OSAL 和 HAL 屏蔽平台差异
- **可扩展性**: 易于添加新协议和新应用
- **可配置性**: Kconfig 灵活配置功能模块

## 核心模块

### OSAL（操作系统抽象层）

**职责**: 屏蔽不同操作系统的差异，提供统一的系统调用接口。

**功能**:
- 线程管理（创建、销毁、同步）
- 进程管理（创建、等待、信号）
- 内存管理（分配、释放、共享内存）
- 文件操作（打开、读写、关闭）
- 网络通信（Socket、TCP/UDP）
- 时间管理（时钟、定时器、延时）

**支持平台**:
- Linux (POSIX)
- Windows (Win32)
- RTOS (FreeRTOS, RT-Thread)
- Bare Metal

### HAL（硬件抽象层）

**职责**: 屏蔽不同硬件平台的差异，提供统一的硬件访问接口。

**功能**:
- CAN 总线（发送、接收、过滤）
- UART 串口（配置、读写、流控）
- I2C 总线（主从模式、10位地址）
- SPI 总线（主从模式、DMA）
- GPIO（输入输出、中断、防抖）
- Watchdog（启动、喂狗、超时）

**支持平台**:
- Generic Linux (SocketCAN, sysfs)
- TI AM62x
- Xilinx Zynq MPSoC

### PCL（协议控制层）

**职责**: 管理通信协议的注册、查找和调度。

**功能**:
- 协议注册（版本管理）
- 协议查找（按 ID 或名称）
- 协议调度（解析、处理、响应）
- 协议验证（CRC、校验和）

### PDL（协议数据层）

**职责**: 实现具体的通信协议和数据处理。

**功能**:
- Watchdog 协议（心跳、超时检测）
- Satellite 协议（卫星平台通信）
- BMC 协议（基板管理控制器）
- MCU 协议（微控制器通信）

### ACL（应用控制层）

**职责**: 提供应用级的控制和管理功能。

**功能**:
- 遥测数据管理（采集、缓存、上报）
- 遥控指令处理（解析、执行、响应）
- 配置管理（加载、保存、验证）
- 状态监控（健康检查、告警）

## 📦 模块依赖关系

### 依赖图

```mermaid
graph LR
    subgraph "产品应用"
        APP1[Collector App]
        APP2[Comm App]
        APP3[Health App]
    end
    
    subgraph "产品库"
        LIBCCM[libccm]
        H200[h200_am625]
    end
    
    subgraph "核心模块"
        ACL[ACL]
        PDL[PDL]
        PCL[PCL]
        HAL[HAL]
        OSAL[OSAL]
    end
    
    APP1 --> H200
    APP1 --> LIBCCM
    APP2 --> H200
    APP2 --> LIBCCM
    APP3 --> H200
    
    H200 --> ACL
    LIBCCM --> OSAL
    
    ACL --> PDL
    ACL --> OSAL
    
    PDL --> PCL
    PDL --> HAL
    PDL --> OSAL
    
    PCL --> HAL
    PCL --> OSAL
    
    HAL --> OSAL
    
    style APP1 fill:#e8f5e9
    style APP2 fill:#e8f5e9
    style APP3 fill:#e8f5e9
    style LIBCCM fill:#fff3e0
    style H200 fill:#fff3e0
    style ACL fill:#e1f5ff
    style PDL fill:#e1f5ff
    style PCL fill:#e1f5ff
    style HAL fill:#e1f5ff
    style OSAL fill:#e1f5ff
```

### 依赖规则

1. **单向依赖**: 只能从上到下依赖
2. **最小依赖**: 只依赖直接需要的模块
3. **传递依赖**: CMake 自动处理传递依赖
4. **循环检测**: 构建系统会检测循环依赖

### 依赖示例

```cmake
# 应用只需声明直接依赖
target_link_libraries(collector
    PRIVATE
        h200_am625  # 平台库
        ccm         # 产品库
)

# CMake 自动链接传递依赖：
# h200_am625 → acl → pdl → pcl → hal → osal
```

---

## 🔄 数据流程

### 遥测数据采集流程

```mermaid
sequenceDiagram
    participant App as 应用程序
    participant ACL as ACL层
    participant PDL as PDL层
    participant HAL as HAL层
    participant HW as 硬件设备
    
    App->>ACL: 请求遥测数据
    ACL->>ACL: 查找配置表
    ACL->>PDL: 调用设备驱动
    PDL->>HAL: 读取硬件接口
    HAL->>HW: 访问设备
    HW-->>HAL: 返回原始数据
    HAL-->>PDL: 返回数据
    PDL->>PDL: 协议解析
    PDL-->>ACL: 返回解析后数据
    ACL->>ACL: 数据缓存
    ACL-->>App: 返回遥测数据
```

### 遥控指令处理流程

```mermaid
sequenceDiagram
    participant App as 应用程序
    participant ACL as ACL层
    participant PDL as PDL层
    participant HAL as HAL层
    participant HW as 硬件设备
    
    App->>ACL: 发送遥控指令
    ACL->>ACL: 指令验证
    ACL->>ACL: 查找配置表
    ACL->>PDL: 调用设备驱动
    PDL->>PDL: 协议封装
    PDL->>HAL: 写入硬件接口
    HAL->>HW: 发送数据
    HW-->>HAL: 返回状态
    HAL-->>PDL: 返回结果
    PDL->>PDL: 协议解析
    PDL-->>ACL: 返回执行结果
    ACL->>ACL: 记录日志
    ACL-->>App: 返回执行状态
```

### CAN 通信流程

```mermaid
sequenceDiagram
    participant App as 应用程序
    participant PDL as PDL Satellite
    participant HAL as HAL CAN
    participant Driver as Linux SocketCAN
    participant Bus as CAN总线
    
    Note over App,Bus: 发送流程
    App->>PDL: 发送卫星指令
    PDL->>PDL: 封装协议帧
    PDL->>HAL: HAL_CAN_Write()
    HAL->>Driver: write() 系统调用
    Driver->>Bus: 发送CAN帧
    
    Note over App,Bus: 接收流程
    Bus->>Driver: 接收CAN帧
    Driver->>HAL: read() 返回数据
    HAL->>PDL: 返回CAN帧
    PDL->>PDL: 解析协议
    PDL->>App: 返回卫星数据
```

---

## 依赖关系

```
Apps → ACL → PDL → PCL → HAL → OSAL
```

**规则**:
- 上层可以依赖下层
- 下层不能依赖上层
- 同层之间不能相互依赖

## 目录结构

```
EMS/
├── core/                   # 核心模块（可复用）
│   ├── osal/              # 操作系统抽象层
│   ├── hal/               # 硬件抽象层
│   ├── pcl/               # 协议控制层
│   ├── pdl/               # 协议数据层
│   └── acl/               # 应用控制层
├── products/              # 产品应用（特定产品）
│   └── ccm/              # CCM 产品
│       ├── apps/         # 应用程序
│       ├── libs/         # 产品库
│       └── h200_am625/   # 平台适配
├── tests/                 # 测试代码
│   ├── unit/             # 单元测试
│   ├── performance/      # 性能测试
│   ├── stress/           # 压力测试
│   └── system/           # 系统测试
├── configs/               # 配置文件
├── scripts/               # 构建脚本
└── docs/                  # 文档
```

## 构建系统

EMS 使用非递归 Make 构建系统：

- **顶层 Makefile**: 控制整体构建流程
- **module.mk**: 各模块的构建配置
- **scripts/rules.mk**: 通用构建规则
- **scripts/functions.mk**: 辅助函数
- **Kconfig**: 配置管理系统

## 配置系统

使用 Kconfig 实现灵活的模块裁剪：

- 按需启用/禁用模块
- 选择静态库或动态库
- 配置平台参数（架构、OS、工具链）
- 调整优化级别和调试选项

## 应用程序

### CCM 产品应用

- **ccm_collector**: 数据采集服务
- **ccm_health**: 健康监控服务
- **ccm_logger**: 日志记录服务
- **ccm_supervisor**: 监控管理服务
- **ccm_comm**: 通信管理服务

### 运行模式

- **独立运行**: 每个应用独立进程
- **协同工作**: 通过 IPC 通信（共享内存、消息队列）
- **统一管理**: supervisor 负责启动、监控、重启

## 扩展性

### 添加新平台

1. 在 `core/hal/src/` 下创建平台目录
2. 实现 HAL 接口
3. 在 `core/hal/Kconfig` 中添加配置选项
4. 在 `core/hal/module.mk` 中添加源文件

### 添加新协议

1. 在 `core/pdl/src/` 下创建协议目录
2. 实现协议接口
3. 在 `core/pdl/Kconfig` 中添加配置选项
4. 在 `core/pdl/module.mk` 中添加源文件

### 添加新应用

1. 在 `products/ccm/apps/` 下创建应用目录
2. 创建 `module.mk` 文件
3. 在 `products/ccm/Kconfig` 中添加配置选项
4. 在顶层 `Makefile` 中包含 `module.mk`

## 性能考虑

- **零拷贝**: 使用共享内存减少数据拷贝
- **异步 I/O**: 使用 epoll/select 实现高并发
- **内存池**: 预分配内存减少动态分配开销
- **缓存优化**: 数据结构对齐，提高缓存命中率

## 可靠性设计

- **错误处理**: 统一的错误码和错误处理机制
- **资源管理**: 严格的资源申请和释放
- **看门狗**: 防止进程死锁或崩溃
- **日志记录**: 完整的日志系统用于故障诊断

---

## 🔨 构建流程

### Kconfig + CMake 构建流程

```mermaid
flowchart TD
    Start([开始构建]) --> Config{配置方式}
    
    Config -->|menuconfig| Menu[图形化配置界面]
    Config -->|defconfig| Load[加载预定义配置]
    
    Menu --> Generate[生成 .config]
    Load --> Generate
    
    Generate --> Parse[genconfig.py 解析]
    Parse --> Output1[生成 global_config.cmake]
    Parse --> Output2[生成 global_config.h]
    Parse --> Output3[生成 global_config.mk]
    
    Output1 --> CMake[CMake 配置阶段]
    Output2 --> CMake
    
    CMake --> Check{检查配置}
    Check -->|缺少必要配置| Error1[报错退出]
    Check -->|配置正确| Select[选择性添加源文件]
    
    Select --> Compile[编译阶段]
    Compile --> Link[链接阶段]
    Link --> Output[生成产物]
    
    Output --> Bin[可执行文件<br/>_build/bin/]
    Output --> Lib[库文件<br/>_build/lib/]
    
    Error1 --> End([构建失败])
    Bin --> Success([构建成功])
    Lib --> Success
    
    style Start fill:#e8f5e9
    style Success fill:#c8e6c9
    style Error1 fill:#ffcdd2
    style End fill:#ef9a9a
    style Menu fill:#fff3e0
    style CMake fill:#e1f5ff
    style Compile fill:#b3e5fc
```

### 配置到编译的映射

```mermaid
graph LR
    subgraph "Kconfig 配置"
        K1[CONFIG_OSAL=y]
        K2[CONFIG_HAL=y]
        K3[CONFIG_HAL_CAN=y]
    end
    
    subgraph "CMake 变量"
        C1[CONFIG_OSAL]
        C2[CONFIG_HAL]
        C3[CONFIG_HAL_CAN]
    end
    
    subgraph "源文件选择"
        S1[add_subdirectory osal]
        S2[add_subdirectory hal]
        S3[list APPEND hal_can_linux.c]
    end
    
    subgraph "编译产物"
        O1[libosal.so]
        O2[libhal.so]
        O3[包含 CAN 驱动]
    end
    
    K1 --> C1 --> S1 --> O1
    K2 --> C2 --> S2 --> O2
    K3 --> C3 --> S3 --> O3
    
    style K1 fill:#e8f5e9
    style K2 fill:#e8f5e9
    style K3 fill:#e8f5e9
    style O1 fill:#e1f5ff
    style O2 fill:#e1f5ff
    style O3 fill:#e1f5ff
```

---

## 🎯 典型应用场景

### 场景 1: 卫星数据采集

```mermaid
graph TD
    Start([定时触发]) --> Collect[Collector App]
    Collect --> ACL[ACL_GetTelemetry]
    ACL --> Cache{缓存中有数据?}
    
    Cache -->|是| Return1[返回缓存数据]
    Cache -->|否| PDL[PDL_Satellite_Read]
    
    PDL --> HAL[HAL_CAN_Read]
    HAL --> Parse[解析卫星协议]
    Parse --> Store[存入缓存]
    Store --> Return2[返回数据]
    
    Return1 --> Process[数据处理]
    Return2 --> Process
    Process --> Save[保存到文件]
    Save --> End([完成])
    
    style Start fill:#e8f5e9
    style End fill:#c8e6c9
    style Collect fill:#fff3e0
    style ACL fill:#e1f5ff
    style PDL fill:#b3e5fc
    style HAL fill:#81d4fa
```

### 场景 2: 健康监控

```mermaid
graph TD
    Start([1秒定时器]) --> Health[Health App]
    Health --> Check1[检查 Watchdog]
    Check1 --> Check2[检查温度]
    Check2 --> Check3[检查电压]
    Check3 --> Check4[检查通信状态]
    
    Check4 --> Analyze{分析结果}
    Analyze -->|正常| Log1[记录正常日志]
    Analyze -->|异常| Alert[触发告警]
    
    Alert --> Action1[记录错误日志]
    Action1 --> Action2[发送告警消息]
    Action2 --> Action3[尝试恢复]
    
    Log1 --> End([继续监控])
    Action3 --> End
    
    style Start fill:#e8f5e9
    style End fill:#c8e6c9
    style Health fill:#fff3e0
    style Alert fill:#ffcdd2
```

---

## 📐 设计原则

### 1. 分层原则

- **职责分离**: 每层只负责特定功能
- **接口清晰**: 层与层之间通过明确的 API 交互
- **向下依赖**: 只能依赖下层，不能依赖上层

### 2. 模块化原则

- **高内聚**: 模块内部功能紧密相关
- **低耦合**: 模块之间依赖最小化
- **可替换**: 同层模块可以独立替换

### 3. 可配置原则

- **功能裁剪**: 通过 Kconfig 灵活配置
- **平台适配**: 支持多种操作系统和硬件平台
- **参数化**: 关键参数可配置

### 4. 可扩展原则

- **插件机制**: 易于添加新的协议和驱动
- **注册机制**: 动态注册和查找
- **回调机制**: 支持事件驱动

---

## 🔍 关键设计模式

### 1. 抽象工厂模式 (OSAL/HAL)

```c
// 统一接口，多平台实现
void* HAL_CAN_Open(const char *device, const hal_can_config_t *config);

// Linux 实现
void* HAL_CAN_Open_Linux(...);

// RTOS 实现
void* HAL_CAN_Open_RTOS(...);
```

### 2. 注册表模式 (PCL)

```c
// 协议注册
PCL_RegisterProtocol(protocol_id, handler);

// 协议查找
handler = PCL_FindProtocol(protocol_id);

// 协议调用
handler(data, len);
```

### 3. 观察者模式 (事件通知)

```c
// 注册回调
ACL_RegisterCallback(event_type, callback);

// 触发事件
ACL_NotifyEvent(event_type, data);
```

### 4. 单例模式 (全局管理器)

```c
// 获取全局实例
acl_manager_t* ACL_GetManager(void);
```

---

## 📚 扩展阅读

- [新手入门教程](GETTING_STARTED.md) - 快速上手
- [开发者指南](DEVELOPER_GUIDE.md) - 添加新功能
- [构建指南](CMAKE_BUILD_GUIDE.md) - 构建系统详解
- [配置指南](CONFIGURATION.md) - Kconfig 配置
- [编码规范](CODING_STANDARDS.md) - 代码风格

---

**更新时间**: 2026-05-28  
**版本**: 1.0.0
