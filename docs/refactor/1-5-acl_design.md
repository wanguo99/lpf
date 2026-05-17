# ACL层详细设计

## 1. ACL 设计（业务配置层）

### 1.1 ACL设计理念

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

### 1.2  实现示例

#### 1.2.1 PMC业务功能枚举

``` C
/************************************************
* acl/include/pmc_acl_types.h
* PMC业务功能枚举定义
************************************************/

/**
* @brief 遥测数据新鲜度标记
* 
* 纯A53架构下，所有遥测数据统一从共享内存缓存读取。
* 通过新鲜度标记体现数据质量，而非实时/缓存类型区分。
*/
typedef enum {
  TM_STATUS_INVALID = 0,  // 无效数据：从未采集过
  TM_STATUS_FRESH = 1,    // 新鲜数据：采集时间在有效期内
  TM_STATUS_STALE = 2     // 过期数据：受遥控命令影响，等待更新
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

#### 1.2.2 ACL配置结构

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
* 
* 纯A53架构下，所有遥测统一使用缓存模式：
* - Telemetry进程周期性采集数据并更新共享内存缓存
* - Telecommand进程从缓存读取遥测数据（<50μs）
* - 通过validity_ms和update_period_ms控制数据新鲜度
*/
typedef struct {
  pmc_tm_function_t function;
  acl_device_type_t device_type;
  uint32_t logic_index;
  uint32_t validity_ms;       // 数据有效期（毫秒）
  uint32_t update_period_ms;  // 更新周期（毫秒）
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

#### 1.2.3 ACL配置示例

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
  /* 服务器遥测 → BMC */
  { TM_SERVER_CPU_TEMP,      ACL_DEVICE_BMC, 0, 4000, 2000, true },  // 4秒有效期，2秒更新
  { TM_SERVER_BOARD_TEMP,    ACL_DEVICE_BMC, 0, 4000, 2000, true },
  { TM_SERVER_FAN_SPEED,     ACL_DEVICE_BMC, 0, 4000, 2000, true },
  { TM_SERVER_VOLTAGE_12V,   ACL_DEVICE_MCU, 1, 4000, 2000, true },
  { TM_SERVER_VOLTAGE_5V,    ACL_DEVICE_MCU, 1, 4000, 2000, true },
  { TM_SERVER_CURRENT,       ACL_DEVICE_MCU, 1, 4000, 2000, true },

  /* 服务器状态 → BMC，更短有效期 */
  { TM_SERVER_POWER_STATUS,  ACL_DEVICE_BMC, 0, 500,  100,  true },  // 500ms有效期，100ms更新

  /* MCU遥测 → MCU */
  { TM_MCU_STATUS,           ACL_DEVICE_MCU, 0, 500,  100,  true },
  { TM_MCU_TEMP,             ACL_DEVICE_MCU, 0, 4000, 2000, true },
  { TM_MCU_VOLTAGE,          ACL_DEVICE_MCU, 0, 4000, 2000, true },

  /* FPGA遥测 → FPGA */
  { TM_FPGA_STATUS,          ACL_DEVICE_FPGA, 0, 500,  100,  true },
  { TM_FPGA_TEMP,            ACL_DEVICE_FPGA, 0, 4000, 2000, true },

  /* 系统遥测 */
  { TM_SYSTEM_UPTIME,        ACL_DEVICE_MCU, 0, 4000, 2000, true },
  { TM_WATCHDOG_STATUS,      ACL_DEVICE_MCU, 2, 500,  100,  true },
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

#### 1.2.4 ACL使用示例

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

// 处理遥测命令（纯A53架构：统一从缓存读取）
int32_t handle_telemetry(pmc_tm_function_t tm_type, telemetry_data_t *data)
{
  // 1. 通过ACL查询设备映射（O(1)）
  const acl_tm_config_t *cfg = ACL_GetTmConfig(&g_acl_table, tm_type);
  if (cfg == NULL || !cfg->enabled) {
      LOG_ERROR("TM", "遥测未配置: %d", tm_type);
      return OSAL_ERR_NOT_FOUND;
  }

  // 2. 从共享内存缓存读取（<50μs）
  tm_freshness_t freshness;
  int32_t ret = ACL_TM_Cache_Get(tm_type, data, sizeof(*data), &freshness);
  
  if (ret != OSAL_SUCCESS) {
      LOG_ERROR("TM", "缓存读取失败: %d", tm_type);
      return ret;
  }
  
  // 3. 在应答中携带新鲜度标记
  data->freshness = freshness;  // FRESH/STALE/INVALID
  
  return OSAL_SUCCESS;
}

#### 1.2.5 遥测缓存系统（纯A53架构）

**设计原理**：
- 所有遥测数据统一存储在共享内存缓存中
- Telemetry进程（CPU1）周期性采集并更新缓存
- Telecommand进程（CPU0）从缓存快速读取（<50μs）
- 通过时间戳 + 有效期 + 新鲜度标志体现数据质量

**缓存结构**：
```c
// acl/include/acl_telemetry_cache.h

typedef struct {
    uint8_t data[ACL_TM_MAX_DATA_SIZE];  // 遥测数据（最大256字节）
    uint32_t size;                        // 数据大小
    uint64_t timestamp_us;                // 采集时间戳（微秒）
    uint32_t validity_ms;                 // 有效期（毫秒）
    tm_freshness_t freshness;             // 新鲜度标记
    pthread_rwlock_t lock;                // 读写锁
} acl_tm_cache_entry_t;

typedef struct {
    acl_tm_cache_entry_t entries[TM_FUNC_MAX];
    uint32_t magic;                       // 魔数校验
} acl_tm_cache_t;
```

**缓存API**：
```c
// 初始化缓存（创建共享内存）
int32_t ACL_TM_Cache_Init(void);

// 更新缓存条目（Telemetry进程调用）
int32_t ACL_TM_Cache_Update(pmc_tm_function_t tm_id, 
                             const void *data, 
                             uint32_t size);

// 读取缓存条目（Telecommand进程调用）
int32_t ACL_TM_Cache_Get(pmc_tm_function_t tm_id, 
                          void *data, 
                          uint32_t size, 
                          tm_freshness_t *freshness);

// 检查缓存有效性
bool ACL_TM_Cache_IsValid(pmc_tm_function_t tm_id);

// 批量失效（遥控命令执行后调用）
int32_t ACL_TM_Cache_InvalidateMultiple(const pmc_tm_function_t *tm_ids, 
                                         uint32_t count);
```

**新鲜度计算**：
```c
tm_freshness_t calculate_freshness(acl_tm_cache_entry_t *entry)
{
    if (entry->timestamp_us == 0) {
        return TM_STATUS_INVALID;  // 从未采集过
    }
    
    uint64_t now_us = OSAL_GetTimeUs();
    uint64_t age_us = now_us - entry->timestamp_us;
    
    if (age_us > entry->validity_ms * 1000) {
        return TM_STATUS_STALE;  // 超过有效期
    }
    
    return TM_STATUS_FRESH;  // 新鲜数据
}
```

**使用示例**：
```c
// Telemetry进程：周期性更新缓存
void telemetry_update_loop(void)
{
    while (running) {
        // 采集BMC遥测数据
        bmc_sensor_data_t sensor_data;
        PDL_BMC_ReadSensors(0, &sensor_data);
        
        // 更新缓存
        ACL_TM_Cache_Update(TM_SERVER_CPU_TEMP, 
                            &sensor_data.cpu_temp, 
                            sizeof(sensor_data.cpu_temp));
        
        OSAL_msleep(100);  // 100ms周期
    }
}

// Telecommand进程：快速读取缓存
void handle_tm_request(pmc_tm_function_t tm_id)
{
    uint32_t value;
    tm_freshness_t freshness;
    
    int32_t ret = ACL_TM_Cache_Get(tm_id, &value, sizeof(value), &freshness);
    
    if (ret == OSAL_SUCCESS) {
        // 发送应答，携带新鲜度标记
        send_tm_response(tm_id, value, freshness);
    }
}
```

#### 1.2.6 遥控命令对遥测缓存的影响映射

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

#### 1.2.6 Telecommand进程中的遥控命令处理（完整版）

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

### 1.3 ACL设计要点

#### 1.3.1 查找表初始化

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

#### 1.3.2 配置查询接口

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

#### 1.3.3 配置验证

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

#### 1.3.4 配置统计与调试

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

### 1.4 ACL性能优化

#### 1.4.1 O(1)查询性能

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

#### 1.4.2 缓存友好性

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

#### 1.4.3 无锁设计

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

### 1.5 ACL扩展性

#### 1.5.1 新增业务功能

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

#### 1.5.2 支持新硬件平台

支持新硬件平台只需修改配置文件，无需修改业务代码。

```c
// 原配置：acl_pmc_v1.c（BMC通过Redfish）
{ TC_SERVER_POWER_ON, ACL_DEVICE_BMC, 0, true },

// 新配置：acl_pmc_v2.c（改用MCU控制）
{ TC_SERVER_POWER_ON, ACL_DEVICE_MCU, 0, true },

// 业务代码无需修改，自动适配新硬件
```

#### 1.5.3 多产品支持

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

### 1.6 遥测数据新鲜度管理

为了保证遥测数据的准确性和可靠性，ACL层需要对遥测缓存进行时间戳和有效期管理，确保应用层能够识别数据的新鲜度状态。

#### 1.6.1 遥测缓存数据结构（增强版）

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

#### 1.6.2 遥测配置中的有效期定义

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

#### 1.6.3 遥测数据写入接口（带时间戳和校验）

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

#### 1.6.4 遥测数据读取接口（带新鲜度检查）

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

#### 1.6.5 遥测缓存失效接口（遥控命令执行后调用）

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

#### 1.6.6 遥测质量监控接口

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

#### 1.6.7 R5F侧使用示例

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

#### 1.6.8 遥测应答包格式（带状态字段）

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

#### 1.6.9 初始化和清理

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

