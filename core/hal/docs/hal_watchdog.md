# HAL Watchdog 驱动

## 概述

HAL Watchdog 驱动提供了统一的看门狗硬件访问接口，支持：
- 初始化和配置看门狗设备
- 定期喂狗（重置看门狗定时器）
- 设置和获取超时时间
- 启用/禁用看门狗
- 获取统计信息

## 文件结构

```
hal/
├── include/
│   └── hal_watchdog.h              # 接口定义
└── src/
    ├── linux/
    │   └── hal_watchdog.c          # Linux实现
    └── macos/
        └── hal_watchdog.c          # macOS桩实现

tests/hal/
└── test_hal_watchdog.c             # 单元测试

apps/watchdog_app/
├── src/
│   └── main.c                      # 喂狗应用
└── CMakeLists.txt
```

## API 接口

### 初始化和清理

```c
int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle);
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle);
```

### 喂狗操作

```c
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle);
```

### 配置管理

```c
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle);
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle);
int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec);
int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec);
int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec);
```

### 统计信息

```c
int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count);
```

## 使用示例

### 基本用法

```c
#include "hal_watchdog.h"
#include "osal/osal.h"

hal_watchdog_handle_t wdt_handle;
hal_watchdog_config_t config = {
    .device = "/dev/watchdog",
    .timeout_sec = 60,          // 60秒超时
    .enable_on_init = true      // 初始化时启用
};

// 初始化
int32_t ret = HAL_WATCHDOG_Init(&config, &wdt_handle);
if (ret != OSAL_SUCCESS) {
    LOG_ERROR("APP", "Watchdog init failed");
    return ret;
}

// 定期喂狗
while (running) {
    ret = HAL_WATCHDOG_Kick(wdt_handle);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("APP", "Watchdog kick failed");
    }
    OSAL_msleep(5000);  // 每5秒喂一次
}

// 清理
HAL_WATCHDOG_Deinit(wdt_handle);
```

## 喂狗应用

项目提供了一个完整的喂狗应用示例 `watchdog_app`。

### 编译

```bash
./build.sh
```

### 运行

```bash
# 需要root权限访问看门狗设备
sudo ./build/release/bin/watchdog_app
```

### 功能特性

- **自动喂狗**：每5秒自动喂狗一次
- **统计监控**：每30秒打印统计信息（喂狗次数、超时时间、剩余时间）
- **优雅退出**：支持Ctrl+C优雅退出，自动禁用看门狗

### 配置参数

在 `apps/watchdog_app/src/main.c` 中可以修改：

```c
#define WATCHDOG_KICK_INTERVAL_MS  5000   // 喂狗间隔（毫秒）
#define STATS_INTERVAL_MS          30000  // 统计信息打印间隔（毫秒）

hal_watchdog_config_t config = {
    .device = "/dev/watchdog",
    .timeout_sec = 60,          // 超时时间（秒）
    .enable_on_init = true      // 初始化时启用
};
```

## 测试

### 运行测试

```bash
# 交互式菜单
./build/release/bin/unit-test -i

# 运行所有HAL层测试
./build/release/bin/unit-test -L HAL

# 运行Watchdog测试
./build/release/bin/unit-test -m test_hal_watchdog
```

### 测试用例

- `test_hal_watchdog_init_deinit` - 初始化和反初始化
- `test_hal_watchdog_null_params` - NULL参数检查
- `test_hal_watchdog_kick` - 喂狗功能
- `test_hal_watchdog_timeout` - 超时时间设置和获取
- `test_hal_watchdog_timeleft` - 获取剩余时间
- `test_hal_watchdog_enable_disable` - 启用和禁用
- `test_hal_watchdog_invalid_handle` - 无效句柄

**注意**：测试需要 `/dev/watchdog` 设备存在，如果设备不可用，测试会自动跳过。

## Linux 看门狗设备

### 检查设备

```bash
ls -l /dev/watchdog*
```

### 加载驱动

```bash
# 加载软件看门狗（用于测试）
sudo modprobe softdog

# 检查
ls -l /dev/watchdog
```

### 设备权限

```bash
# 临时修改权限
sudo chmod 666 /dev/watchdog

# 或使用root权限运行应用
sudo ./build/release/bin/watchdog_app
```

## 注意事项

1. **硬件限制**：某些看门狗硬件一旦启用就无法禁用，只能通过喂狗或系统重启来控制
2. **超时时间**：不同硬件支持的超时时间范围不同，驱动会自动调整到硬件支持的最接近值
3. **剩余时间**：`GetTimeleft` 功能不是所有硬件都支持，如果不支持会返回 `OSAL_ERR_NOT_SUPPORTED`
4. **权限要求**：访问 `/dev/watchdog` 通常需要root权限
5. **喂狗频率**：喂狗间隔应该远小于超时时间，建议为超时时间的1/10到1/3

## 平台支持

- **Linux**：完整实现，支持所有功能
- **macOS**：桩实现，仅用于编译，所有函数返回 `OSAL_ERR_NOT_SUPPORTED`
- **其他平台**：需要在 `hal/src/<platform>/` 目录下实现对应的驱动

## 错误码

- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - 参数为NULL
- `OSAL_ERR_GENERIC` - 通用错误（设备打开失败、ioctl失败等）
- `OSAL_ERR_NOT_SUPPORTED` - 功能不支持（硬件或平台限制）
