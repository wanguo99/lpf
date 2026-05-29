# PRL (Protocol Layer) - 协议层

## 概述

PRL (Protocol Layer) 是 EMS SDK 的协议层，位于 Core 层，为不同产品之间的通信提供统一的协议定义和编解码功能。

## 架构定位

```
Products (GSC, PMC, CCM)
  ↓ 调用协议接口
PRL (协议层) ← Core 层
  ├── GSC-PMC 协议
  ├── PMC-CCM 协议
  ├── CCM-Satellite 协议
  └── CCM-Power 协议
  ↓ 使用传输层
OSAL/HAL (网络、CAN 传输)
```

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
