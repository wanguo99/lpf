# PRL (Protocol Layer) - 协议层

## 概述

PRL (Protocol Layer) 是 ES-Middleware SDK 的协议层，位于 Core 层，为不同外设之间的通信提供统一的协议定义和编解码功能。

**设计理念**：
- **统一协议**：所有内部外设使用统一的协议格式
- **设备无关**：通过设备类型字段区分不同设备
- **高性能**：零拷贝设计，CRC 查表加速
- **易扩展**：支持新设备类型和消息类型的快速添加

## 架构定位

```
Products (PMC, PMC, GSC)
  ↓ 调用业务接口
PDL (设备层) ← 封装设备通信逻辑
  ↓ 调用协议接口
PRL (协议层) ← Core 层（本模块）
  ↓ 使用传输层
HAL (CAN, UART, I2C, SPI)
  ↓
OSAL (操作系统抽象层)
```

**PRL 职责**：
- 定义统一的协议格式（协议头 + 负载）
- 提供协议编解码接口（`PRL_Encode` / `PRL_Decode`）
- 管理序列号、时间戳、CRC 校验
- 不关心具体的传输方式和设备业务逻辑

**PDL 职责**：
- 封装具体设备的通信逻辑
- 调用 PRL 进行协议编解码
- 通过 HAL 层进行实际的数据传输
- 处理设备特定的业务逻辑

## 快速开始

### 1. 包含头文件

```c
#include "prl.h"  /* PRL 统一头文件 */
```

### 2. 编码消息

```c
uint8_t buffer[PRL_MAX_PACKET_SIZE];
prl_mcu_version_t version = {1, 2, 3};

int len = PRL_Encode(
    PRL_DEV_TYPE_MCU,           /* 设备类型 */
    PRL_MCU_MSG_GET_VERSION,    /* 消息类型 */
    &version,                   /* 负载数据 */
    sizeof(version),            /* 负载长度 */
    buffer,                     /* 输出缓冲区 */
    sizeof(buffer),             /* 缓冲区大小 */
    0                           /* 标志位 */
);

if (len > 0) {
    /* 通过 HAL 发送 */
    HAL_CAN_Send(handle, buffer, len);
}
```

### 3. 解码消息

```c
uint8_t dev_type, msg_type;
const uint8_t *payload;
uint16_t payload_len;

int ret = PRL_Decode(
    packet,         /* 报文数据 */
    packet_len,     /* 报文长度 */
    &dev_type,      /* 输出：设备类型 */
    &msg_type,      /* 输出：消息类型 */
    &payload,       /* 输出：负载指针（零拷贝） */
    &payload_len    /* 输出：负载长度 */
);

if (ret == PRL_OK) {
    /* 处理消息 */
    handle_message(dev_type, msg_type, payload, payload_len);
}
```

## 支持的设备类型

| 设备类型 | 枚举值 | 头文件 | 说明 | 是否使用 PRL |
|---------|--------|--------|------|-------------|
| MCU | `PRL_DEV_TYPE_MCU` | `prl_mcu.h` | 微控制器 | 是 是 |
| PMC | `PRL_DEV_TYPE_PMC` | `prl_pmc.h` | 通信管理板 | 是 是 |
| PMC | `PRL_DEV_TYPE_PMC` | `prl_pmc.h` | 载荷管理器 | 是 是 |
| GSC | `PRL_DEV_TYPE_GSC` | `prl_gsc.h` | 地面站控制器 | 是 是 |
| POWER | `PRL_DEV_TYPE_POWER` | `prl_power.h` | 电源板 | 是 是 |
| SATELLITE | `PRL_DEV_TYPE_SATELLITE` | - | 卫星平台 | 否 否（使用卫星专用协议） |
| BMC | `PRL_DEV_TYPE_BMC` | - | 基板管理控制器 | 否 否（使用 IPMI/Redfish） |

**注意**：协议是统一的，任何设备都可以与任何设备通信，只需指定正确的设备类型和消息类型。

## 协议格式

### 协议头（20 字节）

```c
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint8_t  version;       /* 协议版本：0x10 (v1.0) */
    uint8_t  dev_type;      /* 设备类型 */
    uint8_t  msg_type;      /* 消息类型 */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 负载长度 */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳（秒） */
    uint16_t crc16;         /* CRC16 校验 */
    uint16_t reserved;      /* 保留 */
} __attribute__((packed)) prl_header_t;
```

### 报文结构

```
+----------------+------------------+
| 协议头 (20B)   | 负载数据 (变长)  |
+----------------+------------------+
|<------- CRC16 校验范围 -------->|
```

## 主要 API

### 编解码接口

```c
/* 编码设备消息 */
int PRL_Encode(uint8_t dev_type, uint8_t msg_type,
               const void *payload, uint16_t payload_len,
               uint8_t *buffer, size_t buffer_size, uint8_t flags);

/* 解码设备消息 */
int PRL_Decode(const uint8_t *packet, size_t packet_len,
               uint8_t *dev_type, uint8_t *msg_type,
               const uint8_t **payload, uint16_t *payload_len);
```

### 工具接口

```c
/* 验证设备类型 */
bool PRL_IsDeviceTypeValid(uint8_t dev_type);

/* 获取设备类型名称 */
const char *PRL_GetDeviceTypeName(uint8_t dev_type);

/* 获取错误描述 */
const char *PRL_GetErrorString(int error_code);

/* 验证报文完整性 */
int PRL_ValidatePacket(const uint8_t *packet, size_t packet_len);

/* 构建应答报文 */
int PRL_BuildResponse(const uint8_t *request_packet, size_t request_len,
                      const void *response_payload, uint16_t response_payload_len,
                      uint8_t *response_buffer, size_t response_buffer_size);
```

## 命名规范

**对外 API**：使用 `PRL_` 前缀（大写）
```c
int PRL_Encode(...);
int PRL_Decode(...);
```

**内部函数**：使用 `prl_` 前缀（小写）
```c
static int prl_validate_header(...);
static uint16_t prl_calc_crc16(...);
```

**类型定义**：使用 `prl_` 前缀 + `_t` 后缀
```c
typedef struct { ... } prl_header_t;
typedef enum { ... } prl_dev_type_t;
```

**宏定义**：使用 `PRL_` 前缀（大写）
```c
#define PRL_MAGIC_NUMBER        0xAA55
#define PRL_MAX_PACKET_SIZE     4096
```

## 在 PDL 中使用

```c
/* PDL_MCU 内部实现示例 */
int32_t PDL_MCU_GetVersion(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
    uint8_t tx_buf[PRL_MAX_PACKET_SIZE];
    uint8_t rx_buf[PRL_MAX_PACKET_SIZE];
    
    /* 1. 使用 PRL 编码请求 */
    int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                         NULL, 0, tx_buf, sizeof(tx_buf),
                         PRL_FLAG_ACK_REQUIRED);
    
    /* 2. 通过 HAL 发送 */
    HAL_CAN_Send(handle->can_handle, tx_buf, len);
    
    /* 3. 通过 HAL 接收 */
    size_t rx_len;
    HAL_CAN_Recv(handle->can_handle, rx_buf, &rx_len, 1000);
    
    /* 4. 使用 PRL 解码响应 */
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    PRL_Decode(rx_buf, rx_len, &dev_type, &msg_type, &payload, &payload_len);
    
    /* 5. 处理业务数据 */
    memcpy(version, payload, sizeof(pdl_mcu_version_t));
    
    return 0;
}
```

## 配置选项

在 `Kconfig` 中配置：

```kconfig
CONFIG_PRL=y                    # 启用协议层
CONFIG_PRL_BUILD_SHARED=y       # 构建为动态库
CONFIG_PRL_MCU=y                # 启用 MCU 设备协议
CONFIG_PRL_PMC=y                # 启用 PMC 设备协议
CONFIG_PRL_PMC=y                # 启用 PMC 设备协议
CONFIG_PRL_GSC=y                # 启用 GSC 设备协议
CONFIG_PRL_POWER=y              # 启用 POWER 设备协议
```

## 错误码

```c
PRL_OK                  = 0     /* 成功 */
PRL_ERR_INVALID_PARAM   = -1    /* 无效参数 */
PRL_ERR_INVALID_MAGIC   = -2    /* 无效魔数 */
PRL_ERR_INVALID_VERSION = -3    /* 无效版本 */
PRL_ERR_INVALID_TYPE    = -4    /* 无效消息类型 */
PRL_ERR_INVALID_LENGTH  = -5    /* 无效长度 */
PRL_ERR_CRC_FAILED      = -6    /* CRC 校验失败 */
PRL_ERR_BUFFER_TOO_SMALL = -7   /* 缓冲区太小 */
PRL_ERR_ENCODE_FAILED   = -8    /* 编码失败 */
PRL_ERR_DECODE_FAILED   = -9    /* 解码失败 */
PRL_ERR_INVALID_DEV_TYPE = -10  /* 无效设备类型 */
```

## 性能特性

- **零拷贝**: 解码时直接返回报文中的数据指针，避免内存拷贝
- **CRC 查表**: 使用查找表加速 CRC16 计算
- **原子操作**: 序列号使用原子操作，线程安全
- **最大报文**: 默认 4KB，可通过 Kconfig 配置

## 文档

- [架构设计文档](../../docs/PRL_ARCHITECTURE.md) - 详细的架构设计和扩展指南
- [使用指南](../../docs/PRL_USAGE_GUIDE.md) - 完整的使用示例和最佳实践
- [命名规范](../../docs/NAMING_CONVENTIONS.md) - 项目命名规范

## 文件列表

```
core/prl/
├── include/
│   ├── prl.h                 # 统一头文件（推荐使用）
│   ├── prl_common.h          # 通用定义（内部）
│   ├── prl_device.h          # 设备消息定义（内部）
│   ├── prl_mcu.h             # MCU 设备协议定义
│   ├── prl_pmc.h             # PMC 设备协议定义
│   ├── prl_pmc.h             # PMC 设备协议定义
│   ├── prl_gsc.h             # GSC 设备协议定义
│   └── prl_power.h           # POWER 设备协议定义
├── src/
│   ├── prl_api.c             # 对外 API 实现
│   ├── prl_common.c          # 通用实现
│   └── prl_device.c          # 设备消息统一实现
├── CMakeLists.txt            # 构建配置
├── Kconfig                   # 配置选项
└── README.md                 # 本文档
```

## 版本历史

- **v1.2** (2026-06-01): 架构重构
  - 从"点对点"协议改为"按设备类型"组织
  - 删除旧的点对点协议文件（prl_gsc_pmc, prl_pmc_pmc, prl_pmc_satellite, prl_pmc_power）
  - 新增按设备类型组织的协议定义（prl_gsc, prl_pmc, prl_pmc, prl_power）
  - 更新配置选项（CONFIG_PRL_MCU, CONFIG_PRL_PMC 等）
  - 符合统一协议的设计理念

- **v1.1** (2026-06-01): 架构优化
  - 统一命名规范（对外 API 使用 PRL_ 前缀）
  - 新增 `prl.h` 统一对外接口
  - 完善文档（架构设计、使用指南）
  - 明确 PRL 与 PDL 的职责边界

- **v1.0** (2026-05-29): 初始版本
  - 完成协议框架设计
  - 完成基础协议实现
  - 定义协议接口

## 依赖

- **OSAL**: 时间函数、内存管理、字符串操作

---

**维护者**: wanguo  
**最后更新**: 2026-06-15
