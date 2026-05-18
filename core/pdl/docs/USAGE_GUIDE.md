# PDL 使用指南

## 卫星平台服务

### 基本使用
```c
#include "pdl_satellite.h"

void cmd_callback(uint32_t cmd_id, const uint8_t *data, uint32_t len)
{
    LOG_INFO("App", "收到卫星命令: 0x%X, 长度: %u", cmd_id, len);
    /* 处理命令 */
}

satellite_pdl_config_t config = {
    .can_device = "can0",
    .can_bitrate = 500000,
    .heartbeat_interval_ms = 5000,
    .cmd_callback = cmd_callback
};

SatellitePDL_Init(&config);

/* 发送遥测 */
uint8_t telemetry[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
SatellitePDL_SendTelemetry(0x200, telemetry, sizeof(telemetry));

SatellitePDL_Deinit();
```

## BMC载荷服务

### 双通道配置
```c
#include "pdl_bmc.h"

bmc_pdl_config_t config = {
    .primary_channel = {
        .type = BMC_CHANNEL_ETHERNET,
        .ip_addr = "192.168.1.100",
        .port = 623
    },
    .backup_channel = {
        .type = BMC_CHANNEL_UART,
        .device = "/dev/ttyS2",
        .baudrate = 115200
    },
    .failover_threshold = 5
};

BMCPDL_Init(&config);

/* 电源控制 */
BMCPDL_PowerOn();
BMCPDL_PowerOff();
BMCPDL_Reset();

BMCPDL_Deinit();
```

## MCU外设服务

### 基本使用
```c
#include "pdl_mcu.h"

mcu_pdl_config_t config = {
    .interface_type = MCU_INTERFACE_UART,
    .uart_device = "/dev/ttyS1",
    .uart_baudrate = 115200
};

MCUPDL_Init(&config);

/* 查询版本 */
char version[32];
MCUPDL_GetVersion(version, sizeof(version));
LOG_INFO("App", "MCU版本: %s", version);

MCUPDL_Deinit();
```

更多示例请参考测试代码 `tests/src/pdl/`。
