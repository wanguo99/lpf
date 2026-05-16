# PDL层详细设计

## 6. PDL 设计（外设驱动层）

### 6.1 设计原则

**PDL（Peripheral Driver Layer）是外设驱动层，负责封装各类外设的通信协议和业务逻辑。**

#### 6.1.1 独立外设服务设计

**核心理念**：每种外设（Satellite/BMC/MCU）完全独立设计，各自暴露专属API，不强行抽象成统一接口。

**为什么不设计统一外设接口？**

```c
// ❌ 错误设计：统一外设接口（最小公分母陷阱）
typedef struct {
    int32_t (*init)(void *config);
    int32_t (*read)(void *handle, void *data);
    int32_t (*write)(void *handle, void *data);
    int32_t (*control)(void *handle, uint32_t cmd, void *arg);
} peripheral_ops_t;

// 问题：
// 1. BMC需要双通道切换，但Satellite不需要
// 2. MCU需要固件升级，但BMC不需要
// 3. Satellite需要心跳机制，但MCU不需要
// 4. 所有特殊功能都要塞进control()，变成万能接口
```

**✅ 正确设计：独立外设服务**

```c
// Satellite服务（卫星平台通信）
int32_t PDL_Satellite_Init(const satellite_service_config_t *config, satellite_service_handle_t *handle);
int32_t PDL_Satellite_SendResponse(satellite_service_handle_t handle, uint32_t seq_num, can_status_t status, uint32_t result);
int32_t PDL_Satellite_SendHeartbeat(satellite_service_handle_t handle, can_status_t status);

// BMC服务（载荷服务器管理）
int32_t PDL_BMC_Init(const bmc_config_t *config, bmc_handle_t *handle);
int32_t PDL_BMC_PowerOn(bmc_handle_t handle);
int32_t PDL_BMC_ReadSensors(bmc_handle_t handle, bmc_sensor_type_t type, bmc_sensor_reading_t *readings, uint32_t max_count, uint32_t *actual_count);
int32_t PDL_BMC_SwitchChannel(bmc_handle_t handle, bmc_channel_t channel);

// MCU服务（微控制器通信）
int32_t PDL_MCU_Init(const mcu_config_t *config, mcu_handle_t *handle);
int32_t PDL_MCU_GetVersion(mcu_handle_t handle, mcu_version_t *version);
int32_t PDL_MCU_FirmwareUpdate(mcu_handle_t handle, const char *firmware_path, void (*progress_callback)(uint32_t percent));
```

**优势**：
- ✅ 每个API完全贴合外设特性，无冗余参数
- ✅ 类型安全，编译期检查
- ✅ 代码清晰，无需运行时类型判断
- ✅ 易于扩展，新增外设不影响现有代码

#### 6.1.2 ACL层配置映射

**APP层通过ACL配置表指定具体外设类型和索引**：

```c
// ACL配置：遥测项映射到具体外设
typedef struct {
    acl_device_type_t device_type;  // 外设类型（SATELLITE/BMC/MCU）
    uint32_t          device_index; // 外设索引（第几个BMC/MCU）
    uint32_t          logic_index;  // 逻辑索引（PDL层句柄索引）
} acl_tm_device_mapping_t;

// APP层根据配置调用对应PDL接口
acl_tm_device_mapping_t *cfg = ACL_GetTelemetryMapping(TM_CPU_TEMP);
switch (cfg->device_type) {
    case ACL_DEVICE_BMC:
        PDL_BMC_ReadSensors(cfg->logic_index, BMC_SENSOR_TEMP, &data, 1, &count);
        break;
    case ACL_DEVICE_MCU:
        PDL_MCU_GetStatus(cfg->logic_index, &status);
        break;
}
```

#### 6.1.3 协议封装与通道管理

**PDL层职责**：
- ✅ 封装通信协议（Redfish/IPMI/CAN协议）
- ✅ 管理通信通道（主备通道、自动切换）
- ✅ 实现重试机制和超时处理
- ✅ 提供统计信息和健康检查

**PDL层禁止**：
- ❌ 直接操作硬件寄存器（应调用HAL层）
- ❌ 包含业务逻辑（业务逻辑在APP层）
- ❌ 设计统一外设抽象接口

---

### 6.2 Satellite服务设计

**功能**：封装卫星平台通信协议，处理遥控命令接收和遥测响应发送。

#### 6.2.1 接口设计

```c
/* 卫星平台服务句柄 */
typedef void* satellite_service_handle_t;

/* 卫星平台服务配置 */
typedef struct {
    const char *can_device;           /* CAN设备名 */
    uint32_t    can_bitrate;          /* CAN波特率 */
    uint32_t    heartbeat_interval_ms;/* 心跳间隔(ms) */
    uint32_t    cmd_timeout_ms;       /* 命令超时(ms) */
} satellite_service_config_t;

/* 命令回调函数类型 */
typedef void (*satellite_cmd_callback_t)(uint8_t cmd_type, uint32_t param, void *user_data);

/* 初始化卫星平台服务 */
int32_t PDL_Satellite_Init(const satellite_service_config_t *config, satellite_service_handle_t *handle);

/* 注册命令回调函数 */
int32_t PDL_Satellite_RegisterCallback(satellite_service_handle_t handle, satellite_cmd_callback_t callback, void *user_data);

/* 发送响应到卫星平台 */
int32_t PDL_Satellite_SendResponse(satellite_service_handle_t handle, uint32_t seq_num, can_status_t status, uint32_t result);

/* 发送心跳到卫星平台 */
int32_t PDL_Satellite_SendHeartbeat(satellite_service_handle_t handle, can_status_t status);

/* 获取服务统计信息 */
int32_t PDL_Satellite_GetStats(satellite_service_handle_t handle, uint32_t *rx_count, uint32_t *tx_count, uint32_t *error_count);
```

#### 6.2.2 设计特点

- **CAN协议封装**：封装CAN帧格式、序列号管理、CRC校验
- **命令接收**：后台线程接收CAN命令，通过回调通知APP层
- **心跳机制**：周期性发送心跳帧，维持与卫星平台的连接
- **统计信息**：记录收发计数、错误计数，用于健康监控

---

### 6.3 BMC服务设计

**功能**：与带BMC的载荷服务器通信，支持IPMI/Redfish协议，实现电源控制、状态查询、传感器读取。

#### 6.3.1 接口设计

```c
/* BMC服务句柄 */
typedef void* bmc_handle_t;

/* 通信通道类型 */
typedef enum {
    BMC_CHANNEL_NETWORK = 0,  /* 网络通道（IPMI over LAN/Redfish） */
    BMC_CHANNEL_SERIAL  = 1   /* 串口通道（IPMI over Serial） */
} bmc_channel_t;

/* BMC协议类型 */
typedef enum {
    BMC_PROTOCOL_IPMI    = 0, /* IPMI协议 */
    BMC_PROTOCOL_REDFISH = 1  /* Redfish协议 */
} bmc_protocol_t;

/* BMC配置 */
typedef struct {
    /* 网络配置 */
    struct {
        bool        enabled;      /* 是否启用 */
        const char *ip_addr;      /* IP地址 */
        uint16_t    port;         /* 端口（默认623） */
        const char *username;     /* 用户名 */
        const char *password;     /* 密码 */
        uint32_t    timeout_ms;   /* 超时时间 */
    } network;

    /* 串口配置 */
    struct {
        bool        enabled;      /* 是否启用 */
        const char *device;       /* 串口设备 */
        uint32_t    baudrate;     /* 波特率 */
        uint32_t    timeout_ms;   /* 超时时间 */
    } serial;

    /* 服务配置 */
    bmc_channel_t primary_channel;      /* 主通道 */
    bool          auto_switch;          /* 自动切换通道 */
    uint32_t      retry_count;          /* 重试次数 */
    uint32_t      health_check_interval;/* 健康检查间隔(ms) */
} bmc_config_t;

/* 初始化BMC服务 */
int32_t PDL_BMC_Init(const bmc_config_t *config, bmc_handle_t *handle);

/* 电源控制 */
int32_t PDL_BMC_PowerOn(bmc_handle_t handle);
int32_t PDL_BMC_PowerOff(bmc_handle_t handle);
int32_t PDL_BMC_PowerReset(bmc_handle_t handle);
int32_t PDL_BMC_GetPowerState(bmc_handle_t handle, bmc_power_state_t *state);

/* 传感器读取 */
int32_t PDL_BMC_ReadSensors(bmc_handle_t handle, bmc_sensor_type_t type, bmc_sensor_reading_t *readings, uint32_t max_count, uint32_t *actual_count);

/* 通道管理 */
int32_t PDL_BMC_SwitchChannel(bmc_handle_t handle, bmc_channel_t channel);
bmc_channel_t PDL_BMC_GetChannel(bmc_handle_t handle);
bool PDL_BMC_IsConnected(bmc_handle_t handle);

/* 统计信息 */
int32_t PDL_BMC_GetStats(bmc_handle_t handle, uint32_t *cmd_count, uint32_t *success_count, uint32_t *fail_count, uint32_t *switch_count);
```

#### 6.3.2 设计特点

- **双通道冗余**：主通道（网络）+ 备份通道（串口），自动故障切换
- **协议支持**：IPMI（成熟稳定）+ Redfish（现代化RESTful API）
- **自动切换**：主通道连续失败达到阈值后自动切换到备份通道
- **健康检查**：后台线程周期性检查连接状态，记录通道切换次数

---

### 6.4 MCU服务设计

**功能**：与MCU通信，支持CAN/串口/I2C/SPI多种接口，实现状态查询、寄存器读写、固件升级。

#### 6.4.1 接口设计

```c
/* MCU服务句柄 */
typedef void* mcu_handle_t;

/* MCU通信接口类型 */
typedef enum {
    MCU_INTERFACE_CAN    = 0, /* CAN总线 */
    MCU_INTERFACE_SERIAL = 1, /* 串口 */
    MCU_INTERFACE_I2C    = 2, /* I2C（预留） */
    MCU_INTERFACE_SPI    = 3  /* SPI（预留） */
} mcu_interface_t;

/* MCU配置 */
typedef struct {
    char              name[64];       /* MCU名称 */
    mcu_interface_t   interface;      /* 通信接口 */

    /* CAN配置 */
    struct {
        const char *device;           /* CAN设备（如can0） */
        uint32_t    bitrate;          /* 波特率 */
        uint32_t    tx_id;            /* 发送CAN ID */
        uint32_t    rx_id;            /* 接收CAN ID */
    } can;

    /* 串口配置 */
    struct {
        const char *device;           /* 串口设备（如/dev/ttyS1） */
        uint32_t    baudrate;         /* 波特率 */
        uint8_t     data_bits;        /* 数据位（5-8） */
        uint8_t     stop_bits;        /* 停止位（1-2） */
        int8_t      parity;           /* 校验位（'N'/'E'/'O'） */
    } serial;

    /* 通用配置 */
    uint32_t cmd_timeout_ms;          /* 命令超时（ms） */
    uint32_t retry_count;             /* 重试次数 */
    bool     enable_crc;              /* 启用CRC校验 */
} mcu_config_t;

/* 初始化MCU驱动 */
int32_t PDL_MCU_Init(const mcu_config_t *config, mcu_handle_t *handle);

/* 版本和状态查询 */
int32_t PDL_MCU_GetVersion(mcu_handle_t handle, mcu_version_t *version);
int32_t PDL_MCU_GetStatus(mcu_handle_t handle, mcu_status_t *status);

/* 控制操作 */
int32_t PDL_MCU_Reset(mcu_handle_t handle);
int32_t PDL_MCU_ReadRegister(mcu_handle_t handle, uint8_t reg_addr, uint8_t *value);
int32_t PDL_MCU_WriteRegister(mcu_handle_t handle, uint8_t reg_addr, uint8_t value);

/* 自定义命令 */
int32_t PDL_MCU_SendCommand(mcu_handle_t handle, uint8_t cmd_code, const uint8_t *data, uint32_t data_len, uint8_t *response, uint32_t resp_size, uint32_t *actual_size);

/* 固件升级 */
int32_t PDL_MCU_FirmwareUpdate(mcu_handle_t handle, const char *firmware_path, void (*progress_callback)(uint32_t percent));
```

#### 6.4.2 设计特点

- **多接口支持**：CAN/串口/I2C/SPI，由配置决定使用哪种
- **协议封装**：封装MCU通信协议（帧格式、CRC校验、应答机制）
- **寄存器访问**：提供寄存器读写接口，用于低级控制
- **固件升级**：支持MCU固件在线升级，带进度回调

---

### 6.5 PDL层实现要点

#### 6.5.1 句柄管理

```c
/* 内部句柄结构（对外不透明） */
typedef struct {
    bool              in_use;
    mcu_config_t      config;
    hal_can_handle_t  can_handle;    /* HAL层CAN句柄 */
    hal_serial_handle_t serial_handle; /* HAL层串口句柄 */
    uint32_t          tx_count;
    uint32_t          rx_count;
    uint32_t          error_count;
} mcu_context_t;

static mcu_context_t mcu_contexts[MAX_MCU_COUNT];

/* 初始化时分配句柄 */
int32_t PDL_MCU_Init(const mcu_config_t *config, mcu_handle_t *handle) {
    /* 1. 查找空闲句柄 */
    mcu_context_t *ctx = find_free_context();
    
    /* 2. 根据接口类型初始化HAL层 */
    if (config->interface == MCU_INTERFACE_CAN) {
        hal_can_config_t can_cfg = {
            .interface = config->can.device,
            .baudrate = config->can.bitrate,
            .rx_timeout = config->cmd_timeout_ms,
            .tx_timeout = config->cmd_timeout_ms
        };
        HAL_CAN_Init(&can_cfg, &ctx->can_handle);
    }
    
    /* 3. 返回句柄 */
    *handle = (mcu_handle_t)ctx;
    return OSAL_SUCCESS;
}
```

#### 6.5.2 协议封装

```c
/* MCU命令帧格式（CAN接口） */
typedef struct {
    uint8_t  cmd_code;      /* 命令码 */
    uint8_t  seq_num;       /* 序列号 */
    uint8_t  data_len;      /* 数据长度 */
    uint8_t  data[5];       /* 数据（CAN最多8字节，减去头部3字节） */
    uint8_t  crc;           /* CRC校验（可选） */
} mcu_can_frame_t;

/* 发送命令并等待响应 */
static int32_t mcu_send_command_internal(mcu_context_t *ctx, uint8_t cmd_code, const uint8_t *data, uint32_t data_len, uint8_t *response, uint32_t *resp_len) {
    /* 1. 构造命令帧 */
    mcu_can_frame_t cmd_frame;
    cmd_frame.cmd_code = cmd_code;
    cmd_frame.seq_num = ctx->seq_num++;
    cmd_frame.data_len = data_len;
    OSAL_Memcpy(cmd_frame.data, data, data_len);
    if (ctx->config.enable_crc) {
        cmd_frame.crc = calculate_crc(&cmd_frame, sizeof(cmd_frame) - 1);
    }
    
    /* 2. 发送CAN帧 */
    can_frame_t can_frame;
    can_frame.can_id = ctx->config.can.tx_id;
    can_frame.can_dlc = sizeof(mcu_can_frame_t);
    OSAL_Memcpy(can_frame.data, &cmd_frame, sizeof(mcu_can_frame_t));
    HAL_CAN_Send(ctx->can_handle, &can_frame);
    
    /* 3. 接收响应帧 */
    HAL_CAN_Recv(ctx->can_handle, &can_frame, ctx->config.cmd_timeout_ms);
    
    /* 4. 解析响应 */
    mcu_can_frame_t *resp_frame = (mcu_can_frame_t *)can_frame.data;
    if (resp_frame->seq_num != cmd_frame.seq_num) {
        return OSAL_ERR_GENERIC; /* 序列号不匹配 */
    }
    OSAL_Memcpy(response, resp_frame->data, resp_frame->data_len);
    *resp_len = resp_frame->data_len;
    
    return OSAL_SUCCESS;
}
```

#### 6.5.3 重试机制

```c
/* 带重试的命令发送 */
int32_t PDL_MCU_SendCommand(mcu_handle_t handle, uint8_t cmd_code, const uint8_t *data, uint32_t data_len, uint8_t *response, uint32_t resp_size, uint32_t *actual_size) {
    mcu_context_t *ctx = (mcu_context_t *)handle;
    int32_t ret;
    
    for (uint32_t i = 0; i <= ctx->config.retry_count; i++) {
        ret = mcu_send_command_internal(ctx, cmd_code, data, data_len, response, actual_size);
        if (ret == OSAL_SUCCESS) {
            ctx->tx_count++;
            ctx->rx_count++;
            return OSAL_SUCCESS;
        }
        
        /* 重试前延迟 */
        if (i < ctx->config.retry_count) {
            OSAL_TaskDelay(10);
        }
    }
    
    ctx->error_count++;
    return ret;
}
```

---
