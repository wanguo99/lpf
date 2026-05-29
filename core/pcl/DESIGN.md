# PCL 配置层设计文档

## 设计原则

### 配置结构体分层定义

```
┌─────────────────────────────────────────┐
│  PCL (配置层)                            │
│  - 只负责配置的定义、读取、存储          │
│  - 直接使用 PDL 配置结构体              │
│  - 提供配置查询接口                      │
│  - 添加配置管理字段（name/enabled）     │
└─────────────────────────────────────────┘
              ↓ (零拷贝引用)
┌─────────────────────────────────────────┐
│  PDL (外设驱动层)                        │
│  - 定义外设级配置结构体                  │
│  - 包含业务逻辑配置 (重试、超时等)       │
│  - 直接嵌入 HAL 配置字段                │
└─────────────────────────────────────────┘
              ↓ (字段嵌入)
┌─────────────────────────────────────────┐
│  HAL (硬件抽象层)                        │
│  - 定义硬件接口配置结构体                │
│  - 纯硬件参数 (波特率、设备名等)         │
└─────────────────────────────────────────┘
```

### 核心优势

1. **零拷贝传递** - PCL → PDL → HAL 无需配置转换
2. **单一定义** - 配置结构只在 PDL/HAL 定义一次
3. **类型安全** - 编译期检查，避免转换错误
4. **易于维护** - 修改配置只需改 PDL/HAL 层

## 配置结构体定义

### HAL 层（硬件配置）

```c
// hal_can.h
typedef struct {
    const char *interface;  // "can0"
    uint32_t    baudrate;
    uint32_t    rx_timeout;
    uint32_t    tx_timeout;
} hal_can_config_t;

// hal_serial.h
typedef struct {
    uint32_t baud_rate;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    uint8_t  parity;
    uint8_t  flow_control;
} hal_serial_config_t;
```

### PDL 层（外设配置，嵌入 HAL 配置）

```c
// pdl_mcu.h
typedef struct {
    char                  name[64];
    pdl_mcu_interface_t   interface;

    /* CAN配置 - 直接嵌入 HAL 字段 */
    struct {
        const char *device;      // 传递给 HAL
        uint32_t    bitrate;     // 传递给 HAL
        uint32_t    rx_timeout;  // 传递给 HAL
        uint32_t    tx_timeout;  // 传递给 HAL
        uint32_t    tx_id;       // PDL 层使用
        uint32_t    rx_id;       // PDL 层使用
    } can;

    /* 串口配置 - 直接嵌入 HAL 字段 */
    struct {
        const char *device;      // 传递给 HAL
        uint32_t    baudrate;    // 传递给 HAL
        uint8_t     data_bits;   // 传递给 HAL
        uint8_t     stop_bits;   // 传递给 HAL
        uint8_t     parity;      // 传递给 HAL
        uint8_t     flow_control;// 传递给 HAL
    } serial;

    /* 业务配置 */
    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
    bool     enable_crc;
} pdl_mcu_config_t;
```

### PCL 层（配置容器，直接引用 PDL 配置）

```c
// pcl_mcu.h
typedef struct {
    /* PCL 配置管理字段 */
    const char *name;         // 用于查询
    const char *description;
    bool        enabled;

    /* PDL 配置（直接引用） */
    pdl_mcu_config_t config;  // 来自 pdl_mcu.h

    /* GPIO控制（可选） */
    pcl_gpio_config_t *reset_gpio;
    pcl_gpio_config_t *irq_gpio;
} pcl_mcu_entry_t;
```

## 使用流程

### 1. 定义配置（PCL 层）

```c
// 定义 MCU 配置
static pcl_mcu_entry_t power_mcu_entry = {
    .name = "power_mcu",
    .description = "Power management MCU",
    .enabled = true,
    .config = {
        .name = "PowerMCU",
        .interface = PDL_MCU_INTERFACE_CAN,
        .can = {
            .device = "can0",
            .bitrate = 500000,
            .rx_timeout = 1000,
            .tx_timeout = 1000,
            .tx_id = 0x100,
            .rx_id = 0x101
        },
        .cmd_timeout_ms = 5000,
        .retry_count = 3,
        .enable_crc = true
    },
    .reset_gpio = NULL,
    .irq_gpio = NULL
};

// 定义 BMC 配置
static pcl_bmc_entry_t payload_bmc_entry = {
    .name = "payload_bmc",
    .description = "Payload BMC controller",
    .enabled = true,
    .config = {
        .network = {
            .enabled = true,
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .serial = {
            .enabled = true,
            .device = "/dev/ttyS0",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = HAL_SERIAL_PARITY_NONE,
            .timeout_ms = 3000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    },
    .power_gpio = NULL,
    .reset_gpio = NULL
};

// 定义板级配置
static pcl_mcu_entry_t *mcu_array[] = {
    &power_mcu_entry,
    NULL  // 数组结束标记
};

static pcl_bmc_entry_t *bmc_array[] = {
    &payload_bmc_entry,
    NULL
};

static pcl_platform_config_t platform_config = {
    .platform_name = "ti",
    .chip_name = "am6254",
    .project_name = "H200_100P",
    .product_name = "h200_100p_base",
    .mcu_arr = mcu_array,
    .bmc_arr = bmc_array,
    .fpga_arr = NULL,
    .switch_arr = NULL
};
```

### 2. 注册配置

```c
int main(void) {
    // 初始化 PCL
    PCL_Init();
    
    // 注册平台配置
    PCL_Register(&platform_config);
    
    return 0;
}
```

### 3. 查询并使用配置（应用层）

```c
void init_mcu(void) {
    // 1. 获取平台配置
    const pcl_platform_config_t *platform = PCL_GetBoard();
    
    // 2. 查找 MCU 配置条目
    const pcl_mcu_entry_t *entry = PCL_HW_FindMCU(platform, "power_mcu");
    if (!entry || !entry->enabled) {
        OSAL_LOG_ERROR("MCU not found or disabled");
        return;
    }
    
    // 3. 直接传递 PDL 配置（零拷贝）
    pdl_mcu_handle_t handle;
    int32_t ret = PDL_MCU_Init(&entry->config, &handle);
    if (ret != OSAL_SUCCESS) {
        OSAL_LOG_ERROR("Failed to init MCU");
        return;
    }
    
    OSAL_LOG_INFO("MCU initialized: %s", entry->name);
}

void init_bmc(void) {
    // 1. 获取平台配置
    const pcl_platform_config_t *platform = PCL_GetBoard();
    
    // 2. 查找 BMC 配置条目
    const pcl_bmc_entry_t *entry = PCL_HW_FindBMC(platform, "payload_bmc");
    if (!entry || !entry->enabled) {
        OSAL_LOG_ERROR("BMC not found or disabled");
        return;
    }
    
    // 3. 直接传递 PDL 配置（零拷贝）
    pdl_bmc_handle_t handle;
    int32_t ret = PDL_BMC_Init(&entry->config, &handle);
    if (ret != OSAL_SUCCESS) {
        OSAL_LOG_ERROR("Failed to init BMC");
        return;
    }
    
    OSAL_LOG_INFO("BMC initialized: %s", entry->name);
}
```

### 4. PDL 层使用配置

```c
// pdl_mcu.c
int32_t PDL_MCU_Init(const pdl_mcu_config_t *config, pdl_mcu_handle_t *handle) {
    // 根据接口类型初始化
    if (config->interface == PDL_MCU_INTERFACE_CAN) {
        // 构造 HAL CAN 配置（直接使用嵌入的字段）
        hal_can_config_t hal_cfg = {
            .interface = config->can.device,
            .baudrate = config->can.bitrate,
            .rx_timeout = config->can.rx_timeout,
            .tx_timeout = config->can.tx_timeout
        };
        
        // 初始化 HAL CAN
        hal_can_handle_t hal_handle;
        int32_t ret = HAL_CAN_Init(&hal_cfg, &hal_handle);
        if (ret != OSAL_SUCCESS) {
            return ret;
        }
        
        // 保存 PDL 层特有的配置（tx_id, rx_id）
        // ...
    }
    else if (config->interface == PDL_MCU_INTERFACE_SERIAL) {
        // 构造 HAL Serial 配置
        hal_serial_config_t hal_cfg = {
            .baud_rate = config->serial.baudrate,
            .data_bits = config->serial.data_bits,
            .stop_bits = config->serial.stop_bits,
            .parity = config->serial.parity,
            .flow_control = config->serial.flow_control
        };
        
        // 初始化 HAL Serial
        hal_serial_handle_t hal_handle;
        int32_t ret = HAL_Serial_Open(config->serial.device, &hal_cfg, &hal_handle);
        if (ret != OSAL_SUCCESS) {
            return ret;
        }
    }
    
    return OSAL_SUCCESS;
}
```

## 配置转换对比

### 旧设计（需要转换）

```c
// PCL 配置 → PDL 配置 → HAL 配置（三次拷贝）
pcl_mcu_cfg_t pcl_cfg;      // PCL 层定义
mcu_config_t pdl_cfg;        // PDL 层定义
hal_can_config_t hal_cfg;    // HAL 层定义

// 需要转换代码
convert_pcl_to_pdl(&pcl_cfg, &pdl_cfg);
convert_pdl_to_hal(&pdl_cfg, &hal_cfg);
```

### 新设计（零拷贝）

```c
// PCL 条目 → PDL 配置 → HAL 配置（零拷贝）
pcl_mcu_entry_t entry;       // PCL 层容器
entry.config;                // PDL 配置（直接引用）
entry.config.can.device;     // HAL 字段（直接嵌入）

// 无需转换，直接传递
PDL_MCU_Init(&entry.config, &handle);
```

## 维护成本对比

### 添加新配置字段

**旧设计**：需要修改三处
1. PCL 层配置结构体
2. PDL 层配置结构体
3. 转换函数

**新设计**：只需修改一处
1. PDL 层配置结构体（HAL 字段直接嵌入）

### 修改配置字段类型

**旧设计**：需要修改三处 + 转换逻辑

**新设计**：只需修改一处

## 总结

### 设计优势

1. **零拷贝** - 配置直接传递，无转换开销
2. **单一定义** - 配置只在 PDL/HAL 定义
3. **类型安全** - 编译期检查
4. **易维护** - 修改配置只需改一处
5. **清晰职责** - PCL 只做配置管理，不定义配置结构

### 各层职责

- **HAL**: 定义硬件接口配置（纯硬件参数）
- **PDL**: 定义外设配置（嵌入 HAL 字段 + 业务配置）
- **PCL**: 配置容器（引用 PDL 配置 + 管理字段）

### 使用建议

1. 新增外设时，在 PDL 层定义配置结构
2. PDL 配置直接嵌入 HAL 字段，避免重复定义
3. PCL 层只添加配置管理字段（name/enabled/description）
4. 应用层通过 PCL API 查询配置，直接传递给 PDL

---

**版本**: 1.0.0  
**最后更新**: 2026-05-29
