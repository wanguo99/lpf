# EMS 架构设计文档

## 1. 系统概述

### 1.1 项目定位

**EMS** (Embedded Middleware System) 是通用嵌入式中间件系统，为嵌入式系统提供硬件抽象和外设管理服务。

**设计目标**：
- 高可靠性：工业级可靠性要求，支持故障检测和自动恢复
- 实时性：CAN消息延迟 < 10ms，命令处理时间 < 100ms
- 可移植性：分层架构，支持跨平台移植（Linux/RTOS）
- 可维护性：模块化设计，配置独立，便于多人协作

### 1.2 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    主机系统 (Host)                           │
│              CAN总线 (500Kbps, ID: 0x100/0x200)             │
└──────────────────────────┬──────────────────────────────────┘
                           │ CAN通信
┌──────────────────────────▼──────────────────────────────────┐
│                  EMS 控制板                                  │
│  ┌────────────────────────────────────────────────────────┐ │
│  │           OSAL运行环境 (所有层使用OSAL接口)             │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  Apps层 (应用层)                                        │ │
│  │  ├── can_gateway        - CAN网关应用                  │ │
│  │  └── protocol_converter - 协议转换应用                 │ │
│  │  └── 读取ACL配置                                        │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  PDL层 (外设驱动层 - Peripheral Driver Layer)          │ │
│  │  ├── pdl_satellite      - 主机接口服务                 │ │
│  │  ├── pdl_bmc            - BMC设备服务                  │ │
│  │  └── pdl_mcu            - MCU外设服务                  │ │
│  │  └── 读取PCL配置                                        │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  HAL层 (硬件抽象层)                                     │ │
│  │  ├── hal_can            - CAN驱动 (SocketCAN)         │ │
│  │  └── hal_serial         - 串口驱动 (termios)          │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  配置库 (不参与调用链)                                  │ │
│  │  ├── ACL - 应用配置层 (Apps读取)                       │ │
│  │  └── PCL - 外设配置库 (PDL读取)                        │ │
│  └────────────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────────────┘
                           │ 以太网/UART
┌──────────────────────────▼──────────────────────────────────┐
│              外部设备 (BMC/Linux Device)                     │
│         IPMI/Redfish (以太网:623 / UART:115200)             │
└─────────────────────────────────────────────────────────────┘

调用链：Apps → PDL → HAL
运行环境：所有层使用OSAL接口
配置库：ACL(Apps读取)、PCL(PDL读取)
```

## 2. 分层架构设计

### 2.1 OSAL层 (操作系统抽象层)

**职责**：封装操作系统API，提供统一的跨平台接口

**设计理念**：跨平台操作系统抽象层，支持多种操作系统移植

**核心模块**：

#### 2.1.1 任务管理（osal_task）

**文件**：`osal_task.h`, `osal_task.c`, `config/task_config.h`

**功能**：基于pthread的任务管理，支持优雅退出

**关键接口**：
- `OSAL_TaskCreate()` - 创建任务，支持栈大小和优先级配置
- `OSAL_TaskDelete()` - 优雅删除任务（5秒超时）
- `OSAL_TaskShouldShutdown()` - 任务检查退出标志
- `OSAL_TaskDelay()` - 任务延时（毫秒级）
- `OSAL_TaskGetId()` - 获取当前任务ID

**实现细节**：
1. **优雅退出机制**（核心特性）：
   - 使用 `shutdown_requested` 标志而非 `pthread_cancel`（避免死锁）
   - 任务循环必须调用 `OSAL_TaskShouldShutdown()` 检查退出
   - 删除时使用 `pthread_timedjoin_np` 等待5秒
   - 超时后使用 `pthread_detach` 而非强制取消

2. **任务表管理**：
   - 静态数组 `OS_task_table[64]`，支持最多64个任务
   - 使用 `pthread_mutex` 保护任务表
   - 任务状态：READY → RUNNING → TERMINATED

3. **引用计数**：
   - 使用 `atomic_int ref_count` 防止任务删除时的竞态条件
   - 初始引用计数为1

4. **优先级映射**：
   - 配置文件定义：CRITICAL(10) → HIGH(50) → NORMAL(100) → LOW(150) → IDLE(200)
   - Linux用户空间优先级调整需要特权，仅记录不强制

**配置**（`task_config.h`）：
- 栈大小：SMALL(32KB), MEDIUM(64KB), LARGE(128KB)
- 优先级：数字越小优先级越高

#### 2.1.2 消息队列（osal_queue）

**文件**：`osal_queue.h`, `osal_queue.c`, `config/queue_config.h`

**功能**：环形缓冲区实现的消息队列，支持阻塞/非阻塞操作

**关键接口**：
- `OSAL_QueueCreate()` - 创建队列
- `OSAL_QueueDelete()` - 删除队列
- `OSAL_QueuePut()` - 发送消息（阻塞）
- `OSAL_QueueGet()` - 接收消息（支持超时）
- `OSAL_QueueGetIdByName()` - 根据名称查找队列

**实现细节**：
1. **环形缓冲区**：
   - 使用 `head`, `tail`, `count` 管理队列
   - 固定大小消息（创建时指定）
   - 深度可配置（默认50）

2. **引用计数机制**（关键优化）：
   - 使用 `atomic_int ref_count` 保护队列对象
   - `queue_acquire()` 增加引用计数
   - `queue_release()` 减少引用计数，计数为0时释放资源
   - 防止删除队列时其他线程正在使用

3. **生命周期管理**：
   - `valid` 标志标记队列是否有效
   - 删除时设置 `valid=false` 并唤醒所有等待线程
   - 等待线程检查 `valid` 标志后返回错误

4. **阻塞机制**：
   - 使用 `pthread_cond_t not_empty` 和 `not_full` 条件变量
   - 支持三种模式：
     - `OS_PEND`：永久等待
     - `timeout > 0`：超时等待（使用 `pthread_cond_timedwait`）
     - `OS_CHECK`：非阻塞检查

5. **溢出保护**：
   - 限制队列深度 ≤ 10000，消息大小 ≤ 64KB
   - 检查乘法溢出：`queue_depth * data_size`

**配置**（`queue_config.h`）：
- 队列深度：50
- 消息最大长度：512字节

#### 2.1.3 互斥锁（osal_mutex）

**文件**：`osal_mutex.h`, `osal_mutex.c`

**功能**：基于pthread_mutex的互斥锁，支持死锁检测

**关键接口**：
- `OSAL_MutexCreate()` - 创建互斥锁
- `OSAL_MutexDelete()` - 删除互斥锁
- `OSAL_MutexLock()` - 获取锁（阻塞）
- `OSAL_MutexUnlock()` - 释放锁
- `OSAL_MutexLockTimeout()` - 带超时的锁获取
- `OSAL_MutexSetDeadlockDetection()` - 设置死锁检测

**实现细节**：
1. **互斥锁表**：
   - 静态数组 `OS_mutex_table[64]`
   - 使用 `mutex_table_mutex` 保护表操作

2. **死锁检测**：
   - 使用 `pthread_mutex_timedlock()` 实现超时
   - 默认阈值：5000ms
   - 超时后触发回调函数 `deadlock_callback_t`
   - 回调参数：互斥锁名称、等待时间

3. **有效性标志**：
   - `valid` 标志防止删除后的锁被使用
   - 删除时先设置 `valid=false`，再销毁锁

4. **锁外操作**：
   - 查找锁时持有 `mutex_table_mutex`
   - 实际加锁/解锁在表锁外进行（避免嵌套锁）

#### 2.1.4 日志系统（osal_log）

**文件**：`osal_log.h`, `osal_log.c`, `config/log_config.h`

**功能**：分级日志系统，支持文件轮转和线程安全

**关键接口**：
- `OSAL_LogInit()` - 初始化日志系统
- `OSAL_LogDebug/Info/Warn/Error/Fatal()` - 分级日志函数
- `OSAL_LogSetLevel()` - 设置日志级别
- `OSAL_LogSetMaxFileSize()` - 设置文件大小限制
- `OSAL_Printf()` - 简单打印（兼容旧接口）

**实现细节**：
1. **日志级别**：
   - DEBUG(0) → INFO(1) → WARN(2) → ERROR(3) → FATAL(4)
   - 默认级别：INFO

2. **线程安全**：
   - 使用 `pthread_mutex_t g_log_mutex` 保护日志写入
   - `localtime_r()` 替代 `localtime()`（线程安全）

3. **日志轮转**（自动）：
   - 检查文件大小，超过阈值（默认10MB）自动轮转
   - 保留N个备份文件（默认5个）
   - 轮转逻辑：`log → log.1 → log.2 → ... → log.5`（删除最旧）

4. **双输出**：
   - 终端输出：带颜色（DEBUG=青色, INFO=绿色, WARN=黄色, ERROR=红色, FATAL=紫色）
   - 文件输出：无颜色，纯文本

5. **时间戳**：
   - 格式：`YYYY-MM-DD HH:MM:SS.mmm`（毫秒精度）
   - 使用 `gettimeofday()` 获取微秒级时间

**配置**（`log_config.h`）：
- 日志文件路径：`/var/log/ems.log`
- 最大文件大小：10MB
- 备份文件数：5个

#### 2.1.5 网络抽象（osal_network）

**文件**：`osal_network.h`, `osal_network.c`

**功能**：Socket抽象层，支持TCP/UDP

**关键接口**：
- `OSAL_SocketOpen()` - 创建socket
- `OSAL_SocketConnect()` - 连接（TCP）
- `OSAL_SocketBind/Listen/Accept()` - 服务端操作
- `OSAL_SocketSend/Recv()` - TCP收发
- `OSAL_SocketSendTo/RecvFrom()` - UDP收发
- `OSAL_SocketSetOpt/GetOpt()` - 设置选项
- `OSAL_SocketSelect()` - 多路复用

**实现细节**：
1. **Socket表**：
   - 静态数组 `g_socket_table[64]`
   - 映射OSAL ID到原生fd

2. **延迟初始化**：
   - 使用双重检查锁初始化 `g_socket_mutex`
   - 线程安全

3. **超时支持**：
   - 使用 `select()` 实现超时
   - 非阻塞connect：设置 `O_NONBLOCK`，用 `select()` 等待

4. **选项映射**：
   - 支持 `SO_REUSEADDR`, `SO_KEEPALIVE`, `TCP_NODELAY` 等
   - 三层：`OS_SOL_SOCKET`, `OS_SOL_TCP`, `OS_SOL_IP`

#### 2.1.6 其他模块

**时钟服务（osal_clock）**：
- 获取本地时间（秒+微秒）
- 系统滴答计数（毫秒）
- 毫秒转滴答（Linux下1:1）

**文件操作（osal_file）**：
- 文件打开/关闭/读写
- 文件定位、标志设置
- 设备控制（ioctl）
- 多路复用（select）

**信号处理（osal_signal）**：
- 信号注册（使用sigaction）
- 信号忽略/恢复默认
- 信号屏蔽/解除屏蔽

**内存管理（osal_heap）**：
- 读取 `/proc/self/status` 获取内存使用
- 阈值告警（默认80%）
- 统计信息（当前使用量、峰值）

**错误处理（osal_error）**：
- 37个标准错误码
- 错误码到字符串映射

**类型系统**：
- 整数类型：`int8/16/32/64`、`uint8/16/32/64`
- OSAL对象ID：`osal_id_t` (uint32)
- 返回值：`OS_SUCCESS(0)`、`OS_ERROR(-1)`等
- 配置常量：`OS_MAX_TASKS(64)`、`OS_MAX_QUEUES(64)`、`OS_MAX_MUTEXES(64)`
- 超时常量：`OS_PEND(0永久等待)`、`OS_CHECK(-1非阻塞)`
- 优先级范围：1-255（数字越小优先级越高）

**Linux实现特点**：
- 任务：基于pthread，支持1-255优先级映射
- 队列：基于pthread_mutex + pthread_cond
- 互斥锁：基于pthread_mutex，支持超时和死锁检测
- 网络：基于Linux socket API
- 文件：基于POSIX文件API

### 2.2 HAL层 (硬件抽象层)

**职责**：封装硬件驱动，隔离硬件差异

**核心模块**：

#### 2.2.1 CAN驱动 (hal_can)

**文件**：`hal_can.h`, `hal_can.c`, `config/can_types.h`, `config/can_config.h`

**实现方式**：SocketCAN (Linux标准CAN接口)

**关键接口**：
- `HAL_CAN_Init()` - 初始化CAN设备（创建socket、绑定接口）
- `HAL_CAN_Deinit()` - 反初始化
- `HAL_CAN_Send()` - 发送CAN帧（阻塞）
- `HAL_CAN_Recv()` - 接收CAN帧（支持超时）
- `HAL_CAN_SetFilter()` - 设置CAN ID过滤器
- `HAL_CAN_GetStats()` - 获取统计信息

**数据结构**：
```c
typedef struct {
    uint32 can_id;        // CAN ID (11位标准帧或29位扩展帧)
    uint8  dlc;           // 数据长度码 (0-8)
    uint8  data[8];       // 数据字段
    uint32 timestamp;     // 接收时间戳（由驱动填充）
} can_frame_t;

typedef struct {
    int sockfd;                  // SocketCAN文件描述符
    uint32 tx_count;             // 发送计数器
    uint32 rx_count;             // 接收计数器
    uint32 err_count;            // 错误计数器
    char interface[IFNAMSIZ];    // 接口名称（如"can0"）
    uint32 baudrate;             // 波特率（仅记录）
    bool initialized;            // 初始化标志
} can_handle_impl_t;
```

**实现细节**：
1. **初始化流程**：
   - 创建SocketCAN原始套接字：`socket(PF_CAN, SOCK_RAW, CAN_RAW)`
   - 通过 `ioctl(SIOCGIFINDEX)` 获取接口索引（需要接口已启动）
   - 绑定到指定CAN接口：`bind(sockfd, &addr, sizeof(addr))`
   - 设置接收/发送超时：`setsockopt(SO_RCVTIMEO/SO_SNDTIMEO)`

2. **发送实现**：
   - 参数校验：检查DLC范围（0-8）
   - 格式转换：`can_frame_t` → `struct can_frame`（Linux内核格式）
   - 原子写入：`write(sockfd, &can_frame, sizeof(struct can_frame))`
   - 统计维护：成功则 `tx_count++`，失败则 `err_count++`

3. **接收实现**：
   - 支持动态超时：通过 `setsockopt` 临时修改 `SO_RCVTIMEO`
   - 阻塞读取：`read(sockfd, &can_frame, sizeof(struct can_frame))`
   - 超时处理：`errno == EAGAIN/EWOULDBLOCK` → 返回 `OS_ERROR_TIMEOUT`
   - 时间戳填充：使用 `OS_GetTickCount()` 记录接收时刻
   - 防御性编程：限制DLC最大值为8，防止越界拷贝

4. **过滤器实现**：
   - 使用SocketCAN标准过滤机制：`setsockopt(SOL_CAN_RAW, CAN_RAW_FILTER)`
   - 过滤规则：`(received_id & mask) == (filter_id & mask)`
   - 支持单个过滤器（可扩展为数组）

5. **统计信息**：
   - 维护三个计数器：发送、接收、错误
   - 线程不安全（假设单线程访问或上层加锁）

**配置参数**（`can_config.h`）：
- 接口名：`can0`
- 波特率：500Kbps（需通过 `ip link set can0 type can bitrate 500000` 配置）
- 队列深度：RX/TX各100帧
- 超时时间：1000ms

**注意事项**：
- 波特率配置外置：代码中 `baudrate` 仅记录，实际需通过 `ip` 命令配置
- 接口预启动要求：初始化前必须 `ip link set can0 up`
- 线程安全缺失：统计计数器无原子操作保护
- 单过滤器限制：`HAL_CAN_SetFilter` 仅支持1个过滤规则

#### 2.2.2 串口驱动 (hal_serial)

**文件**：`hal_serial.h`, `hal_serial.c`, `config/uart_config.h`

**实现方式**：termios (Linux标准串口接口)

**关键接口**：
- `HAL_Serial_Open()` - 打开串口设备
- `HAL_Serial_Close()` - 关闭串口
- `HAL_Serial_Write()` - 写入数据（支持超时）
- `HAL_Serial_Read()` - 读取数据（支持超时）
- `HAL_Serial_Flush()` - 清空缓冲区
- `HAL_Serial_SetConfig()` - 设置配置（接口定义但未实现）

**数据结构**：
```c
typedef struct {
    uint32 baud_rate;      // 波特率（9600-4000000）
    uint8  data_bits;      // 数据位（5/6/7/8）
    uint8  stop_bits;      // 停止位（1/2）
    uint8  parity;         // 校验位（NONE/ODD/EVEN）
    uint8  flow_control;   // 流控（NONE/HW/SW）- 当前未使用
} hal_serial_config_t;

typedef struct {
    int fd;                        // 文件描述符
    hal_serial_config_t config;    // 当前配置
    char device[64];               // 设备路径
} hal_serial_context_t;
```

**实现细节**：
1. **波特率映射**：
   - 支持范围：9600 ~ 4000000 bps
   - 使用termios标准宏：`B9600`, `B115200`, `B4000000` 等
   - 默认回退：不支持的波特率返回 `B115200`

2. **打开流程**：
   - 打开设备：`open(device, O_RDWR | O_NOCTTY | O_NDELAY)`
     - `O_RDWR`：读写模式
     - `O_NOCTTY`：不作为控制终端
     - `O_NDELAY`：非阻塞打开（后续改为阻塞）
   - 设置阻塞模式：`fcntl(fd, F_SETFL, 0)`
   - 获取当前配置：`tcgetattr(fd, &tty)`
   - 配置termios结构体：
     - 波特率：`cfsetispeed/cfsetospeed`
     - 数据位：`CS7/CS8`
     - 停止位：`CSTOPB`
     - 校验位：`PARENB + PARODD`
     - 本地模式：`CLOCAL | CREAD`
     - 禁用硬件流控：`~CRTSCTS`
     - 原始模式：清除 `ICANON/ECHO/IXON` 等标志
   - 应用配置：`tcsetattr(fd, TCSANOW, &tty)`
   - 清空缓冲区：`tcflush(fd, TCIOFLUSH)`

3. **原始模式配置**：
   ```c
   tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // 禁用行缓冲和回显
   tty.c_iflag &= ~(IXON | IXOFF | IXANY);          // 禁用软件流控
   tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
   tty.c_oflag &= ~OPOST;                            // 禁用输出处理
   tty.c_cc[VMIN]  = 0;  // 最小字符数=0
   tty.c_cc[VTIME] = 0;  // 超时=0（配合select实现超时）
   ```

4. **写入实现**：
   - 超时机制：使用 `select` 监听可写事件
   - 阻塞写入：`write(fd, buffer, size)`
   - 返回值：实际写入字节数（可能小于请求大小）

5. **读取实现**：
   - 超时机制：使用 `select` 监听可读事件
   - 非阻塞读取：`read(fd, buffer, size)`
   - 返回值：实际读取字节数（0表示无数据）

6. **缓冲区刷新**：
   - 使用 `tcflush(fd, TCIOFLUSH)` 清空输入输出缓冲区
   - 用于丢弃旧数据，确保读取最新数据

**配置参数**（`uart_config.h`）：
- 设备：`/dev/ttyS0`
- 波特率：115200
- 数据位：8
- 停止位：1
- 校验：无
- 超时：2000ms

**注意事项**：
- 部分写入风险：`write` 可能返回小于请求的字节数，上层需循环写入
- 流控未实现：`flow_control` 参数被忽略，硬编码禁用流控
- `SetConfig` 接口缺失：头文件声明但未实现，无法动态修改配置
- 设备权限要求：需要读写 `/dev/ttyS0` 权限（通常需root或dialout组）

### 2.3 PCL层 (硬件配置层)

**职责**：管理硬件配置和APP配置，实现配置与代码分离

**设计理念**：两层配置架构 - APP层关注"用哪个外设"，硬件层关注"外设怎么连接"

**核心模块**：

#### 2.3.1 硬件配置 (Hardware Config)

**文件**：
- `core/pcl/include/hw_config_types.h` - 配置数据结构定义
- `core/pcl/include/hw_config_api.h` - 配置API接口
- `core/pcl/src/hw_config_api.c` - 配置管理实现
- `core/pcl/src/hw_config_register.c` - 配置注册实现

**配置组织**：
```
core/pcl/platform/{vendor}/{soc}/{product}_{variant}.c
例如：core/pcl/platform/ti/am625/h200_payload_base.c
```

**硬件配置结构**：
- **外设类型**：MCU、BMC、主机接口、传感器、存储设备
- **接口类型**：CAN、UART、I2C、SPI、Ethernet、1553B
- **配置继承**：base → v1 → v2（版本演进）

**关键接口**：
- `PCL_Init()` - 初始化配置系统
- `PCL_HW_GetMCU(id)` - 获取MCU配置
- `PCL_HW_GetBMC(id)` - 获取BMC配置
- `PCL_HW_GetSatellite(id)` - 获取主机接口配置
- `PCL_HW_GetSensor(id)` - 获取传感器配置
- `PCL_HW_GetStorage(id)` - 获取存储设备配置

**配置选择机制**：
1. 环境变量：`HW_CONFIG=h200_payload_v2`
2. 编译选项：`-DHW_CONFIG_DEFAULT="h200_payload_v2"`
3. 默认配置：第一个注册的配置

#### 2.3.2 APP配置 (Application Config)

**设计目标**：APP只关心外设逻辑功能，不关心硬件接口细节

**APP配置结构**：
```c
typedef struct {
    const char *app_name;              // APP名称
    hw_app_device_mapping_t *devices;  // 外设映射列表
    uint32_t device_count;             // 外设数量
} hw_app_config_t;
```

**外设映射**：
```c
typedef struct {
    const char *logical_name;     // 逻辑名称（如"host_comm"）
    hw_device_type_t device_type; // 外设类型（如HW_DEVICE_SATELLITE）
    uint32_t device_id;           // 外设ID（如0）
    uint32_t backup_device_id;    // 备用外设ID（冗余配置）
} hw_app_device_mapping_t;
```

**关键接口**：
- `PCL_APP_Find(app_name)` - 查找APP配置
- `PCL_APP_FindDevice(app_cfg, logical_name)` - 查找外设映射
- `PCL_APP_GetDeviceByMapping(mapping)` - 获取外设配置

**使用示例**：
```c
// 1. 初始化配置系统
PCL_Init();

// 2. 查找APP配置
const hw_app_config_t *app = PCL_APP_Find("can_gateway");

// 3. 查找外设映射
const hw_app_device_mapping_t *mapping = 
    PCL_APP_FindDevice(app, "host_comm");

// 4. 获取硬件配置
const void *device = PCL_APP_GetDeviceByMapping(mapping);
const hw_satellite_config_t *host = (const hw_satellite_config_t *)device;

// 5. 使用硬件配置
if (host->interface.type == HW_INTERFACE_CAN) {
    can_id = host->interface.config.can.can_id;
}
```

**配置示例**（h200_payload_v1.c）：
```c
static hw_app_device_mapping_t app_can_gateway_devices_v1[] = {
    {
        .logical_name = "host_comm",
        .device_type = HW_DEVICE_SATELLITE,
        .device_id = 0,              // 主接口：CAN0
        .backup_device_id = 1        // 备用接口：CAN1（冗余）
    }
};

static hw_app_config_t app_can_gateway_v1 = {
    .app_name = "can_gateway",
    .devices = app_can_gateway_devices_v1,
    .device_count = 1
};
```

**优势**：
- **关注点分离**：APP代码不依赖具体硬件接口
- **配置复用**：同一APP可用于不同硬件平台
- **冗余支持**：通过backup_device_id实现主备切换
- **类型安全**：编译时检查外设类型匹配

### 2.4 PDL层 (外设驱动层)

**职责**：管理各类外部设备，提供统一的外设服务接口

**设计理念**：控制板为核心，将各类外部设备统一抽象为外设

**核心模块**：

#### 2.4.1 主机接口服务 (pdl_satellite)

**文件**：
- `pdl_satellite.h` - 对外业务接口
- `pdl_satellite.c` - 服务管理和任务调度
- `pdl_satellite_can.c` - CAN通信实现
- `pdl_satellite_internal.h` - 内部协议定义

**通信方式**：CAN总线

**关键接口**：
- `PDL_Satellite_Init()` - 初始化主机接口服务
- `PDL_Satellite_RegisterCallback()` - 注册命令回调函数
- `PDL_Satellite_SendResponse()` - 发送命令响应
- `PDL_Satellite_SendHeartbeat()` - 发送心跳
- `PDL_Satellite_GetStats()` - 获取统计信息

**实现细节**：

1. **CAN协议定义**（`pdl_satellite_internal.h`）：
   - **CAN ID**：RX=0x100（主机→控制板），TX=0x101（控制板→主机）
   - **消息类型**：0x01=命令请求，0x02=命令响应，0x03=心跳
   - **帧格式**（8字节）：`[msg_type][seq_num][cmd_type][reserved][data(4字节)]`

2. **任务架构**（`pdl_satellite.c`）：
   - **心跳任务**（`heartbeat_task`）：周期性发送心跳，间隔由配置决定
   - **接收任务**（`can_rx_task`）：阻塞接收CAN消息，超时后重试
   - **回调机制**：收到命令请求后调用用户注册的回调函数

3. **CAN通信层**（`pdl_satellite_can.c`）：
   - 封装HAL层CAN接口（`HAL_CAN_Init/Send/Recv`）
   - **帧解析**：从8字节CAN帧提取消息类型、序列号、命令类型、数据
   - **帧封装**：将业务数据打包成8字节CAN帧
   - **便捷函数**：`satellite_can_send_heartbeat`、`satellite_can_send_response`

4. **优雅退出**：
   - 任务循环检查 `OSAL_TaskShouldShutdown()`
   - 反初始化时先停止任务，再关闭CAN设备

**统计信息**：
- 接收计数（rx_count）
- 发送计数（tx_count）
- 错误计数（error_count）

#### 2.4.2 BMC设备服务 (pdl_bmc)

**文件**：
- `pdl_bmc.h` - 对外业务接口
- `pdl_bmc.c` - 服务管理和通道切换
- `pdl_bmc_ipmi.c` - IPMI协议实现
- `pdl_bmc_redfish.c` - 网络和串口通信实现
- `pdl_bmc_internal.h` - 内部协议定义

**通信方式**：以太网（IPMI over LAN）/ UART（IPMI over Serial）

**关键接口**：
- `PDL_BMC_Init()` - 初始化BMC服务
- `PDL_BMC_PowerOn/PowerOff/PowerReset()` - 电源控制
- `PDL_BMC_GetPowerState()` - 获取电源状态
- `PDL_BMC_ReadSensors()` - 读取传感器（温度、电压、电流、风扇）
- `PDL_BMC_SwitchChannel()` - 手动切换通道
- `PDL_BMC_GetChannel()` - 获取当前通道
- `PDL_BMC_IsConnected()` - 检查连接状态
- `PDL_BMC_GetStats()` - 获取统计信息

**实现细节**：

1. **通道管理**（`pdl_bmc.c`）：
   - **双通道架构**：网络通道（IPMI over LAN）+ 串口通道（IPMI over Serial）
   - **主备切换**：配置主通道，支持手动切换（`PDL_BMC_SwitchChannel`）
   - **统计信息**：记录命令总数、成功/失败次数、通道切换次数
   - **线程安全**：使用互斥锁保护通道切换和命令执行

2. **IPMI协议**（`pdl_bmc_ipmi.c`）：
   - **命令码定义**：
     - `0x00` - Chassis Control（电源控制）
     - `0x01` - Get Chassis Status（状态查询）
     - `0x2D` - Get Sensor Reading（传感器读取）
   - **子命令**：0x00=下电，0x01=上电，0x02=电源循环，0x03=硬复位
   - **帧格式**（简化版）：`[cmd_code][subcmd][data...]`
   - **响应解析**：`[status][data...]`，status=0表示成功
   - **电源状态解析**：从Chassis Status响应的Bit 0判断电源开关状态

3. **通信层**（`pdl_bmc_redfish.c`）：
   - **网络通信**：
     - 使用OSAL Socket接口（`OSAL_SocketOpen/Connect/Send/Recv`）
     - TCP连接到BMC（默认端口623）
     - 超时控制
   - **串口通信**：
     - 使用HAL Serial接口（`HAL_Serial_Open/Write/Read`）
     - 配置：8N1（8数据位，无校验，1停止位）
     - 超时控制
   - **统一接口**：`bmc_redfish_send_recv` 和 `bmc_serial_send_recv` 提供统一的发送/接收接口

4. **函数指针解耦**：
   - IPMI协议层不直接依赖通信层，通过函数指针传递 `send_recv` 函数
   - 例如：`bmc_ipmi_power_on(comm_handle, send_recv_func)`
   - 业务层根据当前通道选择对应的通信函数

5. **自动切换逻辑**：
   - 连续5次失败后自动切换通道
   - 成功后重置失败计数
   - 状态机：DISCONNECTED → CONNECTING → CONNECTED → ERROR

**协议支持**：
- IPMI (Intelligent Platform Management Interface)
- Redfish (RESTful API) - 预留

**通道配置**：
- 主通道：以太网（192.168.1.100:623）
- 备份通道：UART（/dev/ttyS0, 115200）
- 自动切换：连续5次失败后切换
- 故障恢复：成功后重置失败计数

**待实现功能**：
- 传感器读取（`bmc_ipmi_read_sensors` 返回 `OS_ERROR`）
- 原始命令执行（`PDL_BMC_ExecuteCommand` 返回 `OS_ERROR`）

#### 2.4.3 MCU外设服务 (pdl_mcu)

**文件**：
- `pdl_mcu.h` - 对外业务接口
- `pdl_mcu.c` - 驱动管理和命令调度
- `pdl_mcu_can.c` - CAN通信实现
- `pdl_mcu_serial.c` - 串口通信实现
- `pdl_mcu_protocol.c` - 协议工具函数
- `pdl_mcu_internal.h` - 内部协议定义

**通信方式**：CAN / UART / I2C / SPI

**关键接口**：
- `PDL_MCU_Init()` - 初始化MCU服务
- `PDL_MCU_GetVersion()` - 获取版本信息
- `PDL_MCU_GetStatus()` - 获取状态信息
- `PDL_MCU_Reset()` - 复位MCU
- `PDL_MCU_ReadRegister()` - 读寄存器
- `PDL_MCU_WriteRegister()` - 写寄存器
- `PDL_MCU_SendCommand()` - 发送自定义命令
- `PDL_MCU_FirmwareUpdate()` - 固件升级（支持进度回调）

**实现细节**：

1. **多接口支持**（`pdl_mcu.c`）：
   - **接口类型**：CAN、串口、I2C（预留）、SPI（预留）
   - **统一调度**：`mcu_send_command_internal` 根据接口类型分发到对应通信模块
   - **线程安全**：使用互斥锁保护命令执行
   - **命令码定义**：
     - `0x01` - 获取版本
     - `0x02` - 获取状态
     - `0x03` - 复位
     - `0x10` - 读寄存器
     - `0x11` - 写寄存器
     - `0x20` - 上电
     - `0x21` - 下电
     - `0xFF` - 自定义命令

2. **CAN通信**（`pdl_mcu_can.c`）：
   - **帧格式**（发送）：`[cmd_code][data_len][data(最多6字节)]`
   - **帧格式**（接收）：`[status][data_len][data...]`
   - **CAN ID配置**：由配置文件指定TX/RX ID
   - **互斥保护**：接收时加锁，防止多线程冲突
   - **ID过滤**：检查接收帧的CAN ID是否匹配

3. **串口通信**（`pdl_mcu_serial.c`）：
   - **帧格式**：`[0xAA][0x55][cmd][len][data...][crc16_h][crc16_l]`
   - **帧头**：固定为 `0xAA 0x55`
   - **CRC校验**：可选，使用MODBUS CRC16（多项式0xA001）
   - **封装函数**：`mcu_serial_pack_frame` - 添加帧头和CRC
   - **解析函数**：`mcu_serial_unpack_frame` - 校验帧头和CRC
   - **互斥保护**：接收时加锁

4. **协议工具**（`pdl_mcu_protocol.c`）：
   - **CRC16计算**：`mcu_protocol_calc_crc16` - MODBUS标准（初始值0xFFFF，多项式0xA001）
   - **通用帧封装/解析**：预留接口，目前由各通信模块自己实现

5. **版本和状态解析**：
   - **版本信息**：4字节 `[major][minor][patch][build]`，格式化为字符串 "1.2.3.4"
   - **状态信息**：8字节 `[uptime(4字节)][error_code][temperature][voltage_h][voltage_l]`

**命令码**：
- GET_VERSION：获取版本
- GET_STATUS：获取状态
- RESET：复位
- READ_REG/WRITE_REG：寄存器操作
- POWER_ON/OFF：电源控制
- CUSTOM：自定义命令

**协议封装**：
- CAN帧格式：`[cmd][len][data(最多6字节)]`
- 串口帧格式：`[0xAA][0x55][cmd][len][data...][crc16]`
- CRC校验：CRC-16/MODBUS

**待实现功能**：
- I2C/SPI接口（返回 `OS_ERROR`）
- 固件升级（`PDL_MCU_FirmwareUpdate` 返回 `OS_ERROR`）

### 2.5 Apps层 (应用层)

**职责**：实现具体业务逻辑

**核心应用**：

#### 2.5.1 CAN网关 (can_gateway)

**文件**：
- `can_gateway.h` - 接口定义
- `can_gateway.c` - 核心逻辑（328行）
- `main.c` - 启动入口（116行）
- `config/app_config.h` - 版本号定义
- `config/can_protocol.h` - CAN协议定义（177行）

**功能定位**：作为主机系统的CAN通信前端，负责接收/发送CAN消息、维护心跳、提供消息队列

**架构设计**：

**三任务并发架构**：
```
CAN_RX_Task (PRIORITY_CRITICAL)  // 接收CAN消息 → 内部队列
    ↓
g_can_rx_queue (深度10)
    ↓
Protocol_Converter_Task          // 消费命令，生成响应
    ↓
g_can_tx_queue (深度10)
    ↓
CAN_TX_Task (PRIORITY_CRITICAL)  // 发送CAN消息 → 总线

CAN_Heartbeat_Task (PRIORITY_LOW) // 每5秒发送心跳
```

**线程安全设计**：
- **原子统计**：使用C11 `atomic_uint` 保护统计计数器（rx_count/tx_count/err_count）
- **互斥锁**：`g_stats_mutex` 保护序列号递增（避免竞态条件）
- **无锁队列**：OSAL队列本身线程安全

**CAN协议设计（8字节固定帧）**：

**帧结构**（`can_protocol.h`）：
```c
typedef struct {
    uint8  msg_type;   // Byte 0: 消息类型
    uint8  cmd_type;   // Byte 1: 命令类型/状态码
    uint16 seq_num;    // Byte 2-3: 序列号
    uint32 data;       // Byte 4-7: 数据/参数
} __attribute__((packed)) can_msg_t;
```

**消息类型**：
- `CMD_REQ` (0x01) - 命令请求
- `CMD_RESP` (0x02) - 命令响应
- `STATUS_QUERY` (0x03) - 状态查询
- `STATUS_REPORT` (0x04) - 状态上报
- `HEARTBEAT` (0x05) - 心跳
- `ERROR` (0x06) - 错误

**命令类型**：
- 电源控制：`POWER_ON/OFF/RESET`
- 状态查询：`QUERY_STATUS/TEMP/VOLTAGE`
- 配置管理：`SET_CONFIG/GET_CONFIG`

**CAN ID分配**：
- `0x100` - 主机→控制板
- `0x200` - 控制板→主机

**消息处理流程**：

**接收路径**（`can_rx_task`）：
```c
while(1) {
    HAL_CAN_Recv() → 过滤CAN ID → 解析消息 → OSAL_QueuePut(g_can_rx_queue)
    // 错误处理：队列满则丢弃，更新err_count
}
```

**发送路径**（`can_tx_task`）：
```c
while(1) {
    OSAL_QueueGet(g_can_tx_queue) → HAL_CAN_Send()
    // 失败重试：延时10ms后重试一次
}
```

**心跳维护**（`can_heartbeat_task`）：
```c
while(1) {
    OSAL_TaskDelay(5000);
    can_build_heartbeat(&frame, uptime);
    OSAL_QueuePut(g_can_tx_queue);
}
```

**辅助函数**（内联优化）：
- `can_build_cmd_request()` - 构造命令请求
- `can_build_cmd_response()` - 构造命令响应
- `can_build_status_report()` - 构造状态上报
- `can_build_heartbeat()` - 构造心跳消息
- `can_get_msg_type_name()` - 消息类型转字符串（调试）
- `can_get_cmd_type_name()` - 命令类型转字符串（调试）

**统计信息**：
```c
void CAN_Gateway_GetStats(uint32 *rx_count, uint32 *tx_count, uint32 *err_count)
{
    // 原子读取，无需加锁
    *rx_count = atomic_load(&g_stats.rx_count);
    *tx_count = atomic_load(&g_stats.tx_count);
    *err_count = atomic_load(&g_stats.err_count);
}
```

#### 2.5.2 协议转换器 (protocol_converter)

**文件**：
- `protocol_converter.h` - 转换器接口
- `payload_pdl.h` - 设备服务接口
- `protocol_converter.c` - 转换逻辑（375行）
- `payload_pdl.c` - 设备服务实现（709行）
- `main.c` - 启动入口（121行）
- `config/app_config.h` - 超时/重试配置
- `config/ethernet_config.h` - 以太网配置

**功能定位**：作为协议转换中间层，负责CAN命令→IPMI命令转换、通道故障自动切换

**架构设计**：

**单任务消费者模式**：
```c
protocol_converter_task (PRIORITY_HIGH)
    ↓
从 CAN_Gateway_GetRxQueue() 获取命令
    ↓
execute_ipmi_command() → PayloadService_Send/Recv()
    ↓
parse_ipmi_response() → 提取结果
    ↓
CAN_Gateway_SendResponse() → 发送响应
```

**双通道冗余设计**（`payload_pdl.c`）：

**通道抽象**：
```c
typedef enum {
    PAYLOAD_CHANNEL_ETHERNET = 0,  // 主通道
    PAYLOAD_CHANNEL_UART     = 1,  // 备份通道
} payload_channel_t;
```

**上下文结构**：
```c
typedef struct {
    payload_service_config_t config;
    payload_channel_t        current_channel;
    conn_state_t             state;           // 连接状态机
    int32                    eth_fd;          // socket fd
    int32                    uart_fd;         // 串口 fd
    bool                     connected;
    uint32                   fail_count;      // 失败计数
    uint32                   reconnect_count; // 重连计数
    osal_id_t                mutex;           // 线程安全
    
    // 统计信息
    uint32 tx_bytes, rx_bytes, tx_errors, rx_errors;
} payload_service_context_t;
```

**状态机**：
```c
typedef enum {
    CONN_STATE_DISCONNECTED = 0,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECTED,
    CONN_STATE_ERROR
} conn_state_t;
```

**自动切换逻辑**：

**触发条件**（`PayloadService_Send`）：
```c
if (ret < 0 && ctx->config.auto_switch) {
    if (ctx->fail_count >= ctx->config.retry_count) {  // 默认3次
        // 切换通道
        payload_channel_t new_channel = (current == ETHERNET) ? UART : ETHERNET;
        
        if (new_channel == ETHERNET) {
            uart_close() → ethernet_connect()
        } else {
            ethernet_disconnect() → uart_open()
        }
        
        ctx->fail_count = 0;  // 重置计数
    }
}
```

**恢复机制**：
- 成功发送后重置 `fail_count`
- 允许手动切换回主通道（`PayloadService_SwitchChannel`）

**IPMI命令映射**（`execute_ipmi_command`）：

```c
CAN命令                    → IPMI命令
---------------------------------------------------------
CMD_TYPE_POWER_ON         → "ipmitool chassis power on"
CMD_TYPE_POWER_OFF        → "ipmitool chassis power off"
CMD_TYPE_RESET            → "ipmitool chassis power reset"
CMD_TYPE_QUERY_STATUS     → "ipmitool chassis status"
CMD_TYPE_QUERY_TEMP       → "ipmitool sdr type temperature"
CMD_TYPE_QUERY_VOLTAGE    → "ipmitool sdr type voltage"
```

**响应解析**（`parse_ipmi_response`）：

**电源控制**：
```c
if (strstr(response, "Chassis Power Control: Up/On"))
    *result = 1, return STATUS_OK;
if (strstr(response, "Error"))
    return STATUS_COMM_ERROR;
```

**状态查询**：
```c
if (strstr(response, "System Power") && strstr(response, "on"))
    *result = 1;  // 电源开启
else if (strstr(response, "System Power") && strstr(response, "off"))
    *result = 0;  // 电源关闭
```

**温度查询**：
```c
// 查找 "degrees C" 关键字
// 向前扫描提取数字
sscanf(num_start + 1, "%d", &temp_value);
*result = (uint32)temp_value;
```

**电压查询**：
```c
// 查找 "Volts" 关键字
// 提取浮点数并转换为毫伏
sscanf(num_start + 1, "%f", &volt_value);
*result = (uint32)(volt_value * 1000);  // V → mV
```

**以太网实现**（`ethernet_connect`）：

**非阻塞连接**：
```c
1. socket(AF_INET, SOCK_STREAM) → 创建TCP socket
2. setsockopt(SO_KEEPALIVE, TCP_NODELAY) → 优化选项
3. fcntl(O_NONBLOCK) → 设置非阻塞
4. connect() → 发起连接（返回EINPROGRESS）
5. select(5秒超时) → 等待连接完成
6. getsockopt(SO_ERROR) → 检查连接结果
7. fcntl() → 恢复阻塞模式
```

**UART实现**（`uart_open`）：

**配置参数**（8N1，无流控）：
```c
1. open(device, O_RDWR | O_NOCTTY)
2. tcgetattr() → 获取当前配置
3. cfsetospeed/cfsetispeed() → 设置波特率（支持9600~115200）
4. 配置：8位数据位（CS8）、无校验（~PARENB）、1位停止位（~CSTOPB）
5. tcsetattr(TCSANOW) → 应用配置
6. tcflush(TCIOFLUSH) → 清空缓冲区
```

**线程安全保护**：

**互斥锁范围**：
```c
PayloadService_Send/Recv/SwitchChannel/GetChannel:
    OSAL_MutexLock(ctx->mutex);
    // 访问共享状态（current_channel, connected, fail_count）
    // 执行通道切换
    OSAL_MutexUnlock(ctx->mutex);
```

**统计信息**：
```c
void Protocol_Converter_GetStats(uint32 *cmd_count, uint32 *success_count,
                                  uint32 *fail_count, uint32 *timeout_count)
{
    // 原子读取
    *cmd_count = atomic_load(&g_stats.cmd_count);
    *success_count = atomic_load(&g_stats.success_count);
    *fail_count = atomic_load(&g_stats.fail_count);
    *timeout_count = atomic_load(&g_stats.timeout_count);
}
```

#### 2.5.3 外设固件升级服务 (firmware_update)

**功能定位**：接收载荷服务器通过以太网传输的外设固件，升级MCU/FPGA等外设

**架构设计**：

**按需启动服务**：
```
载荷服务器 --[TCP/IP]--> pmc_fwupdate进程 --[PDL接口]--> 外设(MCU/FPGA)
```

**关键特性**：
- **按需启动**：由Supervisor进程按需启动，升级完成后自动退出
- **进程隔离**：独立进程，升级失败不影响关键功能（遥控遥测）
- **可靠传输**：TCP协议 + 分块传输（64KB/块）+ ACK确认
- **完整性校验**：MD5校验（传输前后）+ CRC32校验（升级前）
- **进度反馈**：实时上报升级进度到载荷服务器

**通信协议**：
```c
// 协议帧格式
typedef struct {
    uint32_t magic;        // 魔数：0xAA55AA55
    uint8_t  cmd;          // 命令类型（START/DATA/END/ACK/NACK）
    uint8_t  target;       // 目标外设（MCU/FPGA/CPLD）
    uint16_t seq;          // 序列号
    uint32_t length;       // 数据长度
    uint32_t reserved;     // 保留字段
} fw_protocol_header_t;
```

**传输流程**：
1. 载荷服务器发送开始命令（包含文件名、大小、MD5、目标外设）
2. AM625创建临时文件，返回ACK
3. 分块传输数据（每块最大64KB），每块ACK确认
4. 传输完成后校验MD5
5. 调用PDL接口升级外设（`PDL_MCU_FirmwareUpdate` / `PDL_FPGA_FirmwareUpdate`）
6. 实时上报升级进度
7. 返回升级结果，清理临时文件

**存储路径**：
- 临时目录：`/tmp/firmware/`
- 文件命名：`{target}_fw_v{version}.bin`

**超时机制**：
- 传输超时：60秒无数据则中止
- 空闲超时：10分钟无活动则自动退出进程

## 3. 模块化配置设计

### 3.1 配置下沉原则

**设计理念**：每个模块的配置文件放在模块内部的 `include/config/` 目录

**优势**：
- 模块独立：各模块只依赖自己的配置
- 便于协作：多人开发不冲突
- 易于拆分：支持多仓库拆分
- 依赖清晰：配置依赖关系一目了然

### 3.2 配置文件组织

```
ems/
├── core/                         # 框架核心模块
│   ├── osal/include/config/      # OSAL层配置
│   │   ├── task_config.h         # 任务栈大小、优先级
│   │   ├── queue_config.h        # 队列深度配置
│   │   └── log_config.h          # 日志级别、文件路径
│   ├── hal/include/config/       # HAL层配置
│   │   ├── can_types.h           # CAN帧类型定义
│   │   ├── can_config.h          # CAN接口、波特率
│   │   └── uart_config.h         # 串口设备、波特率
│   └── pdl/include/config/       # PDL层配置（预留）
└── apps/*/include/config/        # Apps层配置
    ├── can_gateway/include/config/
    │   ├── app_config.h          # 系统版本号
    │   └── can_protocol.h        # CAN协议定义
    └── protocol_converter/include/config/
        └── app_config.h          # 超时、重试配置
```

### 3.3 依赖隔离

**依赖关系**：

EMS采用分层架构，各层职责清晰：

- **OSAL层（操作系统抽象层）**：运行环境，为所有层提供统一接口，封装所有系统调用和标准库（任务、队列、互斥锁、网络socket、文件操作、内存操作、字符串操作等）
- **HAL层（硬件抽象层）**：封装硬件设备驱动（CAN、UART等），使用OSAL接口
- **PDL层（外设驱动层）**：管理外设服务（主机接口、BMC、MCU），使用OSAL接口，读取PCL配置
- **ACL层（应用配置层）**：业务功能到设备的映射配置，纯数据结构，被Apps读取
- **PCL层（外设配置库）**：以外设为单位的硬件配置管理，纯数据结构，被PDL读取
- **Apps层（应用层）**：业务应用，使用OSAL接口，读取ACL配置，调用PDL接口

**调用链**：`Apps → PDL → HAL`

**运行环境**：所有层（Apps/PDL/HAL）使用OSAL接口，OSAL封装Linux系统调用

**配置库**：ACL被Apps读取，PCL被PDL读取（不参与调用链）

**重要说明**：
- OSAL是所有层的运行环境，所有层（Apps/PDL/HAL）都必须使用OSAL接口
- 禁止任何层直接调用系统调用或标准库函数（如pthread_create、malloc、memcpy、strlen等）
- 必须使用OSAL封装的接口（如OSAL_TaskCreate、OSAL_Malloc、OSAL_Memcpy、OSAL_Strlen等）
- 网络socket属于操作系统抽象（OSAL），而非硬件抽象（HAL）
- PDL调用OSAL接口（如OSAL_socket）是正确的设计，不是跨层调用

**引用规范**：
```c
#include "config/task_config.h"    // 模块内部配置
#include "osal_task.h"             // OSAL接口
#include "hal_can.h"               // HAL接口
```

## 4. 关键设计特点

### 4.1 优雅退出机制（OSAL任务）

**问题**：`pthread_cancel` 可能导致死锁（任务持有锁时被取消）

**解决方案**：
1. 使用 `shutdown_requested` 原子标志
2. 任务循环检查 `OSAL_TaskShouldShutdown()`
3. 使用 `pthread_timedjoin_np` 等待任务退出（5秒超时）
4. 超时后使用 `pthread_detach` 而非强制取消

**示例代码**：
```c
static void my_task_entry(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    
    while (!OSAL_TaskShouldShutdown())  // 检查退出标志
    {
        // 执行任务逻辑
        do_work();
        
        // 延时
        OSAL_TaskDelay(100);
    }
    
    // 清理资源
    cleanup();
}
```

### 4.2 引用计数保护（OSAL队列）

**问题**：队列删除时可能有其他线程正在使用（use-after-free）

**解决方案**：
1. 使用 `atomic_int` 引用计数
2. 删除时先标记 `valid = false`
3. 引用计数降为0时才释放内存
4. 双重保护：`valid` 标志 + `ref_count`

**关键代码**：
```c
typedef struct {
    bool valid;                    // 队列是否有效
    atomic_int ref_count;          // 引用计数
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    // ...
} OS_queue_record_t;

// 获取队列（增加引用计数）
static OS_queue_record_t* get_queue_record(osal_id_t queue_id)
{
    OS_queue_record_t *record = &OS_queue_table[queue_id];
    if (!record->valid) return NULL;
    atomic_fetch_add(&record->ref_count, 1);
    return record;
}

// 释放队列（减少引用计数）
static void release_queue_record(OS_queue_record_t *record)
{
    if (atomic_fetch_sub(&record->ref_count, 1) == 1) {
        // 引用计数降为0，释放资源
        free(record->buffer);
    }
}
```

### 4.3 死锁检测（OSAL互斥锁）

**问题**：互斥锁死锁难以调试

**解决方案**：
1. 设置死锁检测阈值（默认5秒）
2. 超时后触发回调，记录等待时间
3. 避免嵌套锁：查找和获取分离

**关键代码**：
```c
typedef void (*deadlock_callback_t)(osal_id_t mutex_id, uint32_t wait_time_ms);

int32 OSAL_MutexSetDeadlockDetection(osal_id_t mutex_id, 
                                      uint32_t threshold_ms,
                                      deadlock_callback_t callback);
```

### 4.4 日志轮转（OSAL日志）

**问题**：日志文件无限增长，占满磁盘

**解决方案**：
1. 按大小轮转（默认10MB）
2. 保留N个备份（默认5个）
3. 轮转策略：`log.txt` → `log.txt.1` → `log.txt.2` → ...

**配置**：
```c
OSAL_LogSetMaxFileSize(10 * 1024 * 1024);  // 10MB
OSAL_LogSetMaxFiles(5);                     // 保留5个备份
```

### 4.5 通信冗余（PDL层）

**问题**：单一通信通道故障导致系统不可用

**解决方案**：
1. 主通道：以太网（IPMI over LAN，端口623）
2. 备份通道：UART（IPMI over Serial）
3. 自动切换：连续5次失败后切换
4. 故障恢复：成功后重置失败计数

**状态机**：
```
[以太网] --5次失败--> [UART] --成功--> [以太网]
   ↑                      |
   └──────5次失败─────────┘
```

### 4.6 协议转换（Apps层）

**CAN命令 → IPMI命令映射**：
```c
static int32 execute_ipmi_command(uint8_t cmd_type, uint32_t param, uint32_t *result)
{
    switch (cmd_type) {
        case CMD_TYPE_POWER_ON:
            return PDL_BMC_PowerOn();
        case CMD_TYPE_POWER_OFF:
            return PDL_BMC_PowerOff();
        case CMD_TYPE_RESET:
            return PDL_BMC_Reset();
        case CMD_TYPE_QUERY_STATUS:
            return query_power_state(result);
        case CMD_TYPE_QUERY_TEMP:
            return query_temperature(result);
        case CMD_TYPE_QUERY_VOLTAGE:
            return query_voltage(result);
        default:
            return OS_ERROR;
    }
}
```

**智能解析**：
- 提取温度：从传感器读数中提取温度值（单位：℃）
- 提取电压：从传感器读数中提取电压值（单位：mV）
- 状态映射：IPMI状态 → CAN状态码

## 5. 性能指标

| 指标 | 目标值 | 实际值 | 说明 |
|------|--------|--------|------|
| CAN消息延迟 | < 10ms | ~5ms | 从接收到处理完成 |
| 命令处理时间 | < 100ms | ~50ms | CAN命令到IPMI响应 |
| 内存占用 | < 128MB | ~80MB | 运行时内存占用 |
| CPU占用(空闲) | < 5% | ~2% | 无消息时CPU占用 |
| 日志轮转 | 10MB | 10MB | 单个日志文件大小 |
| 任务退出超时 | 5s | 5s | 优雅退出等待时间 |
| 死锁检测阈值 | 5s | 5s | 互斥锁等待超时 |

## 6. 可靠性设计

### 6.1 故障检测

| 故障类型 | 检测方式 | 恢复策略 |
|---------|---------|---------|
| CAN总线故障 | 错误计数统计 | 自动重试（1次）、日志记录 |
| 以太网故障 | 连续5次失败 | 自动切换到UART |
| 设备离线 | 超时检测 | 返回离线状态码、定期重试 |
| 任务死锁 | 死锁检测（5秒） | 触发回调、记录日志 |
| 内存泄漏 | 引用计数 | 自动释放资源 |

### 6.2 容错机制

- **引用计数**：防止use-after-free
- **优雅退出**：避免强制取消导致的死锁
- **死锁检测**：监控互斥锁等待时间
- **日志轮转**：防止磁盘占满
- **通信冗余**：双通道自动切换
- **错误重试**：自动重试失败操作

### 6.3 调试支持

- **日志系统**：多级别日志、文件轮转、彩色终端
- **统计信息**：收发计数、错误计数、成功率
- **调试模式**：Debug编译、GDB支持
- **测试框架**：58+测试用例覆盖4层架构（OSAL/HAL/PCL/PDL）

## 7. 可移植性设计

### 7.1 平台抽象

**OSAL层**：封装操作系统API，支持跨平台移植

**支持平台**：
- Linux（当前实现）
- FreeRTOS（预留）
- VxWorks（预留）
- RTEMS（预留）

### 7.2 移植步骤

1. 实现OSAL接口（`core/osal/src/<platform>/`）
2. 实现HAL驱动（`core/hal/src/<platform>/`）
3. 修改CMakeLists.txt添加平台支持
4. 运行测试验证

### 7.3 接口稳定性

**OSAL接口**：运行环境，为所有层提供统一接口，保持API稳定

**HAL接口**：硬件抽象，使用OSAL接口

**PDL接口**：外设服务接口，使用OSAL接口，与平台无关

**Apps接口**：业务逻辑，使用OSAL接口，调用PDL接口

## 8. 未来扩展

### 8.1 计划功能

- [ ] 看门狗支持（硬件看门狗 + 软件看门狗）
- [ ] 持久化队列（掉电保护）
- [ ] Redfish协议完整实现
- [ ] 固件升级功能（BMC/MCU）
- [ ] 远程调试接口（GDB Server）
- [ ] 性能监控（CPU/内存/网络）

### 8.2 平台支持

- [ ] FreeRTOS移植
- [ ] VxWorks移植
- [ ] RTEMS移植
- [ ] Zephyr移植

### 8.3 硬件支持

- [ ] SpaceWire接口
- [ ] 1553B总线
- [ ] PCIe接口
- [ ] MIPI CSI-2接口

## 9. 参考资料

- POSIX标准文档
- IPMI规范：https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-home.html
- Redfish规范：https://www.dmtf.org/standards/redfish
- SocketCAN文档：https://www.kernel.org/doc/Documentation/networking/can.txt
- Linux termios：https://man7.org/linux/man-pages/man3/termios.3.html

## 10. 版本历史

### v1.0.0 (2026-04-25)
- 完成5层架构设计（OSAL/HAL/PCL/PDL/Apps）
- 实现CAN网关和协议转换应用
- 完成70个测试用例覆盖
- 模块化配置重构
- Service层重命名为PDL层
- 新增PCL硬件配置库
- 优雅退出机制
- 引用计数保护
- 死锁检测
- 日志轮转
- 通信冗余
- 代码规模：约18,000行（生产代码14,000行，测试代码4,000行）

---

**最后更新**: 2026-05-22  
**维护者**: wanguo
