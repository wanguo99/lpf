# ES-IPC: Embedded Systems Message Bus
## 设计文档 v1.0

**作者**: Claude (基于 NASA cFS SB、openBMC D-Bus、RTOS IPC 模式研究)  
**日期**: 2026-06-10  
**状态**: 设计阶段  
**目标平台**: Linux, Windows, RTOS (FreeRTOS/Zephyr/RTEMS)

---

## 执行摘要

ES-IPC 是一个轻量级、确定性的发布-订阅消息总线，专为 ES-Middleware 在嵌入式 Linux 和 RTOS 平台上设计。它提供线程安全、零分配的消息路由，具有静态配置和针对 TI AM62x 平台优化的无锁数据路径。

### 设计综合

本架构综合了三个竞争设计的最佳元素：
- **MicroBus 设计**：静态分配和位图路由
- **LiteBus 设计**：无锁 SPSC 队列和引用计数
- **ES-IPC 提案**：与 OSAL 的无缝集成

经过批判性评审发现的关键缺陷——包括缺失的 OSAL 队列原语、无锁算法中的 ABA 问题、跨进程同步间隙——已被系统性地解决。

### 核心差异化特性

1. **进程内设计**：消除序列化开销和跨进程故障模式
2. **每订阅者 SPSC 队列**：Lamport 算法提供无等待的有界延迟 <500ns
3. **静态路由表**：完美哈希实现 O(1) 主题查找
4. **引用计数消息池**：基于 epoch 的 ABA 防护确保内存安全
5. **完整 OSAL 集成**：利用现有原语（原子操作、信号量、共享内存），无需新的平台抽象

### 性能目标

- **发布延迟**：单订阅者 <200ns
- **订阅延迟**：缓存命中 <100ns  
- **确定性**：所有操作 O(1)，具有可证明的 WCET 边界
- **适用场景**：实时 CCM 应用（collector、logger、health、supervisor、comm）

---

## 目录

1. [设计原则](#设计原则)
2. [系统架构](#系统架构)
3. [核心组件](#核心组件)
4. [API 规范](#api-规范)
5. [数据结构](#数据结构)
6. [内存占用](#内存占用)
7. [性能特性](#性能特性)
8. [平台支持](#平台支持)
9. [配置选项](#配置选项)
10. [实施路线图](#实施路线图)
11. [测试策略](#测试策略)
12. [限制与权衡](#限制与权衡)
13. [未来扩展](#未来扩展)

---

## 设计原则

### 1. 仅静态分配
- 所有内存在初始化时分配
- 运行时路径中零 malloc/free
- 消除长期运行嵌入式系统中的碎片化和分配失败

### 2. 进程内聚焦
- 仅支持单一地址空间内的线程
- 无跨进程序列化
- 无共享内存同步复杂性
- 匹配实际 CCM 架构（5 个应用共享地址空间）

### 3. 无锁发布路径
- 消息入队仅使用原子操作
- 热路径中无互斥锁
- 通过 OSAL 信号量实现无等待的订阅者通知

### 4. 仅为使用的功能付费
- 通过 Kconfig 进行编译时功能选择
- 可选的统计/跟踪/过滤默认编译排除
- 基本配置的最小占用：6KB ROM + 12KB RAM

### 5. OSAL 原生设计
- 利用现有原语（OSAL_Atomic*、OSAL_Semaphore*、OSAL_Mutex*）
- 无需新的平台抽象层
- 立即移植到所有 OSAL 支持的平台

### 6. 确定性性能
- 所有操作的 WCET 有界
- 通过完美哈希实现 O(1) 主题查找
- 2 的幂次方队列大小用于按位取模
- 无无界循环或递归调用

### 7. 快速失败验证
- 输入验证并提前返回错误
- 带可配置策略的溢出检测（丢弃/阻塞/通知）
- 用于间隙检测的序列号

### 8. 关注点分离
- 核心路由逻辑独立于消息有效载荷格式
- 与现有 PRL 协议层干净集成，无耦合

---

## 系统架构

### 架构概述

ES-IPC 采用三层架构：

```
┌─────────────────────────────────────────────────────────────┐
│                    Publisher Threads                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  App 1   │  │  App 2   │  │  App 3   │  │  App N   │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
└───────┼─────────────┼─────────────┼─────────────┼─────────┘
        │             │             │             │
        └─────────────┴─────────────┴─────────────┘
                      │
        ┌─────────────▼──────────────────────────────┐
        │   Layer 1: Message Pool (Reference Count) │
        │  ┌──────────────────────────────────────┐  │
        │  │ 128 slots × 256 bytes (32KB)        │  │
        │  │ Free-list with epoch-based ABA guard│  │
        │  │ Atomic CAS for lock-free allocation │  │
        │  └──────────────────────────────────────┘  │
        └─────────────┬──────────────────────────────┘
                      │
        ┌─────────────▼──────────────────────────────┐
        │   Layer 2: Routing Engine (Perfect Hash)  │
        │  ┌──────────────────────────────────────┐  │
        │  │ 256 entries × 8 bytes (2KB)         │  │
        │  │ topic_id → subscriber_bitmask (32b) │  │
        │  │ O(1) lookup: index = topic_id & 0xFF│  │
        │  └──────────────────────────────────────┘  │
        └─────────────┬──────────────────────────────┘
                      │ (iterate bitmask)
        ┌─────────────▼──────────────────────────────┐
        │ Layer 3: Delivery Queues (SPSC per sub)   │
        │  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
        │  │ Queue 0  │  │ Queue 1  │  │ Queue N  │ │
        │  │ 64 slots │  │ 64 slots │  │ 64 slots │ │
        │  │ Lamport  │  │ Lamport  │  │ Lamport  │ │
        │  │ SPSC     │  │ SPSC     │  │ SPSC     │ │
        │  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
        └───────┼─────────────┼─────────────┼────────┘
                │             │             │
        ┌───────▼─────────────▼─────────────▼────────┐
        │          Subscriber Threads                 │
        │  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
        │  │  Sub 0   │  │  Sub 1   │  │  Sub N   │ │
        │  └──────────┘  └──────────┘  └──────────┘ │
        └─────────────────────────────────────────────┘
```

### 数据流

1. **发布者调用** `ESIPC_Alloc(topic, size)`
   - CAS 从消息池空闲列表弹出
   - 返回消息指针

2. **发布者填充** `message->payload` 直接（零拷贝）

3. **发布者调用** `ESIPC_Publish(msg)`
   - 哈希查找主题 → 订阅者位掩码
   - 遍历位掩码中的每一位：
     - 原子 `refcount++`
     - SPSC 入队指针
     - 信号量 post

4. **订阅者从** `OSAL_SemaphoreWait` 唤醒
   - SPSC 出队指针
   - 处理有效载荷

5. **订阅者调用** `ESIPC_Release(msg)`
   - 原子 `refcount--`
   - 如果为零，推回空闲列表

**关键路径**：5 次缓存行访问（路由条目、引用计数、队列头、队列尾、消息头）

---

## 核心组件

### 1. 消息池

**描述**：128 个槽 × 256 字节（32KB），带有标记指针的空闲列表栈，用于 ABA 防护。

**特性**：
- 每条消息的原子引用计数，在发布时初始化为 `subscriber_count`
- 通过 OSAL_AtomicCompareExchange 对栈头进行无锁分配
- 池耗尽触发溢出策略（丢弃/阻塞/回调）

**数据结构**：
```c
typedef struct {
    esipc_msg_t messages[ESIPC_MSG_POOL_SIZE];
    osal_atomic_uint64_t free_stack;  // 高 16 位 = epoch, 低 48 位 = 索引
    osal_atomic_uint32_t allocated_count;
    osal_atomic_uint64_t sequence_counter;
} esipc_msg_pool_t;
```

**操作**：
- **Alloc**: CAS 弹出栈头，递增 epoch 防止 ABA
- **Free**: CAS 推回栈头，仅当 refcount == 0

### 2. 路由表

**描述**：256 条目完美哈希表（2KB），映射 `topic_id` → `subscriber_bitmask`（32 位）。

**特性**：
- 从 Kconfig 生成的路由配置静态初始化
- O(1) 查找：`index = (topic_id & 0xFF)`
- 每个条目：16 位 topic_id + 32 位 subscriber_mask + 16 位 flags

**数据结构**：
```c
typedef struct {
    uint16_t topic_id;
    uint16_t flags;
    uint32_t subscriber_mask;  // 位 N 设置 = 订阅者 N 已订阅
} esipc_route_entry_t;

static esipc_route_entry_t g_routing_table[256] 
    __attribute__((section(".rodata")));
```

**操作**：
- **Lookup**: 单次数组索引，1 次缓存行访问
- **Subscribe**: 设置位掩码中的位（在初始化时使用互斥锁）

### 3. SPSC 队列数组

**描述**：32 个队列 × 64 槽 × 8 字节 = 16KB。Lamport 算法，带缓存行对齐的独立头（生产者）和尾（消费者）计数器。

**特性**：
- 2 的幂次方大小实现快速取模（`mask = size - 1`）
- 非阻塞入队，通过 `OSAL_SemaphoreTimedWait` 进行阻塞/定时出队
- 无锁（生产者修改头，消费者修改尾，无争用）

**数据结构**：
```c
typedef struct __attribute__((aligned(64))) {
    osal_atomic_uint32_t head;  // 生产者缓存行
    uint8_t _pad1[60];
    
    osal_atomic_uint32_t tail;  // 消费者缓存行
    uint8_t _pad2[60];
    
    uint32_t capacity;  // 2 的幂次方
    uint32_t mask;      // capacity - 1
    esipc_msg_t *slots[ESIPC_QUEUE_DEPTH];
    osal_semaphore_t *sem;
} esipc_queue_t;
```

**操作**：
- **Enqueue**: 原子递增头，存储指针，post 信号量（20-30ns）
- **Dequeue**: 等待信号量，原子递增尾，加载指针（20-30ns）

### 4. 订阅者注册表

**描述**：32 条目表（1KB），映射 `subscriber_id` → 队列指针、信号量句柄、统计计数器。

**特性**：
- 通过 `ESIPC_SUBSCRIBER_DEFINE()` 宏进行编译时注册
- 为诊断和优雅关闭启用反向查找

**数据结构**：
```c
typedef struct {
    uint16_t subscriber_id;
    esipc_queue_t *queue;
    osal_semaphore_t *sem;
    uint32_t msg_received;
    uint32_t msg_dropped;
} esipc_subscriber_info_t;
```

---

## API 规范

### 初始化 API

#### `ESIPC_Init`
```c
int32_t ESIPC_Init(const esipc_config_t *config);
```
**描述**：用静态配置初始化消息总线。从配置分配消息池、路由表、订阅者队列。

**参数**：
- `config`: 指向配置结构的指针（池大小、路由表、回调）

**返回**：
- `OSAL_SUCCESS`: 初始化成功
- `OSAL_ERR_NO_MEMORY`: 分配失败

**注意**：
- 必须在任何其他 API 之前调用
- 非线程安全（仅从主线程调用）

---

#### `ESIPC_Shutdown`
```c
int32_t ESIPC_Shutdown(void);
```
**描述**：排空所有队列，释放所有消息，释放资源。阻塞直到所有订阅者确认关闭。

**返回**：
- `OSAL_SUCCESS`: 关闭成功
- `OSAL_ERR_TIMEOUT`: 订阅者未优雅退出

**注意**：
- 幂等（安全多次调用）
- 设置全局关闭标志并唤醒所有订阅者

---

#### `ESIPC_GetStats`
```c
const esipc_stats_t* ESIPC_GetStats(void);
```
**描述**：返回指向全局统计结构的指针（发布计数、丢弃计数、峰值队列深度）。

**返回**：
- 指向统计结构的指针（只读，无需锁定）
- 如果 `CONFIG_ESIPC_STATS` 禁用，则为 NULL

---

### 发布 API

#### `ESIPC_Alloc`
```c
esipc_msg_t* ESIPC_Alloc(uint16_t topic_id, uint16_t payload_size);
```
**描述**：从池中分配消息。如果池耗尽返回 NULL。

**参数**：
- `topic_id`: 主题标识符（用于路由）
- `payload_size`: 有效载荷字节数（必须 ≤ `ESIPC_MAX_PAYLOAD_SIZE`）

**返回**：
- 指向已分配消息的指针，或 NULL（池满）

**注意**：
- 非阻塞
- 消息头预填充 topic_id、时间戳、序列号

---

#### `ESIPC_Publish`
```c
int32_t ESIPC_Publish(esipc_msg_t *msg);
```
**描述**：将预分配的消息发布给所有订阅者。原子递增引用计数，入队到订阅者队列，post 信号量。

**参数**：
- `msg`: 指向已分配消息的指针（来自 `ESIPC_Alloc`）

**返回**：
- `OSAL_SUCCESS`: 发布成功
- `OSAL_ERR_INVALID_POINTER`: msg 为 NULL
- `OSAL_ERR_NAME_NOT_FOUND`: 主题未路由

**注意**：
- 对发布者非阻塞
- 消息所有权转移到 IPC（发布后不要访问）

---

#### `ESIPC_Send`
```c
int32_t ESIPC_Send(uint16_t topic_id, const void *payload, uint16_t size);
```
**描述**：组合 Alloc + OSAL_memcpy + Publish 的便利包装器。

**参数**：
- `topic_id`: 主题标识符
- `payload`: 指向有效载荷数据的指针
- `size`: 有效载荷字节数

**返回**：
- 与 `ESIPC_Publish` 相同的错误码，外加：
- `OSAL_ERR_NO_MEMORY`: 池满

**注意**：
- 对于小消息（分配开销可接受）有用

---

### 订阅 API

#### `ESIPC_Subscribe`
```c
int32_t ESIPC_Subscribe(uint16_t topic_id, esipc_queue_t **queue_out);
```
**描述**：注册订阅者到主题，返回队列句柄。

**参数**：
- `topic_id`: 主题标识符
- `queue_out`: 接收队列句柄的输出参数

**返回**：
- `OSAL_SUCCESS`: 订阅成功
- `OSAL_ERR_RESOURCE_LIMIT`: 超过最大订阅者（32）

**注意**：
- 必须在初始化时调用（不是运行时）
- 线程安全（注册期间使用互斥锁）

---

#### `ESIPC_Receive`
```c
esipc_msg_t* ESIPC_Receive(esipc_queue_t *queue, int32_t timeout_ms);
```
**描述**：从订阅者队列出队消息。

**参数**：
- `queue`: 队列句柄（来自 `ESIPC_Subscribe`）
- `timeout_ms`: 超时毫秒
  - `0`: 非阻塞轮询
  - `-1`: 无限等待
  - `N`: 毫秒超时

**返回**：
- 指向消息的指针，或 NULL（超时/关闭）

**注意**：
- 阻塞变体使用 `OSAL_SemaphoreTimedWait`
- 调用者必须在完成时调用 `ESIPC_Release`

---

#### `ESIPC_Release`
```c
void ESIPC_Release(esipc_msg_t *msg);
```
**描述**：释放消息引用。原子递减引用计数，如果为零返回到池。

**参数**：
- `msg`: 指向消息的指针（来自 `ESIPC_Receive`）

**注意**：
- 幂等（对 NULL 调用安全）
- 每个接收到的消息必须精确调用一次以防止泄漏

---

#### `ESIPC_Unsubscribe`
```c
int32_t ESIPC_Unsubscribe(esipc_queue_t *queue);
```
**描述**：注销订阅者，刷新队列，释放所有待处理消息。

**参数**：
- `queue`: 队列句柄

**返回**：
- `OSAL_SUCCESS`: 取消订阅成功
- `OSAL_ERR_BUSY`: 消息在途（超时后重试）

**注意**：
- 阻塞直到队列排空
- 通常仅在关闭期间调用

---

## 数据结构

### 消息头
```c
typedef struct __attribute__((aligned(16))) {
    uint16_t topic_id;           // 主题标识符
    uint16_t payload_size;       // 有效载荷字节数
    uint32_t sequence;           // 序列号（间隙检测）
    uint32_t timestamp_us;       // 微秒时间戳
    osal_atomic_uint32_t refcount;  // 引用计数器
    uint32_t reserved;           // 保留供将来使用
} esipc_msg_header_t;  // 16 字节，ARM64 上的单缓存行
```

### 消息结构
```c
typedef struct __attribute__((aligned(64))) {
    esipc_msg_header_t header;
    uint8_t payload[ESIPC_MAX_PAYLOAD_SIZE];  // 240 字节默认
} esipc_msg_t;  // 总共 256 字节，每 1KB 页面 4 条消息
```

