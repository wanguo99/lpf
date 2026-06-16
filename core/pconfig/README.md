# PConfig (外设配置库)

## 模块概述

PConfig (Peripheral Configuration Library) 是硬件配置库，参考Linux设备树架构，以外设为单位管理硬件配置。

**设计理念**：
- 外设为单位的配置管理（类似设备树）
- 配置与代码分离
- 接口内嵌（每个外设配置包含其通信接口）
- 运行时查询
- 支持多平台多产品配置

**支持的外设类型**：
- MCU外设（微控制器）
- 主机接口
- BMC设备（基板管理控制器）
- 传感器外设
- 存储设备
- 应用配置

## 主要特性

- **外设为单位**：每个外设独立配置，包含名称、类型、接口等信息
- **配置分离**：硬件配置与业务代码完全分离
- **接口内嵌**：外设配置内嵌通信接口配置（CAN/UART/I2C/SPI/Ethernet）
- **运行时查询**：支持按名称、类型查询外设配置
- **多平台支持**：嵌套目录结构支持多厂商、多平台、多产品
- **版本管理**：支持同一产品的多个硬件版本

## 配置架构

### 平台配置目录结构

```
pcl/platform/
├── ti/am6254/              # TI AM6254平台
│   └── H200_100P/          # H200-100P产品（100P算力）
│       ├── h200_100p_base.c    # 基础配置（所有版本共享）
│       ├── h200_100p_v1.c      # V1版本特定配置
│       └── h200_100p_v2.c      # V2版本特定配置
└── vendor_demo/            # 演示厂商
    └── platform_demo/      # 演示平台
        └── project_demo/   # 演示项目（2P算力）
            ├── product_demo_base.c
            ├── product_demo_v1.c
            └── product_demo_v2.c
```

### 配置选择机制

PCONFIG支持三种配置选择方式（优先级从高到低）：

1. **环境变量**（最高优先级）
   ```bash
   export PCONFIG_PLATFORM=ti/am6254/H200_100P
   export PCONFIG_VERSION=v2
   ```

2. **编译选项**
   ```bash
   cmake ../.. -DPCONFIG_PLATFORM=ti/am6254/H200_100P -DPCONFIG_VERSION=v2
   ```

3. **默认配置**（最低优先级）
   ```c
   // 在 pconfig_register.c 中定义
   #ifndef PCONFIG_PLATFORM
   #define PCONFIG_PLATFORM "vendor_demo/platform_demo/project_demo"
   #endif
   ```

## 编译说明

### 快速开始

```bash
# 在项目根目录使用统一构建命令
make tests_x86_pconfig_defconfig  # 加载 PCONFIG 测试配置
make

# 或使用其他包含 PCONFIG 的配置
make ccm_h200_100p_am625_debug_defconfig
make
```

PConfig 模块会作为核心模块的一部分自动编译。

### 构建配置

PConfig 模块通过 Config.in 配置启用：

```kconfig
CONFIG_PCONFIG=y          # 启用平台配置层
CONFIG_PCONFIG_PLATFORM="ti/am6254/H200_100P"  # 平台路径
CONFIG_PCONFIG_VERSION="v2"                     # 硬件版本
```

也可以通过环境变量或 CMake 参数指定平台：

```bash
# 方法1: 环境变量（最高优先级）
export PCONFIG_PLATFORM=ti/am6254/H200_100P
export PCONFIG_VERSION=v2
make

# 方法2: 在 menuconfig 中配置
make menuconfig
# 导航到 "Platform Configuration" → "Platform Path"
```

编译输出：
- 库文件：`_build/lib/libpconfig.a`
- 包含选定平台的硬件配置数据

## 模块结构

```
pcl/
├── include/                    # 公共接口头文件
│   ├── api/                    # API接口
│   │   └── pconfig_api.h           # PCONFIG查询接口
│   ├── internal/               # 内部接口
│   │   └── pconfig_register.h      # 配置注册接口
│   ├── peripheral/             # 外设配置定义
│   │   ├── pconfig_common.h        # 通用类型（GPIO、电源域）
│   │   ├── pconfig_hardware_interface.h  # 硬件接口定义
│   │   ├── pconfig_mcu.h           # MCU外设配置
│   │   ├── pconfig_bmc.h           # BMC外设配置
│   │   ├── pconfig_satellite.h     # 主机接口配置
│   │   ├── pconfig_sensor.h        # 传感器外设配置
│   │   ├── pconfig_storage.h       # 存储设备配置
│   │   └── pconfig_app.h           # APP配置
│   └── pcl.h                   # 总头文件
├── src/                        # 源代码
│   ├── pconfig_api.c               # API实现
│   └── pconfig_register.c          # 配置注册
└── platform/                   # 平台配置（嵌套目录结构）
    ├── ti/am6254/              # TI AM6254平台
    │   └── H200_100P/          # H200-100P产品
    │       ├── h200_100p_base.c
    │       ├── h200_100p_v1.c
    │       └── h200_100p_v2.c
    └── vendor_demo/            # 演示厂商
        └── platform_demo/      # 演示平台
            └── project_demo/   # 演示项目
                ├── product_demo_base.c
                ├── product_demo_v1.c
                └── product_demo_v2.c
```

## 依赖关系

**PCONFIG依赖**：
- OSAL层：基础类型定义（`osal_types.h`）
- 无其他模块依赖（纯配置库）

**被依赖**：
- PDL层：外设驱动层（唯一使用者）

**重要**：PCONFIG必须保持完全平台无关，只包含配置数据结构。

## 使用示例

### 在PDL层中使用PCONFIG

**CMakeLists.txt配置**：
```cmake
# 链接PCONFIG接口库（获取头文件路径）
target_link_libraries(pdl PUBLIC es_middleware::pconfig_public_api)

# 链接PCONFIG实现库（运行时链接）
target_link_libraries(pdl PRIVATE es_middleware::pcl)
```

**代码中使用**：
```c
#include <pconfig.h>

int main(void)
{
    /* 初始化PCONFIG */
    int32 ret = PCONFIG_Init();
    if (ret != OS_SUCCESS) {
        LOG_ERROR("APP", "PConfig init failed");
        return -1;
    }
    
    /* 查询MCU配置 */
    const pconfig_mcu_t *mcu = PCONFIG_GetMCUConfig("mcu0");
    if (mcu != NULL) {
        LOG_INFO("APP", "MCU: %s, Type: %d", mcu->name, mcu->interface_type);
        
        if (mcu->interface_type == PCONFIG_INTERFACE_CAN) {
            LOG_INFO("APP", "CAN Interface: %s, Baudrate: %u",
                     mcu->can_interface.device,
                     mcu->can_interface.baudrate);
        }
    }
    
    /* 查询主机接口配置 */
    const pconfig_satellite_t *host = PCONFIG_GetSatelliteConfig();
    if (host != NULL) {
        LOG_INFO("APP", "Host Interface: %s, Protocol: %d",
                 host->name, host->protocol);
    }
    
    /* 查询BMC配置 */
    const pconfig_bmc_t *bmc = PCONFIG_GetBMCConfig("bmc0");
    if (bmc != NULL) {
        LOG_INFO("APP", "BMC: %s, IP: %s:%u",
                 bmc->name,
                 bmc->ethernet_interface.ip_address,
                 bmc->ethernet_interface.port);
    }
    
    /* 枚举所有MCU */
    uint32 count = PCONFIG_GetMCUCount();
    for (uint32 i = 0; i < count; i++) {
        const pconfig_mcu_t *mcu_item = PCONFIG_GetMCUByIndex(i);
        LOG_INFO("APP", "MCU[%u]: %s", i, mcu_item->name);
    }
    
    return 0;
}
```

## 配置示例

### MCU外设配置

```c
/* platform/ti/am6254/H200_100P/h200_100p_base.c */
#include "pconfig.h"

static const pconfig_mcu_t mcu_configs[] = {
    {
        .name = "mcu0",
        .description = "主MCU（STM32F407）",
        .interface_type = PCONFIG_INTERFACE_CAN,
        .can_interface = {
            .device = "can0",
            .baudrate = 500000,
            .mode = PCONFIG_CAN_MODE_NORMAL
        },
        .power_domain = PCONFIG_POWER_DOMAIN_MAIN,
        .reset_gpio = {
            .port = 'A',
            .pin = 0,
            .active_level = PCONFIG_GPIO_ACTIVE_LOW
        }
    },
    {
        .name = "mcu1",
        .description = "备份MCU（STM32F407）",
        .interface_type = PCONFIG_INTERFACE_UART,
        .uart_interface = {
            .device = "/dev/ttyS1",
            .baudrate = 115200,
            .data_bits = 8,
            .parity = 'N',
            .stop_bits = 1
        },
        .power_domain = PCONFIG_POWER_DOMAIN_BACKUP
    }
};

const pconfig_mcu_t* PCONFIG_GetMCUConfigs(uint32 *count)
{
    *count = OSAL_sizeof(mcu_configs) / OSAL_sizeof(mcu_configs[0]);
    return mcu_configs;
}
```

### 卫星平台配置

```c
static const pconfig_satellite_t satellite_config = {
    .name = "satellite_platform",
    .description = "卫星平台接口",
    .protocol = PCONFIG_SATELLITE_PROTOCOL_CAN,
    .can_interface = {
        .device = "can1",
        .baudrate = 1000000,
        .mode = PCONFIG_CAN_MODE_NORMAL
    },
    .heartbeat_interval_ms = 5000,
    .command_timeout_ms = 1000
};

const pconfig_satellite_t* PCONFIG_GetSatelliteConfig(void)
{
    return &satellite_config;
}
```

### BMC外设配置

```c
static const pconfig_bmc_t bmc_configs[] = {
    {
        .name = "bmc0",
        .description = "主BMC外设",
        .protocol = PCONFIG_BMC_PROTOCOL_REDFISH,
        .ethernet_interface = {
            .ip_address = "192.168.1.100",
            .port = 443,
            .use_ssl = true
        },
        .uart_interface = {
            .device = "/dev/ttyS2",
            .baudrate = 115200,
            .data_bits = 8,
            .parity = 'N',
            .stop_bits = 1
        },
        .power_domain = PCONFIG_POWER_DOMAIN_MAIN
    }
};

const pconfig_bmc_t* PCONFIG_GetBMCConfigs(uint32 *count)
{
    *count = OSAL_sizeof(bmc_configs) / OSAL_sizeof(bmc_configs[0]);
    return bmc_configs;
}
```

## 开发指南

### 添加新的平台配置

1. 创建平台目录：`platform/<vendor>/<chip>/<product>/`
2. 创建配置文件：
   - `<product>_base.c` - 基础配置（所有版本共享）
   - `<product>_v1.c` - V1版本特定配置
   - `<product>_v2.c` - V2版本特定配置
3. 实现配置函数：
   ```c
   const pconfig_mcu_t* PCONFIG_GetMCUConfigs(uint32 *count);
   const pconfig_satellite_t* PCONFIG_GetSatelliteConfig(void);
   const pconfig_bmc_t* PCONFIG_GetBMCConfigs(uint32 *count);
   ```
4. 在 `CMakeLists.txt` 中添加平台判断
5. 编译验证：
   ```bash
   cmake ../.. -DPCONFIG_PLATFORM=<vendor>/<chip>/<product> -DPCONFIG_VERSION=v1
   make pcl -j$(nproc)
   ```

### 添加新的外设类型

1. 在 `include/peripheral/` 添加外设配置头文件（如 `pconfig_sensor.h`）
2. 定义外设配置结构：
   ```c
   typedef struct {
       char name[64];
       char description[128];
       pconfig_interface_type_t interface_type;
       union {
           pconfig_i2c_interface_t i2c_interface;
           pconfig_spi_interface_t spi_interface;
       };
       pconfig_power_domain_t power_domain;
   } pconfig_sensor_t;
   ```
3. 在 `include/api/pconfig_api.h` 添加查询接口：
   ```c
   const pconfig_sensor_t* PCONFIG_GetSensorConfig(const char *name);
   const pconfig_sensor_t* PCONFIG_GetSensorByIndex(uint32 index);
   uint32 PCONFIG_GetSensorCount(void);
   ```
4. 在 `src/pconfig_api.c` 实现查询函数
5. 在平台配置文件中添加外设配置

### 编码规范（重要）

**必须遵守**：
- [正确] PCONFIG必须保持完全平台无关
- [错误] 禁止包含任何系统头文件（`<unistd.h>`, `<sys/socket.h>` 等）
- [错误] 禁止调用任何系统API或OSAL接口
- [正确] 只能使用OSAL类型定义（`osal_types.h`）
- [正确] 配置数据必须使用 `const` 修饰
- [正确] 使用 `static` 限制配置数据的作用域

**示例**：
```c
/* [正确] 正确 - 纯配置数据 */
static const pconfig_mcu_t mcu_configs[] = {
    {
        .name = "mcu0",
        .interface_type = PCONFIG_INTERFACE_CAN,
        .can_interface = {
            .device = "can0",
            .baudrate = 500000
        }
    }
};

/* [错误] 错误 - 包含系统调用 */
const pconfig_mcu_t* PCONFIG_GetMCUConfigs(uint32 *count)
{
    int fd = open("/dev/can0", O_RDWR);  // 禁止
    *count = 1;
    return mcu_configs;
}
```

## 测试

```bash
# 编译测试
./build.sh -d

# 运行PCONFIG测试
./build/bin/unit-test -L PConfig           # 运行所有PCONFIG测试
./build/bin/unit-test -m test_pcl_api  # 运行API测试
./build/bin/unit-test -i               # 交互式菜单
```

**测试覆盖**：
- 配置查询测试
- 外设枚举测试
- 多平台配置测试

## 常见问题

**Q: 如何切换到不同的平台配置？**
```bash
# 方法1: 使用环境变量
export PCONFIG_PLATFORM=ti/am6254/H200_100P
export PCONFIG_VERSION=v2
./build.sh

# 方法2: 使用CMake参数
cmake ../.. -DPCONFIG_PLATFORM=ti/am6254/H200_100P -DPCONFIG_VERSION=v2
make -j$(nproc)
```

**Q: 如何添加新的硬件版本？**
```bash
# 1. 创建版本配置文件
cp platform/ti/am6254/H200_100P/h200_100p_v2.c \
   platform/ti/am6254/H200_100P/h200_100p_v3.c

# 2. 修改配置内容
vim platform/ti/am6254/H200_100P/h200_100p_v3.c

# 3. 编译验证
cmake ../.. -DPCONFIG_PLATFORM=ti/am6254/H200_100P -DPCONFIG_VERSION=v3
make pcl -j$(nproc)
```

**Q: PCONFIG和HAL的区别？**
- PConfig：硬件配置库，纯数据配置，不包含任何代码逻辑
- HAL：硬件抽象层，封装硬件驱动，包含实际的硬件操作代码

**Q: 为什么PCONFIG不能调用OSAL接口？**
```c
/* PCONFIG是纯配置库，必须保持完全平台无关 */
/* 只能包含配置数据结构，不能包含任何代码逻辑 */
/* 这样可以确保配置在任何平台上都能正确编译 */
```

**Q: 如何查看当前使用的平台配置？**
```bash
# 查看编译配置
cd build
cmake -L ../.. | grep PConfig

# 或在代码中查询
PCONFIG_Init();
LOG_INFO("APP", "Platform: %s, Version: %s",
         PCONFIG_GetPlatform(), PCONFIG_GetVersion());
```

## 设计原则

1. **配置分离**：硬件配置与业务代码完全分离
2. **外设为单位**：以外设为单位管理配置，类似Linux设备树
3. **接口内嵌**：每个外设配置包含其通信接口配置
4. **平台无关**：PCONFIG必须保持完全平台无关，只包含配置数据
5. **运行时查询**：支持运行时动态查询配置信息

## 参考文档

- [PCONFIG详细文档](docs/README.md)
- [架构设计](docs/ARCHITECTURE.md)
- [API参考](docs/API_REFERENCE.md)
- [配置规范](docs/CONFIGURATION_GUIDE.md)
- [平台适配指南](docs/PLATFORM_PORTING.md)
- [编码规范](../docs/CODING_STANDARDS.md)
