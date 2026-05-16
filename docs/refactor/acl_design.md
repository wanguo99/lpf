# ACL层详细设计

## 8. ACL 设计（业务配置层）

### 8.1 ACL设计理念

ACL（Application Configuration Layer，业务配置层）是连接业务逻辑和硬件实现的桥梁，它通过配置映射将抽象的业务功能映射到具体的硬件设备。

#### 8.1.1 设计目标

1. **业务与硬件解耦**：业务代码只关心功能（如"服务器上电"），不关心具体硬件（BMC还是MCU）
2. **配置驱动**：通过修改配置文件即可适配不同硬件平台，无需修改业务代码
3. **O(1)查询性能**：使用数组直接索引，查询配置的时间复杂度为O(1)
4. **类型安全**：使用枚举类型而非字符串，编译时检查错误
5. **易于维护**：配置集中管理，清晰可见业务功能与硬件的映射关系

#### 7.1.2 ACL在架构中的位置

```text
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌──────────────────┬──────────────────┬─────────────────┐  │
│  │ Telecommand Proc │ Telemetry Proc   │ Firmware Proc   │  │
│  │                  │                  │                 │  │
│  │ handle_tc(       │ handle_tm(       │ handle_fw(      │  │
│  │   TC_POWER_ON)   │   TM_CPU_TEMP)   │   FW_UPGRADE)   │  │
│  └────────┬─────────┴────────┬─────────┴────────┬────────┘  │
└───────────┼──────────────────┼──────────────────┼───────────┘
            │                  │                  │
            │ 业务功能枚举      │                  │
            ▼                  ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    ACL Configuration Layer                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  ACL Lookup Table (O(1) 直接索引)                    │   │
│  │  ┌────────────────────────────────────────────────┐  │   │
│  │  │ TC_POWER_ON  → BMC[0]                          │  │   │
│  │  │ TC_MCU_RESET → MCU[0]                          │  │   │
│  │  │ TM_CPU_TEMP  → BMC[0], CACHED                  │  │   │
│  │  │ TM_MCU_STATUS→ MCU[0], REALTIME, 800μs timeout │  │   │
│  │  └────────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────┘   │
└───────────┬──────────────────┬──────────────────┬───────────┘
            │ device_type +    │                  │
            │ logic_index      │                  │
            ▼                  ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    PDL Layer (外设驱动层)                    │
│  ┌──────────────┬──────────────┬──────────────────────────┐ │
│  │ BMC Service  │ MCU Service  │ Satellite Service        │ │
│  │ PDL_BMC_*()  │ PDL_MCU_*()  │ PDL_Satellite_*()        │ │
│  └──────────────┴──────────────┴──────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

#### 7.1.3 核心设计原则

**原则1：业务功能枚举化**
- 所有业务功能（遥控/遥测/健康管理）都定义为枚举类型
- 枚举值作为数组索引，实现O(1)查询
- 编译时类型检查，避免运行时错误

**原则2：配置与代码分离**
- 配置数据放在独立的.c文件中（如`acl_pmc_v1.c`）
- 业务代码通过ACL API查询配置，不直接访问配置数组
- 不同产品/版本使用不同配置文件，共享同一套业务代码

**原则3：逻辑索引而非物理索引**
- ACL使用逻辑索引（logic_index），如"第0个BMC"、"第1个MCU"
- 物理索引（如CAN ID、I2C地址）由PCL层管理
- 业务层不需要知道硬件的物理连接细节

**原则4：配置完整性**
- 每个业务功能必须配置设备类型、逻辑索引、使能状态
- 遥测功能额外配置数据类型（缓存/实时）和超时时间
- 配置缺失或错误在初始化时检测，而非运行时

**原则5：命名规范**
- ACL配置文件命名：`acl_{project}_{product}_{version}.c`
- 关注业务功能，不关注硬件平台
- 示例：`acl_pmc_v1.c`、`acl_pmc_v2.c`、`acl_pmc_v1_invalidation.c`

### 7.2  实现示例

#### 7.2.1 PMC业务功能枚举

``` C
/************************************************
* acl/include/pmc_acl_types.h
* PMC业务功能枚举定义
************************************************/

/**
* @brief 遥测数据类型
*/
typedef enum {
  TM_TYPE_CACHED = 0,    // 缓存型：后台采集，从共享内存读取
  TM_TYPE_REALTIME = 1   // 实时型：优先实时查询，超时降级到缓存
} tm_data_type_t;

/**
* @brief 遥测数据新鲜度标记
*/
typedef enum {
  TM_FRESH = 0,      // 新鲜数据：采集时间在有效期内
  TM_STALE = 1,      // 过期数据：受遥控命令影响，等待更新
  TM_INVALID = 2     // 无效数据：从未采集过
} tm_freshness_t;

/**
* @brief PMC遥控功能枚举
*/
typedef enum {
  /* 服务器电源控制 */
  TC_SERVER_POWER_ON = 0,
  TC_SERVER_POWER_OFF,
  TC_SERVER_POWER_RESET,
  TC_SERVER_POWER_CYCLE,

  /* 服务器复位控制 */
  TC_SERVER_SOFT_RESET,
  TC_SERVER_HARD_RESET,

  /* MCU控制 */
  TC_MCU_RESET,
  TC_MCU_POWER_CTRL,

  /* FPGA控制 */
  TC_FPGA_RESET,
  TC_FPGA_CONFIG_LOAD,

  /* 固件升级 */
  TC_FIRMWARE_UPGRADE_START,
  TC_FIRMWARE_UPGRADE_DATA,
  TC_FIRMWARE_UPGRADE_VERIFY,
  TC_FIRMWARE_UPGRADE_COMMIT,

  /* 系统控制 */
  TC_SYSTEM_RESET,
  TC_WATCHDOG_ENABLE,
  TC_WATCHDOG_DISABLE,

  TC_FUNC_MAX
} pmc_tc_function_t;

/**
* @brief PMC遥测功能枚举
*/
typedef enum {
  /* 服务器状态遥测（缓存型） */
  TM_SERVER_CPU_TEMP = 0,        // 缓存型
  TM_SERVER_BOARD_TEMP,          // 缓存型
  TM_SERVER_FAN_SPEED,           // 缓存型
  TM_SERVER_VOLTAGE_12V,         // 缓存型
  TM_SERVER_VOLTAGE_5V,          // 缓存型
  TM_SERVER_VOLTAGE_3V3,         // 缓存型
  TM_SERVER_CURRENT,             // 缓存型

  /* 服务器状态遥测（实时型） */
  TM_SERVER_POWER_STATUS,        // 实时型

  /* MCU状态遥测 */
  TM_MCU_STATUS,                 // 实时型
  TM_MCU_TEMP,                   // 缓存型
  TM_MCU_VOLTAGE,                // 缓存型
  TM_MCU_UPTIME,                 // 缓存型

  /* FPGA状态遥测 */
  TM_FPGA_STATUS,                // 实时型
  TM_FPGA_TEMP,                  // 缓存型
  TM_FPGA_CONFIG_STATUS,         // 实时型

  /* 系统健康遥测 */
  TM_SYSTEM_UPTIME,              // 缓存型
  TM_WATCHDOG_STATUS,            // 实时型
  TM_ERROR_COUNT,                // 缓存型

  TM_FUNC_MAX
} pmc_tm_function_t;

/**
* @brief PMC健康管理功能枚举
*/
typedef enum {
  /* 健康检查项 */
  HM_SERVER_ALIVE = 0,
  HM_BMC_REACHABLE,
  HM_MCU_RESPONSIVE,
  HM_CAN_BUS_STATUS,
  HM_ETHERNET_STATUS,

  /* 故障检测 */
  HM_POWER_FAULT,
  HM_TEMP_OVER_LIMIT,
  HM_VOLTAGE_OUT_RANGE,

  HM_FUNC_MAX
} pmc_hm_function_t;
```

#### 7.2.2 ACL配置结构

``` C
/************************************************
* acl/include/acl_config.h
* ACL配置结构定义
************************************************/

typedef enum {
  ACL_DEVICE_SATELLITE = 0,
  ACL_DEVICE_BMC,
  ACL_DEVICE_MCU,
  ACL_DEVICE_FPGA,
  ACL_DEVICE_MAX
} acl_device_type_t;

/**
* @brief 遥控功能配置
*/
typedef struct {
  pmc_tc_function_t function;
  acl_device_type_t device_type;
  uint32_t logic_index;
  bool enabled;
} acl_tc_config_t;

/**
* @brief 遥测功能配置
*/
typedef struct {
  pmc_tm_function_t function;
  acl_device_type_t device_type;
  uint32_t logic_index;
  tm_data_type_t data_type;      // 缓存型/实时型
  uint32_t realtime_timeout_us;  // 实时查询超时（微秒），仅实时型有效
  bool enabled;
} acl_tm_config_t;

/**
* @brief ACL查找表（O(1)直接索引）
*/
typedef struct {
  acl_tc_config_t tc_table[TC_FUNC_MAX];
  acl_tm_config_t tm_table[TM_FUNC_MAX];
} acl_lookup_table_t;
```

#### 7.2.3 ACL配置示例

``` C
/************************************************
/************************************************
* acl/config/pmc_v1/acl_pmc_v1.c
* PMC v1.0应用配置
* 命名规范：acl_{project}_{product}_{version}.c
* 说明：ACL配置关注业务功能，不关注硬件平台
************************************************/
* PMC v1.0配置（BMC通过Redfish，MCU通过CAN）
************************************************/

static const acl_tc_config_t g_pmc_tc_configs[] = {
  /* 服务器电源控制 → BMC */
  { TC_SERVER_POWER_ON,      ACL_DEVICE_BMC, 0, true },
  { TC_SERVER_POWER_OFF,     ACL_DEVICE_BMC, 0, true },
  { TC_SERVER_POWER_RESET,   ACL_DEVICE_BMC, 0, true },
  { TC_SERVER_SOFT_RESET,    ACL_DEVICE_BMC, 0, true },
  { TC_SERVER_HARD_RESET,    ACL_DEVICE_BMC, 0, true },

  /* MCU控制 → MCU[0] */
  { TC_MCU_RESET,            ACL_DEVICE_MCU, 0, true },
  { TC_MCU_POWER_CTRL,       ACL_DEVICE_MCU, 1, true },  // MCU[1]是电源管理

  /* FPGA控制 → FPGA[0] */
  { TC_FPGA_RESET,           ACL_DEVICE_FPGA, 0, true },
  { TC_FPGA_CONFIG_LOAD,     ACL_DEVICE_FPGA, 0, true },

  /* 看门狗控制 → MCU[2] */
  { TC_WATCHDOG_ENABLE,      ACL_DEVICE_MCU, 2, true },
  { TC_WATCHDOG_DISABLE,     ACL_DEVICE_MCU, 2, true },
};

static const acl_tm_config_t g_pmc_tm_configs[] = {
  /* 服务器遥测（缓存型） → BMC */
  { TM_SERVER_CPU_TEMP,      ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   0,    true },
  { TM_SERVER_BOARD_TEMP,    ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   0,    true },
  { TM_SERVER_FAN_SPEED,     ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   0,    true },
  { TM_SERVER_VOLTAGE_12V,   ACL_DEVICE_MCU, 1, TM_TYPE_CACHED,   0,    true },
  { TM_SERVER_VOLTAGE_5V,    ACL_DEVICE_MCU, 1, TM_TYPE_CACHED,   0,    true },
  { TM_SERVER_CURRENT,       ACL_DEVICE_MCU, 1, TM_TYPE_CACHED,   0,    true },

  /* 服务器遥测（实时型） → BMC，1ms超时 */
  { TM_SERVER_POWER_STATUS,  ACL_DEVICE_BMC, 0, TM_TYPE_REALTIME, 1000, true },

  /* MCU遥测（实时型） → MCU，800μs超时 */
  { TM_MCU_STATUS,           ACL_DEVICE_MCU, 0, TM_TYPE_REALTIME, 800,  true },
  { TM_MCU_TEMP,             ACL_DEVICE_MCU, 0, TM_TYPE_CACHED,   0,    true },
  { TM_MCU_VOLTAGE,          ACL_DEVICE_MCU, 0, TM_TYPE_CACHED,   0,    true },

  /* FPGA遥测（实时型） → FPGA，1ms超时 */
  { TM_FPGA_STATUS,          ACL_DEVICE_FPGA, 0, TM_TYPE_REALTIME, 1000, true },
  { TM_FPGA_TEMP,            ACL_DEVICE_FPGA, 0, TM_TYPE_CACHED,   0,    true },

  /* 系统遥测 */
  { TM_SYSTEM_UPTIME,        ACL_DEVICE_MCU, 0, TM_TYPE_CACHED,   0,    true },
  { TM_WATCHDOG_STATUS,      ACL_DEVICE_MCU, 2, TM_TYPE_REALTIME, 800,  true },
};

/**
* @brief 初始化ACL查找表（O(1)直接索引）
*/
int32_t ACL_Init(acl_lookup_table_t *table)
{
  // 初始化遥控查找表
  for (uint32_t i = 0; i < sizeof(g_pmc_tc_configs) / sizeof(acl_tc_config_t); i++) {
	  const acl_tc_config_t *cfg = &g_pmc_tc_configs[i];
	  table->tc_table[cfg->function] = *cfg;
  }

  // 初始化遥测查找表
  for (uint32_t i = 0; i < sizeof(g_pmc_tm_configs) / sizeof(acl_tm_config_t); i++) {
	  const acl_tm_config_t *cfg = &g_pmc_tm_configs[i];
	  table->tm_table[cfg->function] = *cfg;
  }

  return OSAL_SUCCESS;
}

/**
* @brief 查询遥控配置（O(1)）
*/
const acl_tc_config_t* ACL_GetTcConfig(const acl_lookup_table_t *table, pmc_tc_function_t function)
{
  if (function >= TC_FUNC_MAX) {
	  return NULL;
  }
  return &table->tc_table[function];
}

/**
* @brief 查询遥测配置（O(1)）
*/
const acl_tm_config_t* ACL_GetTmConfig(const acl_lookup_table_t *table, pmc_tm_function_t function)
{
  if (function >= TM_FUNC_MAX) {
	  return NULL;
  }
  return &table->tm_table[function];
}
```

#### 7.2.4 ACL使用示例

``` C
/************************************************
* telecommand进程中的命令处理
************************************************/

// 全局ACL查找表
static acl_lookup_table_t g_acl_table;

// 初始化
ACL_Init(&g_acl_table);

// 处理遥控命令
int32_t handle_telecommand(pmc_tc_function_t cmd_type, uint32_t param)
{
  // 1. 通过ACL查询设备映射（O(1)）
  const acl_tc_config_t *cfg = ACL_GetTcConfig(&g_acl_table, cmd_type);
  if (cfg == NULL || !cfg->enabled) {
	  LOG_ERROR("TC", "命令未配置: %d", cmd_type);
	  return OSAL_ERR_NOT_FOUND;
  }

  // 2. 根据设备类型调用PDL接口
  int32_t ret;
  switch (cfg->device_type) {
	  case ACL_DEVICE_BMC:
		  if (cmd_type == TC_SERVER_POWER_ON) {
			  ret = PDL_BMC_PowerOn(cfg->logic_index);
		  } else if (cmd_type == TC_SERVER_POWER_OFF) {
			  ret = PDL_BMC_PowerOff(cfg->logic_index);
		  } else if (cmd_type == TC_SERVER_POWER_RESET) {
			  ret = PDL_BMC_PowerReset(cfg->logic_index);
		  }
		  break;

	  case ACL_DEVICE_MCU:
		  if (cmd_type == TC_MCU_RESET) {
			  ret = PDL_MCU_Reset(cfg->logic_index);
		  } else if (cmd_type == TC_WATCHDOG_ENABLE) {
			  ret = PDL_MCU_SendCommand(cfg->logic_index, MCU_CMD_WDT_ENABLE, 0);
		  }
		  break;

	  case ACL_DEVICE_FPGA:
		  if (cmd_type == TC_FPGA_RESET) {
			  ret = PDL_FPGA_Reset(cfg->logic_index);
		  }
		  break;

	  default:
		  ret = OSAL_ERR_NOT_SUPPORTED;
  }

  return ret;
}

// 处理遥测命令
int32_t handle_telemetry(pmc_tm_function_t tm_type, telemetry_data_t *data)
{
  // 1. 通过ACL查询设备映射（O(1)）
  const acl_tm_config_t *cfg = ACL_GetTmConfig(&g_acl_table, tm_type);
  if (cfg == NULL || !cfg->enabled) {
	  LOG_ERROR("TM", "遥测未配置: %d", tm_type);
	  return OSAL_ERR_NOT_FOUND;
  }

  // 2. 根据数据类型处理
  if (cfg->data_type == TM_TYPE_CACHED) {
	  // 缓存型：从共享内存读取（<10μs）
	  get_cached_telemetry(tm_type, data);
  } else {
	  // 实时型：优先实时查询，超时降级到缓存
	  tm_freshness_t freshness;
	  int32_t ret = handle_realtime_telemetry(cfg, data, &freshness);
	  
	  if (ret != OSAL_SUCCESS) {
		  LOG_ERROR("TM", "实时遥测失败且缓存无效: %d", tm_type);
		  return ret;
	  }
	  
	  // 在应答中携带新鲜度标记
	  data->freshness = freshness;
  }

  return OSAL_SUCCESS;
}

/**
* @brief 处理实时型遥测（优先实时查询 + 缓存降级）
*/
int32_t handle_realtime_telemetry(const acl_tm_config_t *cfg, 
                                   telemetry_data_t *out_data,
                                   tm_freshness_t *out_freshness)
{
  telemetry_cache_entry_t *cache = &g_tm_cache[cfg->function];
  
  // 1. 尝试实时查询（带超时）
  int32_t ret = query_realtime_tm_with_timeout(cfg, out_data, cfg->realtime_timeout_us);
  
  if (ret == OSAL_SUCCESS) {
	  // 实时查询成功
	  *out_freshness = TM_FRESH;
	  
	  // 更新缓存
	  cache->data = *out_data;
	  cache->timestamp_ns = get_monotonic_ns();
	  cache->freshness = TM_FRESH;
	  
	  return OSAL_SUCCESS;
  }
  
  // 2. 实时查询失败，降级到缓存
  if (cache->valid) {
	  LOG_WARN("TM", "实时查询超时，使用缓存数据（%s）", 
			   cache->freshness == TM_FRESH ? "FRESH" : "STALE");
	  
	  *out_data = cache->data;
	  *out_freshness = cache->freshness;  // 可能是STALE
	  
	  return OSAL_SUCCESS;
  }
  
  // 3. 缓存也无效（从未采集过）
  LOG_ERROR("TM", "实时查询失败且缓存无效");
  *out_freshness = TM_INVALID;
  return OSAL_ERR_NOT_AVAILABLE;
}
```

#### 7.2.5 遥控命令对遥测缓存的影响映射

为了保证实时型遥测数据的准确性，需要在遥控命令执行后标记相关遥测缓存为STALE（过期），等待Telemetry进程重新采集。

```c
/************************************************
/************************************************
* acl/config/pmc_v1/acl_pmc_v1_invalidation.c
* PMC v1.0遥控命令失效映射
************************************************/
* 遥控命令对遥测数据的影响映射
************************************************/

/**
* @brief 遥控命令失效映射
*/
typedef struct {
  pmc_tc_function_t tc_function;
  pmc_tm_function_t affected_tm[8];  // 受影响的遥测项（最多8个）
  uint32_t affected_count;
} tc_tm_invalidation_map_t;

static const tc_tm_invalidation_map_t g_invalidation_map[] = {
  // 电源控制命令 → 影响电源状态遥测
  {
	  .tc_function = TC_SERVER_POWER_ON,
	  .affected_tm = { TM_SERVER_POWER_STATUS },
	  .affected_count = 1
  },
  {
	  .tc_function = TC_SERVER_POWER_OFF,
	  .affected_tm = { TM_SERVER_POWER_STATUS },
	  .affected_count = 1
  },
  {
	  .tc_function = TC_SERVER_POWER_RESET,
	  .affected_tm = { TM_SERVER_POWER_STATUS, TM_SERVER_CPU_TEMP },
	  .affected_count = 2
  },
  
  // MCU复位 → 影响MCU状态遥测
  {
	  .tc_function = TC_MCU_RESET,
	  .affected_tm = { TM_MCU_STATUS, TM_MCU_TEMP, TM_MCU_VOLTAGE },
	  .affected_count = 3
  },
  
  // FPGA复位 → 影响FPGA状态遥测
  {
	  .tc_function = TC_FPGA_RESET,
	  .affected_tm = { TM_FPGA_STATUS, TM_FPGA_CONFIG_STATUS },
	  .affected_count = 2
  },
  
  // 看门狗控制 → 影响看门狗状态遥测
  {
	  .tc_function = TC_WATCHDOG_ENABLE,
	  .affected_tm = { TM_WATCHDOG_STATUS },
	  .affected_count = 1
  },
  {
	  .tc_function = TC_WATCHDOG_DISABLE,
	  .affected_tm = { TM_WATCHDOG_STATUS },
	  .affected_count = 1
  },
};

/**
* @brief 遥控命令执行后，自动失效相关遥测缓存
*/
void ACL_InvalidateAffectedTelemetry(pmc_tc_function_t tc_function)
{
  for (uint32_t i = 0; i < ARRAY_SIZE(g_invalidation_map); i++) {
	  const tc_tm_invalidation_map_t *map = &g_invalidation_map[i];
	  
	  if (map->tc_function == tc_function) {
		  for (uint32_t j = 0; j < map->affected_count; j++) {
			  pmc_tm_function_t tm_func = map->affected_tm[j];
			  g_tm_cache[tm_func].freshness = TM_STALE;
			  
			  LOG_DEBUG("ACL", "遥测缓存标记为STALE: %d", tm_func);
		  }
		  break;
	  }
  }
}
```

#### 7.2.6 Telecommand进程中的遥控命令处理（完整版）

```c
/************************************************
* telecommand进程中的命令处理（包含缓存失效）
************************************************/

// 处理遥控命令
int32_t handle_telecommand(pmc_tc_function_t cmd_type, uint32_t param)
{
  // 1. 通过ACL查询设备映射（O(1)）
  const acl_tc_config_t *cfg = ACL_GetTcConfig(&g_acl_table, cmd_type);
  if (cfg == NULL || !cfg->enabled) {
	  LOG_ERROR("TC", "命令未配置: %d", cmd_type);
	  return OSAL_ERR_NOT_FOUND;
  }

  // 2. 根据设备类型调用PDL接口
  int32_t ret;
  switch (cfg->device_type) {
	  case ACL_DEVICE_BMC:
		  if (cmd_type == TC_SERVER_POWER_ON) {
			  ret = PDL_BMC_PowerOn(cfg->logic_index);
		  } else if (cmd_type == TC_SERVER_POWER_OFF) {
			  ret = PDL_BMC_PowerOff(cfg->logic_index);
		  } else if (cmd_type == TC_SERVER_POWER_RESET) {
			  ret = PDL_BMC_PowerReset(cfg->logic_index);
		  }
		  break;

	  case ACL_DEVICE_MCU:
		  if (cmd_type == TC_MCU_RESET) {
			  ret = PDL_MCU_Reset(cfg->logic_index);
		  } else if (cmd_type == TC_WATCHDOG_ENABLE) {
			  ret = PDL_MCU_SendCommand(cfg->logic_index, MCU_CMD_WDT_ENABLE, 0);
		  }
		  break;

	  case ACL_DEVICE_FPGA:
		  if (cmd_type == TC_FPGA_RESET) {
			  ret = PDL_FPGA_Reset(cfg->logic_index);
		  }
		  break;

	  default:
		  ret = OSAL_ERR_NOT_SUPPORTED;
  }
  
  // 3. 遥控命令执行成功后，失效相关遥测缓存
  if (ret == OSAL_SUCCESS) {
	  ACL_InvalidateAffectedTelemetry(cmd_type);
  }

  return ret;
}
```

### 7.3 ACL设计要点

#### 7.3.1 查找表初始化

ACL查找表在系统启动时初始化一次，之后只读访问，无需加锁。

```c
/************************************************
 * ACL查找表初始化流程
 ************************************************/

// 全局查找表（只读，无需加锁）
static acl_lookup_table_t g_acl_table;
static bool g_acl_initialized = false;

int32_t ACL_Init(void)
{
    if (g_acl_initialized) {
        LOG_WARN("ACL", "ACL已初始化，跳过");
        return OSAL_OK;
    }

    // 1. 清零查找表
    memset(&g_acl_table, 0, sizeof(acl_lookup_table_t));

    // 2. 初始化遥控查找表
    for (uint32_t i = 0; i < ARRAY_SIZE(g_pmc_tc_configs); i++) {
        const acl_tc_config_t *cfg = &g_pmc_tc_configs[i];
        
        // 检查枚举值范围
        if (cfg->function >= TC_FUNC_MAX) {
            LOG_ERROR("ACL", "遥控功能枚举越界: %d", cfg->function);
            return -OSAL_EINVAL;
        }
        
        // 检查重复配置
        if (g_acl_table.tc_table[cfg->function].enabled) {
            LOG_ERROR("ACL", "遥控功能重复配置: %d", cfg->function);
            return -OSAL_EINVAL;
        }
        
        g_acl_table.tc_table[cfg->function] = *cfg;
    }

    // 3. 初始化遥测查找表
    for (uint32_t i = 0; i < ARRAY_SIZE(g_pmc_tm_configs); i++) {
        const acl_tm_config_t *cfg = &g_pmc_tm_configs[i];
        
        // 检查枚举值范围
        if (cfg->function >= TM_FUNC_MAX) {
            LOG_ERROR("ACL", "遥测功能枚举越界: %d", cfg->function);
            return -OSAL_EINVAL;
        }
        
        // 检查重复配置
        if (g_acl_table.tm_table[cfg->function].enabled) {
            LOG_ERROR("ACL", "遥测功能重复配置: %d", cfg->function);
            return -OSAL_EINVAL;
        }
        
        // 检查实时型遥测的超时配置
        if (cfg->data_type == TM_TYPE_REALTIME && cfg->realtime_timeout_us == 0) {
            LOG_ERROR("ACL", "实时型遥测未配置超时: %d", cfg->function);
            return -OSAL_EINVAL;
        }
        
        g_acl_table.tm_table[cfg->function] = *cfg;
    }

    // 4. 初始化失效映射表
    int32_t ret = ACL_InitInvalidationMap();
    if (ret != OSAL_OK) {
        LOG_ERROR("ACL", "初始化失效映射表失败: %d", ret);
        return ret;
    }

    g_acl_initialized = true;
    LOG_INFO("ACL", "ACL初始化完成: TC=%zu, TM=%zu",
             ARRAY_SIZE(g_pmc_tc_configs),
             ARRAY_SIZE(g_pmc_tm_configs));

    return OSAL_OK;
}
```

#### 7.3.2 配置查询接口

```c
/************************************************
 * ACL配置查询接口（线程安全，无锁）
 ************************************************/

/**
 * @brief 查询遥控配置
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tc_config_t* ACL_GetTcConfig(pmc_tc_function_t function)
{
    if (!g_acl_initialized) {
        LOG_ERROR("ACL", "ACL未初始化");
        return NULL;
    }

    if (function >= TC_FUNC_MAX) {
        LOG_ERROR("ACL", "遥控功能枚举越界: %d", function);
        return NULL;
    }

    const acl_tc_config_t *cfg = &g_acl_table.tc_table[function];
    if (!cfg->enabled) {
        LOG_DEBUG("ACL", "遥控功能未使能: %d", function);
        return NULL;
    }

    return cfg;
}

/**
 * @brief 查询遥测配置
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tm_config_t* ACL_GetTmConfig(pmc_tm_function_t function)
{
    if (!g_acl_initialized) {
        LOG_ERROR("ACL", "ACL未初始化");
        return NULL;
    }

    if (function >= TM_FUNC_MAX) {
        LOG_ERROR("ACL", "遥测功能枚举越界: %d", function);
        return NULL;
    }

    const acl_tm_config_t *cfg = &g_acl_table.tm_table[function];
    if (!cfg->enabled) {
        LOG_DEBUG("ACL", "遥测功能未使能: %d", function);
        return NULL;
    }

    return cfg;
}

/**
 * @brief 检查遥控功能是否使能
 */
bool ACL_IsTcEnabled(pmc_tc_function_t function)
{
    if (!g_acl_initialized || function >= TC_FUNC_MAX) {
        return false;
    }
    return g_acl_table.tc_table[function].enabled;
}

/**
 * @brief 检查遥测功能是否使能
 */
bool ACL_IsTmEnabled(pmc_tm_function_t function)
{
    if (!g_acl_initialized || function >= TM_FUNC_MAX) {
        return false;
    }
    return g_acl_table.tm_table[function].enabled;
}
```

#### 7.3.3 配置验证

在系统启动时验证ACL配置的完整性和一致性。

```c
/************************************************
 * ACL配置验证
 ************************************************/

/**
 * @brief 验证ACL配置
 */
int32_t ACL_ValidateConfig(void)
{
    uint32_t error_count = 0;

    // 1. 验证遥控配置
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        const acl_tc_config_t *cfg = &g_acl_table.tc_table[i];
        
        if (!cfg->enabled) {
            continue;  // 未使能的功能跳过
        }

        // 检查设备类型
        if (cfg->device_type >= ACL_DEVICE_MAX) {
            LOG_ERROR("ACL", "TC[%d]: 无效的设备类型 %d", i, cfg->device_type);
            error_count++;
        }

        // 检查逻辑索引（假设最多支持4个同类设备）
        if (cfg->logic_index >= 4) {
            LOG_WARN("ACL", "TC[%d]: 逻辑索引过大 %d", i, cfg->logic_index);
        }
    }

    // 2. 验证遥测配置
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];
        
        if (!cfg->enabled) {
            continue;
        }

        // 检查设备类型
        if (cfg->device_type >= ACL_DEVICE_MAX) {
            LOG_ERROR("ACL", "TM[%d]: 无效的设备类型 %d", i, cfg->device_type);
            error_count++;
        }

        // 检查数据类型
        if (cfg->data_type != TM_TYPE_CACHED && cfg->data_type != TM_TYPE_REALTIME) {
            LOG_ERROR("ACL", "TM[%d]: 无效的数据类型 %d", i, cfg->data_type);
            error_count++;
        }

        // 检查实时型遥测的超时配置
        if (cfg->data_type == TM_TYPE_REALTIME) {
            if (cfg->realtime_timeout_us == 0) {
                LOG_ERROR("ACL", "TM[%d]: 实时型遥测未配置超时", i);
                error_count++;
            } else if (cfg->realtime_timeout_us > 10000) {  // 超过10ms
                LOG_WARN("ACL", "TM[%d]: 实时型遥测超时过长 %uμs", 
                         i, cfg->realtime_timeout_us);
            }
        }
    }

    // 3. 验证失效映射表
    for (uint32_t i = 0; i < ARRAY_SIZE(g_invalidation_map); i++) {
        const tc_tm_invalidation_map_t *map = &g_invalidation_map[i];
        
        // 检查遥控功能是否使能
        if (!ACL_IsTcEnabled(map->tc_function)) {
            LOG_WARN("ACL", "失效映射[%d]: 遥控功能未使能 %d", 
                     i, map->tc_function);
        }

        // 检查受影响的遥测功能
        for (uint32_t j = 0; j < map->affected_count; j++) {
            pmc_tm_function_t tm_func = map->affected_tm[j];
            
            if (tm_func >= TM_FUNC_MAX) {
                LOG_ERROR("ACL", "失效映射[%d]: 遥测功能枚举越界 %d", i, tm_func);
                error_count++;
            }
            
            // 只有实时型遥测才需要失效处理
            const acl_tm_config_t *tm_cfg = ACL_GetTmConfig(tm_func);
            if (tm_cfg && tm_cfg->data_type != TM_TYPE_REALTIME) {
                LOG_WARN("ACL", "失效映射[%d]: 遥测[%d]不是实时型", i, tm_func);
            }
        }
    }

    if (error_count > 0) {
        LOG_ERROR("ACL", "配置验证失败: %u个错误", error_count);
        return -OSAL_EINVAL;
    }

    LOG_INFO("ACL", "配置验证通过");
    return OSAL_OK;
}
```

#### 7.3.4 配置统计与调试

提供配置统计信息，便于调试和运维。

```c
/************************************************
 * ACL配置统计
 ************************************************/

typedef struct {
    uint32_t tc_enabled_count;
    uint32_t tc_disabled_count;
    uint32_t tm_enabled_count;
    uint32_t tm_disabled_count;
    uint32_t tm_cached_count;
    uint32_t tm_realtime_count;
    uint32_t invalidation_map_count;
} acl_statistics_t;

/**
 * @brief 获取ACL配置统计信息
 */
void ACL_GetStatistics(acl_statistics_t *stats)
{
    memset(stats, 0, sizeof(acl_statistics_t));

    // 统计遥控配置
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        if (g_acl_table.tc_table[i].enabled) {
            stats->tc_enabled_count++;
        } else {
            stats->tc_disabled_count++;
        }
    }

    // 统计遥测配置
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];
        
        if (cfg->enabled) {
            stats->tm_enabled_count++;
            
            if (cfg->data_type == TM_TYPE_CACHED) {
                stats->tm_cached_count++;
            } else {
                stats->tm_realtime_count++;
            }
        } else {
            stats->tm_disabled_count++;
        }
    }

    stats->invalidation_map_count = ARRAY_SIZE(g_invalidation_map);
}

/**
 * @brief 打印ACL配置统计信息
 */
void ACL_PrintStatistics(void)
{
    acl_statistics_t stats;
    ACL_GetStatistics(&stats);

    LOG_INFO("ACL", "========== ACL配置统计 ==========");
    LOG_INFO("ACL", "遥控功能: 使能=%u, 禁用=%u, 总计=%u",
             stats.tc_enabled_count, stats.tc_disabled_count, TC_FUNC_MAX);
    LOG_INFO("ACL", "遥测功能: 使能=%u, 禁用=%u, 总计=%u",
             stats.tm_enabled_count, stats.tm_disabled_count, TM_FUNC_MAX);
    LOG_INFO("ACL", "  - 缓存型: %u", stats.tm_cached_count);
    LOG_INFO("ACL", "  - 实时型: %u", stats.tm_realtime_count);
    LOG_INFO("ACL", "失效映射: %u", stats.invalidation_map_count);
    LOG_INFO("ACL", "================================");
}

/**
 * @brief 打印ACL配置详情（调试用）
 */
void ACL_DumpConfig(void)
{
    LOG_INFO("ACL", "========== 遥控配置 ==========");
    for (uint32_t i = 0; i < TC_FUNC_MAX; i++) {
        const acl_tc_config_t *cfg = &g_acl_table.tc_table[i];
        if (cfg->enabled) {
            LOG_INFO("ACL", "TC[%2u]: device=%u, index=%u",
                     i, cfg->device_type, cfg->logic_index);
        }
    }

    LOG_INFO("ACL", "========== 遥测配置 ==========");
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];
        if (cfg->enabled) {
            LOG_INFO("ACL", "TM[%2u]: device=%u, index=%u, type=%s, timeout=%uμs",
                     i, cfg->device_type, cfg->logic_index,
                     cfg->data_type == TM_TYPE_CACHED ? "CACHED" : "REALTIME",
                     cfg->realtime_timeout_us);
        }
    }
}
```

### 7.4 ACL性能优化

#### 7.4.1 O(1)查询性能

ACL使用数组直接索引，查询性能为O(1)，无需遍历或哈希查找。

```c
// 性能对比

// ❌ 方案1：链表遍历 - O(n)
for (node = list_head; node != NULL; node = node->next) {
    if (node->function == target_function) {
        return node->config;
    }
}

// ❌ 方案2：哈希表 - O(1)平均，但有哈希计算开销
uint32_t hash = hash_function(target_function);
return hash_table[hash];

// ✅ 方案3：数组直接索引 - O(1)，无额外开销
return &g_acl_table.tc_table[target_function];
```

**性能测试结果**（在ARM Cortex-A53 @ 1.5GHz上测试）：
- 数组直接索引：~5ns（1-2条指令）
- 哈希表查找：~50ns（包含哈希计算）
- 链表遍历：~200ns（平均遍历一半）

#### 7.4.2 缓存友好性

ACL查找表是连续内存，缓存命中率高。

```c
// 查找表内存布局（连续）
struct acl_lookup_table_t {
    acl_tc_config_t tc_table[TC_FUNC_MAX];  // 连续数组
    acl_tm_config_t tm_table[TM_FUNC_MAX];  // 连续数组
};

// 假设TC_FUNC_MAX=20, TM_FUNC_MAX=30
// sizeof(acl_tc_config_t) = 16字节
// sizeof(acl_tm_config_t) = 24字节
// 总大小 = 20*16 + 30*24 = 320 + 720 = 1040字节
// 可以完全放入L1 Cache（通常32KB）
```

#### 7.4.3 无锁设计

ACL查找表在初始化后只读，无需加锁，支持多线程并发访问。

```c
// 初始化阶段（单线程，启动时）
ACL_Init();  // 写入查找表

// 运行阶段（多线程，只读访问）
// Telecommand进程
const acl_tc_config_t *cfg = ACL_GetTcConfig(TC_POWER_ON);  // 无锁读

// Telemetry进程
const acl_tm_config_t *cfg = ACL_GetTmConfig(TM_CPU_TEMP);  // 无锁读

// Firmware进程
const acl_tc_config_t *cfg = ACL_GetTcConfig(TC_FW_UPGRADE);  // 无锁读
```

### 7.5 ACL扩展性

#### 7.5.1 新增业务功能

新增业务功能只需3步：

**步骤1：扩展枚举定义**
```c
// acl/include/pmc_acl_types.h
typedef enum {
    // ... 现有功能 ...
    TC_NEW_FUNCTION,  // 新增功能
    TC_FUNC_MAX
} pmc_tc_function_t;
```

**步骤2：添加配置**
```c
// acl/config/pmc_v1/acl_pmc_v1.c
static const acl_tc_config_t g_pmc_tc_configs[] = {
    // ... 现有配置 ...
    { TC_NEW_FUNCTION, ACL_DEVICE_MCU, 0, true },  // 新增配置
};
```

**步骤3：实现业务逻辑**
```c
// app/telecommand/tc_handler.c
if (cmd_type == TC_NEW_FUNCTION) {
    ret = PDL_MCU_NewFunction(cfg->logic_index);
}
```

#### 7.5.2 支持新硬件平台

支持新硬件平台只需修改配置文件，无需修改业务代码。

```c
// 原配置：acl_pmc_v1.c（BMC通过Redfish）
{ TC_SERVER_POWER_ON, ACL_DEVICE_BMC, 0, true },

// 新配置：acl_pmc_v2.c（改用MCU控制）
{ TC_SERVER_POWER_ON, ACL_DEVICE_MCU, 0, true },

// 业务代码无需修改，自动适配新硬件
```

#### 7.5.3 多产品支持

不同产品使用不同的ACL配置文件。

```text
acl/config/
├── pmc_v1/
│   ├── acl_pmc_v1.c              # PMC v1.0配置
│   └── acl_pmc_v1_invalidation.c
├── pmc_v2/
│   ├── acl_pmc_v2.c              # PMC v2.0配置（功能更多）
│   └── acl_pmc_v2_invalidation.c
└── demo/
    ├── acl_demo_v1.c             # Demo产品配置（功能简化）
    └── acl_demo_v1_invalidation.c
```

编译时通过CMake选择配置：
```cmake
# CMakeLists.txt
set(ACL_CONFIG "pmc_v1" CACHE STRING "ACL configuration: pmc_v1, pmc_v2, demo")

if(ACL_CONFIG STREQUAL "pmc_v1")
    target_sources(acl PRIVATE
        acl/config/pmc_v1/acl_pmc_v1.c
        acl/config/pmc_v1/acl_pmc_v1_invalidation.c
    )
elseif(ACL_CONFIG STREQUAL "pmc_v2")
    target_sources(acl PRIVATE
        acl/config/pmc_v2/acl_pmc_v2.c
        acl/config/pmc_v2/acl_pmc_v2_invalidation.c
    )
endif()
```

### 7.6 遥测数据新鲜度管理

为了保证遥测数据的准确性和可靠性，ACL层需要对遥测缓存进行时间戳和有效期管理，确保应用层能够识别数据的新鲜度状态。

#### 7.6.1 遥测缓存数据结构（增强版）

```c
/************************************************
 * acl/include/acl_telemetry_cache.h
 * 遥测缓存数据结构（带新鲜度管理）
 ************************************************/

/**
 * @brief 遥测数据新鲜度状态
 */
typedef enum {
    TM_STATUS_INVALID = 0,   // 无效：从未更新或已失效
    TM_STATUS_FRESH = 1,     // 新鲜：在有效期内
    TM_STATUS_STALE = 2,     // 过期：超过有效期但仍可用
} telemetry_status_t;

/**
 * @brief 遥测缓存元数据
 */
typedef struct {
    telemetry_status_t status;      // 数据状态
    uint64_t timestamp_us;          // 微秒级时间戳（最后更新时间）
    uint32_t validity_ms;           // 有效期（毫秒）
    uint32_t update_count;          // 更新次数（检测卡死）
    uint32_t read_count;            // 读取次数（统计）
    uint32_t error_count;           // 错误次数（质量监控）
    uint32_t checksum;              // 数据校验和（CRC32）
} telemetry_metadata_t;

/**
 * @brief 遥测缓存条目（完整版）
 */
typedef struct {
    uint32_t tm_id;                 // 遥测ID
    uint8_t data[256];              // 遥测数据
    uint32_t data_len;              // 数据长度
    telemetry_metadata_t meta;      // 元数据
    osal_id_t lock;                 // 访问锁（OSAL互斥锁）
} telemetry_cache_entry_t;

// 全局遥测缓存表
static telemetry_cache_entry_t g_tm_cache[TM_FUNC_MAX];
```

#### 7.6.2 遥测配置中的有效期定义

在ACL层的遥测配置中增加有效期字段：

```c
/**
 * @brief 遥测功能配置（增强版）
 */
typedef struct {
    pmc_tm_function_t function;
    acl_device_type_t device_type;
    uint32_t logic_index;
    tm_data_type_t data_type;      // 缓存型/实时型
    uint32_t validity_ms;           // 有效期（毫秒）- 新增
    uint32_t update_period_ms;      // 更新周期（毫秒）- 新增
    uint32_t realtime_timeout_us;   // 实时查询超时（微秒），仅实时型有效
    bool enabled;
} acl_tm_config_t;

// 遥测配置表示例
static const acl_tm_config_t g_pmc_tm_configs[] = {
    // 缓存型遥测：有效期 = 更新周期 * 2（允许一次更新失败）
    { TM_SERVER_CPU_TEMP,    ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   2000, 1000, 0,    true },
    { TM_SERVER_BOARD_TEMP,  ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   2000, 1000, 0,    true },
    { TM_SERVER_FAN_SPEED,   ACL_DEVICE_BMC, 0, TM_TYPE_CACHED,   4000, 2000, 0,    true },
    { TM_SERVER_VOLTAGE_12V, ACL_DEVICE_MCU, 1, TM_TYPE_CACHED,   4000, 2000, 0,    true },
    
    // 实时型遥测：有效期很短（100ms），强制实时查询
    { TM_SERVER_POWER_STATUS,ACL_DEVICE_BMC, 0, TM_TYPE_REALTIME, 100,  0,    1000, true },
    { TM_MCU_STATUS,         ACL_DEVICE_MCU, 0, TM_TYPE_REALTIME, 100,  0,    800,  true },
    { TM_FPGA_STATUS,        ACL_DEVICE_FPGA,0, TM_TYPE_REALTIME, 100,  0,    1000, true },
};
```

#### 7.6.3 遥测数据写入接口（带时间戳和校验）

```c
/************************************************
 * acl/src/acl_telemetry_cache.c
 * 遥测缓存管理实现
 ************************************************/

/**
 * @brief 写入遥测数据到缓存（Telemetry进程调用）
 * @param tm_id 遥测ID
 * @param data 遥测数据
 * @param data_len 数据长度
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_WriteTelemetryCache(uint32_t tm_id, const uint8_t *data, uint32_t data_len)
{
    if (tm_id >= TM_FUNC_MAX) {
        LOG_ERROR("ACL", "Invalid tm_id: %u", tm_id);
        return OS_ERROR;
    }
    
    telemetry_cache_entry_t *entry = &g_tm_cache[tm_id];
    const acl_tm_config_t *config = ACL_GetTmConfig(tm_id);
    
    if (config == NULL) {
        LOG_ERROR("ACL", "TM config not found: %u", tm_id);
        return OS_ERROR;
    }
    
    // 加锁
    int32 ret = OSAL_MutexLock(entry->lock);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("ACL", "Failed to lock tm_id %u", tm_id);
        return OS_ERROR;
    }
    
    // 1. 复制数据
    if (data_len > sizeof(entry->data)) {
        LOG_ERROR("ACL", "Data too large: %u > %zu", data_len, sizeof(entry->data));
        OSAL_MutexUnlock(entry->lock);
        return OS_ERROR;
    }
    OSAL_Memcpy(entry->data, data, data_len);
    entry->data_len = data_len;
    
    // 2. 更新元数据
    entry->meta.timestamp_us = OSAL_GetTimeUs();
    entry->meta.validity_ms = config->validity_ms;
    entry->meta.update_count++;
    entry->meta.status = TM_STATUS_FRESH;
    
    // 3. 计算校验和
    entry->meta.checksum = OSAL_CRC32(data, data_len);
    
    // 解锁
    OSAL_MutexUnlock(entry->lock);
    
    LOG_DEBUG("ACL", "TM %u updated, count=%u", tm_id, entry->meta.update_count);
    return OS_SUCCESS;
}
```

#### 7.6.4 遥测数据读取接口（带新鲜度检查）

```c
/**
 * @brief 读取遥测数据（R5F Realtime_TM_Task调用）
 * @param tm_id 遥测ID
 * @param data 输出数据缓冲区
 * @param data_len 输出数据长度
 * @param status 输出数据状态（FRESH/STALE/INVALID）
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_ReadTelemetryCache(uint32_t tm_id, uint8_t *data, uint32_t *data_len, 
                              telemetry_status_t *status)
{
    if (tm_id >= TM_FUNC_MAX || data == NULL || data_len == NULL || status == NULL) {
        LOG_ERROR("ACL", "Invalid parameters");
        return OS_ERROR;
    }
    
    telemetry_cache_entry_t *entry = &g_tm_cache[tm_id];
    uint64_t now = OSAL_GetTimeUs();
    uint64_t age_us;
    
    // 加锁
    int32 ret = OSAL_MutexLock(entry->lock);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("ACL", "Failed to lock tm_id %u", tm_id);
        return OS_ERROR;
    }
    
    // 1. 检查数据新鲜度
    age_us = now - entry->meta.timestamp_us;
    
    if (entry->meta.status == TM_STATUS_INVALID) {
        // 从未更新过
        *status = TM_STATUS_INVALID;
        OSAL_MutexUnlock(entry->lock);
        LOG_WARNING("ACL", "TM %u never updated", tm_id);
        return OS_ERROR;
    }
    
    if (age_us > entry->meta.validity_ms * 1000) {
        // 超过有效期 → STALE
        entry->meta.status = TM_STATUS_STALE;
        LOG_WARNING("ACL", "TM %u stale: age=%llu ms, validity=%u ms",
                   tm_id, age_us / 1000, entry->meta.validity_ms);
    }
    
    if (age_us > entry->meta.validity_ms * 10 * 1000) {
        // 超过有效期10倍 → INVALID
        entry->meta.status = TM_STATUS_INVALID;
        *status = TM_STATUS_INVALID;
        OSAL_MutexUnlock(entry->lock);
        LOG_ERROR("ACL", "TM %u invalid: age=%llu ms", tm_id, age_us / 1000);
        return OS_ERROR;
    }
    
    // 2. 验证校验和
    uint32_t computed_crc = OSAL_CRC32(entry->data, entry->data_len);
    if (computed_crc != entry->meta.checksum) {
        LOG_ERROR("ACL", "TM %u checksum mismatch: 0x%08x != 0x%08x",
                 tm_id, computed_crc, entry->meta.checksum);
        entry->meta.error_count++;
        entry->meta.status = TM_STATUS_INVALID;
        *status = TM_STATUS_INVALID;
        OSAL_MutexUnlock(entry->lock);
        return OS_ERROR;
    }
    
    // 3. 复制数据
    OSAL_Memcpy(data, entry->data, entry->data_len);
    *data_len = entry->data_len;
    *status = entry->meta.status;
    
    // 4. 更新统计
    entry->meta.read_count++;
    
    // 解锁
    OSAL_MutexUnlock(entry->lock);
    
    return OS_SUCCESS;
}
```

#### 7.6.5 遥测缓存失效接口（遥控命令执行后调用）

```c
/**
 * @brief 遥控命令执行后，标记相关遥测为STALE
 * @param tm_id 遥测ID
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_InvalidateTelemetryCache(uint32_t tm_id)
{
    if (tm_id >= TM_FUNC_MAX) {
        LOG_ERROR("ACL", "Invalid tm_id: %u", tm_id);
        return OS_ERROR;
    }
    
    telemetry_cache_entry_t *entry = &g_tm_cache[tm_id];
    
    int32 ret = OSAL_MutexLock(entry->lock);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("ACL", "Failed to lock tm_id %u", tm_id);
        return OS_ERROR;
    }
    
    // 标记为STALE，等待Telemetry进程更新
    entry->meta.status = TM_STATUS_STALE;
    
    OSAL_MutexUnlock(entry->lock);
    
    LOG_INFO("ACL", "TM %u invalidated by telecommand", tm_id);
    return OS_SUCCESS;
}

/**
 * @brief 批量失效（一个遥控命令可能影响多个遥测）
 * @param tm_ids 遥测ID数组
 * @param count 数组长度
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_InvalidateTelemetryCacheBatch(const uint32_t *tm_ids, uint32_t count)
{
    if (tm_ids == NULL || count == 0) {
        LOG_ERROR("ACL", "Invalid parameters");
        return OS_ERROR;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        ACL_InvalidateTelemetryCache(tm_ids[i]);
    }
    
    return OS_SUCCESS;
}
```

#### 7.6.6 遥测质量监控接口

```c
/**
 * @brief 遥测质量统计
 */
typedef struct {
    uint32_t tm_id;
    uint64_t last_update_time;      // 最后更新时间（微秒）
    uint32_t update_count;          // 更新次数
    uint32_t read_count;            // 读取次数
    uint32_t error_count;           // 错误次数
    uint32_t stale_count;           // 过期次数
    telemetry_status_t status;      // 当前状态
} telemetry_quality_stats_t;

/**
 * @brief 获取遥测质量统计
 * @param tm_id 遥测ID
 * @param stats 输出统计信息
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_GetTelemetryQualityStats(uint32_t tm_id, telemetry_quality_stats_t *stats)
{
    if (tm_id >= TM_FUNC_MAX || stats == NULL) {
        LOG_ERROR("ACL", "Invalid parameters");
        return OS_ERROR;
    }
    
    telemetry_cache_entry_t *entry = &g_tm_cache[tm_id];
    
    int32 ret = OSAL_MutexLock(entry->lock);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("ACL", "Failed to lock tm_id %u", tm_id);
        return OS_ERROR;
    }
    
    stats->tm_id = tm_id;
    stats->last_update_time = entry->meta.timestamp_us;
    stats->update_count = entry->meta.update_count;
    stats->read_count = entry->meta.read_count;
    stats->error_count = entry->meta.error_count;
    stats->status = entry->meta.status;
    
    // 计算过期次数（简化，实际需要单独计数）
    stats->stale_count = entry->meta.error_count;
    
    OSAL_MutexUnlock(entry->lock);
    
    return OS_SUCCESS;
}

/**
 * @brief 定期上报遥测质量（通过Logger进程）
 */
void ACL_LogTelemetryQualityReport(void)
{
    telemetry_quality_stats_t stats;
    
    LOG_INFO("TM_QUALITY", "=== Telemetry Quality Report ===");
    
    for (uint32_t tm_id = 0; tm_id < TM_FUNC_MAX; tm_id++) {
        if (ACL_GetTelemetryQualityStats(tm_id, &stats) == OS_SUCCESS) {
            if (stats.update_count > 0) {
                LOG_INFO("TM_QUALITY", "TM %u: updates=%u, reads=%u, errors=%u, status=%d",
                        tm_id, stats.update_count, stats.read_count, 
                        stats.error_count, stats.status);
                
                // 告警：错误率过高
                if (stats.error_count * 100 / stats.update_count > 5) {
                    LOG_WARNING("TM_QUALITY", "TM %u error rate high: %u%%",
                               tm_id, stats.error_count * 100 / stats.update_count);
                }
                
                // 告警：长时间未更新
                uint64_t age_ms = (OSAL_GetTimeUs() - stats.last_update_time) / 1000;
                if (age_ms > 60000) {
                    LOG_WARNING("TM_QUALITY", "TM %u not updated for %llu ms", tm_id, age_ms);
                }
            }
        }
    }
}
```

#### 7.6.7 R5F侧使用示例

```c
/************************************************
 * R5F Realtime_TM_Task中使用
 ************************************************/

void realtime_tm_task(void *arg)
{
    uint8_t data[256];
    uint32_t data_len;
    telemetry_status_t status;
    
    while (!OSAL_TaskShouldShutdown()) {
        // 等待遥测请求
        tm_request_t req;
        if (receive_tm_request(&req) == OS_SUCCESS) {
            
            // 1. 先尝试从缓存读取
            int32 ret = ACL_ReadTelemetryCache(req.tm_id, data, &data_len, &status);
            
            if (ret == OS_SUCCESS && status == TM_STATUS_FRESH) {
                // 缓存数据新鲜，直接使用
                send_tm_response(&req, data, data_len, status);
                LOG_DEBUG("R5F_TM", "TM %u from cache (FRESH)", req.tm_id);
            } else {
                // 缓存数据过期或无效，实时查询
                LOG_WARNING("R5F_TM", "TM %u cache %s, querying device",
                           req.tm_id, status == TM_STATUS_STALE ? "STALE" : "INVALID");
                
                // 2. 从物理设备实时查询（带超时）
                ret = query_device_realtime(req.tm_id, data, &data_len, 1000);
                
                if (ret == OS_SUCCESS) {
                    // 查询成功，发送响应
                    send_tm_response(&req, data, data_len, TM_STATUS_FRESH);
                    
                    // 更新缓存
                    ACL_WriteTelemetryCache(req.tm_id, data, data_len);
                } else {
                    // 查询失败，如果缓存是STALE，仍然使用（降级）
                    if (status == TM_STATUS_STALE) {
                        LOG_WARNING("R5F_TM", "Device query failed, using STALE cache");
                        send_tm_response(&req, data, data_len, TM_STATUS_STALE);
                    } else {
                        // 无可用数据，返回错误
                        send_tm_error_response(&req, TM_ERROR_NO_DATA);
                    }
                }
            }
        }
        
        OSAL_TaskDelay(10);  // 10ms周期
    }
}
```

#### 7.6.8 遥测应答包格式（带状态字段）

```c
/**
 * @brief 遥测应答包（发送给卫星平台）
 */
typedef struct {
    uint32_t tm_id;              // 遥测ID
    uint8_t status;              // 0=INVALID, 1=FRESH, 2=STALE
    uint64_t timestamp_us;       // 数据时间戳（微秒）
    uint32_t data_len;           // 数据长度
    uint8_t data[256];           // 遥测数据
    uint32_t checksum;           // CRC32校验和
} telemetry_response_t;

/**
 * 卫星平台可以根据status字段判断数据可信度：
 * - FRESH (1)：完全可信，数据在有效期内
 * - STALE (2)：可用但可能过期，谨慎使用
 * - INVALID (0)：不可用，忽略此数据
 */
```

#### 7.6.9 初始化和清理

```c
/**
 * @brief 初始化遥测缓存
 * @return OS_SUCCESS成功，OS_ERROR失败
 */
int32 ACL_InitTelemetryCache(void)
{
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        telemetry_cache_entry_t *entry = &g_tm_cache[i];
        
        // 初始化互斥锁
        int32 ret = OSAL_MutexCreate(&entry->lock, "tm_cache", 0);
        if (ret != OS_SUCCESS) {
            LOG_ERROR("ACL", "Failed to create mutex for tm_id %u", i);
            return OS_ERROR;
        }
        
        // 初始化元数据
        entry->tm_id = i;
        entry->data_len = 0;
        entry->meta.status = TM_STATUS_INVALID;
        entry->meta.timestamp_us = 0;
        entry->meta.validity_ms = 0;
        entry->meta.update_count = 0;
        entry->meta.read_count = 0;
        entry->meta.error_count = 0;
        entry->meta.checksum = 0;
    }
    
    LOG_INFO("ACL", "Telemetry cache initialized: %u entries", TM_FUNC_MAX);
    return OS_SUCCESS;
}

/**
 * @brief 清理遥测缓存
 */
void ACL_CleanupTelemetryCache(void)
{
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        telemetry_cache_entry_t *entry = &g_tm_cache[i];
        OSAL_MutexDelete(entry->lock);
    }
    
    LOG_INFO("ACL", "Telemetry cache cleaned up");
}
```

---

## 9. APP 设计 （业务进程）

### 9.1 进程架构

```text
╔═══════════════════════════════════════════════════════════════════════════════════════╗
║                    ARM Cortex-A53 (4核Linux) - 应用处理器                              ║
╠═══════════════════════════════════════════════════════════════════════════════════════╣
║                          Supervisor Process (监控进程)                                 ║
║  ┌─────────────────────────────────────────────────────────────────────────────────┐  ║
║  │ • 最小化设计 (<300行代码)                                                        │  ║
║  │ • 心跳监控 (共享内存原子时间戳, 2秒周期)                                         │  ║
║  │ • 进程崩溃立即重启 (5次/300秒限制)                                               │  ║
║  │ • 喂硬件看门狗 (GPIO控制, 5秒周期)                                               │  ║
║  │ • 故障日志持久化 (/var/log/pmc/supervisor.log)                                   │  ║
║  │ • CPU绑定: CPU1, SCHED_FIFO Priority 50                                          │  ║
║  └─────────────────────────────────────────────────────────────────────────────────┘  ║
╚═══════════════════════════════════════════════════════════════════════════════════════╝
                    │                    │                    │                    │
        ┌───────────┴───────────┬────────┴────────┬───────────┴──────────┬────────┴────────┐
        │                       │                 │                      │                 │
╔═══════▼═══════════════╗ ╔═════▼═════════════╗ ╔═▼═══════════════╗ ╔═▼══════════════╗ ╔═▼══════════════════╗
║  Telecommand Process  ║ ║ Telemetry Process ║ ║ Firmware Process║ ║ Logger Process ║ ║ Hardware Watchdog  ║
║   (实时优先级)        ║ ║  (实时优先级)     ║ ║  (低优先级)     ║ ║  (低优先级)    ║ ║  (Supervisor管理)  ║
╠═══════════════════════╣ ╠═══════════════════╣ ╠═════════════════╣ ╠════════════════╣ ╠════════════════════╣
║ SCHED_FIFO            ║ ║ SCHED_FIFO        ║ ║ SCHED_OTHER     ║ ║ SCHED_OTHER    ║ ║ 10秒超时           ║
║ Priority: 80          ║ ║ Priority: 70      ║ ║ Nice: 19        ║ ║ Nice: 10       ║ ║ GPIO喂狗           ║
║ CPU: CPU2 (隔离)      ║ ║ CPU: CPU3 (隔离)  ║ ║ CPU: CPU0       ║ ║ CPU: CPU0      ║ ║ 系统复位           ║
╠═══════════════════════╣ ╠═══════════════════╣ ╠═════════════════╣ ╠════════════════╣ ╚════════════════════╝
║ 【核心功能】          ║ ║ 【核心功能】      ║ ║ 【核心功能】    ║ ║ 【核心功能】   ║
║ • CAN接收             ║ ║ • 遥测数据采集    ║ ║ • 固件升级      ║ ║ • 日志收集     ║
║ • 遥控命令处理        ║ ║ • 数据打包        ║ ║ • 校验恢复      ║ ║ • 状态归档     ║
║ • 2ms应答             ║ ║ • 周期上报        ║ ║ • 分区管理      ║ ║ • 崩溃分析     ║
║ • 实时遥测查询        ║ ║ • 健康监控        ║ ║ • 回滚机制      ║ ║ • 日志轮转     ║
╠═══════════════════════╣ ╠═══════════════════╣ ╠═════════════════╣ ╠════════════════╣
║ 【内部线程】          ║ ║ 【内部线程】      ║ ║ 【内部线程】    ║ ║ 【内部线程】   ║
║ ├─ CAN_RX_Handler     ║ ║ ├─ 缓存采集线程   ║ ║ ├─ 升级控制     ║ ║ ├─ 日志聚合    ║
║ │  (最高优先级)       ║ ║ │  (1秒周期)      ║ ║ │  (分片传输)   ║ ║ │  (实时收集)  ║
║ ├─ Realtime_TM_Handler║ ║ ├─ 健康监控线程   ║ ║ └─ 校验线程     ║ ║ ├─ 状态快照    ║
║ │  (高优先级)         ║ ║ │  (5秒周期)      ║ ║    (CRC/SHA256) ║ ║ │  (10秒周期)  ║
║ └─ Telecontrol_Executor║║ └─ 状态快照线程   ║ ║                 ║ ║ ├─ 崩溃分析    ║
║    (中优先级)         ║ ║    (10秒周期)     ║ ║                 ║ ║ └─ 日志轮转    ║
╚═══════════════════════╝ ╚═══════════════════╝ ╚═════════════════╝ ╚════════════════╝
        │                         │                       │                │
        └─────────────────────────┴───────────────────────┴────────────────┘
                                  │
        ┌─────────────────────────▼─────────────────────────────────────┐
        │              进程间通信 (OSAL抽象)                             │
        │  • 共享内存 (遥测缓存, <10μs访问)                              │
        │  • 日志环形缓冲区 (无锁写入)                                   │
        │  • 心跳表 (原子时间戳)                                         │
        └─────────────────────────┬─────────────────────────────────────┘
                                  │
        ┌─────────────────────────▼─────────────────────────────────────┐
        │                    Shared Memory (共享内存区)                  │
        ├────────────────────────────────────────────────────────────────┤
        │ • 遥测缓存 (双缓冲, 原子切换, Telemetry写/Telecommand读)       │
        │ • 心跳表 (原子时间戳, 各进程写/Supervisor读)                   │
        │ • 日志环形缓冲区 (1MB, 无锁写入, 4096条目)                     │
        │ • 状态快照区 (服务器状态、外设状态、通信状态)                  │
        └────────────────────────────────────────────────────────────────┘
```



#### 8.1.1 Supervisor进程（监控进程 - A53侧）

**职责**：

- 启动和监控所有A53侧子进程

- 心跳检测（2秒周期，共享内存原子时间戳）

- 进程崩溃检测和立即重启

- **R5F心跳监控（通过IPC检测R5F状态）**

- **R5F故障恢复（通过RemoteProc接口复位R5F）**

- **喂硬件看门狗（通过IPC请求R5F喂狗，5秒周期）**

- 故障日志记录（持久化到Flash）

**设计原则**：

- 代码量<300行（降低自身故障概率）

- 不依赖任何业务逻辑

- 使用最简单的IPC机制（信号+共享内存心跳表+RPMsg）

- **CPU绑定到CPU0，SCHED_FIFO优先级50（高于普通进程，低于实时任务）**

**重启策略**：

```c
typedef struct {
  uint32_t crash_count;           // 崩溃次数
  uint32_t max_restart;           // 最大重启次数：5
  uint32_t restart_window_sec;    // 重启窗口：300秒
  time_t   last_crash_time;       // 最后崩溃时间
} process_recovery_t;

// A53进程重启逻辑
if (进程崩溃) {
  if (time_since_last_crash > 300秒) {
	  crash_count = 0;  // 重置计数
  }

  if (crash_count < 5) {
	  立即重启进程;
	  crash_count++;
	  记录崩溃日志;
  } else {
	  记录严重故障;
	  触发硬件看门狗复位;  // 系统级恢复
  }
}

// R5F故障恢复逻辑
if (R5F心跳超时 > 5秒) {
  LOG_ERROR("Supervisor", "R5F心跳超时，尝试复位");
  
  // 通过RemoteProc接口复位R5F
  int32_t ret = OSAL_RemoteProc_Reset("r5f0");
  if (ret != OSAL_SUCCESS) {
	  LOG_ERROR("Supervisor", "R5F复位失败，触发系统复位");
	  触发硬件看门狗复位;
  }
  
  记录R5F崩溃日志;
}

心跳监控：
// 共享内存心跳表
typedef struct {
  _Atomic uint64_t telemetry_heartbeat;   // A53侧进程心跳
  _Atomic uint64_t firmware_heartbeat;
  _Atomic uint64_t logger_heartbeat;
  _Atomic uint64_t r5f_heartbeat;         // R5F心跳（R5F写，A53读）
} heartbeat_table_t;

// Supervisor检查逻辑（2秒周期）
uint64_t now = get_monotonic_ns();

// 检查A53侧进程
uint64_t last_hb = atomic_load(&hb_table->telemetry_heartbeat);
if (now - last_hb > 5000000000ULL) {  // 5秒超时
  LOG_ERROR("Supervisor", "telemetry进程心跳超时");
  restart_process(PROC_TELEMETRY);
}

// 检查R5F心跳
uint64_t r5f_hb = atomic_load(&hb_table->r5f_heartbeat);
if (now - r5f_hb > 5000000000ULL) {  // 5秒超时
  LOG_ERROR("Supervisor", "R5F心跳超时");
  handle_r5f_failure();
}

// 喂硬件看门狗（通过IPC请求R5F）
if (now - last_watchdog_feed > 5000000000ULL) {
  ipc_msg_t msg = { .type = IPC_MSG_FEED_WATCHDOG };
  OSAL_RPMsg_Send(RPMSG_CHAN_CONTROL, &msg, sizeof(msg));
  last_watchdog_feed = now;
}

```

**看门狗管理（三级保护）**：

**三级看门狗架构**：

```text
┌─────────────────────────────────────────────────────────┐
│ 硬件看门狗（最后防线）- R5F独占                          │
│  • 独立时钟源（32kHz晶振），不受CPU时钟影响               │
│  • 超时时间：5秒                                         │
│  • 超时动作：复位整个SoC（A53+R5F）                      │
│  • 喂狗方式：R5F Watchdog_Task写硬件寄存器                │
│  • 防护对象：整个系统卡死                                 │
├─────────────────────────────────────────────────────────┤
│ R5F软件看门狗（次级防线）                                │
│  • 监控R5F各任务心跳                                     │
│  • 超时时间：2秒                                         │
│  • 超时动作：R5F自复位（不影响A53）                      │
│  • 喂狗方式：各任务向Watchdog_Task报告心跳                │
│  • 防护对象：R5F任务卡死                                 │
├─────────────────────────────────────────────────────────┤
│ A53进程看门狗（初级防线）- Supervisor进程                │
│  • 监控A53各进程心跳                                     │
│  • 超时时间：500ms                                       │
│  • 超时动作：重启进程 → 重启A53 → 请求R5F复位整个系统     │
│  • 喂狗方式：各进程向Supervisor报告心跳                   │
│  • 防护对象：A53进程卡死                                 │
└─────────────────────────────────────────────────────────┘
```

**硬件看门狗实现（R5F侧）**：

```c
// R5F Watchdog_Task（优先级250，仅次于CAN_RX_Task）
void watchdog_task(void *arg) {
    uint64_t last_feed_time[NUM_R5F_TASKS];
    uint32_t task_timeout_ms[NUM_R5F_TASKS] = {
        500,  // CAN_RX_Task
        500,  // Realtime_TM_Task
        1000, // Telecontrol_Task
        1000, // IPC_Handler_Task
        2000, // Heartbeat_Task
    };
    
    // 初始化硬件看门狗
    RTI_WDT->CTRL = RTI_WDT_ENABLE;
    RTI_WDT->PRELOAD = 5000;  // 5秒超时
    RTI_WDT->KEY = RTI_WDT_KEY_ENABLE;
    
    while (1) {
        uint64_t now = get_time_ms();
        bool all_tasks_alive = true;
        
        // 1. 检查各任务心跳
        for (int i = 0; i < NUM_R5F_TASKS; i++) {
            uint64_t elapsed = now - last_feed_time[i];
            
            if (elapsed > task_timeout_ms[i]) {
                LOG_ERROR("R5F_WD", "Task %d timeout: %llu ms", i, elapsed);
                all_tasks_alive = false;
                
                // 任务超时，触发R5F自复位
                trigger_r5f_self_reset();
                break;
            }
        }
        
        // 2. 只有所有任务都正常，才喂硬件看门狗
        if (all_tasks_alive) {
            RTI_WDT->KEY = RTI_WDT_KEY_FEED;  // 喂狗
        }
        
        // 3. 每100ms检查一次
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 各任务报告心跳
void task_report_heartbeat(uint32_t task_id) {
    extern uint64_t last_feed_time[];
    last_feed_time[task_id] = get_time_ms();
}

// R5F自复位
void trigger_r5f_self_reset(void) {
    LOG_FATAL("R5F_WD", "R5F self-reset triggered");
    
    // 通知A53（通过共享内存）
    shared_mem->r5f_reset_reason = R5F_RESET_WATCHDOG;
    shared_mem->r5f_reset_time = get_time_ms();
    
    // 触发软件复位
    SCB->AIRCR = 0x05FA0004;  // ARM Cortex-R5 软件复位
}
```

**A53进程看门狗实现（Supervisor进程）**：

```c
// 进程看门狗数据结构
typedef struct {
    const char *name;
    pid_t pid;
    uint32_t timeout_ms;
    uint32_t restart_count;
    uint32_t max_restart;
    uint64_t last_heartbeat_time;
    recovery_level_t recovery_level;
} process_watchdog_t;

process_watchdog_t process_watchdogs[] = {
    {"Telecommand", 0, 500, 0, 3, 0, RECOVERY_LEVEL_0_RESTART_PROCESS},
    {"Telemetry",   0, 500, 0, 3, 0, RECOVERY_LEVEL_0_RESTART_PROCESS},
    {"Firmware",    0, 2000, 0, 3, 0, RECOVERY_LEVEL_0_RESTART_PROCESS},
    {"Logger",      0, 2000, 0, 3, 0, RECOVERY_LEVEL_0_RESTART_PROCESS},
};

// 故障恢复等级
typedef enum {
    RECOVERY_LEVEL_0_RESTART_PROCESS,  // 重启进程
    RECOVERY_LEVEL_1_RESTART_A53,      // 重启A53（R5F继续运行）
    RECOVERY_LEVEL_2_RESET_SYSTEM,     // 复位整个系统（通过R5F）
    RECOVERY_LEVEL_3_SAFE_MODE,        // 安全模式（最小功能）
} recovery_level_t;

// Supervisor看门狗检查线程
void supervisor_watchdog_thread(void *arg) {
    while (!supervisor_should_shutdown) {
        uint64_t now = OSAL_GetTimeMs();
        
        for (int i = 0; i < NUM_PROCESSES; i++) {
            process_watchdog_t *wd = &process_watchdogs[i];
            uint64_t elapsed = now - wd->last_heartbeat_time;
            
            if (elapsed > wd->timeout_ms) {
                LOG_ERROR("SUPERVISOR", "Process %s timeout: %llu ms",
                         wd->name, elapsed);
                
                handle_process_timeout(wd);
            }
        }
        
        // 每100ms检查一次
        OSAL_TaskDelay(100);
    }
}

// 处理进程超时
void handle_process_timeout(process_watchdog_t *wd) {
    wd->restart_count++;
    
    // 5分钟内超时3次，升级恢复等级
    if (wd->restart_count >= 3) {
        wd->recovery_level++;
        wd->restart_count = 0;
    }
    
    switch (wd->recovery_level) {
        case RECOVERY_LEVEL_0_RESTART_PROCESS:
            LOG_WARNING("SUPERVISOR", "Restarting process %s", wd->name);
            restart_process(wd);
            break;
            
        case RECOVERY_LEVEL_1_RESTART_A53:
            LOG_ERROR("SUPERVISOR", "Restarting A53 (R5F continues)");
            // R5F继续维持遥控遥测，A53重启
            system("reboot");
            break;
            
        case RECOVERY_LEVEL_2_RESET_SYSTEM:
            LOG_FATAL("SUPERVISOR", "Resetting entire system via R5F");
            // 通过IPC请求R5F复位整个系统
            request_r5f_reset_system();
            break;
            
        case RECOVERY_LEVEL_3_SAFE_MODE:
            LOG_FATAL("SUPERVISOR", "Entering safe mode");
            enter_safe_mode();
            break;
    }
}

// 请求R5F复位整个系统
void request_r5f_reset_system(void) {
    ipc_message_t msg;
    msg.type = IPC_MSG_RESET_SYSTEM;
    msg.data.reset_reason = "A53 process watchdog timeout";
    
    // 发送IPC消息到R5F
    OSAL_RPMsgSend(r5f_endpoint, &msg, sizeof(msg));
    
    // 等待R5F复位（5秒后硬件看门狗会触发）
    OSAL_TaskDelay(5000);
}
```

**R5F心跳监控（Supervisor进程）**：

```c
// 监控R5F心跳
void supervisor_monitor_r5f_heartbeat(void) {
    uint64_t last_r5f_heartbeat = 0;
    uint32_t r5f_timeout_ms = 2000;  // R5F心跳超时2秒
    
    while (!supervisor_should_shutdown) {
        uint64_t now = OSAL_GetTimeMs();
        uint64_t r5f_heartbeat = shared_mem->r5f_heartbeat_time;
        
        if (r5f_heartbeat != last_r5f_heartbeat) {
            // R5F心跳正常
            last_r5f_heartbeat = r5f_heartbeat;
        } else {
            // R5F心跳超时
            uint64_t elapsed = now - last_r5f_heartbeat;
            if (elapsed > r5f_timeout_ms) {
                LOG_ERROR("SUPERVISOR", "R5F heartbeat timeout: %llu ms", elapsed);
                
                // 尝试通过RemoteProc复位R5F
                reset_r5f_via_remoteproc();
            }
        }
        
        OSAL_TaskDelay(500);
    }
}

// 通过RemoteProc复位R5F
void reset_r5f_via_remoteproc(void) {
    LOG_WARNING("SUPERVISOR", "Resetting R5F via RemoteProc");
    
    // 1. 停止R5F
    system("echo stop > /sys/class/remoteproc/remoteproc0/state");
    OSAL_TaskDelay(100);
    
    // 2. 重新加载固件
    system("echo r5f_firmware.elf > /sys/class/remoteproc/remoteproc0/firmware");
    
    // 3. 启动R5F
    system("echo start > /sys/class/remoteproc/remoteproc0/state");
    
    LOG_INFO("SUPERVISOR", "R5F reset completed");
}
```

**看门狗测试**：

```c
// 测试用例：模拟进程卡死
TEST_CASE(test_watchdog_process_timeout) {
    // 1. 停止Telemetry进程的心跳
    stop_process_heartbeat("Telemetry");
    
    // 2. 等待超时（500ms）
    OSAL_TaskDelay(600);
    
    // 3. 验证Supervisor重启了Telemetry进程
    TEST_ASSERT_TRUE(is_process_restarted("Telemetry"));
}

// 测试用例：模拟R5F卡死
TEST_CASE(test_watchdog_r5f_timeout) {
    // 1. 停止R5F心跳
    shared_mem->r5f_heartbeat_time = 0;
    
    // 2. 等待超时（2秒）
    OSAL_TaskDelay(2100);
    
    // 3. 验证Supervisor复位了R5F
    TEST_ASSERT_TRUE(is_r5f_reset());
}
```

**看门狗配置参数**：

| 看门狗类型 | 超时时间 | 检查周期 | 超时动作 | 防护对象 |
|-----------|---------|---------|---------|---------|
| 硬件看门狗 | 5秒 | 100ms | 复位SoC | 整个系统卡死 |
| R5F软件看门狗 | 2秒 | 100ms | R5F自复位 | R5F任务卡死 |
| A53进程看门狗 | 500ms | 100ms | 重启进程 | A53进程卡死 |

**看门狗故障恢复流程**：

```text
进程超时 → 重启进程（3次）
    ↓ 失败
重启A53（R5F继续运行）
    ↓ 失败
R5F复位整个系统
    ↓ 失败
硬件看门狗复位SoC
```

#### 8.1.2 R5F实时任务设计（硬实时域）

**R5F任务架构**：

```text
R5F核心 (FreeRTOS/裸机) - 硬实时域
│
├── Task 1: CAN_RX_Task (优先级255，最高)
│   ├── CAN接收中断处理（<10μs）
│   ├── 命令解析（<200μs）
│   ├── 命令分类：
│   │   ├── 遥控命令 → Telecontrol_Task（<500μs）
│   │   └── 实时型遥测 → Realtime_TM_Task（<800μs）
│   ├── 应答封装（<100μs）
│   ├── CAN发送（<100μs）
│   └── 总延迟：<1.3ms（确定性保证）
│
├── Task 2: Realtime_TM_Task (优先级200)
│   ├── 处理实时型遥测查询
│   ├── 从物理设备读取最新数据：
│   │   ├── 读取MCU状态（CAN查询，<300μs）
│   │   ├── 读取传感器数据（I2C/SPI，<200μs）
│   │   └── 读取GPIO状态（寄存器读取，<10μs）
│   ├── 更新共享内存缓存（供A53读取）
│   └── 返回数据给CAN_RX_Task
│
├── Task 3: Telecontrol_Task (优先级180)
│   ├── 处理遥控命令执行
│   ├── 调用PDL接口控制外设：
│   │   ├── PDL_MCU_SendCommand（CAN发送，<500μs）
│   │   ├── PDL_GPIO_Set（寄存器操作，<10μs）
│   │   └── PDL_Satellite_SendResponse（CAN发送，<100μs）
│   ├── 标记相关遥测缓存为STALE
│   └── 返回执行结果给CAN_RX_Task
│
├── Task 4: IPC_Handler_Task (优先级100)
│   ├── 处理A53发来的RPMsg消息
│   ├── 消息类型：
│   │   ├── 固件升级命令（设置标志位，等待重启）
│   │   ├── 配置更新（更新本地配置）
│   │   └── 日志请求（将日志缓冲区同步到共享内存）
│   └── 非实时路径，不影响硬实时任务
│
├── Task 5: Heartbeat_Task (优先级50)
│   ├── 向A53发送心跳（1秒周期）
│   ├── 更新共享内存心跳时间戳
│   ├── 监控自身健康状态
│   └── 检测A53心跳（超时则降级运行）
│
└── Task 6: Watchdog_Task (优先级250)
    ├── 喂硬件看门狗（5秒周期）
    ├── 监控其他任务心跳
    ├── 检测任务死锁
    └── 故障时触发系统复位
```

**R5F硬实时路径详细分析**：

```c
// Task 1: CAN_RX_Task（最高优先级255）
void CAN_RX_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    can_frame_t frame;
    
    while (!OSAL_TaskShouldShutdown())
    {
        // 等待CAN中断（阻塞，中断唤醒）
        int32_t ret = HAL_CAN_Receive(CAN0_HANDLE, &frame, OSAL_WAIT_FOREVER);
        if (ret != OS_SUCCESS) continue;
        
        uint64_t start_time = OSAL_GetMonotonicNs();  // 记录开始时间
        
        // 1. 命令解析（<200μs）
        tc_command_t cmd;
        ret = parse_can_command(&frame, &cmd);
        if (ret != OS_SUCCESS) {
            send_error_response(&frame, ERROR_INVALID_CMD);
            continue;
        }
        
        // 2. 命令分类（<50μs）
        if (cmd.type == CMD_TYPE_TELECONTROL) {
            // 遥控命令：异步执行
            ret = send_to_telecontrol_task(&cmd);
            send_ack_response(&frame, STATUS_OK);  // 立即应答
        }
        else if (cmd.type == CMD_TYPE_TELEMETRY_CACHED) {
            // 缓存型遥测：直接读共享内存（<10μs）
            tm_data_t data;
            ret = read_tm_cache(cmd.tm_id, &data);
            send_tm_response(&frame, &data);
        }
        else if (cmd.type == CMD_TYPE_TELEMETRY_REALTIME) {
            // 实时型遥测：调用Realtime_TM_Task（<800μs）
            tm_data_t data;
            ret = query_realtime_tm(cmd.tm_id, &data, 1000);  // 1ms超时
            send_tm_response(&frame, &data);
        }
        
        uint64_t end_time = OSAL_GetMonotonicNs();
        uint64_t latency_us = (end_time - start_time) / 1000;
        
        // 确保<1.3ms
        if (latency_us > 1300) {
            LOG_WARN("R5F", "CAN应答超时: %llu us", latency_us);
        }
    }
}

// Task 2: Realtime_TM_Task（优先级200）
void Realtime_TM_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    tm_query_t query;
    
    while (!OSAL_TaskShouldShutdown())
    {
        // 等待查询请求（从CAN_RX_Task发来）
        int32_t ret = OSAL_QueueReceive(g_realtime_tm_queue, &query, OSAL_WAIT_FOREVER);
        if (ret != OS_SUCCESS) continue;
        
        tm_data_t data;
        
        // 根据遥测ID查询物理设备
        switch (query.tm_id) {
            case TM_MCU_STATUS:
                // CAN查询MCU（<300μs）
                ret = PDL_MCU_GetStatus(0, &data.mcu_status);
                break;
                
            case TM_SENSOR_TEMP:
                // I2C读取传感器（<200μs）
                ret = PDL_Sensor_ReadTemp(0, &data.temperature);
                break;
                
            case TM_GPIO_STATE:
                // 寄存器读取GPIO（<10μs）
                ret = HAL_GPIO_Read(GPIO_PIN_5, &data.gpio_state);
                break;
                
            default:
                ret = OS_ERROR;
                break;
        }
        
        // 更新共享内存缓存（供A53读取）
        if (ret == OS_SUCCESS) {
            update_tm_cache(query.tm_id, &data, TM_STATUS_FRESH);
        }
        
        // 返回结果给CAN_RX_Task
        OSAL_QueueSend(query.response_queue, &data, 0);
    }
}

// Task 3: Telecontrol_Task（优先级180）
void Telecontrol_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    tc_command_t cmd;
    
    while (!OSAL_TaskShouldShutdown())
    {
        // 等待遥控命令（从CAN_RX_Task发来）
        int32_t ret = OSAL_QueueReceive(g_telecontrol_queue, &cmd, OSAL_WAIT_FOREVER);
        if (ret != OS_SUCCESS) continue;
        
        // 执行遥控命令
        switch (cmd.tc_id) {
            case TC_MCU_POWER_ON:
                ret = PDL_MCU_SendCommand(0, MCU_CMD_POWER_ON, 0);
                // 标记相关遥测缓存为STALE
                invalidate_tm_cache(TM_MCU_POWER_STATE);
                break;
                
            case TC_GPIO_SET:
                ret = HAL_GPIO_Write(cmd.gpio_pin, cmd.gpio_value);
                invalidate_tm_cache(TM_GPIO_STATE);
                break;
                
            default:
                ret = OS_ERROR;
                break;
        }
        
        // 记录执行结果（可选）
        if (ret != OS_SUCCESS) {
            LOG_ERROR("R5F", "遥控命令执行失败: %d", cmd.tc_id);
        }
    }
}

// Task 4: IPC_Handler_Task（优先级100）
void IPC_Handler_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    ipc_msg_t msg;
    
    while (!OSAL_TaskShouldShutdown())
    {
        // 等待A53发来的RPMsg消息
        int32_t ret = OSAL_RPMsg_Receive(RPMSG_CHAN_CONTROL, &msg, sizeof(msg), OSAL_WAIT_FOREVER);
        if (ret <= 0) continue;
        
        // 处理消息
        switch (msg.type) {
            case IPC_MSG_FW_UPGRADE:
                // 设置固件升级标志位，等待重启
                g_fw_upgrade_pending = true;
                OSAL_Memcpy(&g_fw_upgrade_info, &msg.fw_info, sizeof(fw_info_t));
                break;
                
            case IPC_MSG_CONFIG_UPDATE:
                // 更新本地配置
                update_local_config(&msg.config);
                break;
                
            case IPC_MSG_LOG_REQUEST:
                // 将日志缓冲区同步到共享内存
                sync_log_to_shared_memory();
                break;
                
            default:
                LOG_WARN("R5F", "未知IPC消息类型: %d", msg.type);
                break;
        }
    }
}

// Task 5: Heartbeat_Task（优先级50）
void Heartbeat_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    
    while (!OSAL_TaskShouldShutdown())
    {
        // 更新R5F心跳时间戳（写入共享内存）
        uint64_t now = OSAL_GetMonotonicNs();
        atomic_store(&g_shared_mem->r5f_heartbeat, now);
        
        // 检查A53心跳
        uint64_t a53_hb = atomic_load(&g_shared_mem->a53_heartbeat);
        if (now - a53_hb > 10000000000ULL) {  // 10秒超时
            LOG_WARN("R5F", "A53心跳超时，进入降级模式");
            g_a53_alive = false;
        } else {
            g_a53_alive = true;
        }
        
        // 1秒周期
        OSAL_TaskDelay(1000);
    }
}

// Task 6: Watchdog_Task（优先级250）
void Watchdog_Task(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    uint64_t last_feed_time = OSAL_GetMonotonicNs();
    
    while (!OSAL_TaskShouldShutdown())
    {
        uint64_t now = OSAL_GetMonotonicNs();
        
        // 检查其他任务心跳（通过任务内部心跳表）
        bool all_tasks_alive = check_all_tasks_heartbeat();
        
        if (all_tasks_alive) {
            // 喂硬件看门狗
            HAL_Watchdog_Feed();
            last_feed_time = now;
        } else {
            // 检测到任务死锁，停止喂狗，等待硬件看门狗复位
            LOG_ERROR("R5F", "检测到任务死锁，停止喂狗");
            // 不喂狗，5秒后硬件看门狗会复位系统
        }
        
        // 5秒周期（硬件看门狗超时10秒，留有余量）
        OSAL_TaskDelay(5000);
    }
}
```

**R5F内存布局**：

```c
// TCM（256KB）- 零等待访问
├── R5F代码段（128KB）
│   ├── 中断向量表
│   ├── CAN_RX_Task代码
│   ├── Realtime_TM_Task代码
│   └── 关键路径函数
├── R5F数据段（64KB）
│   ├── 任务栈（6个任务 × 8KB）
│   ├── CAN接收缓冲区（8KB）
│   └── 关键数据结构
└── R5F BSS段（64KB）
    └── 未初始化数据

// DDR（8MB）- 非关键路径
├── 遥测缓存（2MB）
├── 日志缓冲区（2MB）
├── PDL缓冲区（2MB）
└── 预留（2MB）

// 共享内存（4MB）- 与A53通信
├── 遥测缓存（2MB，R5F写，A53读）
├── 心跳表（4KB）
├── 日志环形缓冲区（1MB）
└── RPMsg缓冲区（1MB）
```

**R5F启动流程**：

```c
// R5F main函数
int main(void)
{
    // 1. 硬件初始化
    HAL_Init();
    HAL_CAN_Init(CAN0_HANDLE, 500000);  // 500kbps
    HAL_Watchdog_Init(10000);  // 10秒超时
    
    // 2. 共享内存映射
    g_shared_mem = OSAL_SHM_Map(IPC_SHARED_MEM_BASE, IPC_SHARED_MEM_SIZE);
    
    // 3. RPMsg初始化
    OSAL_RPMsg_Init(RPMSG_CHAN_CONTROL);
    
    // 4. 创建任务
    OSAL_TaskCreate(&g_can_rx_task, "CAN_RX", CAN_RX_Task, NULL, 255);
    OSAL_TaskCreate(&g_realtime_tm_task, "Realtime_TM", Realtime_TM_Task, NULL, 200);
    OSAL_TaskCreate(&g_telecontrol_task, "Telecontrol", Telecontrol_Task, NULL, 180);
    OSAL_TaskCreate(&g_ipc_handler_task, "IPC_Handler", IPC_Handler_Task, NULL, 100);
    OSAL_TaskCreate(&g_heartbeat_task, "Heartbeat", Heartbeat_Task, NULL, 50);
    OSAL_TaskCreate(&g_watchdog_task, "Watchdog", Watchdog_Task, NULL, 250);
    
    // 5. 启动FreeRTOS调度器（如果使用FreeRTOS）
    vTaskStartScheduler();
    
    // 6. 如果是裸机，进入空闲循环
    while (1) {
        __WFI();  // 等待中断
    }
    
    return 0;
}
```

#### 8.1.3 Telemetry进程（遥测采集 - A53侧）

**Telecommand进程架构**：

```text
Telecommand进程 (CPU2隔离, SCHED_FIFO 80)
│
├── CAN_RX_Handler线程 (最高优先级)
│   ├── CAN中断接收（<5μs中断延迟）
│   ├── 命令解析（<50μs）
│   ├── ACL查询（<10μs，O(1)直接索引）
│   ├── 命令类型判断：
│   │   ├── 遥控命令 → 放入遥控队列 → 立即应答STATUS_OK
│   │   ├── 缓存型遥测 → 共享内存读取（<10μs）→ 应答
│   │   └── 实时型遥测 → 调用Realtime_TM_Handler → 应答
│   └── 2ms应答保证（关键路径<1.2ms）
│
├── Realtime_TM_Handler线程 (高优先级)
│   ├── 处理"实时型"遥测请求
│   ├── 直接查询PDL（<500μs）：
│   │   ├── PDL_BMC_GetPowerState（从BMC缓存读，<200μs）
│   │   └── PDL_MCU_GetStatus（CAN查询，<300μs）
│   ├── 超时降级机制（1ms超时）：
│   │   └── 超时后读共享内存缓存（<10μs）
│   └── 更新共享内存缓存（供下次快速读取）
│
└── Telecontrol_Executor线程 (中优先级)
    ├── 从遥控队列取命令
    ├── ACL查询设备映射
    ├── 调用PDL接口（可能耗时）：
    │   ├── PDL_BMC_PowerOn/Off/Reset（1-5秒）
    │   └── PDL_MCU_SendCommand（<500ms）
    └── 异步执行（不阻塞CAN应答）
```

**2ms应答路径优化**：

```c
// CAN_RX_Handler线程（关键路径，运行在CPU2）
void *can_rx_handler_thread(void *arg)
{
    can_frame_t frame;
    uint8_t resp_buffer[64];  // 预分配
    
    // 设置实时调度
    struct sched_param param;
    param.sched_priority = 90;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    
    // 绑定到CPU2
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    
    while (!should_shutdown) {
        // 1. 等待CAN数据（阻塞在epoll）
        int ret = epoll_wait(epoll_fd, events, 1, -1);
        
        uint64_t start_ns = get_monotonic_ns();
        
        // 2. 从CAN socket读取帧（<20μs）
        HAL_CAN_ReadFrame(CAN0, &frame);
        
        // 3. 解析命令（<50μs）
        uint8_t cmd_type = frame.data[0];
        uint32_t param = *(uint32_t*)&frame.data[1];
        
        // 4. ACL查询（<10μs，O(1)直接索引）
        const acl_config_t *cfg = &g_acl_table[cmd_type];
        
        // 5. 处理命令
        if (cmd_type == CMD_TYPE_TELECONTROL) {
            // 遥控：放入队列，立即应答
            queue_push(tc_queue, &frame);
            send_can_response(STATUS_OK, 0);  // <100μs
            
        } else if (cmd_type == CMD_TYPE_TELEMETRY) {
            // 遥测：根据数据类型处理
            if (cfg->data_type == TM_TYPE_CACHED) {
                // 缓存型：从共享内存读（<10μs）
                telemetry_data_t *data = get_cached_tm_from_shm(cfg->tm_id);
                send_can_telemetry_response(cfg->tm_id, data);  // <100μs
                
            } else {
                // 实时型：通知Realtime_TM_Handler，等待结果（<800μs）
                tm_request_t req = { .tm_id = cfg->tm_id, .timeout_ms = 1 };
                queue_push(realtime_tm_queue, &req);
                
                // 等待结果（最多1ms）
                tm_response_t resp;
                if (queue_pop_timeout(realtime_tm_resp_queue, &resp, 1) == 0) {
                    // 实时查询成功
                    send_can_telemetry_response_with_freshness(cfg->tm_id, &resp.data, FRESH);
                } else {
                    // 超时降级到缓存
                    telemetry_data_t *data = get_cached_tm_from_shm(cfg->tm_id);
                    if (data->freshness != INVALID) {
                        send_can_telemetry_response_with_freshness(cfg->tm_id, data, data->freshness);
                    } else {
                        send_can_response(STATUS_ERROR, ERR_NOT_AVAILABLE);
                    }
                }
            }
        }
        
        uint64_t end_ns = get_monotonic_ns();
        uint64_t latency_us = (end_ns - start_ns) / 1000;
        
        // 记录延迟（用于性能分析）
        if (latency_us > 2000) {
            LOG_WARN("CAN_RX", "应答超时: %lu us", latency_us);
        }
    }
    
    return NULL;
}
```

**关键优化点**：

1. **CPU隔离**：进程绑定到CPU2，通过`isolcpus=2`隔离，无调度干扰
2. **实时调度**：SCHED_FIFO优先级80，确保CAN中断后立即调度
3. **中断亲和性**：CAN中断绑定到CPU2，避免跨核中断延迟
4. **内存锁定**：`mlockall()`锁定进程内存，避免页面交换
5. **预分配**：所有缓冲区启动时预分配，运行时零malloc
6. **降级机制**：实时查询超时后立即降级到缓存，保证2ms应答

#### 8.1.3 Telemetry进程（后台遥测采集）

职责：

- 周期性采集缓存型遥测数据

- 写入共享内存双缓冲区

- 健康监控（服务器、BMC、MCU状态）

- 状态快照（供logger进程归档）

线程架构：

``` text
telemetry_process (SCHED_FIFO 70, CPU3隔离)
│
├── cache_collector_thread（1秒周期）
│   ├── 采集缓存型遥测：
│   │   ├── PDL_BMC_ReadSensors（CPU温度、电压、电流）
│   │   ├── PDL_MCU_GetStatus（板卡温度、电源状态）
│   │   └── 系统状态（内存、CPU使用率）
│   ├── 写入双缓冲区：
│   │   ├── write_idx = read_idx ^ 1;  // 切换缓冲区
│   │   ├── buffer[write_idx] = new_data;
│   │   └── atomic_store(&read_idx, write_idx);  // 原子切换
│   └── 容错：采集失败不影响遥控
│
├── health_monitor_thread（5秒周期）
│   ├── 监控服务器健康状态：
│   │   ├── BMC连接状态（ping/心跳）
│   │   ├── 服务器电源状态
│   │   └── 温度/电压异常检测
│   ├── 监控MCU状态：
│   │   ├── CAN通信状态
│   │   └── 看门狗状态
│   └── 更新心跳时间戳（供Supervisor监控）
│
└── status_snapshot_thread（10秒周期）
  ├── 生成状态快照：
  │   ├── 服务器状态（电源、温度、风扇）
  │   ├── 外设状态（BMC、MCU、FPGA）
  │   └── 通信状态（CAN、以太网）
  └── 写入共享内存（供logger进程归档）
```

双缓冲设计（无锁）：

``` C 
typedef struct {
  float cpu_temp;
  float board_temp;
  float voltage_12v;
  float voltage_5v;
  float current;
  uint32_t fan_speed;
  uint64_t timestamp_ns;
  tm_freshness_t freshness;  // 新增：新鲜度标记
  bool valid;
} telemetry_data_t;

typedef struct {
  telemetry_data_t buffer[2];
  _Atomic uint32_t read_index;   // 0或1
} telemetry_cache_t;

// 全局遥测缓存（所有遥测项，包括实时型）
typedef struct {
  telemetry_data_t data;
  uint64_t timestamp_ns;
  tm_freshness_t freshness;   // FRESH / STALE / INVALID
  bool valid;
} telemetry_cache_entry_t;

static telemetry_cache_entry_t g_tm_cache[TM_FUNC_MAX];

// 写入（telemetry进程）
void update_telemetry_cache(telemetry_cache_t *cache, const telemetry_data_t *data)
{
  uint32_t read_idx = atomic_load(&cache->read_index);
  uint32_t write_idx = read_idx ^ 1;  // 切换缓冲区

  cache->buffer[write_idx] = *data;   // 写入新数据

  atomic_store(&cache->read_index, write_idx);  // 原子切换
}

// 读取（telecommand进程）
void get_cached_telemetry(telemetry_cache_t *cache, telemetry_data_t *data)
{
  uint32_t read_idx = atomic_load(&cache->read_index);
  *data = cache->buffer[read_idx];  // 无锁读取
}
```

**Telemetry进程的采集策略（包括实时型遥测的缓存）**：

```c
void* cache_collector_thread(void* arg)
{
  while (1) {
	  uint64_t now = get_monotonic_ns();
	  
	  // 遍历所有遥测项（包括实时型）
	  for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
		  const acl_tm_config_t *cfg = &g_acl_table.tm_table[i];
		  telemetry_cache_entry_t *cache = &g_tm_cache[i];
		  
		  // 根据数据类型决定采集周期
		  uint32_t max_age_ms;
		  if (cfg->data_type == TM_TYPE_REALTIME) {
			  max_age_ms = 500;   // 实时型：500ms采集一次（作为降级备份）
		  } else {
			  max_age_ms = 1000;  // 缓存型：1秒采集一次
		  }
		  
		  uint64_t age_ms = (now - cache->timestamp_ns) / 1000000;
		  if (age_ms >= max_age_ms) {
			  // 采集数据
			  telemetry_data_t data;
			  int32_t ret = collect_telemetry(cfg, &data);
			  
			  if (ret == OSAL_SUCCESS) {
				  cache->data = data;
				  cache->timestamp_ns = now;
				  
				  // 如果之前是STALE，现在恢复为FRESH
				  if (cache->freshness == TM_STALE) {
					  cache->freshness = TM_FRESH;
					  LOG_INFO("TM", "缓存已更新为FRESH: %d", i);
				  }
				  
				  cache->valid = true;
			  }
		  }
	  }
	  
	  OSAL_TaskDelay(100);  // 100ms周期检查
  }
}
```

#### 8.1.4 Firmware进程（固件升级管理）

职责：

- 管理板固件升级（主备分区A/B）

- FPGA固件升级

- 固件校验和恢复

- 升级期间不会收到遥控遥测（卫星平台保证）

线程架构：

``` text
firmware_process (SCHED_BATCH, nice=19)
│
├── upgrade_control_thread（低优先级）
│   ├── 接收升级命令（从卫星平台）
│   ├── 下载固件数据（分片传输）
│   ├── 写入备份分区（A/B切换）
│   └── 升级流程：
│       1. 接收升级命令 → 进入升级模式
│       2. 擦除备份分区
│       3. 接收固件数据（分片，带CRC校验）
│       4. 写入备份分区
│       5. 校验固件完整性（CRC32/SHA256）
│       6. 设置启动标志（原子操作）
│       7. 重启系统（从备份分区启动）
│       8. 启动成功 → 提交升级
│       9. 启动失败 → 自动回滚到主分区
│
└── verify_thread（低优先级）
  ├── CRC32校验
  ├── 签名验证（可选，航天级要求）
  └── 分区切换（原子操作）
```

主备分区机制：

``` C
typedef struct {
  uint32_t magic;           // 魔数：0xDEADBEEF
  uint32_t version;         // 固件版本
  uint32_t size;            // 固件大小
  uint32_t crc32;           // CRC32校验
  uint8_t  sha256[32];      // SHA256签名（可选）
  uint32_t boot_count;      // 启动次数
  uint32_t boot_success;    // 启动成功标志
} firmware_header_t;

// 启动逻辑（Bootloader）
if (partition_B.boot_success == 0 && partition_B.boot_count < 3) {
  // 尝试从B分区启动
  boot_from_partition_B();
  partition_B.boot_count++;
} else if (partition_B.boot_count >= 3) {
  // B分区启动失败3次，回滚到A分区
  boot_from_partition_A();
  partition_B.boot_success = 0;
  partition_B.boot_count = 0;
} else {
  // 正常从A分区启动
  boot_from_partition_A();
}

// 应用启动后（firmware进程）
if (current_partition == B && boot_count > 0) {
  // 标记启动成功
  partition_B.boot_success = 1;
  partition_B.boot_count = 0;
}
```

##### 8.1.4.3 A/B分区升级机制

###### 分区布局

```
eMMC/Flash分区布局（总计256MB）：
┌─────────────────────────────────────────────────────┐
│ Boot Partition（4MB，只读）                          │
│  • U-Boot bootloader                                │
│  • 升级逻辑和签名验证                                 │
│  • 分区选择逻辑                                      │
├─────────────────────────────────────────────────────┤
│ Partition A（80MB）- 当前运行                        │
│  ├─ A53 Kernel（10MB）                              │
│  ├─ A53 Rootfs（60MB）                              │
│  ├─ R5F Firmware（1MB）                             │
│  ├─ FPGA Bitstream（8MB）                           │
│  └─ Metadata（1MB）：版本、签名、CRC、启动计数        │
├─────────────────────────────────────────────────────┤
│ Partition B（80MB）- 备份/升级目标                   │
│  ├─ A53 Kernel（10MB）                              │
│  ├─ A53 Rootfs（60MB）                              │
│  ├─ R5F Firmware（1MB）                             │
│  ├─ FPGA Bitstream（8MB）                           │
│  └─ Metadata（1MB）                                 │
├─────────────────────────────────────────────────────┤
│ Data Partition（75MB）- 持久化数据                   │
│  ├─ 配置文件（5MB）                                  │
│  ├─ 日志文件（50MB）                                 │
│  └─ 遥测历史（20MB）                                 │
├─────────────────────────────────────────────────────┤
│ Recovery Partition（15MB）- 最小恢复系统             │
│  • 最小化Linux + 升级工具                            │
│  • 用于紧急恢复                                      │
└─────────────────────────────────────────────────────┘
```

###### 固件包格式

```c
// 固件包头部（256字节）
typedef struct {
    uint32_t magic;              // 魔数：0x464D5750 ("FWPK")
    uint32_t version;            // 固件版本号
    uint32_t total_size;         // 总大小（字节）
    uint32_t component_count;    // 组件数量
    uint8_t  signature[256];     // RSA-2048签名
    uint8_t  public_key_hash[32];// 公钥SHA-256哈希
    uint64_t build_time;         // 编译时间戳
    char     build_id[64];       // 编译ID（Git commit）
    uint32_t crc32;              // 整个包的CRC32
} firmware_package_header_t;

// 固件组件（每个组件一个）
typedef struct {
    uint32_t type;               // 组件类型（A53_KERNEL/A53_ROOTFS/R5F_FW/FPGA）
    uint32_t offset;             // 在包中的偏移
    uint32_t size;               // 组件大小
    uint32_t target_offset;      // 目标分区偏移
    uint8_t  sha256[32];         // 组件SHA-256哈希
    uint32_t crc32;              // 组件CRC32
} firmware_component_t;

// 组件类型
typedef enum {
    FW_COMPONENT_A53_KERNEL = 1,
    FW_COMPONENT_A53_ROOTFS = 2,
    FW_COMPONENT_R5F_FIRMWARE = 3,
    FW_COMPONENT_FPGA_BITSTREAM = 4,
} firmware_component_type_t;
```

###### 升级流程

```c
// 固件升级状态机
typedef enum {
    FW_STATE_IDLE,
    FW_STATE_DOWNLOADING,
    FW_STATE_VERIFYING,
    FW_STATE_WRITING,
    FW_STATE_VALIDATING,
    FW_STATE_READY_TO_REBOOT,
    FW_STATE_FAILED,
} firmware_upgrade_state_t;

// 固件升级主流程
int32 firmware_upgrade_process(const char *fw_package_path) {
    firmware_upgrade_state_t state = FW_STATE_IDLE;
    firmware_package_header_t header;
    int32 ret;
    
    // 1. 下载固件包（从卫星平台）
    state = FW_STATE_DOWNLOADING;
    ret = download_firmware_from_satellite(fw_package_path);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Download failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 2. 验证固件包签名
    state = FW_STATE_VERIFYING;
    ret = verify_firmware_signature(fw_package_path, &header);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Signature verification failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 3. 验证固件包完整性（CRC32）
    ret = verify_firmware_integrity(fw_package_path, &header);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Integrity check failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 4. 写入到备份分区（Partition B）
    state = FW_STATE_WRITING;
    ret = write_firmware_to_partition(fw_package_path, PARTITION_B);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Write to partition failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 5. 验证写入的数据
    state = FW_STATE_VALIDATING;
    ret = validate_partition_data(PARTITION_B, &header);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Partition validation failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 6. 设置启动标志
    ret = set_boot_partition(PARTITION_B);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Set boot partition failed");
        state = FW_STATE_FAILED;
        return OS_ERROR;
    }
    
    // 7. 准备重启
    state = FW_STATE_READY_TO_REBOOT;
    LOG_INFO("FIRMWARE", "Firmware upgrade ready, rebooting in 5 seconds...");
    
    // 通知卫星平台
    notify_satellite_upgrade_ready();
    
    // 延迟重启，给卫星平台时间接收确认
    OSAL_TaskDelay(5000);
    
    // 8. 重启系统
    system("reboot");
    
    return OS_SUCCESS;
}
```

###### 签名验证

```c
// RSA-2048签名验证
int32 verify_firmware_signature(const char *fw_path, firmware_package_header_t *header) {
    uint8_t public_key[256];
    uint8_t computed_hash[32];
    int32 ret;
    
    // 1. 读取固件包头部
    ret = read_firmware_header(fw_path, header);
    if (ret != OS_SUCCESS) {
        return OS_ERROR;
    }
    
    // 2. 验证魔数
    if (header->magic != 0x464D5750) {
        LOG_ERROR("FIRMWARE", "Invalid magic number: 0x%08x", header->magic);
        return OS_ERROR;
    }
    
    // 3. 加载公钥（从安全存储区）
    ret = load_public_key(public_key, sizeof(public_key));
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Failed to load public key");
        return OS_ERROR;
    }
    
    // 4. 验证公钥哈希
    sha256(public_key, sizeof(public_key), computed_hash);
    if (memcmp(computed_hash, header->public_key_hash, 32) != 0) {
        LOG_ERROR("FIRMWARE", "Public key hash mismatch");
        return OS_ERROR;
    }
    
    // 5. 计算固件包哈希（除签名字段外）
    ret = compute_firmware_hash(fw_path, computed_hash);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "Failed to compute firmware hash");
        return OS_ERROR;
    }
    
    // 6. 验证RSA签名
    ret = rsa_verify(public_key, computed_hash, 32, header->signature, 256);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("FIRMWARE", "RSA signature verification failed");
        return OS_ERROR;
    }
    
    LOG_INFO("FIRMWARE", "Signature verification passed");
    return OS_SUCCESS;
}
```

###### U-Boot启动逻辑

```c
// U-Boot环境变量
boot_partition=A          // 当前启动分区（A或B）
boot_count=0              // 启动尝试次数
boot_max_tries=3          // 最大尝试次数
partition_a_valid=1       // 分区A有效标志
partition_b_valid=0       // 分区B有效标志

// U-Boot启动脚本
if test ${boot_partition} = "A"; then
    if test ${boot_count} -lt ${boot_max_tries}; then
        // 尝试启动分区A
        setenv boot_count $((boot_count + 1))
        saveenv
        load mmc 0:2 ${kernel_addr} /boot/zImage
        load mmc 0:2 ${dtb_addr} /boot/am625-pmc.dtb
        bootz ${kernel_addr} - ${dtb_addr}
    else
        // 分区A启动失败3次，回滚到分区B
        echo "Partition A failed, rolling back to B"
        setenv boot_partition B
        setenv boot_count 0
        setenv partition_a_valid 0
        saveenv
        reset
    fi
else
    // 启动分区B（同样的逻辑）
    ...
fi
```

###### 启动后自检

```c
// Supervisor进程启动后自检
void supervisor_boot_validation(void) {
    char boot_partition[8];
    int boot_count;
    
    // 1. 读取启动分区
    read_uboot_env("boot_partition", boot_partition, sizeof(boot_partition));
    read_uboot_env("boot_count", &boot_count, sizeof(boot_count));
    
    LOG_INFO("SUPERVISOR", "Booted from partition %s, attempt %d",
             boot_partition, boot_count);
    
    // 2. 执行自检
    bool self_test_passed = true;
    
    // 检查关键进程
    if (!check_process_running("Telecommand")) {
        LOG_ERROR("SUPERVISOR", "Telecommand process not running");
        self_test_passed = false;
    }
    
    // 检查R5F心跳
    if (!check_r5f_heartbeat()) {
        LOG_ERROR("SUPERVISOR", "R5F heartbeat not detected");
        self_test_passed = false;
    }
    
    // 检查CAN通信
    if (!check_can_communication()) {
        LOG_ERROR("SUPERVISOR", "CAN communication failed");
        self_test_passed = false;
    }
    
    // 3. 根据自检结果更新启动标志
    if (self_test_passed) {
        // 自检通过，标记当前分区为有效
        LOG_INFO("SUPERVISOR", "Self-test passed, marking partition valid");
        
        if (strcmp(boot_partition, "A") == 0) {
            write_uboot_env("partition_a_valid", "1");
        } else {
            write_uboot_env("partition_b_valid", "1");
        }
        
        // 重置启动计数
        write_uboot_env("boot_count", "0");
    } else {
        // 自检失败，等待U-Boot重启并尝试其他分区
        LOG_ERROR("SUPERVISOR", "Self-test failed, waiting for reboot");
        // U-Boot会在下次启动时检测到boot_count未清零，触发回滚
    }
}
```

###### 回滚机制

```
升级流程：
1. 当前运行分区A（v1.0.0）
2. 下载新固件到分区B（v1.1.0）
3. 验证签名和完整性
4. 设置boot_partition=B, boot_count=0
5. 重启

启动流程：
1. U-Boot读取boot_partition=B
2. 加载分区B的内核和rootfs
3. boot_count++（现在=1）
4. 启动Linux

自检流程：
5. Supervisor执行自检
6. 如果成功：boot_count=0, partition_b_valid=1
7. 如果失败：boot_count保持为1

回滚流程（如果自检失败）：
8. 重启后，U-Boot发现boot_count=1（未清零）
9. boot_count++（现在=2）
10. 再次尝试启动分区B
11. 如果再次失败，boot_count=3
12. boot_count >= boot_max_tries，触发回滚
13. 设置boot_partition=A, boot_count=0
14. 启动分区A（回到v1.0.0）
```

###### 紧急恢复模式

```c
// 如果A/B分区都失败，进入Recovery模式
void enter_recovery_mode(void) {
    LOG_FATAL("SUPERVISOR", "Both partitions failed, entering recovery mode");
    
    // 1. 设置U-Boot环境变量
    write_uboot_env("boot_partition", "RECOVERY");
    
    // 2. 重启进入Recovery分区
    system("reboot");
    
    // Recovery模式功能：
    // - 最小化Linux系统
    // - 通过CAN接收新固件
    // - 重新写入A/B分区
    // - 恢复出厂设置
}
```

###### 固件升级测试

```c
// 测试用例：正常升级
TEST_CASE(test_firmware_upgrade_success) {
    // 1. 准备测试固件包
    create_test_firmware_package("test_fw_v1.1.0.bin");
    
    // 2. 执行升级
    int32 ret = firmware_upgrade_process("test_fw_v1.1.0.bin");
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    
    // 3. 验证分区B已写入
    TEST_ASSERT_TRUE(is_partition_valid(PARTITION_B));
    
    // 4. 验证启动标志已设置
    char boot_partition[8];
    read_uboot_env("boot_partition", boot_partition, sizeof(boot_partition));
    TEST_ASSERT_EQUAL_STRING("B", boot_partition);
}

// 测试用例：签名验证失败
TEST_CASE(test_firmware_upgrade_invalid_signature) {
    // 1. 准备无效签名的固件包
    create_invalid_signature_firmware("test_fw_invalid.bin");
    
    // 2. 执行升级
    int32 ret = firmware_upgrade_process("test_fw_invalid.bin");
    TEST_ASSERT_EQUAL(OS_ERROR, ret);
    
    // 3. 验证分区B未被修改
    TEST_ASSERT_FALSE(is_partition_modified(PARTITION_B));
}

// 测试用例：升级后回滚
TEST_CASE(test_firmware_upgrade_rollback) {
    // 1. 模拟升级到有问题的固件
    simulate_bad_firmware_upgrade();
    
    // 2. 模拟重启和自检失败
    simulate_reboot_and_self_test_fail();
    
    // 3. 验证回滚到分区A
    char boot_partition[8];
    read_uboot_env("boot_partition", boot_partition, sizeof(boot_partition));
    TEST_ASSERT_EQUAL_STRING("A", boot_partition);
}
```

###### 固件升级安全措施总结

| 安全措施 | 目的 | 实现方式 |
|---------|------|---------|
| RSA-2048签名 | 防止恶意固件 | 私钥签名，公钥验证 |
| SHA-256哈希 | 检测篡改 | 每个组件独立哈希 |
| CRC32校验 | 检测传输错误 | 整包和组件双重校验 |
| A/B分区 | 升级失败回滚 | 双分区，原子切换 |
| 启动计数 | 检测启动失败 | U-Boot计数，3次失败回滚 |
| 自检机制 | 验证系统功能 | 启动后检查关键功能 |
| Recovery模式 | 紧急恢复 | 最小化系统，重新刷写 |

#### 8.1.5 Logger进程（日志收集）

职责：

- 收集所有进程的运行日志

- 收集服务器和外设状态信息

- 收集崩溃日志和coredump

- 日志轮转和压缩

- 日志持久化到Flash

线程架构：

``` text
logger_process (SCHED_OTHER, priority=0, nice=10)
│
├── log_collector_thread（实时收集）
│   ├── 从共享内存日志环形缓冲区读取日志
│   ├── 日志来源：
│   │   ├── Supervisor进程日志
│   │   ├── Telecommand进程日志
│   │   ├── Telemetry进程日志
│   │   ├── Firmware进程日志
│   │   └── 内核日志（dmesg）
│   ├── 日志分类：
│   │   ├── ERROR：错误日志（立即写入Flash）
│   │   ├── WARN：警告日志
│   │   ├── INFO：信息日志
│   │   └── DEBUG：调试日志（可配置关闭）
│   └── 写入日志文件：
│       ├── /var/log/pmc/supervisor.log
│       ├── /var/log/pmc/telecommand.log
│       ├── /var/log/pmc/telemetry.log
│       └── /var/log/pmc/firmware.log
│
├── status_archiver_thread（10秒周期）
│   ├── 从共享内存读取状态快照
│   ├── 归档内容：
│   │   ├── 服务器状态：
│   │   │   ├── 电源状态（开/关/未知）
│   │   │   ├── CPU温度、板卡温度
│   │   │   ├── 电压（12V/5V/3
│   │   │   ├── 电流、功率
│   │   │   └── 风扇转速
│   │   ├── 外设状态：
│   │   │   ├── BMC连接状态（主通道/备份通道）
│   │   │   ├── MCU通信状态（CAN总线状态）
│   │   │   ├── FPGA状态（配置状态、温度）
│   │   │   └── 看门狗状态
│   │   ├── 通信状态：
│   │   │   ├── CAN总线统计（收发计数、错误计数）
│   │   │   ├── 以太网统计（丢包率、延迟）
│   │   │   └── 串口统计
│   │   └── 系统状态：
│   │       ├── CPU使用率、内存使用率
│   │       ├── 进程状态（运行/崩溃/重启次数）
│   │       └── 系统运行时间
│   └── 写入状态日志：
│       └── /var/log/pmc/status.log（JSON格式）
│
├── crash_analyzer_thread（事件触发）
│   ├── 监听进程崩溃信号（SIGCHLD）
│   ├── 收集崩溃信息：
│   │   ├── 进程名称、PID
│   │   ├── 崩溃时间、信号类型（SIGSEGV/SIGABRT）
│   │   ├── 崩溃前日志（最后100行）
│   │   ├── Coredump文件（如果启用）
│   │   └── 调用栈（backtrace）
│   ├── 生成崩溃报告：
│   │   └── /var/log/pmc/crash/crash_<timestamp>.log
│   └── 通知Supervisor（触发重启）
│
└── log_rotator_thread（每小时执行）
  ├── 日志轮转策略：
  │   ├── 单个日志文件最大10MB
  │   ├── 保留最近7天日志
  │   ├── 超过限制自动压缩（gzip）
  │   └── 压缩后移动到归档目录
  ├── 日志文件管理：
  │   ├── /var/log/pmc/*.log（当前日志）
  │   ├── /var/log/pmc/archive/*.log.gz（归档日志）
  │   └── /var/log/pmc/crash/*.log（崩溃日志，永久保留）
  └── Flash空间管理：
	  ├── 监控Flash剩余空间
	  ├── 空间不足时删除最旧归档
	  └── 保留最近24小时日志（不可删除）
```

共享内存日志环形缓冲区：

``` C
#define LOG_RING_BUFFER_SIZE (1024 * 1024)  // 1MB

typedef struct {
  uint64_t timestamp_ns;
  uint32_t pid;
  uint8_t  level;  // ERROR/WARN/INFO/DEBUG
  char     module[16];
  char     message[256];
} log_entry_t;

typedef struct {
  log_entry_t entries[4096];  // 环形缓冲区
  _Atomic uint32_t write_index;
  _Atomic uint32_t read_index;
  _Atomic uint32_t lost_count;  // 丢失日志计数
} log_ring_buffer_t;

// 写入日志（各进程）
void LOG_Write(uint8_t level, const char *module, const char *fmt, .)
{
  log_entry_t entry;
  entry.timestamp_ns = get_monotonic_ns();
  entry.pid = getpid();
  entry.level = level;
  strncpy(entry.module, module, 16);

  va_list args;
  va_start(args, fmt);
  vsnprintf(entry.message, 256, fmt, args);
  va_end(args);

  uint32_t write_idx = atomic_fetch_add(&g_log_buffer->write_index, 1) % 4096;
  g_log_buffer->entries[write_idx] = entry;
}

// 读取日志（logger进程）
bool LOG_Read(log_entry_t *entry)
{
  uint32_t read_idx = atomic_load(&g_log_buffer->read_index);
  uint32_t write_idx = atomic_load(&g_log_buffer->write_index);

  if (read_idx == write_idx) {
	  return false;  // 无新日志
  }

  *entry = g_log_buffer->entries[read_idx % 4096];
  atomic_store(&g_log_buffer->read_index, read_idx + 1);
  return true;
}
```

日志格式示例：

1. 运行日志（/var/log/pmc/telecommand.log）

``` text
2026-05-16 10:23:45.123456 [INFO] [TC] CAN命令接收: cmd=0x01, param=0x00
2026-05-16 10:23:45.123789 [INFO] [TC] ACL查询: TC_POWER_ON -> BMC[0]
2026-05-16 10:23:45.124012 [INFO] [TC] 应答发送: status=OK, latency=556us
2026-05-16 10:23:46.234567 [ERROR] [TC] BMC通信超时: ip=192.168.1.100, timeout=500ms
2026-05-16 10:23:46.234890 [WARN] [TC] 切换到备份通道: IPMI over Serial
```

2. 状态日志（/var/log/pmc/status.log，JSON格式）

``` json
{
"timestamp": "2026-05-16T10:23:50.000Z",
"server": {
  "power_state": "on",
  "cpu_temp": 65.5,
  "board_temp": 45.2,
  "voltage_12v": 12.05,
  "voltage_5v": 5.02,
  "current": 3.5,
  "fan_speed": 4500
},
"peripherals": {
  "bmc": {
	"connected": true,
	"channel": "primary",
	"protocol": "redfish",
	"response_time_ms": 120
  },
  "mcu": {
	"connected": true,
	"can_status": "ok",
	"temp": 42.0,
	"watchdog_enabled": true
  },
  "fpga": {
	"temp": 55.0
  }
},
"communication": {
  "can": {
	"tx_count": 12345,
	"rx_count": 12340,
	"error_count": 5
  },
  "ethernet": {
	"packet_loss": 0.01,
	"latency_ms": 2.5
  }
},
"system": {
  "cpu_usage": 35.5,
  "memory_usage": 45.2,
  "uptime_sec": 86400,
  "processes": {
	"telecommand": "running",
	"telemetry": "running",
	"firmware": "running",
	"logger": "running"
  }
}
}
```

3. 崩溃日志（/var/log/pmc/crash/crash_20260516_102350.log）

``` text
=== Process Crash Report ===
Time: 2026-05-16 10:23:50.123456
Process: telemetry
PID: 1234
Signal: SIGSEGV (Segmentation fault)
Address: 0x00000000 (NULL pointer dereference)

=== Last 10 Log Entries ===
2026-05-16 10:23:49.123456 [INFO] [TM] 开始采集遥测数据
2026-05-16 10:23:49.234567 [INFO] [TM] BMC传感器读取成功
2026-05-16 10:23:49.345678 [ERROR] [TM] MCU通信失败: timeout
2026-05-16 10:23:50.123456 [ERROR] [TM] 进程崩溃: SIGSEGV

=== Backtrace ===
#0  0x00007f1234567890 in cache_collector_thread () at telemetry.c:123
#1  0x00007f1234567900 in pthread_start () at pthread.c:456
#2  0x00007f1234567910 in clone () at clone.S:78

=== Coredump ===
Coredump saved to: /var/log/pmc/crash/core.1234

=== Recovery Action ===
Supervisor restarted telemetry process at 2026-05-16 10:23:51.000000
```

---



### 8.2. 可靠性设计

#### 8.2.1 三级故障恢复

| 级别              | 触发条件                     | 恢复机制                 | 恢复时间 | 适用场景                  |
| ----------------- | ---------------------------- | ------------------------ | -------- | ------------------------- |
| **级别1：线程级** | 线程崩溃                     | 进程内检测并重启线程     | <100ms   | 非关键线程（如日志线程）  |
| **级别2：进程级** | 进程崩溃                     | Supervisor检测并重启进程 | <1秒     | 所有进程（5次/300秒限制） |
| **级别3：系统级** | Supervisor崩溃或进程重启超限 | 硬件看门狗触发系统复位   | 10秒     | 系统级故障，从主分区启动  |

#### 8.2.2 进程隔离效果

| 场景      | 崩溃进程        | 检测机制           | 恢复时间 | 影响范围                                                     | 恢复后状态                                       |
| --------- | --------------- | ------------------ | -------- | ------------------------------------------------------------ | ------------------------------------------------ |
| **场景1** | Telemetry进程   | Supervisor心跳超时 | <1秒     | 缓存型遥测返回旧数据（标记stale）<br>实时型遥测正常<br>遥控功能不受影响 | 1秒内恢复正常采集                                |
| **场景2** | Telecommand进程 | Supervisor心跳超时 | <1秒     | 遥控遥测短暂中断<br>卫星平台检测到心跳丢失                   | 立即恢复通信<br>连续崩溃5次→系统复位             |
| **场景3** | Firmware进程    | Supervisor心跳超时 | <1秒     | 固件升级中断<br>遥控遥测不受影响                             | 未完成→保持当前分区<br>已完成→下次从备份分区启动 |

#### 8.2.3 辐射容错

单粒子翻转（SEU）防护：

``` C
// 1. 进程级隔离
//    SEU翻转只影响单个进程地址空间
//    其他进程不受影响

// 2. 关键数据保护
typedef struct {
  uint32_t magic;      // 魔数：0xDEADBEEF
  uint32_t data;       // 实际数据
  uint32_t crc32;      // CRC32校验
} protected_data_t;

// 3. 周期性校验（每秒）
void verify_critical_data(protected_data_t *data)
{
  if (data->magic != 0xDEADBEEF || calc_crc32(&data->data) != data->crc32) {
	  LOG_ERROR("SEU", "数据损坏检测，恢复中.");
	  restore_from_backup(data);
  }
}

// 4. 共享内存ECC保护（硬件支持）
//    使用支持ECC的DDR内存
//    单比特错误自动纠正
//    双比特错误触发中断

单粒子闩锁（SEL）防护：
// 1. 硬件看门狗强制复位
//    Supervisor喂狗周期：5秒
//    看门狗超时：10秒
//    超时后硬件复位

// 2. 电源监控芯片
//    监控异常电流（SEL特征）
//    检测到异常 → 触发电源复位
```

#### 8.2.4 降级运行模式

| 模式          | 运行进程                        | 可用功能                                | 受限功能             | 触发条件             |
| ------------- | ------------------------------- | --------------------------------------- | -------------------- | -------------------- |
| **正常模式**  | 全部4个进程                     | 遥控、遥测（缓存+实时）、固件升级、日志 | 无                   | 系统正常运行         |
| **降级模式1** | Telecommand + Firmware + Logger | 遥控、实时型遥测、固件升级、日志        | 缓存型遥测返回旧数据 | Telemetry进程崩溃    |
| **降级模式2** | Telecommand + Logger            | 遥控、日志                              | 遥测功能不可用       | Telemetry连续崩溃5次 |
| **安全模式**  | 仅Supervisor                    | 喂看门狗                                | 遥控遥测全部不可用   | Telecommand进程禁用  |
| **系统复位**  | 重启中                          | 无                                      | 全部功能暂停         | 硬件看门狗触发       |

---

### 8.3. 多核异构优化策略

#### 8.3.1 CPU调度优化（A53侧）

``` C
// 1. Supervisor进程：实时调度，高优先级，绑定CPU0
struct sched_param param;
param.sched_priority = 50;
sched_setscheduler(0, SCHED_FIFO, &param);
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// 2. Telemetry进程：普通调度，低优先级，CPU1-3
param.sched_priority = 0;
sched_setscheduler(0, SCHED_OTHER, &param);
setpriority(PRIO_PROCESS, 0, 10);  // nice=10
CPU_ZERO(&cpuset);
for (int i = 1; i <= 3; i++) CPU_SET(i, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// 3. Firmware进程：批处理调度，最低优先级，CPU1-3
sched_setscheduler(0, SCHED_BATCH, &param);
setpriority(PRIO_PROCESS, 0, 19);  // nice=19

// 4. Logger进程：普通调度，低优先级，CPU1-3
param.sched_priority = 0;
sched_setscheduler(0, SCHED_OTHER, &param);
setpriority(PRIO_PROCESS, 0, 10);  // nice=10
```

**A53侧调度效果（多核负载均衡）**：

| 进程        | 调度策略    | 优先级  | CPU绑定 | CPU时间片分配         | 说明                      |
| ----------- | ----------- | ------- | ------- | --------------------- | ------------------------- |
| Supervisor  | SCHED_FIFO  | 50      | CPU0    | 独占CPU0（<5%负载）   | 监控进程，高优先级        |
| Telemetry   | SCHED_OTHER | nice=10 | CPU1-3  | 共享CPU1-3（<10%负载）| 后台采集，负载均衡        |
| Logger      | SCHED_OTHER | nice=10 | CPU1-3  | 共享CPU1-3（<3%负载） | 日志收集，低优先级        |
| Firmware    | SCHED_BATCH | nice=19 | CPU1-3  | 系统空闲时运行        | 最低优先级，升级时独占    |

#### 8.3.2 R5F侧任务优先级配置

``` C
// FreeRTOS任务优先级配置（数字越大优先级越高）
#define PRIORITY_WATCHDOG       (configMAX_PRIORITIES - 1)  // 最高优先级：7
#define PRIORITY_CAN_RX         (configMAX_PRIORITIES - 2)  // 次高优先级：6
#define PRIORITY_IPC_HANDLER    (configMAX_PRIORITIES - 3)  // 高优先级：5
#define PRIORITY_REALTIME_TM    (configMAX_PRIORITIES - 4)  // 高优先级：4
#define PRIORITY_TELECONTROL    (configMAX_PRIORITIES - 5)  // 中优先级：3
#define PRIORITY_HEARTBEAT      (configMAX_PRIORITIES - 7)  // 低优先级：1

// 任务创建
xTaskCreateStatic(CAN_RX_Task, "CAN_RX", 2048, NULL, PRIORITY_CAN_RX, 
                  can_rx_stack, &can_rx_tcb);
xTaskCreateStatic(Realtime_TM_Task, "RT_TM", 1024, NULL, PRIORITY_REALTIME_TM,
                  rt_tm_stack, &rt_tm_tcb);
xTaskCreateStatic(Telecontrol_Task, "TC", 1024, NULL, PRIORITY_TELECONTROL,
                  tc_stack, &tc_tcb);
xTaskCreateStatic(IPC_Handler_Task, "IPC", 1024, NULL, PRIORITY_IPC_HANDLER,
                  ipc_stack, &ipc_tcb);
xTaskCreateStatic(Heartbeat_Task, "HB", 512, NULL, PRIORITY_HEARTBEAT,
                  hb_stack, &hb_tcb);
xTaskCreateStatic(Watchdog_Task, "WDT", 512, NULL, PRIORITY_WATCHDOG,
                  wdt_stack, &wdt_tcb);

// 核心亲和性配置（R5F双核）
vTaskCoreAffinitySet(can_rx_task_handle, (1 << 0));      // R5F_0
vTaskCoreAffinitySet(rt_tm_task_handle, (1 << 0));       // R5F_0
vTaskCoreAffinitySet(tc_task_handle, (1 << 1));          // R5F_1
vTaskCoreAffinitySet(ipc_task_handle, (1 << 1));         // R5F_1
vTaskCoreAffinitySet(hb_task_handle, (1 << 1));          // R5F_1
vTaskCoreAffinitySet(wdt_task_handle, (1 << 1));         // R5F_1
```

**R5F侧调度效果（双核实时）**：

| 任务            | 优先级 | 核心  | 说明                      |
| --------------- | ------ | ----- | ------------------------- |
| Watchdog_Task   | 7      | R5F_1 | 最高优先级，喂狗          |
| CAN_RX_Task     | 6      | R5F_0 | 次高优先级，2ms应答       |
| IPC_Handler     | 5      | R5F_1 | 高优先级，核间通信        |
| Realtime_TM     | 4      | R5F_0 | 高优先级，实时遥测        |
| Telecontrol     | 3      | R5F_1 | 中优先级，遥控执行        |
| Heartbeat       | 1      | R5F_1 | 低优先级，心跳更新        |

**实时性保证（多核异构）**：

| 场景          | 处理流程               | 实时性保证   | 核心 |
| ------------- | ---------------------- | ------------ | ---- |
| 遥控命令到达  | R5F CAN中断 → CAN_RX_Task | <1.13ms应答 | R5F_0 |
| 快遥/慢遥到达 | R5F CAN中断 → CAN_RX_Task | <100μs应答  | R5F_0 |
| 实时遥测查询  | R5F Realtime_TM_Task   | <700μs完成  | R5F_0 |
| 后台采集      | A53 Telemetry进程      | 不影响实时性 | A53 CPU1-3 |
| 核间通信      | RPMsg + 共享内存       | <100μs延迟  | A53↔R5F |

#### 8.3.3 中断优化（多核异构）

**A53侧（Linux）**：

``` shell
# 1. 禁用CPU0的非关键中断（专用于Supervisor）
echo 0 > /proc/irq/XX/smp_affinity  # 禁用非关键中断

# 2. 网络中断绑定到CPU1-3
echo e > /proc/irq/YY/smp_affinity  # 0b1110 = CPU1-3

# 3. 使用PREEMPT_RT内核（可选）
# 提升Linux实时性，但R5F已保证硬实时
```

**R5F侧（FreeRTOS）**：

``` C
// 1. CAN中断配置（最高优先级）
HwiP_Params hwiParams;
HwiP_Params_init(&hwiParams);
hwiParams.priority = 0;  // 最高优先级
hwiParams.arg = (uintptr_t)can_handle;
HwiP_create(CAN0_INT, CAN_ISR, &hwiParams);

// 2. Mailbox中断配置（核间通信）
hwiParams.priority = 1;  // 次高优先级
HwiP_create(MAILBOX_INT, Mailbox_ISR, &hwiParams);

// 3. 中断嵌套配置
// FreeRTOS允许中断嵌套，CAN中断可抢占Mailbox中断
```

#### 8.3.4 内存优化（多核异构）

**A53侧（DDR）**：

``` C
// 1. 预分配所有内存（避免运行时分配）
static telemetry_cache_t g_tm_cache;
static uint8_t g_log_buffer[1024 * 1024];

// 2. 锁定Supervisor进程内存（避免swap和缺页中断）
mlockall(MCL_CURRENT | MCL_FUTURE);

// 3. 共享内存配置为Non-cacheable（与R5F共享）
int shm_fd = shm_open("/ipc_shm", O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, 16 * 1024 * 1024);  // 16MB
void *shm = mmap(NULL, 16 * 1024 * 1024, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_NONCACHE, shm_fd, 0);  // Non-cacheable
```

**R5F侧（TCM + DDR）**：

``` C
// 1. TCM内存布局（256KB，零等待）
#pragma DATA_SECTION(g_acl_table, ".tcm_data")
acl_config_t g_acl_table[256];  // ACL查找表放在TCM

#pragma CODE_SECTION(CAN_RX_Task, ".tcm_code")
void CAN_RX_Task(void *pvParameters) {
  // 关键代码放在TCM
}

// 2. 共享内存映射（固定物理地址）
#define IPC_SHM_BASE  0x70000000  // DDR中的共享内存区
ipc_shared_memory_t *shm = (ipc_shared_memory_t *)IPC_SHM_BASE;

// 3. 预分配所有缓冲区（静态分配）
static uint8_t can_rx_buffer[1024] __attribute__((section(".tcm_data")));
static uint8_t can_tx_buffer[1024] __attribute__((section(".tcm_data")));
```

#### 8.3.5 缓存优化（多核异构）

**A53侧（Cacheable DDR）**：

``` C
// 1. 数据结构对齐（避免false sharing）
typedef struct {
  _Atomic uint32_t read_index;
  uint8_t padding1[60];  // 填充到64字节（缓存行大小）
  _Atomic uint32_t write_index;
  uint8_t padding2[60];
} cache_aligned_queue_t;

// 2. 热路径数据局部性
typedef struct {
  // 热数据（频繁访问）
  _Atomic uint64_t heartbeat;
  uint32_t cmd_count;
  uint32_t error_count;

  // 冷数据（偶尔访问）
  char name[64];
  uint32_t config_flags;
} hot_cold_separated_t;

// 3. 预取关键数据
__builtin_prefetch(&g_acl_table, 0, 3);  // 预取ACL查找表

```

---

### 8.4 进程系统配置

```ini
# /etc/pmc/pmc.conf

# PMC系统配置文件

[system]
platform = ti/am625
product = pmc_v1
version = 1.0.0

[supervisor]
heartbeat_interval_ms = 2000
heartbeat_timeout_ms = 5000
watchdog_feed_interval_ms = 5000
max_restart_count = 5
restart_window_sec = 300

[telecommand]
sched_policy = SCHED_FIFO
sched_priority = 99
cpu_affinity = 0
memory_lock = true
can_device = can0
can_bitrate = 500000
cmd_timeout_ms = 100

[telemetry]
sched_policy = SCHED_OTHER
sched_priority = 0
nice = 10
cache_collect_interval_ms = 1000
health_check_interval_ms = 5000
status_snapshot_interval_ms = 10000

[firmware]
sched_policy = SCHED_BATCH
nice = 19
partition_a = /dev/mmcblk0p1
partition_b = /dev/mmcblk0p2
max_boot_attempts = 3

[logger]
sched_policy = SCHED_OTHER
nice = 10
log_level = INFO
log_dir = /var/log/pmc
max_log_size_mb = 10
max_archive_days = 7
crash_log_dir = /var/log/pmc/crash

```

---
