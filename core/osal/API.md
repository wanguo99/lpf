# OSAL API 参考文档

OSAL (Operating System Abstraction Layer) 提供跨平台的操作系统接口。

---

## 📋 目录

1. [线程管理](#线程管理)
2. [互斥锁](#互斥锁)
3. [信号量](#信号量)
4. [条件变量](#条件变量)
5. [共享内存](#共享内存)
6. [文件操作](#文件操作)
7. [网络通信](#网络通信)
8. [时间管理](#时间管理)
9. [内存管理](#内存管理)
10. [日志系统](#日志系统)

---

## 线程管理

### OSAL_ThreadCreate

创建新线程。

```c
int32_t OSAL_ThreadCreate(
    osal_thread_t **thread,
    void* (*start_routine)(void*),
    void *arg
);
```

**参数**:
- `thread`: 线程句柄指针（输出）
- `start_routine`: 线程函数
- `arg`: 传递给线程函数的参数

**返回值**:
- `0`: 成功
- `< 0`: 失败

**示例**:
```c
static void* thread_func(void *arg) {
    printf("Thread running\n");
    return NULL;
}

int main(void) {
    osal_thread_t *thread = NULL;
    int32_t ret = OSAL_ThreadCreate(&thread, thread_func, NULL);
    if (ret != 0) {
        printf("Failed to create thread\n");
        return -1;
    }
    
    OSAL_ThreadJoin(thread);
    return 0;
}
```

---

### OSAL_ThreadJoin

等待线程结束。

```c
int32_t OSAL_ThreadJoin(osal_thread_t *thread);
```

**参数**:
- `thread`: 线程句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

**注意**: 必须调用此函数回收线程资源。

---

### OSAL_Sleep

休眠指定毫秒数。

```c
void OSAL_Sleep(uint32_t milliseconds);
```

**参数**:
- `milliseconds`: 休眠时间（毫秒）

**示例**:
```c
printf("Start\n");
OSAL_Sleep(1000);  // 休眠 1 秒
printf("End\n");
```

---

## 互斥锁

### OSAL_MutexCreate

创建互斥锁。

```c
int32_t OSAL_MutexCreate(osal_mutex_t **mutex);
```

**参数**:
- `mutex`: 互斥锁句柄指针（输出）

**返回值**:
- `0`: 成功
- `< 0`: 失败

---

### OSAL_MutexLock

加锁。

```c
int32_t OSAL_MutexLock(osal_mutex_t *mutex);
```

**参数**:
- `mutex`: 互斥锁句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

**注意**: 如果锁已被占用，会阻塞等待。

---

### OSAL_MutexUnlock

解锁。

```c
int32_t OSAL_MutexUnlock(osal_mutex_t *mutex);
```

**参数**:
- `mutex`: 互斥锁句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

---

### OSAL_MutexDestroy

销毁互斥锁。

```c
int32_t OSAL_MutexDestroy(osal_mutex_t *mutex);
```

**参数**:
- `mutex`: 互斥锁句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

**示例**:
```c
static osal_mutex_t *mutex = NULL;
static int counter = 0;

void increment(void) {
    OSAL_MutexLock(mutex);
    counter++;
    OSAL_MutexUnlock(mutex);
}

int main(void) {
    OSAL_MutexCreate(&mutex);
    
    // 创建多个线程...
    
    OSAL_MutexDestroy(mutex);
    return 0;
}
```

---

## 信号量

### OSAL_SemaphoreCreate

创建信号量。

```c
int32_t OSAL_SemaphoreCreate(
    osal_semaphore_t **sem,
    uint32_t initial_value
);
```

**参数**:
- `sem`: 信号量句柄指针（输出）
- `initial_value`: 初始值

**返回值**:
- `0`: 成功
- `< 0`: 失败

---

### OSAL_SemaphoreWait

等待信号量（P 操作）。

```c
int32_t OSAL_SemaphoreWait(osal_semaphore_t *sem);
```

**参数**:
- `sem`: 信号量句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

**注意**: 如果信号量为 0，会阻塞等待。

---

### OSAL_SemaphorePost

释放信号量（V 操作）。

```c
int32_t OSAL_SemaphorePost(osal_semaphore_t *sem);
```

**参数**:
- `sem`: 信号量句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

---

### OSAL_SemaphoreDestroy

销毁信号量。

```c
int32_t OSAL_SemaphoreDestroy(osal_semaphore_t *sem);
```

**参数**:
- `sem`: 信号量句柄

**返回值**:
- `0`: 成功
- `< 0`: 失败

**示例（生产者-消费者）**:
```c
static osal_semaphore_t *sem = NULL;

void* producer(void *arg) {
    for (int i = 0; i < 5; i++) {
        printf("Produce item %d\n", i);
        OSAL_SemaphorePost(sem);
        OSAL_Sleep(100);
    }
    return NULL;
}

void* consumer(void *arg) {
    for (int i = 0; i < 5; i++) {
        OSAL_SemaphoreWait(sem);
        printf("Consume item %d\n", i);
    }
    return NULL;
}

int main(void) {
    OSAL_SemaphoreCreate(&sem, 0);
    
    // 创建生产者和消费者线程...
    
    OSAL_SemaphoreDestroy(sem);
    return 0;
}
```

---

## 内存管理

### OSAL_Malloc

分配内存。

```c
void* OSAL_Malloc(size_t size);
```

**参数**:
- `size`: 字节数

**返回值**:
- 成功: 内存指针
- 失败: `NULL`

---

### OSAL_Free

释放内存。

```c
void OSAL_Free(void *ptr);
```

**参数**:
- `ptr`: 内存指针

**示例**:
```c
char *buffer = OSAL_Malloc(1024);
if (buffer == NULL) {
    printf("Out of memory\n");
    return -1;
}

// 使用 buffer...

OSAL_Free(buffer);
```

---

## 时间管理

### OSAL_GetTime

获取当前时间（秒）。

```c
int32_t OSAL_GetTime(time_t *seconds);
```

**参数**:
- `seconds`: 时间输出（秒）

**返回值**:
- `0`: 成功
- `< 0`: 失败

---

### OSAL_GetTimeMs

获取当前时间（毫秒）。

```c
uint64_t OSAL_GetTimeMs(void);
```

**返回值**:
- 当前时间（毫秒）

**示例**:
```c
uint64_t start = OSAL_GetTimeMs();

// 执行操作...

uint64_t end = OSAL_GetTimeMs();
printf("Elapsed: %llu ms\n", end - start);
```

---

## 日志系统

### OSAL_LOG_DEBUG

输出调试日志。

```c
OSAL_LOG_DEBUG(const char *format, ...);
```

**参数**:
- `format`: 格式字符串（printf 风格）
- `...`: 可变参数

**示例**:
```c
int value = 42;
OSAL_LOG_DEBUG("Debug value: %d", value);
```

---

### OSAL_LOG_INFO

输出信息日志。

```c
OSAL_LOG_INFO(const char *format, ...);
```

---

### OSAL_LOG_ERROR

输出错误日志。

```c
OSAL_LOG_ERROR(const char *format, ...);
```

**示例**:
```c
int ret = some_function();
if (ret < 0) {
    OSAL_LOG_ERROR("Function failed with error: %d", ret);
}
```

---

## 版本信息

### print_version_info

打印版本信息。

```c
void print_version_info(void);
```

**输出**:
```
=== ES-Middleware Version Information ===
Version:      1.0.0
Build Date:   2026-05-28
Build Time:   18:00:00
Git Commit:   8544fb3
```

---

## 错误码

| 错误码 | 说明 |
|--------|------|
| 0 | 成功 |
| -1 | 一般错误 |
| -2 | 参数错误 |
| -3 | 内存不足 |
| -4 | 超时 |
| -5 | 资源忙 |

---

## 最佳实践

### 1. 资源管理

始终配对创建和销毁：

```c
// ✅ 正确
OSAL_MutexCreate(&mutex);
// 使用 mutex...
OSAL_MutexDestroy(mutex);

// ❌ 错误 - 忘记销毁
OSAL_MutexCreate(&mutex);
// 使用 mutex...
// 忘记调用 OSAL_MutexDestroy
```

### 2. 错误处理

检查返回值：

```c
// ✅ 正确
int32_t ret = OSAL_ThreadCreate(&thread, func, NULL);
if (ret != 0) {
    OSAL_LOG_ERROR("Failed to create thread");
    return -1;
}

// ❌ 错误 - 不检查返回值
OSAL_ThreadCreate(&thread, func, NULL);
```

### 3. 避免死锁

按固定顺序加锁：

```c
// ✅ 正确 - 固定顺序
OSAL_MutexLock(mutex1);
OSAL_MutexLock(mutex2);
// 临界区...
OSAL_MutexUnlock(mutex2);
OSAL_MutexUnlock(mutex1);

// ❌ 错误 - 可能死锁
// 线程 A: Lock(mutex1) -> Lock(mutex2)
// 线程 B: Lock(mutex2) -> Lock(mutex1)
```

---

## 平台支持

| 平台 | 支持状态 | 说明 |
|------|----------|------|
| Linux | ✅ 完全支持 | POSIX 实现 |
| macOS | ✅ 完全支持 | POSIX 实现 |
| Windows | 🚧 开发中 | Win32 实现 |
| RTOS | 🚧 计划中 | FreeRTOS/RT-Thread |

---

## 相关文档

- [新手入门教程](../docs/GETTING_STARTED.md)
- [开发者指南](../docs/DEVELOPER_GUIDE.md)
- [示例代码](../examples/02_osal_thread/)
- [架构文档](../docs/ARCHITECTURE.md)

---

**版本**: 1.0.0  
**最后更新**: 2026-05-28
