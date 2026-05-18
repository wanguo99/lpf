# Apps 架构设计

## 应用层设计原则

### 1. 平台无关性

应用层必须保持完全平台无关：
- ✅ 使用OSAL接口（`OSAL_TaskCreate()`, `OSAL_Printf()`）
- ✅ 使用HAL接口（`HAL_CAN_Send()`, `HAL_Serial_Send()`）
- ✅ 使用PDL接口（`SatellitePDL_Init()`, `BMCPDL_PowerOn()`）
- ❌ 禁止包含系统头文件（`<unistd.h>`, `<pthread.h>`）
- ❌ 禁止直接调用系统API

### 2. 应用程序结构

```
main()
  ↓
初始化OSAL（无需显式调用）
  ↓
注册信号处理
  ↓
创建资源（队列、互斥锁）
  ↓
创建任务
  ↓
主循环（等待退出信号）
  ↓
清理资源
  ↓
退出
```

### 3. 任务模型

**推荐的任务结构**：
```c
static void worker_task(void *arg)
{
    /* 初始化 */
    LOG_INFO("Worker", "任务启动");
    
    /* 主循环 */
    while (!OSAL_TaskShouldShutdown())
    {
        /* 执行工作 */
        do_work();
        
        /* 延时 */
        OSAL_TaskDelay(interval_ms);
    }
    
    /* 清理资源 */
    cleanup();
    
    LOG_INFO("Worker", "任务退出");
}
```

### 4. 信号处理

**优雅退出机制**：
```c
static volatile bool g_running = true;

static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT || sig == OS_SIGNAL_TERM) {
        LOG_INFO("Main", "收到退出信号");
        g_running = false;
    }
}

int main(void)
{
    /* 注册信号 */
    OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
    OSAL_SignalRegister(OS_SIGNAL_TERM, signal_handler);
    
    /* 主循环 */
    while (g_running) {
        OSAL_TaskDelay(1000);
    }
    
    /* 清理 */
    cleanup_resources();
    
    return 0;
}
```

## 应用程序模板

### 最小应用模板

```c
#include "osal.h"

int main(void)
{
    LOG_INFO("Main", "应用启动");
    
    /* 应用逻辑 */
    
    LOG_INFO("Main", "应用退出");
    return 0;
}
```

### 多任务应用模板

```c
#include "osal.h"

static volatile bool g_running = true;
static osal_id_t g_task_id;
static osal_id_t g_queue_id;

static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT) {
        g_running = false;
    }
}

static void worker_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown()) {
        /* 工作逻辑 */
        OSAL_TaskDelay(1000);
    }
}

int main(void)
{
    /* 注册信号 */
    OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
    
    /* 创建队列 */
    OSAL_QueueCreate(&g_queue_id, "Queue", 10, 64, 0);
    
    /* 创建任务 */
    OSAL_TaskCreate(&g_task_id, "Worker", worker_task, NULL,
                    16*1024, 100, 0);
    
    /* 主循环 */
    while (g_running) {
        OSAL_TaskDelay(1000);
    }
    
    /* 清理 */
    OSAL_TaskDelete(g_task_id);
    OSAL_QueueDelete(g_queue_id);
    
    return 0;
}
```

## CMakeLists.txt 配置

```cmake
# 应用程序
add_executable(my_app
    src/main.c
)

# 链接库
target_link_libraries(my_app
    osal
    hal
    pdl
    pcl
)

# 包含目录
target_include_directories(my_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

## 相关文档

- [使用指南](USAGE_GUIDE.md)
- [sample_app示例](../sample_app/README.md)
