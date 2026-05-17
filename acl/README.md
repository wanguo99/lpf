# ACL层（Application Configuration Layer）

## 概述

ACL（Application Configuration Layer，业务配置层）是连接业务逻辑和硬件实现的桥梁，通过配置映射将抽象的业务功能映射到具体的硬件设备。

## 设计目标

1. **业务与硬件解耦**：业务代码只关心功能（如"服务器上电"），不关心具体硬件（BMC还是MCU）
2. **配置驱动**：通过修改配置文件即可适配不同硬件平台，无需修改业务代码
3. **O(1)查询性能**：使用数组直接索引，查询配置的时间复杂度为O(1)
4. **类型安全**：使用枚举类型而非字符串，编译时检查错误
5. **易于维护**：配置集中管理，清晰可见业务功能与硬件的映射关系

## 架构位置

```
Apps层 (业务逻辑)
    ↓ 业务功能枚举 (TC_POWER_ON, TM_CPU_TEMP)
ACL层 (业务配置)
    ↓ 设备类型 + 逻辑索引 (BMC[0], MCU[1])
PDL层 (外设驱动)
    ↓ 硬件操作
HAL层 (硬件抽象)
```

## 目录结构

```
acl/
├── include/
│   ├── acl_api.h              # ACL API接口
│   ├── acl_config.h           # ACL配置结构定义
│   └── pmc_acl_types.h        # PMC业务功能枚举
├── src/
│   └── acl_api.c              # ACL API实现
├── config/
│   └── pmc_v1/
│       ├── acl_pmc_v1.c                # PMC v1.0配置
│       └── acl_pmc_v1_invalidation.c   # 遥控命令失效映射
└── CMakeLists.txt
```

## 核心概念

### 1. 业务功能枚举

所有业务功能（遥控/遥测/健康管理）都定义为枚举类型：

```c
typedef enum {
    TC_SERVER_POWER_ON = 0,
    TC_SERVER_POWER_OFF,
    TC_MCU_RESET,
    // ...
    TC_FUNC_MAX
} pmc_tc_function_t;

typedef enum {
    TM_SERVER_CPU_TEMP = 0,
    TM_SERVER_POWER_STATUS,
    TM_MCU_STATUS,
    // ...
    TM_FUNC_MAX
} pmc_tm_function_t;
```

### 2. 配置结构

#### 遥控配置
```c
typedef struct {
    pmc_tc_function_t function;    /* 遥控功能枚举 */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引 */
    bool enabled;                  /* 是否使能 */
} acl_tc_config_t;
```

#### 遥测配置
```c
typedef struct {
    pmc_tm_function_t function;    /* 遥测功能枚举 */
    acl_device_type_t device_type; /* 设备类型 */
    uint32_t logic_index;          /* 逻辑索引 */
    tm_data_type_t data_type;      /* 缓存型/实时型 */
    uint32_t validity_ms;          /* 有效期（毫秒） */
    uint32_t update_period_ms;     /* 更新周期（毫秒） */
    uint32_t realtime_timeout_us;  /* 实时查询超时（微秒） */
    bool enabled;                  /* 是否使能 */
} acl_tm_config_t;
```

### 3. O(1)查找表

```c
typedef struct {
    acl_tc_config_t tc_table[TC_FUNC_MAX];
    acl_tm_config_t tm_table[TM_FUNC_MAX];
} acl_lookup_table_t;
```

## API接口

### 初始化

```c
int32_t ACL_Init(void);
```

### 查询配置（O(1)）

```c
const acl_tc_config_t* ACL_GetTcConfig(pmc_tc_function_t function);
const acl_tm_config_t* ACL_GetTmConfig(pmc_tm_function_t function);
```

### 检查使能状态

```c
bool ACL_IsTcEnabled(pmc_tc_function_t function);
bool ACL_IsTmEnabled(pmc_tm_function_t function);
```

### 遥测缓存失效

```c
void ACL_InvalidateAffectedTelemetry(pmc_tc_function_t tc_function);
```

### 配置验证

```c
int32_t ACL_ValidateConfig(void);
```

### 统计与调试

```c
void ACL_GetStatistics(acl_statistics_t *stats);
void ACL_PrintStatistics(void);
void ACL_DumpConfig(void);
```

## 使用示例

### 1. 初始化

```c
int32_t ret = ACL_Init();
if (ret != OSAL_SUCCESS) {
    LOG_ERROR("APP", "ACL初始化失败");
    return ret;
}

/* 验证配置 */
ret = ACL_ValidateConfig();
if (ret != OSAL_SUCCESS) {
    LOG_ERROR("APP", "ACL配置验证失败");
    return ret;
}

/* 打印统计信息 */
ACL_PrintStatistics();
```

### 2. 处理遥控命令

```c
int32_t handle_telecommand(pmc_tc_function_t cmd_type, uint32_t param)
{
    /* 1. 通过ACL查询设备映射（O(1)） */
    const acl_tc_config_t *cfg = ACL_GetTcConfig(cmd_type);
    if (cfg == NULL || !cfg->enabled) {
        LOG_ERROR("TC", "命令未配置: %d", cmd_type);
        return OSAL_ERR_NOT_FOUND;
    }

    /* 2. 根据设备类型调用PDL接口 */
    int32_t ret;
    switch (cfg->device_type) {
        case ACL_DEVICE_BMC:
            if (cmd_type == TC_SERVER_POWER_ON) {
                ret = PDL_BMC_PowerOn(cfg->logic_index);
            }
            break;
        case ACL_DEVICE_MCU:
            if (cmd_type == TC_MCU_RESET) {
                ret = PDL_MCU_Reset(cfg->logic_index);
            }
            break;
        default:
            ret = OSAL_ERR_NOT_SUPPORTED;
    }

    /* 3. 遥控命令执行成功后，失效相关遥测缓存 */
    if (ret == OSAL_SUCCESS) {
        ACL_InvalidateAffectedTelemetry(cmd_type);
    }

    return ret;
}
```

### 3. 处理遥测查询

```c
int32_t handle_telemetry(pmc_tm_function_t tm_type, telemetry_data_t *data)
{
    /* 1. 通过ACL查询设备映射（O(1)） */
    const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_type);
    if (cfg == NULL || !cfg->enabled) {
        LOG_ERROR("TM", "遥测未配置: %d", tm_type);
        return OSAL_ERR_NOT_FOUND;
    }

    /* 2. 根据数据类型处理 */
    if (cfg->data_type == TM_TYPE_CACHED) {
        /* 缓存型：从共享内存读取（<10μs） */
        return get_cached_telemetry(tm_type, data);
    } else {
        /* 实时型：优先实时查询，超时降级到缓存 */
        return handle_realtime_telemetry(cfg, data);
    }
}
```

## 配置文件

### PMC v1.0配置示例

```c
/* acl/config/pmc_v1/acl_pmc_v1.c */

const acl_tc_config_t g_pmc_tc_configs[] = {
    /* 服务器电源控制 → BMC */
    { TC_SERVER_POWER_ON,  ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_OFF, ACL_DEVICE_BMC, 0, true },
    
    /* MCU控制 → MCU[0] */
    { TC_MCU_RESET,        ACL_DEVICE_MCU, 0, true },
};

const acl_tm_config_t g_pmc_tm_configs[] = {
    /* 服务器遥测（缓存型） → BMC，1秒更新，2秒有效期 */
    { TM_SERVER_CPU_TEMP,  ACL_DEVICE_BMC, 0, TM_TYPE_CACHED, 2000, 1000, 0, true },
    
    /* 服务器遥测（实时型） → BMC，100ms有效期，1ms超时 */
    { TM_SERVER_POWER_STATUS, ACL_DEVICE_BMC, 0, TM_TYPE_REALTIME, 100, 0, 1000, true },
};
```

## 性能特性

- **O(1)查询**：数组直接索引，查询时间~5ns（1-2条指令）
- **缓存友好**：查找表连续内存，L1 Cache命中率高
- **无锁设计**：初始化后只读，支持多线程并发访问
- **零拷贝**：返回const指针，无内存分配

## 编译配置

在CMakeLists.txt中选择配置：

```cmake
set(ACL_CONFIG "pmc_v1" CACHE STRING "ACL configuration")
```

支持的配置：
- `pmc_v1`: PMC v1.0配置
- `pmc_v2`: PMC v2.0配置（未来）
- `demo`: Demo配置（未来）

## 依赖关系

- **依赖**：OSAL层（日志、内存操作）
- **被依赖**：Apps层（应用程序）

## 注意事项

1. **枚举值作为数组索引**：枚举值必须从0开始连续递增
2. **配置完整性**：所有使能的功能必须配置设备类型和逻辑索引
3. **实时型遥测**：必须配置超时时间（realtime_timeout_us > 0）
4. **失效映射**：遥控命令执行后自动失效相关遥测缓存
5. **线程安全**：ACL_Init()必须在单线程环境下调用，之后的查询接口线程安全

## 未来扩展

1. **动态配置**：支持运行时修改配置（需要加锁）
2. **配置热加载**：支持从文件加载配置
3. **多产品支持**：支持更多产品配置（pmc_v2, demo等）
4. **健康管理配置**：添加健康管理功能配置
