# OSAL 使用指南

本文档提供OSAL的实用示例和最佳实践。

## 目录

- [快速开始](#快速开始)
- [任务管理](#任务管理)
- [消息队列](#消息队列)
- [互斥锁](#互斥锁)
- [信号量](#信号量)
- [条件变量](#条件变量)
- [信号处理](#信号处理)
- [日志系统](#日志系统)
- [错误处理](#错误处理)
- [最佳实践](#最佳实践)
- [常见问题](#常见问题)

---

## 快速开始

### 最小示例

```c
#include "osal.h"

int main(void)
{
    /* OSAL作为用户态库，无需显式初始化 */
    
    LOG_INFO("Main", "应用启动");
    
    /* 使用OSAL接口 */
    OSAL_TaskDelay(1000);
    
    LOG_INFO("Main", "应用退出");
    return 0;
}
```

### 编译链接

```bash
# CMakeLists.txt
target_link_libraries(my_app osal)
```

---

## 任务管理

### 创建和删除任务

```c
#include "osal.h"

/* 任务入口函数 */
static void worker_task(void *arg)
{
    uint32_t counter = 0;
    
    LOG_INFO("Worker", "任务启动");
    
    /* 任务循环 - 必须检查退出标志 */
    while (!OSAL_TaskShouldShutdown())
    {
        LOG_INFO("Worker", "工作中: %u", counter++);
        OSAL_TaskDelay(1000);  /* 延时1秒 */
    }
    
    LOG_INFO("Worker", "任务退出");
}

int main(void)
{
    osal_id_t task_id;
    int32_t ret;
    
    /* 创建任务 */
    ret = OSAL_TaskCreate(&task_id, "WorkerTask",
                          worker_task, NULL,
                          16*1024,  /* 栈大小: 16KB */
                          100,      /* 优先级: 100 */
                          0);       /* 标志: 0 */
    if (ret != OS_SUCCESS) {
        LOG_ERROR("Main", "创建任务失败: %d", ret);
        return -1;
    }
    
    LOG_INFO("Main", "任务创建成功");
    
    /* 等待5秒 */
    OSAL_TaskDelay(5000);
    
    /* 删除任务 */
    ret = OSAL_TaskDelete(task_id);
    if (ret == OS_SUCCESS) {
        LOG_INFO("Main", "任务已删除");
    }
    
    return 0;
}
```

### 任务间传递参数

```c
typedef struct {
    uint32_t interval_ms;
    const char *name;
} task_config_t;

static void configurable_task(void *arg)
{
    task_config_t *config = (task_config_t *)arg;
    
    LOG_INFO(config->name, "任务启动，间隔: %u ms", config->interval_ms);
    
    while (!OSAL_TaskShouldShutdown())
    {
        /* 使用配置参数 */
        OSAL_TaskDelay(config->interval_ms);
    }
}

int main(void)
{
    task_config_t config = {
        .interval_ms = 500,
        .name = "FastTask"
    };
    
    osal_id_t task_id;
    OSAL_TaskCreate(&task_id, "ConfigTask",
                    configurable_task, &config,
                    16*1024, 100, 0);
    
    /* 注意: config必须在任务生命周期内有效 */
    OSAL_TaskDelay(5000);
    OSAL_TaskDelete(task_id);
    
    return 0;
}
```

---

## 消息队列

### 基本队列通信

```c
#include "osal.h"

#define QUEUE_DEPTH     10
#define MSG_SIZE        64

static osal_id_t g_queue_id;

/* 生产者任务 */
static void producer_task(void *arg)
{
    uint32_t counter = 0;
    
    while (!OSAL_TaskShouldShutdown())
    {
        char msg[MSG_SIZE];
        OSAL_Snprintf(msg, sizeof(msg), "Message #%u", counter++);
        
        /* 发送消息（超时1秒） */
        int32_t ret = OSAL_QueuePut(g_queue_id, msg, sizeof(msg), 1000);
        if (ret == OS_SUCCESS) {
            LOG_INFO("Producer", "发送: %s", msg);
        } else if (ret == OS_QUEUE_TIMEOUT) {
            LOG_WARN("Producer", "队列发送超时");
        }
        
        OSAL_TaskDelay(500);
    }
}

/* 消费者任务 */
static void consumer_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown())
    {
        char msg[MSG_SIZE];
        uint32_t size;
        
        /* 接收消息（超时2秒） */
        int32_t ret = OSAL_QueueGet(g_queue_id, msg, sizeof(msg), &size, 2000);
        if (ret == OS_SUCCESS) {
            LOG_INFO("Consumer", "接收: %s (大小: %u)", msg, size);
        } else if (ret == OS_QUEUE_TIMEOUT) {
            LOG_WARN("Consumer", "队列接收超时");
        }
    }
}

int main(void)
{
    osal_id_t producer_id, consumer_id;
    
    /* 创建队列 */
    OSAL_QueueCreate(&g_queue_id, "MsgQueue", QUEUE_DEPTH, MSG_SIZE, 0);
    
    /* 创建生产者和消费者任务 */
    OSAL_TaskCreate(&producer_id, "Producer", producer_task, NULL, 16*1024, 100, 0);
    OSAL_TaskCreate(&consumer_id, "Consumer", consumer_task, NULL, 16*1024, 100, 0);
    
    /* 运行10秒 */
    OSAL_TaskDelay(10000);
    
    /* 清理 */
    OSAL_TaskDelete(producer_id);
    OSAL_TaskDelete(consumer_id);
    OSAL_QueueDelete(g_queue_id);
    
    return 0;
}
```

### 非阻塞队列操作

```c
/* 非阻塞发送 */
int32_t ret = OSAL_QueuePut(queue_id, data, size, 0);
if (ret == OS_QUEUE_FULL) {
    LOG_WARN("Worker", "队列已满，丢弃消息");
}

/* 非阻塞接收 */
ret = OSAL_QueueGet(queue_id, data, size, &copied, OS_CHECK);
if (ret == OS_QUEUE_EMPTY) {
    LOG_DEBUG("Worker", "队列为空");
}
```

---

## 互斥锁

### 保护共享资源

```c
#include "osal.h"

static osal_id_t g_mutex_id;
static uint32_t g_shared_counter = 0;

static void increment_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown())
    {
        /* 获取锁 */
        int32_t ret = OSAL_MutexLock(g_mutex_id, 1000);
        if (ret == OS_SUCCESS) {
            /* 临界区 */
            g_shared_counter++;
            LOG_INFO("Task", "计数器: %u", g_shared_counter);
            
            /* 释放锁 */
            OSAL_MutexUnlock(g_mutex_id);
        } else {
            LOG_ERROR("Task", "获取锁超时");
        }
        
        OSAL_TaskDelay(100);
    }
}

int main(void)
{
    osal_id_t task1_id, task2_id;
    
    /* 创建互斥锁 */
    OSAL_MutexCreate(&g_mutex_id, "CounterMutex", 0);
    
    /* 创建两个任务同时访问共享资源 */
    OSAL_TaskCreate(&task1_id, "Task1", increment_task, NULL, 16*1024, 100, 0);
    OSAL_TaskCreate(&task2_id, "Task2", increment_task, NULL, 16*1024, 100, 0);
    
    OSAL_TaskDelay(5000);
    
    /* 清理 */
    OSAL_TaskDelete(task1_id);
    OSAL_TaskDelete(task2_id);
    OSAL_MutexDelete(g_mutex_id);
    
    LOG_INFO("Main", "最终计数: %u", g_shared_counter);
    
    return 0;
}
```

### 避免死锁

```c
/* 错误示例 - 可能死锁 */
void bad_example(void)
{
    OSAL_MutexLock(mutex1, OS_PEND);  /* 永久等待 */
    OSAL_MutexLock(mutex2, OS_PEND);  /* 如果mutex2被占用，死锁 */
    /* ... */
    OSAL_MutexUnlock(mutex2);
    OSAL_MutexUnlock(mutex1);
}

/* 正确示例 - 使用超时 */
void good_example(void)
{
    int32_t ret1 = OSAL_MutexLock(mutex1, 1000);
    if (ret1 != OS_SUCCESS) {
        LOG_ERROR("Worker", "获取mutex1超时");
        return;
    }
    
    int32_t ret2 = OSAL_MutexLock(mutex2, 1000);
    if (ret2 != OS_SUCCESS) {
        LOG_ERROR("Worker", "获取mutex2超时");
        OSAL_MutexUnlock(mutex1);  /* 释放已获取的锁 */
        return;
    }
    
    /* 临界区 */
    
    OSAL_MutexUnlock(mutex2);
    OSAL_MutexUnlock(mutex1);
}
```

---

## 信号量

信号量用于资源计数和线程同步，适合生产者-消费者模式和资源池管理。

### 生产者-消费者模式

```c
#include "osal.h"
#include <pthread.h>

#define BUFFER_SIZE 10

static osal_semaphore_t *empty_sem = NULL;  /* 空槽位计数 */
static osal_semaphore_t *full_sem = NULL;   /* 数据计数 */
static osal_mutex_t *buffer_mutex = NULL;   /* 缓冲区保护 */

static int32_t buffer[BUFFER_SIZE];
static uint32_t in_idx = 0;
static uint32_t out_idx = 0;
static volatile bool running = true;

/* 生产者线程 */
static void *producer_thread(void *arg)
{
    uint32_t data = 0;
    
    while (running) {
        /* 等待空槽位 */
        if (OSAL_SemaphoreWait(empty_sem) != OSAL_SUCCESS) {
            break;
        }
        
        /* 获取缓冲区锁 */
        OSAL_MutexLock(buffer_mutex);
        
        /* 生产数据 */
        buffer[in_idx] = data;
        LOG_INFO("Producer", "生产数据: %u (位置: %u)", data, in_idx);
        in_idx = (in_idx + 1) % BUFFER_SIZE;
        data++;
        
        /* 释放缓冲区锁 */
        OSAL_MutexUnlock(buffer_mutex);
        
        /* 增加数据计数 */
        OSAL_SemaphorePost(full_sem);
        
        OSAL_Sleep(100);  /* 模拟生产延时 */
    }
    
    return NULL;
}

/* 消费者线程 */
static void *consumer_thread(void *arg)
{
    while (running) {
        /* 等待数据 */
        if (OSAL_SemaphoreTimedWait(full_sem, 1000) != OSAL_SUCCESS) {
            continue;  /* 超时，继续等待 */
        }
        
        /* 获取缓冲区锁 */
        OSAL_MutexLock(buffer_mutex);
        
        /* 消费数据 */
        int32_t data = buffer[out_idx];
        LOG_INFO("Consumer", "消费数据: %d (位置: %u)", data, out_idx);
        out_idx = (out_idx + 1) % BUFFER_SIZE;
        
        /* 释放缓冲区锁 */
        OSAL_MutexUnlock(buffer_mutex);
        
        /* 增加空槽位计数 */
        OSAL_SemaphorePost(empty_sem);
        
        OSAL_Sleep(150);  /* 模拟消费延时 */
    }
    
    return NULL;
}

int main(void)
{
    pthread_t producer, consumer;
    
    /* 创建信号量 */
    OSAL_SemaphoreCreate(&empty_sem, BUFFER_SIZE);  /* 初始有 BUFFER_SIZE 个空槽位 */
    OSAL_SemaphoreCreate(&full_sem, 0);             /* 初始无数据 */
    OSAL_MutexCreate(&buffer_mutex);
    
    /* 创建线程 */
    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);
    
    LOG_INFO("Main", "生产者-消费者启动，按 Ctrl+C 退出");
    
    /* 运行 10 秒 */
    OSAL_Sleep(10000);
    
    /* 停止线程 */
    running = false;
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    
    /* 清理资源 */
    OSAL_SemaphoreDelete(empty_sem);
    OSAL_SemaphoreDelete(full_sem);
    OSAL_MutexDelete(buffer_mutex);
    
    LOG_INFO("Main", "程序退出");
    return 0;
}
```

### 资源池管理

```c
#include "osal.h"

#define MAX_CONNECTIONS 5

static osal_semaphore_t *conn_sem = NULL;

/* 获取连接 */
static int32_t acquire_connection(void)
{
    LOG_INFO("Worker", "等待连接...");
    
    /* 尝试获取连接（超时 3 秒） */
    int32_t ret = OSAL_SemaphoreTimedWait(conn_sem, 3000);
    if (ret == OSAL_SUCCESS) {
        LOG_INFO("Worker", "获取连接成功");
        return 0;
    } else if (ret == OSAL_ERR_TIMEOUT) {
        LOG_WARN("Worker", "连接池已满，获取超时");
        return -1;
    } else {
        LOG_ERROR("Worker", "获取连接失败: %d", ret);
        return -1;
    }
}

/* 释放连接 */
static void release_connection(void)
{
    OSAL_SemaphorePost(conn_sem);
    LOG_INFO("Worker", "释放连接");
}

/* 工作线程 */
static void *worker_thread(void *arg)
{
    int32_t worker_id = *(int32_t *)arg;
    
    for (int i = 0; i < 3; i++) {
        /* 获取连接 */
        if (acquire_connection() == 0) {
            /* 使用连接执行任务 */
            LOG_INFO("Worker%d", "执行任务 %d", worker_id, i + 1);
            OSAL_Sleep(1000);  /* 模拟任务执行 */
            
            /* 释放连接 */
            release_connection();
        }
        
        OSAL_Sleep(500);
    }
    
    return NULL;
}

int main(void)
{
    pthread_t workers[10];
    int32_t worker_ids[10];
    
    /* 创建信号量（最多 5 个并发连接） */
    OSAL_SemaphoreCreate(&conn_sem, MAX_CONNECTIONS);
    
    LOG_INFO("Main", "连接池大小: %d", MAX_CONNECTIONS);
    
    /* 创建 10 个工作线程竞争 5 个连接 */
    for (int i = 0; i < 10; i++) {
        worker_ids[i] = i + 1;
        pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]);
    }
    
    /* 等待所有线程完成 */
    for (int i = 0; i < 10; i++) {
        pthread_join(workers[i], NULL);
    }
    
    /* 清理 */
    OSAL_SemaphoreDelete(conn_sem);
    
    LOG_INFO("Main", "所有任务完成");
    return 0;
}
```

### 非阻塞尝试

```c
/* 非阻塞尝试获取信号量 */
int32_t ret = OSAL_SemaphoreTimedWait(sem, 0);
if (ret == OSAL_SUCCESS) {
    /* 获取成功，执行任务 */
    do_work();
    OSAL_SemaphorePost(sem);
} else {
    /* 资源不可用，跳过或稍后重试 */
    LOG_DEBUG("Worker", "资源忙，跳过");
}
```

---

## 条件变量

条件变量用于线程间的等待/通知机制，适合事件驱动和状态同步场景。

### 任务队列

```c
#include "osal.h"
#include <pthread.h>

#define MAX_TASKS 100

typedef struct {
    void (*func)(void *);
    void *arg;
} task_t;

static task_t task_queue[MAX_TASKS];
static uint32_t task_count = 0;
static osal_mutex_t *queue_mutex = NULL;
static osal_cond_t *queue_cond = NULL;
static volatile bool shutdown = false;

/* 添加任务 */
static void add_task(void (*func)(void *), void *arg)
{
    OSAL_MutexLock(queue_mutex);
    
    if (task_count < MAX_TASKS) {
        task_queue[task_count].func = func;
        task_queue[task_count].arg = arg;
        task_count++;
        
        LOG_INFO("Main", "添加任务，队列长度: %u", task_count);
        
        /* 通知工作线程 */
        OSAL_CondSignal(queue_cond);
    } else {
        LOG_WARN("Main", "任务队列已满");
    }
    
    OSAL_MutexUnlock(queue_mutex);
}

/* 工作线程 */
static void *worker_thread(void *arg)
{
    int32_t worker_id = *(int32_t *)arg;
    
    while (1) {
        OSAL_MutexLock(queue_mutex);
        
        /* 等待任务（使用循环防止虚假唤醒） */
        while (task_count == 0 && !shutdown) {
            LOG_DEBUG("Worker%d", "等待任务...", worker_id);
            OSAL_CondWait(queue_cond, queue_mutex);
        }
        
        /* 检查是否需要退出 */
        if (shutdown && task_count == 0) {
            OSAL_MutexUnlock(queue_mutex);
            break;
        }
        
        /* 取出任务 */
        task_t task = task_queue[0];
        for (uint32_t i = 0; i < task_count - 1; i++) {
            task_queue[i] = task_queue[i + 1];
        }
        task_count--;
        
        LOG_INFO("Worker%d", "执行任务，剩余: %u", worker_id, task_count);
        
        OSAL_MutexUnlock(queue_mutex);
        
        /* 执行任务（在锁外执行） */
        task.func(task.arg);
    }
    
    LOG_INFO("Worker%d", "线程退出", worker_id);
    return NULL;
}

/* 示例任务函数 */
static void example_task(void *arg)
{
    int32_t task_id = *(int32_t *)arg;
    LOG_INFO("Task", "执行任务 %d", task_id);
    OSAL_Sleep(500);  /* 模拟任务执行 */
}

int main(void)
{
    pthread_t workers[3];
    int32_t worker_ids[3];
    int32_t task_ids[10];
    
    /* 创建同步原语 */
    OSAL_MutexCreate(&queue_mutex);
    OSAL_CondCreate(&queue_cond);
    
    /* 创建工作线程 */
    for (int i = 0; i < 3; i++) {
        worker_ids[i] = i + 1;
        pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]);
    }
    
    LOG_INFO("Main", "任务队列启动，3 个工作线程");
    
    /* 添加任务 */
    for (int i = 0; i < 10; i++) {
        task_ids[i] = i + 1;
        add_task(example_task, &task_ids[i]);
        OSAL_Sleep(200);
    }
    
    /* 等待所有任务完成 */
    OSAL_Sleep(2000);
    
    /* 通知线程退出 */
    OSAL_MutexLock(queue_mutex);
    shutdown = true;
    OSAL_CondBroadcast(queue_cond);  /* 唤醒所有工作线程 */
    OSAL_MutexUnlock(queue_mutex);
    
    /* 等待线程退出 */
    for (int i = 0; i < 3; i++) {
        pthread_join(workers[i], NULL);
    }
    
    /* 清理 */
    OSAL_CondDelete(queue_cond);
    OSAL_MutexDelete(queue_mutex);
    
    LOG_INFO("Main", "程序退出");
    return 0;
}
```

### 事件通知

```c
#include "osal.h"

static osal_mutex_t *event_mutex = NULL;
static osal_cond_t *event_cond = NULL;
static volatile bool event_ready = false;
static int32_t event_data = 0;

/* 等待事件 */
static void *waiter_thread(void *arg)
{
    int32_t thread_id = *(int32_t *)arg;
    
    OSAL_MutexLock(event_mutex);
    
    LOG_INFO("Waiter%d", "等待事件...", thread_id);
    
    /* 等待事件（循环检查条件） */
    while (!event_ready) {
        OSAL_CondWait(event_cond, event_mutex);
    }
    
    LOG_INFO("Waiter%d", "收到事件，数据: %d", thread_id, event_data);
    
    OSAL_MutexUnlock(event_mutex);
    
    return NULL;
}

/* 触发事件 */
static void trigger_event(int32_t data)
{
    OSAL_MutexLock(event_mutex);
    
    /* 设置事件数据 */
    event_data = data;
    event_ready = true;
    
    LOG_INFO("Main", "触发事件，数据: %d", data);
    
    /* 唤醒所有等待线程 */
    OSAL_CondBroadcast(event_cond);
    
    OSAL_MutexUnlock(event_mutex);
}

int main(void)
{
    pthread_t waiters[5];
    int32_t thread_ids[5];
    
    /* 创建同步原语 */
    OSAL_MutexCreate(&event_mutex);
    OSAL_CondCreate(&event_cond);
    
    /* 创建等待线程 */
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&waiters[i], NULL, waiter_thread, &thread_ids[i]);
    }
    
    LOG_INFO("Main", "5 个线程等待事件");
    
    /* 等待 2 秒后触发事件 */
    OSAL_Sleep(2000);
    trigger_event(42);
    
    /* 等待所有线程完成 */
    for (int i = 0; i < 5; i++) {
        pthread_join(waiters[i], NULL);
    }
    
    /* 清理 */
    OSAL_CondDelete(event_cond);
    OSAL_MutexDelete(event_mutex);
    
    LOG_INFO("Main", "程序退出");
    return 0;
}
```

### 超时等待

```c
/* 等待事件（超时 5 秒） */
OSAL_MutexLock(mutex);

while (!condition_met) {
    int32_t ret = OSAL_CondTimedWait(cond, mutex, 5000);
    if (ret == OSAL_ERR_TIMEOUT) {
        LOG_WARN("Worker", "等待事件超时");
        OSAL_MutexUnlock(mutex);
        return -1;
    }
}

/* 条件满足，继续处理 */
process_event();

OSAL_MutexUnlock(mutex);
```

### 条件变量最佳实践

```c
/* ✅ 正确 - 循环检查条件（防止虚假唤醒） */
OSAL_MutexLock(mutex);
while (!condition) {
    OSAL_CondWait(cond, mutex);
}
/* 处理 */
OSAL_MutexUnlock(mutex);

/* ❌ 错误 - 不检查条件 */
OSAL_MutexLock(mutex);
OSAL_CondWait(cond, mutex);  /* 可能虚假唤醒 */
/* 处理 */
OSAL_MutexUnlock(mutex);

/* ✅ 正确 - 修改状态后通知 */
OSAL_MutexLock(mutex);
condition = true;
OSAL_CondSignal(cond);  /* 先修改状态，再通知 */
OSAL_MutexUnlock(mutex);

/* ❌ 错误 - 通知前未持有锁 */
condition = true;  /* 数据竞争 */
OSAL_CondSignal(cond);
```

---

## 信号处理

### 优雅退出

```c
#include "osal.h"

static volatile bool g_running = true;

/* 信号处理函数 */
static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT || sig == OS_SIGNAL_TERM) {
        LOG_INFO("Main", "收到退出信号");
        g_running = false;
    }
}

int main(void)
{
    /* 注册信号处理 */
    OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);   /* Ctrl+C */
    OSAL_SignalRegister(OS_SIGNAL_TERM, signal_handler);  /* kill */
    
    LOG_INFO("Main", "应用启动，按Ctrl+C退出");
    
    /* 主循环 */
    while (g_running)
    {
        /* 执行工作 */
        OSAL_TaskDelay(1000);
    }
    
    LOG_INFO("Main", "应用退出");
    return 0;
}
```

---

## 日志系统

### 日志级别

```c
/* DEBUG - 调试信息 */
LOG_DEBUG("Module", "变量值: %d", value);

/* INFO - 一般信息 */
LOG_INFO("Module", "任务启动成功");

/* WARN - 警告信息 */
LOG_WARN("Module", "队列接近满载: %u/%u", count, depth);

/* ERROR - 错误信息 */
LOG_ERROR("Module", "打开文件失败: %s", path);

/* FATAL - 致命错误 */
LOG_FATAL("Module", "内存分配失败，程序退出");
```

### 日志输出格式

```
[2026-04-26 10:30:45.123] [INFO] [Worker] 任务启动成功
[2026-04-26 10:30:46.456] [ERROR] [CAN] 发送失败: -1
```

### 简单打印

```c
/* 不带时间戳和级别的简单打印 */
OSAL_Printf("========================================\n");
OSAL_Printf("  应用版本: %s\n", APP_VERSION);
OSAL_Printf("  OSAL版本: %s\n", OS_GetVersionString());
OSAL_Printf("========================================\n");
```

---

## 错误处理

### 检查返回值

```c
/* 所有OSAL函数都返回int32状态码 */
int32_t ret = OSAL_TaskCreate(&task_id, "MyTask", task_entry, NULL,
                               16*1024, 100, 0);
if (ret != OS_SUCCESS) {
    /* 获取错误描述 */
    const char *err_str = OSAL_GetErrorString(ret);
    LOG_ERROR("Main", "创建任务失败: %s (%d)", err_str, ret);
    return -1;
}
```

### 常见错误码

```c
OS_SUCCESS              /* 成功 */
OS_ERROR                /* 通用错误 */
OS_INVALID_POINTER      /* 无效指针 */
OS_ERR_INVALID_ID       /* 无效ID */
OS_ERR_NAME_TOO_LONG    /* 名称过长 */
OS_ERR_NO_FREE_IDS      /* 无可用ID */
OS_ERR_NAME_TAKEN       /* 名称已存在 */
OS_SEM_TIMEOUT          /* 超时 */
OS_QUEUE_EMPTY          /* 队列为空 */
OS_QUEUE_FULL           /* 队列已满 */
OS_QUEUE_TIMEOUT        /* 队列超时 */
OS_ERR_NO_MEMORY        /* 内存不足 */
```

---

## 最佳实践

### 1. 任务编写规范

```c
/* ✅ 正确 - 检查退出标志 */
static void good_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown())
    {
        /* 执行工作 */
        OSAL_TaskDelay(100);
    }
    /* 清理资源 */
}

/* ❌ 错误 - 无限循环 */
static void bad_task(void *arg)
{
    while (1)  /* 无法优雅退出 */
    {
        /* 执行工作 */
        OSAL_TaskDelay(100);
    }
}
```

### 2. 资源管理

```c
/* ✅ 正确 - 成对创建和删除 */
osal_id_t task_id;
OSAL_TaskCreate(&task_id, "MyTask", task_entry, NULL, 16*1024, 100, 0);
/* 使用任务 */
OSAL_TaskDelete(task_id);  /* 必须删除 */

/* ✅ 正确 - 错误时清理 */
osal_id_t queue_id, task_id;

if (OSAL_QueueCreate(&queue_id, "Queue", 10, 64, 0) != OS_SUCCESS) {
    return -1;
}

if (OSAL_TaskCreate(&task_id, "Task", task_entry, NULL, 16*1024, 100, 0) != OS_SUCCESS) {
    OSAL_QueueDelete(queue_id);  /* 清理已创建的资源 */
    return -1;
}
```

### 3. 使用超时

```c
/* ✅ 正确 - 使用超时避免永久阻塞 */
int32_t ret = OSAL_MutexLock(mutex_id, 1000);  /* 1秒超时 */
if (ret == OS_SEM_TIMEOUT) {
    LOG_ERROR("Worker", "获取锁超时，可能死锁");
    return;
}

/* ❌ 错误 - 永久等待可能导致死锁 */
OSAL_MutexLock(mutex_id, OS_PEND);  /* 危险 */
```

### 4. 日志使用

```c
/* ✅ 正确 - 使用LOG宏 */
LOG_INFO("Module", "任务启动");
LOG_ERROR("Module", "错误: %d", ret);

/* ❌ 错误 - 直接使用printf */
printf("任务启动\n");  /* 不推荐 */
```

### 5. 命名规范

```c
/* ✅ 正确 - 描述性名称 */
OSAL_TaskCreate(&task_id, "WorkerTask", ...);
OSAL_QueueCreate(&queue_id, "MsgQueue", ...);
OSAL_MutexCreate(&mutex_id, "DataMutex", ...);

/* ❌ 错误 - 无意义名称 */
OSAL_TaskCreate(&task_id, "Task1", ...);
OSAL_QueueCreate(&queue_id, "Q", ...);
```

---

## 常见问题

### Q1: 任务无法退出？

**原因**: 任务循环未检查`OSAL_TaskShouldShutdown()`

**解决**:
```c
while (!OSAL_TaskShouldShutdown())  /* 必须检查 */
{
    /* 任务逻辑 */
}
```

### Q2: 队列操作失败？

**原因**: 队列已满或已空

**解决**:
```c
int32_t ret = OSAL_QueuePut(queue_id, data, size, 1000);
if (ret == OS_QUEUE_TIMEOUT) {
    LOG_WARN("Worker", "队列满，消息被丢弃");
}
```

### Q3: 互斥锁超时？

**原因**: 可能死锁或锁持有时间过长

**解决**:
- 检查是否忘记释放锁
- 减少临界区代码
- 检查锁的获取顺序

### Q4: 内存泄漏？

**原因**: 资源未释放

**解决**:
```c
/* 确保所有创建的资源都被删除 */
OSAL_TaskDelete(task_id);
OSAL_QueueDelete(queue_id);
OSAL_MutexDelete(mutex_id);
```

### Q5: 如何调试？

**方法**:
1. 使用LOG_DEBUG输出调试信息
2. 检查返回值和错误码
3. 使用GDB调试
4. 查看日志文件

---

## 完整示例

参考 [sample_app](../../apps/sample_app/README.md) 获取完整的应用示例。

## 相关文档

- [架构设计](ARCHITECTURE.md)
- [API参考](API_REFERENCE.md)
- [模块概述](README.md)
# OSAL API 参考手册

本文档提供OSAL所有公共API的详细说明。

## 目录

- [IPC模块 - 进程间通信](#ipc模块---进程间通信)
  - [任务管理](#任务管理)
  - [消息队列](#消息队列)
  - [互斥锁](#互斥锁)
  - [信号量](#信号量)
  - [条件变量](#条件变量)
  - [原子操作](#原子操作)
- [SYS模块 - 系统调用](#sys模块---系统调用)
  - [时钟](#时钟)
  - [信号处理](#信号处理)
  - [文件操作](#文件操作)
  - [Select](#select)
  - [环境变量](#环境变量)
  - [时间操作](#时间操作)
- [NET模块 - 网络](#net模块---网络)
  - [Socket](#socket)
  - [Termios](#termios)
- [LIB模块 - 标准库](#lib模块---标准库)
  - [字符串操作](#字符串操作)
  - [内存管理](#内存管理)
  - [错误处理](#错误处理)
- [UTIL模块 - 工具](#util模块---工具)
  - [日志系统](#日志系统)
  - [版本信息](#版本信息)

---

## IPC模块 - 进程间通信

### 任务管理

#### OSAL_TaskCreate

创建新任务。

```c
int32_t OSAL_TaskCreate(osal_id_t *task_id,
                        const char *task_name,
                        osal_task_entry entry_point,
                        void *arg,
                        uint32_t stack_size,
                        uint32_t priority,
                        uint32_t flags);
```

**参数**：
- `task_id` - [输出] 任务ID
- `task_name` - 任务名称（最大20字符）
- `entry_point` - 任务入口函数
- `arg` - 传递给任务的参数
- `stack_size` - 栈大小（字节）
- `priority` - 优先级（0-255，数值越大优先级越高）
- `flags` - 标志位（保留，传0）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_TOO_LONG` - 名称过长
- `OS_ERR_NO_FREE_IDS` - 无可用任务槽
- `OS_ERR_NAME_TAKEN` - 名称已存在
- `OS_ERROR` - 创建失败

**示例**：
```c
void my_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown()) {
        /* 任务逻辑 */
        OSAL_TaskDelay(1000);
    }
}

osal_id_t task_id;
int32_t ret = OSAL_TaskCreate(&task_id, "MyTask", my_task, NULL,
                               16*1024, 100, 0);
if (ret != OS_SUCCESS) {
    LOG_ERROR("Main", "创建任务失败: %d", ret);
}
```

#### OSAL_TaskDelete

删除任务。

```c
int32_t OSAL_TaskDelete(osal_id_t task_id);
```

**参数**：
- `task_id` - 任务ID

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERR_INVALID_ID` - 无效任务ID
- `OS_ERROR` - 删除失败

**注意**：
- 删除前会设置shutdown标志并等待任务退出（5秒超时）
- 任务应在循环中检查`OSAL_TaskShouldShutdown()`
- 超时后任务会被detach而非强制取消

#### OSAL_TaskShouldShutdown

检查任务是否应该退出。

```c
bool OSAL_TaskShouldShutdown(void);
```

**返回值**：
- `true` - 应该退出
- `false` - 继续运行

**示例**：
```c
void worker_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown()) {
        /* 执行工作 */
        OSAL_TaskDelay(100);
    }
    /* 清理资源 */
}
```

#### OSAL_TaskDelay

任务延时。

```c
int32_t OSAL_TaskDelay(uint32_t milliseconds);
```

**参数**：
- `milliseconds` - 延时时间（毫秒）

**返回值**：
- `OS_SUCCESS` - 成功

#### OSAL_TaskGetId

获取当前任务ID。

```c
osal_id_t OSAL_TaskGetId(void);
```

**返回值**：
- 当前任务ID
- 0 - 如果不在OSAL任务上下文中

#### OSAL_TaskGetIdByName

根据名称查找任务ID。

```c
int32_t OSAL_TaskGetIdByName(osal_id_t *task_id, const char *task_name);
```

**参数**：
- `task_id` - [输出] 任务ID
- `task_name` - 任务名称

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_NOT_FOUND` - 未找到任务

---

### 消息队列

#### OSAL_QueueCreate

创建消息队列。

```c
int32_t OSAL_QueueCreate(osal_id_t *queue_id,
                         const char *queue_name,
                         uint32_t queue_depth,
                         uint32_t data_size,
                         uint32_t flags);
```

**参数**：
- `queue_id` - [输出] 队列ID
- `queue_name` - 队列名称（最大20字符）
- `queue_depth` - 队列深度（最大消息数）
- `data_size` - 单个消息大小（字节）
- `flags` - 标志位（保留，传0）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_TOO_LONG` - 名称过长
- `OS_QUEUE_INVALID_SIZE` - 无效大小
- `OS_ERR_NO_FREE_IDS` - 无可用队列槽
- `OS_ERR_NAME_TAKEN` - 名称已存在
- `OS_ERR_NO_MEMORY` - 内存不足

**限制**：
- `queue_depth` ≤ 10000
- `data_size` ≤ 65536

**示例**：
```c
osal_id_t queue_id;
int32_t ret = OSAL_QueueCreate(&queue_id, "MsgQueue", 10, 64, 0);
if (ret != OS_SUCCESS) {
    LOG_ERROR("Main", "创建队列失败: %d", ret);
}
```

#### OSAL_QueueDelete

删除消息队列。

```c
int32_t OSAL_QueueDelete(osal_id_t queue_id);
```

**参数**：
- `queue_id` - 队列ID

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERR_INVALID_ID` - 无效队列ID

**注意**：
- 删除会唤醒所有等待线程
- 使用引用计数，最后一个使用者释放资源

#### OSAL_QueuePut

向队列发送消息。

```c
int32_t OSAL_QueuePut(osal_id_t queue_id,
                      const void *data,
                      uint32_t size,
                      uint32_t timeout);
```

**参数**：
- `queue_id` - 队列ID
- `data` - 消息数据
- `size` - 消息大小（必须≤创建时的data_size）
- `timeout` - 超时时间（毫秒），0表示非阻塞，OS_PEND表示永久等待

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_INVALID_ID` - 无效队列ID
- `OS_QUEUE_INVALID_SIZE` - 消息过大
- `OS_QUEUE_FULL` - 队列已满（非阻塞模式）
- `OS_QUEUE_TIMEOUT` - 超时

**示例**：
```c
char msg[64] = "Hello";
int32_t ret = OSAL_QueuePut(queue_id, msg, sizeof(msg), 1000);
if (ret == OS_QUEUE_TIMEOUT) {
    LOG_WARN("Worker", "队列发送超时");
}
```

#### OSAL_QueueGet

从队列接收消息。

```c
int32_t OSAL_QueueGet(osal_id_t queue_id,
                      void *data,
                      uint32_t size,
                      uint32_t *size_copied,
                      int32_t timeout);
```

**参数**：
- `queue_id` - 队列ID
- `data` - [输出] 接收缓冲区
- `size` - 缓冲区大小
- `size_copied` - [输出] 实际复制的字节数（可为NULL）
- `timeout` - 超时时间（毫秒），OS_CHECK表示非阻塞，OS_PEND表示永久等待

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_INVALID_ID` - 无效队列ID
- `OS_QUEUE_EMPTY` - 队列为空（非阻塞模式）
- `OS_QUEUE_TIMEOUT` - 超时

**示例**：
```c
char msg[64];
uint32_t size;
int32_t ret = OSAL_QueueGet(queue_id, msg, sizeof(msg), &size, 5000);
if (ret == OS_SUCCESS) {
    LOG_INFO("Stats", "接收消息: %s (大小: %u)", msg, size);
}
```

#### OSAL_QueueGetIdByName

根据名称查找队列ID。

```c
int32_t OSAL_QueueGetIdByName(osal_id_t *queue_id, const char *queue_name);
```

**参数**：
- `queue_id` - [输出] 队列ID
- `queue_name` - 队列名称

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_NOT_FOUND` - 未找到队列

---

### 互斥锁

#### OSAL_MutexCreate

创建互斥锁。

```c
int32_t OSAL_MutexCreate(osal_id_t *mutex_id,
                         const char *mutex_name,
                         uint32_t options);
```

**参数**：
- `mutex_id` - [输出] 互斥锁ID
- `mutex_name` - 互斥锁名称（最大20字符）
- `options` - 选项（保留，传0）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_TOO_LONG` - 名称过长
- `OS_ERR_NO_FREE_IDS` - 无可用互斥锁槽
- `OS_ERR_NAME_TAKEN` - 名称已存在
- `OS_ERROR` - 创建失败

#### OSAL_MutexDelete

删除互斥锁。

```c
int32_t OSAL_MutexDelete(osal_id_t mutex_id);
```

**参数**：
- `mutex_id` - 互斥锁ID

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERR_INVALID_ID` - 无效互斥锁ID

#### OSAL_MutexLock

获取互斥锁。

```c
int32_t OSAL_MutexLock(osal_id_t mutex_id, int32_t timeout);
```

**参数**：
- `mutex_id` - 互斥锁ID
- `timeout` - 超时时间（毫秒），OS_PEND表示永久等待

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERR_INVALID_ID` - 无效互斥锁ID
- `OS_SEM_TIMEOUT` - 超时（可能死锁）

**注意**：
- 内置5秒死锁检测
- 支持递归锁（同一线程可多次获取）

**示例**：
```c
int32_t ret = OSAL_MutexLock(mutex_id, 1000);
if (ret == OS_SUCCESS) {
    /* 临界区代码 */
    OSAL_MutexUnlock(mutex_id);
} else {
    LOG_ERROR("Worker", "获取锁超时");
}
```

#### OSAL_MutexUnlock

释放互斥锁。

```c
int32_t OSAL_MutexUnlock(osal_id_t mutex_id);
```

**参数**：
- `mutex_id` - 互斥锁ID

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERR_INVALID_ID` - 无效互斥锁ID

#### OSAL_MutexGetIdByName

根据名称查找互斥锁ID。

```c
int32_t OSAL_MutexGetIdByName(osal_id_t *mutex_id, const char *mutex_name);
```

**参数**：
- `mutex_id` - [输出] 互斥锁ID
- `mutex_name` - 互斥锁名称

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERR_NAME_NOT_FOUND` - 未找到互斥锁

---

### 信号量

OSAL 提供 POSIX 信号量的简化封装，支持计数信号量和超时等待。

#### OSAL_SemaphoreCreate

创建信号量。

```c
int32_t OSAL_SemaphoreCreate(osal_semaphore_t **sem, uint32_t initial_value);
```

**参数**：
- `sem` - [输出] 信号量句柄指针
- `initial_value` - 初始值（0 - INT32_MAX）

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - sem 为 NULL
- `OSAL_ERR_INVALID_SEM_VALUE` - initial_value 超出范围
- `OSAL_ERR_GENERIC` - 系统调用失败

**示例**：
```c
osal_semaphore_t *sem = NULL;
int32_t ret = OSAL_SemaphoreCreate(&sem, 1);
if (ret == OSAL_SUCCESS) {
    /* 使用信号量 */
    OSAL_SemaphoreDelete(sem);
}
```

#### OSAL_SemaphoreDelete

销毁信号量。

```c
int32_t OSAL_SemaphoreDelete(osal_semaphore_t *sem);
```

**参数**：
- `sem` - 信号量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - sem 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

#### OSAL_SemaphoreWait

等待信号量（阻塞）。

```c
int32_t OSAL_SemaphoreWait(osal_semaphore_t *sem);
```

**参数**：
- `sem` - 信号量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - sem 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

**注意**：
- 如果信号量值为 0，线程将阻塞直到其他线程调用 Post
- 成功后信号量值减 1

#### OSAL_SemaphoreTimedWait

等待信号量（超时）。

```c
int32_t OSAL_SemaphoreTimedWait(osal_semaphore_t *sem, uint32_t timeout_ms);
```

**参数**：
- `sem` - 信号量句柄
- `timeout_ms` - 超时时间（毫秒），0 表示非阻塞

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - sem 为 NULL
- `OSAL_ERR_TIMEOUT` - 超时
- `OSAL_ERR_GENERIC` - 系统调用失败

**示例**：
```c
/* 非阻塞尝试 */
if (OSAL_SemaphoreTimedWait(sem, 0) == OSAL_SUCCESS) {
    /* 获取成功 */
}

/* 超时等待 */
if (OSAL_SemaphoreTimedWait(sem, 1000) == OSAL_ERR_TIMEOUT) {
    LOG_WARN("Worker", "等待信号量超时");
}
```

#### OSAL_SemaphorePost

释放信号量。

```c
int32_t OSAL_SemaphorePost(osal_semaphore_t *sem);
```

**参数**：
- `sem` - 信号量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - sem 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

**注意**：
- 信号量值加 1
- 如果有线程在等待，将唤醒一个线程

**生产者-消费者示例**：
```c
/* 生产者 */
void producer(void *arg) {
    osal_semaphore_t *sem = (osal_semaphore_t *)arg;
    
    while (running) {
        produce_data();
        OSAL_SemaphorePost(sem);  /* 通知消费者 */
    }
}

/* 消费者 */
void consumer(void *arg) {
    osal_semaphore_t *sem = (osal_semaphore_t *)arg;
    
    while (running) {
        OSAL_SemaphoreWait(sem);  /* 等待数据 */
        consume_data();
    }
}
```

---

### 条件变量

OSAL 提供 POSIX 条件变量的简化封装，用于线程间的等待/通知机制。

#### OSAL_CondCreate

创建条件变量。

```c
int32_t OSAL_CondCreate(osal_cond_t **cond);
```

**参数**：
- `cond` - [输出] 条件变量句柄指针

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

#### OSAL_CondDelete

销毁条件变量。

```c
int32_t OSAL_CondDelete(osal_cond_t *cond);
```

**参数**：
- `cond` - 条件变量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

#### OSAL_CondWait

等待条件变量（阻塞）。

```c
int32_t OSAL_CondWait(osal_cond_t *cond, osal_mutex_t *mutex);
```

**参数**：
- `cond` - 条件变量句柄
- `mutex` - 互斥锁句柄（必须已锁定）

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 或 mutex 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

**注意**：
- 调用前必须持有 mutex
- 函数会原子地释放 mutex 并进入等待状态
- 被唤醒后会重新获取 mutex

#### OSAL_CondTimedWait

等待条件变量（超时）。

```c
int32_t OSAL_CondTimedWait(osal_cond_t *cond, osal_mutex_t *mutex, uint32_t timeout_ms);
```

**参数**：
- `cond` - 条件变量句柄
- `mutex` - 互斥锁句柄（必须已锁定）
- `timeout_ms` - 超时时间（毫秒）

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 或 mutex 为 NULL
- `OSAL_ERR_TIMEOUT` - 超时
- `OSAL_ERR_GENERIC` - 系统调用失败

#### OSAL_CondSignal

唤醒一个等待线程。

```c
int32_t OSAL_CondSignal(osal_cond_t *cond);
```

**参数**：
- `cond` - 条件变量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

**注意**：
- 如果有多个线程在等待，只唤醒一个
- 如果没有线程在等待，调用无效果

#### OSAL_CondBroadcast

唤醒所有等待线程。

```c
int32_t OSAL_CondBroadcast(osal_cond_t *cond);
```

**参数**：
- `cond` - 条件变量句柄

**返回值**：
- `OSAL_SUCCESS` - 成功
- `OSAL_ERR_INVALID_POINTER` - cond 为 NULL
- `OSAL_ERR_GENERIC` - 系统调用失败

**注意**：
- 唤醒所有等待该条件变量的线程
- 适用于多个线程等待同一条件的场景

**使用示例**：
```c
/* 共享数据 */
static int32_t data_ready = 0;
static osal_mutex_t *mutex = NULL;
static osal_cond_t *cond = NULL;

/* 生产者线程 */
void producer(void *arg) {
    OSAL_MutexLock(mutex);
    data_ready = 1;
    OSAL_CondSignal(cond);  /* 通知消费者 */
    OSAL_MutexUnlock(mutex);
}

/* 消费者线程 */
void consumer(void *arg) {
    OSAL_MutexLock(mutex);
    while (!data_ready) {
        OSAL_CondWait(cond, mutex);  /* 等待数据就绪 */
    }
    /* 处理数据 */
    data_ready = 0;
    OSAL_MutexUnlock(mutex);
}
```

**最佳实践**：
1. 始终在循环中检查条件（防止虚假唤醒）
2. 条件变量必须与互斥锁配合使用
3. 修改共享状态前必须持有互斥锁
4. Signal 用于单个等待者，Broadcast 用于多个等待者

---

### 原子操作

OSAL提供C11标准原子操作的封装。

```c
#include <stdatomic.h>

/* 原子类型 */
atomic_int counter = 0;
atomic_uint flags = 0;

/* 原子操作 */
atomic_fetch_add(&counter, 1);      /* counter++ */
atomic_fetch_sub(&counter, 1);      /* counter-- */
int value = atomic_load(&counter);  /* 读取 */
atomic_store(&counter, 10);         /* 写入 */
```

---

## SYS模块 - 系统调用

### 时钟

#### OSAL_GetLocalTime

获取本地时间。

```c
int32_t OSAL_GetLocalTime(os_time_t *time_struct);
```

**参数**：
- `time_struct` - [输出] 时间结构

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针

### 信号处理

#### OSAL_SignalRegister

注册信号处理函数。

```c
int32_t OSAL_SignalRegister(int32_t signal, osal_signal_handler_t handler);
```

**参数**：
- `signal` - 信号类型（OS_SIGNAL_INT, OS_SIGNAL_TERM, OS_SIGNAL_HUP）
- `handler` - 信号处理函数

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERROR` - 注册失败

**示例**：
```c
static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT) {
        LOG_INFO("Main", "收到Ctrl+C信号");
        g_running = false;
    }
}

OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
```

#### OSAL_SignalUnregister

注销信号处理函数。

```c
int32_t OSAL_SignalUnregister(int32_t signal);
```

**参数**：
- `signal` - 信号类型

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR` - 注销失败

### 文件操作

#### OSAL_FileOpen

打开文件。

```c
int32_t OSAL_FileOpen(int32_t *fd, const char *path, int32_t flags, int32_t mode);
```

**参数**：
- `fd` - [输出] 文件描述符
- `path` - 文件路径
- `flags` - 打开标志（OS_READ, OS_WRITE, OS_CREATE等）
- `mode` - 文件权限（八进制，如0644）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERROR` - 打开失败

#### OSAL_FileClose

关闭文件。

```c
int32_t OSAL_FileClose(int32_t fd);
```

**参数**：
- `fd` - 文件描述符

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR` - 关闭失败

#### OSAL_FileRead

读取文件。

```c
int32_t OSAL_FileRead(int32_t fd, void *buffer, uint32_t size);
```

**参数**：
- `fd` - 文件描述符
- `buffer` - [输出] 读取缓冲区
- `size` - 读取大小

**返回值**：
- 实际读取的字节数
- `OS_ERROR` - 读取失败

#### OSAL_FileWrite

写入文件。

```c
int32_t OSAL_FileWrite(int32_t fd, const void *buffer, uint32_t size);
```

**参数**：
- `fd` - 文件描述符
- `buffer` - 写入数据
- `size` - 写入大小

**返回值**：
- 实际写入的字节数
- `OS_ERROR` - 写入失败

### Select

#### OSAL_SelectSingle

单文件描述符select。

```c
int32_t OSAL_SelectSingle(int32_t fd, uint32_t timeout_ms);
```

**参数**：
- `fd` - 文件描述符
- `timeout_ms` - 超时时间（毫秒）

**返回值**：
- `OS_SUCCESS` - 有数据可读
- `OS_ERROR_TIMEOUT` - 超时
- `OS_ERROR` - 错误

### 环境变量

#### OSAL_Getenv

获取环境变量。

```c
const char* OSAL_Getenv(const char *name);
```

**参数**：
- `name` - 环境变量名

**返回值**：
- 环境变量值
- `NULL` - 未找到

#### OSAL_Setenv

设置环境变量。

```c
int32_t OSAL_Setenv(const char *name, const char *value, int32_t overwrite);
```

**参数**：
- `name` - 环境变量名
- `value` - 环境变量值
- `overwrite` - 是否覆盖（1=覆盖，0=不覆盖）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR` - 失败

---

## NET模块 - 网络

### Socket

#### OSAL_SocketOpen

创建socket。

```c
int32_t OSAL_SocketOpen(int32_t *sock_id, int32_t domain, int32_t type);
```

**参数**：
- `sock_id` - [输出] socket ID
- `domain` - 地址族（OS_SOCKET_DOMAIN_INET, OS_SOCKET_DOMAIN_INET6）
- `type` - socket类型（OS_SOCKET_STREAM, OS_SOCKET_DGRAM）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_INVALID_POINTER` - 无效指针
- `OS_ERROR` - 创建失败

#### OSAL_SocketClose

关闭socket。

```c
int32_t OSAL_SocketClose(int32_t sock_id);
```

**参数**：
- `sock_id` - socket ID

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR` - 关闭失败

#### OSAL_SocketBind

绑定socket。

```c
int32_t OSAL_SocketBind(int32_t sock_id, const os_sockaddr_t *addr);
```

**参数**：
- `sock_id` - socket ID
- `addr` - 地址结构

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR` - 绑定失败

#### OSAL_SocketConnect

连接socket。

```c
int32_t OSAL_SocketConnect(int32_t sock_id, const os_sockaddr_t *addr, int32_t timeout);
```

**参数**：
- `sock_id` - socket ID
- `addr` - 目标地址
- `timeout` - 超时时间（毫秒）

**返回值**：
- `OS_SUCCESS` - 成功
- `OS_ERROR_TIMEOUT` - 超时
- `OS_ERROR` - 连接失败

---

## LIB模块 - 标准库

### 字符串操作

```c
uint32_t OSAL_Strlen(const char *str);
int32_t OSAL_Strcmp(const char *s1, const char *s2);
char* OSAL_Strcpy(char *dest, const char *src);
char* OSAL_Strncpy(char *dest, const char *src, uint32_t n);
int32_t OSAL_Snprintf(char *str, uint32_t size, const char *format, ...);
```

### 内存管理

```c
void* OSAL_Malloc(uint32_t size);
void OSAL_Free(void *ptr);
void* OSAL_Memset(void *s, int32_t c, uint32_t n);
void* OSAL_Memcpy(void *dest, const void *src, uint32_t n);
```

### 错误处理

#### OSAL_GetErrorString

获取错误描述。

```c
const char* OSAL_GetErrorString(int32_t error_code);
```

**参数**：
- `error_code` - 错误码

**返回值**：
- 错误描述字符串

**错误码列表**（共37个）：
```c
OS_SUCCESS              =  0   /* 成功 */
OS_ERROR                = -1   /* 通用错误 */
OS_INVALID_POINTER      = -2   /* 无效指针 */
OS_ERR_INVALID_ID       = -3   /* 无效ID */
OS_ERR_NAME_TOO_LONG    = -4   /* 名称过长 */
OS_ERR_NO_FREE_IDS      = -5   /* 无可用ID */
OS_ERR_NAME_TAKEN       = -6   /* 名称已存在 */
OS_SEM_TIMEOUT          = -7   /* 信号量超时 */
OS_QUEUE_EMPTY          = -8   /* 队列为空 */
OS_QUEUE_FULL           = -9   /* 队列已满 */
OS_QUEUE_TIMEOUT        = -10  /* 队列超时 */
OS_QUEUE_INVALID_SIZE   = -11  /* 队列大小无效 */
OS_ERR_NO_MEMORY        = -12  /* 内存不足 */
/* ... 更多错误码 ... */
```

---

## UTIL模块 - 工具

### 日志系统

#### LOG宏

```c
LOG_DEBUG(module, fmt, ...)   /* DEBUG级别 */
LOG_INFO(module, fmt, ...)    /* INFO级别 */
LOG_WARN(module, fmt, ...)    /* WARN级别 */
LOG_ERROR(module, fmt, ...)   /* ERROR级别 */
LOG_FATAL(module, fmt, ...)   /* FATAL级别 */
```

**参数**：
- `module` - 模块名称（字符串）
- `fmt` - 格式化字符串
- `...` - 可变参数

**示例**：
```c
LOG_INFO("Worker", "任务启动");
LOG_ERROR("Worker", "发送失败: %d", ret);
LOG_WARN("Stats", "队列接收超时");
```

#### OSAL_Printf

简单打印（无格式）。

```c
void OSAL_Printf(const char *format, ...);
```

**参数**：
- `format` - 格式化字符串
- `...` - 可变参数

**示例**：
```c
OSAL_Printf("应用版本: %s\n", APP_VERSION);
```

### 版本信息

#### OS_GetVersionString

获取OSAL版本字符串。

```c
const char* OS_GetVersionString(void);
```

**返回值**：
- 版本字符串（如"OSAL v1.0.0"）

**示例**：
```c
OSAL_Printf("OSAL版本: %s\n", OS_GetVersionString());
```

---

## 常量定义

### 超时常量

```c
#define OS_PEND     -1   /* 永久等待 */
#define OS_CHECK     0   /* 非阻塞检查 */
```

### 资源限制

```c
#define OS_MAX_TASKS        64   /* 最大任务数 */
#define OS_MAX_QUEUES       64   /* 最大队列数 */
#define OS_MAX_MUTEXES      64   /* 最大互斥锁数 */
#define OS_MAX_API_NAME     20   /* 最大名称长度 */
```

### 信号类型

```c
#define OS_SIGNAL_INT   1   /* SIGINT (Ctrl+C) */
#define OS_SIGNAL_TERM  2   /* SIGTERM */
#define OS_SIGNAL_HUP   3   /* SIGHUP */
```

---

## 类型定义

```c
typedef char        char;      /* 字符串类型 */
typedef int8_t      int8;       /* 8位有符号整数 */
typedef uint8_t     uint8;      /* 8位无符号整数 */
typedef int16_t     int16;      /* 16位有符号整数 */
typedef uint16_t    uint16;     /* 16位无符号整数 */
typedef int32_t     int32;      /* 32位有符号整数 */
typedef uint32_t    uint32;     /* 32位无符号整数 */
typedef int64_t     int64;      /* 64位有符号整数 */
typedef uint64_t    uint64;     /* 64位无符号整数 */
typedef bool        bool;       /* 布尔类型 */
typedef uint32_t    osal_id_t;  /* OSAL对象ID */
```

---

## 相关文档

- [架构设计](ARCHITECTURE.md)
- [使用指南](USAGE_GUIDE.md)
- [模块概述](README.md)
# OSAL 数据类型使用指南

本文档详细说明 OSAL 库中各种数据类型的使用场景和最佳实践。

## 核心原则

**禁止使用 C 语言基本类型**：
- ❌ `int`, `long`, `unsigned int`, `unsigned long`
- ❌ `char` (用于字符串时)
- ✅ 使用 OSAL 定义的固定宽度类型和语义化类型

## 类型分类和使用场景

### 1. 固定宽度整数类型

用于需要明确位宽的场景（协议、硬件寄存器、二进制数据）。

```c
/* 有符号整数 */
int8_t    value8;     // 8位有符号整数 (-128 ~ 127)
int16_t   value16;    // 16位有符号整数 (-32768 ~ 32767)
int32_t   value32;    // 32位有符号整数 (-2^31 ~ 2^31-1)
int64_t   value64;    // 64位有符号整数 (-2^63 ~ 2^63-1)

/* 无符号整数 */
uint8_t   byte;       // 8位无符号整数 (0 ~ 255)
uint16_t  word;       // 16位无符号整数 (0 ~ 65535)
uint32_t  dword;      // 32位无符号整数 (0 ~ 2^32-1)
uint64_t  qword;      // 64位无符号整数 (0 ~ 2^64-1)
```

**使用场景**：
- 网络协议字段（CAN ID、IP地址、端口号）
- 硬件寄存器访问
- 二进制数据解析
- 位域操作

**示例**：
```c
/* CAN 消息 ID */
uint32_t can_id = 0x123;

/* 硬件寄存器 */
volatile uint32_t *reg = (uint32_t *)0x40000000;

/* 协议数据包 */
struct packet {
    uint16_t length;
    uint8_t  type;
    uint8_t  data[256];
};
```

### 2. 字符串类型

用于文本数据（设备名、日志消息、配置字符串）。

```c
char device_name[64];           // 设备名称
char log_message[256];          // 日志消息
const char *interface = "can0"; // 字符串常量
char parity = 'N';              // 单个字符
```

**使用场景**：
- 设备名称、文件路径
- 日志消息、错误描述
- 配置参数（"115200", "8N1"）
- 与标准 C 库交互（`strcpy`, `strlen`, `fopen`）

**重要**：
- `char` 底层是 `char`，与标准 C 库完全兼容
- 用于文本数据，区别于 `uint8_t`（二进制数据）

### 3. 平台相关大小类型

用于内存大小、数组索引、缓冲区长度等平台相关的场景。

```c
osal_size_t   buffer_size;    // 缓冲区大小（无符号）
osal_ssize_t  bytes_read;     // 读取字节数（有符号，可表示错误）
osal_uintptr_t addr;          // 指针大小的整数
osal_intptr_t  offset;        // 有符号指针偏移
osal_ptrdiff_t diff;          // 指针差值
```

**平台适配**：
- 16位平台：16位类型
- 32位平台：32位类型
- 64位平台：64位类型

**使用场景**：
- 内存分配大小：`OSAL_Malloc(osal_size_t size)`
- 数组索引：`for (osal_size_t i = 0; i < count; i++)`
- 指针运算：`osal_uintptr_t addr = OSAL_PTR_TO_UINT(ptr)`
- 返回值（可能为负）：`osal_ssize_t ret = OSAL_Read(...)`

**示例**：
```c
/* 内存分配 */
osal_size_t size = 1024;
void *buffer = OSAL_Malloc(size);

/* 指针转整数（地址计算） */
osal_uintptr_t addr = OSAL_PTR_TO_UINT(buffer);
if (OSAL_IS_ALIGNED(addr, 64)) {
    /* 地址已对齐 */
}

/* 读取操作（返回负数表示错误） */
osal_ssize_t bytes = OSAL_Read(fd, buffer, size);
if (bytes < 0) {
    /* 错误处理 */
}
```

### 4. 文件偏移类型

用于文件操作中的偏移量（支持大文件）。

```c
osal_off_t offset;  // 文件偏移量（64位，支持大文件）
```

**使用场景**：
- 文件定位：`OSAL_Lseek(fd, offset, SEEK_SET)`
- 大文件支持（>2GB）

**示例**：
```c
/* 定位到文件末尾 */
osal_off_t file_size = OSAL_Lseek(fd, 0, SEEK_END);

/* 读取大文件 */
osal_off_t offset = 5000000000LL;  // 5GB 偏移
OSAL_Lseek(fd, offset, SEEK_SET);
```

### 5. 时间类型

用于时间戳、延迟、超时等时间相关操作。

```c
osal_time_t  timestamp;   // 时间戳（秒，64位，无2038年问题）
osal_usec_t  delay_us;    // 微秒延迟
osal_nsec_t  delay_ns;    // 纳秒延迟
```

**使用场景**：
- 时间戳：`osal_time_t now = OSAL_GetTime()`
- 高精度延迟：`OSAL_DelayUsec(osal_usec_t usec)`
- 性能测量：`osal_nsec_t elapsed = end_ns - start_ns`

**示例**：
```c
/* 获取当前时间 */
osal_time_t now = OSAL_GetTime();

/* 微秒级延迟 */
osal_usec_t delay = 1000;  // 1ms
OSAL_DelayUsec(delay);

/* 性能测量 */
osal_nsec_t start = OSAL_GetTimeNs();
do_work();
osal_nsec_t end = OSAL_GetTimeNs();
osal_nsec_t elapsed = end - start;
```

### 6. 原子类型

用于多线程环境下的无锁数据结构和状态标志。

```c
osal_atomic_uint32_t counter;     // 原子计数器
osal_atomic_bool_t   flag;        // 原子布尔标志
osal_atomic_int32_t  ref_count;   // 引用计数
```

**使用场景**：
- 引用计数
- 状态标志（启动/停止）
- 无锁队列/栈
- 统计计数器

**示例**：
```c
/* 原子计数器 */
osal_atomic_uint32_t request_count = 0;

void handle_request(void) {
    OSAL_ATOMIC_INC(&request_count);
    /* 处理请求 */
}

/* 原子标志 */
osal_atomic_bool_t shutdown_flag = false;

void task_entry(void *arg) {
    while (!OSAL_ATOMIC_LOAD(&shutdown_flag)) {
        do_work();
    }
}

void shutdown(void) {
    OSAL_ATOMIC_STORE(&shutdown_flag, true);
}

/* CAS 操作 */
uint32_t expected = 0;
uint32_t desired = 1;
if (OSAL_ATOMIC_CAS(&counter, &expected, desired)) {
    /* 成功：counter 从 0 变为 1 */
}
```

### 7. OSAL 对象 ID 类型

用于 OSAL 对象的唯一标识符。

```c
osal_id_t task_id;    // 任务 ID
osal_id_t queue_id;   // 队列 ID
osal_id_t mutex_id;   // 互斥锁 ID
osal_id_t timer_id;   // 定时器 ID
```

**使用场景**：
- 任务创建：`OSAL_TaskCreate(&task_id, ...)`
- 队列操作：`OSAL_QueueGet(queue_id, ...)`
- 互斥锁：`OSAL_MutexLock(mutex_id)`

**示例**：
```c
osal_id_t task_id;
int32_t ret = OSAL_TaskCreate(&task_id, "worker", task_entry, NULL);
if (ret != OSAL_SUCCESS) {
    /* 错误处理 */
}
```

### 8. 布尔类型

用于逻辑判断和标志位。

```c
bool is_ready;        // 状态标志
bool has_error;       // 错误标志
```

**使用场景**：
- 状态标志
- 函数返回值（成功/失败）
- 条件判断

**示例**：
```c
bool is_initialized = false;

int32_t init(void) {
    if (is_initialized) {
        return OSAL_ERR_ALREADY_EXISTS;
    }
    /* 初始化代码 */
    is_initialized = true;
    return OSAL_SUCCESS;
}
```

## 类型转换辅助宏

### 指针与整数转换

```c
/* 指针转整数 */
void *ptr = get_buffer();
osal_uintptr_t addr = OSAL_PTR_TO_UINT(ptr);

/* 整数转指针 */
osal_uintptr_t reg_addr = 0x40000000;
volatile uint32_t *reg = OSAL_UINT_TO_PTR(reg_addr);
```

### 对齐操作

```c
/* 向上对齐到64字节边界 */
osal_size_t size = 100;
osal_size_t aligned_size = OSAL_ALIGN_UP(size, 64);  // 128

/* 向下对齐 */
osal_uintptr_t addr = 0x1234;
osal_uintptr_t aligned_addr = OSAL_ALIGN_DOWN(addr, 16);  // 0x1230

/* 检查是否对齐 */
if (OSAL_IS_ALIGNED(ptr, 64)) {
    /* 指针已对齐到64字节边界 */
}
```

### 数组和结构体操作

```c
/* 数组元素个数 */
uint32_t array[10];
osal_size_t count = OSAL_ARRAY_SIZE(array);  // 10

/* 结构体成员偏移 */
struct config {
    uint32_t id;
    char name[32];
};
osal_size_t offset = OSAL_OFFSETOF(struct config, name);

/* 通过成员指针获取结构体指针 */
struct config cfg;
char *name_ptr = cfg.name;
struct config *cfg_ptr = OSAL_CONTAINER_OF(name_ptr, struct config, name);
```

### 位操作

```c
uint32_t flags = 0;

/* 设置位 */
OSAL_BIT_SET(flags, 3);      // flags |= (1 << 3)

/* 清除位 */
OSAL_BIT_CLR(flags, 3);      // flags &= ~(1 << 3)

/* 测试位 */
if (OSAL_BIT_TEST(flags, 3)) {
    /* 位3已设置 */
}

/* 翻转位 */
OSAL_BIT_TOGGLE(flags, 3);   // flags ^= (1 << 3)
```

## 字节序转换

用于网络协议和跨平台数据交换。

```c
/* 主机序 -> 网络序（大端） */
uint16_t port_host = 8080;
uint16_t port_net = OSAL_HTONS(port_host);

uint32_t ip_host = 0xC0A80001;  // 192.168.0.1
uint32_t ip_net = OSAL_HTONL(ip_host);

/* 网络序 -> 主机序 */
uint16_t port = OSAL_NTOHS(port_net);
uint32_t ip = OSAL_NTOHL(ip_net);

/* 64位转换 */
uint64_t value64 = 0x123456789ABCDEF0;
uint64_t value64_net = OSAL_HTONLL(value64);
```

## 对齐和打包

用于硬件寄存器、DMA 缓冲区、网络协议等。

```c
/* 对齐到64字节边界（缓存行大小） */
struct dma_buffer {
    uint8_t data[1024];
} OSAL_ALIGNED(64);

/* 紧凑结构体（无填充） */
struct OSAL_PACKED protocol_header {
    uint16_t length;
    uint8_t  type;
    uint8_t  flags;
};

/* 避免伪共享（多线程） */
struct thread_data {
    osal_atomic_uint32_t counter;
    uint8_t padding[OSAL_CACHE_LINE_SIZE - sizeof(osal_atomic_uint32_t)];
} OSAL_ALIGNED(OSAL_CACHE_LINE_SIZE);
```

## 返回值类型

所有 OSAL 函数返回 `int32_t` 状态码。

```c
int32_t ret;

/* 成功 */
ret = OSAL_TaskCreate(...);
if (ret == OSAL_SUCCESS) {
    /* 成功 */
}

/* 错误处理 */
ret = OSAL_QueueGet(queue_id, buffer, size, timeout);
if (ret != OSAL_SUCCESS) {
    switch (ret) {
        case OSAL_ERR_TIMEOUT:
            /* 超时 */
            break;
        case OSAL_ERR_INVALID_ID:
            /* 无效ID */
            break;
        default:
            /* 其他错误 */
            break;
    }
}
```

## 常见错误和最佳实践

### ❌ 错误示例

```c
/* 错误1：使用基本类型 */
int count;                    // 应使用 int32_t 或 osal_size_t
unsigned int flags;           // 应使用 uint32_t
long offset;                  // 应使用 int32_t/int64_t/osal_off_t

/* 错误2：字符串使用 uint8_t */
uint8_t name[32];             // 应使用 char
strcpy(name, "device");       // 类型不匹配

/* 错误3：指针转整数不安全 */
int addr = (int)ptr;          // 64位平台会截断

/* 错误4：硬编码对齐 */
if ((int)ptr % 64 == 0) {     // 应使用 OSAL_IS_ALIGNED
}
```

### ✅ 正确示例

```c
/* 正确1：使用固定宽度类型 */
int32_t count;
uint32_t flags;
osal_off_t offset;

/* 正确2：字符串使用 char */
char name[32];
OSAL_Strcpy(name, "device");

/* 正确3：安全的指针转换 */
osal_uintptr_t addr = OSAL_PTR_TO_UINT(ptr);

/* 正确4：使用对齐宏 */
if (OSAL_IS_ALIGNED(ptr, 64)) {
    /* 已对齐 */
}
```

## 平台兼容性矩阵

| 类型 | 16位平台 | 32位平台 | 64位平台 | 用途 |
|------|---------|---------|---------|------|
| `int8_t` | 1字节 | 1字节 | 1字节 | 固定宽度 |
| `int16_t` | 2字节 | 2字节 | 2字节 | 固定宽度 |
| `int32_t` | 4字节 | 4字节 | 4字节 | 固定宽度 |
| `int64_t` | 8字节 | 8字节 | 8字节 | 固定宽度 |
| `osal_size_t` | 2字节 | 4字节 | 8字节 | 平台相关 |
| `osal_uintptr_t` | 2字节 | 4字节 | 8字节 | 指针大小 |
| `osal_off_t` | 4字节 | 8字节 | 8字节 | 文件偏移 |
| `osal_time_t` | 8字节 | 8字节 | 8字节 | 时间戳 |

## 总结

1. **永远不要使用 C 基本类型**（`int`, `long`, `unsigned`）
2. **固定宽度类型**用于协议、硬件、二进制数据
3. **平台相关类型**用于内存大小、数组索引
4. **char** 用于文本，**uint8_t** 用于二进制
5. **原子类型**用于多线程无锁编程
6. **使用辅助宏**进行类型转换和对齐操作
7. **编译时断言**确保类型大小正确

遵循这些规则，可以确保代码在 16/32/64 位平台、Linux/RTOS 之间完全兼容。
