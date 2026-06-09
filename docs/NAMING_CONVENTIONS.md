# ES-Middleware 命名规范

本文档定义 ES-Middleware 项目的命名规范，确保代码库的一致性和可维护性。

## 1. 通用规则

### 1.1 命名风格

- **C 语言标识符**: 使用 `snake_case`（小写字母 + 下划线）
- **宏定义**: 使用 `UPPER_CASE`（大写字母 + 下划线）
- **类型定义**: 使用 `snake_case_t` 后缀
- **枚举值**: 使用 `UPPER_CASE` 或带模块前缀的 `MODULE_UPPER_CASE`

### 1.2 命名长度

- **最小长度**: 3 个字符（除了循环变量 `i`, `j`, `k`）
- **最大长度**: 31 个字符（符合 C99 标准）
- **推荐长度**: 8-20 个字符

### 1.3 缩写规则

- 使用行业标准缩写（如 `CAN`, `I2C`, `SPI`, `UART`）
- 自定义缩写需在模块文档中说明
- 避免过度缩写（如 `tmp` → `temp`, `cnt` → `count`）

## 2. 模块前缀规范

### 2.1 核心模块前缀

每个核心模块使用统一的前缀，避免全局命名空间污染：

| 模块 | 前缀 | 示例 |
|------|------|------|
| OSAL | `OSAL_` / `osal_` | `OSAL_ThreadCreate()`, `osal_thread_t` |
| HAL | `HAL_` / `hal_` | `HAL_CAN_Init()`, `hal_can_frame_t` |
| PCONFIG | `PCONFIG_` / `pconfig_` | `PCONFIG_GetDevice()`, `pconfig_device_t` |
| PDL | `PDL_` / `pdl_` | `PDL_BMC_Init()`, `pdl_bmc_handle_t` |
| PRL | `PRL_` / `prl_` | `PRL_Encode()`, `prl_frame_t` |
| ACONFIG | `ACONFIG_` / `aconfig_` | `ACONFIG_Init()`, `aconfig_tc_config_t` |

### 2.2 产品模块前缀

产品模块使用产品名称作为前缀：

| 产品 | 前缀 | 示例 |
|------|------|------|
| CCM | `CCM_` / `ccm_` | `CCM_Collector_Init()`, `ccm_tm_cache_t` |

### 2.3 头文件保护宏前缀

头文件保护宏必须包含模块前缀，避免冲突：

```c
// ✅ 正确：包含模块前缀
#ifndef HAL_CAN_TYPES_H
#define HAL_CAN_TYPES_H
...
#endif /* HAL_CAN_TYPES_H */

// ❌ 错误：缺少模块前缀
#ifndef CAN_TYPES_H
#define CAN_TYPES_H
...
#endif /* CAN_TYPES_H */
```

## 3. 枚举命名规范

### 3.1 枚举类型命名

枚举类型使用 `snake_case_t` 后缀：

```c
typedef enum {
    ...
} module_enum_name_t;
```

### 3.2 枚举值命名

**规则**: 枚举值必须包含模块前缀，避免全局命名空间污染。

```c
// ✅ 正确：包含模块前缀
typedef enum {
    ACONFIG_TC_POWER_ON = 0,
    ACONFIG_TC_POWER_OFF = 1,
    ACONFIG_TC_POWER_RESET = 2,
    ACONFIG_TC_FUNC_MAX = 1000
} aconfig_tc_function_t;

// ❌ 错误：缺少模块前缀
typedef enum {
    TC_POWER_ON = 0,      // 可能与其他模块冲突
    TC_POWER_OFF = 1,
    TC_FUNC_MAX = 1000
} aconfig_tc_function_t;
```

### 3.3 枚举值分组

使用注释和数值范围对枚举值分组：

```c
typedef enum {
    /* 电源控制类 (0-99) */
    ACONFIG_TC_POWER_ON = 0,
    ACONFIG_TC_POWER_OFF = 1,
    
    /* 复位控制类 (100-199) */
    ACONFIG_TC_SOFT_RESET = 100,
    ACONFIG_TC_HARD_RESET = 101,
    
    /* 预留扩展空间 */
    ACONFIG_TC_FUNC_MAX = 1000
} aconfig_tc_function_t;
```

## 4. 结构体命名规范

### 4.1 结构体类型命名

结构体类型使用 `snake_case_t` 后缀：

```c
typedef struct {
    ...
} module_struct_name_t;
```

### 4.2 结构体成员命名

**规则**: 成员名称应清晰表达用途，避免歧义。

#### 4.2.1 时间相关成员

```c
// ✅ 推荐：明确表达含义
typedef struct {
    uint32_t data_validity_ms;           /* 数据有效期（毫秒） */
    uint32_t background_update_period_ms; /* 后台更新周期（毫秒） */
} aconfig_tm_config_t;

// ❌ 避免：含义不明确
typedef struct {
    uint32_t validity_ms;      /* 什么的有效期？ */
    uint32_t update_period_ms; /* 什么的更新周期？ */
} aconfig_tm_config_t;
```

#### 4.2.2 指针成员

```c
// ✅ 推荐：明确表达用途
typedef struct {
    void *user_context;        /* 用户上下文（项目特定） */
    void *private_data;        /* 私有数据（内部使用） */
} module_config_t;

// ❌ 避免：含义模糊
typedef struct {
    void *extra_data;          /* 什么额外数据？ */
    void *data;                /* 什么数据？ */
} module_config_t;
```

#### 4.2.3 布尔成员

布尔成员使用 `is_`, `has_`, `can_`, `should_` 等前缀：

```c
typedef struct {
    bool is_enabled;           /* 是否使能 */
    bool has_error;            /* 是否有错误 */
    bool can_retry;            /* 是否可重试 */
} module_status_t;
```

## 5. 函数命名规范

### 5.1 公共 API 函数

公共 API 函数使用模块前缀 + 动词 + 名词：

```c
// 格式：MODULE_Verb_Noun()
int32_t OSAL_ThreadCreate(osal_thread_t *thread, ...);
int32_t HAL_CAN_Init(hal_can_handle_t *handle);
int32_t ACONFIG_RegisterTable(const aconfig_config_table_t *table);
```

### 5.2 内部静态函数

内部静态函数使用小写 + 下划线，不需要模块前缀：

```c
static int32_t parse_config_entry(const char *line);
static void update_cache_timestamp(cache_entry_t *entry);
```

### 5.3 回调函数类型

回调函数类型使用 `_callback_t` 或 `_handler_t` 后缀：

```c
typedef void (*osal_signal_handler_t)(int32_t sig);
typedef int32_t (*hal_can_rx_callback_t)(const hal_can_frame_t *frame);
```

## 6. 宏定义命名规范

### 6.1 常量宏

常量宏使用模块前缀 + 大写：

```c
#define OSAL_THREAD_NAME_MAX_LEN    32
#define HAL_CAN_MAX_DATA_LEN        8
#define ACONFIG_TC_FUNC_MAX         1000
```

### 6.2 函数宏

函数宏使用模块前缀 + 大写，参数使用小写：

```c
#define OSAL_MIN(a, b)              ((a) < (b) ? (a) : (b))
#define HAL_CAN_IS_EXTENDED(id)     ((id) & 0x80000000)
```

### 6.3 条件编译宏

条件编译宏使用 `CONFIG_` 前缀（Kconfig 生成）：

```c
#ifdef CONFIG_OSAL
    /* OSAL 相关代码 */
#endif

#ifdef CONFIG_HAL_CAN
    /* CAN 相关代码 */
#endif
```

## 7. 文件命名规范

### 7.1 源文件

- **头文件**: `module_name.h`
- **实现文件**: `module_name.c`
- **平台特定**: `module_name_platform.c`（如 `osal_thread_linux.c`）

### 7.2 目录结构

```
module/
├── include/
│   └── module_api.h          # 公共 API
├── src/
│   ├── module_core.c         # 核心实现
│   ├── module_utils.c        # 工具函数
│   └── platform/
│       ├── module_linux.c    # Linux 平台实现
│       └── module_freertos.c # FreeRTOS 平台实现
└── tests/
    └── test_module.c         # 单元测试
```

## 8. 变量命名规范

### 8.1 全局变量

全局变量使用 `g_` 前缀 + 模块前缀：

```c
static osal_mutex_t g_osal_global_lock;
static hal_can_handle_t g_hal_can_handles[HAL_CAN_MAX_CHANNELS];
```

### 8.2 静态变量

静态变量使用 `s_` 前缀：

```c
static uint32_t s_init_count = 0;
static bool s_is_initialized = false;
```

### 8.3 局部变量

局部变量使用 `snake_case`，无前缀：

```c
int32_t ret;
uint32_t data_len;
hal_can_frame_t frame;
```

### 8.4 指针变量

指针变量名称应体现其指向的类型：

```c
osal_thread_t *thread;          // ✅ 清晰
hal_can_frame_t *frame_ptr;     // ✅ 可接受
void *ptr;                      // ❌ 避免（除非必要）
```

## 9. 常见命名模式

### 9.1 初始化/清理

```c
int32_t MODULE_Init(void);
int32_t MODULE_Cleanup(void);
int32_t MODULE_Deinit(void);    // 或使用 Deinit
```

### 9.2 创建/销毁

```c
int32_t MODULE_Create(module_handle_t *handle, ...);
int32_t MODULE_Destroy(module_handle_t handle);
```

### 9.3 打开/关闭

```c
int32_t MODULE_Open(const char *name, module_handle_t *handle);
int32_t MODULE_Close(module_handle_t handle);
```

### 9.4 读/写

```c
int32_t MODULE_Read(module_handle_t handle, uint8_t *buf, uint32_t len);
int32_t MODULE_Write(module_handle_t handle, const uint8_t *buf, uint32_t len);
```

### 9.5 获取/设置

```c
int32_t MODULE_GetConfig(module_handle_t handle, module_config_t *config);
int32_t MODULE_SetConfig(module_handle_t handle, const module_config_t *config);
```

## 10. 命名反模式（避免）

### 10.1 避免的命名

```c
// ❌ 避免：匈牙利命名法
int32_t iCount;
uint32_t u32DataLen;

// ❌ 避免：过度缩写
int32_t cnt;
int32_t tmp;
int32_t buf_sz;

// ❌ 避免：无意义的名称
int32_t data;
void *ptr;
int32_t value;

// ❌ 避免：拼音命名
int32_t shuju_len;
void *huancun;
```

### 10.2 推荐的命名

```c
// ✅ 推荐：清晰的英文命名
int32_t count;
uint32_t data_len;
int32_t buffer_size;
uint32_t frame_count;
void *cache_ptr;
```

## 11. 特殊场景命名

### 11.1 位域结构体

```c
typedef struct {
    uint32_t is_enabled : 1;       /* 使能标志 */
    uint32_t has_error : 1;        /* 错误标志 */
    uint32_t reserved : 30;        /* 保留位 */
} module_flags_t;
```

### 11.2 联合体

```c
typedef union {
    uint32_t raw;                  /* 原始值 */
    struct {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    } bytes;
} module_data_t;
```

### 11.3 函数指针表

```c
typedef struct {
    int32_t (*init)(void);
    int32_t (*read)(uint8_t *buf, uint32_t len);
    int32_t (*write)(const uint8_t *buf, uint32_t len);
    int32_t (*cleanup)(void);
} module_ops_t;
```

## 12. 命名检查清单

在提交代码前，检查以下项目：

- [ ] 所有公共 API 函数包含模块前缀
- [ ] 所有枚举值包含模块前缀
- [ ] 头文件保护宏包含模块前缀
- [ ] 结构体成员名称清晰表达用途
- [ ] 全局变量使用 `g_` 前缀
- [ ] 静态变量使用 `s_` 前缀
- [ ] 宏定义使用大写字母
- [ ] 类型定义使用 `_t` 后缀
- [ ] 避免使用匈牙利命名法
- [ ] 避免过度缩写

## 13. 参考资源

- [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- [MISRA C:2012 Guidelines](https://www.misra.org.uk/)
- [NASA C Style Guide](https://ntrs.nasa.gov/citations/19950022400)

---

**最后更新**: 2026-06-01  
**维护者**: wanguo  
**版本**: 1.0
