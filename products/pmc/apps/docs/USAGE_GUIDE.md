# Apps 使用指南

## 创建新应用

### 步骤1：创建应用目录

```bash
cd apps
mkdir my_app
cd my_app
mkdir src include
```

### 步骤2：创建CMakeLists.txt

```cmake
# my_app/CMakeLists.txt
add_executable(my_app
    src/main.c
)

target_link_libraries(my_app
    osal
    hal
    pdl
    pcl
)

target_include_directories(my_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

### 步骤3：创建main.c

```c
#include "osal.h"

int main(void)
{
    LOG_INFO("MyApp", "应用启动");
    
    /* 应用逻辑 */
    OSAL_TaskDelay(5000);
    
    LOG_INFO("MyApp", "应用退出");
    return 0;
}
```

### 步骤4：添加到总构建

编辑 `apps/CMakeLists.txt`，添加：
```cmake
add_subdirectory(my_app)
```

### 步骤5：编译运行

```bash
./build.sh
./build/bin/my_app
```

## 使用HAL层

```c
#include "osal.h"
#include "hal_can.h"

int main(void)
{
    /* 初始化CAN */
    hal_can_config_t config = {
        .device = "can0",
        .bitrate = 500000
    };
    
    hal_can_handle_t handle;
    HAL_CAN_Init(&config, &handle);
    
    /* 发送CAN帧 */
    hal_can_frame_t frame = {
        .can_id = 0x123,
        .can_dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };
    HAL_CAN_Send(&handle, &frame, 1000);
    
    /* 清理 */
    HAL_CAN_Deinit(&handle);
    
    return 0;
}
```

## 使用PDL层

```c
#include "osal.h"
#include "pdl_satellite.h"

void cmd_callback(uint32_t cmd_id, const uint8_t *data, uint32_t len)
{
    LOG_INFO("App", "收到命令: 0x%X", cmd_id);
}

int main(void)
{
    /* 初始化卫星平台服务 */
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
    
    /* 运行 */
    OSAL_TaskDelay(60000);
    
    /* 清理 */
    SatellitePDL_Deinit();
    
    return 0;
}
```

## 使用PCL层

```c
#include "osal.h"
#include "pcl_api.h"
#include "hal_serial.h"

int main(void)
{
    /* 初始化PCL */
    HW_Config_Init();
    HW_Config_RegisterAll();
    
    /* 获取板级配置 */
    const pcl_board_config_t *board = HW_Config_SelectDefault();
    
    /* 查找MCU配置 */
    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(board, "stm32_mcu");
    if (mcu != NULL && mcu->interface_type == HW_INTERFACE_UART) {
        /* 使用配置初始化串口 */
        hal_serial_config_t serial_cfg = {
            .device = mcu->interface_cfg.uart.device,
            .baudrate = mcu->interface_cfg.uart.baudrate,
            .data_bits = mcu->interface_cfg.uart.data_bits,
            .stop_bits = mcu->interface_cfg.uart.stop_bits,
            .parity = mcu->interface_cfg.uart.parity
        };
        
        hal_serial_handle_t handle;
        HAL_Serial_Init(&serial_cfg, &handle);
        
        /* 使用串口 */
        
        HAL_Serial_Deinit(&handle);
    }
    
    return 0;
}
```

## 参考示例

完整的应用示例请参考：
- [sample_app](../sample_app/README.md) - 基础示例
- [sample_app/src/main.c](../sample_app/src/main.c) - 源代码

## 相关文档

- [架构设计](ARCHITECTURE.md)
- [OSAL使用指南](../../../../core/osal/docs/USAGE_GUIDE.md)
- [HAL使用指南](../../../../core/hal/docs/USAGE_GUIDE.md)
- [PDL使用指南](../../../../core/pdl/docs/USAGE_GUIDE.md)
