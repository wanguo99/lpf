# ACL 模块架构分析报告

## 1. 模块职责与边界分析

### 1.1 架构定位

ACL（Application Configuration Layer，应用配置层）在 EMS 架构中处于**业务逻辑与硬件实现的桥接层**，位于依赖链的顶端：

```
Apps → ACL → PDL → (PRL, HAL, PCL) → OSAL
```

根据设计原则，ACL 的核心职责应该是：
- **业务功能到物理设备的映射**（功能 ID → 设备类型 + 逻辑索引）
- **配置管理**（集中管理业务与硬件的映射关系）
- **O(1) 查询性能**（通过数组直接索引）

### 1.2 职责边界问题

**问题 1：职责混淆 - 遥测缓存不属于配置层**

`acl_telemetry_cache.c` 实现了完整的遥测数据缓存系统，包括：
- 数据存储（256 字节缓冲区）
- 时间戳管理
- 新鲜度判断（FRESH/STALE/INVALID）
- CRC32 校验
- 互斥锁保护
- 统计信息

**根本问题**：这是**运行时数据管理**，不是**配置管理**。

```c
// acl_telemetry_cache.c:14
static telemetry_cache_entry_t g_tm_cache[TM_FUNC_MAX];  // 1000 * 300+ 字节 ≈ 300KB
```

**影响**：
1. **内存占用过大**：TM_FUNC_MAX=1000，每个条目 ~300 字节，总计约 300KB 静态内存
2. **职责越界**：ACL 应该只管"配置"，不应该管"数据"
3. **依赖倒置**：缓存管理应该在更高层（Apps 层或独立的 Data Management 层）
4. **可测试性差**：配置层混入运行时状态，难以单元测试

**建议**：
- 将 `acl_telemetry_cache.*` 移出 ACL，创建独立的 `TelemetryCache` 模块
- ACL 只保留配置查询接口，不管理运行时数据
- 缓存模块依赖 ACL 获取配置（validity_ms, update_period_ms），但不属于 ACL

---

**问题 2：业务枚举定义位置不当**

`acl_tc.h` 和 `acl_tm.h` 定义了具体的业务功能枚举：

```c
// acl_tc.h:15-43
typedef enum {
    TC_POWER_ON = 0,
    TC_POWER_OFF = 1,
    TC_MCU_RESET = 102,
    TC_FIRMWARE_UPGRADE_START = 200,
    // ...
    TC_FUNC_MAX = 1000
} acl_tc_function_t;
```

**问题**：
- ACL 声称是"通用框架"（README.md:4），但包含了具体业务枚举
- 不同产品的业务功能不同（卫星 vs 工业控制 vs 医疗设备）
- 修改业务功能需要修改 ACL 核心代码

**建议**：
- 将业务枚举移到 `products/` 目录（如 `products/ccm/include/ccm_functions.h`）
- ACL 只提供通用类型 `uint32_t function_id`（已在 acl_types.h 中定义）
- 通过 `ACL_RegisterTable()` 注入业务配置

---

**问题 3：配置表硬编码在 ACL 层**

README.md 示例显示配置表定义在 `acl/config/pmc_v1/acl_pmc_v1.c`：

```c
const acl_tc_config_t g_pmc_tc_configs[] = {
    { TC_SERVER_POWER_ON,  ACL_DEVICE_BMC, 0, true },
    // ...
};
```

**问题**：
- 配置表应该属于产品层（products/ccm），不应该在 core/acl
- 违反了"Core 不依赖 Products"的架构原则
- 不同产品需要修改 ACL 源码

**实际情况**：检查目录结构发现 `config/` 目录不存在，说明文档与实现不一致。

---

### 1.3 正确的职责划分

| 层级 | 职责 | 示例 |
|------|------|------|
| **Apps** | 业务逻辑、数据处理 | 遥测采集进程、遥控处理进程 |
| **Data Management** | 运行时数据缓存、共享内存 | 遥测缓存、日志缓冲区 |
| **ACL** | 业务功能→设备映射（只读配置） | TC_POWER_ON → BMC[0] |
| **PDL** | 设备驱动（协议编解码+传输） | BMC_PowerOn(0) |
| **PRL/HAL/PCL** | 协议层、硬件抽象、外设配置 | CAN 发送、UART 配置 |
| **OSAL** | 操作系统抽象 | 线程、互斥锁、时间 |

---

## 2. 目录结构与代码组织分析

### 2.1 当前目录结构

```
core/acl/
├── include/
│   ├── acl_api.h              # ✅ 核心 API
│   ├── acl_types.h            # ✅ 通用类型定义
│   ├── acl_config.h           # ⚠️  仅包含其他头文件
│   ├── acl_tc.h               # ❌ 业务枚举（应移到 products/）
│   ├── acl_tm.h               # ❌ 业务枚举（应移到 products/）
│   └── acl_telemetry_cache.h  # ❌ 运行时缓存（应独立模块）
├── src/
│   ├── acl_api.c              # ✅ 核心实现
│   └── acl_telemetry_cache.c  # ❌ 运行时缓存（应独立模块）
├── CMakeLists.txt
├── Kconfig
├── API.md
└── README.md
```

### 2.2 问题分析

**问题 4：头文件职责不清**

`acl_config.h` 只是一个聚合头文件：

```c
// acl_config.h:1-14
#ifndef ACL_CONFIG_H
#define ACL_CONFIG_H

#include "acl_types.h"
#include "acl_tc.h"
#include "acl_tm.h"

#endif
```

**问题**：
- 文件名暗示"配置"，但实际只是包含其他头文件
- 引入了不应该在 ACL 的业务枚举（acl_tc.h, acl_tm.h）
- 增加了编译依赖

**建议**：删除此文件，让用户显式包含需要的头文件。

---

**问题 5：公开/私有头文件未分离**

所有头文件都在 `include/` 目录，没有区分公开 API 和内部实现：

```
include/
├── acl_api.h              # 公开 API
├── acl_types.h            # 公开类型
├── acl_telemetry_cache.h  # 公开 API（但不应该在 ACL）
├── acl_tc.h               # 业务枚举（不应该在 ACL）
└── acl_tm.h               # 业务枚举（不应该在 ACL）
```

**建议**：
```
include/
├── acl_api.h              # 公开 API
└── acl_types.h            # 公开类型
src/
└── acl_internal.h         # 内部实现（如果需要）
```

---

**问题 6：缺少配置表示例**

README.md 提到 `config/pmc_v1/` 目录，但实际不存在：

```bash
# 预期结构
acl/
├── config/
│   └── pmc_v1/
│       ├── acl_pmc_v1.c
│       └── acl_pmc_v1_invalidation.c
```

**实际情况**：目录不存在，说明文档过时或未实现。

**建议**：
- 配置表应该在 `products/ccm/config/` 目录
- ACL 只提供注册机制，不包含具体配置

---

## 3. 接口设计分析

### 3.1 核心 API 设计

`acl_api.h` 定义了 8 个核心接口：

```c
int32_t ACL_Init(void);
int32_t ACL_RegisterTable(const acl_config_table_t *table);
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id);
const acl_tm_config_t* ACL_GetTmConfig(uint32_t function_id);
bool ACL_IsTcEnabled(uint32_t function_id);
bool ACL_IsTmEnabled(uint32_t function_id);
int32_t ACL_GetInvalidationMap(uint32_t source_tm_id, ...);
int32_t ACL_GetStatistics(acl_statistics_t *stats);
void ACL_PrintConfig(void);
```

### 3.2 设计优点

✅ **O(1) 查询性能**：直接数组索引

```c
// acl_api.c:56-59
if (function_id < g_acl_table->tc_count) {
    return &g_acl_table->tc_table[function_id];
}
```

✅ **返回 const 指针**：防止误修改配置

```c
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id);
```

✅ **注册机制**：支持运行时注入配置

```c
int32_t ACL_RegisterTable(const acl_config_table_t *table);
```

---

### 3.3 接口设计问题

**问题 7：function_id 与数组索引的假设未验证**

代码假设 `function_id` 就是数组索引：

```c
// acl_api.c:56-59
if (function_id < g_acl_table->tc_count) {
    return &g_acl_table->tc_table[function_id];
}
```

**问题**：
- 如果配置表不是按 function_id 顺序排列，会返回错误配置
- 没有验证 `tc_table[i].function_id == i`
- 文档要求"枚举值必须从 0 开始连续递增"（README.md:281），但代码未检查

**示例错误配置**：

```c
const acl_tc_config_t g_tc_configs[] = {
    { .function_id = 10, .device_type = ACL_DEVICE_BMC, ... },  // 索引 0
    { .function_id = 20, .device_type = ACL_DEVICE_MCU, ... },  // 索引 1
};

// 查询 function_id=10 时，会返回索引 10 的配置（越界或错误）
ACL_GetTcConfig(10);  // 期望返回 BMC，实际返回 NULL 或错误数据
```

**建议**：
1. 在 `ACL_RegisterTable()` 中验证配置表：
   ```c
   for (i = 0; i < table->tc_count; i++) {
       if (table->tc_table[i].function_id != i) {
           LOG_ERROR("ACL", "TC config[%u] has wrong function_id %u", 
                     i, table->tc_table[i].function_id);
           return OSAL_ERR_INVALID_CONFIG;
       }
   }
   ```

2. 或者使用哈希表/二分查找，不依赖索引顺序

---

**问题 8：ACL_GetInvalidationMap 接口设计不佳**

```c
int32_t ACL_GetInvalidationMap(uint32_t source_tm_id,
                                uint32_t *affected_ids,
                                uint32_t max_count,
                                uint32_t *actual_count);
```

**问题**：
1. **需要预分配缓冲区**：调用者必须猜测 `max_count`
2. **可能截断数据**：如果 `actual_count > max_count`，数据被截断但返回成功
3. **两次调用模式**：需要先调用获取 `actual_count`，再分配缓冲区，再调用一次

**当前实现的 bug**：

```c
// acl_api.c:124-128
uint32_t copy_count = (map->affected_count < max_count) ?
                       map->affected_count : max_count;
OSAL_Memcpy(affected_ids, map->affected_tm_ids, copy_count * sizeof(uint32_t));
*actual_count = map->affected_count;  // ⚠️ 返回真实数量，但可能已截断
return OSAL_SUCCESS;  // ⚠️ 截断时应该返回错误
```

**建议**：
```c
// 方案 1：返回指针（零拷贝）
int32_t ACL_GetInvalidationMap(uint32_t source_tm_id,
                                const uint32_t **affected_ids,
                                uint32_t *count);

// 方案 2：明确截断错误
if (map->affected_count > max_count) {
    return OSAL_ERR_BUFFER_TOO_SMALL;
}
```

---

**问题 9：缺少配置验证接口**

README.md 提到 `ACL_ValidateConfig()`（第 134 行），但 `acl_api.h` 中不存在此接口。

**建议**：添加验证接口

```c
/**
 * @brief 验证配置表的完整性和一致性
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 */
int32_t ACL_ValidateConfig(void);
```

实现应检查：
- function_id 连续性
- device_type 有效性
- logic_index 合理性
- enabled 配置的完整性
- 失效映射的循环依赖

---

**问题 10：线程安全性未明确**

README.md 声称"无锁设计"（第 257 行），但未明确说明：
- `ACL_RegisterTable()` 是否可以在多线程环境调用？
- 查询接口是否真的线程安全？

**当前实现**：

```c
// acl_api.c:10
static const acl_config_table_t *g_acl_table = NULL;

// acl_api.c:25-44
int32_t ACL_RegisterTable(const acl_config_table_t *table)
{
    if (NULL != g_acl_table) {
        LOG_WARN("ACL", "Table already registered, overwriting");
    }
    g_acl_table = table;  // ⚠️ 非原子操作，多线程不安全
    return OSAL_SUCCESS;
}
```

**问题**：
- 指针赋值在某些架构上不是原子操作
- 如果一个线程正在注册，另一个线程查询，可能读到部分更新的指针

**建议**：
1. 明确文档说明：`ACL_RegisterTable()` 必须在单线程初始化阶段调用
2. 添加初始化状态检查：
   ```c
   static bool g_acl_initialized = false;
   
   int32_t ACL_RegisterTable(const acl_config_table_t *table) {
       if (g_acl_initialized) {
           return OSAL_ERR_ALREADY_INITIALIZED;
       }
       g_acl_table = table;
       __sync_synchronize();  // 内存屏障
       g_acl_initialized = true;
       return OSAL_SUCCESS;
   }
   ```

---

## 4. 实现质量分析

### 4.1 acl_api.c 实现分析

**优点**：
- ✅ 代码简洁（204 行）
- ✅ 逻辑清晰
- ✅ 使用 OSAL 抽象（日志、内存操作）

**问题 11：错误处理不一致**

```c
// acl_api.c:52-54
if (NULL == g_acl_table || NULL == g_acl_table->tc_table) {
    return NULL;  // ⚠️ 未记录日志
}

// acl_api.c:27-30
if (NULL == table) {
    LOG_ERROR("ACL", "Invalid table pointer");  // ✅ 记录日志
    return OSAL_ERR_INVALID_POINTER;
}
```

**建议**：统一错误处理策略

```c
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id)
{
    if (NULL == g_acl_table) {
        LOG_ERROR("ACL", "Table not registered");
        return NULL;
    }
    
    if (NULL == g_acl_table->tc_table) {
        LOG_ERROR("ACL", "TC table is NULL");
        return NULL;
    }
    
    if (function_id >= g_acl_table->tc_count) {
        LOG_WARN("ACL", "TC function_id %u out of range (max %u)", 
                 function_id, g_acl_table->tc_count);
        return NULL;
    }
    
    return &g_acl_table->tc_table[function_id];
}
```

---

**问题 12：ACL_GetInvalidationMap 的 O(n) 查找**

```c
// acl_api.c:120-130
for (i = 0; i < g_acl_table->inv_count; i++) {
    const acl_invalidation_map_t *map = &g_acl_table->inv_map[i];
    if (map->source_tm_id == source_tm_id) {
        // 找到映射
    }
}
```

**问题**：
- 线性查找，时间复杂度 O(n)
- 与 README.md 声称的"O(1) 查询性能"不一致

**建议**：
1. 如果失效映射数量少（<10），线性查找可接受
2. 如果数量多，使用哈希表或将 `source_tm_id` 作为数组索引

---

### 4.2 acl_telemetry_cache.c 实现分析

**问题 13：硬编码数组大小**

```c
// acl_telemetry_cache.c:14
static telemetry_cache_entry_t g_tm_cache[TM_FUNC_MAX];  // TM_FUNC_MAX = 1000
```

**问题**：
- 静态分配 1000 个条目，每个 ~300 字节，总计 ~300KB
- 即使只使用 10 个遥测，也占用 300KB
- 不符合嵌入式系统的内存约束

**建议**：
1. 使用动态分配：
   ```c
   static telemetry_cache_entry_t *g_tm_cache = NULL;
   static uint32_t g_tm_cache_size = 0;
   
   int32_t ACL_TelemetryCache_Init(uint32_t max_tm_count) {
       g_tm_cache = OSAL_Malloc(max_tm_count * sizeof(telemetry_cache_entry_t));
       g_tm_cache_size = max_tm_count;
   }
   ```

2. 或使用哈希表（稀疏存储）

---

**问题 14：互斥锁粒度过粗**

```c
// acl_telemetry_cache.c:103-120
OSAL_MutexLock(g_cache_mutex);
entry = &g_tm_cache[tm_id];
OSAL_Memcpy(entry->data, data, data_len);  // 最多 256 字节
entry->timestamp_us = get_monotonic_us();
entry->checksum = calculate_crc32(data, data_len);  // 计算密集
OSAL_MutexUnlock(g_cache_mutex);
```

**问题**：
- 单个全局锁保护整个缓存表
- 写入一个条目时，阻塞所有其他读写操作
- CRC32 计算在锁内进行（最多 256 字节，~1-2μs）

**影响**：
- 多线程并发性能差
- 违反了"<50μs 读取"的性能目标（README.md:64）

**建议**：
1. 使用细粒度锁（每个条目一个锁）：
   ```c
   typedef struct {
       osal_mutex_t *lock;
       telemetry_cache_entry_t entry;
   } cache_slot_t;
   ```

2. 或使用读写锁（多读单写）：
   ```c
   static osal_rwlock_t *g_cache_rwlock = NULL;
   
   int32_t ACL_TelemetryCache_Read(...) {
       OSAL_RWLockReadLock(g_cache_rwlock);
       // 读取数据
       OSAL_RWLockReadUnlock(g_cache_rwlock);
   }
   ```

3. 或使用无锁数据结构（原子操作 + 内存屏障）

---

**问题 15：CRC32 计算效率低**

```c
// acl_telemetry_cache.c:31-45
static uint32_t calculate_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {  // ⚠️ 内层循环，8 次移位
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}
```

**问题**：
- 逐位计算，时间复杂度 O(8n)
- 对于 256 字节数据，需要 2048 次循环
- 在互斥锁内执行，影响并发性能

**建议**：
1. 使用查表法（空间换时间）：
   ```c
   static const uint32_t crc32_table[256] = { /* 预计算表 */ };
   
   static uint32_t calculate_crc32(const uint8_t *data, uint32_t len) {
       uint32_t crc = 0xFFFFFFFF;
       for (uint32_t i = 0; i < len; i++) {
           crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
       }
       return ~crc;
   }
   ```

2. 或使用硬件 CRC（如果 SoC 支持）

3. 或在锁外计算 CRC：
   ```c
   uint32_t checksum = calculate_crc32(data, data_len);
   OSAL_MutexLock(g_cache_mutex);
   OSAL_Memcpy(entry->data, data, data_len);
   entry->checksum = checksum;
   OSAL_MutexUnlock(g_cache_mutex);
   ```

---

**问题 16：时间戳精度不一致**

```c
// acl_telemetry_cache.c:23-26
static uint64_t get_monotonic_us(void)
{
    return OSAL_GetMonotonicTime();  // ⚠️ 返回值单位未知
}
```

**问题**：
- 函数名暗示返回微秒，但 `OSAL_GetMonotonicTime()` 的单位未明确
- 如果 OSAL 返回毫秒，所有时间计算都会错误

**建议**：
1. 检查 OSAL 接口定义，确认单位
2. 添加单位转换：
   ```c
   static uint64_t get_monotonic_us(void)
   {
       uint64_t ms = OSAL_GetMonotonicTime();
       return ms * 1000;  // 毫秒转微秒
   }
   ```

---

**问题 17：数据竞争风险**

```c
// acl_telemetry_cache.c:154-160
if (entry->freshness == ACL_TM_STATUS_INVALID) {
    freshness = ACL_TM_STATUS_INVALID;
} else if (age_ms < entry->validity_ms) {  // ⚠️ validity_ms 可能未初始化
    freshness = ACL_TM_STATUS_FRESH;
} else {
    freshness = ACL_TM_STATUS_STALE;
}
```

**问题**：
- `entry->validity_ms` 在 `ACL_TelemetryCache_Write()` 中未设置
- 依赖外部（ACL 配置）提供，但未验证

**建议**：
1. 在 `Write()` 时从 ACL 配置读取 `validity_ms`：
   ```c
   int32_t ACL_TelemetryCache_Write(uint32_t tm_id, const uint8_t *data, uint32_t data_len)
   {
       const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_id);
       if (cfg == NULL) {
           return OSAL_ERR_NOT_FOUND;
       }
       
       entry->validity_ms = cfg->validity_ms;
       // ...
   }
   ```

2. 或在 `Init()` 时预填充 `validity_ms`

---

**问题 18：缺少边界检查**

```c
// acl_telemetry_cache.c:99-101
if (!g_cache_initialized || tm_id >= TM_FUNC_MAX || !data || data_len == 0 || data_len > 256) {
    return OSAL_ERR_INVALID_SIZE;  // ⚠️ 错误码不准确
}
```

**问题**：
- 多种错误（未初始化、ID 越界、指针为空、长度错误）返回同一错误码
- 调用者无法区分错误原因

**建议**：
```c
if (!g_cache_initialized) {
    return OSAL_ERR_NOT_INITIALIZED;
}
if (tm_id >= TM_FUNC_MAX) {
    return OSAL_ERR_INVALID_PARAM;
}
if (!data) {
    return OSAL_ERR_INVALID_POINTER;
}
if (data_len == 0 || data_len > 256) {
    return OSAL_ERR_INVALID_SIZE;
}
```

---

## 5. 依赖关系分析

### 5.1 CMakeLists.txt 分析

```cmake
# acl/CMakeLists.txt:23-24
list(APPEND ADD_REQUIREMENTS osal pdl)
```

**声明的依赖**：
- OSAL（操作系统抽象层）✅ 合理
- PDL（外设驱动层）❌ **不合理**

### 5.2 依赖倒置问题

**问题 19：ACL 不应该依赖 PDL**

根据架构设计：
```
Apps → ACL → PDL → (PRL, HAL, PCL) → OSAL
```

ACL 是**配置层**，应该只依赖 OSAL（类型定义、日志）。

**当前实现**：
- `acl_api.c` 只使用了 OSAL（LOG_*, OSAL_Memcpy, OSAL_Memset）
- `acl_telemetry_cache.c` 只使用了 OSAL（互斥锁、时间、内存）
- **没有直接调用 PDL 接口**

**结论**：CMakeLists.txt 中的 `pdl` 依赖是**错误的**。

**建议**：
```cmake
# 修正后的依赖
list(APPEND ADD_REQUIREMENTS osal)
```

---

**问题 20：Kconfig 依赖配置错误**

```kconfig
# acl/Kconfig:6-7
select OSAL
select PDL
```

**问题**：
- `select PDL` 会强制启用 PDL 及其所有依赖（HAL, PCL, PRL）
- ACL 实际上不需要 PDL
- 导致不必要的模块被编译

**建议**：
```kconfig
config ACL
    bool "Application Configuration Layer"
    default y
    select OSAL
    help
      Application Configuration Layer provides mapping between
      business functions and physical devices.
```

---

## 6. 文档质量分析

### 6.1 README.md 分析

**优点**：
- ✅ 结构清晰（概述、架构、配置、API、性能）
- ✅ 包含示例代码
- ✅ 说明了设计目标

**问题 21：文档与实现不一致**

1. **配置表位置**：
   - 文档：`acl/config/pmc_v1/acl_pmc_v1.c`（第 89 行）
   - 实际：目录不存在

2. **API 接口**：
   - 文档：`ACL_ValidateConfig()`（第 134 行）
   - 实际：`acl_api.h` 中不存在

3. **性能指标**：
   - 文档：查询延迟 <50μs（第 64 行）
   - 实际：未提供性能测试代码

**建议**：
- 更新文档，移除不存在的配置表示例
- 删除未实现的 API 说明，或实现该 API
- 添加性能测试代码

---

**问题 22：缺少关键设计决策说明**

文档未解释：
- 为什么选择数组索引而不是哈希表？
- 为什么 function_id 必须连续？
- 如何处理稀疏的 function_id 空间？
- 为什么遥测缓存在 ACL 而不是独立模块？

**建议**：添加"设计决策"章节

```markdown
## 设计决策

### 为什么使用数组索引？

- **性能**：O(1) 查询，无哈希冲突
- **简单**：代码简洁，易于调试
- **限制**：要求 function_id 连续，不适合稀疏 ID 空间

### 如何处理稀疏 ID？

如果业务功能 ID 不连续（如 1, 100, 200），有两种方案：
1. 使用哈希表（牺牲性能换取灵活性）
2. 重新映射 ID（在产品层定义连续的枚举）
```

---

### 6.2 API.md 分析

**问题 23：API 文档过于简略**

当前 API.md 只列出了函数签名，缺少：
- 参数详细说明
- 返回值含义
- 错误码列表
- 使用示例
- 线程安全性说明
- 性能特征

**建议**：使用 Doxygen 风格注释

```c
/**
 * @brief 获取遥控功能的配置信息
 * 
 * @param[in] function_id 功能 ID（必须 < TC_FUNC_MAX）
 * 
 * @return 配置指针（只读），失败返回 NULL
 * 
 * @note 线程安全：是（只读操作）
 * @note 时间复杂度：O(1)
 * @note 调用前必须先调用 ACL_RegisterTable()
 * 
 * @warning 返回的指针指向静态配置表，不要修改或释放
 * 
 * @example
 * const acl_tc_config_t *cfg = ACL_GetTcConfig(TC_POWER_ON);
 * if (cfg && cfg->enabled) {
 *     PDL_SendCommand(cfg->device_type, cfg->logic_index, ...);
 * }
 */
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id);
```

---

## 7. 测试覆盖率分析

### 7.1 当前测试状态

检查 `tests/core/acl/` 目录：

**发现**：测试目录不存在或为空。

**问题 24：缺少单元测试**

ACL 模块没有单元测试，无法验证：
- 配置表注册是否正确
- 边界条件处理（ID 越界、空指针）
- 失效映射查找逻辑
- 统计信息准确性
- 遥测缓存的新鲜度判断

**影响**：
- 无法保证代码质量
- 重构时容易引入 bug
- 无法验证性能指标

---

### 7.2 建议的测试用例

```c
// tests/core/acl/test_acl_api.c

// 测试 1：基本配置注册和查询
void test_acl_register_and_query(void)
{
    const acl_tc_config_t tc_configs[] = {
        { 0, ACL_DEVICE_BMC, 0, true },
        { 1, ACL_DEVICE_MCU, 0, true },
    };
    
    acl_config_table_t table = {
        .tc_table = tc_configs,
        .tc_count = 2,
    };
    
    assert(ACL_RegisterTable(&table) == OSAL_SUCCESS);
    
    const acl_tc_config_t *cfg = ACL_GetTcConfig(0);
    assert(cfg != NULL);
    assert(cfg->device_type == ACL_DEVICE_BMC);
}

// 测试 2：边界条件
void test_acl_boundary_conditions(void)
{
    // 未注册时查询
    assert(ACL_GetTcConfig(0) == NULL);
    
    // ID 越界
    ACL_RegisterTable(&valid_table);
    assert(ACL_GetTcConfig(9999) == NULL);
    
    // 空指针
    assert(ACL_RegisterTable(NULL) == OSAL_ERR_INVALID_POINTER);
}

// 测试 3：失效映射
void test_acl_invalidation_map(void)
{
    uint32_t affected_ids[10];
    uint32_t actual_count;
    
    int32_t ret = ACL_GetInvalidationMap(TM_TEMPERATURE, 
                                          affected_ids, 10, &actual_count);
    assert(ret == OSAL_SUCCESS);
    assert(actual_count == 3);  // 假设有 3 个受影响的遥测
}

// 测试 4：遥测缓存
void test_acl_telemetry_cache(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    
    // 写入
    assert(ACL_TelemetryCache_Write(0, data, 3) == OSAL_SUCCESS);
    
    // 读取
    uint8_t read_buf[256];
    uint32_t read_len;
    acl_tm_status_t status;
    
    assert(ACL_TelemetryCache_Read(0, read_buf, &read_len, &status) == OSAL_SUCCESS);
    assert(read_len == 3);
    assert(memcmp(read_buf, data, 3) == 0);
    assert(status == ACL_TM_STATUS_FRESH);
}

// 测试 5：性能测试
void test_acl_performance(void)
{
    uint64_t start = OSAL_GetMonotonicTime();
    
    for (int i = 0; i < 10000; i++) {
        ACL_GetTcConfig(i % 100);
    }
    
    uint64_t elapsed = OSAL_GetMonotonicTime() - start;
    double avg_us = (elapsed * 1000.0) / 10000;
    
    printf("Average query time: %.2f μs\n", avg_us);
    assert(avg_us < 1.0);  // 应该远小于 1μs
}
```

---

## 8. 架构改进建议

### 8.1 短期改进（1-2 周）

**优先级 1：修复依赖关系**
- 移除 CMakeLists.txt 和 Kconfig 中的 `pdl` 依赖
- 验证编译通过

**优先级 2：修复接口 bug**
- 在 `ACL_RegisterTable()` 中验证 function_id 连续性
- 修复 `ACL_GetInvalidationMap()` 的截断错误
- 统一错误处理和日志记录

**优先级 3：改进错误处理**
- 区分不同错误类型，返回准确的错误码
- 所有错误路径都记录日志

---

### 8.2 中期改进（1-2 月）

**优先级 4：重构遥测缓存**
- 将 `acl_telemetry_cache.*` 移到独立模块（`core/tm_cache/`）
- 使用动态分配替代静态数组
- 优化 CRC32 计算（查表法）
- 使用细粒度锁或读写锁

**优先级 5：移除业务枚举**
- 将 `acl_tc.h` 和 `acl_tm.h` 移到 `products/ccm/include/`
- ACL 只保留通用类型定义
- 更新文档和示例

**优先级 6：添加单元测试**
- 覆盖所有公开 API
- 测试边界条件和错误路径
- 性能基准测试

---

### 8.3 长期改进（3-6 月）

**优先级 7：支持动态配置**
- 支持运行时添加/删除配置项
- 支持配置热更新（无需重启）
- 添加配置版本管理

**优先级 8：优化内存占用**
- 使用哈希表支持稀疏 ID 空间
- 按需分配配置表（不预分配 1000 个条目）
- 支持配置压缩（如果配置表很大）

**优先级 9：增强可观测性**
- 添加详细的统计信息（查询次数、缓存命中率）
- 支持运行时配置导出（JSON/YAML）
- 集成 tracing 框架（如 LTTng）

---

## 9. 总结

### 9.1 核心问题

| 问题 | 严重性 | 影响 |
|------|--------|------|
| 遥测缓存职责越界 | 🔴 高 | 内存占用大、职责混乱、难以测试 |
| 业务枚举在 Core 层 | 🔴 高 | 违反架构原则、降低可复用性 |
| 依赖关系错误 | 🟡 中 | 不必要的编译依赖 |
| function_id 未验证 | 🟡 中 | 可能返回错误配置 |
| 缺少单元测试 | 🟡 中 | 代码质量无保证 |
| 文档与实现不一致 | 🟢 低 | 误导用户 |

### 9.2 架构评分

| 维度 | 评分 | 说明 |
|------|------|------|
| **职责单一性** | 3/10 | 混入了运行时数据管理 |
| **接口设计** | 6/10 | 基本合理，但有 bug 和设计缺陷 |
| **实现质量** | 5/10 | 代码简洁，但缺少验证和测试 |
| **性能** | 7/10 | O(1) 查询，但缓存锁粒度过粗 |
| **可维护性** | 4/10 | 职责混乱、缺少测试 |
| **可复用性** | 3/10 | 包含业务枚举，难以复用 |
| **文档质量** | 5/10 | 结构清晰，但与实现不一致 |

**总体评分**：4.7/10（需要重大改进）

### 9.3 关键建议

1. **立即行动**：
   - 修复依赖关系（移除 PDL 依赖）
   - 验证 function_id 连续性
   - 修复 `ACL_GetInvalidationMap()` bug

2. **近期规划**：
   - 将遥测缓存移到独立模块
   - 将业务枚举移到 products/ 目录
   - 添加单元测试

3. **长期目标**：
   - 重新设计 ACL 为纯配置层
   - 支持动态配置和热更新
   - 优化内存占用和并发性能

---

**分析完成日期**：2026-05-31  
**分析者**：Claude (Kiro AI)  
**文档版本**：1.0
