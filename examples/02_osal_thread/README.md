# OSAL 线程示例

演示如何使用 OSAL 创建线程、互斥锁和信号量。

---

## 📖 学习目标

- 学习如何创建和管理线程
- 学习如何使用互斥锁保护共享资源
- 学习如何使用信号量进行线程同步
- 理解多线程编程的基本概念

---

## 🚀 快速开始

### 编译运行

```bash
python3 build.py config sample_default
python3 build.py build
./_build/bin/example_osal_thread
```

### 预期输出

```
=================================
  OSAL Thread Example
=================================

Creating threads...
Thread 1: Count 0
Thread 2: Count 0
Thread 1: Count 1
Thread 2: Count 1
Thread 1: Count 2
Thread 2: Count 2
...
Threads finished.
Counter value: 10
```

---

## 📝 代码说明

### 创建线程

```c
// 线程函数
static void* thread_func(void *arg)
{
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < 5; i++) {
        printf("Thread %d: Count %d\n", thread_id, i);
        OSAL_Sleep(100);  // 休眠 100ms
    }
    
    return NULL;
}

// 主函数
int main(void)
{
    osal_thread_t *thread1 = NULL;
    osal_thread_t *thread2 = NULL;
    
    int id1 = 1, id2 = 2;
    
    // 创建线程
    OSAL_ThreadCreate(&thread1, thread_func, &id1);
    OSAL_ThreadCreate(&thread2, thread_func, &id2);
    
    // 等待线程结束
    OSAL_ThreadJoin(thread1);
    OSAL_ThreadJoin(thread2);
    
    return 0;
}
```

### 使用互斥锁

```c
static osal_mutex_t *mutex = NULL;
static int counter = 0;

static void* thread_with_mutex(void *arg)
{
    for (int i = 0; i < 1000; i++) {
        // 加锁
        OSAL_MutexLock(mutex);
        counter++;
        OSAL_MutexUnlock(mutex);
    }
    return NULL;
}

int main(void)
{
    // 创建互斥锁
    OSAL_MutexCreate(&mutex);
    
    // 创建线程...
    
    // 销毁互斥锁
    OSAL_MutexDestroy(mutex);
    return 0;
}
```

### 使用信号量

```c
static osal_semaphore_t *sem = NULL;

static void* producer(void *arg)
{
    for (int i = 0; i < 5; i++) {
        printf("Producing item %d\n", i);
        OSAL_SemaphorePost(sem);  // 发送信号
        OSAL_Sleep(100);
    }
    return NULL;
}

static void* consumer(void *arg)
{
    for (int i = 0; i < 5; i++) {
        OSAL_SemaphoreWait(sem);  // 等待信号
        printf("Consuming item %d\n", i);
    }
    return NULL;
}
```

---

## 🔧 实验任务

### 任务 1: 修改线程数量

尝试创建 4 个线程，观察输出。

### 任务 2: 测试互斥锁

注释掉互斥锁代码，观察 counter 的最终值是否正确。

### 任务 3: 生产者-消费者

实现一个完整的生产者-消费者模型。

---

## 📚 相关 API

### 线程 API

- `OSAL_ThreadCreate()` - 创建线程
- `OSAL_ThreadJoin()` - 等待线程结束
- `OSAL_Sleep()` - 休眠

### 互斥锁 API

- `OSAL_MutexCreate()` - 创建互斥锁
- `OSAL_MutexLock()` - 加锁
- `OSAL_MutexUnlock()` - 解锁
- `OSAL_MutexDestroy()` - 销毁互斥锁

### 信号量 API

- `OSAL_SemaphoreCreate()` - 创建信号量
- `OSAL_SemaphoreWait()` - 等待信号
- `OSAL_SemaphorePost()` - 发送信号
- `OSAL_SemaphoreDestroy()` - 销毁信号量

---

## 💡 注意事项

1. **资源清理**: 始终销毁创建的线程、互斥锁和信号量
2. **死锁**: 注意避免死锁（多个线程互相等待）
3. **竞态条件**: 使用互斥锁保护共享资源

---

## 📚 下一步

- [03_hal_can](../03_hal_can/) - 学习 CAN 总线通信
- [04_hal_uart](../04_hal_uart/) - 学习串口通信

---

**难度**: ⭐⭐ (简单)  
**时间**: 15 分钟
