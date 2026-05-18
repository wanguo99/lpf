# Sample应用

这是一个最小化的示例应用，用于展示如何正确使用EMS各层接口。

## 功能说明

### 架构设计

```
┌─────────────────────────────────────────┐
│  Main线程                                │
│  - 初始化OSAL                            │
│  - 创建队列和任务                        │
│  - 等待退出信号                          │
│  - 清理资源                              │
└─────────────────────────────────────────┘
         │
         ├──> Worker任务
         │    - 每秒生成一条消息
         │    - 发送到消息队列
         │    - 更新原子计数器
         │
         └──> Stats任务
              - 从队列接收消息
              - 定期打印统计信息

消息队列: Worker → Stats
```

### 演示的OSAL接口

**初始化和关闭**
- `OS_API_Init()` - 初始化OSAL
- `OS_API_Teardown()` - 关闭OSAL

**任务管理**
- `OSAL_TaskCreate()` - 创建任务
- `OSAL_TaskDelete()` - 删除任务
- `OSAL_TaskShouldShutdown()` - 检查退出标志
- `OSAL_TaskDelay()` - 任务延时

**消息队列**
- `OSAL_QueueCreate()` - 创建队列
- `OSAL_QueuePut()` - 发送消息（带超时）
- `OSAL_QueueGet()` - 接收消息（带超时）
- `OSAL_QueueDelete()` - 删除队列

**信号处理**
- `OSAL_SignalRegister()` - 注册信号处理函数
- 优雅退出机制

**日志系统**
- `OSAL_INFO()` - 信息日志
- `OSAL_WARN()` - 警告日志
- `OSAL_ERROR()` - 错误日志
- `OSAL_Printf()` - 简单打印

**原子操作**
- `atomic_uint` - 原子计数器
- `atomic_fetch_add()` - 原子递增
- `atomic_load()` - 原子读取

## 编译和运行

### 编译

```bash
# 编译整个项目
./build.sh

# 或仅编译sample_app
cd build
make sample_app
cd ..
```

### 运行

```bash
./build/bin/sample_app
```

### 预期输出

```
========================================
  Sample应用 v1.0.0
  OSAL版本: 1.0.0
========================================

[Main] OSAL初始化成功
[Main] 信号处理注册成功
[Main] 消息队列创建成功 (深度: 10, 消息大小: 64字节)
[Main] 工作任务创建成功
[Main] 统计任务创建成功

应用启动成功！按Ctrl+C退出。

[Worker] 工作任务启动
[Stats] 统计任务启动
[Worker] 发送消息: Message #0
[Stats] 接收消息: Message #0 (大小: 64字节)
[Worker] 发送消息: Message #1
[Stats] 接收消息: Message #1 (大小: 64字节)
...

========== 应用统计 ==========
已处理消息数: 5
==============================

^C
[Main] 收到退出信号，正在关闭应用...
[Main] 开始清理资源...
[Worker] 工作任务退出
[Stats] 统计任务退出
[Main] 工作任务已删除
[Main] 统计任务已删除
[Main] 消息队列已删除
[Main] OSAL已关闭

应用已退出。
```

## 开发指南

### 添加新功能

1. **添加HAL层硬件访问**
   ```c
   #include "hal_can.h"
   
   hal_can_handle_t can_handle;
   ret = HAL_CAN_Init(&can_config, &can_handle);
   ```

2. **添加PDL层外设服务**
   ```c
   #include "pdl_satellite.h"
   
   ret = SatellitePDL_Init(&sat_config);
   ```

3. **添加互斥锁保护**
   ```c
   osal_id_t mutex_id;
   OSAL_MutexCreate(&mutex_id, "MyMutex", 0);
   OSAL_MutexLock(mutex_id, 1000);
   // 临界区代码
   OSAL_MutexUnlock(mutex_id);
   ```

### 编码规范

1. **使用OSAL类型**
   - `char` - 字符串
   - `int32`, `uint32` - 固定大小整数
   - `bool` - 布尔类型

2. **错误处理**
   - 所有函数返回值必须检查
   - 使用 `OS_SUCCESS` 和 `OS_ERROR`

3. **任务编写**
   - 循环中检查 `OSAL_TaskShouldShutdown()`
   - 退出前清理资源

4. **日志规范**
   - 使用 `OSAL_INFO/WARN/ERROR` 宏
   - 不要直接使用 `printf`

## 参考文档

- [OSAL接口文档](../../osal/README.md)
- [HAL接口文档](../../hal/README.md)
- [PDL接口文档](../../pdl/README.md)
- [编码规范](../../docs/CODING_STANDARDS.md)
