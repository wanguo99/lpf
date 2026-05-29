# EMS 模块命名规范统一总结

## 优化目标

统一所有模块的数据类型和结构体命名规范，确保命名空间隔离和代码可维护性。

## 命名规范

### 规则

所有模块的类型定义必须使用模块前缀：

| 模块 | 前缀 | 示例 |
|------|------|------|
| **OSAL** | `osal_` | `osal_thread_t`, `osal_mutex_t` |
| **HAL** | `hal_` | `hal_can_config_t`, `hal_serial_handle_t` |
| **PCL** | `pcl_` | `pcl_mcu_entry_t`, `pcl_bmc_entry_t` |
| **PDL** | `pdl_` | `pdl_mcu_config_t`, `pdl_bmc_handle_t` |
| **ACL** | `acl_` | `acl_device_type_t`, `acl_tm_config_t` |

### 格式

- **结构体**: `<模块>_<名称>_t`
- **枚举**: `<模块>_<名称>_t`
- **句柄**: `<模块>_<名称>_handle_t`
- **枚举常量**: `<模块>_<名称>_<值>`

## 本次修改

### HAL 配置类型文件

**修改文件**（3 个）：
- `core/hal/include/config/can_types.h`
- `core/hal/include/config/spi_types.h`
- `core/hal/include/config/i2c_types.h`

**修改内容**：

| 旧命名 | 新命名 | 说明 |
|--------|--------|------|
| `can_frame_t` | `hal_can_frame_t` | CAN 帧结构 |
| `spi_transfer_t` | `hal_spi_transfer_t` | SPI 传输消息 |
| `i2c_msg_t` | `hal_i2c_msg_t` | I2C 传输消息 |

### HAL 头文件

**修改文件**（3 个）：
- `core/hal/include/hal_can.h`
- `core/hal/include/hal_spi.h`
- `core/hal/include/hal_i2c.h`

**修改内容**：更新函数声明中的类型引用

### HAL 实现文件

**修改文件**（3 个）：
- `core/hal/src/linux/hal_can_linux.c`
- `core/hal/src/linux/hal_spi_linux.c`
- `core/hal/src/linux/hal_i2c_linux.c`

**修改内容**：更新函数实现中的类型引用

## 各模块命名现状

### OSAL 层

✅ **已符合规范**

```c
// 类型定义
typedef void* osal_thread_t;
typedef void* osal_mutex_t;
typedef void* osal_semaphore_t;
typedef uint32_t osal_id_t;
typedef int64_t osal_time_t;
```

### HAL 层

✅ **已符合规范**（本次修改后）

```c
// 句柄类型
typedef void* hal_can_handle_t;
typedef void* hal_serial_handle_t;
typedef void* hal_i2c_handle_t;
typedef void* hal_spi_handle_t;
typedef void* hal_gpio_handle_t;
typedef void* hal_watchdog_handle_t;

// 配置结构
typedef struct {
    const char *interface;
    uint32_t    baudrate;
    uint32_t    rx_timeout;
    uint32_t    tx_timeout;
} hal_can_config_t;

typedef struct {
    uint32_t baud_rate;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    uint8_t  parity;
    uint8_t  flow_control;
} hal_serial_config_t;

// 数据类型
typedef struct {
    uint32_t can_id;
    uint8_t  dlc;
    uint8_t  data[8];
    uint32_t timestamp;
} hal_can_frame_t;

typedef struct {
    const uint8_t *tx_buf;
    uint8_t       *rx_buf;
    uint32_t       len;
    uint32_t       speed_hz;
    uint16_t       delay_usecs;
    uint8_t        bits_per_word;
    uint8_t        cs_change;
} hal_spi_transfer_t;

typedef struct {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
} hal_i2c_msg_t;

// 枚举类型
typedef enum {
    HAL_GPIO_DIRECTION_IN = 0,
    HAL_GPIO_DIRECTION_OUT = 1
} hal_gpio_direction_t;

typedef enum {
    HAL_GPIO_LEVEL_LOW = 0,
    HAL_GPIO_LEVEL_HIGH = 1
} hal_gpio_level_t;
```

### PCL 层

✅ **已符合规范**

```c
// 配置容器
typedef struct {
    const char       *name;
    const char       *description;
    bool              enabled;
    pdl_mcu_config_t  config;
    pcl_gpio_config_t *reset_gpio;
    pcl_gpio_config_t *irq_gpio;
} pcl_mcu_entry_t;

typedef struct {
    const char       *name;
    const char       *description;
    bool              enabled;
    pdl_bmc_config_t  config;
    pcl_gpio_config_t *power_gpio;
    pcl_gpio_config_t *reset_gpio;
} pcl_bmc_entry_t;

// 通用类型
typedef struct {
    uint32_t gpio_num;
    uint32_t pin_mux;
    bool     active_low;
    bool     pull_up;
    bool     pull_down;
} pcl_gpio_config_t;

typedef struct {
    const char        *name;
    pcl_gpio_config_t *enable_gpio;
    uint32_t           voltage_mv;
    uint32_t           current_ma;
    uint32_t           startup_delay_ms;
} pcl_power_domain_t;

// 枚举类型
typedef enum {
    PCL_HW_INTERFACE_NONE = 0,
    PCL_HW_INTERFACE_CAN,
    PCL_HW_INTERFACE_UART,
    PCL_HW_INTERFACE_I2C,
    PCL_HW_INTERFACE_SPI,
    PCL_HW_INTERFACE_MAX
} pcl_hw_interface_type_t;
```

### PDL 层

✅ **已符合规范**（前一次修改）

```c
// MCU 模块
typedef void* pdl_mcu_handle_t;

typedef enum {
    PDL_MCU_INTERFACE_CAN = 0,
    PDL_MCU_INTERFACE_SERIAL = 1,
    PDL_MCU_INTERFACE_I2C = 2,
    PDL_MCU_INTERFACE_SPI = 3
} pdl_mcu_interface_t;

typedef struct {
    char                  name[64];
    pdl_mcu_interface_t   interface;
    // ... 配置字段
} pdl_mcu_config_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t build;
    char    version_string[32];
} pdl_mcu_version_t;

typedef struct {
    bool     online;
    uint32_t uptime_sec;
    uint8_t  error_code;
    float    temperature;
    uint16_t voltage_mv;
    uint64_t timestamp_us;
} pdl_mcu_status_t;

// BMC 模块
typedef void* pdl_bmc_handle_t;

typedef enum {
    PDL_BMC_CHANNEL_NETWORK = 0,
    PDL_BMC_CHANNEL_SERIAL  = 1
} pdl_bmc_channel_t;

typedef enum {
    PDL_BMC_PROTOCOL_IPMI = 0,
    PDL_BMC_PROTOCOL_REDFISH = 1
} pdl_bmc_protocol_t;

typedef enum {
    PDL_BMC_POWER_OFF = 0,
    PDL_BMC_POWER_ON  = 1,
    PDL_BMC_POWER_UNKNOWN = 2
} pdl_bmc_power_state_t;

typedef struct {
    pdl_bmc_power_state_t power_state;
    bool                  bmc_ready;
    uint32_t              uptime_sec;
    float                 cpu_temp;
    float                 inlet_temp;
    uint64_t              timestamp_us;
} pdl_bmc_status_t;

typedef struct {
    // ... 配置字段
} pdl_bmc_config_t;

typedef enum {
    PDL_BMC_SENSOR_TEMP = 0,
    PDL_BMC_SENSOR_VOLTAGE = 1,
    PDL_BMC_SENSOR_CURRENT = 2,
    PDL_BMC_SENSOR_FAN = 3
} pdl_bmc_sensor_type_t;

typedef struct {
    pdl_bmc_sensor_type_t type;
    char                  name[64];
    float                 value;
    char                  unit[16];
    bool                  valid;
    uint64_t              timestamp_us;
} pdl_bmc_sensor_reading_t;

// Satellite 模块
typedef void* pdl_satellite_handle_t;

typedef enum {
    PDL_SATELLITE_STATUS_OK = 0x00,
    PDL_SATELLITE_STATUS_ERROR = 0x01,
    PDL_SATELLITE_STATUS_BUSY = 0x02,
    PDL_SATELLITE_STATUS_TIMEOUT = 0x03
} pdl_satellite_status_t;

typedef struct {
    const char *can_device;
    uint32_t    can_bitrate;
    uint32_t    can_rx_timeout;
    uint32_t    can_tx_timeout;
    uint32_t    heartbeat_interval_ms;
    uint32_t    cmd_timeout_ms;
} pdl_satellite_config_t;

typedef void (*pdl_satellite_cmd_callback_t)(uint8_t cmd_type, uint32_t param, void *user_data);
```

### ACL 层

✅ **已符合规范**

```c
// 设备类型
typedef enum {
    ACL_DEVICE_SATELLITE = 0,
    ACL_DEVICE_BMC,
    ACL_DEVICE_MCU,
    ACL_DEVICE_FPGA,
    ACL_DEVICE_SWITCH,
    ACL_DEVICE_SENSOR,
    ACL_DEVICE_ACTUATOR,
    ACL_DEVICE_MAX
} acl_device_type_t;

// 遥测状态
typedef enum {
    ACL_TM_STATUS_INVALID = 0,
    ACL_TM_STATUS_FRESH = 1,
    ACL_TM_STATUS_STALE = 2
} acl_tm_status_t;

// 配置结构
typedef struct {
    uint32_t          function_id;
    acl_device_type_t device_type;
    uint32_t          logic_index;
    bool              enabled;
    void             *extra_data;
} acl_tc_config_t;

typedef struct {
    uint32_t          function_id;
    acl_device_type_t device_type;
    uint32_t          logic_index;
    uint32_t          validity_ms;
    uint32_t          update_period_ms;
    bool              enabled;
    void             *extra_data;
} acl_tm_config_t;

typedef struct {
    uint32_t  source_tm_id;
    uint32_t *affected_tm_ids;
    uint32_t  affected_count;
} acl_invalidation_map_t;

typedef struct {
    const char                   *name;
    const acl_tc_config_t        *tc_table;
    uint32_t                      tc_count;
    const acl_tm_config_t        *tm_table;
    uint32_t                      tm_count;
    const acl_invalidation_map_t *inv_map;
    uint32_t                      inv_count;
} acl_config_table_t;
```

## 编译验证

```bash
✅ 编译成功
[100%] Built target collector
[100%] Built target comm
```

## 命名规范优势

### 1. 命名空间隔离

- 避免不同模块之间的类型名冲突
- 清晰标识类型所属模块

### 2. 代码可读性

- 一眼看出类型来自哪个模块
- 便于理解代码的层次结构

### 3. 易于搜索

- 可以快速定位所有模块相关定义
- 便于代码审查和重构

### 4. 风格统一

- 所有模块遵循相同的命名规范
- 降低学习成本

## 命名规范检查清单

在添加新类型时，请确保：

- [ ] 类型名使用模块前缀（`osal_`, `hal_`, `pcl_`, `pdl_`, `acl_`）
- [ ] 结构体和枚举以 `_t` 结尾
- [ ] 句柄类型以 `_handle_t` 结尾
- [ ] 枚举常量使用模块前缀和大写（如 `HAL_GPIO_LEVEL_HIGH`）
- [ ] 函数名使用模块前缀和大写（如 `HAL_CAN_Init`, `PDL_MCU_Init`）
- [ ] 宏定义使用模块前缀和大写（如 `HAL_SERIAL_PARITY_NONE`）

## 示例

### 正确命名 ✅

```c
// HAL 层
typedef void* hal_can_handle_t;
typedef struct { ... } hal_can_config_t;
typedef struct { ... } hal_can_frame_t;
typedef enum { ... } hal_gpio_direction_t;
#define HAL_SERIAL_PARITY_NONE 0
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle);

// PDL 层
typedef void* pdl_mcu_handle_t;
typedef struct { ... } pdl_mcu_config_t;
typedef enum { ... } pdl_mcu_interface_t;
#define PDL_MCU_INTERFACE_CAN 0
int32_t PDL_MCU_Init(const pdl_mcu_config_t *config, pdl_mcu_handle_t *handle);

// PCL 层
typedef struct { ... } pcl_mcu_entry_t;
typedef struct { ... } pcl_gpio_config_t;
typedef enum { ... } pcl_hw_interface_type_t;
const pcl_mcu_entry_t* PCL_HW_FindMCU(const pcl_platform_config_t *platform, const char *name);
```

### 错误命名 ❌

```c
// 缺少模块前缀
typedef void* can_handle_t;           // ❌ 应该是 hal_can_handle_t
typedef struct { ... } mcu_config_t;  // ❌ 应该是 pdl_mcu_config_t
typedef enum { ... } device_type_t;   // ❌ 应该是 acl_device_type_t

// 枚举常量缺少前缀
typedef enum {
    INTERFACE_CAN = 0,                // ❌ 应该是 PDL_MCU_INTERFACE_CAN
    INTERFACE_SERIAL = 1              // ❌ 应该是 PDL_MCU_INTERFACE_SERIAL
} mcu_interface_t;

// 宏定义缺少前缀
#define PARITY_NONE 0                 // ❌ 应该是 HAL_SERIAL_PARITY_NONE
```

## 总结

通过本次命名规范统一，EMS 项目的所有模块（OSAL、HAL、PCL、PDL、ACL）都遵循了一致的命名规范：

1. **类型定义**：使用模块前缀 + 名称 + `_t`
2. **枚举常量**：使用模块前缀 + 名称（大写）
3. **函数名**：使用模块前缀（大写）+ 功能名
4. **宏定义**：使用模块前缀（大写）+ 名称

这确保了代码的可维护性、可读性和扩展性，为项目的长期发展奠定了良好的基础。

---

**优化完成时间**: 2026-05-29  
**编译状态**: ✅ 成功  
**命名规范**: ⭐⭐⭐⭐⭐
