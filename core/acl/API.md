# ACL API 参考文档

ACL (Application Control Layer) 提供应用配置和控制接口。

---

## 📋 目录

1. [核心 API](#核心-api)
2. [遥测管理](#遥测管理)
3. [遥控管理](#遥控管理)
4. [配置查询](#配置查询)
5. [数据类型](#数据类型)

---

## 核心 API

### ACL_Init

初始化 ACL 层。

```c
int32_t ACL_Init(void);
```

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_*`: 失败

**说明**: 必须在使用其他 ACL 函数之前调用。

**示例**:
```c
int32_t ret = ACL_Init();
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize ACL\n");
    return -1;
}
```

---

### ACL_RegisterTable

注册配置表。

```c
int32_t ACL_RegisterTable(const acl_config_table_t *table);
```

**参数**:
- `table`: 配置表指针
  - `name`: 配置表名称
  - `tc_table`: 遥控配置表
  - `tc_count`: 遥控配置数量
  - `tm_table`: 遥测配置表
  - `tm_count`: 遥测配置数量
  - `inv_map`: 失效映射表
  - `inv_count`: 失效映射数量

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_*`: 失败

**说明**: 通常在项目初始化时调用，注册项目特定的配置。

**示例**:
```c
// 定义遥控配置表
static const acl_tc_config_t tc_configs[] = {
    {
        .function_id = TC_POWER_ON,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    },
    {
        .function_id = TC_POWER_OFF,
        .device_type = ACL_DEVICE_BMC,
        .logic_index = 0,
        .enabled = true,
        .extra_data = NULL
    }
};

// 定义遥测配置表
static const acl_tm_config_t tm_configs[] = {
    {
        .function_id = TM_CPU_TEMP,
        .device_type = ACL_DEVICE_SENSOR,
        .logic_index = 0,
        .validity_ms = 5000,
        .update_period_ms = 1000,
        .enabled = true,
        .extra_data = NULL
    },
    {
        .function_id = TM_VOLTAGE_12V,
        .device_type = ACL_DEVICE_SENSOR,
        .logic_index = 1,
        .validity_ms = 10000,
        .update_period_ms = 2000,
        .enabled = true,
        .extra_data = NULL
    }
};

// 定义失效映射表
static uint32_t power_affected[] = {TM_CPU_TEMP, TM_VOLTAGE_12V};
static const acl_invalidation_map_t inv_maps[] = {
    {
        .source_tm_id = TM_POWER_STATUS,
        .affected_tm_ids = power_affected,
        .affected_count = 2
    }
};

// 注册配置表
static const acl_config_table_t config_table = {
    .name = "MyProject",
    .tc_table = tc_configs,
    .tc_count = sizeof(tc_configs) / sizeof(tc_configs[0]),
    .tm_table = tm_configs,
    .tm_count = sizeof(tm_configs) / sizeof(tm_configs[0]),
    .inv_map = inv_maps,
    .inv_count = sizeof(inv_maps) / sizeof(inv_maps[0])
};

int32_t ret = ACL_RegisterTable(&config_table);
if (ret != OSAL_SUCCESS) {
    printf("Failed to register config table\n");
    return -1;
}
```

---

## 遥测管理

### ACL_GetTmConfig

查询遥测配置。

```c
const acl_tm_config_t* ACL_GetTmConfig(uint32_t function_id);
```

**参数**:
- `function_id`: 功能 ID

**返回值**:
- 配置指针（只读）
- `NULL`: 失败

**说明**: O(1) 时间复杂度。

**示例**:
```c
const acl_tm_config_t *config = ACL_GetTmConfig(TM_CPU_TEMP);
if (config != NULL) {
    printf("TM Config:\n");
    printf("  Device Type: %d\n", config->device_type);
    printf("  Logic Index: %u\n", config->logic_index);
    printf("  Validity: %u ms\n", config->validity_ms);
    printf("  Update Period: %u ms\n", config->update_period_ms);
    printf("  Enabled: %s\n", config->enabled ? "Yes" : "No");
}
```

---

### ACL_IsTmEnabled

检查遥测功能是否使能。

```c
bool ACL_IsTmEnabled(uint32_t function_id);
```

**参数**:
- `function_id`: 功能 ID

**返回值**:
- `true`: 使能
- `false`: 禁用

**示例**:
```c
if (ACL_IsTmEnabled(TM_CPU_TEMP)) {
    // 读取 CPU 温度
    float temp = read_cpu_temperature();
    printf("CPU Temperature: %.2f C\n", temp);
}
```

---

### ACL_GetInvalidationMap

获取失效映射。

```c
int32_t ACL_GetInvalidationMap(
    uint32_t source_tm_id,
    uint32_t *affected_ids,
    uint32_t max_count,
    uint32_t *actual_count
);
```

**参数**:
- `source_tm_id`: 源遥测 ID
- `affected_ids`: 受影响的遥测 ID 数组（输出，调用者分配）
- `max_count`: 数组最大容量
- `actual_count`: 实际受影响的遥测数量（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_*`: 失败

**说明**: 当源遥测失效时，返回所有受影响的遥测 ID。

**示例**:
```c
uint32_t affected_ids[10];
uint32_t count;

int32_t ret = ACL_GetInvalidationMap(TM_POWER_STATUS, 
                                      affected_ids, 10, &count);
if (ret == OSAL_SUCCESS) {
    printf("When TM_POWER_STATUS fails, %u telemetries are affected:\n", count);
    for (uint32_t i = 0; i < count; i++) {
        printf("  - TM ID: %u\n", affected_ids[i]);
    }
}
```

---

## 遥控管理

### ACL_GetTcConfig

查询遥控配置。

```c
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id);
```

**参数**:
- `function_id`: 功能 ID

**返回值**:
- 配置指针（只读）
- `NULL`: 失败

**说明**: O(1) 时间复杂度。

**示例**:
```c
const acl_tc_config_t *config = ACL_GetTcConfig(TC_POWER_ON);
if (config != NULL) {
    printf("TC Config:\n");
    printf("  Device Type: %d\n", config->device_type);
    printf("  Logic Index: %u\n", config->logic_index);
    printf("  Enabled: %s\n", config->enabled ? "Yes" : "No");
}
```

---

### ACL_IsTcEnabled

检查遥控功能是否使能。

```c
bool ACL_IsTcEnabled(uint32_t function_id);
```

**参数**:
- `function_id`: 功能 ID

**返回值**:
- `true`: 使能
- `false`: 禁用

**示例**:
```c
if (ACL_IsTcEnabled(TC_POWER_ON)) {
    // 执行开机命令
    int32_t ret = execute_power_on();
    if (ret == OSAL_SUCCESS) {
        printf("Power on successful\n");
    }
} else {
    printf("Power on command is disabled\n");
}
```

---

## 配置查询

### ACL_GetStatistics

获取配置统计信息。

```c
int32_t ACL_GetStatistics(acl_statistics_t *stats);
```

**参数**:
- `stats`: 统计信息（输出）
  - `tc_enabled_count`: 使能的遥控数量
  - `tc_disabled_count`: 禁用的遥控数量
  - `tm_enabled_count`: 使能的遥测数量
  - `tm_disabled_count`: 禁用的遥测数量
  - `invalidation_map_count`: 失效映射数量

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_*`: 失败

**示例**:
```c
acl_statistics_t stats;
int32_t ret = ACL_GetStatistics(&stats);
if (ret == OSAL_SUCCESS) {
    printf("ACL Statistics:\n");
    printf("  TC Enabled: %u\n", stats.tc_enabled_count);
    printf("  TC Disabled: %u\n", stats.tc_disabled_count);
    printf("  TM Enabled: %u\n", stats.tm_enabled_count);
    printf("  TM Disabled: %u\n", stats.tm_disabled_count);
    printf("  Invalidation Maps: %u\n", stats.invalidation_map_count);
}
```

---

### ACL_PrintConfig

打印配置信息（调试用）。

```c
void ACL_PrintConfig(void);
```

**说明**: 打印所有已注册的配置信息，用于调试。

**示例**:
```c
// 打印配置信息
ACL_PrintConfig();
```

---

## 数据类型

### acl_device_type_t

设备类型枚举。

```c
typedef enum {
    ACL_DEVICE_SATELLITE = 0,  /* 卫星设备 */
    ACL_DEVICE_BMC,            /* 板级管理控制器 */
    ACL_DEVICE_MCU,            /* 微控制器 */
    ACL_DEVICE_FPGA,           /* 可编程逻辑器件 */
    ACL_DEVICE_SWITCH,         /* 网络交换机 */
    ACL_DEVICE_SENSOR,         /* 传感器 */
    ACL_DEVICE_ACTUATOR,       /* 执行器 */
    ACL_DEVICE_MAX
} acl_device_type_t;
```

---

### acl_tm_status_t

遥测数据新鲜度标记。

```c
typedef enum {
    ACL_TM_STATUS_INVALID = 0,   /* 无效：从未更新或已失效 */
    ACL_TM_STATUS_FRESH = 1,     /* 新鲜：在有效期内 */
    ACL_TM_STATUS_STALE = 2      /* 过期：超过有效期但仍可用 */
} acl_tm_status_t;
```

---

### acl_tc_config_t

遥控功能配置。

```c
typedef struct {
    uint32_t function_id;          /* 功能 ID */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引 */
    bool enabled;                  /* 是否使能 */
    void *extra_data;              /* 扩展数据 */
} acl_tc_config_t;
```

---

### acl_tm_config_t

遥测功能配置。

```c
typedef struct {
    uint32_t function_id;          /* 功能 ID */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引 */
    uint32_t validity_ms;          /* 有效期（毫秒） */
    uint32_t update_period_ms;     /* 后台更新周期（毫秒） */
    bool enabled;                  /* 是否使能 */
    void *extra_data;              /* 扩展数据 */
} acl_tm_config_t;
```

---

### acl_invalidation_map_t

失效映射配置。

```c
typedef struct {
    uint32_t source_tm_id;         /* 源遥测 ID */
    uint32_t *affected_tm_ids;     /* 受影响的遥测 ID 数组 */
    uint32_t affected_count;       /* 受影响的遥测数量 */
} acl_invalidation_map_t;
```

---

### acl_config_table_t

ACL 配置表。

```c
typedef struct {
    const char *name;                      /* 配置表名称 */
    const acl_tc_config_t *tc_table;       /* 遥控配置表 */
    uint32_t tc_count;                     /* 遥控配置数量 */
    const acl_tm_config_t *tm_table;       /* 遥测配置表 */
    uint32_t tm_count;                     /* 遥测配置数量 */
    const acl_invalidation_map_t *inv_map; /* 失效映射表 */
    uint32_t inv_count;                    /* 失效映射数量 */
} acl_config_table_t;
```

---

## 遥控功能 ID

### 电源控制类 (0-99)

```c
TC_POWER_ON = 0,      /* 电源开机 */
TC_POWER_OFF = 1,     /* 电源关机 */
TC_POWER_RESET = 2,   /* 电源复位 */
TC_POWER_CYCLE = 3,   /* 电源循环 */
```

### 复位控制类 (100-199)

```c
TC_SOFT_RESET = 100,        /* 软复位 */
TC_HARD_RESET = 101,        /* 硬复位 */
TC_MCU_RESET = 102,         /* MCU 复位 */
TC_MCU_POWER_CTRL = 103,    /* MCU 电源控制 */
TC_FPGA_RESET = 104,        /* FPGA 复位 */
TC_FPGA_CONFIG_LOAD = 105,  /* FPGA 配置加载 */
```

### 固件升级类 (200-299)

```c
TC_FIRMWARE_UPGRADE_START = 200,   /* 固件升级开始 */
TC_FIRMWARE_UPGRADE_DATA = 201,    /* 固件升级数据 */
TC_FIRMWARE_UPGRADE_VERIFY = 202,  /* 固件升级验证 */
TC_FIRMWARE_UPGRADE_COMMIT = 203,  /* 固件升级提交 */
```

### 系统控制类 (300-399)

```c
TC_SYSTEM_RESET = 300,       /* 系统复位 */
TC_WATCHDOG_ENABLE = 301,    /* 看门狗使能 */
TC_WATCHDOG_DISABLE = 302,   /* 看门狗禁用 */
```

---

## 遥测功能 ID

### 温度遥测类 (0-99)

```c
TM_CPU_TEMP = 0,      /* CPU 温度 */
TM_BOARD_TEMP = 1,    /* 板卡温度 */
TM_MCU_TEMP = 2,      /* MCU 温度 */
TM_FPGA_TEMP = 3,     /* FPGA 温度 */
TM_FAN_SPEED = 4,     /* 风扇转速 */
```

### 电压遥测类 (100-199)

```c
TM_VOLTAGE_12V = 100,  /* 12V 电压 */
TM_VOLTAGE_5V = 101,   /* 5V 电压 */
TM_VOLTAGE_3V3 = 102,  /* 3.3V 电压 */
TM_CURRENT = 103,      /* 电流 */
```

### 状态遥测类 (200-299)

```c
TM_POWER_STATUS = 200,        /* 电源状态 */
TM_MCU_STATUS = 201,          /* MCU 状态 */
TM_FPGA_STATUS = 202,         /* FPGA 状态 */
TM_FPGA_CONFIG_STATUS = 203,  /* FPGA 配置状态 */
TM_WATCHDOG_STATUS = 204,     /* 看门狗状态 */
```

### 系统遥测类 (300-399)

```c
TM_SYSTEM_UPTIME = 300,  /* 系统运行时间 */
TM_MCU_UPTIME = 301,     /* MCU 运行时间 */
TM_ERROR_COUNT = 302,    /* 错误计数 */
```

---

## 最佳实践

### 1. 配置表设计

使用静态常量定义配置表：

```c
// ✅ 正确 - 使用静态常量
static const acl_tc_config_t tc_configs[] = {
    { .function_id = TC_POWER_ON, /* ... */ },
    { .function_id = TC_POWER_OFF, /* ... */ }
};

// ❌ 错误 - 使用动态分配
acl_tc_config_t *tc_configs = malloc(sizeof(acl_tc_config_t) * 10);
```

### 2. 功能 ID 分组

按功能类别分组，预留扩展空间：

```c
// ✅ 正确 - 分组并预留空间
typedef enum {
    TC_POWER_ON = 0,      // 电源控制类 0-99
    TC_POWER_OFF = 1,
    
    TC_SOFT_RESET = 100,  // 复位控制类 100-199
    TC_HARD_RESET = 101,
    
    TC_FUNC_MAX = 1000    // 预留扩展空间
} acl_tc_function_t;

// ❌ 错误 - 连续编号，难以扩展
typedef enum {
    TC_POWER_ON = 0,
    TC_POWER_OFF = 1,
    TC_SOFT_RESET = 2,
    TC_HARD_RESET = 3
} acl_tc_function_t;
```

### 3. 失效映射

合理设计失效映射关系：

```c
// ✅ 正确 - 明确失效关系
static uint32_t power_affected[] = {
    TM_CPU_TEMP,      // 电源失效时 CPU 温度无效
    TM_VOLTAGE_12V,   // 电源失效时电压无效
    TM_FAN_SPEED      // 电源失效时风扇转速无效
};

static const acl_invalidation_map_t inv_maps[] = {
    {
        .source_tm_id = TM_POWER_STATUS,
        .affected_tm_ids = power_affected,
        .affected_count = 3
    }
};
```

### 4. 配置查询

使用配置查询优化性能：

```c
// ✅ 正确 - 先查询配置，避免无效操作
if (ACL_IsTcEnabled(TC_POWER_ON)) {
    execute_power_on();
}

// ❌ 错误 - 直接执行，可能被禁用
execute_power_on();
```

### 5. 遥测有效期

合理设置遥测有效期：

```c
// ✅ 正确 - 根据更新频率设置有效期
{
    .function_id = TM_CPU_TEMP,
    .validity_ms = 5000,        // 5 秒有效期
    .update_period_ms = 1000,   // 1 秒更新一次
    .enabled = true
}

// ❌ 错误 - 有效期小于更新周期
{
    .function_id = TM_CPU_TEMP,
    .validity_ms = 500,         // 0.5 秒有效期
    .update_period_ms = 1000,   // 1 秒更新一次（数据总是过期）
    .enabled = true
}
```

---

## 错误码

| 错误码 | 说明 |
|--------|------|
| `OSAL_SUCCESS` | 成功 |
| `OSAL_ERR_GENERIC` | 一般错误 |
| `OSAL_ERR_INVALID_PARAM` | 参数无效 |
| `OSAL_ERR_NOT_FOUND` | 未找到配置 |

---

## 平台支持

| 平台 | 支持状态 |
|------|----------|
| Linux | ✅ 完全支持 |
| macOS | ✅ 完全支持 |
| Windows | 🚧 开发中 |
| RTOS | 🚧 计划中 |

---

## 相关文档

- [PDL API 参考](../pdl/API.md)
- [HAL API 参考](../hal/API.md)
- [OSAL API 参考](../osal/API.md)
- [开发者指南](../../docs/DEVELOPER_GUIDE.md)
- [架构文档](../../docs/ARCHITECTURE.md)

---

**版本**: 1.0.0  
**最后更新**: 2026-05-29
