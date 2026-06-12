# PRL (Protocol Layer) 架构设计文档

## 1. 概述

PRL（Protocol Layer，协议层）是 ES-Middleware SDK 的核心协议层，为不同外设之间的通信提供统一的协议定义和编解码功能。

### 1.1 设计目标

- **统一协议**：所有内部外设使用统一的协议格式
- **设备无关**：通过设备类型字段区分不同设备
- **高性能**：零拷贝设计，CRC 查表加速
- **易扩展**：支持新设备类型和消息类型的快速添加
- **类型安全**：强类型 API，编译期检查

### 1.2 架构定位

```
┌─────────────────────────────────────────────────────────┐
│                    Products Layer                        │
│              (CCM, PMC, GSC Applications)                │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    ACONFIG Layer                         │
│           (Application Configuration Layer)              │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      PDL Layer                           │
│         (Peripheral Device Layer - 设备驱动)             │
│  ┌──────────┬──────────┬──────────┬──────────────────┐  │
│  │ PDL_MCU  │ PDL_BMC  │ PDL_SAT  │ PDL_WATCHDOG ... │  │
│  └──────────┴──────────┴──────────┴──────────────────┘  │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      PRL Layer                           │
│         (Protocol Layer - 协议编解码)                    │
│  ┌────────────────────────────────────────────────────┐ │
│  │  统一协议头 + 设备类型字段 + 消息类型 + 负载       │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      HAL Layer                           │
│         (Hardware Abstraction Layer)                     │
│  ┌──────────┬──────────┬──────────┬──────────────────┐  │
│  │ HAL_CAN  │ HAL_UART │ HAL_I2C  │ HAL_SPI ...      │  │
│  └──────────┴──────────┴──────────┴──────────────────┘  │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      OSAL Layer                          │
│         (Operating System Abstraction Layer)             │
└─────────────────────────────────────────────────────────┘
```

### 1.3 与 PDL 的关系

**PRL（协议层）职责**：
- 定义统一的协议格式
- 提供协议编解码接口
- 管理序列号、时间戳、CRC 校验
- 不关心具体的传输方式

**PDL（设备层）职责**：
- 封装具体设备的通信逻辑
- 调用 PRL 进行协议编解码
- 通过 HAL 层进行实际的数据传输
- 处理设备特定的业务逻辑

**示例**：
```c
// PDL_MCU 内部实现
int32_t PDL_MCU_GetVersion(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
    // 1. 使用 PRL 编码请求
    uint8_t tx_buf[PRL_MAX_PACKET_SIZE];
    int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                         NULL, 0, tx_buf, OSAL_SIZEOF(tx_buf), 0);
    
    // 2. 通过 HAL 发送
    HAL_CAN_Send(handle->can_handle, tx_buf, len);
    
    // 3. 通过 HAL 接收
    uint8_t rx_buf[PRL_MAX_PACKET_SIZE];
    size_t rx_len;
    HAL_CAN_Recv(handle->can_handle, rx_buf, &rx_len);
    
    // 4. 使用 PRL 解码响应
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    PRL_Decode(rx_buf, rx_len, &dev_type, &msg_type, &payload, &payload_len);
    
    // 5. 解析业务数据
    memcpy(version, payload, OSAL_SIZEOF(pdl_mcu_version_t));
    
    return OSAL_SUCCESS;
}
```

## 2. 协议格式

### 2.1 协议头定义

```c
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint8_t  version;       /* 协议版本：高4位主版本，低4位次版本 */
    uint8_t  dev_type;      /* 设备类型（区分不同外设） */
    uint8_t  msg_type;      /* 消息类型（设备特定） */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 负载长度（不包括协议头） */
    uint32_t seq;           /* 序列号（用于去重和排序） */
    uint32_t timestamp;     /* 时间戳（秒） */
    uint16_t crc16;         /* CRC16 校验（整个报文） */
    uint16_t reserved;      /* 保留字段（未来扩展） */
} __attribute__((packed)) prl_header_t;
```

**大小**：20 字节

**字段说明**：
- `magic`：固定值 0xAA55，用于快速识别协议报文
- `version`：协议版本，当前为 0x10（v1.0）
- `dev_type`：设备类型，用于区分不同外设（MCU、CCM、PMC 等）
- `msg_type`：消息类型，由各设备自定义
- `flags`：标志位（需要应答、是应答、加密、压缩等）
- `length`：负载数据长度，不包括协议头
- `seq`：序列号，自动递增，用于去重和排序
- `timestamp`：时间戳（秒），用于超时检测
- `crc16`：CRC16-CCITT 校验，覆盖整个报文
- `reserved`：保留字段，用于未来扩展

### 2.2 报文结构

```
+----------------+------------------+
| 协议头 (20B)   | 负载数据 (变长)  |
+----------------+------------------+
|<------- CRC16 校验范围 -------->|
```

### 2.3 设备类型定义

```c
typedef enum {
    PRL_DEV_TYPE_UNKNOWN    = 0x00,     /* 未知设备 */
    PRL_DEV_TYPE_MCU        = 0x01,     /* 微控制器 */
    PRL_DEV_TYPE_CCM        = 0x02,     /* 通信管理板 */
    PRL_DEV_TYPE_PMC        = 0x03,     /* 载荷管理器 */
    PRL_DEV_TYPE_GSC        = 0x04,     /* 地面站控制器 */
    PRL_DEV_TYPE_SATELLITE  = 0x05,     /* 卫星平台（不使用 PRL） */
    PRL_DEV_TYPE_POWER      = 0x06,     /* 电源板 */
    PRL_DEV_TYPE_BMC        = 0x07,     /* 基板管理控制器（不使用 PRL） */
} prl_dev_type_t;
```

**注意**：
- `PRL_DEV_TYPE_SATELLITE`：卫星平台使用卫星专用协议，不使用 PRL
- `PRL_DEV_TYPE_BMC`：BMC 使用 IPMI/Redfish 协议，不使用 PRL

### 2.4 标志位定义

```c
#define PRL_FLAG_ACK_REQUIRED   0x01    /* 需要应答 */
#define PRL_FLAG_IS_ACK         0x02    /* 是应答报文 */
#define PRL_FLAG_ENCRYPTED      0x04    /* 加密报文（预留） */
#define PRL_FLAG_COMPRESSED     0x08    /* 压缩报文（预留） */
```

## 3. API 设计

### 3.1 命名规范

**对外 API**：使用 `PRL_` 前缀（大写）
```c
int PRL_Encode(...);
int PRL_Decode(...);
int PRL_Init(...);
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

### 3.2 核心 API

#### 3.2.1 编码接口

```c
/**
 * @brief 编码设备消息
 * @param dev_type 设备类型
 * @param msg_type 消息类型
 * @param payload 负载数据
 * @param payload_len 负载长度
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param flags 标志位
 * @return 成功返回编码后的总长度，失败返回负数错误码
 */
int PRL_Encode(uint8_t dev_type, uint8_t msg_type,
               const void *payload, uint16_t payload_len,
               uint8_t *buffer, size_t buffer_size, uint8_t flags);
```

#### 3.2.2 解码接口

```c
/**
 * @brief 解码设备消息
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param dev_type 输出：设备类型
 * @param msg_type 输出：消息类型
 * @param payload 输出：负载数据指针（指向 packet 内部，零拷贝）
 * @param payload_len 输出：负载长度
 * @return 成功返回 PRL_OK，失败返回负数错误码
 */
int PRL_Decode(const uint8_t *packet, size_t packet_len,
               uint8_t *dev_type, uint8_t *msg_type,
               const uint8_t **payload, uint16_t *payload_len);
```

#### 3.2.3 工具接口

```c
/**
 * @brief 验证设备类型是否有效
 */
bool PRL_IsDeviceTypeValid(uint8_t dev_type);

/**
 * @brief 获取设备类型名称
 */
const char *PRL_GetDeviceTypeName(uint8_t dev_type);

/**
 * @brief 获取错误码描述
 */
const char *PRL_GetErrorString(int error_code);
```

### 3.3 内部工具函数

```c
/* 协议头操作 */
void prl_init_header(prl_header_t *hdr, uint8_t dev_type, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags);
int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type);

/* CRC 计算 */
uint16_t prl_calc_crc16(const uint8_t *data, size_t len);
void prl_set_packet_crc(uint8_t *packet, size_t total_len);
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);

/* 序列号和时间戳 */
uint32_t prl_get_next_seq(void);
uint32_t prl_get_timestamp(void);
```

## 4. 使用示例

### 4.1 基本编解码

```c
#include "prl.h"

/* 编码示例 */
void example_encode(void)
{
    /* 准备负载数据 */
    prl_mcu_version_t version = {
        .major = 1,
        .minor = 2,
        .patch = 3,
    };
    
    /* 编码 */
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                         &version, sizeof(version),
                         buffer, sizeof(buffer), 0);
    
    if (len > 0) {
        /* 发送 buffer */
        send_to_device(buffer, len);
    }
}

/* 解码示例 */
void example_decode(const uint8_t *packet, size_t packet_len)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    
    /* 解码 */
    int ret = PRL_Decode(packet, packet_len,
                         &dev_type, &msg_type,
                         &payload, &payload_len);
    
    if (ret == PRL_OK) {
        /* 根据设备类型和消息类型处理 */
        if (dev_type == PRL_DEV_TYPE_MCU) {
            handle_mcu_message(msg_type, payload, payload_len);
        }
    }
}
```

### 4.2 在 PDL 中使用

```c
/* PDL_MCU 内部实现示例 */
int32_t PDL_MCU_GetVersion(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
    uint8_t tx_buf[PRL_MAX_PACKET_SIZE];
    uint8_t rx_buf[PRL_MAX_PACKET_SIZE];
    size_t rx_len;
    int ret;
    
    /* 1. 编码请求 */
    int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                         NULL, 0, tx_buf, sizeof(tx_buf),
                         PRL_FLAG_ACK_REQUIRED);
    if (len < 0) {
        return OSAL_ERR_GENERIC;
    }
    
    /* 2. 发送请求 */
    ret = HAL_CAN_Send(handle->can_handle, tx_buf, len);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    /* 3. 接收响应 */
    ret = HAL_CAN_Recv(handle->can_handle, rx_buf, &rx_len, 1000);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    /* 4. 解码响应 */
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    
    ret = PRL_Decode(rx_buf, rx_len, &dev_type, &msg_type,
                     &payload, &payload_len);
    if (ret != PRL_OK) {
        return OSAL_ERR_GENERIC;
    }
    
    /* 5. 验证响应 */
    if (dev_type != PRL_DEV_TYPE_MCU ||
        msg_type != PRL_MCU_MSG_GET_VERSION ||
        payload_len != sizeof(prl_mcu_version_t)) {
        return OSAL_ERR_GENERIC;
    }
    
    /* 6. 复制数据 */
    memcpy(version, payload, sizeof(prl_mcu_version_t));
    
    return OSAL_SUCCESS;
}
```

## 5. 扩展指南

### 5.1 添加新设备类型

1. 在 `prl_common.h` 中添加设备类型枚举：
```c
typedef enum {
    ...
    PRL_DEV_TYPE_NEW_DEVICE = 0x08,     /* 新设备 */
} prl_dev_type_t;
```

2. 在 `prl_device.h` 中定义消息类型：
```c
typedef enum {
    PRL_NEW_MSG_HEARTBEAT   = 0x01,
    PRL_NEW_MSG_STATUS      = 0x02,
    ...
} prl_new_msg_type_t;
```

3. 定义消息结构体：
```c
typedef struct {
    uint32_t field1;
    uint16_t field2;
    ...
} prl_new_status_t;
```

4. 在 PDL 层实现设备驱动，调用 PRL API 进行编解码

### 5.2 添加新消息类型

1. 在设备的消息类型枚举中添加：
```c
typedef enum {
    ...
    PRL_MCU_MSG_NEW_COMMAND = 0x10,
} prl_mcu_msg_type_t;
```

2. 定义消息结构体：
```c
typedef struct {
    uint32_t param1;
    uint32_t param2;
} prl_mcu_new_command_t;
```

3. 在 PDL 层实现对应的业务函数

## 6. 性能优化

### 6.1 零拷贝设计

解码时直接返回报文内部的指针，避免内存拷贝：
```c
const uint8_t *payload;  /* 指向 packet 内部，无需额外分配 */
PRL_Decode(packet, len, &dev_type, &msg_type, &payload, &payload_len);
```

### 6.2 CRC 查表加速

使用预计算的 CRC16 查找表，避免运行时计算：
```c
static const uint16_t crc16_table[256] = { ... };
```

### 6.3 原子操作

序列号使用原子操作，线程安全且无锁：
```c
uint32_t prl_get_next_seq(void)
{
    return __sync_fetch_and_add(&g_seq_number, 1);
}
```

## 7. 错误处理

### 7.1 错误码定义

```c
typedef enum {
    PRL_OK                  = 0,    /* 成功 */
    PRL_ERR_INVALID_PARAM   = -1,   /* 无效参数 */
    PRL_ERR_INVALID_MAGIC   = -2,   /* 无效魔数 */
    PRL_ERR_INVALID_VERSION = -3,   /* 无效版本 */
    PRL_ERR_INVALID_TYPE    = -4,   /* 无效消息类型 */
    PRL_ERR_INVALID_LENGTH  = -5,   /* 无效长度 */
    PRL_ERR_CRC_FAILED      = -6,   /* CRC 校验失败 */
    PRL_ERR_BUFFER_TOO_SMALL = -7,  /* 缓冲区太小 */
    PRL_ERR_ENCODE_FAILED   = -8,   /* 编码失败 */
    PRL_ERR_DECODE_FAILED   = -9,   /* 解码失败 */
    PRL_ERR_INVALID_DEV_TYPE = -10, /* 无效设备类型 */
} prl_error_t;
```

### 7.2 错误处理示例

```c
int len = PRL_Encode(...);
if (len < 0) {
    LOG_ERROR("PRL", "Encode failed: %s", PRL_GetErrorString(len));
    return OSAL_ERR_GENERIC;
}
```

## 8. 注意事项

### 8.1 字节序

**当前实现**：使用主机字节序（Host Byte Order）

**跨平台通信**：如果需要在不同字节序的平台间通信，需要：
1. 在协议头中添加字节序标志
2. 使用网络字节序（Big Endian）
3. 提供字节序转换函数

### 8.2 结构体对齐

所有协议结构体使用 `__attribute__((packed))` 避免填充：
```c
typedef struct {
    ...
} __attribute__((packed)) prl_header_t;
```

### 8.3 缓冲区大小

编码时确保缓冲区足够大：
```c
uint8_t buffer[PRL_MAX_PACKET_SIZE];  /* 推荐使用最大值 */
```

### 8.4 CRC 校验

发送前必须调用 `prl_set_packet_crc()`，接收后必须验证 CRC。

## 9. 测试

### 9.1 单元测试

```c
void test_encode_decode(void)
{
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    prl_mcu_version_t version_in = {1, 2, 3};
    
    /* 编码 */
    int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                         &version_in, sizeof(version_in),
                         buffer, sizeof(buffer), 0);
    assert(len > 0);
    
    /* 解码 */
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    
    int ret = PRL_Decode(buffer, len, &dev_type, &msg_type,
                         &payload, &payload_len);
    assert(ret == PRL_OK);
    assert(dev_type == PRL_DEV_TYPE_MCU);
    assert(msg_type == PRL_MCU_MSG_GET_VERSION);
    assert(payload_len == sizeof(prl_mcu_version_t));
    
    /* 验证数据 */
    const prl_mcu_version_t *version_out = (const prl_mcu_version_t *)payload;
    assert(version_out->major == 1);
    assert(version_out->minor == 2);
    assert(version_out->patch == 3);
}
```

### 9.2 性能测试

```c
void test_performance(void)
{
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t payload[1024];
    
    uint64_t start = get_time_us();
    
    for (int i = 0; i < 10000; i++) {
        PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                   payload, sizeof(payload),
                   buffer, sizeof(buffer), 0);
    }
    
    uint64_t end = get_time_us();
    
    printf("Encode 10000 packets: %llu us\n", end - start);
    printf("Average: %.2f us/packet\n", (end - start) / 10000.0);
}
```

## 10. 未来扩展

### 10.1 加密支持

在协议头中预留了 `PRL_FLAG_ENCRYPTED` 标志位，未来可以添加：
- AES-128/256 加密
- 密钥协商机制
- 加密上下文管理

### 10.2 压缩支持

在协议头中预留了 `PRL_FLAG_COMPRESSED` 标志位，未来可以添加：
- LZ4 快速压缩
- ZLIB 高压缩比
- 自适应压缩策略

### 10.3 消息路由

可以添加消息路由层，支持：
- 消息订阅/发布
- 消息队列
- 异步回调
- 消息过滤

---

**文档版本**：v1.0  
**最后更新**：2026-06-01  
**维护者**：wanguo
