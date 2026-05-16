# HAL层详细设计

## 4. HAL 设计（硬件抽象层）

### 4.1 设计原则

**HAL（Hardware Abstraction Layer）是硬件抽象层，提供统一的硬件驱动接口，屏蔽不同平台的硬件差异。**

#### 4.1.1 核心理念

- ✅ **硬件抽象**：封装硬件寄存器操作，提供统一的驱动接口
- ✅ **平台隔离**：Linux/RTOS平台实现分离，上层代码无需修改
- ✅ **句柄管理**：使用不透明句柄，隐藏内部实现细节
- ✅ **OSAL依赖**：所有系统调用必须通过OSAL封装，不直接调用系统API

#### 4.1.2 HAL层职责

**HAL层负责**：
- ✅ 硬件设备初始化和配置
- ✅ 数据收发接口（阻塞/非阻塞/超时）
- ✅ 错误处理和统计信息
- ✅ 平台特定实现（Linux/RTOS）

**HAL层禁止**：
- ❌ 包含业务逻辑（业务逻辑在PDL层）
- ❌ 直接调用系统API（必须通过OSAL）
- ❌ 跨平台代码混合（平台实现必须分离）

#### 4.1.3 HAL层架构

```
hal/
├── include/                        # HAL公开接口
│   ├── hal_can.h                  # CAN驱动接口
│   ├── hal_serial.h               # 串口驱动接口
│   ├── hal_i2c.h                  # I2C驱动接口
│   ├── hal_spi.h                  # SPI驱动接口
│   ├── hal_watchdog.h             # 看门狗接口
│   └── config/                    # 配置类型定义
│       ├── can_types.h            # CAN类型定义
│       ├── i2c_types.h            # I2C类型定义
│       └── spi_types.h            # SPI类型定义
└── src/
    ├── linux/                     # Linux平台实现
    │   ├── hal_can_linux.c
    │   ├── hal_serial_linux.c
    │   ├── hal_i2c_linux.c
    │   └── hal_spi_linux.c
    └── rtos/                      # RTOS平台实现（预留）
        └── ...
```

---

### 4.2 CAN驱动设计

**功能**：提供统一的CAN总线访问接口，支持标准帧和扩展帧。

#### 4.2.1 接口设计

```c
/* CAN句柄（不透明） */
typedef void* hal_can_handle_t;

/* CAN配置 */
typedef struct {
    const char *interface;       /* CAN接口名（如"can0"） */
    uint32_t    baudrate;        /* 波特率（如500000） */
    uint32_t    rx_timeout;      /* 接收超时（ms） */
    uint32_t    tx_timeout;      /* 发送超时（ms） */
} hal_can_config_t;

/* CAN帧结构（标准定义） */
typedef struct {
    uint32_t can_id;             /* CAN ID（11位或29位） */
    uint8_t  can_dlc;            /* 数据长度（0-8） */
    uint8_t  data[8];            /* 数据 */
    uint8_t  flags;              /* 标志（扩展帧/远程帧） */
} can_frame_t;

/* 初始化CAN驱动 */
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle);

/* 关闭CAN驱动 */
int32_t HAL_CAN_Deinit(hal_can_handle_t handle);

/* 发送CAN帧 */
int32_t HAL_CAN_Send(hal_can_handle_t handle, const can_frame_t *frame);

/* 接收CAN帧 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, can_frame_t *frame, int32_t timeout);

/* 设置CAN过滤器 */
int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask);

/* 获取CAN统计信息 */
int32_t HAL_CAN_GetStats(hal_can_handle_t handle, uint32_t *tx_count, uint32_t *rx_count, uint32_t *err_count);

/* 设置错误回调 */
int32_t HAL_CAN_SetErrorCallback(hal_can_handle_t handle, void (*callback)(hal_can_handle_t handle, int32_t error_code));

/* 设置错误恢复阈值 */
int32_t HAL_CAN_SetErrorThreshold(hal_can_handle_t handle, uint32_t threshold);
```

#### 4.2.2 Linux平台实现要点

```c
/* 内部句柄结构 */
typedef struct {
    int32_t  sockfd;             /* SocketCAN文件描述符 */
    char     interface[16];      /* 接口名 */
    uint32_t baudrate;
    uint32_t rx_timeout;
    uint32_t tx_timeout;
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t err_count;
    void (*error_callback)(hal_can_handle_t, int32_t);
} hal_can_context_t;

/* 初始化实现（Linux SocketCAN） */
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle) {
    hal_can_context_t *ctx = OSAL_Malloc(sizeof(hal_can_context_t));
    
    /* 1. 创建SocketCAN套接字（使用OSAL封装） */
    ctx->sockfd = OSAL_socket(PF_CAN, SOCK_RAW, CAN_RAW);
    
    /* 2. 绑定到CAN接口 */
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(config->interface);
    OSAL_bind(ctx->sockfd, (struct sockaddr *)&addr, sizeof(addr));
    
    /* 3. 设置超时 */
    struct timeval tv;
    tv.tv_sec = config->rx_timeout / 1000;
    tv.tv_usec = (config->rx_timeout % 1000) * 1000;
    OSAL_setsockopt(ctx->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    *handle = (hal_can_handle_t)ctx;
    return OSAL_SUCCESS;
}

/* 发送实现 */
int32_t HAL_CAN_Send(hal_can_handle_t handle, const can_frame_t *frame) {
    hal_can_context_t *ctx = (hal_can_context_t *)handle;
    
    /* 构造Linux CAN帧 */
    struct can_frame linux_frame;
    linux_frame.can_id = frame->can_id;
    linux_frame.can_dlc = frame->can_dlc;
    OSAL_Memcpy(linux_frame.data, frame->data, frame->can_dlc);
    
    /* 发送（使用OSAL封装） */
    int32_t ret = OSAL_write(ctx->sockfd, &linux_frame, sizeof(linux_frame));
    if (ret == sizeof(linux_frame)) {
        ctx->tx_count++;
        return OSAL_SUCCESS;
    }
    
    ctx->err_count++;
    return OSAL_ERR_GENERIC;
}

/* 接收实现 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, can_frame_t *frame, int32_t timeout) {
    hal_can_context_t *ctx = (hal_can_context_t *)handle;
    
    /* 设置超时（如果指定） */
    if (timeout >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        OSAL_setsockopt(ctx->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    
    /* 接收（使用OSAL封装） */
    struct can_frame linux_frame;
    int32_t ret = OSAL_read(ctx->sockfd, &linux_frame, sizeof(linux_frame));
    if (ret == sizeof(linux_frame)) {
        frame->can_id = linux_frame.can_id;
        frame->can_dlc = linux_frame.can_dlc;
        OSAL_Memcpy(frame->data, linux_frame.data, linux_frame.can_dlc);
        ctx->rx_count++;
        return OSAL_SUCCESS;
    }
    
    if (ret == OSAL_ERR_TIMEOUT) {
        return OSAL_ERR_TIMEOUT;
    }
    
    ctx->err_count++;
    return OSAL_ERR_GENERIC;
}
```

---

### 4.3 串口驱动设计

**功能**：提供统一的串口访问接口，支持多种波特率和配置。

#### 4.3.1 接口设计

```c
/* 串口句柄（不透明） */
typedef void* hal_serial_handle_t;

/* 串口配置 */
typedef struct {
    uint32_t baud_rate;          /* 波特率 */
    uint8_t  data_bits;          /* 数据位（5/6/7/8） */
    uint8_t  stop_bits;          /* 停止位（1/2） */
    uint8_t  parity;             /* 校验位（NONE/ODD/EVEN） */
    uint8_t  flow_control;       /* 流控（NONE/HW/SW） */
} hal_serial_config_t;

/* 打开串口设备 */
int32_t HAL_Serial_Open(const char *device, const hal_serial_config_t *config, hal_serial_handle_t *handle);

/* 关闭串口设备 */
int32_t HAL_Serial_Close(hal_serial_handle_t handle);

/* 写入数据 */
int32_t HAL_Serial_Write(hal_serial_handle_t handle, const void *buffer, uint32_t size, int32_t timeout);

/* 读取数据 */
int32_t HAL_Serial_Read(hal_serial_handle_t handle, void *buffer, uint32_t size, int32_t timeout);

/* 刷新缓冲区 */
int32_t HAL_Serial_Flush(hal_serial_handle_t handle);

/* 设置串口配置 */
int32_t HAL_Serial_SetConfig(hal_serial_handle_t handle, const hal_serial_config_t *config);
```

#### 4.3.2 Linux平台实现要点

```c
/* 内部句柄结构 */
typedef struct {
    int32_t fd;                  /* 文件描述符 */
    char    device[64];          /* 设备路径 */
    hal_serial_config_t config;  /* 当前配置 */
} hal_serial_context_t;

/* 打开串口实现 */
int32_t HAL_Serial_Open(const char *device, const hal_serial_config_t *config, hal_serial_handle_t *handle) {
    hal_serial_context_t *ctx = OSAL_Malloc(sizeof(hal_serial_context_t));
    
    /* 1. 打开设备（使用OSAL封装） */
    ctx->fd = OSAL_open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (ctx->fd < 0) {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }
    
    /* 2. 配置串口参数 */
    struct termios options;
    OSAL_tcgetattr(ctx->fd, &options);
    
    /* 设置波特率 */
    speed_t baud = B115200;  /* 根据config->baud_rate映射 */
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    
    /* 设置数据位 */
    options.c_cflag &= ~CSIZE;
    switch (config->data_bits) {
        case 5: options.c_cflag |= CS5; break;
        case 6: options.c_cflag |= CS6; break;
        case 7: options.c_cflag |= CS7; break;
        case 8: options.c_cflag |= CS8; break;
    }
    
    /* 设置校验位 */
    if (config->parity == HAL_SERIAL_PARITY_NONE) {
        options.c_cflag &= ~PARENB;
    } else if (config->parity == HAL_SERIAL_PARITY_ODD) {
        options.c_cflag |= PARENB | PARODD;
    } else if (config->parity == HAL_SERIAL_PARITY_EVEN) {
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
    }
    
    /* 设置停止位 */
    if (config->stop_bits == 2) {
        options.c_cflag |= CSTOPB;
    } else {
        options.c_cflag &= ~CSTOPB;
    }
    
    /* 应用配置 */
    OSAL_tcsetattr(ctx->fd, TCSANOW, &options);
    
    OSAL_Memcpy(&ctx->config, config, sizeof(hal_serial_config_t));
    *handle = (hal_serial_handle_t)ctx;
    return OSAL_SUCCESS;
}
```

---

### 4.4 I2C驱动设计

**功能**：提供统一的I2C总线访问接口，支持标准速率和快速速率。

#### 4.4.1 接口设计

```c
/* I2C句柄（不透明） */
typedef void* hal_i2c_handle_t;

/* I2C配置 */
typedef struct {
    const char *device;          /* I2C设备（如"/dev/i2c-0"） */
    uint32_t    timeout;         /* 传输超时（ms） */
} hal_i2c_config_t;

/* I2C消息结构 */
typedef struct {
    uint16_t addr;               /* 从设备地址（7位） */
    uint16_t flags;              /* 标志（读/写） */
    uint16_t len;                /* 数据长度 */
    uint8_t *buf;                /* 数据缓冲区 */
} i2c_msg_t;

/* 打开I2C设备 */
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle);

/* 关闭I2C设备 */
int32_t HAL_I2C_Close(hal_i2c_handle_t handle);

/* 写入数据到I2C从设备 */
int32_t HAL_I2C_Write(hal_i2c_handle_t handle, uint16_t slave_addr, const uint8_t *buffer, uint32_t size);

/* 从I2C从设备读取数据 */
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr, uint8_t *buffer, uint32_t size);

/* 写寄存器操作 */
int32_t HAL_I2C_WriteReg(hal_i2c_handle_t handle, uint16_t slave_addr, uint8_t reg_addr, const uint8_t *buffer, uint32_t size);

/* 读寄存器操作 */
int32_t HAL_I2C_ReadReg(hal_i2c_handle_t handle, uint16_t slave_addr, uint8_t reg_addr, uint8_t *buffer, uint32_t size);

/* 执行I2C传输（支持组合传输） */
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, i2c_msg_t *msgs, uint32_t num);
```

#### 4.4.2 设计特点

- **寄存器访问**：提供专用的寄存器读写接口，简化常见操作
- **组合传输**：支持I2C组合传输（先写后读），避免总线释放
- **超时保护**：所有操作支持超时，防止总线挂死

---

### 4.5 SPI驱动设计

**功能**：提供统一的SPI总线访问接口，支持全双工传输。

#### 4.5.1 接口设计

```c
/* SPI句柄（不透明） */
typedef void* hal_spi_handle_t;

/* SPI配置 */
typedef struct {
    const char *device;          /* SPI设备（如"/dev/spidev0.0"） */
    uint8_t     mode;            /* SPI模式（0-3） */
    uint8_t     bits_per_word;   /* 每字位数（通常为8） */
    uint32_t    max_speed_hz;    /* 最大速率（Hz） */
    uint32_t    timeout;         /* 传输超时（ms） */
} hal_spi_config_t;

/* SPI传输结构 */
typedef struct {
    const uint8_t *tx_buf;       /* 发送缓冲区 */
    uint8_t       *rx_buf;       /* 接收缓冲区 */
    uint32_t       len;          /* 传输长度 */
    uint32_t       speed_hz;     /* 传输速率 */
    uint16_t       delay_usecs;  /* 传输后延迟（us） */
    uint8_t        bits_per_word;/* 每字位数 */
    uint8_t        cs_change;    /* 片选变化标志 */
} spi_transfer_t;

/* 打开SPI设备 */
int32_t HAL_SPI_Open(const hal_spi_config_t *config, hal_spi_handle_t *handle);

/* 关闭SPI设备 */
int32_t HAL_SPI_Close(hal_spi_handle_t handle);

/* SPI写操作 */
int32_t HAL_SPI_Write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size);

/* SPI读操作 */
int32_t HAL_SPI_Read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size);

/* SPI全双工传输 */
int32_t HAL_SPI_Transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer, uint8_t *rx_buffer, uint32_t size);

/* SPI批量传输（支持多段传输） */
int32_t HAL_SPI_TransferMulti(hal_spi_handle_t handle, spi_transfer_t *transfers, uint32_t num);

/* 设置SPI配置 */
int32_t HAL_SPI_SetConfig(hal_spi_handle_t handle, const hal_spi_config_t *config);
```

#### 4.5.2 设计特点

- **全双工支持**：同时发送和接收数据
- **批量传输**：支持多段传输，减少片选切换开销
- **灵活配置**：支持运行时修改速率、模式等参数

---

### 4.6 看门狗驱动设计

**功能**：提供统一的看门狗接口，用于系统复位保护。

#### 4.6.1 接口设计

```c
/* 看门狗句柄（不透明） */
typedef void* hal_watchdog_handle_t;

/* 看门狗配置 */
typedef struct {
    const char *device;          /* 看门狗设备（如"/dev/watchdog"） */
    uint32_t    timeout_sec;     /* 超时时间（秒） */
    bool        enable_on_open;  /* 打开时自动启用 */
} hal_watchdog_config_t;

/* 打开看门狗设备 */
int32_t HAL_Watchdog_Open(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle);

/* 关闭看门狗设备 */
int32_t HAL_Watchdog_Close(hal_watchdog_handle_t handle);

/* 喂狗 */
int32_t HAL_Watchdog_Kick(hal_watchdog_handle_t handle);

/* 启用看门狗 */
int32_t HAL_Watchdog_Enable(hal_watchdog_handle_t handle);

/* 禁用看门狗 */
int32_t HAL_Watchdog_Disable(hal_watchdog_handle_t handle);

/* 设置超时时间 */
int32_t HAL_Watchdog_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec);

/* 获取剩余时间 */
int32_t HAL_Watchdog_GetTimeLeft(hal_watchdog_handle_t handle, uint32_t *time_left_sec);
```

---

### 4.7 HAL层设计要点

#### 4.7.1 句柄管理模式

**所有HAL驱动使用统一的句柄管理模式**：

```c
/* 1. 不透明句柄类型 */
typedef void* hal_xxx_handle_t;

/* 2. 内部上下文结构（对外不可见） */
typedef struct {
    /* 硬件相关字段 */
    int32_t fd;
    /* 配置字段 */
    hal_xxx_config_t config;
    /* 统计字段 */
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t err_count;
} hal_xxx_context_t;

/* 3. 初始化时分配上下文 */
int32_t HAL_XXX_Init(const hal_xxx_config_t *config, hal_xxx_handle_t *handle) {
    hal_xxx_context_t *ctx = OSAL_Malloc(sizeof(hal_xxx_context_t));
    /* 初始化硬件 */
    *handle = (hal_xxx_handle_t)ctx;
    return OSAL_SUCCESS;
}

/* 4. 操作时转换句柄 */
int32_t HAL_XXX_Operation(hal_xxx_handle_t handle, ...) {
    hal_xxx_context_t *ctx = (hal_xxx_context_t *)handle;
    /* 使用ctx访问内部字段 */
}

/* 5. 反初始化时释放上下文 */
int32_t HAL_XXX_Deinit(hal_xxx_handle_t handle) {
    hal_xxx_context_t *ctx = (hal_xxx_context_t *)handle;
    /* 关闭硬件 */
    OSAL_Free(ctx);
    return OSAL_SUCCESS;
}
```

#### 4.7.2 错误处理

**统一的错误码和错误处理机制**：

```c
/* 错误码（来自OSAL层） */
#define OSAL_SUCCESS        0
#define OSAL_ERR_GENERIC   -1
#define OSAL_ERR_TIMEOUT   -2
#define OSAL_ERR_INVALID_POINTER -3
#define OSAL_ERR_NOT_SUPPORTED -4

/* 所有HAL接口返回int32_t状态码 */
int32_t HAL_XXX_Operation(...) {
    if (/* 参数检查失败 */) {
        return OSAL_ERR_INVALID_POINTER;
    }
    
    if (/* 操作超时 */) {
        return OSAL_ERR_TIMEOUT;
    }
    
    if (/* 操作失败 */) {
        return OSAL_ERR_GENERIC;
    }
    
    return OSAL_SUCCESS;
}
```

#### 4.7.3 平台隔离

**Linux和RTOS平台实现完全分离**：

```
hal/src/
├── linux/                   # Linux平台实现
│   ├── hal_can_linux.c     # 使用SocketCAN
│   ├── hal_serial_linux.c  # 使用termios
│   ├── hal_i2c_linux.c     # 使用i2c-dev
│   └── hal_spi_linux.c     # 使用spidev
└── rtos/                    # RTOS平台实现（预留）
    ├── hal_can_rtos.c      # 使用RTOS CAN驱动
    ├── hal_serial_rtos.c   # 使用RTOS串口驱动
    ├── hal_i2c_rtos.c      # 使用RTOS I2C驱动
    └── hal_spi_rtos.c      # 使用RTOS SPI驱动
```

**编译时选择平台实现**：

```cmake
# CMakeLists.txt
if(PLATFORM STREQUAL "linux")
    target_sources(hal PRIVATE
        src/linux/hal_can_linux.c
        src/linux/hal_serial_linux.c
        src/linux/hal_i2c_linux.c
        src/linux/hal_spi_linux.c
    )
elseif(PLATFORM STREQUAL "rtos")
    target_sources(hal PRIVATE
        src/rtos/hal_can_rtos.c
        src/rtos/hal_serial_rtos.c
        src/rtos/hal_i2c_rtos.c
        src/rtos/hal_spi_rtos.c
    )
endif()
```

#### 4.7.4 OSAL依赖

**HAL层所有系统调用必须通过OSAL封装**：

```c
/* ❌ 错误：直接调用系统API */
int fd = open("/dev/can0", O_RDWR);
write(fd, buffer, size);
close(fd);

/* ✅ 正确：使用OSAL封装 */
int32_t fd = OSAL_open("/dev/can0", O_RDWR);
OSAL_write(fd, buffer, size);
OSAL_close(fd);
```

**优势**：
- ✅ 平台无关：OSAL层处理平台差异
- ✅ 类型安全：使用OSAL固定大小类型（int32_t而非int）
- ✅ 错误统一：OSAL统一错误码和错误处理

---

