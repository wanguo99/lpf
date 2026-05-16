# PCL层详细设计

## 5. PCL 设计（外设配置层）

### 5.1 PCL设计原则

**PCL（Peripheral Configuration Library）是外设配置层，提供纯数据结构的硬件配置。**

#### 5.1.1 核心理念

- ✅ **纯数据结构**：只包含配置数据，不包含任何函数实现或业务逻辑
- ✅ **平台组织**：按`vendor/chip/product`三级目录组织配置文件
- ✅ **类型安全**：使用强类型结构体，编译期检查
- ✅ **硬件抽象**：配置描述硬件连接关系，不涉及业务逻辑

#### 5.1.2 配置层次

```
pcl/
├── include/
│   ├── api/pcl_api.h              # PCL公开API
│   ├── peripheral/                 # 外设配置类型定义
│   │   ├── pcl_satellite.h        # 卫星平台配置类型
│   │   ├── pcl_bmc.h              # BMC配置类型
│   │   ├── pcl_mcu.h              # MCU配置类型
│   │   └── pcl_hardware_interface.h # 硬件接口配置（CAN/UART/I2C/SPI）
│   └── internal/                   # PCL内部头文件
│       ├── pcl_board.h            # 板级配置结构
│       └── pcl_common.h           # 公共定义
└── platform/                       # 平台配置实现
    ├── ti/am625/pmc_v1/           # TI AM625 PMC v1.0产品
    │   └── pcl_ti_am625_pmc_v1.c  # 硬件配置数据（命名规范：pcl_{platform}_{chip}_{project}_{version}.c）
    └── vendor_demo/demo_board/    # 演示平台
        └── pcl_vendor_demo_demo_board_v1.c
```

#### 5.1.3 配置数据流

```
┌─────────────────────────────────────────────────────────────┐
│  PCL配置数据流                                               │
│                                                              │
│  1. 编译时：选择平台配置                                     │
│     CMake: -DPCL_PLATFORM=ti/am625/pmc_v1                   │
│                                                              │
│  2. 初始化：PCL_Init()加载配置                               │
│     pcl/platform/ti/am625/pmc_v1/pcl_ti_am625_pmc_v1.c       │
│                                                              │
│  3. PDL层读取：PDL_XXX_Init()从PCL获取配置                  │
│     const pcl_mcu_cfg_t *cfg = PCL_GetMCUConfig(0);         │
│     PDL_MCU_Init(cfg, &handle);                             │
│                                                              │
│  4. PDL层调用HAL：根据配置初始化HAL层                        │
│     if (cfg->interface_type == PCL_HW_INTERFACE_CAN) {      │
│         HAL_CAN_Init(&cfg->interface_cfg.can, &can_handle); │
│     }                                                        │
└─────────────────────────────────────────────────────────────┘
```

---

### 5.2 硬件接口配置

**PCL提供统一的硬件接口配置类型，支持CAN/UART/I2C/SPI/GPIO等。**

#### 5.2.1 硬件接口类型枚举

```c
/* 硬件接口类型 */
typedef enum {
    PCL_HW_INTERFACE_NONE = 0,
    PCL_HW_INTERFACE_CAN,        /* CAN总线 */
    PCL_HW_INTERFACE_UART,       /* 串口 */
    PCL_HW_INTERFACE_I2C,        /* I2C总线 */
    PCL_HW_INTERFACE_SPI,        /* SPI总线 */
    PCL_HW_INTERFACE_SPACEWIRE,  /* SpaceWire（航天专用） */
    PCL_HW_INTERFACE_1553B,      /* MIL-STD-1553B（航天专用） */
    PCL_HW_INTERFACE_ETHERNET,   /* 以太网 */
    PCL_HW_INTERFACE_MAX
} pcl_hw_interface_type_t;
```

#### 5.2.2 CAN接口配置

```c
/* CAN接口配置 */
typedef struct {
    const char *device;          /* CAN设备名（如"can0"） */
    uint32_t    bitrate;         /* 波特率（如500000） */
    uint32_t    tx_id;           /* 发送CAN ID */
    uint32_t    rx_id;           /* 接收CAN ID */
    bool        loopback;        /* 回环模式（测试用） */
    bool        listen_only;     /* 只听模式（监控用） */
} pcl_can_cfg_t;
```

#### 5.2.3 UART接口配置

```c
/* UART接口配置 */
typedef struct {
    const char *device;          /* 串口设备（如"/dev/ttyS0"） */
    uint32_t    baudrate;        /* 波特率 */
    uint8_t     data_bits;       /* 数据位（5-8） */
    uint8_t     stop_bits;       /* 停止位（1-2） */
    int8_t      parity;          /* 校验位（'N'/'E'/'O'） */
    uint8_t     flow_control;    /* 流控（0=无，1=硬件，2=软件） */
} pcl_uart_cfg_t;
```

#### 5.2.4 I2C接口配置

```c
/* I2C接口配置 */
typedef struct {
    uint8_t  bus;                /* I2C总线号 */
    uint16_t addr;               /* 从设备地址（7位或10位） */
    uint32_t speed;              /* 速率（Hz，如100000=100kHz） */
} pcl_i2c_cfg_t;
```

#### 5.2.5 SPI接口配置

```c
/* SPI接口配置 */
typedef struct {
    const char *device;          /* SPI设备（如"/dev/spidev0.0"） */
    uint8_t     mode;            /* SPI模式（0-3） */
    uint8_t     bits_per_word;   /* 每字位数（通常为8） */
    uint32_t    max_speed_hz;    /* 最大速率（Hz） */
} pcl_spi_cfg_t;
```

#### 5.2.6 GPIO配置

```c
/* GPIO配置 */
typedef struct {
    uint32_t gpio_num;           /* GPIO编号 */
    uint8_t  direction;          /* 方向（0=输入，1=输出） */
    uint8_t  active_level;       /* 有效电平（0=低，1=高） */
} pcl_gpio_config_t;
```

---

### 5.3 外设配置结构

#### 5.3.1 卫星平台配置

```c
/* 卫星平台配置 */
typedef struct {
    /* 外设基本信息 */
    const char *name;                    /* 卫星平台名称（如"satellite_bus"） */
    const char *description;             /* 描述信息 */
    bool        enabled;                 /* 是否启用 */

    /* 硬件通信接口配置（使用联合体） */
    pcl_hw_interface_type_t interface_type;
    union {
        pcl_spacewire_cfg_t spacewire;   /* SpaceWire接口 */
        pcl_1553b_cfg_t     mil1553b;    /* 1553B接口 */
        pcl_can_cfg_t       can;         /* CAN接口 */
        pcl_uart_cfg_t      uart;        /* 串口接口 */
    } interface_cfg;

    /* 卫星平台特定配置 */
    uint32_t cmd_timeout_ms;             /* 命令超时（ms） */
    uint32_t retry_count;                /* 重试次数 */
    bool     enable_telemetry;           /* 启用遥测 */

    /* GPIO控制（可选） */
    pcl_gpio_config_t *power_gpio;       /* 电源控制GPIO */
    pcl_gpio_config_t *reset_gpio;       /* 复位GPIO */
} pcl_satellite_cfg_t;
```

#### 5.3.2 BMC配置

```c
/* BMC外设配置 */
typedef struct {
    /* 外设基本信息 */
    const char *name;                    /* BMC名称（如"payload_bmc"） */
    const char *description;             /* 描述信息 */
    bool        enabled;                 /* 是否启用 */

    /* 主通道配置 */
    struct {
        bool               enabled;      /* 是否启用 */
        pcl_bmc_protocol_t protocol;     /* 协议类型（IPMI/Redfish） */
        union {
            pcl_bmc_ipmi_lan_cfg_t  ipmi_lan;   /* IPMI over LAN */
            pcl_bmc_redfish_cfg_t   redfish;    /* Redfish */
        } cfg;
    } primary_channel;

    /* 备份通道配置（通常是IPMI over Serial） */
    struct {
        bool               enabled;      /* 是否启用 */
        pcl_bmc_protocol_t protocol;     /* 协议类型 */
        pcl_bmc_ipmi_serial_cfg_t cfg;   /* IPMI over Serial */
    } backup_channel;

    /* BMC特定配置 */
    uint32_t cmd_timeout_ms;             /* 命令超时（ms） */
    uint32_t retry_count;                /* 重试次数 */
    bool     auto_switch;                /* 自动切换通道 */
    uint32_t failover_threshold;         /* 故障切换阈值（连续失败次数） */
    uint32_t health_check_interval;      /* 健康检查间隔（ms） */

    /* GPIO控制（可选） */
    pcl_gpio_config_t *power_gpio;       /* 电源控制GPIO */
    pcl_gpio_config_t *reset_gpio;       /* 复位GPIO */
} pcl_bmc_cfg_t;
```

#### 5.3.3 MCU配置

```c
/* MCU外设配置 */
typedef struct {
    /* 外设基本信息 */
    const char *name;                    /* MCU名称（如"stm32_mcu"） */
    const char *description;             /* 描述信息 */
    bool        enabled;                 /* 是否启用 */

    /* 硬件通信接口配置（使用联合体） */
    pcl_hw_interface_type_t interface_type;
    union {
        pcl_can_cfg_t  can;              /* CAN接口 */
        pcl_uart_cfg_t uart;             /* 串口接口 */
        pcl_i2c_cfg_t  i2c;              /* I2C接口 */
        pcl_spi_cfg_t  spi;              /* SPI接口 */
    } interface_cfg;

    /* MCU特定配置 */
    uint32_t cmd_timeout_ms;             /* 命令超时（ms） */
    uint32_t retry_count;                /* 重试次数 */
    bool     enable_crc;                 /* 启用CRC校验 */

    /* GPIO控制（可选） */
    pcl_gpio_config_t *reset_gpio;       /* 复位GPIO */
    pcl_gpio_config_t *irq_gpio;         /* 中断GPIO */
} pcl_mcu_cfg_t;
```

---

### 5.4 板级配置

**板级配置将所有外设配置组织在一起，形成完整的硬件配置描述。**

```c
/* 板级配置 */
typedef struct {
    const char *platform;                /* 平台标识（如"ti/am625"） */
    const char *product;                 /* 产品标识（如"pmc_v1"） */
    const char *version;                 /* 版本号（如"1.0.0"） */

    /* 外设配置数组（以NULL结尾） */
    const pcl_satellite_cfg_t **satellites;  /* 卫星平台配置数组 */
    const pcl_bmc_cfg_t       **bmcs;        /* BMC配置数组 */
    const pcl_mcu_cfg_t       **mcus;        /* MCU配置数组 */
    const pcl_sensor_cfg_t    **sensors;     /* 传感器配置数组 */
    const pcl_storage_cfg_t   **storages;    /* 存储配置数组 */
} pcl_board_config_t;
```

---

### 5.5 PCL API设计

**PCL提供简洁的API供PDL层查询配置。**

```c
/* 初始化PCL（加载平台配置） */
int32_t PCL_Init(const char *platform_name);

/* 获取板级配置 */
const pcl_board_config_t *PCL_GetBoardConfig(void);

/* 获取外设配置（按索引） */
const pcl_satellite_cfg_t *PCL_GetSatelliteConfig(uint32_t index);
const pcl_bmc_cfg_t       *PCL_GetBMCConfig(uint32_t index);
const pcl_mcu_cfg_t       *PCL_GetMCUConfig(uint32_t index);

/* 获取外设配置（按名称） */
const pcl_satellite_cfg_t *PCL_FindSatelliteByName(const char *name);
const pcl_bmc_cfg_t       *PCL_FindBMCByName(const char *name);
const pcl_mcu_cfg_t       *PCL_FindMCUByName(const char *name);

/* 获取外设数量 */
uint32_t PCL_GetSatelliteCount(void);
uint32_t PCL_GetBMCCount(void);
uint32_t PCL_GetMCUCount(void);
```

---

### 5.6 平台配置示例

- **卫星平台**：1个CAN接口（can0，500kbps，ID 0x200/0x100）
- **BMC**：1个双通道配置（主通道Redfish网络，备份通道IPMI串口）
- **MCU**：3个MCU（电源控制CAN、电源监控I2C、看门狗CAN）

**配置特点**：
- ✅ 类型安全：编译期检查配置正确性
- ✅ 灵活性：支持多种硬件接口组合
- ✅ 可扩展：新增外设只需添加配置项
- ✅ 平台隔离：不同平台配置完全独立

**完整的PMC v1.0平台配置示例）**：

```C
/************************************************
* pcl/platform/ti/am625/pmc_v1/pcl_ti_am625_pmc_v1.c
* PMC v1.0硬件配置
* 命名规范：pcl_{platform}_{chip}_{project}_{version}.c
************************************************/

// 卫星平台接口（CAN）
static const pcl_satellite_cfg_t satellite_can = {
  .name = "satellite_can",
  .interface_type = PCL_HW_INTERFACE_CAN,
  .interface_cfg.can = {
	  .device = "can0",
	  .bitrate = 500000,
	  .tx_id = 0x200,
	  .rx_id = 0x100,
	  .loopback = false,
	  .listen_only = false
  },
  .cmd_timeout_ms = 100,
  .retry_count = 3
};

// BMC接口（Redfish主通道 + IPMI串口备份）
static const pcl_bmc_cfg_t bmc_payload = {
  .name = "bmc_payload",
  .primary_channel = {
	  .enabled = true,
	  .protocol = PCL_BMC_PROTOCOL_REDFISH,
	  .cfg.redfish = {
		  .ip_addr = "192.168.1.100",
		  .port = 443,
		  .username = "admin",
		  .password = "admin",
		  .timeout_ms = 500
	  }
  },
  .backup_channel = {
	  .enabled = true,
	  .protocol = PCL_BMC_PROTOCOL_IPMI,
	  .cfg.ipmi_serial = {
		  .device = "/dev/ttyS2",
		  .baudrate = 115200,
		  .timeout_ms = 1000
	  }
  },
  .auto_switch = true,
  .failover_threshold = 3,
  .health_check_interval = 5000
};

// MCU接口（CAN）
static const pcl_mcu_cfg_t mcu_power_ctrl = {
  .name = "mcu_power",
  .interface_type = PCL_HW_INTERFACE_CAN,
  .interface_cfg.can = {
	  .device = "can0",
	  .bitrate = 500000,
	  .tx_id = 0x300,
	  .rx_id = 0x301
  },
  .cmd_timeout_ms = 500,
  .retry_count = 3,
  .enable_crc = true
};

static const pcl_mcu_cfg_t mcu_power_monitor = {
  .name = "mcu_monitor",
  .interface_type = PCL_HW_INTERFACE_I2C,
  .interface_cfg.i2c = {
	  .bus = 1,
	  .addr = 0x50,
	  .speed = 100000
  },
  .cmd_timeout_ms = 200,
  .retry_count = 3
};

static const pcl_mcu_cfg_t mcu_watchdog = {
  .name = "mcu_watchdog",
  .interface_type = PCL_HW_INTERFACE_CAN,
  .interface_cfg.can = {
	  .device = "can0",
	  .bitrate = 500000,
	  .tx_id = 0x400,
	  .rx_id = 0x401
  },
  .cmd_timeout_ms = 100,
  .retry_count = 1
};

// 板级配置
static const pcl_board_config_t pmc_v1_board = {
  .platform = "ti/am625",
  .product = "pmc_v1",
  .version = "1.0.0",

  .satellites = {
	  &satellite_can,
	  NULL
  },

  .bmcs = {
	  &bmc_payload,
	  NULL
  },

  .mcus = {
	  &mcu_power_ctrl,    // MCU[0]
	  &mcu_power_monitor, // MCU[1]
	  &mcu_watchdog,      // MCU[2]
	  NULL
  }
};
```
---
