# OSAL层详细设计

## 1. OSAL 设计（系统抽象层）

### 1.1 OSAL 设计理念

OSAL（Operating System Abstraction Layer，操作系统抽象层）是EMS架构中实现跨平台兼容的核心组件，它在应用代码和底层操作系统之间提供统一的抽象接口，使得上层代码无需关心具体的操作系统实现细节。

#### 1.1.1 设计目标

1. **平台无关性**：上层代码（APP/ACL/PDL/HAL）完全不依赖具体操作系统API
2. **二进制兼容性**：支持32位/64位系统，确保数据结构在不同架构下的一致性
3. **架构可移植性**：支持ARM/AArch64/RISC-V等不同处理器架构
4. **OS类型兼容**：同时支持Linux（标准/实时）和RTOS（FreeRTOS/RT-Thread等）
5. **最小性能开销**：抽象层应尽可能薄，避免引入不必要的性能损耗
6. **易于移植**：新平台移植只需实现OSAL接口，无需修改上层代码

#### 1.1.2 核心设计原则

**原则1：统一类型定义**
- 使用OSAL定义的标准类型（如`osal_size_t`、`osal_pid_t`），而非平台相关类型（如`size_t`、`pid_t`）
- 确保类型在32位/64位系统下的语义一致性
- 避免使用可变长度类型（如`long`、`unsigned long`）

**原则2：接口语义统一**
- 所有OSAL接口在不同平台下必须保持相同的语义
- 返回值约定：成功返回0，失败返回负数错误码
- 参数约定：输出参数使用指针，输入参数使用值或const指针

**原则3：平台差异隔离**
- 平台相关实现放在`core/osal/src/{platform}/`目录下
- 使用条件编译隔离平台差异，但不在头文件中暴露平台宏
- 通过配置系统选择平台，而非手动定义宏

**原则4：最小依赖**
- OSAL不依赖任何第三方库
- 仅依赖标准C库（C99）和操作系统提供的系统调用
- 不使用C++特性，保持纯C实现

**原则5：错误处理一致性**
- 统一的错误码定义（`osal_errno.h`）
- 错误码与平台无关，上层代码不需要处理平台特定错误
- 提供错误码到字符串的转换函数

#### 1.1.3 分层架构

```text
┌─────────────────────────────────────────────────────────────┐
│           Application / ACL / PDL / HAL                     │
│         (平台无关代码，只使用OSAL接口)                       │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ OSAL API (统一接口)
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    OSAL Common Layer                        │
│  ┌──────────────┬──────────────┬──────────────────────┐    │
│  │ 类型定义      │ 错误处理      │ 公共工具函数          │    │
│  │ osal_types.h │ osal_errno.h │ osal_common.h        │    │
│  └──────────────┴──────────────┴──────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ 平台选择（编译时）
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Platform Adaptation Layer                      │
│  ┌──────────────────┬──────────────────┬─────────────────┐ │
│  │  Linux (POSIX)   │  FreeRTOS        │  RT-Thread      │ │
│  │  osal/src/linux/ │  osal/src/freertos/ │ osal/src/rtthread/ │ │
│  └──────────────────┴──────────────────┴─────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Operating System / Hardware                    │
│     Linux Kernel / FreeRTOS / RT-Thread / ...               │
└─────────────────────────────────────────────────────────────┘
```

#### 1.1.4 支持的平台矩阵

| 操作系统 | 架构 | 位宽 | 状态 | 备注 |
|---------|------|------|------|------|
| Linux (标准) | ARM Cortex-A | 32位 | ✓ 支持 | Debian/Ubuntu |
| Linux (标准) | AArch64 | 64位 | ✓ 支持 | Debian/Ubuntu |
| Linux (标准) | RISC-V | 64位 | ✓ 支持 | 需要GCC 10+ |
| Linux (实时) | ARM Cortex-A | 32位 | ✓ 支持 | PREEMPT_RT补丁 |
| Linux (实时) | AArch64 | 64位 | ✓ 支持 | PREEMPT_RT补丁 |
| FreeRTOS | ARM Cortex-M | 32位 | ○ 计划 | 需要适配层开发 |
| FreeRTOS | RISC-V | 32位 | ○ 计划 | 需要适配层开发 |
| RT-Thread | ARM Cortex-M | 32位 | ○ 计划 | 需要适配层开发 |

**说明**：
- ✓ 支持：已实现并测试
- ○ 计划：架构已预留，待实现
- 32位系统：指针4字节，`long`类型4字节
- 64位系统：指针8字节，`long`类型8字节（LP64模型）

### 1.2 OSAL 多进程支持（新增）

为支持多进程架构（如PMC产品），OSAL层需要提供进程管理、共享内存和进程间同步接口。

#### 1.2.1 进程管理接口

```c
/************************************************
 * osal/include/sys/osal_process.h
 * 进程管理接口（支持多进程架构）
 ************************************************/

/**
 * @brief 创建子进程
 *
 * @param[out] proc_id  进程ID
 * @param[in]  path     可执行文件路径
 * @param[in]  argv     参数列表（NULL结尾）
 * @param[in]  envp     环境变量（NULL结尾，NULL表示继承父进程）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_FREE_IDS 无法创建进程
 */
int32 OSAL_ProcessCreate(osal_id_t *proc_id, const char *path,
                         char *const argv[], char *const envp[]);

/**
 * @brief 等待子进程退出
 *
 * @param[in]  proc_id     进程ID
 * @param[out] status      退出状态码（可选，NULL表示不关心）
 * @param[in]  timeout_ms  超时时间（0表示不等待，-1表示永久等待）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32 OSAL_ProcessWait(osal_id_t proc_id, int32 *status, int32 timeout_ms);

/**
 * @brief 发送信号给进程
 *
 * @param[in] proc_id  进程ID
 * @param[in] signal   信号编号（SIGTERM/SIGKILL等）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32 OSAL_ProcessKill(osal_id_t proc_id, int32 signal);

/**
 * @brief 检查进程是否存在
 *
 * @param[in] proc_id  进程ID
 *
 * @return true 进程存在
 * @return false 进程不存在
 */
bool OSAL_ProcessExists(osal_id_t proc_id);

/**
 * @brief 获取当前进程ID
 *
 * @return 进程ID
 */
osal_id_t OSAL_ProcessGetId(void);

/**
 * @brief 获取父进程ID
 *
 * @return 父进程ID
 */
osal_id_t OSAL_ProcessGetParentId(void);
```

#### 1.2.2 共享内存接口

```c
/************************************************
 * osal/include/ipc/osal_shm.h
 * 共享内存接口（支持进程间通信）
 ************************************************/

/**
 * @brief 创建共享内存
 *
 * @param[out] shm_id  共享内存ID
 * @param[in]  name    共享内存名称（全局唯一）
 * @param[in]  size    大小（字节）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_FREE_IDS 无法创建共享内存
 */
int32 OSAL_ShmCreate(osal_id_t *shm_id, const char *name, uint32 size);

/**
 * @brief 打开已存在的共享内存
 *
 * @param[out] shm_id  共享内存ID
 * @param[in]  name    共享内存名称
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_INVALID_ID 共享内存不存在
 */
int32 OSAL_ShmOpen(osal_id_t *shm_id, const char *name);

/**
 * @brief 映射共享内存到进程地址空间
 *
 * @param[in]  shm_id  共享内存ID
 * @param[out] addr    映射后的地址
 * @param[in]  size    映射大小
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32 OSAL_ShmMap(osal_id_t shm_id, void **addr, uint32 size);

/**
 * @brief 取消映射共享内存
 *
 * @param[in] addr  映射地址
 * @param[in] size  映射大小
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32 OSAL_ShmUnmap(void *addr, uint32 size);

/**
 * @brief 关闭共享内存
 *
 * @param[in] shm_id  共享内存ID
 *
 * @return OSAL_SUCCESS 成功
 */
int32 OSAL_ShmClose(osal_id_t shm_id);

/**
 * @brief 删除共享内存
 *
 * @param[in] name  共享内存名称
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32 OSAL_ShmUnlink(const char *name);
```

#### 1.2.3 进程间互斥锁接口

```c
/************************************************
 * osal/include/ipc/osal_mutex.h（扩展）
 * 进程间互斥锁接口
 ************************************************/

/**
 * @brief 创建互斥锁
 *
 * @param[out] mutex_id  互斥锁ID
 * @param[in]  name      互斥锁名称
 * @param[in]  shared    是否进程间共享
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_FREE_IDS 无法创建互斥锁
 *
 * @note 如果shared=true，创建进程间互斥锁（需要放在共享内存中）
 *       如果shared=false，创建线程间互斥锁（进程内使用）
 */
int32 OSAL_MutexCreate(osal_id_t *mutex_id, const char *name, bool shared);

/**
 * @brief 带超时的加锁
 *
 * @param[in] mutex_id    互斥锁ID
 * @param[in] timeout_ms  超时时间（毫秒，0表示立即返回，-1表示永久等待）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 *
 * @note 用于避免死锁，建议所有进程间互斥锁都使用带超时的加锁
 */
int32 OSAL_MutexLockTimeout(osal_id_t mutex_id, uint32 timeout_ms);

/**
 * @brief 初始化共享内存中的互斥锁
 *
 * @param[in] mutex_ptr  互斥锁指针（指向共享内存）
 * @param[in] shared     是否进程间共享
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 *
 * @note 用于在共享内存中初始化互斥锁，供多进程使用
 */
int32 OSAL_MutexInitShared(void *mutex_ptr, bool shared);

/**
 * @brief 销毁共享内存中的互斥锁
 *
 * @param[in] mutex_ptr  互斥锁指针（指向共享内存）
 *
 * @return OSAL_SUCCESS 成功
 */
int32 OSAL_MutexDestroyShared(void *mutex_ptr);
```

/**
* @brief 获取调度策略
*/
int32_t OSAL_SchedGetScheduler(osal_pid_t pid,
							 osal_sched_policy_t *policy,
							 int32_t *priority);

/**
* @brief 设置CPU亲和性
*/
int32_t OSAL_SchedSetAffinity(osal_pid_t pid, uint32_t cpu_mask);

/**
* @brief 锁定内存
*/
int32_t OSAL_MemLockAll(void);

/**
* @brief 解锁内存
*/
int32_t OSAL_MemUnlockAll(void);
```

---

### 1.3 跨平台类型定义

OSAL提供统一的类型定义，确保代码在不同平台（32位/64位、不同架构）下的二进制兼容性和语义一致性。

#### 1.3.1 基础整数类型

```c
/************************************************
 * osal/include/osal_types.h
 * 跨平台基础类型定义
 ************************************************/

/* 固定宽度整数类型（所有平台保证相同） */
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;

#if defined(__LP64__) || defined(_WIN64)
    /* 64位系统：long是64位 */
    typedef signed long    int64_t;
    typedef unsigned long  uint64_t;
#else
    /* 32位系统：long long是64位 */
    typedef signed long long   int64_t;
    typedef unsigned long long uint64_t;
#endif

/* 布尔类型 */
#ifndef __cplusplus
    typedef uint8_t bool;
    #define true  1
    #define false 0
#endif
```

#### 1.3.2 平台相关类型

```c
/************************************************
 * 平台相关类型的统一抽象
 ************************************************/

/* 指针大小的整数类型（32位系统4字节，64位系统8字节） */
#if defined(__LP64__) || defined(_WIN64)
    typedef int64_t   osal_intptr_t;
    typedef uint64_t  osal_uintptr_t;
#else
    typedef int32_t   osal_intptr_t;
    typedef uint32_t  osal_uintptr_t;
#endif

/* 大小类型（用于内存大小、数组索引等） */
typedef osal_uintptr_t  osal_size_t;
typedef osal_intptr_t   osal_ssize_t;

/* 进程ID类型 */
typedef int32_t  osal_pid_t;

/* 线程ID类型 */
typedef osal_uintptr_t  osal_tid_t;

/* 时间类型 */
typedef int64_t  osal_time_t;      // 时间戳（微秒）
typedef uint32_t osal_tick_t;      // 系统滴答（毫秒）

/* 文件描述符类型 */
typedef int32_t  osal_fd_t;

/* 句柄类型（不透明指针） */
typedef void*  osal_handle_t;
```

#### 1.3.3 类型选择原则

**原则1：避免使用可变长度类型**

❌ **不推荐**：
```c
long count;              // 32位系统4字节，64位系统8字节
unsigned long size;      // 长度不确定
```

✅ **推荐**：
```c
int32_t count;           // 所有平台都是4字节
uint64_t size;           // 所有平台都是8字节
osal_size_t size;        // 跟随指针大小
```

**原则2：指针相关使用osal_size_t**

✅ **正确用法**：
```c
void* buffer = malloc(size);
osal_size_t size = 1024;              // 内存大小
osal_uintptr_t addr = (osal_uintptr_t)buffer;  // 指针转整数
```

**原则3：时间戳使用64位类型**

✅ **正确用法**：
```c
osal_time_t timestamp = OSAL_GetTimeUs();  // 微秒时间戳（64位）
osal_tick_t tick = OSAL_GetTick();         // 系统滴答（32位，够用）
```

#### 1.3.4 结构体对齐与打包

为确保结构体在不同平台下的二进制兼容性，需要显式控制对齐和打包。

```c
/************************************************
 * 结构体对齐宏定义
 ************************************************/

/* 编译器对齐控制 */
#if defined(__GNUC__)
    #define OSAL_PACKED  __attribute__((packed))
    #define OSAL_ALIGNED(n)  __attribute__((aligned(n)))
#elif defined(_MSC_VER)
    #define OSAL_PACKED
    #define OSAL_ALIGNED(n)  __declspec(align(n))
#else
    #define OSAL_PACKED
    #define OSAL_ALIGNED(n)
#endif

/* 结构体打包（禁用自动对齐） */
#if defined(__GNUC__)
    #define OSAL_PACK_BEGIN
    #define OSAL_PACK_END  __attribute__((packed))
#elif defined(_MSC_VER)
    #define OSAL_PACK_BEGIN  __pragma(pack(push, 1))
    #define OSAL_PACK_END    __pragma(pack(pop))
#else
    #define OSAL_PACK_BEGIN
    #define OSAL_PACK_END
#endif
```

**使用示例**：

```c
/* 示例1：网络协议结构体（需要紧凑打包） */
OSAL_PACK_BEGIN
typedef struct {
    uint8_t  type;       // 1字节
    uint16_t length;     // 2字节（紧跟type，无填充）
    uint32_t checksum;   // 4字节
    uint8_t  data[256];  // 数据
} OSAL_PACK_END protocol_packet_t;

/* 示例2：共享内存结构体（需要缓存行对齐） */
typedef struct {
    uint32_t counter;
    uint8_t  padding[60];  // 填充到64字节（缓存行大小）
} OSAL_ALIGNED(64) shared_counter_t;
```

#### 1.3.5 字节序处理

不同架构的字节序可能不同（ARM通常是小端，某些RISC-V可能是大端），需要提供字节序转换函数。

```c
/************************************************
 * 字节序转换函数
 ************************************************/

/* 检测当前平台字节序 */
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define OSAL_LITTLE_ENDIAN 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define OSAL_BIG_ENDIAN 1
    #endif
#else
    /* 默认假设小端（ARM/x86/RISC-V大多数实现） */
    #define OSAL_LITTLE_ENDIAN 1
#endif

/* 字节序转换宏 */
#define OSAL_SWAP16(x) \
    ((uint16_t)(((x) >> 8) | ((x) << 8)))

#define OSAL_SWAP32(x) \
    ((uint32_t)(((x) >> 24) | \
                (((x) & 0x00FF0000) >> 8) | \
                (((x) & 0x0000FF00) << 8) | \
                ((x) << 24)))

#define OSAL_SWAP64(x) \
    ((uint64_t)(((x) >> 56) | \
                (((x) & 0x00FF000000000000ULL) >> 40) | \
                (((x) & 0x0000FF0000000000ULL) >> 24) | \
                (((x) & 0x000000FF00000000ULL) >> 8) | \
                (((x) & 0x00000000FF000000ULL) << 8) | \
                (((x) & 0x0000000000FF0000ULL) << 24) | \
                (((x) & 0x000000000000FF00ULL) << 40) | \
                ((x) << 56)))

/* 主机序与网络序转换（网络序是大端） */
#ifdef OSAL_LITTLE_ENDIAN
    #define OSAL_HTONS(x)  OSAL_SWAP16(x)
    #define OSAL_HTONL(x)  OSAL_SWAP32(x)
    #define OSAL_HTONLL(x) OSAL_SWAP64(x)
    #define OSAL_NTOHS(x)  OSAL_SWAP16(x)
    #define OSAL_NTOHL(x)  OSAL_SWAP32(x)
    #define OSAL_NTOHLL(x) OSAL_SWAP64(x)
#else
    #define OSAL_HTONS(x)  (x)
    #define OSAL_HTONL(x)  (x)
    #define OSAL_HTONLL(x) (x)
    #define OSAL_NTOHS(x)  (x)
    #define OSAL_NTOHL(x)  (x)
    #define OSAL_NTOHLL(x) (x)
#endif
```

#### 1.3.6 原子操作类型

为支持无锁编程和多核同步，提供原子操作类型。

```c
/************************************************
 * 原子操作类型
 ************************************************/

/* 原子整数类型 */
typedef struct {
    volatile int32_t value;
} osal_atomic32_t;

typedef struct {
    volatile int64_t value;
} osal_atomic64_t;

/* 原子操作接口 */
int32_t OSAL_AtomicLoad32(const osal_atomic32_t *atomic);
void    OSAL_AtomicStore32(osal_atomic32_t *atomic, int32_t value);
int32_t OSAL_AtomicAdd32(osal_atomic32_t *atomic, int32_t value);
int32_t OSAL_AtomicSub32(osal_atomic32_t *atomic, int32_t value);
bool    OSAL_AtomicCAS32(osal_atomic32_t *atomic, int32_t expected, int32_t desired);

int64_t OSAL_AtomicLoad64(const osal_atomic64_t *atomic);
void    OSAL_AtomicStore64(osal_atomic64_t *atomic, int64_t value);
int64_t OSAL_AtomicAdd64(osal_atomic64_t *atomic, int64_t value);
int64_t OSAL_AtomicSub64(osal_atomic64_t *atomic, int64_t value);
bool    OSAL_AtomicCAS64(osal_atomic64_t *atomic, int64_t expected, int64_t desired);
```

**实现说明**：
- **ARM32**：使用`ldrex/strex`指令实现
- **AArch64**：使用`ldxr/stxr`指令实现
- **RISC-V**：使用`lr.w/sc.w`（32位）或`lr.d/sc.d`（64位）指令实现
- **GCC内建函数**：优先使用`__atomic_*`系列函数（GCC 4.7+）

#### 1.3.7 类型安全检查

编译时检查类型大小，确保跨平台一致性。

```c
/************************************************
 * 编译时类型大小检查
 ************************************************/

/* 静态断言宏 */
#define OSAL_STATIC_ASSERT(cond, msg) \
    typedef char static_assertion_##msg[(cond) ? 1 : -1]

/* 类型大小检查 */
OSAL_STATIC_ASSERT(sizeof(int8_t) == 1, int8_size_must_be_1);
OSAL_STATIC_ASSERT(sizeof(int16_t) == 2, int16_size_must_be_2);
OSAL_STATIC_ASSERT(sizeof(int32_t) == 4, int32_size_must_be_4);
OSAL_STATIC_ASSERT(sizeof(int64_t) == 8, int64_size_must_be_8);
OSAL_STATIC_ASSERT(sizeof(osal_size_t) == sizeof(void*), size_t_must_match_pointer);

/* 对齐检查 */
OSAL_STATIC_ASSERT(sizeof(osal_atomic32_t) == 4, atomic32_size_must_be_4);
OSAL_STATIC_ASSERT(sizeof(osal_atomic64_t) == 8, atomic64_size_must_be_8);
```



### 1.4 关于平台适配层设计（Linux/RTOS适配）

OSAL通过平台适配层实现对不同操作系统（Linux/RTOS）的支持，每个平台提供统一接口的不同实现。

#### 1.4.1 目录结构

```text
osal/
├── include/                    # 公共头文件（平台无关）
│   ├── osal_types.h           # 类型定义
│   ├── osal_errno.h           # 错误码定义
│   ├── sys/
│   │   ├── osal_thread.h      # 线程接口
│   │   ├── osal_mutex.h       # 互斥锁接口
│   │   ├── osal_sem.h         # 信号量接口
│   │   ├── osal_timer.h       # 定时器接口
│   │   ├── osal_process_mgmt.h # 进程管理接口
│   │   └── osal_sched.h       # 调度接口
│   └── ipc/
│       ├── osal_queue.h       # 消息队列接口
│       └── osal_shm.h         # 共享内存接口
│
├── src/
│   ├── common/                # 平台无关的公共实现
│   │   ├── osal_errno.c       # 错误处理
│   │   └── osal_utils.c       # 工具函数
│   │
│   ├── linux/                 # Linux平台实现
│   │   ├── osal_thread_linux.c
│   │   ├── osal_mutex_linux.c
│   │   ├── osal_sem_linux.c
│   │   ├── osal_timer_linux.c
│   │   ├── osal_process_linux.c
│   │   ├── osal_sched_linux.c
│   │   ├── osal_queue_linux.c
│   │   └── osal_shm_linux.c
│   │
│   ├── freertos/              # FreeRTOS平台实现
│   │   ├── osal_thread_freertos.c
│   │   ├── osal_mutex_freertos.c
│   │   ├── osal_sem_freertos.c
│   │   ├── osal_timer_freertos.c
│   │   └── osal_queue_freertos.c
│   │
│   └── rtthread/              # RT-Thread平台实现
│       ├── osal_thread_rtthread.c
│       ├── osal_mutex_rtthread.c
│       ├── osal_sem_rtthread.c
│       ├── osal_timer_rtthread.c
│       └── osal_queue_rtthread.c
│
└── build/
    ├── linux.mk               # Linux编译配置
    ├── freertos.mk            # FreeRTOS编译配置
    └── rtthread.mk            # RT-Thread编译配置
```

#### 1.4.2 Linux平台适配

Linux平台使用POSIX接口实现OSAL，支持标准Linux和实时Linux（PREEMPT_RT）。

**线程实现示例**：

```c
/************************************************
 * osal/src/linux/osal_thread_linux.c
 * Linux平台线程实现
 ************************************************/

#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include "osal_thread.h"

typedef struct {
    pthread_t       thread;
    osal_thread_fn  entry;
    void           *arg;
    bool            joinable;
} osal_thread_linux_t;

int32_t OSAL_ThreadCreate(osal_thread_t *thread,
                          const osal_thread_attr_t *attr,
                          osal_thread_fn entry,
                          void *arg)
{
    osal_thread_linux_t *linux_thread;
    pthread_attr_t pthread_attr;
    int ret;

    linux_thread = malloc(sizeof(osal_thread_linux_t));
    if (!linux_thread) {
        return -OSAL_ENOMEM;
    }

    linux_thread->entry = entry;
    linux_thread->arg = arg;
    linux_thread->joinable = (attr && attr->detached) ? false : true;

    pthread_attr_init(&pthread_attr);

    /* 设置栈大小 */
    if (attr && attr->stack_size > 0) {
        pthread_attr_setstacksize(&pthread_attr, attr->stack_size);
    }

    /* 设置调度策略（实时线程） */
    if (attr && attr->priority > 0) {
        struct sched_param param;
        param.sched_priority = attr->priority;
        pthread_attr_setschedpolicy(&pthread_attr, SCHED_FIFO);
        pthread_attr_setschedparam(&pthread_attr, &param);
        pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);
    }

    /* 设置分离状态 */
    if (attr && attr->detached) {
        pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
    }

    ret = pthread_create(&linux_thread->thread, &pthread_attr, 
                         (void*(*)(void*))entry, arg);
    pthread_attr_destroy(&pthread_attr);

    if (ret != 0) {
        free(linux_thread);
        return -OSAL_ERROR;
    }

    *thread = (osal_thread_t)linux_thread;
    return OSAL_OK;
}

int32_t OSAL_ThreadJoin(osal_thread_t thread, void **retval)
{
    osal_thread_linux_t *linux_thread = (osal_thread_linux_t*)thread;
    int ret;

    if (!linux_thread->joinable) {
        return -OSAL_EINVAL;
    }

    ret = pthread_join(linux_thread->thread, retval);
    free(linux_thread);

    return (ret == 0) ? OSAL_OK : -OSAL_ERROR;
}
```

**共享内存实现示例**：

```c
/************************************************
 * osal/src/linux/osal_shm_linux.c
 * Linux平台共享内存实现
 ************************************************/

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "osal_shm.h"

typedef struct {
    int         fd;
    osal_size_t size;
    char        name[64];
} osal_shm_linux_t;

int32_t OSAL_ShmCreate(const char *name,
                       osal_size_t size,
                       osal_shm_t *shm)
{
    osal_shm_linux_t *linux_shm;
    int fd;

    linux_shm = malloc(sizeof(osal_shm_linux_t));
    if (!linux_shm) {
        return -OSAL_ENOMEM;
    }

    /* 创建共享内存对象 */
    fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        free(linux_shm);
        return -OSAL_ERROR;
    }

    /* 设置大小 */
    if (ftruncate(fd, size) < 0) {
        close(fd);
        shm_unlink(name);
        free(linux_shm);
        return -OSAL_ERROR;
    }

    linux_shm->fd = fd;
    linux_shm->size = size;
    strncpy(linux_shm->name, name, sizeof(linux_shm->name) - 1);

    *shm = (osal_shm_t)linux_shm;
    return OSAL_OK;
}

void* OSAL_ShmMap(osal_shm_t shm, osal_size_t size)
{
    osal_shm_linux_t *linux_shm = (osal_shm_linux_t*)shm;
    void *addr;

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, 
                linux_shm->fd, 0);
    if (addr == MAP_FAILED) {
        return NULL;
    }

    return addr;
}
```

#### 1.4.3 FreeRTOS平台适配

FreeRTOS是轻量级RTOS，主要用于Cortex-M系列微控制器，不支持进程和虚拟内存。

**线程实现示例**：

```c
/************************************************
 * osal/src/freertos/osal_thread_freertos.c
 * FreeRTOS平台线程实现
 ************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "osal_thread.h"

typedef struct {
    TaskHandle_t    task;
    osal_thread_fn  entry;
    void           *arg;
} osal_thread_freertos_t;

int32_t OSAL_ThreadCreate(osal_thread_t *thread,
                          const osal_thread_attr_t *attr,
                          osal_thread_fn entry,
                          void *arg)
{
    osal_thread_freertos_t *freertos_thread;
    BaseType_t ret;
    UBaseType_t priority;
    uint32_t stack_size;

    freertos_thread = pvPortMalloc(sizeof(osal_thread_freertos_t));
    if (!freertos_thread) {
        return -OSAL_ENOMEM;
    }

    freertos_thread->entry = entry;
    freertos_thread->arg = arg;

    /* 转换优先级（OSAL优先级 -> FreeRTOS优先级） */
    priority = (attr && attr->priority > 0) ? 
               attr->priority : tskIDLE_PRIORITY + 1;

    /* 转换栈大小（字节 -> 字数） */
    stack_size = (attr && attr->stack_size > 0) ? 
                 (attr->stack_size / sizeof(StackType_t)) : 
                 configMINIMAL_STACK_SIZE;

    ret = xTaskCreate((TaskFunction_t)entry,
                      attr ? attr->name : "osal_task",
                      stack_size,
                      arg,
                      priority,
                      &freertos_thread->task);

    if (ret != pdPASS) {
        vPortFree(freertos_thread);
        return -OSAL_ERROR;
    }

    *thread = (osal_thread_t)freertos_thread;
    return OSAL_OK;
}

int32_t OSAL_ThreadJoin(osal_thread_t thread, void **retval)
{
    /* FreeRTOS不支持线程join，只能等待任务删除 */
    /* 这里可以通过事件组或通知机制模拟 */
    return -OSAL_ENOTSUP;
}
```

**消息队列实现示例**：

```c
/************************************************
 * osal/src/freertos/osal_queue_freertos.c
 * FreeRTOS平台消息队列实现
 ************************************************/

#include "FreeRTOS.h"
#include "queue.h"
#include "osal_queue.h"

typedef struct {
    QueueHandle_t queue;
    osal_size_t   msg_size;
} osal_queue_freertos_t;

int32_t OSAL_QueueCreate(const char *name,
                         uint32_t max_msgs,
                         osal_size_t msg_size,
                         osal_queue_t *queue)
{
    osal_queue_freertos_t *freertos_queue;

    freertos_queue = pvPortMalloc(sizeof(osal_queue_freertos_t));
    if (!freertos_queue) {
        return -OSAL_ENOMEM;
    }

    freertos_queue->queue = xQueueCreate(max_msgs, msg_size);
    if (!freertos_queue->queue) {
        vPortFree(freertos_queue);
        return -OSAL_ERROR;
    }

    freertos_queue->msg_size = msg_size;
    *queue = (osal_queue_t)freertos_queue;
    return OSAL_OK;
}

int32_t OSAL_QueueSend(osal_queue_t queue,
                       const void *msg,
                       uint32_t timeout_ms)
{
    osal_queue_freertos_t *freertos_queue = (osal_queue_freertos_t*)queue;
    TickType_t ticks;
    BaseType_t ret;

    ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? 
            portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    ret = xQueueSend(freertos_queue->queue, msg, ticks);
    return (ret == pdPASS) ? OSAL_OK : -OSAL_ETIMEDOUT;
}
```

#### 1.4.4 平台差异对比

| 功能 | Linux (POSIX) | FreeRTOS | RT-Thread | 适配策略 |
|------|--------------|----------|-----------|---------|
| **进程** | ✓ 支持 | ✗ 不支持 | ✗ 不支持 | RTOS返回ENOTSUP |
| **线程** | pthread | Task | Thread | 统一抽象为OSAL_Thread |
| **互斥锁** | pthread_mutex | Mutex | Mutex | 统一抽象为OSAL_Mutex |
| **信号量** | sem_t | Semaphore | Semaphore | 统一抽象为OSAL_Sem |
| **消息队列** | mqueue | Queue | MessageQueue | 统一抽象为OSAL_Queue |
| **共享内存** | shm_open | ✗ 不支持 | ✗ 不支持 | RTOS使用静态内存池模拟 |
| **定时器** | timer_create | Software Timer | Timer | 统一抽象为OSAL_Timer |
| **实时调度** | SCHED_FIFO/RR | 优先级抢占 | 优先级抢占 | 优先级映射 |
| **CPU亲和性** | sched_setaffinity | ✗ 不支持 | ✗ 不支持 | RTOS返回ENOTSUP |
| **内存锁定** | mlockall | N/A（无虚拟内存） | N/A | RTOS直接返回成功 |

#### 1.4.5 不支持功能的处理策略

对于某些平台不支持的功能，OSAL采用以下策略：

**策略1：返回错误码**
```c
/* FreeRTOS不支持进程管理 */
int32_t OSAL_ProcessCreate(const char *name, const char *path,
                           char *const argv[], osal_pid_t *pid)
{
#ifdef OSAL_PLATFORM_LINUX
    /* Linux实现 */
    return linux_process_create(name, path, argv, pid);
#else
    /* RTOS不支持 */
    return -OSAL_ENOTSUP;
#endif
}
```

**策略2：提供替代实现**
```c
/* FreeRTOS不支持共享内存，使用静态内存池模拟 */
int32_t OSAL_ShmCreate(const char *name, osal_size_t size, osal_shm_t *shm)
{
#ifdef OSAL_PLATFORM_LINUX
    return linux_shm_create(name, size, shm);
#else
    /* RTOS使用静态内存池 */
    return freertos_static_mem_alloc(name, size, shm);
#endif
}
```

**策略3：空操作（NOP）**
```c
/* RTOS无虚拟内存，内存锁定无意义 */
int32_t OSAL_MemLockAll(void)
{
#ifdef OSAL_PLATFORM_LINUX
    return (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) ? OSAL_OK : -OSAL_ERROR;
#else
    /* RTOS直接返回成功 */
    return OSAL_OK;
#endif
}
```

### 1.5 编译配置与条件编译

#### 1.5.1 平台选择机制

通过CMake或Makefile配置选择目标平台，避免手动定义宏。

**CMake配置示例**：

```cmake
# CMakeLists.txt

# 平台选择（通过-DOSAL_PLATFORM=linux指定）
set(OSAL_PLATFORM "linux" CACHE STRING "Target platform: linux, freertos, rtthread")

# 架构选择
set(OSAL_ARCH "aarch64" CACHE STRING "Target architecture: arm, aarch64, riscv")

# 位宽检测
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OSAL_BITS 64)
else()
    set(OSAL_BITS 32)
endif()

# 生成配置头文件
configure_file(
    ${CMAKE_SOURCE_DIR}/osal/include/osal_config.h.in
    ${CMAKE_BINARY_DIR}/osal/include/osal_config.h
)

# 添加平台相关源文件
if(OSAL_PLATFORM STREQUAL "linux")
    file(GLOB OSAL_PLATFORM_SRCS "osal/src/linux/*.c")
    target_compile_definitions(osal PRIVATE OSAL_PLATFORM_LINUX)
elseif(OSAL_PLATFORM STREQUAL "freertos")
    file(GLOB OSAL_PLATFORM_SRCS "osal/src/freertos/*.c")
    target_compile_definitions(osal PRIVATE OSAL_PLATFORM_FREERTOS)
elseif(OSAL_PLATFORM STREQUAL "rtthread")
    file(GLOB OSAL_PLATFORM_SRCS "osal/src/rtthread/*.c")
    target_compile_definitions(osal PRIVATE OSAL_PLATFORM_RTTHREAD)
endif()

target_sources(osal PRIVATE ${OSAL_PLATFORM_SRCS})
```

**配置头文件模板**：

```c
/************************************************
 * osal/include/osal_config.h.in
 * OSAL配置头文件（由CMake生成）
 ************************************************/

#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

/* 平台定义 */
#cmakedefine OSAL_PLATFORM_LINUX
#cmakedefine OSAL_PLATFORM_FREERTOS
#cmakedefine OSAL_PLATFORM_RTTHREAD

/* 架构定义 */
#define OSAL_ARCH_@OSAL_ARCH@

/* 位宽定义 */
#define OSAL_BITS @OSAL_BITS@

/* 版本信息 */
#define OSAL_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define OSAL_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define OSAL_VERSION_PATCH @PROJECT_VERSION_PATCH@

#endif /* OSAL_CONFIG_H */
```

#### 1.5.2 条件编译最佳实践

**原则1：平台判断集中在源文件，不在头文件**

❌ **不推荐**（头文件中暴露平台宏）：
```c
/* osal_thread.h */
#ifdef OSAL_PLATFORM_LINUX
    typedef pthread_t osal_thread_t;
#else
    typedef TaskHandle_t osal_thread_t;
#endif
```

✅ **推荐**（使用不透明句柄）：
```c
/* osal_thread.h */
typedef void* osal_thread_t;  // 不透明句柄

/* osal_thread_linux.c */
typedef struct {
    pthread_t thread;
    /* ... */
} osal_thread_linux_t;

/* osal_thread_freertos.c */
typedef struct {
    TaskHandle_t task;
    /* ... */
} osal_thread_freertos_t;
```

**原则2：使用函数指针表实现多态**

```c
/************************************************
 * 平台操作函数表
 ************************************************/

typedef struct {
    int32_t (*thread_create)(osal_thread_t*, const osal_thread_attr_t*, 
                             osal_thread_fn, void*);
    int32_t (*thread_join)(osal_thread_t, void**);
    int32_t (*thread_detach)(osal_thread_t);
    /* ... */
} osal_platform_ops_t;

/* 平台初始化时注册操作表 */
extern const osal_platform_ops_t* g_osal_ops;

int32_t OSAL_ThreadCreate(osal_thread_t *thread,
                          const osal_thread_attr_t *attr,
                          osal_thread_fn entry,
                          void *arg)
{
    return g_osal_ops->thread_create(thread, attr, entry, arg);
}
```

### 1.6 平台移植指南

#### 1.6.1 移植新平台的步骤

**步骤1：创建平台目录**
```bash
mkdir -p osal/src/newplatform
```

**步骤2：实现必需接口**

必须实现的核心接口：
- 线程管理：`OSAL_ThreadCreate/Join/Detach/Sleep`
- 互斥锁：`OSAL_MutexCreate/Lock/Unlock/Destroy`
- 信号量：`OSAL_SemCreate/Wait/Post/Destroy`
- 时间：`OSAL_GetTimeUs/GetTick/Sleep`
- 内存：`OSAL_Malloc/Free/Calloc/Realloc`

可选接口（不支持则返回`-OSAL_ENOTSUP`）：
- 进程管理：`OSAL_ProcessCreate/Wait/Kill`
- 共享内存：`OSAL_ShmCreate/Open/Map/Unmap`
- 实时调度：`OSAL_SchedSetScheduler/SetAffinity`

**步骤3：添加编译配置**
```cmake
# CMakeLists.txt
elseif(OSAL_PLATFORM STREQUAL "newplatform")
    file(GLOB OSAL_PLATFORM_SRCS "osal/src/newplatform/*.c")
    target_compile_definitions(osal PRIVATE OSAL_PLATFORM_NEWPLATFORM)
endif()
```

**步骤4：运行测试套件**
```bash
mkdir build && cd build
cmake -DOSAL_PLATFORM=newplatform ..
make
make test
```

#### 1.6.2 移植检查清单

- [ ] 所有必需接口已实现
- [ ] 类型定义正确（32位/64位兼容）
- [ ] 原子操作正确实现（使用平台原子指令）
- [ ] 字节序处理正确
- [ ] 错误码映射正确
- [ ] 通过OSAL测试套件
- [ ] 性能测试通过（线程创建、锁开销等）
- [ ] 内存泄漏检查通过
- [ ] 多线程压力测试通过

---

