# EMS 统一设备协议架构文档

## 1. 概述

EMS（Equipment Management System）采用统一的设备协议架构，所有设备（MCU、CCM、PMC、GSC、SATELLITE、POWER、BMC）共用相同的协议栈，通过设备类型字段（`dev_type`）进行区分。

### 1.1 设计目标

- **统一性**：所有设备使用相同的协议头和编解码逻辑
- **可扩展性**：新增设备只需添加设备类型枚举值
- **跨平台**：协议层只依赖 OSAL，支持 Linux 和 RTOS
- **传输无关**：协议层不绑定特定传输介质
- **互操作性**：任意两个设备之间都可以建立通信

### 1.2 架构分层

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (ACL)                          │
│              业务功能与物理外设的逻辑映射                  │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                  外设驱动层 (PDL)                        │
│         设备管理、状态监控、业务接口封装                   │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   协议层 (PRL)                           │
│         统一设备协议、消息编解码、CRC 校验                 │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                  硬件抽象层 (HAL)                        │
│         传输介质适配（TCP/UDP/CAN/UART/I2C/SPI）         │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│              操作系统抽象层 (OSAL)                       │
│         跨平台系统调用封装（Linux/RTOS）                  │
└─────────────────────────────────────────────────────────┘
```

## 2. 协议头格式

### 2.1 协议头结构

```c
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint8_t  version;       /* 协议版本 */
    uint8_t  dev_type;      /* 设备类型 */
    uint8_t  msg_type;      /* 消息类型 */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 数据长度（不包括头） */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳（秒） */
    uint16_t crc16;         /* CRC16 校验 */
    uint16_t reserved;      /* 保留字段 */
} __attribute__((packed)) prl_header_t;
```

**总大小**：20 字节

### 2.2 字段说明

| 字段 | 类型 | 大小 | 说明 |
|------|------|------|------|
| magic | uint16_t | 2 | 魔数，固定为 0xAA55，用于帧同步 |
| version | uint8_t | 1 | 协议版本，高4位主版本，低4位次版本 |
| dev_type | uint8_t | 1 | 设备类型，区分不同设备 |
| msg_type | uint8_t | 1 | 消息类型，设备特定的消息编号 |
| flags | uint8_t | 1 | 标志位，如 ACK_REQUIRED、IS_ACK 等 |
| length | uint16_t | 2 | 负载数据长度，不包括协议头 |
| seq | uint32_t | 4 | 序列号，用于消息跟踪和去重 |
| timestamp | uint32_t | 4 | 时间戳（秒），消息发送时间 |
| crc16 | uint16_t | 2 | CRC16 校验，覆盖整个报文 |
| reserved | uint16_t | 2 | 保留字段，用于未来扩展 |

### 2.3 报文格式

```
┌──────────────┬──────────────┬──────────────┐
│  协议头 (20) │  负载数据 (N) │              │
│  prl_header_t│   payload    │              │
└──────────────┴──────────────┴──────────────┘
 <------------- CRC16 覆盖范围 -------------->
```

## 3. 设备类型定义

### 3.1 设备类型枚举

```c
typedef enum {
    PRL_DEV_TYPE_UNKNOWN    = 0x00,     /* 未知设备 */
    PRL_DEV_TYPE_MCU        = 0x01,     /* 微控制器 */
    PRL_DEV_TYPE_CCM        = 0x02,     /* 通信管理板 */
    PRL_DEV_TYPE_PMC        = 0x03,     /* 载荷管理器 */
    PRL_DEV_TYPE_GSC        = 0x04,     /* 地面站控制器 */
    PRL_DEV_TYPE_SATELLITE  = 0x05,     /* 卫星平台 */
    PRL_DEV_TYPE_POWER      = 0x06,     /* 电源板 */
    PRL_DEV_TYPE_BMC        = 0x07,     /* 基板管理控制器 */
} prl_dev_type_t;
```

### 3.2 设备说明

| 设备类型 | 编号 | 说明 | 通信方式 |
|---------|------|------|---------|
| MCU | 0x01 | 微控制器，用于外设控制 | CAN/UART/I2C/SPI |
| CCM | 0x02 | 通信管理板，负责卫星通信 | 以太网/CAN |
| PMC | 0x03 | 载荷管理器，管理载荷设备 | 万兆以太网 |
| GSC | 0x04 | 地面站控制器，地面控制端 | 万兆以太网 |
| SATELLITE | 0x05 | 卫星平台，提供轨道姿态数据 | CAN 总线 |
| POWER | 0x06 | 电源板，电源管理 | CAN 总线 |
| BMC | 0x07 | 基板管理控制器，服务器管理 | 以太网（Redfish/IPMI） |

## 4. 消息类型定义

### 4.1 命名规则

消息类型使用**设备前缀命名**（方案B）：

- MCU 消息：`PRL_MCU_MSG_*`
- CCM 消息：`PRL_CCM_MSG_*`
- PMC 消息：`PRL_PMC_MSG_*`
- GSC 消息：`PRL_GSC_MSG_*`
- SATELLITE 消息：`PRL_SAT_MSG_*`
- POWER 消息：`PRL_POWER_MSG_*`
- BMC 消息：`PRL_BMC_MSG_*`

### 4.2 MCU 消息类型

```c
typedef enum {
    PRL_MCU_MSG_GET_VERSION     = 0x01,     /* 获取版本信息 */
    PRL_MCU_MSG_GET_STATUS      = 0x02,     /* 获取状态 */
    PRL_MCU_MSG_RESET           = 0x03,     /* 复位 */
    PRL_MCU_MSG_HEARTBEAT       = 0x04,     /* 心跳 */
    PRL_MCU_MSG_SET_CONFIG      = 0x05,     /* 设置配置 */
    PRL_MCU_MSG_GET_CONFIG      = 0x06,     /* 获取配置 */
    PRL_MCU_MSG_POWER_ON        = 0x20,     /* 上电 */
    PRL_MCU_MSG_POWER_OFF       = 0x21,     /* 下电 */
    PRL_MCU_MSG_CUSTOM          = 0xFF,     /* 自定义命令 */
} prl_mcu_msg_type_t;
```

### 4.3 CCM 消息类型

```c
typedef enum {
    PRL_CCM_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_CCM_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_CCM_MSG_COMMAND         = 0x03,     /* 遥控指令 */
    PRL_CCM_MSG_TIME_SYNC       = 0x04,     /* 时间同步 */
    PRL_CCM_MSG_ORBIT_DATA      = 0x05,     /* 轨道数据 */
    PRL_CCM_MSG_ATTITUDE_DATA   = 0x06,     /* 姿态数据 */
    PRL_CCM_MSG_POWER_STATUS    = 0x07,     /* 电源状态 */
    PRL_CCM_MSG_THERMAL_STATUS  = 0x08,     /* 热控状态 */
    PRL_CCM_MSG_ACK             = 0xFF,     /* 应答 */
} prl_ccm_msg_type_t;
```

### 4.4 PMC 消息类型

```c
typedef enum {
    PRL_PMC_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_PMC_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_PMC_MSG_COMMAND         = 0x03,     /* 遥控指令 */
    PRL_PMC_MSG_FIRMWARE_UPDATE = 0x04,     /* 固件升级 */
    PRL_PMC_MSG_NODE_MANAGE     = 0x05,     /* 节点管理 */
    PRL_PMC_MSG_POWER_CONTROL   = 0x06,     /* 电源控制 */
    PRL_PMC_MSG_STATUS_QUERY    = 0x07,     /* 状态查询 */
    PRL_PMC_MSG_ACK             = 0xFF,     /* 应答 */
} prl_pmc_msg_type_t;
```

### 4.5 通用消息约定

所有设备都包含以下通用消息：

- **心跳消息**（0x01）：用于保活和链路质量检测
- **应答消息**（0xFF）：用于确认和错误反馈

## 5. 编解码接口

### 5.1 统一编码接口

```c
int prl_device_encode(uint8_t dev_type, uint8_t msg_type,
                      const void *payload, uint16_t payload_len,
                      uint8_t *buffer, size_t buffer_size, uint8_t flags);
```

**参数说明**：
- `dev_type`：设备类型（PRL_DEV_TYPE_*）
- `msg_type`：消息类型（设备特定）
- `payload`：负载数据指针
- `payload_len`：负载数据长度
- `buffer`：输出缓冲区
- `buffer_size`：缓冲区大小
- `flags`：标志位（如 PRL_FLAG_ACK_REQUIRED）

**返回值**：
- 成功：返回编码后的总长度（> 0）
- 失败：返回负数错误码

### 5.2 统一解码接口

```c
int prl_device_decode(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type, uint8_t *msg_type,
                      const uint8_t **payload, uint16_t *payload_len);
```

**参数说明**：
- `packet`：报文数据
- `packet_len`：报文长度
- `dev_type`：输出设备类型
- `msg_type`：输出消息类型
- `payload`：输出负载数据指针（指向 packet 内部）
- `payload_len`：输出负载长度

**返回值**：
- 成功：返回 PRL_OK (0)
- 失败：返回负数错误码

## 6. 使用示例

### 6.1 编码示例

```c
/* MCU 心跳消息 */
uint8_t buffer[256];
int ret;

ret = prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                        NULL, 0, buffer, sizeof(buffer), 0);
if (ret > 0) {
    /* 发送 buffer，长度为 ret */
    send_to_transport(buffer, ret);
}
```

### 6.2 解码示例

```c
/* 接收并解码消息 */
uint8_t packet[256];
uint8_t dev_type, msg_type;
const uint8_t *payload;
uint16_t payload_len;
int ret;

/* 从传输层接收数据 */
int len = recv_from_transport(packet, sizeof(packet));

/* 解码消息 */
ret = prl_device_decode(packet, len, &dev_type, &msg_type,
                        &payload, &payload_len);
if (ret == PRL_OK) {
    /* 根据设备类型和消息类型处理 */
    if (dev_type == PRL_DEV_TYPE_MCU) {
        switch (msg_type) {
            case PRL_MCU_MSG_HEARTBEAT:
                handle_mcu_heartbeat(payload, payload_len);
                break;
            case PRL_MCU_MSG_GET_STATUS:
                handle_mcu_status(payload, payload_len);
                break;
        }
    }
}
```

### 6.3 PDL 层使用示例

```c
/* PDL_MCU 获取版本 */
int32_t PDL_MCU_GetVersion(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len, ret;
    uint32_t actual_size;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_get_version(tx_buf, sizeof(tx_buf));
    if (tx_len < 0) {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求并接收响应 */
    ret = mcu_send_command_internal(ctx, MCU_CMD_GET_VERSION,
                                    tx_buf, tx_len,
                                    rx_buf, sizeof(rx_buf), &actual_size);

    if (ret == OSAL_SUCCESS) {
        /* 使用 PRL 协议解码响应 */
        ret = pdl_mcu_decode_get_version(rx_buf, actual_size, version);
    }

    return ret;
}
```

## 7. CRC 校验

### 7.1 CRC 算法

使用 **CRC16-CCITT** 算法：
- 多项式：0x1021
- 初始值：0xFFFF
- 覆盖范围：整个报文（协议头 + 负载）
- CRC 字段在计算时置为 0

### 7.2 CRC 设置

```c
void prl_set_packet_crc(uint8_t *packet, size_t total_len);
```

### 7.3 CRC 验证

```c
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);
```

## 8. 错误码定义

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

## 9. 扩展性设计

### 9.1 新增设备类型

1. 在 `prl_dev_type_t` 中添加新的设备类型枚举
2. 在 `prl_device.h` 中定义设备的消息类型
3. 在 `prl_device.c` 中更新设备类型名称表
4. 无需修改协议编解码逻辑

### 9.2 新增消息类型

1. 在对应设备的消息类型枚举中添加新消息
2. 在 PDL 层实现消息的编解码封装
3. 无需修改 PRL 层的通用编解码接口

### 9.3 设备间互联

任意两个设备之间都可以建立通信：
- 使用统一的协议头和编解码逻辑
- 通过 HAL 层适配不同的传输介质
- 通过 ACL 和 PCL 配置通信链路

## 10. 性能考虑

### 10.1 协议开销

- 协议头：20 字节（固定）
- 最小报文：20 字节（无负载）
- 最大报文：20 + 4096 = 4116 字节

### 10.2 CRC 计算

- 使用查表法加速 CRC 计算
- 时间复杂度：O(n)，n 为报文长度
- 空间复杂度：256 * 2 = 512 字节（CRC 表）

### 10.3 零拷贝设计

- 解码时返回指向原始报文的指针
- 避免不必要的内存拷贝
- 适用于高性能场景

## 11. 安全考虑

### 11.1 数据完整性

- CRC16 校验覆盖整个报文
- 检测传输错误和数据篡改

### 11.2 版本兼容性

- 协议版本号支持主版本和次版本
- 主版本不兼容时拒绝解码
- 次版本兼容时可以解码

### 11.3 序列号

- 用于消息跟踪和去重
- 防止重放攻击

## 12. 测试

### 12.1 单元测试

- `test_prl_common.c`：通用功能测试
- `test_prl_device.c`：设备协议测试

### 12.2 测试覆盖

- 所有设备类型
- 正常流程和异常流程
- 边界条件（零负载、最大负载）
- 数据完整性（CRC 校验）
- 参数验证

## 13. 参考文件

### 13.1 协议层

- `core/prl/include/prl_common.h` - 协议通用定义
- `core/prl/include/prl_device.h` - 设备协议定义
- `core/prl/src/prl_common.c` - 协议通用实现
- `core/prl/src/prl_device.c` - 设备协议实现

### 13.2 驱动层

- `core/pdl/src/pdl_mcu/pdl_mcu_protocol.h` - MCU 协议封装
- `core/pdl/src/pdl_mcu/pdl_mcu_protocol_new.c` - MCU 协议实现

### 13.3 测试

- `tests/unit/prl/test_prl_common.c` - 通用功能测试
- `tests/unit/prl/test_prl_device.c` - 设备协议测试

## 14. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024-05 | 初始版本，统一设备协议架构 |

---

**文档维护者**：EMS 开发团队  
**最后更新**：2024-05-30
