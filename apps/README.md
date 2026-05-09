# Apps (应用层)

## 模块概述

Apps层包含基于EMS构建的应用程序，展示如何使用OSAL、HAL、PDL等抽象层。

**设计理念**：
- 完全平台无关，只使用抽象层接口
- 模块化应用设计，便于扩展和维护
- 每个应用独立目录，独立编译

**当前应用**：
- `sample_app`：示例应用，展示OSAL基本用法

## 应用列表

### sample_app - 示例应用
最小化示例应用，展示OSAL基本用法：
- 任务创建和管理
- 消息队列通信
- 信号处理和优雅退出
- 日志系统使用

详细文档：[sample_app/README.md](sample_app/README.md)

## 编译说明

### 快速开始

```bash
# 在项目根目录编译所有应用
./build.sh              # Release模式
./build.sh -d           # Debug模式
```

### 单独编译应用

```bash
# 方法1: 使用CMake直接编译
mkdir -p build && cd build
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make sample_app -j$(nproc)
cd ../..

# 方法2: 在已配置的构建目录中编译
cd build
make sample_app -j$(nproc)
cd ../..
```

### 支持的编译参数

#### CMake配置参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `CMAKE_BUILD_TYPE` | STRING | Release | 编译类型：Release/Debug |
| `PLATFORM` | STRING | native | 目标平台：native/generic-linux |

#### 编译类型说明

**Release模式**（默认）：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Release
```
- 优化级别：`-O2`
- 无调试信息
- 适用于生产环境

**Debug模式**：
```bash
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
```
- 优化级别：`-O0`
- 包含调试信息：`-g`
- 适用于开发调试

### 配置编译参数

#### 示例1: Debug模式编译

```bash
cd build
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make sample_app -j$(nproc)
cd ../..
```

#### 示例2: 交叉编译ARM平台

```bash
cd build
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=generic-linux \
    -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc
make sample_app -j$(nproc)
cd ../..
```

### 编译输出

```
output/
└── target/
    └── bin/
        └── sample_app         # 示例应用
```

### 常用编译命令

```bash
# 完整编译流程
./build.sh -d                   # Debug模式编译所有

# 仅编译sample_app
cd build && make sample_app -j$(nproc) && cd ../..

# 清理并重新编译
./build.sh -c && ./build.sh

# 查看编译日志
cat build.log | grep -A 5 "Sample application"
```

## 应用结构

```
apps/
├── sample_app/                 # 示例应用
│   ├── README.md               # 应用说明
│   ├── CMakeLists.txt          # 编译配置
│   └── src/
│       └── main.c              # 主程序
└── docs/                       # 应用层文档
```

## 运行应用

### 运行sample_app

```bash
# 直接运行
./build/bin/sample_app

# 使用GDB调试
gdb ./build/bin/sample_app
(gdb) run
```

## 添加新应用

### 步骤1: 创建应用目录

```bash
mkdir -p apps/my_app/src
```

### 步骤2: 创建CMakeLists.txt

创建 `apps/my_app/CMakeLists.txt`：
```cmake
# My Application

# 源文件
set(MY_APP_SOURCES
    src/main.c
)

# 创建可执行文件
add_executable(my_app ${MY_APP_SOURCES})

# 链接依赖
target_link_libraries(my_app
    ems::osal       # OSAL层
    ems::hal        # HAL层（可选）
    ems::pdl        # PDL层（可选）
    Threads::Threads
)

# 设置输出目录
set_target_properties(my_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_TARGET}/bin
)

# 安装规则
install(TARGETS my_app
    RUNTIME DESTINATION bin
)

message(STATUS "My application configured")
```

### 步骤3: 创建主程序

创建 `apps/my_app/src/main.c`：
```c
#include <osal.h>

int main(void)
{
    /* 初始化日志 */
    LOG_INFO("MY_APP", "Application starting...");
    
    /* 应用逻辑 */
    // TODO: 实现应用功能
    
    LOG_INFO("MY_APP", "Application finished");
    return 0;
}
```

### 步骤4: 注册到Apps层

编辑 `apps/CMakeLists.txt`：
```cmake
# Apps层 - 应用程序

# 添加示例应用
add_subdirectory(sample_app)

# 添加新应用
add_subdirectory(my_app)
```

### 步骤5: 编译并运行

```bash
./build.sh -d
./build/bin/my_app
```

## 应用开发指南

### 使用OSAL接口

```c
#include <osal.h>

/* 任务管理 */
osal_id_t task_id;
OSAL_TaskCreate(&task_id, "MyTask", task_func, NULL, 8192, 100, 0);

/* 队列通信 */
osal_id_t queue_id;
OSAL_QueueCreate(&queue_id, "MyQueue", 10, sizeof(msg_t), 0);

/* 互斥锁 */
osal_id_t mutex_id;
OSAL_MutexCreate(&mutex_id, "MyMutex", 0);

/* 日志输出 */
LOG_INFO("APP", "Message: %s", msg);
LOG_ERROR("APP", "Error: %d", error_code);
```

### 使用HAL接口

```c
#include <hal_can.h>
#include <hal_serial.h>

/* CAN通信 */
hal_can_config_t can_cfg = {
    .interface = "can0",
    .baudrate = 500000
};
hal_can_handle_t can_handle;
HAL_CAN_Init(&can_cfg, &can_handle);

/* 串口通信 */
hal_serial_config_t uart_cfg = {
    .device = "/dev/ttyS0",
    .baudrate = 115200
};
hal_serial_handle_t uart_handle;
HAL_Serial_Init(&uart_cfg, &uart_handle);
```

### 使用PDL接口

```c
#include <pdl_mcu.h>
#include <pdl_satellite.h>

/* MCU外设 */
pdl_mcu_config_t mcu_cfg = {
    .name = "mcu0",
    .interface_type = PDL_INTERFACE_CAN
};
pdl_mcu_handle_t mcu_handle;
PDL_MCU_Init(&mcu_cfg, &mcu_handle);

/* 卫星接口 */
pdl_satellite_config_t sat_cfg = {
    .can_interface = "can1"
};
pdl_satellite_handle_t sat_handle;
PDL_Satellite_Init(&sat_cfg, &sat_handle);
```

## 编码规范（重要）

**必须遵守**：
- ✅ 使用抽象层接口：OSAL/HAL/PDL
- ❌ 禁止直接调用系统调用：`socket()`, `pthread_create()`
- ✅ 使用OSAL日志：`LOG_INFO()`, `LOG_ERROR()`
- ❌ 禁止使用：`printf()`, `fprintf()`
- ✅ 使用OSAL类型：`int32`, `uint32`, `char`
- ❌ 禁止使用：`int`, `unsigned int`, `char`

**示例**：
```c
/* ✅ 正确 */
#include <osal.h>

int main(void)
{
    osal_id_t task_id;
    int32 ret = OSAL_TaskCreate(&task_id, "MyTask", task_func, NULL, 
                                 8192, OSAL_PRIORITY_C(100), 0);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("APP", "Task create failed: %d", ret);
        return -1;
    }
    
    LOG_INFO("APP", "Application started");
    OSAL_TaskDelay(1000);
    
    return 0;
}

/* ❌ 错误 */
#include <pthread.h>
#include <stdio.h>

int main(void)
{
    pthread_t thread;  // 禁止
    int ret = pthread_create(&thread, NULL, task_func, NULL);  // 禁止
    if (ret != 0) {
        printf("Task create failed\n");  // 禁止
        return -1;
    }
    
    sleep(1);  // 禁止
    return 0;
}
```

## 应用配置

### 使用PCL配置

```c
#include <pcl.h>

int main(void)
{
    /* 初始化PCL */
    PCL_Init();
    
    /* 获取板级配置 */
    const pcl_board_t *board = PCL_GetBoardConfig();
    LOG_INFO("APP", "Board: %s", board->name);
    
    /* 获取MCU配置 */
    const pcl_mcu_t *mcu = PCL_GetMCUConfig("mcu0");
    if (mcu != NULL) {
        /* 使用配置初始化PDL */
        pdl_mcu_config_t pdl_cfg = {
            .name = mcu->name,
            .interface_type = mcu->interface_type
        };
        PDL_MCU_Init(&pdl_cfg, &handle);
    }
    
    return 0;
}
```

## 调试应用

### 使用GDB调试

```bash
# 编译Debug版本
./build.sh -d

# 启动GDB
gdb ./build/bin/sample_app

# GDB命令
(gdb) break main          # 设置断点
(gdb) run                 # 运行
(gdb) next                # 单步执行
(gdb) print variable      # 打印变量
(gdb) bt                  # 查看调用栈
(gdb) continue            # 继续执行
```

### 启用日志调试

```c
/* 设置日志级别 */
OSAL_LogSetLevel(OSAL_LOG_DEBUG);

/* 输出调试日志 */
LOG_DEBUG("APP", "Debug info: %d", value);
```

## 常见问题

**Q: 应用可以直接调用系统API吗？**
```c
/* ❌ 禁止 - 应用必须保持平台无关 */
int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);  // 禁止

/* ✅ 正确 - 使用HAL层接口 */
HAL_CAN_Init(&can_cfg, &handle);  // 正确
```

**Q: 如何处理应用错误？**
```c
int32 ret = OSAL_TaskCreate(&task_id, "MyTask", task_func, NULL, 
                             8192, 100, 0);
if (ret != OS_SUCCESS) {
    LOG_ERROR("APP", "Task create failed: %d", ret);
    return -1;  // 返回错误码
}
```

**Q: 应用如何优雅退出？**
```c
static volatile bool g_running = true;

void signal_handler(int sig)
{
    g_running = false;
}

int main(void)
{
    /* 注册信号处理 */
    OSAL_SignalRegister(OSAL_SIGINT, signal_handler);
    
    /* 主循环 */
    while (g_running) {
        /* 应用逻辑 */
        OSAL_TaskDelay(100);
    }
    
    /* 清理资源 */
    cleanup();
    
    LOG_INFO("APP", "Application exited");
    return 0;
}
```

**Q: 如何在应用中使用多线程？**
```c
static void worker_task(void *arg)
{
    while (!OSAL_TaskShouldShutdown()) {
        /* 任务逻辑 */
        OSAL_TaskDelay(100);
    }
}

int main(void)
{
    /* 创建工作任务 */
    osal_id_t task_id;
    OSAL_TaskCreate(&task_id, "Worker", worker_task, NULL, 
                    8192, 100, 0);
    
    /* 主线程逻辑 */
    OSAL_TaskDelay(5000);
    
    /* 等待任务退出 */
    OSAL_TaskDelete(task_id);
    
    return 0;
}
```

## 设计原则

1. **平台无关**：应用层必须保持完全平台无关
2. **使用抽象接口**：通过OSAL/HAL/PDL访问底层
3. **优雅退出**：支持信号处理和资源清理
4. **错误处理**：检查所有返回值

## 参考文档

- [sample_app文档](sample_app/README.md)
- [应用层架构设计](docs/ARCHITECTURE.md)
- [使用指南](docs/USAGE_GUIDE.md)
- [OSAL文档](../osal/README.md)
- [HAL文档](../hal/README.md)
- [PDL文档](../pdl/README.md)
- [编码规范](../docs/CODING_STANDARDS.md)
