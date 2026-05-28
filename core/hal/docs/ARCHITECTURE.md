# HAL 架构设计

## 设计理念

HAL (Hardware Abstraction Layer) 采用**平台隔离**设计，将硬件相关代码封装在平台特定目录中，为上层提供统一的硬件访问接口。

## 核心原则

1. **平台隔离**：硬件相关代码隔离在`src/<platform>/`目录
2. **OSAL依赖**：必须使用OSAL封装的系统调用，禁止直接调用`socket()`, `open()`, `close()`等
3. **统一接口**：所有平台实现相同的API
4. **句柄管理**：使用句柄封装硬件资源

## 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    HAL Public API                        │
│  (hal_can.h, hal_serial.h - 平台无关接口)                │
└─────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
┌───────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐
│  Linux实现   │  │  TI AM62    │  │  NXP i.MX8  │
│  SocketCAN   │  │  硬件CAN    │  │  硬件CAN    │
│  termios     │  │  硬件UART   │  │  硬件UART   │
└───────┬──────┘  └──────┬──────┘  └──────┬──────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          │
┌─────────────────────────▼─────────────────────────────┐
│                    OSAL Layer                          │
│  OSAL_socket(), OSAL_open(), OSAL_close(), etc.      │
└───────────────────────────────────────────────────────┘
```

## CAN驱动架构

### 数据结构

```c
/* CAN配置 */
typedef struct {
    const char *device;      /* 设备名（如"can0"） */
    uint32_t bitrate;         /* 波特率 */
    hal_can_mode_t mode;      /* 模式（正常/环回/静默） */
} hal_can_config_t;

/* CAN句柄 */
typedef struct {
    int32_t sockfd;           /* Socket文件描述符 */
    char device[32];         /* 设备名 */
    uint32_t bitrate;         /* 波特率 */
    bool is_open;             /* 打开标志 */
    hal_can_stats_t stats;    /* 统计信息 */
} hal_can_handle_t;

/* CAN帧 */
typedef struct {
    uint32_t can_id;          /* CAN ID */
    uint8_t can_dlc;          /* 数据长度 */
    uint8_t data[8];          /* 数据 */
} hal_can_frame_t;
```

### Linux实现（SocketCAN）

**初始化流程**：
```
1. 使用OSAL_socket()创建CAN socket
2. 使用OSAL_ioctl()获取接口索引
3. 使用OSAL_bind()绑定到CAN接口
4. 设置socket选项（超时、过滤器等）
```

**发送流程**：
```
1. 构造CAN帧
2. 使用OSAL_write()发送
3. 更新统计信息
```

**接收流程**：
```
1. 使用OSAL_read()接收（带超时）
2. 解析CAN帧
3. 更新统计信息
```

**关键代码**：
```c
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
    /* 创建socket（使用OSAL封装） */
    int32_t sockfd = OSAL_socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0) {
        return OS_ERROR;
    }
    
    /* 获取接口索引 */
    struct ifreq ifr;
    OSAL_Strcpy(ifr.ifr_name, config->device);
    OSAL_ioctl(sockfd, SIOCGIFINDEX, &ifr);
    
    /* 绑定接口 */
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    OSAL_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    
    /* 初始化句柄 */
    handle->sockfd = sockfd;
    handle->is_open = true;
    
    return OS_SUCCESS;
}
```

## 串口驱动架构

### 数据结构

```c
/* 串口配置 */
typedef struct {
    const char *device;      /* 设备路径（如"/dev/ttyS0"） */
    uint32_t baudrate;        /* 波特率 */
    uint8_t data_bits;        /* 数据位（5-8） */
    uint8_t stop_bits;        /* 停止位（1-2） */
    char parity;             /* 校验位（'N'/'E'/'O'） */
} hal_serial_config_t;

/* 串口句柄 */
typedef struct {
    int32_t fd;               /* 文件描述符 */
    char device[64];         /* 设备路径 */
    uint32_t baudrate;        /* 波特率 */
    bool is_open;             /* 打开标志 */
} hal_serial_handle_t;
```

### Linux实现（termios）

**初始化流程**：
```
1. 使用OSAL_open()打开串口设备
2. 使用OSAL_tcgetattr()获取当前配置
3. 配置波特率、数据位、停止位、校验位
4. 设置原始模式（禁用回显、规范模式等）
5. 使用OSAL_tcsetattr()应用配置
```

**关键代码**：
```c
int32_t HAL_Serial_Init(const hal_serial_config_t *config, hal_serial_handle_t *handle)
{
    /* 打开串口（使用OSAL封装） */
    int32_t fd = OSAL_open(config->device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return OS_ERROR;
    }
    
    /* 获取当前配置 */
    struct termios tty;
    OSAL_tcgetattr(fd, &tty);
    
    /* 设置波特率 */
    speed_t speed = baudrate_to_speed(config->baudrate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    
    /* 设置数据位、停止位、校验位 */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= (config->data_bits == 8) ? CS8 : CS7;
    tty.c_cflag |= (config->stop_bits == 2) ? CSTOPB : 0;
    
    /* 设置原始模式 */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    
    /* 应用配置 */
    OSAL_tcsetattr(fd, TCSANOW, &tty);
    
    /* 初始化句柄 */
    handle->fd = fd;
    handle->is_open = true;
    
    return OS_SUCCESS;
}
```

## 平台移植

### 添加新平台

1. 创建平台目录：`src/<platform_name>/`
2. 实现所有HAL接口
3. 修改CMakeLists.txt选择平台实现

**示例**：添加TI AM6254平台
```
hal/
├── src/
│   ├── linux/              # Linux实现
│   │   ├── hal_can_linux.c
│   │   └── hal_serial_linux.c
│   └── ti_am62/            # TI AM6254实现（新增）
│       ├── hal_can_ti.c
│       └── hal_serial_ti.c
```

### 平台选择

通过CMake选项选择平台：
```cmake
set(HAL_PLATFORM "linux" CACHE STRING "HAL platform")

if(HAL_PLATFORM STREQUAL "linux")
    set(HAL_SOURCES src/linux/hal_can_linux.c src/linux/hal_serial_linux.c)
elseif(HAL_PLATFORM STREQUAL "ti_am62")
    set(HAL_SOURCES src/ti_am62/hal_can_ti.c src/ti_am62/hal_serial_ti.c)
endif()
```

## 错误处理

所有HAL函数返回int32状态码：
```c
OS_SUCCESS              /* 成功 */
OS_ERROR                /* 通用错误 */
OS_INVALID_POINTER      /* 无效指针 */
OS_ERROR_TIMEOUT        /* 超时 */
OS_ERR_NO_MEMORY        /* 内存不足 */
```

## 性能指标

- **CAN发送延迟**：< 1ms
- **CAN接收延迟**：< 2ms
- **串口发送延迟**：< 1ms
- **串口接收延迟**：取决于波特率和数据量

## 相关文档

- [API参考](API_REFERENCE.md)
- [使用指南](USAGE_GUIDE.md)
- [OSAL架构](../../osal/docs/ARCHITECTURE.md)
