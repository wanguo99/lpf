# PCL 架构设计

## 设计理念

PCL采用**以外设为单位**的设计，类似Linux设备树，将硬件配置以外设为核心进行组织。

## 外设为单位 vs 接口为单位

### 传统方式（接口为单位）
```
配置CAN接口 → 配置UART接口 → 配置I2C接口
然后在代码中指定哪个接口连接哪个外设
```

**问题**：
- 配置分散，难以理解
- 接口与外设的映射关系不直观
- 更换接口需要修改多处

### PCL方式（外设为单位）
```c
/* MCU外设配置（包含其通信接口） */
pcl_mcu_cfg_t mcu_stm32 = {
    .name = "stm32_mcu",
    .interface_type = HW_INTERFACE_UART,  /* 使用UART通信 */
    .interface_cfg.uart = {
        .device = "/dev/ttyS1",
        .baudrate = 115200
    },
    .cmd_timeout_ms = 500,
    .enable_crc = true
};
```

**优势**：
- 配置集中：一个外设的所有配置在一起
- 易于理解：从外设角度思考
- 便于移植：更换接口只需修改外设配置

## 核心数据结构

### 接口配置（联合体）

```c
typedef enum {
    HW_INTERFACE_CAN,
    HW_INTERFACE_UART,
    HW_INTERFACE_I2C,
    HW_INTERFACE_SPI,
    HW_INTERFACE_ETHERNET
} pcl_interface_type_t;

/* 每个外设包含接口配置联合体 */
typedef struct {
    pcl_interface_type_t interface_type;
    union {
        pcl_can_cfg_t  can;
        pcl_uart_cfg_t uart;
        pcl_i2c_cfg_t  i2c;
        pcl_spi_cfg_t  spi;
        pcl_ethernet_cfg_t ethernet;
    } interface_cfg;
} pcl_interface_t;
```

**优势**：
- 类型安全：编译时检查
- 内存高效：联合体只占用最大接口的内存
- 易于扩展：添加新接口类型只需扩展枚举和联合体

### 外设配置

#### MCU外设
```c
typedef struct {
    const char *name;
    bool enabled;
    
    /* 通信接口（内嵌） */
    pcl_interface_type_t interface_type;
    union {
        pcl_can_cfg_t  can;
        pcl_uart_cfg_t uart;
        pcl_i2c_cfg_t  i2c;
        pcl_spi_cfg_t  spi;
    } interface_cfg;
    
    /* MCU特定配置 */
    uint32_t cmd_timeout_ms;
    uint32_t retry_count;
    bool enable_crc;
    
    /* GPIO控制 */
    pcl_gpio_config_t *reset_gpio;
    pcl_gpio_config_t *irq_gpio;
} pcl_mcu_cfg_t;
```

#### BMC外设（双通道）
```c
typedef struct {
    const char *name;
    bool enabled;
    
    /* 主通道（以太网） */
    struct {
        pcl_interface_type_t type;
        pcl_ethernet_cfg_t cfg;
    } primary_channel;
    
    /* 备份通道（串口） */
    struct {
        pcl_interface_type_t type;
        pcl_uart_cfg_t cfg;
    } backup_channel;
    
    /* BMC特定配置 */
    uint32_t cmd_timeout_ms;
    uint32_t failover_threshold;
    
    /* GPIO控制 */
    pcl_gpio_config_t *power_gpio;
    pcl_gpio_config_t *reset_gpio;
} pcl_bmc_cfg_t;
```

### 板级配置

```c
typedef struct {
    /* 板级信息 */
    const char *platform;
    const char *product;
    const char *version;
    const char *description;
    
    /* 外设配置列表（以外设为单位） */
    pcl_mcu_cfg_t **mcus;
    uint32_t mcu_count;
    
    pcl_bmc_cfg_t **bmcs;
    uint32_t bmc_count;
    
    pcl_satellite_cfg_t **satellites;
    uint32_t satellite_count;
    
    pcl_sensor_cfg_t **sensors;
    uint32_t sensor_count;
    
    pcl_storage_cfg_t **storages;
    uint32_t storage_count;
} pcl_board_config_t;
```

## 平台组织结构

### 嵌套目录结构

```
platform/
├── ti/am6254/                    # 厂商/芯片
│   ├── H200_100P/                # 产品
│   │   ├── h200_100p_base.c      # 基础配置
│   │   ├── h200_100p_v1.c        # V1.0配置
│   │   └── h200_100p_v2.c        # V2.0配置
│   └── H200_32P/
│       ├── h200_32p_base.c
│       ├── h200_32p_v1.c
│       └── h200_32p_v2.c
└── vendor_demo/platform_demo/project_demo/
    ├── product_demo_base.c
    ├── product_demo_v1.c
    └── product_demo_v2.c
```

### 配置选择机制

1. **环境变量**：`XCONFIG_PLATFORM=ti/am6254/H200_100P/v1`
2. **编译选项**：`-DXCONFIG_PLATFORM=...`
3. **默认配置**：代码中指定默认配置

## 配置注册机制

```c
/* 注册配置 */
void HW_Config_RegisterAll(void)
{
    /* TI AM6254平台 */
    HW_Config_Register(&pcl_h200_100p_base);
    HW_Config_Register(&pcl_h200_100p_v1);
    HW_Config_Register(&pcl_h200_100p_v2);
    
    /* 演示平台 */
    HW_Config_Register(&pcl_demo_base);
    HW_Config_Register(&pcl_demo_v1);
    HW_Config_Register(&pcl_demo_v2);
}

/* 选择配置 */
const pcl_board_config_t* HW_Config_SelectDefault(void)
{
    /* 1. 检查环境变量 */
    const char *env = OSAL_Getenv("XCONFIG_PLATFORM");
    if (env != NULL) {
        return HW_Config_FindByName(env);
    }
    
    /* 2. 使用编译选项 */
#ifdef XCONFIG_PLATFORM
    return HW_Config_FindByName(XCONFIG_PLATFORM);
#endif
    
    /* 3. 使用默认配置 */
    return &pcl_h200_100p_base;
}
```

## 设计优势

1. **外设为中心**：从外设角度思考配置，更符合硬件设计思维
2. **配置集中**：一个外设的所有配置（接口+参数）集中在一起
3. **内存高效**：使用联合体，每个外设只占用最大接口类型的内存
4. **类型安全**：编译时检查配置类型
5. **易于维护**：修改外设通信接口只需修改配置，无需改动代码
6. **支持冗余**：BMC等外设原生支持主备双通道配置
7. **航天适配**：原生支持SpaceWire、1553B等航天接口

## 与传统配置方式对比

| 特性 | 传统方式（接口为单位） | PCL（外设为单位） |
|------|----------------------|---------------------|
| 配置组织 | 按接口类型分组 | 按外设分组 |
| 配置查找 | 先找接口，再关联外设 | 直接查找外设 |
| 接口变更 | 需修改多处配置 | 只修改外设配置 |
| 代码可读性 | 需理解接口-外设映射 | 直观清晰 |
| 冗余支持 | 需额外设计 | 原生支持（如BMC双通道） |

## 相关文档

- [API参考](API_REFERENCE.md)
- [使用指南](USAGE_GUIDE.md)
