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
Products (CCM, PMC, GSC)
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
| MCU | `PRL_DEV_TYPE_MCU` | `prl_mcu.h` | 微控制器 | ✅ 是 |
| CCM | `PRL_DEV_TYPE_CCM` | `prl_ccm.h` | 通信管理板 | ✅ 是 |
| PMC | `PRL_DEV_TYPE_PMC` | `prl_pmc.h` | 载荷管理器 | ✅ 是 |
| GSC | `PRL_DEV_TYPE_GSC` | `prl_gsc.h` | 地面站控制器 | ✅ 是 |
| POWER | `PRL_DEV_TYPE_POWER` | `prl_power.h` | 电源板 | ✅ 是 |
| SATELLITE | `PRL_DEV_TYPE_SATELLITE` | - | 卫星平台 | ❌ 否（使用卫星专用协议） |
| BMC | `PRL_DEV_TYPE_BMC` | - | 基板管理控制器 | ❌ 否（使用 IPMI/Redfish） |

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
    
    return OSAL_SUCCESS;
}
```

## 配置选项

在 `Kconfig` 中配置：

```kconfig
CONFIG_PRL=y                    # 启用协议层
CONFIG_PRL_BUILD_SHARED=y       # 构建为动态库
CONFIG_PRL_MCU=y                # 启用 MCU 设备协议
CONFIG_PRL_CCM=y                # 启用 CCM 设备协议
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
│   ├── prl_mcu.h             # MCU 设备协议
│   ├── prl_ccm.h             # CCM 设备协议
│   ├── prl_pmc.h             # PMC 设备协议
│   ├── prl_gsc.h             # GSC 设备协议
│   └── prl_power.h           # POWER 设备协议
├── src/
│   ├── prl_api.c             # 对外 API 实现
│   ├── prl_common.c          # 通用实现
│   ├── prl_device.c          # 设备消息实现
│   ├── prl_mcu.c             # MCU 协议实现
│   ├── prl_ccm.c             # CCM 协议实现
│   ├── prl_pmc.c             # PMC 协议实现
│   ├── prl_gsc.c             # GSC 协议实现
│   └── prl_power.c           # POWER 协议实现
├── CMakeLists.txt            # 构建配置
├── Kconfig                   # 配置选项
└── README.md                 # 本文档
```

## 版本历史

- **v1.2** (2026-06-01): 架构重构
  - 从"点对点"协议改为"按设备类型"组织
  - 删除 prl_gsc_pmc, prl_pmc_ccm, prl_ccm_satellite, prl_ccm_power
  - 新增 prl_gsc, prl_pmc, prl_ccm, prl_power
  - 更新配置选项（CONFIG_PRL_MCU, CONFIG_PRL_CCM 等）
  - 符合统一协议的设计理念

- **v1.1** (2026-06-01): 架构优化
  - 统一命名规范（对外 API 使用 PRL_ 前缀）
  - 新增 `prl.h` 统一对外接口
  - 完善文档（架构设计、使用指南）
  - 明确 PRL 与 PDL 的职责边界

- **v1.0** (2026-05-29): 初始版本
  - 完成协议框架设计
  - 完成 PMC-CCM 协议实现
  - 定义其他协议接口

## 依赖

- **OSAL**: 时间函数、内存管理、字符串操作

---

**维护者**: wanguo  
**最后更新**: 2026-06-01

## 支持的协议

### 1. GSC-PMC 协议 (`prl_gsc_pmc.h`)
- **用途**: 地面站 ↔ 载荷管理器
- **传输**: 万兆以太网
- **消息类型**:
  - 心跳
  - 遥测数据
  - 遥控指令
  - 文件传输
  - 数据库同步
  - 日志上传

### 2. PMC-CCM 协议 (`prl_pmc_ccm.h`)
- **用途**: 载荷管理器 ↔ 通信管理板
- **传输**: 千兆以太网
- **消息类型**:
  - 心跳
  - 遥测数据
  - 遥控指令
  - 固件升级
  - 节点管理
  - 电源控制
  - 状态查询

### 3. CCM-Satellite 协议 (`prl_ccm_satellite.h`)
- **用途**: 通信管理板 ↔ 卫星平台
- **传输**: CAN 总线 + OC 指令
- **消息类型**:
  - 心跳
  - 遥测数据
  - 遥控指令
  - 时间同步
  - 轨道数据
  - 姿态数据
  - 电源状态
  - 热控状态

### 4. CCM-Power 协议 (`prl_ccm_power.h`)
- **用途**: 通信管理板 ↔ 电源管理板
- **传输**: CAN 总线
- **消息类型**:
  - 心跳
  - 上电/下电
  - 电压/电流/温度查询
  - 状态上报
  - 告警

## 协议格式

### 协议头（所有协议通用）

```c
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint16_t version;       /* 协议版本 */
    uint8_t  msg_type;      /* 消息类型 */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 数据长度 */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳（秒） */
    uint16_t crc16;         /* CRC16 校验 */
    uint16_t reserved;      /* 保留 */
} prl_header_t;
```

**大小**: 20 字节

### 报文结构

```
+----------------+------------------+----------+
| 协议头 (20B)   | 消息体 (变长)    | CRC (包含在头中) |
+----------------+------------------+----------+
```

### 标志位

- `PRL_FLAG_ACK_REQUIRED` (0x01): 需要应答
- `PRL_FLAG_IS_ACK` (0x02): 是应答报文
- `PRL_FLAG_ENCRYPTED` (0x04): 加密报文
- `PRL_FLAG_COMPRESSED` (0x08): 压缩报文

## 使用方法

### 1. 发送消息（编码）

```c
#include "prl_pmc_ccm.h"

/* 准备消息 */
prl_pmc_ccm_heartbeat_t heartbeat = {
    .sender_status = 0x01,
    .link_quality = 95,
    .packet_loss = 50,
    .rtt_ms = 10,
};

/* 编码 */
uint8_t buf[PRL_MAX_PACKET_SIZE];
size_t len = sizeof(buf);
int ret = prl_pmc_ccm_encode_heartbeat(&heartbeat, buf, &len);

/* 发送 */
if (ret == PRL_OK) {
    osal_socket_send(socket_fd, buf, len);
}
```

### 2. 接收消息（解码）

```c
/* 接收 */
uint8_t buf[PRL_MAX_PACKET_SIZE];
size_t len;
osal_socket_recv(socket_fd, buf, &len);

/* 解析协议头 */
const prl_header_t *hdr = (const prl_header_t *)buf;

/* 根据消息类型解码 */
switch (hdr->msg_type) {
    case PRL_PMC_CCM_MSG_HEARTBEAT: {
        prl_pmc_ccm_heartbeat_t heartbeat;
        int ret = prl_pmc_ccm_decode_heartbeat(buf, len, &heartbeat);
        if (ret == PRL_OK) {
            /* 处理心跳消息 */
        }
        break;
    }
    // ... 其他消息类型
}
```

### 3. 变长消息处理

```c
/* 编码变长消息 */
prl_pmc_ccm_telemetry_t telemetry = { /* ... */ };
uint8_t tm_data[128] = { /* 遥测数据 */ };
size_t tm_data_len = 64;

uint8_t buf[PRL_MAX_PACKET_SIZE];
size_t len = sizeof(buf);
prl_pmc_ccm_encode_telemetry(&telemetry, tm_data, tm_data_len, buf, &len);

/* 解码变长消息 */
prl_pmc_ccm_telemetry_t telemetry;
uint8_t *data;
size_t data_len;
prl_pmc_ccm_decode_telemetry(buf, len, &telemetry, &data, &data_len);
/* data 指向报文中的数据，无需额外分配内存 */
```

## 配置选项

在 `Kconfig` 中配置：

```kconfig
CONFIG_PRL=y                    # 启用协议层
CONFIG_PRL_BUILD_SHARED=y       # 构建为动态库
CONFIG_PRL_GSC_PMC=y            # 启用 GSC-PMC 协议
CONFIG_PRL_PMC_CCM=y            # 启用 PMC-CCM 协议
CONFIG_PRL_CCM_SATELLITE=y      # 启用 CCM-Satellite 协议
CONFIG_PRL_CCM_POWER=y          # 启用 CCM-Power 协议
CONFIG_PRL_CRC_CHECK=y          # 启用 CRC 校验
CONFIG_PRL_SEQUENCE_CHECK=y     # 启用序列号检查
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
```

## 工具函数

```c
/* CRC16 计算 */
uint16_t prl_calc_crc16(const uint8_t *data, size_t len);

/* 获取序列号 */
uint32_t prl_get_next_seq(void);

/* 获取时间戳 */
uint32_t prl_get_timestamp(void);

/* 初始化协议头 */
void prl_init_header(prl_header_t *hdr, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags);

/* 验证协议头 */
int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type);

/* 设置报文 CRC */
void prl_set_packet_crc(uint8_t *packet, size_t total_len);

/* 验证报文 CRC */
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);
```

## 示例代码

完整示例请参考 `prl_usage_example.c`。

## 协议扩展

### 添加新消息类型

1. 在协议头文件中定义消息类型枚举
2. 定义消息结构体
3. 实现编码函数
4. 实现解码函数

示例：

```c
/* 1. 定义消息类型 */
typedef enum {
    PRL_PMC_CCM_MSG_NEW_TYPE = 0x10,
} prl_pmc_ccm_msg_type_t;

/* 2. 定义消息结构 */
typedef struct {
    uint32_t field1;
    uint32_t field2;
} prl_pmc_ccm_new_type_t;

/* 3. 编码函数 */
int prl_pmc_ccm_encode_new_type(const prl_pmc_ccm_new_type_t *msg,
                                 uint8_t *buf, size_t *len);

/* 4. 解码函数 */
int prl_pmc_ccm_decode_new_type(const uint8_t *buf, size_t len,
                                 prl_pmc_ccm_new_type_t *msg);
```

## 性能考虑

- **零拷贝**: 解码时直接返回报文中的数据指针，避免内存拷贝
- **CRC 查表**: 使用查找表加速 CRC16 计算
- **原子操作**: 序列号使用原子操作，线程安全
- **最大报文**: 默认 4KB，可通过 Kconfig 配置

## 注意事项

1. **字节序**: 所有多字节字段使用主机字节序，跨平台通信需注意
2. **对齐**: 结构体使用 `__attribute__((packed))` 避免填充
3. **变长数据**: 变长数据必须放在结构体末尾，使用 `data[0]` 占位
4. **CRC 校验**: 发送前必须调用 `prl_set_packet_crc()`
5. **缓冲区大小**: 编码时确保缓冲区足够大（建议使用 `PRL_MAX_PACKET_SIZE`）

## 依赖

- **OSAL**: 时间函数、内存管理

## 文件列表

```
core/prl/
├── include/
│   ├── prl_common.h          # 通用定义
│   ├── prl_gsc_pmc.h         # GSC-PMC 协议
│   ├── prl_pmc_ccm.h         # PMC-CCM 协议
│   ├── prl_ccm_satellite.h   # CCM-Satellite 协议
│   └── prl_ccm_power.h       # CCM-Power 协议
├── src/
│   ├── prl_common.c          # 通用实现
│   ├── prl_gsc_pmc.c         # GSC-PMC 实现（待实现）
│   ├── prl_pmc_ccm.c         # PMC-CCM 实现
│   ├── prl_ccm_satellite.c   # CCM-Satellite 实现（待实现）
│   └── prl_ccm_power.c       # CCM-Power 实现（待实现）
├── prl_usage_example.c       # 使用示例
├── CMakeLists.txt            # 构建配置
├── Kconfig                   # 配置选项
└── README.md                 # 本文档
```

## 待实现

- [ ] `prl_gsc_pmc.c` - GSC-PMC 协议实现
- [ ] `prl_ccm_satellite.c` - CCM-Satellite 协议实现
- [ ] `prl_ccm_power.c` - CCM-Power 协议实现
- [ ] 单元测试
- [ ] 性能测试

## 版本历史

- **v1.0** (2026-05-29): 初始版本
  - 完成协议框架设计
  - 完成 PMC-CCM 协议实现
  - 定义其他三个协议接口
