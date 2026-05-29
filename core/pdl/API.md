# PDL API 参考文档

PDL (Peripheral Driver Layer) 提供外设驱动和协议封装接口。

---

## 📋 目录

1. [卫星通信](#卫星通信)
2. [BMC 管理](#bmc-管理)
3. [MCU 通信](#mcu-通信)
4. [看门狗服务](#看门狗服务)

---

## 卫星通信

### PDL_Satellite_Init

初始化卫星平台服务。

```c
int32_t PDL_Satellite_Init(
    const satellite_service_config_t *config,
    satellite_service_handle_t *handle
);
```

**参数**:
- `config`: 服务配置
  - `can_device`: CAN 设备名（如 "can0"）
  - `can_bitrate`: CAN 波特率（如 500000）
  - `heartbeat_interval_ms`: 心跳间隔（毫秒）
  - `cmd_timeout_ms`: 命令超时（毫秒）
- `handle`: 返回的服务句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
satellite_service_config_t config = {
    .can_device = "can0",
    .can_bitrate = 500000,
    .heartbeat_interval_ms = 1000,
    .cmd_timeout_ms = 5000
};

satellite_service_handle_t handle = NULL;
int32_t ret = PDL_Satellite_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize satellite service\n");
    return -1;
}
```

---

### PDL_Satellite_Deinit

反初始化卫星平台服务。

```c
int32_t PDL_Satellite_Deinit(satellite_service_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_Satellite_RegisterCallback

注册命令回调函数。

```c
int32_t PDL_Satellite_RegisterCallback(
    satellite_service_handle_t handle,
    satellite_cmd_callback_t callback,
    void *user_data
);
```

**参数**:
- `handle`: 服务句柄
- `callback`: 回调函数
- `user_data`: 用户数据

**返回值**:
- `OSAL_SUCCESS`: 成功

**回调函数原型**:
```c
void callback(uint8_t cmd_type, uint32_t param, void *user_data);
```

**示例**:
```c
void cmd_handler(uint8_t cmd_type, uint32_t param, void *user_data)
{
    printf("Received command: type=%d, param=%u\n", cmd_type, param);
    
    // 处理命令...
    
    // 发送响应
    satellite_service_handle_t handle = (satellite_service_handle_t)user_data;
    PDL_Satellite_SendResponse(handle, param, STATUS_OK, 0);
}

PDL_Satellite_RegisterCallback(handle, cmd_handler, handle);
```

---

### PDL_Satellite_SendResponse

发送响应到卫星平台。

```c
int32_t PDL_Satellite_SendResponse(
    satellite_service_handle_t handle,
    uint32_t seq_num,
    can_status_t status,
    uint32_t result
);
```

**参数**:
- `handle`: 服务句柄
- `seq_num`: 序列号
- `status`: 状态码
  - `STATUS_OK`: 成功
  - `STATUS_ERROR`: 错误
  - `STATUS_BUSY`: 忙碌
  - `STATUS_TIMEOUT`: 超时
- `result`: 结果数据

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_Satellite_SendHeartbeat

发送心跳到卫星平台。

```c
int32_t PDL_Satellite_SendHeartbeat(
    satellite_service_handle_t handle,
    can_status_t status
);
```

**参数**:
- `handle`: 服务句柄
- `status`: 当前状态

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
// 定期发送心跳
while (running) {
    PDL_Satellite_SendHeartbeat(handle, STATUS_OK);
    OSAL_Sleep(1000);
}
```

---

### PDL_Satellite_GetStats

获取服务统计信息。

```c
int32_t PDL_Satellite_GetStats(
    satellite_service_handle_t handle,
    uint32_t *rx_count,
    uint32_t *tx_count,
    uint32_t *error_count
);
```

**参数**:
- `handle`: 服务句柄
- `rx_count`: 接收计数（输出）
- `tx_count`: 发送计数（输出）
- `error_count`: 错误计数（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功

---

## BMC 管理

### PDL_BMC_Init

初始化 BMC 服务。

```c
int32_t PDL_BMC_Init(
    const bmc_config_t *config,
    bmc_handle_t *handle
);
```

**参数**:
- `config`: BMC 配置
  - `network`: 网络配置
    - `enabled`: 是否启用
    - `ip_addr`: IP 地址
    - `port`: 端口（默认 623）
    - `username`: 用户名
    - `password`: 密码
    - `timeout_ms`: 超时时间
  - `serial`: 串口配置
    - `enabled`: 是否启用
    - `device`: 串口设备
    - `baudrate`: 波特率
    - `timeout_ms`: 超时时间
  - `primary_channel`: 主通道
    - `BMC_CHANNEL_NETWORK`: 网络通道
    - `BMC_CHANNEL_SERIAL`: 串口通道
  - `auto_switch`: 自动切换通道
  - `retry_count`: 重试次数
  - `health_check_interval`: 健康检查间隔（毫秒）
- `handle`: 返回的服务句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
bmc_config_t config = {
    .network = {
        .enabled = true,
        .ip_addr = "192.168.1.100",
        .port = 623,
        .username = "admin",
        .password = "admin",
        .timeout_ms = 5000
    },
    .serial = {
        .enabled = false
    },
    .primary_channel = BMC_CHANNEL_NETWORK,
    .auto_switch = true,
    .retry_count = 3,
    .health_check_interval = 10000
};

bmc_handle_t handle = NULL;
int32_t ret = PDL_BMC_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize BMC service\n");
    return -1;
}
```

---

### PDL_BMC_Deinit

反初始化 BMC 服务。

```c
int32_t PDL_BMC_Deinit(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_BMC_PowerOn

电源开机。

```c
int32_t PDL_BMC_PowerOn(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
int32_t ret = PDL_BMC_PowerOn(handle);
if (ret == OSAL_SUCCESS) {
    printf("Power on successful\n");
} else {
    printf("Power on failed\n");
}
```

---

### PDL_BMC_PowerOff

电源关机。

```c
int32_t PDL_BMC_PowerOff(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_BMC_PowerReset

电源复位。

```c
int32_t PDL_BMC_PowerReset(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_BMC_GetPowerState

查询电源状态。

```c
int32_t PDL_BMC_GetPowerState(
    bmc_handle_t handle,
    bmc_power_state_t *state
);
```

**参数**:
- `handle`: 服务句柄
- `state`: 电源状态（输出）
  - `BMC_POWER_OFF`: 关机
  - `BMC_POWER_ON`: 开机
  - `BMC_POWER_UNKNOWN`: 未知

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
bmc_power_state_t state;
int32_t ret = PDL_BMC_GetPowerState(handle, &state);
if (ret == OSAL_SUCCESS) {
    printf("Power state: %s\n", 
           state == BMC_POWER_ON ? "ON" : "OFF");
}
```

---

### PDL_BMC_ReadSensors

读取传感器数据。

```c
int32_t PDL_BMC_ReadSensors(
    bmc_handle_t handle,
    bmc_sensor_type_t type,
    bmc_sensor_reading_t *readings,
    uint32_t max_count,
    uint32_t *actual_count
);
```

**参数**:
- `handle`: 服务句柄
- `type`: 传感器类型
  - `BMC_SENSOR_TEMP`: 温度
  - `BMC_SENSOR_VOLTAGE`: 电压
  - `BMC_SENSOR_CURRENT`: 电流
  - `BMC_SENSOR_FAN`: 风扇转速
- `readings`: 读数数组（输出）
- `max_count`: 数组大小
- `actual_count`: 实际读取数量（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
bmc_sensor_reading_t readings[10];
uint32_t count;

int32_t ret = PDL_BMC_ReadSensors(handle, BMC_SENSOR_TEMP, 
                                   readings, 10, &count);
if (ret == OSAL_SUCCESS) {
    for (uint32_t i = 0; i < count; i++) {
        printf("%s: %.2f %s\n", 
               readings[i].name, 
               readings[i].value, 
               readings[i].unit);
    }
}
```

---

### PDL_BMC_ExecuteCommand

执行原始 IPMI 命令。

```c
int32_t PDL_BMC_ExecuteCommand(
    bmc_handle_t handle,
    const char *cmd,
    char *response,
    uint32_t resp_size
);
```

**参数**:
- `handle`: 服务句柄
- `cmd`: 命令字符串
- `response`: 响应缓冲区（输出）
- `resp_size`: 缓冲区大小

**返回值**:
- `>= 0`: 实际接收字节数
- `< 0`: 错误码

**示例**:
```c
char response[256];
int32_t len = PDL_BMC_ExecuteCommand(handle, "chassis status", 
                                      response, sizeof(response));
if (len > 0) {
    printf("Response: %s\n", response);
}
```

---

### PDL_BMC_SwitchChannel

切换通信通道。

```c
int32_t PDL_BMC_SwitchChannel(
    bmc_handle_t handle,
    bmc_channel_t channel
);
```

**参数**:
- `handle`: 服务句柄
- `channel`: 目标通道

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_BMC_GetChannel

获取当前通道。

```c
bmc_channel_t PDL_BMC_GetChannel(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- 当前通道

---

### PDL_BMC_IsConnected

检查连接状态。

```c
bool PDL_BMC_IsConnected(bmc_handle_t handle);
```

**参数**:
- `handle`: 服务句柄

**返回值**:
- `true`: 已连接
- `false`: 未连接

---

### PDL_BMC_GetStats

获取服务统计信息。

```c
int32_t PDL_BMC_GetStats(
    bmc_handle_t handle,
    uint32_t *cmd_count,
    uint32_t *success_count,
    uint32_t *fail_count,
    uint32_t *switch_count
);
```

**参数**:
- `handle`: 服务句柄
- `cmd_count`: 命令总数（输出）
- `success_count`: 成功数（输出）
- `fail_count`: 失败数（输出）
- `switch_count`: 通道切换次数（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功

---

## MCU 通信

### PDL_MCU_Init

初始化 MCU 驱动。

```c
int32_t PDL_MCU_Init(
    const mcu_config_t *config,
    mcu_handle_t *handle
);
```

**参数**:
- `config`: MCU 配置
  - `name`: MCU 名称
  - `interface`: 通信接口
    - `MCU_INTERFACE_CAN`: CAN 总线
    - `MCU_INTERFACE_SERIAL`: 串口
    - `MCU_INTERFACE_I2C`: I2C（预留）
    - `MCU_INTERFACE_SPI`: SPI（预留）
  - `can`: CAN 配置
    - `device`: CAN 设备
    - `bitrate`: 波特率
    - `tx_id`: 发送 CAN ID
    - `rx_id`: 接收 CAN ID
  - `serial`: 串口配置
    - `device`: 串口设备
    - `baudrate`: 波特率
    - `data_bits`: 数据位
    - `stop_bits`: 停止位
    - `parity`: 校验位
  - `cmd_timeout_ms`: 命令超时
  - `retry_count`: 重试次数
  - `enable_crc`: 启用 CRC 校验
- `handle`: 返回的 MCU 句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例（CAN 接口）**:
```c
mcu_config_t config = {
    .name = "MCU1",
    .interface = MCU_INTERFACE_CAN,
    .can = {
        .device = "can0",
        .bitrate = 500000,
        .tx_id = 0x100,
        .rx_id = 0x101
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_crc = true
};

mcu_handle_t handle = NULL;
int32_t ret = PDL_MCU_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize MCU\n");
    return -1;
}
```

**示例（串口接口）**:
```c
mcu_config_t config = {
    .name = "MCU2",
    .interface = MCU_INTERFACE_SERIAL,
    .serial = {
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0  // 无校验
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_crc = true
};

mcu_handle_t handle = NULL;
PDL_MCU_Init(&config, &handle);
```

---

### PDL_MCU_Deinit

反初始化 MCU 驱动。

```c
int32_t PDL_MCU_Deinit(mcu_handle_t handle);
```

**参数**:
- `handle`: MCU 句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_MCU_GetVersion

获取 MCU 版本。

```c
int32_t PDL_MCU_GetVersion(
    mcu_handle_t handle,
    mcu_version_t *version
);
```

**参数**:
- `handle`: MCU 句柄
- `version`: 版本信息（输出）
  - `major`: 主版本号
  - `minor`: 次版本号
  - `patch`: 修订号
  - `build`: 构建号
  - `version_string`: 版本字符串

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
mcu_version_t version;
int32_t ret = PDL_MCU_GetVersion(handle, &version);
if (ret == OSAL_SUCCESS) {
    printf("MCU Version: %d.%d.%d.%d (%s)\n",
           version.major, version.minor, version.patch, version.build,
           version.version_string);
}
```

---

### PDL_MCU_GetStatus

获取 MCU 状态。

```c
int32_t PDL_MCU_GetStatus(
    mcu_handle_t handle,
    mcu_status_t *status
);
```

**参数**:
- `handle`: MCU 句柄
- `status`: 状态信息（输出）
  - `online`: 在线状态
  - `uptime_sec`: 运行时间（秒）
  - `error_code`: 错误码
  - `temperature`: 温度（℃）
  - `voltage_mv`: 电压（毫伏）
  - `timestamp_us`: 数据采集时间戳（微秒）

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
mcu_status_t status;
int32_t ret = PDL_MCU_GetStatus(handle, &status);
if (ret == OSAL_SUCCESS) {
    printf("MCU Status:\n");
    printf("  Online: %s\n", status.online ? "Yes" : "No");
    printf("  Uptime: %u seconds\n", status.uptime_sec);
    printf("  Temperature: %.2f C\n", status.temperature);
    printf("  Voltage: %u mV\n", status.voltage_mv);
}
```

---

### PDL_MCU_Reset

MCU 复位。

```c
int32_t PDL_MCU_Reset(mcu_handle_t handle);
```

**参数**:
- `handle`: MCU 句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### PDL_MCU_ReadRegister

读取 MCU 寄存器。

```c
int32_t PDL_MCU_ReadRegister(
    mcu_handle_t handle,
    uint8_t reg_addr,
    uint8_t *value
);
```

**参数**:
- `handle`: MCU 句柄
- `reg_addr`: 寄存器地址
- `value`: 读取值（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
uint8_t value;
int32_t ret = PDL_MCU_ReadRegister(handle, 0x10, &value);
if (ret == OSAL_SUCCESS) {
    printf("Register 0x10 = 0x%02X\n", value);
}
```

---

### PDL_MCU_WriteRegister

写入 MCU 寄存器。

```c
int32_t PDL_MCU_WriteRegister(
    mcu_handle_t handle,
    uint8_t reg_addr,
    uint8_t value
);
```

**参数**:
- `handle`: MCU 句柄
- `reg_addr`: 寄存器地址
- `value`: 写入值

**返回值**:
- `OSAL_SUCCESS`: 成功

**示例**:
```c
// 写入寄存器 0x10
int32_t ret = PDL_MCU_WriteRegister(handle, 0x10, 0x42);
```

---

### PDL_MCU_SendCommand

发送自定义命令到 MCU。

```c
int32_t PDL_MCU_SendCommand(
    mcu_handle_t handle,
    uint8_t cmd_code,
    const uint8_t *data,
    uint32_t data_len,
    uint8_t *response,
    uint32_t resp_size,
    uint32_t *actual_size
);
```

**参数**:
- `handle`: MCU 句柄
- `cmd_code`: 命令码
- `data`: 命令数据
- `data_len`: 数据长度
- `response`: 响应缓冲区（输出）
- `resp_size`: 缓冲区大小
- `actual_size`: 实际响应长度（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
uint8_t cmd_data[] = {0x01, 0x02, 0x03};
uint8_t response[64];
uint32_t resp_len;

int32_t ret = PDL_MCU_SendCommand(handle, 0x10, 
                                   cmd_data, sizeof(cmd_data),
                                   response, sizeof(response), &resp_len);
if (ret == OSAL_SUCCESS) {
    printf("Response length: %u\n", resp_len);
}
```

---

### PDL_MCU_FirmwareUpdate

MCU 固件升级。

```c
int32_t PDL_MCU_FirmwareUpdate(
    mcu_handle_t handle,
    const char *firmware_path,
    void (*progress_callback)(uint32_t percent)
);
```

**参数**:
- `handle`: MCU 句柄
- `firmware_path`: 固件文件路径
- `progress_callback`: 进度回调函数（可选）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
void update_progress(uint32_t percent)
{
    printf("Firmware update progress: %u%%\n", percent);
}

int32_t ret = PDL_MCU_FirmwareUpdate(handle, 
                                      "/path/to/firmware.bin",
                                      update_progress);
if (ret == OSAL_SUCCESS) {
    printf("Firmware update successful\n");
}
```

---

## 看门狗服务

### PDL_WATCHDOG_Init

初始化看门狗服务。

```c
int32_t PDL_WATCHDOG_Init(
    const watchdog_config_t *config,
    watchdog_handle_t *handle
);
```

**参数**:
- `config`: 看门狗配置
  - `name`: 看门狗名称
  - `device`: 设备路径（如 "/dev/watchdog"）
  - `timeout_sec`: 超时时间（秒）
  - `mode`: 工作模式
    - `WATCHDOG_MODE_MANUAL`: 手动模式（应用自己调用 Kick）
    - `WATCHDOG_MODE_AUTO`: 自动模式（PDL 内部线程自动喂狗）
  - `kick_interval_ms`: 自动模式下的喂狗间隔（毫秒）
  - `enable_on_init`: 初始化时是否启用看门狗
- `handle`: 返回的看门狗句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

**示例（自动模式）**:
```c
watchdog_config_t config = {
    .name = "system_watchdog",
    .device = "/dev/watchdog",
    .timeout_sec = 30,
    .mode = WATCHDOG_MODE_AUTO,
    .kick_interval_ms = 10000,  // 每 10 秒喂狗
    .enable_on_init = true
};

watchdog_handle_t handle = NULL;
int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize watchdog\n");
    return -1;
}

// 启动自动喂狗服务
PDL_WATCHDOG_Start(handle);
```

**示例（手动模式）**:
```c
watchdog_config_t config = {
    .name = "manual_watchdog",
    .device = "/dev/watchdog",
    .timeout_sec = 60,
    .mode = WATCHDOG_MODE_MANUAL,
    .enable_on_init = true
};

watchdog_handle_t handle = NULL;
PDL_WATCHDOG_Init(&config, &handle);

// 应用程序需要定期调用 PDL_WATCHDOG_Kick
while (running) {
    // 执行任务...
    
    PDL_WATCHDOG_Kick(handle);
    OSAL_Sleep(20000);  // 20 秒
}
```

---

### PDL_WATCHDOG_Deinit

关闭看门狗服务。

```c
int32_t PDL_WATCHDOG_Deinit(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL

---

### PDL_WATCHDOG_Start

启动自动喂狗服务。

```c
int32_t PDL_WATCHDOG_Start(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

**说明**: 仅在自动模式下有效，启动内部喂狗线程。

---

### PDL_WATCHDOG_Stop

停止自动喂狗服务。

```c
int32_t PDL_WATCHDOG_Stop(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL

**说明**: 仅在自动模式下有效，停止内部喂狗线程。

---

### PDL_WATCHDOG_Kick

手动喂狗。

```c
int32_t PDL_WATCHDOG_Kick(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

**说明**: 在手动模式下使用，或在自动模式下额外喂狗。

---

### PDL_WATCHDOG_GetStatus

获取看门狗状态。

```c
int32_t PDL_WATCHDOG_GetStatus(
    watchdog_handle_t handle,
    watchdog_status_t *status
);
```

**参数**:
- `handle`: 看门狗句柄
- `status`: 状态信息（输出）
  - `enabled`: 是否已启用
  - `running`: 自动喂狗线程是否运行中
  - `timeout_sec`: 超时时间（秒）
  - `kick_count`: 喂狗次数
  - `kick_interval_ms`: 喂狗间隔（毫秒）
  - `mode`: 工作模式
  - `last_kick_time_us`: 上次喂狗时间戳（微秒）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL

**示例**:
```c
watchdog_status_t status;
int32_t ret = PDL_WATCHDOG_GetStatus(handle, &status);
if (ret == OSAL_SUCCESS) {
    printf("Watchdog Status:\n");
    printf("  Enabled: %s\n", status.enabled ? "Yes" : "No");
    printf("  Running: %s\n", status.running ? "Yes" : "No");
    printf("  Timeout: %u seconds\n", status.timeout_sec);
    printf("  Kick count: %u\n", status.kick_count);
    printf("  Mode: %s\n", 
           status.mode == WATCHDOG_MODE_AUTO ? "Auto" : "Manual");
}
```

---

### PDL_WATCHDOG_SetInterval

设置喂狗间隔。

```c
int32_t PDL_WATCHDOG_SetInterval(
    watchdog_handle_t handle,
    uint32_t interval_ms
);
```

**参数**:
- `handle`: 看门狗句柄
- `interval_ms`: 喂狗间隔（毫秒）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

**说明**: 仅在自动模式下有效。

---

### PDL_WATCHDOG_Enable

启用看门狗。

```c
int32_t PDL_WATCHDOG_Enable(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

---

### PDL_WATCHDOG_Disable

禁用看门狗。

```c
int32_t PDL_WATCHDOG_Disable(watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_NOT_SUPPORTED`: 硬件不支持禁用
- `OSAL_ERR_INVALID_POINTER`: 参数为 NULL
- `OSAL_ERR_GENERIC`: 失败

**注意**: 某些硬件看门狗一旦启用就无法禁用。

---

## 错误码

| 错误码 | 说明 |
|--------|------|
| `OSAL_SUCCESS` | 成功 |
| `OSAL_ERR_GENERIC` | 一般错误 |
| `OSAL_ERR_INVALID_POINTER` | 参数为 NULL |
| `OSAL_ERR_TIMEOUT` | 超时 |
| `OSAL_ERR_NOT_SUPPORTED` | 不支持的操作 |

---

## 最佳实践

### 1. 资源管理

始终配对初始化和释放：

```c
// ✅ 正确
satellite_service_handle_t handle = NULL;
PDL_Satellite_Init(&config, &handle);
// 使用 handle...
PDL_Satellite_Deinit(handle);

// ❌ 错误 - 忘记释放
PDL_Satellite_Init(&config, &handle);
// 使用 handle...
// 忘记调用 PDL_Satellite_Deinit
```

### 2. 错误处理

检查返回值：

```c
// ✅ 正确
int32_t ret = PDL_BMC_PowerOn(handle);
if (ret != OSAL_SUCCESS) {
    OSAL_LOG_ERROR("Power on failed: %d", ret);
    return -1;
}

// ❌ 错误 - 不检查返回值
PDL_BMC_PowerOn(handle);
```

### 3. 看门狗使用

选择合适的工作模式：

```c
// ✅ 正确 - 自动模式适合简单应用
watchdog_config_t config = {
    .mode = WATCHDOG_MODE_AUTO,
    .kick_interval_ms = 10000
};
PDL_WATCHDOG_Init(&config, &handle);
PDL_WATCHDOG_Start(handle);

// ✅ 正确 - 手动模式适合复杂应用
watchdog_config_t config = {
    .mode = WATCHDOG_MODE_MANUAL
};
PDL_WATCHDOG_Init(&config, &handle);

while (running) {
    if (all_tasks_healthy()) {
        PDL_WATCHDOG_Kick(handle);
    }
    OSAL_Sleep(10000);
}
```

### 4. MCU 通信

选择合适的通信接口：

```c
// ✅ 正确 - 根据硬件选择接口
mcu_config_t config = {
    .interface = MCU_INTERFACE_CAN,  // 或 MCU_INTERFACE_SERIAL
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_crc = true  // 启用 CRC 提高可靠性
};
```

### 5. BMC 管理

配置通道自动切换：

```c
// ✅ 正确 - 启用自动切换提高可靠性
bmc_config_t config = {
    .network = { .enabled = true, /* ... */ },
    .serial = { .enabled = true, /* ... */ },
    .primary_channel = BMC_CHANNEL_NETWORK,
    .auto_switch = true,  // 网络失败时自动切换到串口
    .retry_count = 3
};
```

---

## 平台支持

| 平台 | Satellite | BMC | MCU | Watchdog |
|------|-----------|-----|-----|----------|
| Linux | ✅ | ✅ | ✅ | ✅ |
| macOS | ❌ | ❌ | ❌ | ❌ |
| Windows | 🚧 | 🚧 | 🚧 | 🚧 |
| RTOS | 🚧 | 🚧 | 🚧 | 🚧 |

**说明**:
- ✅ 完全支持
- 🚧 开发中
- ❌ 不支持

---

## 相关文档

- [HAL API 参考](../hal/API.md)
- [OSAL API 参考](../osal/API.md)
- [开发者指南](../../docs/DEVELOPER_GUIDE.md)
- [架构文档](../../docs/ARCHITECTURE.md)

---

**版本**: 1.0.0  
**最后更新**: 2026-05-29
