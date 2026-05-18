# EMS 编码规范

## 1. 总则

### 1.1 适用范围
本规范适用于 EMS 项目的所有 C 代码，包括 OSAL、HAL、PCL、PDL 和 Apps 层。

### 1.2 设计原则
- **航空航天级可靠性**：代码必须具备容错和自愈能力
- **分层隔离**：严格遵循 5 层架构，不允许跨层直接调用
- **模块独立**：各模块只依赖自己的配置，便于多人协作
- **硬件抽象**：上层应用与底层硬件解耦
- **零拷贝传输**：关键路径避免不必要的内存拷贝

### 1.3 强制要求
- 遵循 **Linux Kernel Coding Style**
- 遵循 **MISRA C** 规范（航空航天安全标准）
- 编译选项：`-Wall -Wextra -Werror`（所有警告视为错误）
- C 标准：**C99/C11**
- **系统调用封装**：所有系统调用必须在 OSAL 层封装，其他模块只允许使用 OSAL 对外提供的封装接口
- **日志接口规范**：非必要情况下，日志打印优先使用 `OSAL_INFO` 等带等级的接口，而不是使用 `OSAL_printf`

---

## 2. 文件组织

### 2.1 目录结构
每个模块遵循标准目录结构：
```
module/
├── include/              # 头文件
│   ├── api/             # 对外API（可选，仅顶层模块）
│   ├── internal/        # 内部公共头文件（可选）
│   ├── peripheral/      # 外设私有头文件（可选）
│   └── config/          # 模块配置（可选）
├── src/                 # 源代码
│   └── linux/          # Linux平台实现
└── platform/            # 平台配置（仅PCL）
```

### 2.2 头文件组织
**三层头文件架构**（参考 Linux Kernel）：
- **api/**：对外公共接口（供上层使用）
- **internal/**：内部公共头文件（模块内部共享）
- **peripheral/**：外设私有头文件（设备特定）

**示例**（PCL 层）：
```c
/* PDL层使用 */
#include "pcl_api.h"

/* PCL内部源文件使用 */
#include "internal/pcl.h"
```

### 2.3 文件头注释
每个文件必须包含标准头注释：
```c
/************************************************************************
 * 模块名称
 *
 * 功能：
 * - 功能点1
 * - 功能点2
 *
 * 职责：（可选）
 * - 职责描述
 *
 * 命名规范：（可选）
 * - PREFIX_*  - 接口说明
 ************************************************************************/
```

**实际示例**：
```c
/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCL_*       - 通用接口
 * - PCL_HW_*    - 硬件配置接口
 * - PCL_APP_*   - APP配置接口
 ************************************************************************/
```

---

## 3. 系统调用封装规范

### 3.1 封装原则
**所有系统调用必须在 OSAL 层封装，其他模块只允许使用 OSAL 对外提供的封装接口。**

**目的**：
- **平台隔离**：上层代码与操作系统解耦，便于跨平台移植
- **统一管理**：系统资源（文件、线程、内存）集中管理，便于调试和监控
- **错误处理**：统一的错误码和日志，提高代码可维护性
- **安全加固**：在 OSAL 层统一进行参数校验和安全检查

### 3.2 禁止的系统调用
**HAL、PCL、PDL、Apps 层禁止直接使用以下系统调用**：

| 类别 | 禁止使用 | 必须使用 OSAL 封装 |
|------|----------|-------------------|
| 线程 | `pthread_create`, `pthread_join`, `pthread_exit` | `OSAL_TaskCreate`, `OSAL_TaskDelete` |
| 互斥锁 | `pthread_mutex_lock`, `pthread_mutex_unlock` | `OSAL_MutexLock`, `OSAL_MutexUnlock` |
| 信号量 | `sem_wait`, `sem_post` | `OSAL_SemTake`, `OSAL_SemGive` |
| 队列 | `mq_send`, `mq_receive` | `OSAL_QueuePut`, `OSAL_QueueGet` |
| 文件 | `open`, `close`, `read`, `write` | `OSAL_FileOpen`, `OSAL_FileClose`, `OSAL_FileRead`, `OSAL_FileWrite` |
| 内存分配 | `malloc`, `free`, `mmap` | `OSAL_Malloc`, `OSAL_Free` |
| 内存操作 | `memset`, `memcpy`, `memmove`, `memcmp` | `OSAL_Memset`, `OSAL_Memcpy`, `OSAL_Memmove`, `OSAL_Memcmp` |
| 字符串操作 | `strlen`, `strcmp`, `strncmp`, `strcpy`, `strncpy`, `strcat`, `strncat` | `OSAL_Strlen`, `OSAL_Strcmp`, `OSAL_Strncmp`, `OSAL_Strcpy`, `OSAL_Strncpy`, `OSAL_Strcat`, `OSAL_Strncat` |
| 字符串格式化 | `sprintf`, `snprintf`, `sscanf` | `OSAL_Sprintf`, `OSAL_Snprintf`, `OSAL_Sscanf` |
| 时间 | `sleep`, `usleep`, `nanosleep` | `OSAL_TaskDelay` |
| 定时器 | `timer_create`, `timer_settime` | `OSAL_TimerCreate`, `OSAL_TimerSet` |
| 日志 | `printf`, `fprintf`, `syslog` | `OSAL_INFO`, `OSAL_ERROR` 等 |

### 3.3 正确示例

```c
/* ❌ 错误：HAL层直接使用系统调用 */
void hal_can_task(void *arg)
{
    pthread_mutex_lock(&mutex);  // ❌ 禁止
    sleep(1);                     // ❌ 禁止
    printf("CAN task\n");         // ❌ 禁止
    pthread_mutex_unlock(&mutex); // ❌ 禁止
}

/* ✅ 正确：使用OSAL封装 */
void hal_can_task(void *arg)
{
    OSAL_MutexLock(mutex_id);     // ✓
    OSAL_TaskDelay(1000);         // ✓
    OSAL_INFO("HAL_CAN", "CAN task running");  // ✓
    OSAL_MutexUnlock(mutex_id);   // ✓
}
```

```c
/* ❌ 错误：PDL层直接使用文件操作 */
int32 pdl_config_load(const char *path)
{
    int fd = open(path, O_RDONLY);  // ❌ 禁止
    if (fd < 0) {
        return -1;
    }
    read(fd, buffer, size);         // ❌ 禁止
    close(fd);                      // ❌ 禁止
    return 0;
}

/* ✅ 正确：使用OSAL文件接口 */
int32 pdl_config_load(const char *path)
{
    int32 fd;
    int32 ret;
    
    ret = OSAL_FileOpen(path, OS_READ_ONLY, &fd);  // ✓
    if (ret != OS_SUCCESS) {
        OSAL_ERROR("PDL", "Failed to open config file: %s", path);
        return OS_ERROR;
    }
    
    ret = OSAL_FileRead(fd, buffer, size);  // ✓
    OSAL_FileClose(fd);                     // ✓
    
    return ret;
}
```

### 3.4 例外情况
**仅以下情况允许直接使用系统调用**：

1. **OSAL 层内部实现**：OSAL 层封装系统调用时
2. **HAL 层硬件操作**：访问硬件寄存器、设备文件（如 `/dev/can0`）时，但必须使用 OSAL 的文件接口打开设备
3. **性能关键路径**：经过充分论证和团队评审后，可在注释中说明原因

```c
/* HAL层硬件操作示例 */
int32 HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
    int32 fd;
    int32 ret;
    
    /* 使用OSAL接口打开设备 */
    ret = OSAL_FileOpen(config->device, OS_READ_WRITE, &fd);  // ✓
    if (ret != OS_SUCCESS) {
        return OS_ERROR;
    }
    
    /* 硬件特定的ioctl操作（允许） */
    ret = ioctl(fd, SIOCGIFINDEX, &ifr);  // ✓ 硬件操作
    if (ret < 0) {
        OSAL_FileClose(fd);
        return OS_ERROR;
    }
    
    return OS_SUCCESS;
}
```

---

## 4. 命名规范

### 4.1 文件命名

#### 源代码文件命名

| 层级 | 格式 | 示例 |
|------|------|------|
| OSAL | `osal_<submodule>.c/h` | `osal_task.c`, `osal_queue.h` |
| HAL | `hal_<device>.c/h` | `hal_can.c`, `hal_serial.h` |
| PCL | `pcl_<module>.c/h` | `pcl_api.c`, `pcl_register.h` |
| PDL | `pdl_<peripheral>.c/h` | `pdl_satellite.c`, `pdl_bmc.h` |
| Apps | `apps_<application>.c/h` | `apps_can_gateway.c`, `apps_protocol_converter.h` |

#### 测试文件命名

| 格式 | 示例 | 说明 |
|------|------|------|
| `test_<module>_<submodule>.c` | `test_osal_task.c`, `test_hal_can.c` | 测试文件以 `test_` 开头 |

**测试模块声明**：
```c
TEST_MODULE_BEGIN(test_<module>_<submodule>)
    /* 测试用例 */
TEST_MODULE_END(test_<module>_<submodule>)
```

### 4.2 分层命名规范

#### 对外API命名（Public API）

| 层级 | 格式 | 示例 | 说明 |
|------|------|------|------|
| OSAL | `OSAL_<Module><Function>()` | `OSAL_TaskCreate()`, `OSAL_QueuePut()` | 大写前缀+大驼峰 |
| HAL | `HAL_<Device><Function>()` | `HAL_CANInit()`, `HAL_SerialOpen()` | 大写前缀+大驼峰 |
| PCL | `PCL_<Module><Function>()` | `PCL_Init()`, `PCL_Register()` | 大写前缀+大驼峰 |
| PDL | `PDL_<Peripheral><Function>()` | `PDL_SatelliteInit()`, `PDL_BMCPowerOn()` | 大写前缀+大驼峰 |
| Apps | `<Application><Function>()` | `CanGatewayInit()`, `ProtocolConverterInit()` | 大驼峰 |

#### 内部函数命名（Internal/Private）

| 层级 | 格式 | 示例 | 说明 |
|------|------|------|------|
| OSAL | `osal_<module>_<function>()` | `osal_task_find_by_id()` | 小写前缀+小写+下划线 |
| HAL | `hal_<device>_<function>()` | `hal_can_set_filter()` | 小写前缀+小写+下划线 |
| PCL | `pcl_<module>_<function>()` | `pcl_find_config()` | 小写前缀+小写+下划线 |
| PDL | `pdl_<peripheral>_<function>()` | `pdl_satellite_parse_frame()` | 小写前缀+小写+下划线 |
| Apps | `<application>_<function>()` | `can_gateway_process_rx()` | 小写+下划线 |

**重要规则：**
- ✅ 内部函数使用 `static` 限制作用域
- ✅ 使用小写+下划线格式（符合C标准和Linux内核风格）
- ❌ **禁止**使用下划线开头（`_xxx` 或 `__xxx`），这些被C标准保留

#### 测试用例命名（重要）

**格式**：`test_<module>_<function>_<scenario>`

**规则**：
- `test_` 前缀（固定）
- `<module>` 小写（如 osal, hal, pdl, apps）
- `<function>` 小写+下划线（如 file_open_close, task_create, can_init）
- `<scenario>` 小写+下划线（如 success, null_handle, invalid_param）
- **全部使用小写+下划线，无大写字母**

**示例**：

```c
/* OSAL层测试 */
TEST_MODULE_BEGIN(test_osal_task)
    TEST_CASE(test_osal_task_create_success)
    TEST_CASE(test_osal_task_create_null_handle)
    TEST_CASE(test_osal_task_delete_success)
    TEST_CASE(test_osal_task_delay_success)
TEST_MODULE_END(test_osal_task)

/* HAL层测试 */
TEST_MODULE_BEGIN(test_hal_can)
    TEST_CASE(test_hal_can_init_success)
    TEST_CASE(test_hal_can_init_null_handle)
    TEST_CASE(test_hal_can_send_success)
    TEST_CASE(test_hal_can_recv_timeout)
TEST_MODULE_END(test_hal_can)

/* PDL层测试 */
TEST_MODULE_BEGIN(test_pdl_satellite)
    TEST_CASE(test_pdl_satellite_init_success)
    TEST_CASE(test_pdl_satellite_init_null_config)
    TEST_CASE(test_pdl_satellite_send_command_success)
TEST_MODULE_END(test_pdl_satellite)

/* Apps层测试 */
TEST_MODULE_BEGIN(test_apps_can_gateway)
    TEST_CASE(test_apps_can_gateway_init_success)
    TEST_CASE(test_apps_can_gateway_process_message_success)
TEST_MODULE_END(test_apps_can_gateway)
```

**反例（禁止使用）**：

```c
/* ❌ 错误1：使用大写字母 */
TEST_CASE(test_HAL_CAN_Init_Success)        // ❌ 有大写字母
TEST_CASE(test_OSAL_FileOpen_Success)       // ❌ 有大写字母
TEST_CASE(TEST_HAL_CANInit_Success)         // ❌ 有大写字母

/* ❌ 错误2：使用驼峰命名 */
TEST_CASE(test_hal_canInit_success)         // ❌ 使用驼峰canInit
TEST_CASE(test_osal_taskCreate_success)     // ❌ 使用驼峰taskCreate

/* ❌ 错误3：缺少下划线分隔 */
TEST_CASE(test_halcaninit_success)          // ❌ 缺少下划线

/* ✅ 正确的命名 */
TEST_CASE(test_hal_can_init_success)         // ✓ 全部小写+下划线
TEST_CASE(test_osal_file_open_success)       // ✓ 全部小写+下划线
TEST_CASE(test_pdl_satellite_init_success)   // ✓ 全部小写+下划线
TEST_CASE(test_apps_can_gateway_init_success) // ✓ 全部小写+下划线
```

### 4.3 函数命名详解

#### 对外API函数（Public API）

**格式**：`<MODULE>_<Module><Function>()`

**示例**：
```c
/* OSAL层 - 对外API */
int32 OSAL_TaskCreate(osal_id_t *task_id, const char *name, ...);
int32 OSAL_TaskDelete(osal_id_t task_id);
int32 OSAL_QueueCreate(osal_id_t *queue_id, uint32 depth, ...);
int32 OSAL_QueuePut(osal_id_t queue_id, const void *data, uint32 size, int32 timeout);
int32 OSAL_MutexLock(osal_id_t mutex_id);

/* HAL层 - 对外API */
int32 HAL_CANInit(const char *interface);
int32 HAL_CANSend(int32 fd, const can_frame_t *frame);
int32 HAL_SerialOpen(const char *device, const hal_serial_config_t *config);

/* PCL层 - 对外API */
int32 PCL_Init(void);
int32 PCL_Register(const pcl_board_config_t *config);
const pcl_mcu_cfg_t* PCL_HWFindMCU(const char *name);

/* PDL层 - 对外API */
int32 PDL_SatelliteInit(const satellite_config_t *config);
int32 PDL_SatelliteSendCommand(uint8 cmd_type, const uint8 *data);
int32 PDL_BMCPowerOn(uint32 slot_id);
int32 PDL_BMCGetStatus(uint32 slot_id, uint8 *status);
```

#### 内部函数（Internal/Private）

**格式**：`<module>_<submodule>_<function>()`

**示例**：
```c
/* OSAL层 - 内部函数 */
static int32 osal_task_table_init(void);
static osal_id_t osal_task_find_free_slot(void);
static osal_task_record_t* osal_task_find_by_id(osal_id_t task_id);
static void osal_task_cleanup(osal_id_t task_id);

static int32 osal_queue_acquire(osal_queue_record_t *queue);
static void osal_queue_release(osal_queue_record_t *queue);

/* HAL层 - 内部函数 */
static int32 hal_can_set_filter(int fd, uint32 filter_id);
static void hal_can_update_stats(hal_can_handle_t *handle);
static int32 hal_serial_configure_termios(int fd, const hal_serial_config_t *config);

/* PDL层 - 内部函数 */
static int32 pdl_satellite_parse_frame(const can_frame_t *frame);
static void pdl_satellite_update_stats(void);
static int32 pdl_bmc_send_ipmi_command(uint8 cmd, const uint8 *data, uint32 len);
```

**命名规则：**
- 对外API：大写前缀 + 大驼峰（`OSAL_TaskCreate`）
- 内部函数：小写前缀 + 小写+下划线（`osal_task_find_by_id`）
- 内部函数必须声明为 `static`

### 4.4 变量命名

#### 全局变量
- **格式**：`g_<module>_<name>`
- **示例**：
```c
/* OSAL层全局变量 */
static osal_task_record_t g_task_table[OS_MAX_TASKS];
static pthread_mutex_t g_task_table_mutex;
static bool g_osal_initialized = false;

/* PCL层全局变量 */
static pcl_registry_t g_registry = {0};
static bool g_pcl_initialized = false;
```

#### 静态变量
- **格式**：`s_<name>` 或直接使用 `static` + 描述性名称
- **示例**：
```c
/* 文件作用域静态变量 */
static uint32 s_init_count = 0;
static bool s_debug_enabled = false;

/* 或者直接使用描述性名称 */
static uint32 init_count = 0;
static bool debug_enabled = false;
```

#### 局部变量
- **格式**：小写+下划线
- **示例**：
```c
int32 ret;
uint32 success_count = 0;
osal_id_t task_id;
const pcl_board_config_t *config = NULL;
osal_task_record_t *record = NULL;
```

#### 常量
- **格式**：全大写+下划线
- **示例**：
```c
#define MAX_BOARD_CONFIGS 32
#define OS_MAX_TASKS 64
#define CONFIG_COUNT (sizeof(g_all_configs) / sizeof(g_all_configs[0]))
#define DEFAULT_TIMEOUT_MS 1000
```

### 4.5 类型命名
- **结构体**：`<模块>_<名称>_t`，如 `pcl_board_config_t`
- **枚举**：`<模块>_<名称>_e`，如 `pcl_device_type_e`
- **枚举值**：全大写，如 `PCL_DEV_MCU`
- **函数指针**：`<模块>_<名称>_t`，如 `satellite_cmd_callback_t`

**示例**：
```c
/* 结构体 */
typedef struct {
    const pcl_board_config_t *configs[MAX_BOARD_CONFIGS];
    uint32 count;
    const pcl_board_config_t *current;
} pcl_registry_t;

/* 枚举 */
typedef enum {
    PCL_DEV_MCU = 0,
    PCL_DEV_BMC,
    PCL_DEV_SATELLITE,
    PCL_DEV_SENSOR,
    PCL_DEV_STORAGE
} pcl_device_type_e;

/* 函数指针 */
typedef void (*satellite_cmd_callback_t)(uint8 cmd_type, const uint8 *data, void *user_data);
```

### 4.6 宏命名
- **配置宏**：全大写，如 `TASK_STACK_SIZE_DEFAULT`
- **功能宏**：全大写，如 `LOG_INFO`, `TEST_ASSERT_EQUAL`

---

## 5. 数据类型规范

### 5.1 强制要求

**本项目严格禁止使用 C 语言原生数据类型，必须使用 OSAL 层封装的类型。**

**目的**：
- **平台无关性**：确保代码在不同平台（32位/64位、不同架构）上行为一致
- **类型安全**：固定宽度类型避免整数溢出和类型转换问题
- **语义清晰**：类型名称明确表达数据用途
- **可移植性**：便于移植到 RTOS 和嵌入式平台

### 5.2 禁止使用的原生类型

**以下原生类型在所有层（OSAL/HAL/PCL/PDL/Apps）中严格禁止使用**：

| 禁止使用 | 原因 | 必须使用 OSAL 类型 |
|---------|------|-------------------|
| `char` | 允许用于字符串，禁止用于数值计算 | 字符串使用 `char`，字节数据使用 `int8`/`uint8` |
| `int` | 宽度平台相关（16/32位） | `int32` 或 `int16` |
| `unsigned int` | 宽度平台相关 | `uint32` 或 `uint16` |
| `short` | 宽度不明确 | `int16` |
| `unsigned short` | 宽度不明确 | `uint16` |
| `long` | 宽度平台相关（32/64位） | `int32` 或 `int64` |
| `unsigned long` | 宽度平台相关 | `uint32` 或 `uint64` |
| `long long` | 宽度不明确 | `int64` |
| `unsigned long long` | 宽度不明确 | `uint64` |
| `size_t` | 宽度平台相关 | `osal_size_t` |
| `ssize_t` | 宽度平台相关 | `osal_ssize_t` |
| `intptr_t` | 宽度平台相关 | `osal_size_t` 或 `uintptr_t`（仅指针转换） |
| `ptrdiff_t` | 宽度平台相关 | `osal_ssize_t` |

### 5.3 OSAL 类型系统

**所有类型定义在 `core/osal/include/osal_types.h` 中。**

#### 5.3.1 固定宽度整数类型

```c
/* 有符号整数 */
int8      // 8位有符号整数  (-128 ~ 127)
int16     // 16位有符号整数 (-32768 ~ 32767)
int32     // 32位有符号整数 (-2^31 ~ 2^31-1)
int64     // 64位有符号整数 (-2^63 ~ 2^63-1)

/* 无符号整数 */
uint8     // 8位无符号整数  (0 ~ 255)
uint16    // 16位无符号整数 (0 ~ 65535)
uint32    // 32位无符号整数 (0 ~ 2^32-1)
uint64    // 64位无符号整数 (0 ~ 2^64-1)
```

**使用场景**：
- `uint8`/`int8`：字节数据、标志位、小范围计数
- `uint16`/`int16`：端口号、小数值
- `uint32`/`int32`：通用整数、ID、计数器、返回值
- `uint64`/`int64`：时间戳、大数值

#### 5.3.2 字符串类型

```c
char      // 字符串类型（与标准 C 库兼容）
```

**使用场景**：
- 设备名称：`char device_name[64];`
- 日志消息：`char log_message[256];`
- 字符串指针：`const char *interface;`
- 单个字符：`char parity = 'N';`

**重要**：`char` 用于文本数据，`uint8` 用于二进制字节数据。

#### 5.3.3 布尔类型

```c
bool      // 布尔类型（true/false）
```

**使用场景**：
- 标志位：`bool is_initialized = false;`
- 条件判断：`bool is_valid(const config_t *cfg);`

#### 5.3.4 平台相关大小类型

```c
osal_size_t    // 无符号大小类型（32位平台=uint32，64位平台=uint64）
osal_ssize_t   // 有符号大小类型（32位平台=int32，64位平台=int64）
```

**使用场景**：
- 内存大小：`osal_size_t buffer_size;`
- 数组索引：`osal_size_t index;`
- 读写字节数：`osal_ssize_t bytes_read;`

#### 5.3.5 OSAL 对象 ID 类型

```c
osal_id_t      // OSAL 对象 ID（底层是 uint32）
```

**使用场景**：
- 任务 ID：`osal_id_t task_id;`
- 队列 ID：`osal_id_t queue_id;`
- 互斥锁 ID：`osal_id_t mutex_id;`

### 5.4 正确示例

```c
/* ✅ 正确：使用 OSAL 类型 */
int32 HAL_CAN_Init(const char *device, hal_can_handle_t *handle)
{
    uint32 filter_id = 0x100;
    uint8 dlc = 8;
    int32 ret;
    osal_size_t buffer_size = 1024;
    bool is_initialized = false;
    
    /* ... */
    
    return OSAL_SUCCESS;
}

/* ✅ 正确：字符串使用 char */
void log_device_info(const char *device_name)
{
    char buffer[256];
    OSAL_Snprintf(buffer, sizeof(buffer), "Device: %s", device_name);
    OSAL_INFO("HAL", "%s", buffer);
}

/* ✅ 正确：二进制数据使用 uint8 */
int32 parse_can_frame(const uint8 *data, uint8 length)
{
    uint16 voltage = (data[0] << 8) | data[1];
    uint32 timestamp = (data[2] << 24) | (data[3] << 16) | 
                       (data[4] << 8) | data[5];
    return OSAL_SUCCESS;
}
```

### 5.5 错误示例

```c
/* ❌ 错误：使用原生类型 */
int HAL_CAN_Init(const char *device, void *handle)  // ❌ int, char
{
    unsigned int filter_id = 0x100;  // ❌ unsigned int
    unsigned char dlc = 8;           // ❌ unsigned char
    int ret;                         // ❌ int
    size_t buffer_size = 1024;       // ❌ size_t
    
    /* ... */
    
    return 0;
}

/* ❌ 错误：混用原生类型和 OSAL 类型 */
int32 process_data(const char *name, int count)  // ❌ char, int
{
    uint32 i;
    for (i = 0; i < count; i++) {  // ❌ count 是 int 类型
        /* ... */
    }
    return OSAL_SUCCESS;
}
```

### 5.6 类型转换规则

#### 5.6.1 指针与整数转换

```c
/* ✅ 正确：使用 uintptr_t 进行指针转换 */
void *ptr = get_pointer();
uintptr_t addr = (uintptr_t)ptr;  // ✓ 使用 uintptr_t

/* ❌ 错误：使用固定宽度类型转换指针 */
uint32 addr = (uint32)ptr;  // ❌ 在 64 位平台会截断
```

#### 5.6.2 有符号与无符号转换

```c
/* ✅ 正确：显式转换并检查范围 */
int32 signed_value = -100;
uint32 unsigned_value;

if (signed_value >= 0) {
    unsigned_value = (uint32)signed_value;  // ✓ 检查后转换
} else {
    OSAL_ERROR("MODULE", "Cannot convert negative value to unsigned");
    return OSAL_ERR_INVALID_PARAM;
}

/* ❌ 错误：隐式转换 */
int32 signed_value = -100;
uint32 unsigned_value = signed_value;  // ❌ 隐式转换，结果错误
```

### 5.7 例外情况

**仅以下情况允许使用原生类型**：

1. **OSAL 层内部实现**：封装系统调用时，与系统 API 交互
   ```c
   /* osal_file.c - OSAL 层内部实现 */
   int32 OSAL_open(const char *pathname, int32 flags, int32 mode)
   {
       int fd = open(pathname, flags, mode);  // ✓ 系统调用返回 int
       if (fd < 0) {
           return OSAL_ERR_FILE;
       }
       return fd;
   }
   ```

2. **与第三方库交互**：调用外部库 API 时
   ```c
   /* 调用第三方库（如 libmodbus） */
   int32 read_modbus_register(uint16 addr)
   {
       int rc = modbus_read_registers(ctx, addr, 1, &value);  // ✓ 库 API 返回 int
       if (rc == -1) {
           return OSAL_ERR_GENERIC;
       }
       return OSAL_SUCCESS;
   }
   ```

3. **必须在注释中说明原因**：
   ```c
   /* 与 libusb 交互，必须使用 int 类型 */
   int rc = libusb_init(&ctx);  // ✓ 有注释说明
   ```

### 5.8 检查清单

代码提交前检查：
- [ ] 无 `char`（除了 OSAL 层系统调用封装）
- [ ] 无 `int`/`unsigned int`（除了第三方库交互）
- [ ] 无 `short`/`long`/`long long`
- [ ] 无 `size_t`/`ssize_t`（使用 `osal_size_t`/`osal_ssize_t`）
- [ ] 二进制数据使用 `uint8`
- [ ] 所有整数使用固定宽度类型（`int8`/`int16`/`int32`/`int64`）
- [ ] 指针转换使用 `uintptr_t`
- [ ] 有符号/无符号转换有范围检查

---

## 6. 代码风格

### 6.1 缩进与空格
- **缩进**：4 个空格（禁止使用 Tab）
- **行宽**：建议不超过 100 字符
- **函数参数**：过长时换行对齐

**示例**：
```c
/* 正确 */
int32 PCL_Register(const pcl_board_config_t *config)
{
    if (config == NULL) {
        LOG_ERROR("XCONFIG", "Invalid config pointer");
        return OS_ERROR;
    }
    
    return OS_SUCCESS;
}

/* 参数换行 */
const pcl_satellite_cfg_t* PCL_HW_FindSatellite(
    const pcl_board_config_t *board,
    const char *name)
{
    /* ... */
}
```

### 6.2 大括号风格
- **函数**：左大括号另起一行
- **控制语句**：左大括号跟随语句（K&R 风格）

**示例**：
```c
/* 函数 */
int32 PCL_Init(void)
{
    /* ... */
}

/* 控制语句 */
if (config == NULL) {
    return OS_ERROR;
}

for (uint32 i = 0; i < count; i++) {
    /* ... */
}

while (!OSAL_TaskShouldShutdown()) {
    /* ... */
}
```

### 4.3 注释规范
**原则**：默认不写注释，仅在以下情况添加：
- 非显而易见的 WHY（隐藏约束、微妙不变量、特定 Bug 的 Workaround）
- 硬件时序和寄存器操作（必须引用芯片手册章节号）
- 复杂算法的关键步骤

**禁止**：
- 解释 WHAT（代码本身已说明）
- 引用当前任务/调用者（"used by X", "added for Y"）
- 冗长的多行注释块

**示例**：
```c
/* 正确：解释WHY */
/* 优先级：环境变量 > 编译选项 > 默认配置 */
platform = getenv("PCL_PLATFORM");

/* 检查重复注册（避免配置冲突） */
for (uint32 i = 0; i < g_registry.count; i++) {
    /* ... */
}

/* 错误：解释WHAT */
/* 初始化注册表 */  // ❌ 代码已经说明
memset(&g_registry, 0, sizeof(g_registry));

/* 错误：引用任务 */
/* 这个函数用于CAN网关 */  // ❌ 属于PR描述，不属于代码
```

### 4.4 函数文档注释
**对外 API** 必须使用 Doxygen 风格注释：
```c
/**
 * @brief 注册硬件配置
 *
 * @param config 配置指针
 * @return OS_SUCCESS 成功
 * @return OS_ERROR 失败
 */
int32 PCL_Register(const pcl_board_config_t *config);
```

**内部函数** 可省略文档注释（函数名已足够清晰）。

### 5.5 条件判断规范（Yoda Conditions）

**强制要求**：所有条件判断必须将常量放在比较运算符左侧（Yoda条件风格）

**目的**：
- **防止赋值错误**：避免将 `==` 误写为 `=` 导致的赋值bug
- **编译器保护**：`if (NULL = ptr)` 会产生编译错误，而 `if (ptr = NULL)` 可能通过编译
- **代码一致性**：统一的代码风格，提高可读性
- **航空航天标准**：符合 MISRA C 和 DO-178C 防御性编程要求

**规范**：
```c
/* ✅ 正确：常量在左侧 */
if (NULL == ptr) {
    return OS_ERROR;
}

if (OSAL_SUCCESS == ret) {
    LOG_INFO("MODULE", "Operation succeeded");
}

if (0 == count) {
    return OS_ERROR;
}

if (true == flag) {
    do_something();
}

if (ETIMEDOUT == errno) {
    handle_timeout();
}

/* ❌ 错误：变量在左侧 */
if (ptr == NULL) {          // ❌ 应改为 if (NULL == ptr)
    return OS_ERROR;
}

if (ret == OSAL_SUCCESS) {  // ❌ 应改为 if (OSAL_SUCCESS == ret)
    return OS_SUCCESS;
}

if (count == 0) {           // ❌ 应改为 if (0 == count)
    return OS_ERROR;
}
```

**适用范围**：
- NULL 指针检查：`if (NULL == ptr)`
- 枚举值比较：`if (OSAL_SUCCESS == ret)`
- 宏定义常量比较：`if (OS_MAX_TASKS == count)`
- 数字字面量比较：`if (0 == size)`, `if (1 == flag)`
- 布尔值比较：`if (true == enabled)`, `if (false == initialized)`
- 错误码比较：`if (ETIMEDOUT == errno)`, `if (EAGAIN == err)`

**注意事项**：
- **仅适用于 `==` 和 `!=` 运算符**
- **不适用于 `<`, `>`, `<=`, `>=` 运算符**（保持自然顺序）
- **两个变量比较时保持自然顺序**
- **字符串比较函数返回值**：`if (0 == strcmp(a, b))` 或 `if (0 == OSAL_Strcmp(a, b))`

**完整示例**：
```c
/* ✅ 正确：等值比较，常量在左 */
if (NULL == ptr) {
    return OSAL_ERR_INVALID_POINTER;
}

if (OSAL_SUCCESS == OSAL_MutexCreate(&mutex_id, "mutex", 0)) {
    LOG_INFO("MODULE", "Mutex created");
}

if (0 == g_initialized) {
    initialize_module();
}

if (0 == strcmp(name, "default")) {
    use_default_config();
}

/* ✅ 正确：范围比较保持自然顺序 */
if (count > 0) {
    process_data();
}

if (size < MAX_SIZE) {
    allocate_buffer();
}

if (timeout >= 1000) {
    LOG_WARN("MODULE", "Long timeout: %d ms", timeout);
}

/* ✅ 正确：两个变量比较保持自然顺序 */
if (current_size == max_size) {
    resize_buffer();
}

if (tx_count != rx_count) {
    LOG_ERROR("MODULE", "Count mismatch: tx=%u, rx=%u", tx_count, rx_count);
}

/* ✅ 正确：复合条件 */
if (NULL != config && 0 == config->count) {
    return OSAL_ERR_INVALID_SIZE;
}

if (OSAL_SUCCESS == ret && 0 < bytes_read) {
    process_buffer(buffer, bytes_read);
}
```

**实际代码示例**：
```c
/* OSAL 层 - 队列操作 */
int32_t OSAL_QueueCreate(osal_id_t *queue_id, const char *queue_name, ...)
{
    if (NULL == queue_id)
        return OSAL_ERR_INVALID_POINTER;
    
    if (NULL == queue_name || 0 == strlen(queue_name))
        return OSAL_ERR_NAME_TOO_LONG;
    
    if (0 == queue_depth || 0 == data_size)
        return OSAL_ERR_QUEUE_INVALID_SIZE;
    
    /* ... */
}

/* HAL 层 - CAN 驱动 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, can_frame_t *frame, int32_t timeout)
{
    if (NULL == handle || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;
    
    ret = OSAL_read(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret < 0) {
        int32_t err = OSAL_GetErrno();
        if (OSAL_EAGAIN == err || OSAL_EWOULDBLOCK == err)
            return OSAL_ERR_TIMEOUT;
        return OSAL_ERR_GENERIC;
    }
    
    /* ... */
}

/* PDL 层 - 卫星服务 */
int32_t PDL_SatelliteInit(const satellite_service_config_t *config, ...)
{
    if (NULL == config || NULL == handle)
        return OSAL_ERR_GENERIC;
    
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex, "sat_mutex", 0)) {
        LOG_ERROR("SAT", "Failed to create mutex");
        return OSAL_ERR_GENERIC;
    }
    
    /* ... */
}
```

**编译器检查**：
现代编译器（GCC/Clang）会对 `if (ptr = NULL)` 产生警告，但 Yoda 条件提供了额外的保护层：
```c
/* 编译错误：常量不能作为左值 */
if (NULL = ptr) {  // ✅ 编译器报错：cannot assign to NULL
    /* ... */
}

/* 可能通过编译（取决于警告级别） */
if (ptr = NULL) {  // ⚠️ 可能只是警告，不是错误
    /* ... */
}
```

---

## 7. 错误处理

### 4.1 返回值规范
- **成功**：`OS_SUCCESS` (0)
- **失败**：`OS_ERROR` (-1) 或具体错误码
- **所有函数返回值必须检查**

**示例**：
```c
/* 正确 */
int32 ret = PCL_Register(config);
if (ret != OS_SUCCESS) {
    LOG_ERROR("XCONFIG", "Failed to register config");
    return OS_ERROR;
}

/* 错误：未检查返回值 */
PCL_Register(config);  // ❌
```

### 4.2 参数校验
**对外 API** 必须校验所有参数：
```c
int32 PCL_Register(const pcl_board_config_t *config)
{
    if (!g_initialized) {
        LOG_ERROR("XCONFIG", "Library not initialized");
        return OS_ERROR;
    }

    if (config == NULL) {
        LOG_ERROR("XCONFIG", "Invalid config pointer");
        return OS_ERROR;
    }

    /* ... */
}
```

**内部函数** 可使用断言（Debug 模式）：
```c
static void internal_process(const data_t *data)
{
    assert(data != NULL);  /* Debug模式检查 */
    /* ... */
}
```

### 4.3 边界检查
数组访问必须检查边界：
```c
/* 正确 */
if (id >= board->mcu_count) {
    return NULL;
}
return board->mcus[id];

/* 错误：未检查边界 */
return board->mcus[id];  // ❌ 可能越界
```

---

## 8. 内存管理

### 4.1 禁止事项
- **关键路径禁止动态分配**：中断处理、DMA 传输禁止使用 `malloc`/`kmalloc`
- **必须检查返回值**：所有 `malloc` 必须检查是否为 NULL
- **禁止内存泄漏**：每个 `malloc` 必须有对应的 `free`

### 4.2 推荐做法
- **预分配内存池**：关键路径使用静态缓冲区或内存池
- **栈上分配**：小对象优先使用栈（注意栈大小限制）

**示例**：
```c
/* 正确：预分配 */
static uint8 g_rx_buffer[1024];

/* 正确：栈上分配 */
void process_message(void)
{
    can_frame_t frame;  /* 小对象，栈上分配 */
    /* ... */
}

/* 正确：检查malloc返回值 */
void *buffer = malloc(size);
if (buffer == NULL) {
    LOG_ERROR("MODULE", "Memory allocation failed");
    return OS_ERROR;
}
/* 使用buffer */
free(buffer);
```

---

## 9. 日志规范

### 7.1 日志接口
**禁止**直接使用 `printf`/`fprintf`，**必须**使用 OSAL 日志接口：

```c
/* 带等级的日志接口（推荐） */
OSAL_INFO("MODULE", "format", ...);
OSAL_DEBUG("MODULE", "format", ...);
OSAL_WARN("MODULE", "format", ...);
OSAL_ERROR("MODULE", "format", ...);
OSAL_FATAL("MODULE", "format", ...);

/* 简单输出（仅在必要时使用） */
OSAL_printf("message\n");

/* 兼容宏（映射到OSAL接口） */
LOG_DEBUG("MODULE", "format", ...);
LOG_INFO("MODULE", "format", ...);
LOG_WARN("MODULE", "format", ...);
LOG_ERROR("MODULE", "format", ...);
LOG_FATAL("MODULE", "format", ...);
```

**使用原则**：
- **优先使用带等级的接口**：`OSAL_INFO`、`OSAL_ERROR` 等，便于日志过滤和分级管理
- **避免使用 `OSAL_printf`**：仅在简单调试或特殊场景下使用
- **禁止使用 `OS_printf`**：这是内部接口，应使用 `OSAL_printf` 或带等级的接口

### 7.2 日志级别
| 级别 | 用途 | 示例 |
|------|------|------|
| DEBUG | 调试信息 | 函数入口/出口、变量值 |
| INFO | 正常信息 | 初始化成功、配置加载 |
| WARN | 警告信息 | 配置重复、非致命错误 |
| ERROR | 错误信息 | 操作失败、参数无效 |
| FATAL | 致命错误 | 系统崩溃、无法恢复 |

### 7.3 日志示例
```c
/* 推荐：使用带等级的接口 */
OSAL_INFO("XCONFIG", "Hardware configuration library initialized");
OSAL_ERROR("XCONFIG", "Failed to register config[%d]: %s/%s/%s",
           i, config->platform, config->product, config->version);
OSAL_WARN("XCONFIG", "Config already registered: %s/%s/%s",
          config->platform, config->product, config->version);

/* 或使用兼容宏 */
LOG_INFO("XCONFIG", "Hardware configuration library initialized");
LOG_ERROR("XCONFIG", "Failed to register config[%d]: %s/%s/%s",
          i, config->platform, config->product, config->version);

/* 不推荐：使用简单输出 */
OSAL_printf("Hardware configuration library initialized\n");  // ❌ 缺少模块名和等级
```

---

## 10. 任务编程

### 8.1 任务入口函数
**标准模板**：
```c
static void task_entry(void *arg)
{
    osal_id_t task_id = OS_TaskGetId();
    
    LOG_INFO("MODULE", "Task started");
    
    while (!OS_TaskShouldShutdown()) {
        /* 执行任务逻辑 */
        do_work();
        
        /* 延时 */
        OS_TaskDelay(100);
    }
    
    /* 清理资源 */
    cleanup();
    
    LOG_INFO("MODULE", "Task stopped");
}
```

### 8.2 优雅退出
**禁止**：
- 使用 `while(1)` 无限循环
- 使用 `pthread_cancel` 强制取消

**必须**：
- 检查 `OS_TaskShouldShutdown()`
- 退出前清理资源

**实际示例**：
```c
static void heartbeat_task(void *arg)
{
    satellite_service_context_t *ctx = (satellite_service_context_t *)arg;

    LOG_INFO("SAT", "Heartbeat task started");

    while (!OSAL_TaskShouldShutdown()) {
        /* 发送心跳 */
        if (satellite_can_send_heartbeat(ctx->can_handle, STATUS_OK) == OS_SUCCESS) {
            ctx->tx_count++;
        } else {
            ctx->error_count++;
        }

        /* 延迟 */
        OSAL_TaskDelay(ctx->config.heartbeat_interval_ms);
    }

    LOG_INFO("SAT", "Heartbeat task stopped");
}
```

---

## 11. 配置管理

### 9.1 配置文件位置
**配置下沉原则**：每个模块的配置文件放在模块内部的 `include/config/` 目录。

**示例**：
```
osal/include/config/task_config.h      # OSAL任务配置
hal/include/config/can_config.h        # HAL CAN配置
apps/can_gateway/include/config/app_config.h  # 应用配置
```

### 9.2 配置文件格式
```c
/************************************************************************
 * 模块配置
 ************************************************************************/

#ifndef MODULE_CONFIG_H
#define MODULE_CONFIG_H

/* 配置项 */
#define CONFIG_ITEM_1  100
#define CONFIG_ITEM_2  "value"

#endif /* MODULE_CONFIG_H */
```

### 9.3 依赖隔离
- 各模块**只依赖自己的配置**
- **禁止**引用其他模块的配置文件

---

## 12. 安全编程

### 10.1 禁止事项
- **命令注入**：禁止直接拼接用户输入到 `system()`
- **缓冲区溢出**：使用 `strncpy`/`snprintf` 而非 `strcpy`/`sprintf`
- **SQL 注入**：使用参数化查询
- **XSS**：Web 接口必须转义输出

### 10.2 字符串操作
```c
/* 正确 */
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';

snprintf(buffer, sizeof(buffer), "format %s", str);

/* 错误 */
strcpy(dest, src);  // ❌ 可能溢出
sprintf(buffer, "format %s", str);  // ❌ 可能溢出
```

### 10.3 硬件操作
- **必须假设硬件会失败**：Flash 会坏、FPGA 会复位、链路会断开
- **必须包含看门狗和复位恢复逻辑**
- **寄存器操作必须引用芯片手册章节号**

---

## 13. 测试规范

### 11.1 测试框架
使用项目统一测试框架：
```c
#include "unittest_framework.h"

TEST_MODULE_BEGIN(test_module_name)

TEST_CASE(test_case_name)
{
    /* 准备 */
    int32 ret;
    
    /* 执行 */
    ret = function_under_test();
    
    /* 验证 */
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
}

TEST_MODULE_END()
```

### 11.2 完整测试文件示例

```c
// 文件：test_osal_file.c

#include "unittest_framework.h"
#include "osal_file.h"

TEST_MODULE_BEGIN(test_osal_file)

TEST_CASE(test_osal_file_open_close_success)
{
    int32 fd;
    int32 ret;
    
    /* 打开文件 */
    ret = osal_file_open("/tmp/test.txt", O_RDWR | O_CREAT, &fd);
    TEST_ASSERT_EQUAL(0, ret);
    
    /* 关闭文件 */
    ret = osal_file_close(fd);
    TEST_ASSERT_EQUAL(0, ret);
}

TEST_CASE(test_osal_file_open_invalid_path)
{
    int32 fd;
    int32 ret;
    
    /* 打开无效路径 */
    ret = osal_file_open("/invalid/path/test.txt", O_RDWR, &fd);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

TEST_CASE(test_osal_file_read_write_success)
{
    int32 fd;
    int32 ret;
    char write_buf[] = "test data";
    char read_buf[32] = {0};
    
    /* 打开文件 */
    ret = osal_file_open("/tmp/test.txt", O_RDWR | O_CREAT, &fd);
    TEST_ASSERT_EQUAL(0, ret);
    
    /* 写入数据 */
    ret = osal_file_write(fd, write_buf, sizeof(write_buf));
    TEST_ASSERT_EQUAL(sizeof(write_buf), ret);
    
    /* 读取数据 */
    osal_file_lseek(fd, 0, SEEK_SET);
    ret = osal_file_read(fd, read_buf, sizeof(write_buf));
    TEST_ASSERT_EQUAL(sizeof(write_buf), ret);
    TEST_ASSERT_STRING_EQUAL(write_buf, read_buf);
    
    /* 关闭文件 */
    osal_file_close(fd);
}

TEST_MODULE_END(test_osal_file)
```

### 11.3 测试命名检查清单

#### 文件命名
- [ ] 文件名使用小写+下划线：`module_submodule.c`
- [ ] 测试文件以`test_`开头：`test_module_submodule.c`

#### 函数命名
- [ ] 函数名使用小写+下划线：`module_submodule_function()`
- [ ] 无大写字母，无驼峰命名

#### 测试用例命名
- [ ] 以`test_`开头（固定前缀）
- [ ] 模块名小写：`test_osal_`, `test_hal_`, `test_pdl_`, `test_apps_`
- [ ] 函数名小写+下划线：`file_open_close`, `task_create`, `can_init`
- [ ] 场景名小写+下划线：`success`, `null_handle`, `invalid_param`
- [ ] **全部小写，无大写字母，无驼峰命名**

### 11.4 测试覆盖
- **新功能必须编写单元测试**
- **Bug 修复必须添加回归测试**
- **关键路径必须覆盖边界条件**

---

## 14. 版本控制

### 12.1 提交信息格式
```
<type>: <subject>

<body>

<footer>
```

**Type**：
- `feat`: 新功能
- `fix`: Bug 修复
- `refactor`: 重构
- `docs`: 文档
- `test`: 测试
- `chore`: 构建/工具

**示例**：
```
feat: 添加PCL硬件配置库

- 实现板级配置注册和查询
- 支持MCU/BMC/卫星平台外设配置
- 添加APP设备映射功能

Closes #123
```

### 12.2 分支管理
- `master`: 主分支（稳定版本）
- `develop`: 开发分支
- `feature/xxx`: 功能分支
- `bugfix/xxx`: Bug 修复分支

---

## 15. 性能优化

### 13.1 关键路径优化
- **零拷贝传输**：避免不必要的 `memcpy`
- **预分配内存**：避免运行时 `malloc`
- **缓存友好**：数据结构按缓存行对齐

### 13.2 性能指标
- CAN 消息延迟：< 10ms
- 命令处理时间：< 100ms
- 内存占用：< 128MB
- CPU 占用（空闲）：< 5%

---

## 16. 文档规范

### 14.1 必需文档
- `README.md`：模块概述、使用方法
- `CLAUDE.md`：项目指导文档（供 AI 助手使用）
- `CODING_STANDARDS.md`：本文档

### 14.2 代码即文档
- **优先使用清晰的命名**而非注释
- **函数名应说明意图**：`PCL_HW_FindMCU` 优于 `find_mcu`
- **避免缩写**：`config` 优于 `cfg`（除非是行业标准）

---

## 17. 检查清单

### 15.1 代码提交前检查
- [ ] 编译无警告（`-Wall -Wextra -Werror`）
- [ ] 所有测试通过
- [ ] 添加了必要的单元测试
- [ ] 更新了相关文档
- [ ] 代码符合本规范
- [ ] 无内存泄漏（Valgrind 检查）
- [ ] 日志使用 OSAL 接口（无 `printf`）
- [ ] 所有返回值已检查
- [ ] 参数已校验（对外 API）

### 15.2 Code Review 检查
- [ ] 是否遵循分层架构
- [ ] 是否有跨层直接调用
- [ ] 错误处理是否完整
- [ ] 是否有潜在的内存泄漏
- [ ] 是否有缓冲区溢出风险
- [ ] 任务是否能优雅退出
- [ ] 是否有不必要的复杂度

---

## 18. 参考资料

- [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- [MISRA C:2012 Guidelines](https://www.misra.org.uk/)
- [DO-178C](https://en.wikipedia.org/wiki/DO-178C) - 航空软件安全标准
- [IEC 61508](https://en.wikipedia.org/wiki/IEC_61508) - 功能安全标准

---

## 附录：常见错误示例

### A.1 内部函数命名错误

```c
/* 错误1：使用下划线开头（违反C标准） */
static int32 _osal_task_find_by_id(osal_id_t id);  // ❌ 保留标识符
static void __osal_queue_acquire(void);            // ❌ 保留标识符

/* 错误2：内部函数使用对外API格式 */
static int32 OSAL_TaskFindById(osal_id_t id);     // ❌ 应该用小写

/* 错误3：对外API使用内部函数格式 */
int32 osal_task_create(osal_id_t *task_id, ...);  // ❌ 应该用大写

/* 正确：内部函数使用小写+下划线 */
static int32 osal_task_find_by_id(osal_id_t id);  // ✓
static void osal_queue_acquire(void);              // ✓

/* 正确：对外API使用大写+大驼峰 */
int32 OSAL_TaskCreate(osal_id_t *task_id, ...);   // ✓
```

### A.2 跨层调用
```c
/* 错误：Apps层直接调用HAL层 */
HAL_CANSend(fd, &frame);  // ❌

/* 正确：通过PDL层 */
PDL_SatelliteSendCommand(cmd);  // ✓
```

### A.2 未检查返回值
```c
/* 错误 */
malloc(size);  // ❌
PCL_Register(config);  // ❌

/* 正确 */
void *ptr = malloc(size);
if (ptr == NULL) {
    return OS_ERROR;
}

int32 ret = PCL_Register(config);
if (ret != OS_SUCCESS) {
    LOG_ERROR("MODULE", "Register failed");
    return OS_ERROR;
}
```

### A.3 不安全的字符串操作
```c
/* 错误 */
strcpy(dest, src);  // ❌
sprintf(buf, "%s", str);  // ❌

/* 正确 */
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';

snprintf(buf, sizeof(buf), "%s", str);
```

### A.4 任务无法退出
```c
/* 错误 */
static void task_entry(void *arg)
{
    while (1) {  // ❌ 无法退出
        do_work();
        sleep(1);
    }
}

/* 正确 */
static void task_entry(void *arg)
{
    while (!OS_TaskShouldShutdown()) {  // ✓
        do_work();
        OS_TaskDelay(1000);
    }
}
```

---

**版本**：v1.0  
**日期**：2026-04-25  
**维护者**：EMS 开发团队
