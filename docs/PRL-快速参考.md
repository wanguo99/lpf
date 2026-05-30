# PRL 协议层快速参考

## 设备类型

| 设备 | 枚举值 | 说明 |
|------|--------|------|
| MCU | `PRL_DEV_TYPE_MCU` (0x01) | 微控制器 |
| CCM | `PRL_DEV_TYPE_CCM` (0x02) | 通信管理板 |
| PMC | `PRL_DEV_TYPE_PMC` (0x03) | 载荷管理器 |
| GSC | `PRL_DEV_TYPE_GSC` (0x04) | 地面站控制器 |
| SATELLITE | `PRL_DEV_TYPE_SATELLITE` (0x05) | 卫星平台 |
| POWER | `PRL_DEV_TYPE_POWER` (0x06) | 电源板 |
| BMC | `PRL_DEV_TYPE_BMC` (0x07) | 基板管理控制器 |

## 协议头结构（20字节）

```c
typedef struct {
    uint16_t magic;         /* 0xAA55 */
    uint8_t  version;       /* 协议版本 */
    uint8_t  dev_type;      /* 设备类型 */
    uint8_t  msg_type;      /* 消息类型 */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 负载长度 */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳 */
    uint16_t crc16;         /* CRC16 */
    uint16_t reserved;      /* 保留 */
} prl_header_t;
```

## 常用消息类型

### MCU
- `PRL_MCU_MSG_GET_VERSION` (0x01) - 获取版本
- `PRL_MCU_MSG_GET_STATUS` (0x02) - 获取状态
- `PRL_MCU_MSG_RESET` (0x03) - 复位
- `PRL_MCU_MSG_HEARTBEAT` (0x04) - 心跳

### CCM
- `PRL_CCM_MSG_HEARTBEAT` (0x01) - 心跳
- `PRL_CCM_MSG_TELEMETRY` (0x02) - 遥测
- `PRL_CCM_MSG_COMMAND` (0x03) - 遥控
- `PRL_CCM_MSG_ACK` (0xFF) - 应答

### PMC
- `PRL_PMC_MSG_HEARTBEAT` (0x01) - 心跳
- `PRL_PMC_MSG_TELEMETRY` (0x02) - 遥测
- `PRL_PMC_MSG_COMMAND` (0x03) - 遥控
- `PRL_PMC_MSG_ACK` (0xFF) - 应答

## 编解码接口

### 编码

```c
int prl_device_encode(uint8_t dev_type, uint8_t msg_type,
                      const void *payload, uint16_t payload_len,
                      uint8_t *buffer, size_t buffer_size, uint8_t flags);
```

**返回值**：成功返回编码后的长度（> 0），失败返回负数错误码

### 解码

```c
int prl_device_decode(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type, uint8_t *msg_type,
                      const uint8_t **payload, uint16_t *payload_len);
```

**返回值**：成功返回 `PRL_OK` (0)，失败返回负数错误码

## 使用示例

### 发送心跳

```c
uint8_t buffer[256];
int ret;

ret = prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                        NULL, 0, buffer, sizeof(buffer), 0);
if (ret > 0) {
    send_data(buffer, ret);
}
```

### 接收并解析

```c
uint8_t packet[256];
uint8_t dev_type, msg_type;
const uint8_t *payload;
uint16_t payload_len;

int len = recv_data(packet, sizeof(packet));
int ret = prl_device_decode(packet, len, &dev_type, &msg_type,
                             &payload, &payload_len);
if (ret == PRL_OK) {
    // 处理消息
}
```

## 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `PRL_OK` | 0 | 成功 |
| `PRL_ERR_INVALID_PARAM` | -1 | 无效参数 |
| `PRL_ERR_INVALID_MAGIC` | -2 | 无效魔数 |
| `PRL_ERR_INVALID_VERSION` | -3 | 无效版本 |
| `PRL_ERR_CRC_FAILED` | -6 | CRC 校验失败 |
| `PRL_ERR_BUFFER_TOO_SMALL` | -7 | 缓冲区太小 |
| `PRL_ERR_INVALID_DEV_TYPE` | -10 | 无效设备类型 |

## 关键文件

- `core/prl/include/prl_common.h` - 协议通用定义
- `core/prl/include/prl_device.h` - 设备协议定义
- `core/prl/src/prl_device.c` - 设备协议实现
