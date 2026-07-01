# PDM 子系统架构详解

## 目录

1. [架构概览](#一架构概览)
2. [分层架构图](#二分层架构图)
3. [核心框架详解](#三核心框架详解)
4. [传输层详解](#四传输层详解)
5. [完整初始化流程](#五完整初始化流程)
6. [设备树绑定](#六设备树绑定)
7. [关键设计模式](#七关键设计模式)
8. [目录结构](#八目录结构)

---

## 一、架构概览

PDM (Peripheral Device Manager) 是一个 Linux 内核框架，用于通过统一的总线架构管理嵌入式外设设备。它实现了**分层驱动模型**，将设备逻辑与传输机制分离，支持运行时选择后端（I2C/SPI/UART/CAN）。

### 1.1 核心特性

- **基于总线的架构**：实现标准 Linux `bus_type`，自动设备-驱动匹配
- **传输抽象层**：通过设备树选择可插拔后端（I2C/SPI/UART/CAN）
- **基于链接器的注册**：使用 ELF 段标记自动发现驱动和后端
- **设备树驱动**：通过 compatible 字符串匹配实现零配置枚举
- **稳定的设备编号**：基于 IDA 的 ID 分配确保一致的 `/dev/pdm/*` 节点映射
- **多层诊断**：DebugFS、procfs 和 sysfs 接口用于运行时检查

### 1.2 支持的外设

- **LED 设备**：GPIO 和 PWM 控制后端
- **MCU 设备**：多传输通信（UART/I2C/SPI/CAN）+ 协议层

### 1.3 代码规模

约 **2135 行**核心框架代码，分布在 **30 个源文件**

---

## 二、分层架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         用户空间                                     │
├─────────────────────────────────────────────────────────────────────┤
│  /dev/pdm/led0      │  /dev/pdm/mcu0  │  /proc/pdm  │  /sys/bus/pdm│
└──────────────┬──────┴──────────┬───────┴──────┬──────┴──────┬───────┘
               │                 │              │             │
┌──────────────┴─────────────────┴──────────────┴─────────────┴───────┐
│                     字符设备层                                        │
│  pdm_cdev_register() - 创建 /dev 节点，绑定 inode/fops              │
└──────────────────────────────────┬───────────────────────────────────┘
                                   │
┌──────────────────────────────────┴───────────────────────────────────┐
│                        驱动层                                         │
├──────────────────────────────┬───────────────────────────────────────┤
│   pdm_led_driver             │   pdm_mcu_driver                      │
│   - probe/remove/match       │   - probe/remove/match                │
│   - LED 状态机               │   - 协议层                            │
│   - 后端选择                 │   - 传输操作分发                      │
└──────────────┬───────────────┴──────────────┬────────────────────────┘
               │                              │
┌──────────────┴──────────────────────────────┴────────────────────────┐
│                    PDM 总线基础设施                                   │
│  pdm_bus_type - 标准 Linux bus_type，包含：                         │
│    • 设备-驱动匹配（compatible 字符串 + 自定义 match()）            │
│    • Probe/remove 分发                                               │
│    • 电源管理钩子                                                    │
│    • Sysfs/uevent 集成                                              │
│                                                                       │
│  pdm_device - 设备模型，包含：                                       │
│    • 类型绑定（LED/MCU）                                             │
│    • ID 分配（每种设备类型一个 IDA）                                 │
│    • 状态跟踪                                                        │
│    • 能力标志                                                        │
└───────────────────────────────┬───────────────────────────────────────┘
                                │
┌───────────────────────────────┴───────────────────────────────────────┐
│                   后端注册表层                                         │
│  pdm_backend_find() - 运行时后端查找，根据：                          │
│    • device_type (PDM_LED_DEVICE_TYPE / PDM_MCU_DEVICE_TYPE)         │
│    • backend_class (CONTROL / TRANSPORT)                             │
│    • compatible 字符串匹配                                            │
│                                                                       │
│  链接器段：__start_pdm_backend_entries → __stop_...                  │
│    [pdm_led_gpio_entry] [pdm_led_pwm_entry] [pdm_mcu_i2c_entry] ... │
└──────────────┬────────────────────────────────────┬───────────────────┘
               │                                    │
    ┌──────────┴──────────┐            ┌───────────┴──────────────────┐
    │  控制后端           │            │   传输后端                    │
    ├─────────────────────┤            ├───────────────────────────────┤
    │ pdm_led_gpio_ops    │            │ pdm_mcu_i2c_ops               │
    │  - setup/cleanup    │            │  - setup/cleanup/xfer         │
    │  - apply            │            │                               │
    │                     │            │ pdm_mcu_spi_ops               │
    │ pdm_led_pwm_ops     │            │  - setup/cleanup/xfer         │
    │  - setup/cleanup    │            │                               │
    │  - apply            │            │ pdm_mcu_uart_ops              │
    └──────────┬──────────┘            │  - setup/cleanup/xfer         │
               │                       │                               │
               │                       │ pdm_mcu_can_ops               │
               │                       │  - setup/cleanup/xfer         │
               │                       └───────────┬───────────────────┘
               │                                   │
┌──────────────┴───────────────────────────────────┴───────────────────┐
│                     Linux 内核子系统                                  │
├───────────────────────────────────────────────────────────────────────┤
│  GPIO    │  PWM    │  I2C Core  │  SPI Core  │  TTY/Serdev  │  CAN   │
│  Layer   │  Layer  │            │            │              │  Layer │
└──────────┴─────────┴────────────┴────────────┴──────────────┴────────┘
               │                                   │
┌──────────────┴───────────────────────────────────┴───────────────────┐
│                        硬件层                                         │
│  GPIO 引脚  │  PWM 控制器  │  I2C/SPI/UART 控制器  │  CAN           │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 三、核心框架详解

### 3.1 总线基础设施 (`bus/` - 5 个文件)

#### 3.1.1 pdm_bus.c (252 行)

实现 `pdm_bus_type`，这是整个 PDM 框架的核心：

```c
struct bus_type pdm_bus_type = {
    .name       = "pdm",
    .match      = pdm_bus_device_match,      // 三层匹配算法
    .probe      = pdm_bus_device_probe,      // 设备绑定编排
    .remove     = pdm_bus_device_remove,
    .uevent     = pdm_bus_uevent,
    .pm         = &pdm_bus_pm_ops,
};
```

**关键操作：**

##### 1. pdm_bus_device_match() (101-135 行)

三层匹配算法，按优先级顺序：

```c
static int pdm_bus_device_match(struct device *dev, struct device_driver *drv)
{
    struct pdm_device *pdm_dev = to_pdm_device(dev);
    struct pdm_driver *pdm_drv = to_pdm_driver(drv);
    
    // 第 1 层：自定义驱动 match() 回调
    if (pdm_drv->match) {
        return pdm_drv->match(pdm_dev);
        // 例如 pdm_led_match() 检查后端可用性
    }
    
    // 第 2 层：设备树 of_match_table 比较
    if (pdm_drv->driver.of_match_table) {
        return of_match_device(pdm_drv->driver.of_match_table, dev) != NULL;
    }
    
    // 第 3 层：手动 compatible 字符串迭代
    if (pdm_dev->compatible) {
        // 遍历 of_match_table 进行字符串比较
    }
    
    return 0;
}
```

**匹配示例：**

- LED 设备（compatible = "pdm,led-gpio"）匹配 `pdm_led_driver`
- 在 `pdm_led_match()` 中检查是否存在 GPIO 后端
- 如果后端不可用，匹配失败，设备保持未绑定状态

##### 2. pdm_bus_device_probe() (26-62 行)

设备绑定编排器，执行以下步骤：

```c
static int pdm_bus_device_probe(struct device *dev)
{
    struct pdm_device *pdm_dev = to_pdm_device(dev);
    struct pdm_driver *pdm_drv = to_pdm_driver(dev->driver);
    int ret;
    
    // 步骤 1: 从 IDA 分配稳定 ID
    ret = pdm_device_bind(pdm_dev, device_type, capabilities);
    if (ret < 0)
        return ret;
    
    // 步骤 2: 调用驱动的 probe() 回调
    if (pdm_drv->probe) {
        ret = pdm_drv->probe(pdm_dev);
        if (ret < 0) {
            pdm_device_unbind(pdm_dev);
            return ret;
        }
    }
    
    // 步骤 3: 更新设备状态
    pdm_dev->state = PDM_MANAGER_DEVICE_STATE_BOUND;
    
    return 0;
}
```

##### 3. pdm_bus_register_driver() (214-238 行)

驱动注册辅助函数：

```c
int pdm_bus_register_driver(struct pdm_driver *pdm_drv)
{
    struct device_driver *drv = &pdm_drv->driver;
    
    // 设置标准字段
    drv->owner = THIS_MODULE;
    drv->bus = &pdm_bus_type;
    
    // 复制 of_match_table（如果提供）
    if (pdm_drv->of_match_table)
        drv->of_match_table = pdm_drv->of_match_table;
    
    // 调用 Linux 核心的 driver_register()
    return driver_register(drv);
}
```

#### 3.1.2 pdm_device.c (236 行)

设备生命周期管理：

##### 核心数据结构

```c
struct pdm_device {
    struct device dev;              // 嵌入式 Linux 设备
    const char *compatible;         // 主 compatible 字符串
    void *config_data;              // 驱动特定配置数据
    u32 type;                       // PDM_LED_DEVICE_TYPE, PDM_MCU_DEVICE_TYPE
    u64 capabilities;               // 能力位掩码
    u32 state;                      // REGISTERED, BOUND, ERROR
    s32 last_error;                 // 最后一次错误码
    u32 error_count;                // 错误计数器（用于 /dev/pdm_manager）
    
    /* 增强的错误跟踪 */
    struct pdm_error_record errors[PDM_ERROR_HISTORY_SIZE];  // 错误历史（循环缓冲）
    u32 error_write_index;          // 错误历史写入索引
    u32 total_error_count;          // 总错误计数
    
    int id;                         // 从 IDA 分配的稳定 ID
    int requested_id;               // DTS reg/pdm,id 属性
    bool id_allocated;              // ID 是否已分配标志
};

struct pdm_error_record {
    s32 error_code;                 // 错误码（负数 errno 值）
    u32 error_type;                 // 错误上下文（PROBE/RUNTIME/PM）
    u64 timestamp_ns;               // 错误发生时间戳（ktime_get_ns）
    u32 count;                      // 连续发生次数
};
```

##### 关键函数

**1. pdm_device_alloc() (136-154 行)**

分配并初始化 `pdm_device`：

```c
struct pdm_device *pdm_device_alloc(size_t config_size)
{
    struct pdm_device *pdm_dev;
    
    // 分配设备结构 + 配置数据
    pdm_dev = kzalloc(sizeof(*pdm_dev) + config_size, GFP_KERNEL);
    if (!pdm_dev)
        return NULL;
    
    // 初始化嵌入式设备
    device_initialize(&pdm_dev->dev);
    pdm_dev->dev.bus = &pdm_bus_type;
    pdm_dev->dev.release = pdm_device_release;  // 自动清理
    
    // 初始化状态
    pdm_dev->state = PDM_MANAGER_DEVICE_STATE_REGISTERED;
    pdm_dev->requested_id = -1;  // 默认自动分配
    
    if (config_size > 0)
        pdm_dev->config_data = pdm_dev + 1;
    
    return pdm_dev;
}
```

**2. pdm_device_bind() (211-236 行)**

类型绑定 + ID 分配：

```c
int pdm_device_bind(struct pdm_device *pdm_dev, u32 device_type, u32 capabilities)
{
    struct pdm_device_id_allocator *allocator;
    int id;
    
    pdm_dev->device_type = device_type;
    pdm_dev->capabilities = capabilities;
    
    // 查找类型特定的 IDA 分配器
    allocator = pdm_device_id_allocator_get(device_type);
    if (!allocator)
        return -EINVAL;
    
    // 从 IDA 分配 ID
    if (pdm_dev->requested_id >= 0) {
        // 请求特定 ID（来自 DTS reg 属性）
        id = ida_alloc_range(&allocator->ida, 
                           pdm_dev->requested_id, 
                           pdm_dev->requested_id, 
                           GFP_KERNEL);
    } else {
        // 自动分配下一个可用 ID
        id = ida_alloc(&allocator->ida, GFP_KERNEL);
    }
    
    if (id < 0)
        return id;
    
    pdm_dev->id = id;
    return 0;
}
```

**ID 分配示例：**

- LED 设备：`/dev/pdm/led0`, `/dev/pdm/led1`, ...
- MCU 设备：`/dev/pdm/mcu0`, `/dev/pdm/mcu1`, ...

**3. pdm_device_register() (157-184 行)**

向 Linux 驱动核心注册设备：

```c
int pdm_device_register(struct pdm_device *pdm_dev, const char *name)
{
    // 设置设备名称（例如 "led0", "mcu0"）
    dev_set_name(&pdm_dev->dev, name);
    
    // 注册设备（触发总线匹配）
    return device_add(&pdm_dev->dev);
    // 这会触发：
    // 1. pdm_bus_device_match() 对所有已注册的驱动
    // 2. 如果匹配，调用 pdm_bus_device_probe()
}
```

#### 3.1.3 pdm_of_bus.c (140 行)

设备树枚举：

```c
static int pdm_of_bus_probe(struct platform_device *pdev)
{
    struct device_node *child;
    struct pdm_device *pdm_dev;
    const char *compatible;
    int device_id;
    
    // 遍历所有子节点
    for_each_available_child_of_node(pdev->dev.of_node, child) {
        // 步骤 1: 读取 compatible 字符串
        if (of_property_read_string(child, "compatible", &compatible)) {
            dev_warn(&pdev->dev, "Missing compatible for %pOF\n", child);
            continue;
        }
        
        // 步骤 2: 读取设备 ID（reg 或 pdm,id 属性）
        device_id = pdm_of_bus_child_id(child);
        
        // 步骤 3: 分配 pdm_device
        pdm_dev = pdm_device_alloc(0);
        if (!pdm_dev) {
            of_node_put(child);
            continue;
        }
        
        // 步骤 4: 设置设备属性
        pdm_dev->dev.parent = &pdev->dev;
        pdm_dev->dev.of_node = of_node_get(child);
        pdm_dev->compatible = compatible;
        pdm_device_set_requested_id(pdm_dev, device_id);
        
        // 步骤 5: 注册设备（触发匹配）
        pdm_device_register(pdm_dev, child->name);
    }
    
    return 0;
}
```

**DTS 子节点 ID 解析** (82-93 行)：

```c
static int pdm_of_bus_child_id(struct device_node *node)
{
    u32 id;
    
    // 优先使用 pdm,id 属性
    if (!of_property_read_u32(node, "pdm,id", &id))
        return (int)id;
    
    // 回退到 reg 属性
    if (!of_property_read_u32(node, "reg", &id))
        return (int)id;
    
    // 如果都不存在，返回 -1（自动分配）
    return -1;
}
```

#### 3.1.4 pdm_cdev.c - 字符设备接口

创建用户空间可访问的设备节点：

```c
int pdm_cdev_register(struct pdm_cdev *client, struct pdm_device *pdm_dev,
                      const char *name, const char *nodename,
                      const struct file_operations *fops,
                      void (*release)(struct pdm_cdev *client))
{
    int minor;

    // 从 PDM 字符设备号池分配 minor
    minor = ida_alloc_max(&pdm_cdev_ida, PDM_CDEV_MINORS - 1, GFP_KERNEL);

    // 记录逻辑名称和 /dev/pdm/<nodename> 节点名
    strscpy(client->name, name, sizeof(client->name));
    strscpy(client->nodename, nodename, sizeof(client->nodename));

    // 初始化字符设备并挂到 PDM class 下
    client->dev.class = &pdm_cdev_class;
    client->dev.parent = &pdm_dev->dev;
    client->dev.devt = MKDEV(MAJOR(pdm_cdev_devt), minor);
    dev_set_name(&client->dev, "%s", client->nodename);

    cdev_init(&client->cdev, fops);
    return cdev_device_add(&client->cdev, &client->dev);
}
```

**用户空间访问路径：**

```
/dev/pdm/led0 → pdm_led_fops.ioctl → pdm_led_ioctl() → 后端 apply()
/dev/pdm/mcu0     → pdm_mcu_fops.ioctl → pdm_mcu_ioctl() → 传输层 xfer()
```

---

### 3.2 注册表系统 (`registry/` - 6 个文件)

#### 3.2.1 链接器段架构

PDM 使用 **ELF 链接器段**在链接时收集驱动/后端条目，消除手动注册数组的需要。

**核心思想：**

1. 每个驱动/后端在其源文件中声明一个条目
2. 使用 `__section("pdm_driver_entries")` 将条目放入特殊段
3. 链接器自动收集所有条目到连续内存区域
4. 运行时遍历段边界符号进行初始化

#### 3.2.2 pdm_driver_registry.c (68 行)

驱动注册表：

```c
// 链接器提供的段边界符号
extern struct pdm_driver_entry __start_pdm_driver_entries[];
extern struct pdm_driver_entry __stop_pdm_driver_entries[];

struct pdm_driver_entry {
    const char *name;
    int (*init)(void);
    void (*exit)(void);
};

void pdm_driver_entries_init(void)
{
    struct pdm_driver_entry *entry;
    
    // 遍历所有驱动条目
    for (entry = __start_pdm_driver_entries; 
         entry < __stop_pdm_driver_entries; 
         entry++) {
        pr_info("PDM: Initializing driver '%s'\n", entry->name);
        
        if (entry->init) {
            int ret = entry->init();
            if (ret < 0)
                pr_err("PDM: Driver '%s' init failed: %d\n", 
                       entry->name, ret);
        }
    }
}
```

**驱动注册宏** (`pdm_bus.h` 21-28 行)：

```c
#define pdm_driver_register(name, init_fn, exit_fn)                     \
    static struct pdm_driver_entry __pdm_driver_entry_##name            \
    __used __section("pdm_driver_entries") = {                          \
        .name = #name,                                                  \
        .init = init_fn,                                                \
        .exit = exit_fn,                                                \
    }
```

**在 pdm_led.c 中的使用** (410 行)：

```c
// 声明 LED 驱动初始化和退出函数
static int pdm_led_driver_init(void)
{
    return pdm_bus_register_driver(&pdm_led_driver);
}

static void pdm_led_driver_exit(void)
{
    pdm_bus_unregister_driver(&pdm_led_driver);
}

// 使用宏注册驱动
pdm_driver_register(led, pdm_led_driver_init, pdm_led_driver_exit);

/* 宏展开后：
static struct pdm_driver_entry __pdm_driver_entry_led 
__used __section("pdm_driver_entries") = {
    .name = "led",
    .init = pdm_led_driver_init,
    .exit = pdm_led_driver_exit,
}; */
```

**Makefile 链接顺序** 确保段标记正确包围条目：

```makefile
# 段开始标记
pdm-y += registry/pdm_driver_registry_start.o

# 驱动对象（包含段条目）
pdm-y += drivers/led/pdm_led.o
pdm-y += drivers/mcu/pdm_mcu.o

# 段结束标记
pdm-y += registry/pdm_driver_registry_stop.o
```

**pdm_driver_registry_start.c**：

```c
// 定义段开始符号
struct pdm_driver_entry __start_pdm_driver_entries[0] 
    __section("pdm_driver_entries") = {};
```

**pdm_driver_registry_stop.c**：

```c
// 定义段结束符号
struct pdm_driver_entry __stop_pdm_driver_entries[0] 
    __section("pdm_driver_entries") = {};
```

#### 3.2.3 pdm_backend_registry.c (118 行)

后端注册表（类似机制）：

```c
struct pdm_backend_entry {
    u32 device_type;                      // PDM_LED_DEVICE_TYPE, PDM_MCU_DEVICE_TYPE
    u32 backend_class;                    // PDM_BACKEND_CLASS_CONTROL / TRANSPORT
    const struct of_device_id *matches;   // Compatible 字符串表
    const void *ops;                      // 后端操作结构指针
    int (*init)(void);
    void (*exit)(void);
};

// 链接器提供的段边界
extern struct pdm_backend_entry __start_pdm_backend_entries[];
extern struct pdm_backend_entry __stop_pdm_backend_entries[];
```

**后端查找函数** (93-118 行)：

```c
const struct pdm_backend_entry *pdm_backend_find(
    u32 device_type, 
    u32 backend_class, 
    const char *compatible)
{
    struct pdm_backend_entry *entry;
    struct device fake_dev = {};
    struct device_node *np;
    
    // 创建临时设备节点用于匹配
    np = of_find_compatible_node(NULL, NULL, compatible);
    if (np)
        fake_dev.of_node = np;
    
    // 遍历所有后端条目
    for (entry = __start_pdm_backend_entries; 
         entry < __stop_pdm_backend_entries; 
         entry++) {
        // 检查设备类型
        if (entry->device_type != device_type)
            continue;
        
        // 检查后端类别
        if (entry->backend_class != backend_class)
            continue;
        
        // 检查 compatible 字符串匹配
        if (of_match_device(entry->matches, &fake_dev)) {
            of_node_put(np);
            return entry;
        }
    }
    
    of_node_put(np);
    return NULL;
}
```

**后端注册宏**：

```c
#define pdm_backend_register(name, dev_type, class, matches, ops, init, exit) \
    static struct pdm_backend_entry __pdm_backend_entry_##name             \
    __used __section("pdm_backend_entries") = {                            \
        .device_type = dev_type,                                           \
        .backend_class = class,                                            \
        .matches = matches,                                                \
        .ops = ops,                                                        \
        .init = init,                                                      \
        .exit = exit,                                                      \
    }
```

**在 pdm_led_gpio.c 中的使用** (156-161 行)：

```c
static const struct of_device_id pdm_led_gpio_of_match[] = {
    { .compatible = "pdm,led-gpio" },
    { /* sentinel */ }
};

static const struct pdm_led_backend_ops pdm_led_gpio_ops = {
    .setup = pdm_led_gpio_setup,
    .cleanup = pdm_led_gpio_cleanup,
    .apply = pdm_led_gpio_apply,
};

pdm_backend_register(
    led_gpio,
    PDM_LED_DEVICE_TYPE,
    PDM_BACKEND_CLASS_CONTROL,
    pdm_led_gpio_of_match,
    &pdm_led_gpio_ops,
    NULL,  // 无需特殊初始化
    NULL
);
```

---

### 3.3 诊断层 (`diag/` - 4 个文件)

#### 3.3.1 pdm_debugfs.c - DebugFS 接口

```
/sys/kernel/debug/pdm/
├── devices           # 列出所有 PDM 设备及其状态
├── drivers           # 列出所有已注册的驱动
└── backends          # 列出所有已注册的后端
```

**设备列表输出示例：**

```
# cat /sys/kernel/debug/pdm/devices
Name      Type  ID  State    Compatible         Driver
led0      LED   0   bound    pdm,led-gpio       pdm_led
led1      LED   1   bound    pdm,led-pwm        pdm_led
mcu0      MCU   0   bound    pdm,mcu-i2c        pdm_mcu
```


**后端列表输出示例：**

```
# cat /sys/kernel/debug/pdm/backends
Name           Type  Class      Compatible
led_gpio       LED   CONTROL    pdm,led-gpio
led_pwm        LED   CONTROL    pdm,led-pwm
mcu_i2c        MCU   TRANSPORT  pdm,mcu-i2c
mcu_spi        MCU   TRANSPORT  pdm,mcu-spi
mcu_uart       MCU   TRANSPORT  pdm,mcu-uart
mcu_can        MCU   TRANSPORT  pdm,mcu-can
```

#### 3.3.2 pdm_proc.c - Procfs 接口

```
/proc/pdm/
├── devices           # 设备清单
└── stats             # 框架统计信息
```

**统计信息示例：**

```
# cat /proc/pdm/stats
Total Devices:        3
Bound Devices:        3
Unbound Devices:      0
Registered Drivers:   2
Registered Backends:  6
```

#### 3.3.3 pdm_sysfs.c - Sysfs 属性

每个设备在 sysfs 中有自己的目录：

```
/sys/bus/pdm/devices/led0/
├── device_type       # "LED"
├── capabilities      # "0x00000101"
├── state             # "bound"
├── compatible        # "pdm,led-gpio"
├── driver -> ../../../bus/pdm/drivers/pdm_led
└── uevent            # 标准设备 uevent
```

**读取示例：**

```bash
# cat /sys/bus/pdm/devices/led0/device_type
LED

# cat /sys/bus/pdm/devices/led0/state
bound

# cat /sys/bus/pdm/devices/led0/compatible
pdm,led-gpio
```

---

## 四、传输层详解

### 4.1 传输抽象模型

PDM 实现**基于能力的传输层**，设备驱动（MCU）声明所需能力，后端通过标准化 ops 结构提供它们。

#### 4.1.1 核心结构

**传输操作结构** (`pdm_mcu_internal.h` 23-31 行)：

```c
struct pdm_mcu_transport_ops {
    const char *name;                                         // 传输名称
    int (*setup)(struct pdm_mcu_instance *inst);             // 初始化
    void (*cleanup)(struct pdm_mcu_instance *inst);          // 清理
    int (*xfer)(struct pdm_mcu_instance *inst,               // 数据传输
                struct pdm_mcu_xfer *xfer);
    u32 capabilities;                                        // 能力标志
    // PDM_MCU_CAP_TRANSPORT_I2C, PDM_MCU_CAP_TRANSPORT_SPI, 等
};
```

**传输描述符** (`pdm_mcu.h` 18-25 行)：

```c
struct pdm_mcu_xfer {
    u32 command;              // 命令 ID
    const void *tx_buf;       // 发送缓冲区
    void *rx_buf;             // 接收缓冲区
    size_t tx_len;            // 发送长度
    size_t rx_len;            // 接收缓冲区大小
    size_t actual_rx_len;     // 实际接收的字节数
};
```

**MCU 实例结构** (`pdm_mcu_internal.h` 40-68 行)：

```c
struct pdm_mcu_instance {
    struct pdm_cdev_instance base;               // 带字符设备的基础实例
    const struct pdm_mcu_transport_ops *transport_ops;  // 传输操作
    
    // 传输特定数据（联合体）
    union {
        struct {
            struct i2c_client *client;
            u32 command_bytes;
            u32 rx_timeout_ms;
        } i2c;
        
        struct {
            struct spi_device *spi;
            u32 command_bytes;
            u32 rx_timeout_ms;
        } spi;
        
        struct {
            struct serdev_device *serdev;
            struct kfifo rx_fifo;
            spinlock_t rx_lock;
            wait_queue_head_t rx_wait;
            u32 rx_timeout_ms;
        } uart;
        
        struct {
            struct socket *sock;
            u32 can_id;
            u32 timeout_ms;
        } can;
    } transport;
};
```

#### 4.1.2 传输层设计优势

1. **解耦设备逻辑和通信机制**
   - MCU 驱动只关心协议层（命令/响应）
   - 传输细节由后端处理

2. **运行时后端选择**
   - 通过 DTS compatible 字符串选择
   - 无需重新编译驱动

3. **统一的传输接口**
   - 所有传输后端实现相同的 `transport_ops`
   - 上层代码无需关心底层差异

4. **易于扩展**
   - 添加新传输类型只需实现新后端
   - 不影响现有驱动代码

---

### 4.2 I2C 传输后端 (`pdm_mcu_i2c.c` - 239 行)

#### 4.2.1 注册

```c
static const struct of_device_id pdm_mcu_i2c_of_match[] = {
    { .compatible = "pdm,mcu-i2c" },
    { /* sentinel */ }
};

static const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops = {
    .name = "i2c",
    .setup = pdm_mcu_i2c_setup,
    .cleanup = pdm_mcu_i2c_cleanup,
    .xfer = pdm_mcu_i2c_xfer,
    .capabilities = PDM_MCU_CAP_TRANSPORT_I2C,
};

pdm_backend_register(
    mcu_i2c,
    PDM_MCU_DEVICE_TYPE,
    PDM_BACKEND_CLASS_TRANSPORT,
    pdm_mcu_i2c_of_match,
    &pdm_mcu_i2c_ops,
    pdm_mcu_i2c_driver_register,
    pdm_mcu_i2c_driver_unregister
);
```

#### 4.2.2 Setup 操作 (83-106 行)

```c
static int pdm_mcu_i2c_setup(struct pdm_mcu_instance *inst)
{
    struct pdm_mcu_bus_device *bus_dev = inst->base.pdm_dev->config_data;
    struct i2c_client *client = bus_dev->bus.i2c;
    struct device *dev = &inst->base.pdm_dev->dev;
    u32 cmd_bytes, timeout_ms;
    
    // 存储 I2C 客户端指针
    inst->transport.i2c.client = client;
    
    // 解析 DTS 属性：命令字节数
    if (!of_property_read_u32(dev->of_node, "pdm,command-bytes", &cmd_bytes))
        inst->transport.i2c.command_bytes = cmd_bytes;
    else
        inst->transport.i2c.command_bytes = 1;  // 默认 1 字节
    
    // 解析 DTS 属性：接收超时
    if (!of_property_read_u32(dev->of_node, "rx-timeout-ms", &timeout_ms))
        inst->transport.i2c.rx_timeout_ms = timeout_ms;
    else
        inst->transport.i2c.rx_timeout_ms = 100;  // 默认 100ms
    
    dev_info(dev, "I2C transport: cmd_bytes=%u, timeout=%ums\n",
             inst->transport.i2c.command_bytes,
             inst->transport.i2c.rx_timeout_ms);
    
    return 0;
}
```

#### 4.2.3 Transfer 操作 (151-165 行)

```c
static int pdm_mcu_i2c_xfer(struct pdm_mcu_instance *inst, 
                           struct pdm_mcu_xfer *xfer)
{
    return pdm_mcu_i2c_cmd_xfer(inst, 
                               xfer->command, 
                               xfer->tx_buf, xfer->tx_len,
                               xfer->rx_buf, xfer->rx_len,
                               &xfer->actual_rx_len);
}
```

#### 4.2.4 命令传输实现 (108-149 行)

```c
static int pdm_mcu_i2c_cmd_xfer(struct pdm_mcu_instance *inst,
                               u32 command,
                               const void *tx_buf, size_t tx_len,
                               void *rx_buf, size_t rx_len,
                               size_t *actual_rx_len)
{
    u8 cmd_prefix[4];
    size_t prefix_len = inst->transport.i2c.command_bytes;
    int ret;
    
    // 编码命令为大端序
    switch (prefix_len) {
    case 1:
        cmd_prefix[0] = command & 0xFF;
        break;
    case 2:
        cmd_prefix[0] = (command >> 8) & 0xFF;
        cmd_prefix[1] = command & 0xFF;
        break;
    case 4:
        cmd_prefix[0] = (command >> 24) & 0xFF;
        cmd_prefix[1] = (command >> 16) & 0xFF;
        cmd_prefix[2] = (command >> 8) & 0xFF;
        cmd_prefix[3] = command & 0xFF;
        break;
    default:
        return -EINVAL;
    }
    
    // 执行 I2C 传输：命令前缀 + 有效载荷
    ret = pdm_mcu_i2c_bus_xfer(inst->transport.i2c.client,
                               cmd_prefix, prefix_len,
                               tx_buf, tx_len,
                               rx_buf, rx_len);
    
    if (ret < 0)
        return ret;
    
    if (actual_rx_len)
        *actual_rx_len = rx_len;
    
    return 0;
}
```

#### 4.2.5 底层 I2C 传输 (46-81 行)

```c
static int pdm_mcu_i2c_bus_xfer(struct i2c_client *client,
                               const void *cmd_buf, size_t cmd_len,
                               const void *tx_buf, size_t tx_len,
                               void *rx_buf, size_t rx_len)
{
    struct i2c_msg msgs[2];
    int num_msgs = 0;
    u8 *combined_tx = NULL;
    int ret;
    
    // 准备发送消息（命令 + 数据）
    if (cmd_len + tx_len > 0) {
        combined_tx = kmalloc(cmd_len + tx_len, GFP_KERNEL);
        if (!combined_tx)
            return -ENOMEM;
        
        // 复制命令和数据到组合缓冲区
        memcpy(combined_tx, cmd_buf, cmd_len);
        if (tx_len > 0)
            memcpy(combined_tx + cmd_len, tx_buf, tx_len);
        
        msgs[num_msgs].addr = client->addr;
        msgs[num_msgs].flags = 0;  // 写标志
        msgs[num_msgs].buf = combined_tx;
        msgs[num_msgs].len = cmd_len + tx_len;
        num_msgs++;
    }
    
    // 准备接收消息
    if (rx_len > 0) {
        msgs[num_msgs].addr = client->addr;
        msgs[num_msgs].flags = I2C_M_RD;  // 读标志
        msgs[num_msgs].buf = rx_buf;
        msgs[num_msgs].len = rx_len;
        num_msgs++;
    }
    
    // 执行 I2C 传输
    ret = i2c_transfer(client->adapter, msgs, num_msgs);
    
    kfree(combined_tx);
    
    if (ret < 0)
        return ret;
    if (ret != num_msgs)
        return -EIO;
    
    return 0;
}
```

**I2C 传输时序：**

```
开始 | 地址+W | 命令字节 | 数据字节 | 停止 | 开始 | 地址+R | 响应字节 | 停止
  S     0x50      0x01       [...]      P      S     0x50      [...]      P
```

---

### 4.3 SPI 传输后端 (`pdm_mcu_spi.c` - 251 行)

#### 4.3.1 注册

```c
static const struct of_device_id pdm_mcu_spi_of_match[] = {
    { .compatible = "pdm,mcu-spi" },
    { /* sentinel */ }
};

static const struct pdm_mcu_transport_ops pdm_mcu_spi_ops = {
    .name = "spi",
    .setup = pdm_mcu_spi_setup,
    .cleanup = pdm_mcu_spi_cleanup,
    .xfer = pdm_mcu_spi_xfer,
    .capabilities = PDM_MCU_CAP_TRANSPORT_SPI,
};

pdm_backend_register(
    mcu_spi,
    PDM_MCU_DEVICE_TYPE,
    PDM_BACKEND_CLASS_TRANSPORT,
    pdm_mcu_spi_of_match,
    &pdm_mcu_spi_ops,
    pdm_mcu_spi_driver_register,
    pdm_mcu_spi_driver_unregister
);
```

#### 4.3.2 Setup 操作 (74-96 行)

```c
static int pdm_mcu_spi_setup(struct pdm_mcu_instance *inst)
{
    struct pdm_mcu_bus_device *bus_dev = inst->base.pdm_dev->config_data;
    struct spi_device *spi = bus_dev->bus.spi;
    struct device *dev = &inst->base.pdm_dev->dev;
    u32 cmd_bytes, timeout_ms;
    
    inst->transport.spi.spi = spi;
    
    // 解析命令编码方式
    if (!of_property_read_u32(dev->of_node, "pdm,command-bytes", &cmd_bytes))
        inst->transport.spi.command_bytes = clamp(cmd_bytes, 1U, 4U);
    else
        inst->transport.spi.command_bytes = 1;
    
    // 解析超时
    if (!of_property_read_u32(dev->of_node, "rx-timeout-ms", &timeout_ms))
        inst->transport.spi.rx_timeout_ms = timeout_ms;
    else
        inst->transport.spi.rx_timeout_ms = 100;
    
    return 0;
}
```


#### 4.3.3 Transfer 操作 (123-140 行)

```c
static int pdm_mcu_spi_cmd_xfer(struct pdm_mcu_instance *inst,
                               u32 command,
                               const void *tx_buf, size_t tx_len,
                               void *rx_buf, size_t rx_len,
                               size_t *actual_rx_len)
{
    u8 cmd_prefix[4];
    size_t prefix_len = inst->transport.spi.command_bytes;
    int ret;
    
    // 编码命令为大端序
    pdm_mcu_spi_encode_prefix(command, cmd_prefix, prefix_len);
    
    // 执行 SPI 传输：命令前缀 + 有效载荷
    ret = pdm_mcu_spi_bus_xfer(inst->transport.spi.spi,
                               cmd_prefix, prefix_len,
                               tx_buf, tx_len,
                               rx_buf, rx_len);
    
    if (ret < 0)
        return ret;
    
    if (actual_rx_len)
        *actual_rx_len = rx_len;
    
    return 0;
}
```

#### 4.3.4 底层 SPI 传输 (38-72 行)

```c
static int pdm_mcu_spi_bus_xfer(struct spi_device *spi,
                               const void *cmd_buf, size_t cmd_len,
                               const void *tx_buf, size_t tx_len,
                               void *rx_buf, size_t rx_len)
{
    struct spi_transfer xfers[2];
    struct spi_message msg;
    u8 *combined_buf = NULL;
    int num_xfers = 0;
    int ret;
    
    memset(xfers, 0, sizeof(xfers));
    spi_message_init(&msg);
    
    // 传输 1: 命令 + 发送数据
    if (cmd_len + tx_len > 0) {
        combined_buf = kmalloc(cmd_len + tx_len, GFP_KERNEL);
        if (!combined_buf)
            return -ENOMEM;
        
        memcpy(combined_buf, cmd_buf, cmd_len);
        if (tx_len > 0)
            memcpy(combined_buf + cmd_len, tx_buf, tx_len);
        
        xfers[num_xfers].tx_buf = combined_buf;
        xfers[num_xfers].len = cmd_len + tx_len;
        spi_message_add_tail(&xfers[num_xfers], &msg);
        num_xfers++;
    }
    
    // 传输 2: 接收数据
    if (rx_len > 0) {
        xfers[num_xfers].rx_buf = rx_buf;
        xfers[num_xfers].len = rx_len;
        spi_message_add_tail(&xfers[num_xfers], &msg);
        num_xfers++;
    }
    
    // 同步执行 SPI 消息
    ret = spi_sync(spi, &msg);
    
    kfree(combined_buf);
    
    return ret;
}
```

**SPI 传输时序：**

```
CS↓ | CMD | TX_DATA | CS↑  CS↓ | RX_DATA | CS↑
     0x01   [...]             [...]
```

---

### 4.4 UART 传输后端 (`pdm_mcu_uart.c` + `pdm_mcu_uart_serdev.c` - 338 行)

#### 4.4.1 架构

UART 传输采用**双层架构**：

- **pdm_mcu_uart.c**：协议层，定义命令编码和传输逻辑
- **pdm_mcu_uart_serdev.c**：Serdev 实现，处理底层 UART 操作和 FIFO 缓冲

**弱符号链接机制：**

```c
// pdm_mcu_uart.c - 定义弱符号
int __weak pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst)
{
    return -ENOSYS;  // 如果没有实现，返回错误
}

// pdm_mcu_uart_serdev.c - 强符号覆盖
int pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst)
{
    // 实际的 serdev 初始化
}
```

#### 4.4.2 注册

```c
static const struct of_device_id pdm_mcu_uart_of_match[] = {
    { .compatible = "pdm,mcu-uart" },
    { /* sentinel */ }
};

static const struct pdm_mcu_transport_ops pdm_mcu_uart_ops = {
    .name = "uart",
    .setup = pdm_mcu_uart_setup,
    .cleanup = pdm_mcu_uart_cleanup,
    .xfer = pdm_mcu_uart_xfer,
    .capabilities = PDM_MCU_CAP_TRANSPORT_UART,
};

pdm_backend_register(
    mcu_uart,
    PDM_MCU_DEVICE_TYPE,
    PDM_BACKEND_CLASS_TRANSPORT,
    pdm_mcu_uart_of_match,
    &pdm_mcu_uart_ops,
    NULL,  // serdev 驱动单独注册
    NULL
);
```

#### 4.4.3 Serdev Setup (193-249 行在 uart_serdev.c)

```c
int pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst)
{
    struct pdm_mcu_bus_device *bus_dev = inst->base.pdm_dev->config_data;
    struct serdev_device *serdev = bus_dev->bus.serdev;
    struct device *dev = &inst->base.pdm_dev->dev;
    u32 speed, timeout_ms;
    int ret;
    
    // 打开 serdev 设备
    ret = serdev_device_open(serdev);
    if (ret < 0) {
        dev_err(dev, "Failed to open serdev: %d\n", ret);
        return ret;
    }
    
    // 配置波特率
    if (!of_property_read_u32(dev->of_node, "current-speed", &speed))
        serdev_device_set_baudrate(serdev, speed);
    else
        serdev_device_set_baudrate(serdev, 115200);
    
    // 配置流控
    if (of_property_read_bool(dev->of_node, "uart-has-rtscts"))
        serdev_device_set_flow_control(serdev, true);
    
    // 初始化接收 FIFO
    ret = kfifo_alloc(&inst->transport.uart.rx_fifo, 512, GFP_KERNEL);
    if (ret < 0) {
        serdev_device_close(serdev);
        return ret;
    }
    
    spin_lock_init(&inst->transport.uart.rx_lock);
    init_waitqueue_head(&inst->transport.uart.rx_wait);
    
    // 解析超时
    if (!of_property_read_u32(dev->of_node, "rx-timeout-ms", &timeout_ms))
        inst->transport.uart.rx_timeout_ms = timeout_ms;
    else
        inst->transport.uart.rx_timeout_ms = 1000;
    
    dev_info(dev, "UART transport initialized: %u baud\n", speed);
    
    return 0;
}
```

#### 4.4.4 Serdev 接收回调 (26-51 行在 uart_serdev.c)

```c
static int pdm_mcu_serdev_receive_buf(struct serdev_device *serdev,
                                      const u8 *buf, size_t count)
{
    struct pdm_mcu_bus_device *bus_dev = serdev_device_get_drvdata(serdev);
    struct pdm_mcu_instance *inst;
    unsigned long flags;
    
    if (!bus_dev || !bus_dev->pdm_dev)
        return 0;
    
    inst = pdm_device_get_drvdata(bus_dev->pdm_dev);
    if (!inst)
        return 0;
    
    // 将数据推入 FIFO（线程安全）
    spin_lock_irqsave(&inst->transport.uart.rx_lock, flags);
    kfifo_in(&inst->transport.uart.rx_fifo, buf, count);
    spin_unlock_irqrestore(&inst->transport.uart.rx_lock, flags);
    
    // 唤醒等待数据的线程
    wake_up_interruptible(&inst->transport.uart.rx_wait);
    
    return count;  // 告诉 serdev 我们消费了所有数据
}
```

#### 4.4.5 Transfer 操作 (100-114 行在 uart.c)

```c
static int pdm_mcu_uart_xfer(struct pdm_mcu_instance *inst, 
                            struct pdm_mcu_xfer *xfer)
{
    u8 cmd_prefix[4];
    int ret;
    
    // 编码命令为固定 4 字节大端序
    cmd_prefix[0] = (xfer->command >> 24) & 0xFF;
    cmd_prefix[1] = (xfer->command >> 16) & 0xFF;
    cmd_prefix[2] = (xfer->command >> 8) & 0xFF;
    cmd_prefix[3] = xfer->command & 0xFF;
    
    // 写命令
    ret = pdm_mcu_uart_write_exact(inst, cmd_prefix, 4);
    if (ret < 0)
        return ret;
    
    // 写数据
    if (xfer->tx_len > 0) {
        ret = pdm_mcu_uart_write_exact(inst, xfer->tx_buf, xfer->tx_len);
        if (ret < 0)
            return ret;
    }
    
    // 读响应
    if (xfer->rx_len > 0) {
        ret = pdm_mcu_uart_read_bus(inst, xfer->rx_buf, xfer->rx_len, 
                                    &xfer->actual_rx_len);
        if (ret < 0)
            return ret;
    }
    
    return 0;
}
```

#### 4.4.6 阻塞读取实现 (142-191 行在 uart_serdev.c)

```c
int pdm_mcu_uart_read_bus(struct pdm_mcu_instance *inst,
                          void *buf, size_t len, size_t *actual_len)
{
    unsigned long timeout = msecs_to_jiffies(inst->transport.uart.rx_timeout_ms);
    size_t total_read = 0;
    unsigned long flags;
    int ret;
    
    while (total_read < len) {
        // 等待数据可用（带超时）
        ret = wait_event_interruptible_timeout(
            inst->transport.uart.rx_wait,
            kfifo_len(&inst->transport.uart.rx_fifo) > 0,
            timeout
        );
        
        if (ret == 0) {
            // 超时
            dev_warn(&inst->base.pdm_dev->dev, 
                    "UART read timeout after %zu/%zu bytes\n",
                    total_read, len);
            if (actual_len)
                *actual_len = total_read;
            return total_read > 0 ? 0 : -ETIMEDOUT;
        }
        
        if (ret < 0) {
            // 被信号中断
            return ret;
        }
        
        // 从 FIFO 读取数据
        spin_lock_irqsave(&inst->transport.uart.rx_lock, flags);
        ret = kfifo_out(&inst->transport.uart.rx_fifo, 
                       (u8 *)buf + total_read, 
                       len - total_read);
        spin_unlock_irqrestore(&inst->transport.uart.rx_lock, flags);
        
        total_read += ret;
    }
    
    if (actual_len)
        *actual_len = total_read;
    
    return 0;
}
```

**UART 传输时序：**

```
TX: | CMD (4B) | TX_DATA | 
RX:                        | RX_DATA |

线程行为：
1. 写命令和数据（非阻塞）
2. 阻塞等待响应（wait_event_interruptible_timeout）
3. IRQ 触发 → serdev 回调 → 数据入 FIFO → 唤醒等待线程
4. 线程从 FIFO 读取数据
```

---

### 4.5 CAN 传输后端 (`pdm_mcu_can.c` - 215 行)

#### 4.5.1 注册

```c
static const struct of_device_id pdm_mcu_can_of_match[] = {
    { .compatible = "pdm,mcu-can" },
    { /* sentinel */ }
};

static const struct pdm_mcu_transport_ops pdm_mcu_can_ops = {
    .name = "can",
    .setup = pdm_mcu_can_setup,
    .cleanup = pdm_mcu_can_cleanup,
    .xfer = pdm_mcu_can_xfer,
    .capabilities = PDM_MCU_CAP_TRANSPORT_CAN,
};

pdm_backend_register(
    mcu_can,
    PDM_MCU_DEVICE_TYPE,
    PDM_BACKEND_CLASS_TRANSPORT,
    pdm_mcu_can_of_match,
    &pdm_mcu_can_ops,
    NULL,
    NULL
);
```

#### 4.5.2 Setup 操作 (74-96 行)

```c
static int pdm_mcu_can_setup(struct pdm_mcu_instance *inst)
{
    struct device *dev = &inst->base.pdm_dev->dev;
    const char *can_ifname;
    struct net_device *can_netdev;
    struct socket *sock;
    struct sockaddr_can addr;
    u32 can_id, timeout_ms;
    int ret;
    
    // 读取 CAN 接口名称
    ret = of_property_read_string(dev->of_node, "can-ifname", &can_ifname);
    if (ret < 0) {
        dev_err(dev, "Missing can-ifname property\n");
        return ret;
    }
    
    // 创建 RAW CAN socket
    ret = sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW, &sock);
    if (ret < 0) {
        dev_err(dev, "Failed to create CAN socket: %d\n", ret);
        return ret;
    }
    
    // 查找 CAN 网络接口
    can_netdev = dev_get_by_name(&init_net, can_ifname);
    if (!can_netdev) {
        sock_release(sock);
        return -ENODEV;
    }
    
    // 绑定到接口
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = can_netdev->ifindex;
    
    ret = kernel_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    dev_put(can_netdev);
    
    if (ret < 0) {
        sock_release(sock);
        return ret;
    }
    
    // 保存 socket
    inst->transport.can.sock = sock;
    
    // 解析 CAN ID 和超时
    if (!of_property_read_u32(dev->of_node, "can-id", &can_id))
        inst->transport.can.can_id = can_id;
    else
        inst->transport.can.can_id = 0x123;  // 默认 ID
    
    if (!of_property_read_u32(dev->of_node, "rx-timeout-ms", &timeout_ms))
        inst->transport.can.timeout_ms = timeout_ms;
    else
        inst->transport.can.timeout_ms = 500;
    
    dev_info(dev, "CAN transport: interface=%s, id=0x%x\n",
             can_ifname, inst->transport.can.can_id);
    
    return 0;
}
```


#### 4.5.3 Transfer 操作 (125-160 行)

```c
static int pdm_mcu_can_xfer(struct pdm_mcu_instance *inst, 
                           struct pdm_mcu_xfer *xfer)
{
    struct can_frame tx_frame, rx_frame;
    int ret;
    
    // 构造发送帧
    memset(&tx_frame, 0, sizeof(tx_frame));
    tx_frame.can_id = inst->transport.can.can_id;
    
    // 命令 ID（4 字节）
    tx_frame.data[0] = (xfer->command >> 24) & 0xFF;
    tx_frame.data[1] = (xfer->command >> 16) & 0xFF;
    tx_frame.data[2] = (xfer->command >> 8) & 0xFF;
    tx_frame.data[3] = xfer->command & 0xFF;
    
    // 数据（最多 4 字节）
    size_t payload_len = min(xfer->tx_len, 4UL);
    if (payload_len > 0)
        memcpy(&tx_frame.data[4], xfer->tx_buf, payload_len);
    
    tx_frame.can_dlc = 4 + payload_len;
    
    // 发送帧
    ret = pdm_mcu_can_send_frame(inst, &tx_frame);
    if (ret < 0)
        return ret;
    
    // 接收响应帧
    if (xfer->rx_len > 0) {
        ret = pdm_mcu_can_recv_frame(inst, &rx_frame, 
                                     inst->transport.can.timeout_ms);
        if (ret < 0)
            return ret;
        
        // 提取响应数据
        size_t rx_len = min(xfer->rx_len, (size_t)rx_frame.can_dlc);
        memcpy(xfer->rx_buf, rx_frame.data, rx_len);
        
        if (xfer->actual_rx_len)
            *xfer->actual_rx_len = rx_len;
    }
    
    return 0;
}
```

**CAN 帧格式：**

```
TX Frame:
┌────────┬──────────────────┬──────────────────┐
│ CAN ID │ CMD (4B)         │ DATA (0-4B)      │
│ 0x123  │ 0x00 0x00 0x01 0x00 │ payload...    │
└────────┴──────────────────┴──────────────────┘

RX Frame:
┌────────┬──────────────────────────────────┐
│ CAN ID │ RESPONSE DATA (0-8B)             │
│ 0x123  │ response payload...              │
└────────┴──────────────────────────────────┘
```

---

## 五、完整初始化流程

### 5.1 启动序列

**代码引用**：`kernel/pdm/pdm.c:pdm_module_init()`

```
模块加载时序：

步骤 1: pdm_device_ids_init()
  功能：为每种设备类型（MCU、LED）初始化 IDA 分配器
  位置：pdm_device.c:21-50
  
  代码：
  ida_init(&led_allocator.ida);
  ida_init(&mcu_allocator.ida);

步骤 2: pdm_bus_init()
  功能：初始化 PDM 总线（内部调用 bus_register 和 pdm_of_bus_init）
  位置：pdm.c:43，实际实现在 pdm_bus.c
  
  效果：
  - 调用 bus_register(&pdm_bus_type) 注册总线
  - 创建 /sys/bus/pdm/
  - 创建 /sys/bus/pdm/devices/
  - 创建 /sys/bus/pdm/drivers/
  - 调用 pdm_of_bus_init() 注册 DT 枚举驱动
  - 启用设备-驱动匹配机制
  
  匹配：compatible = "vendor,pdm-bus"

步骤 3: pdm_cdev_init()
  功能：初始化字符设备子系统
  位置：pdm.c:49
  
  效果：
  - 创建 PDM 字符设备类
  - 准备 /dev/pdm/ 目录结构

步骤 4: pdm_backend_entries_init()
  功能：初始化所有后端
  位置：pdm_backend_registry.c:76-91
  
  遍历：__start_pdm_backend_entries → __stop_pdm_backend_entries
  
  对每个后端调用：
  - pdm_mcu_i2c_driver_register()
  - pdm_mcu_spi_driver_register()
  - pdm_mcu_uart_serdev_driver_register()
  - pdm_mcu_can_driver_register()
  - 等

步骤 5: pdm_driver_entries_init()
  功能：初始化所有驱动
  位置：pdm.c:61，实际实现在 pdm_driver_registry.c:33-48
  
  遍历：__start_pdm_driver_entries → __stop_pdm_driver_entries
  
  对每个驱动调用：
  - pdm_led_driver_init()
    → pdm_bus_register_driver(&pdm_led_driver)
    → driver_register()
  
  - pdm_mcu_driver_init()
    → pdm_bus_register_driver(&pdm_mcu_driver)
    → driver_register()

步骤 6: pdm_debugfs_init() + pdm_proc_init()
  功能：创建诊断接口
  位置：pdm_debugfs.c:148, pdm_proc.c:89
  
  效果：
  - /sys/kernel/debug/pdm/
  - /proc/pdm/
```

**初始化依赖图：**

```
pdm_module_init()
    │
    ├─→ pdm_device_ids_init()       [IDA 分配器]
    │       位置：pdm.c:37
    │
    ├─→ pdm_bus_init()               [PDM 总线注册]
    │       │ 位置：pdm.c:43
    │       │
    │       ├─→ bus_register(&pdm_bus_type)
    │       │       └─→ /sys/bus/pdm/ 创建
    │       │
    │       └─→ pdm_of_bus_init()    [DT 枚举驱动]
    │               └─→ 等待 DTS 匹配
    │
    ├─→ pdm_cdev_init()             [字符设备子系统]
    │       位置：pdm.c:49
    │
    ├─→ pdm_backend_entries_init()  [后端初始化]
    │       │ 位置：pdm.c:55
    │       │
    │       ├─→ I2C 驱动注册
    │       ├─→ SPI 驱动注册
    │       ├─→ UART 驱动注册
    │       └─→ CAN 驱动注册
    │
    ├─→ pdm_driver_entries_init()   [PDM 驱动初始化]
    │       │ 位置：pdm.c:61
    │       │
    │       ├─→ LED 驱动注册
    │       └─→ MCU 驱动注册
    │
    └─→ pdm_mock_devices_init()     [Mock 设备（可选）]
            位置：pdm.c:67
```

---

### 5.2 DTS 解析流程

#### 5.2.1 示例 DTS 节点

```dts
&i2c1 {
    status = "okay";
    
    pdm_bus: pdm@50 {
        compatible = "vendor,pdm-bus";
        reg = <0x50>;
        #address-cells = <1>;
        #size-cells = <0>;
        
        mcu0: mcu@0 {
            compatible = "pdm,mcu-i2c";
            reg = <0>;
            pdm,command-bytes = <1>;
            rx-timeout-ms = <200>;
        };
    };
};

&gpio1 {
    status = "okay";
};

/ {
    pdm_devices: pdm {
        compatible = "vendor,pdm-bus";
        
        led0: led@0 {
            compatible = "pdm,led-gpio";
            reg = <0>;
            gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
        };
        
        led1: led@1 {
            compatible = "pdm,led-pwm";
            reg = <1>;
            pwms = <&pwm1 0 1000000>;  /* 1MHz */
            pdm,max-brightness = <255>;
        };
    };
};
```

#### 5.2.2 解析流程详解

```
1. 平台设备匹配
   内核启动 → 解析 DTS → 创建 platform_device
   
   platform_device {
       .name = "pdm",
       .of_node = &pdm_bus,  // DTS 节点引用
   }
   
   → 匹配 pdm_of_bus_driver (compatible = "vendor,pdm-bus")
   → 调用 pdm_of_bus_probe()
   
   位置：pdm_of_bus.c:107-129

2. 子节点枚举
   for_each_available_child_of_node(pdev->dev.of_node, child)
   
   遍历：
   - child[0] = mcu@0 节点
   - child[1] = led@0 节点
   - child[2] = led@1 节点
   
   位置：pdm_of_bus.c:111

3. 对于每个子节点：

   a. 读取 compatible 字符串
      ret = of_property_read_string(child, "compatible", &compatible);
      
      mcu@0 → "pdm,mcu-i2c"
      led@0 → "pdm,led-gpio"
      led@1 → "pdm,led-pwm"
   
   b. 读取设备 ID
      device_id = pdm_of_bus_child_id(child);
      
      优先级（通用设备）：
      1. pdm,id 属性
      2. reg 属性
      3. -1（自动分配）
      
      注意：MCU 设备有额外的 pdm,index 属性（优先级在 pdm,id 和 reg 之间）
      
      mcu@0 → reg = <0> → device_id = 0
      led@0 → reg = <0> → device_id = 0
      led@1 → reg = <1> → device_id = 1
      
      位置：pdm_of_bus.c:44-56
   
   c. 分配 pdm_device
      pdm_dev = pdm_device_alloc(0);
      
      初始化：
      - pdm_dev->dev.bus = &pdm_bus_type
      - pdm_dev->dev.parent = &pdev->dev
      - pdm_dev->dev.of_node = of_node_get(child)
      - pdm_dev->compatible = compatible
      - pdm_dev->requested_id = device_id
      - pdm_dev->state = REGISTERED
      
      位置：pdm_device.c:136-154
   
   d. 注册设备
      pdm_device_register(pdm_dev, child->name);
      
      效果：
      - dev_set_name(&pdm_dev->dev, child->name)
        mcu@0 → "mcu0"
        led@0 → "led0"
        led@1 → "led1"
      
      - device_add(&pdm_dev->dev)
        → 触发总线匹配
        → 如果匹配成功，调用 probe
      
      位置：pdm_device.c:157-184

4. 总线匹配流程
   device_add() → bus_probe_device() → device_attach()
   → bus_for_each_drv() → __device_attach_driver()
   → driver_match_device() → pdm_bus_device_match()
   
   pdm_bus_device_match():
   
   对于 mcu0 (compatible = "pdm,mcu-i2c"):
   - 遍历所有已注册的 pdm_driver
   - 检查 pdm_mcu_driver.of_match_table
   - 找到匹配 → 调用 pdm_bus_device_probe()
   
   对于 led0 (compatible = "pdm,led-gpio"):
   - 检查 pdm_led_driver.match() 回调
   - pdm_led_match() 检查 GPIO 后端是否可用
   - 如果可用 → 调用 pdm_bus_device_probe()
   
   位置：pdm_bus.c:101-135

5. 设备绑定（probe）
   pdm_bus_device_probe(&pdm_dev->dev)
   
   步骤：
   a. 分配设备 ID
      pdm_device_bind(pdm_dev, device_type, capabilities)
      
      对于 LED：
      - device_type = PDM_LED_DEVICE_TYPE
      - IDA 分配：led0 → id=0, led1 → id=1
      
      对于 MCU：
      - device_type = PDM_MCU_DEVICE_TYPE
      - IDA 分配：mcu0 → id=0
      
      位置：pdm_device.c:211-236
   
   b. 调用驱动 probe
      pdm_drv->probe(pdm_dev)
      
      对于 LED：pdm_led_probe()
      对于 MCU：pdm_mcu_probe()
   
   c. 更新状态
      pdm_dev->state = PDM_MANAGER_DEVICE_STATE_BOUND
   
   位置：pdm_bus.c:26-62
```

---

### 5.3 驱动注册流程

#### 5.3.1 LED 驱动注册示例

```
步骤流程：

1. pdm_led_driver_init() 被调用
   触发：pdm_driver_entries_init() 遍历链接器段
   位置：pdm_led.c:392-407

2. 定义 pdm_driver 结构
   static struct pdm_driver pdm_led_driver = {
       .driver = {
           .name = "pdm_led",
           .owner = THIS_MODULE,
       },
       .probe = pdm_led_probe,
       .remove = pdm_led_remove,
       .match = pdm_led_match,
       .of_match_table = pdm_led_of_match,
   };
   
   static const struct of_device_id pdm_led_of_match[] = {
       { .compatible = "pdm,led-gpio" },
       { .compatible = "pdm,led-pwm" },
       { /* sentinel */ }
   };
   
   位置：pdm_led.c:370-390

3. 注册驱动
   pdm_bus_register_driver(&pdm_led_driver)
   
   内部实现：
   - drv->owner = THIS_MODULE
   - drv->bus = &pdm_bus_type
   - drv->of_match_table = pdm_led_of_match
   - driver_register(drv)
   
   位置：pdm_bus.c:214-238

4. driver_register() 效果
   - 将驱动添加到 pdm_bus_type.drivers 链表
   - 创建 /sys/bus/pdm/drivers/pdm_led/
   - 触发对所有未绑定设备的匹配尝试

5. 匹配算法触发
   对于每个 state=REGISTERED 的 pdm_device：
   
   调用 pdm_bus_device_match():
   
   a. 自定义 match() 回调
      pdm_led_match(pdm_dev)
      
      功能：检查后端可用性
      
      if (compatible == "pdm,led-gpio")
          backend = pdm_backend_find(PDM_LED_DEVICE_TYPE,
                                    PDM_BACKEND_CLASS_CONTROL,
                                    "pdm,led-gpio")
      
      if (backend)
          return 1;  // 匹配成功
      else
          return 0;  // 后端不可用，匹配失败
      
      位置：pdm_led.c:268-293
   
   b. of_match_table 匹配
      如果 match() 返回 1，检查 of_match_table
      
      of_match_device(pdm_led_of_match, &pdm_dev->dev)
      
      比较 pdm_dev->compatible 与表中的字符串
   
   位置：pdm_bus.c:101-135

6. 匹配成功后调用 probe
   pdm_bus_device_probe() → pdm_led_probe(pdm_dev)
   
   见下一节
```

#### 5.3.2 LED 驱动 Probe 流程

```c
static int pdm_led_probe(struct pdm_device *pdm_dev)
{
    const struct pdm_backend_entry *entry;
    struct pdm_led_instance *inst;
    char nodename[32];
    int ret;

    // 步骤 1: 分配实例并初始化 PDM 字符设备基类
    inst = kzalloc(sizeof(*inst), GFP_KERNEL);
    if (!inst)
        return -ENOMEM;

    pdm_cdev_instance_init(&inst->base, pdm_dev);

    // 步骤 2: 按 compatible 选择 LED 控制后端
    entry = pdm_backend_find(PDM_LED_DEVICE_TYPE,
                             PDM_BACKEND_CLASS_CONTROL,
                             pdm_dev->compatible);
    inst->ops = entry ? entry->ops : &pdm_led_memory_ops;

    // 步骤 3: 读取设备树配置并初始化后端
    ret = pdm_led_read_dt_config(inst);
    if (ret)
        goto err_free;

    ret = inst->ops->setup(inst);
    if (ret)
        goto err_free;

    // 步骤 4: 创建统一命名空间下的 /dev/pdm/ledN 节点
    snprintf(nodename, sizeof(nodename), "led%d", pdm_dev->id);
    ret = pdm_cdev_register(&inst->base.cdev, pdm_dev, "pdm-led",
                            nodename, &pdm_led_fops, pdm_led_client_release);
    if (ret)
        goto err_cleanup_backend;

    pdm_device_set_drvdata(pdm_dev, inst);
    LOG_INFO("Registered LED %s backend for %s",
             inst->ops->name, dev_name(&pdm_dev->dev));
    return 0;

err_cleanup_backend:
    inst->ops->cleanup(inst);
err_free:
    kfree(inst);
    return ret;
}
```

**位置**：pdm_led.c:295-372


---

### 5.4 PDM Driver 和 Transport Driver 的关联机制

#### 5.4.1 MCU 设备的关联流程

```
1. MCU 设备 probe 阶段
   pdm_mcu_probe(pdm_dev)
   位置：pdm_mcu.c:239-338

2. 获取总线设备信息
   struct pdm_mcu_bus_device *bus_dev;
   bus_dev = pdm_device_get_bus_device(pdm_dev);
   
   bus_dev 包含：
   - bus_type: PDM_MCU_BUS_TYPE_I2C / SPI / UART / CAN
   - bus 联合体：
     union {
         struct i2c_client *i2c;
         struct spi_device *spi;
         struct serdev_device *serdev;
     } bus;

3. 根据总线类型查找传输后端
   const struct pdm_backend_entry *backend_entry;
   const char *backend_compatible;
   
   switch (bus_dev->bus_type) {
   case PDM_MCU_BUS_TYPE_I2C:
       backend_compatible = "pdm,mcu-i2c";
       break;
   case PDM_MCU_BUS_TYPE_SPI:
       backend_compatible = "pdm,mcu-spi";
       break;
   case PDM_MCU_BUS_TYPE_UART:
       backend_compatible = "pdm,mcu-uart";
       break;
   case PDM_MCU_BUS_TYPE_CAN:
       backend_compatible = "pdm,mcu-can";
       break;
   }
   
   backend_entry = pdm_backend_find(PDM_MCU_DEVICE_TYPE,
                                   PDM_BACKEND_CLASS_TRANSPORT,
                                   backend_compatible);
   
   位置：pdm_backend_registry.c:93-118

4. 绑定传输操作
   inst->transport_ops = backend_entry->ops;
   
   inst->transport_ops 指向：
   - pdm_mcu_i2c_ops（I2C）
   - pdm_mcu_spi_ops（SPI）
   - pdm_mcu_uart_ops（UART）
   - pdm_mcu_can_ops（CAN）

5. 调用传输 setup
   if (inst->transport_ops->setup) {
       ret = inst->transport_ops->setup(inst);
       if (ret < 0)
           return ret;
   }
   
   setup 功能：
   - 解析 DTS 传输特定属性
   - 初始化传输层状态
   - 配置硬件参数（波特率、超时等）

6. 之后的数据传输
   用户空间 ioctl → pdm_mcu_ioctl()
   → pdm_mcu_protocol_dispatch()
   → inst->transport_ops->xfer(inst, &xfer)
   → 具体传输后端实现
```

#### 5.4.2 调用链示例（I2C 传输）

```
完整调用链：

用户空间
    ↓
  ioctl(fd, PDM_MCU_IOCTL_XFER, &req)
    ↓
系统调用进入内核
    ↓
  pdm_mcu_ioctl()                      [pdm_mcu.c:150-178]
    ↓
  pdm_mcu_protocol_dispatch()          [pdm_mcu_protocol.c:48-76]
    ↓
  inst->transport_ops->xfer()          [函数指针调用]
    ↓
  pdm_mcu_i2c_xfer()                   [pdm_mcu_i2c.c:151-165]
    ↓
  pdm_mcu_i2c_cmd_xfer()               [pdm_mcu_i2c.c:108-149]
    ↓
  pdm_mcu_i2c_bus_xfer()               [pdm_mcu_i2c.c:46-81]
    ↓
  i2c_transfer()                       [Linux I2C 核心]
    ↓
  i2c_adapter->algo->master_xfer()     [硬件适配器驱动]
    ↓
硬件 I2C 控制器
```

**数据流示例：**

```c
// 用户空间代码
struct pdm_mcu_xfer_req req = {
    .command = 0x01,
    .tx_buf = data,
    .tx_len = 4,
    .rx_len = 8,
};
ioctl(fd, PDM_MCU_IOCTL_XFER, &req);

// 内核中：
// 1. pdm_mcu_protocol_dispatch() 填充 xfer
struct pdm_mcu_xfer xfer = {
    .command = req.command,
    .tx_buf = kernel_buf,
    .tx_len = req.tx_len,
    .rx_buf = kernel_buf,
    .rx_len = req.rx_len,
};

// 2. 调用传输层
inst->transport_ops->xfer(inst, &xfer);

// 3. I2C 后端编码命令
u8 cmd_prefix = 0x01;  // 1 字节命令

// 4. 构造 I2C 消息
struct i2c_msg msgs[2] = {
    {  // 写：命令 + 数据
        .addr = 0x50,
        .flags = 0,
        .buf = [0x01, data[0], data[1], data[2], data[3]],
        .len = 5,
    },
    {  // 读：响应
        .addr = 0x50,
        .flags = I2C_M_RD,
        .buf = rx_buf,
        .len = 8,
    },
};

// 5. 执行 I2C 传输
i2c_transfer(adapter, msgs, 2);

// 6. 响应返回用户空间
copy_to_user(req.rx_buf, rx_buf, xfer.actual_rx_len);
```

---

## 六、设备树绑定

### 6.1 LED 设备

#### 6.1.1 GPIO 后端

```dts
led0: led@0 {
    compatible = "pdm,led-gpio";
    reg = <0>;                              // 设备 ID
    gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;   // GPIO 引脚
    default-state = "off";                  // 可选：默认状态
};
```

**DTS 属性解析**：

- `compatible`: `"pdm,led-gpio"` - 触发 GPIO 后端匹配
- `reg`: 设备 ID（0 → `/dev/pdm/led0`）
- `gpios`: GPIO 引脚描述符
  - `&gpio1`: GPIO 控制器引用
  - `10`: 引脚编号
  - `GPIO_ACTIVE_HIGH`: 高电平有效

**代码引用**：`pdm_led_gpio.c:76-91`

```c
static int pdm_led_gpio_setup(struct pdm_led_instance *inst)
{
    struct gpio_desc *gpio;
    
    // 从 DTS 获取 GPIO
    gpio = devm_gpiod_get(&inst->base.pdm_dev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(gpio))
        return PTR_ERR(gpio);
    
    inst->backend.gpio.gpio = gpio;
    
    return 0;
}
```

#### 6.1.2 PWM 后端

```dts
led1: led@1 {
    compatible = "pdm,led-pwm";
    reg = <1>;
    pwms = <&pwm1 0 1000000>;    // PWM 控制器, 通道, 周期(ns)
    pdm,max-brightness = <255>;  // 可选：最大亮度
};
```

**DTS 属性解析**：

- `compatible`: `"pdm,led-pwm"` - 触发 PWM 后端匹配
- `pwms`: PWM 描述符
  - `&pwm1`: PWM 控制器引用
  - `0`: PWM 通道
  - `1000000`: 周期（1ms = 1MHz）
- `pdm,max-brightness`: 最大亮度值（默认 255）

**代码引用**：`pdm_led_pwm.c:89-113`

```c
static int pdm_led_pwm_setup(struct pdm_led_instance *inst)
{
    struct pwm_device *pwm;
    u32 max_brightness;
    
    // 从 DTS 获取 PWM
    pwm = devm_pwm_get(&inst->base.pdm_dev->dev, NULL);
    if (IS_ERR(pwm))
        return PTR_ERR(pwm);
    
    inst->backend.pwm.pwm = pwm;
    inst->backend.pwm.period_ns = pwm_get_period(pwm);
    
    // 读取最大亮度
    if (!of_property_read_u32(dev->of_node, "pdm,max-brightness", &max_brightness))
        inst->backend.pwm.max_brightness = max_brightness;
    else
        inst->backend.pwm.max_brightness = 255;
    
    return 0;
}
```

---

### 6.2 MCU 设备

#### 6.2.1 I2C 传输

```dts
&i2c1 {
    status = "okay";
    
    mcu_device: mcu@50 {
        compatible = "pdm,mcu-i2c";
        reg = <0x50>;                    // I2C 从设备地址
        pdm,command-bytes = <1>;         // 命令编码字节数 (1/2/4)
        rx-timeout-ms = <200>;           // 接收超时（毫秒）
    };
};

pdm {
    compatible = "vendor,pdm-bus";
    
    mcu0: mcu@0 {
        compatible = "pdm,mcu-i2c";
        reg = <0>;                       // PDM 设备 ID
        pdm,i2c-device = <&mcu_device>;  // 引用 I2C 从设备
    };
};
```

**DTS 属性解析**：

- `compatible`: `"pdm,mcu-i2c"` - 触发 I2C 传输后端
- `reg`: I2C 从设备地址（0x50）
- `pdm,command-bytes`: 命令编码方式
  - `1`: 单字节命令（0x00-0xFF）
  - `2`: 双字节命令（大端序）
  - `4`: 四字节命令（大端序）
- `rx-timeout-ms`: 接收超时时间

#### 6.2.2 SPI 传输

```dts
&spi1 {
    status = "okay";
    
    mcu_device: mcu@0 {
        compatible = "pdm,mcu-spi";
        reg = <0>;                       // SPI CS 引脚编号
        spi-max-frequency = <1000000>;   // 1MHz
        pdm,command-bytes = <2>;
        rx-timeout-ms = <100>;
    };
};

pdm {
    compatible = "vendor,pdm-bus";
    
    mcu0: mcu@0 {
        compatible = "pdm,mcu-spi";
        reg = <0>;
        pdm,spi-device = <&mcu_device>;
    };
};
```

#### 6.2.3 UART 传输

```dts
&uart2 {
    status = "okay";
    
    mcu_device {
        compatible = "pdm,mcu-uart";
        current-speed = <115200>;        // 波特率
        uart-has-rtscts;                 // 启用硬件流控
        rx-timeout-ms = <1000>;
    };
};

pdm {
    compatible = "vendor,pdm-bus";
    
    mcu0: mcu@0 {
        compatible = "pdm,mcu-uart";
        reg = <0>;
        pdm,uart-device = <&mcu_device>;
    };
};
```

**DTS 属性解析**：

- `current-speed`: 波特率（默认 115200）
- `uart-has-rtscts`: 启用 RTS/CTS 硬件流控
- `rx-timeout-ms`: 接收超时

#### 6.2.4 CAN 传输

```dts
&can0 {
    status = "okay";
};

pdm {
    compatible = "vendor,pdm-bus";
    
    mcu0: mcu@0 {
        compatible = "pdm,mcu-can";
        reg = <0>;
        can-ifname = "can0";             // CAN 网络接口名称
        can-id = <0x123>;                // CAN 标识符
        rx-timeout-ms = <500>;
    };
};
```

**DTS 属性解析**：

- `can-ifname`: CAN 网络接口名称（必需）
- `can-id`: CAN 帧标识符（默认 0x123）
- `rx-timeout-ms`: 接收超时

---

## 七、关键设计模式

### 7.1 分层架构（Layered Architecture）

PDM 采用清晰的分层设计：

```
┌─────────────────────────────────────┐
│    应用层 (Userspace)               │  ← ioctl 接口
├─────────────────────────────────────┤
│    驱动层 (Driver)                  │  ← 设备逻辑
├─────────────────────────────────────┤
│    总线层 (Bus)                     │  ← 设备-驱动匹配
├─────────────────────────────────────┤
│    传输层 (Transport)               │  ← 通信抽象
├─────────────────────────────────────┤
│    硬件抽象层 (HAL)                 │  ← Linux 子系统
└─────────────────────────────────────┘
```

**优势：**
- 关注点分离
- 易于测试和维护
- 清晰的依赖关系

### 7.2 策略模式（Strategy Pattern）

传输后端使用策略模式实现可插拔：

```c
// 策略接口
struct pdm_mcu_transport_ops {
    int (*setup)(struct pdm_mcu_instance *inst);
    void (*cleanup)(struct pdm_mcu_instance *inst);
    int (*xfer)(struct pdm_mcu_instance *inst, struct pdm_mcu_xfer *xfer);
};

// 具体策略
const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops = { ... };
const struct pdm_mcu_transport_ops pdm_mcu_spi_ops = { ... };

// 上下文（运行时选择策略）
inst->transport_ops = selected_backend->ops;
inst->transport_ops->xfer(inst, &xfer);
```

**优势：**
- 运行时后端选择
- 无需修改驱动代码
- 易于添加新传输类型

### 7.3 注册表模式（Registry Pattern）

使用链接器段实现自动注册：

```c
// 注册宏
#define pdm_backend_register(name, ...) \
    static struct pdm_backend_entry __pdm_backend_entry_##name \
    __used __section("pdm_backend_entries") = { ... }

// 查找
const struct pdm_backend_entry *pdm_backend_find(...) {
    for (entry = __start_pdm_backend_entries; 
         entry < __stop_pdm_backend_entries; 
         entry++) { ... }
}
```

**优势：**
- 零配置注册
- 编译时收集
- 无需维护全局数组

### 7.4 工厂模式（Factory Pattern）

设备创建使用工厂模式：

```c
// 工厂函数
struct pdm_device *pdm_device_alloc(size_t config_size)
{
    struct pdm_device *pdm_dev;
    
    pdm_dev = kzalloc(sizeof(*pdm_dev) + config_size, GFP_KERNEL);
    device_initialize(&pdm_dev->dev);
    pdm_dev->dev.bus = &pdm_bus_type;
    pdm_dev->dev.release = pdm_device_release;
    
    return pdm_dev;
}
```

**优势：**
- 统一的对象创建
- 自动资源管理
- 一致的初始化

### 7.5 观察者模式（Observer Pattern）

使用 Linux 设备模型的 uevent 机制：

```c
static int pdm_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    struct pdm_device *pdm_dev = to_pdm_device(dev);
    
    add_uevent_var(env, "DEVICE_TYPE=%s", pdm_device_type_name(pdm_dev));
    add_uevent_var(env, "COMPATIBLE=%s", pdm_dev->compatible);
    
    return 0;
}
```

**效果：**
- 用户空间通过 udev 监听设备事件
- 自动触发设备节点创建
- 支持热插拔通知

### 7.6 单例模式（Singleton Pattern）

总线类型是单例：

```c
struct bus_type pdm_bus_type = {
    .name = "pdm",
    ...
};

// 系统中只有一个 PDM 总线实例
bus_register(&pdm_bus_type);
```

---

## 八、目录结构

```
kernel/pdm/
├── pdm.c                          # 主模块入口
├── Makefile                       # 构建系统
├── Kconfig                        # 内核配置
│
├── bus/                           # 总线基础设施 (5 文件)
│   ├── pdm_bus.c                  # PDM 总线实现
│   ├── pdm_device.c               # 设备模型
│   ├── pdm_of_bus.c               # 设备树枚举
│   ├── pdm_cdev.c                 # 字符设备接口
│   └── pdm_compat_bus.c           # 兼容性层
│
├── registry/                      # 注册表系统 (6 文件)
│   ├── pdm_driver_registry.c      # 驱动注册表
│   ├── pdm_driver_registry_start.c
│   ├── pdm_driver_registry_stop.c
│   ├── pdm_backend_registry.c     # 后端注册表
│   ├── pdm_backend_registry_start.c
│   └── pdm_backend_registry_stop.c
│
├── diag/                          # 诊断接口 (4 文件)
│   ├── pdm_debugfs.c              # DebugFS 接口
│   ├── pdm_proc.c                 # Procfs 接口
│   ├── pdm_sysfs.c                # Sysfs 属性
│   └── pdm_diag_interfaces.c      # 诊断辅助
│
├── drivers/                       # 外设驱动
│   ├── led/                       # LED 驱动 (4 文件)
│   │   ├── pdm_led.c              # LED 驱动核心
│   │   ├── pdm_led_internal.h     # 内部头文件
│   │   ├── pdm_led_gpio.c         # GPIO 控制后端
│   │   └── pdm_led_pwm.c          # PWM 控制后端
│   │
│   ├── mcu/                       # MCU 驱动 (8 文件)
│   │   ├── pdm_mcu.c              # MCU 驱动核心
│   │   ├── pdm_mcu_internal.h     # 内部头文件
│   │   ├── pdm_mcu_protocol.c     # 协议层
│   │   ├── pdm_mcu_i2c.c          # I2C 传输后端
│   │   ├── pdm_mcu_spi.c          # SPI 传输后端
│   │   ├── pdm_mcu_uart.c         # UART 传输层
│   │   ├── pdm_mcu_uart_serdev.c  # UART Serdev 实现
│   │   └── pdm_mcu_can.c          # CAN 传输后端
│   │
│   └── mock/                      # Mock 设备（测试用）
│       ├── pdm_mock_devices.c     # Mock 设备实现
│       └── pdm_mock_devices.h     # Mock 设备头文件
```

**头文件位置**：

```
kernel/include/pdm/
├── bus/
│   ├── pdm_bus.h                  # 总线接口定义
│   └── pdm_device.h               # 设备结构定义
├── registry/
│   ├── pdm_backend.h              # 后端注册接口
│   └── pdm_driver.h               # 驱动注册接口
├── compat/
│   ├── pdm_compat_features.h      # 内核版本特性检测
│   ├── pdm_compat_i2c.h           # I2C 接口兼容性
│   └── pdm_compat_sysfs.h         # Sysfs 接口兼容性
└── diag/
    └── pdm_diag.h                 # 诊断接口定义
```

**兼容性层说明**：

PDM 框架使用兼容性头文件适配不同 Linux 内核版本的 API 变化：

- **pdm_compat_features.h**：内核版本特性检测
  - 支持 Linux 5.10+ 内核
  - 检测 `proc_ops` vs `file_operations`（5.6+）
  - 检测 `sysfs_emit()` 可用性（5.10+）
  - 检测 SPI remove 回调签名变化（6.1+）
  - 检测 sockaddr 结构变化（7.0+）

- **pdm_compat_i2c.h**：I2C probe 接口适配
  - 旧版：`probe(struct i2c_client *, const struct i2c_device_id *)`
  - 新版：`probe(struct i2c_client *)`（5.13+）

- **pdm_compat_sysfs.h**：Sysfs 接口适配
  - 提供 `sysfs_emit()` 的回退实现
  - 统一的属性显示函数签名

**用户空间头文件**：

```
uapi/pdm/
├── pdm_led.h                      # LED ioctl 定义
└── pdm_mcu.h                      # MCU ioctl 定义
```

---

## 总结

PDM 子系统是一个设计良好的 Linux 内核框架，具有以下特点：

1. **模块化设计**：清晰的层次结构，职责分离
2. **可扩展性**：易于添加新设备类型和传输后端
3. **DTS 驱动**：零配置自动枚举
4. **类型安全**：编译时类型检查
5. **运行时灵活**：动态后端选择
6. **标准化接口**：遵循 Linux 驱动模型
7. **诊断友好**：多层次调试接口

该框架为嵌入式外设管理提供了统一、高效、易维护的解决方案。


---

## 九、DTS 节点到 PDM 总线的挂载机制

### 9.1 核心问题

在实际使用中，我们只在 DTS 中配置**一个节点**：

```dts
&i2c1 {
    clock-frequency = <100000>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_i2c1>;
    status = "okay";

    pdm_mcu_i2c: mcu@10 {
        compatible = "pdm,mcu-i2c";
        reg = <0x10>;
        pdm,id = <2>;
        rx-timeout-ms = <100>;
        command-bytes = <1>;
        address-bytes = <1>;
    };
};
```

这个节点物理上在 **I2C 总线**下，但最终却能挂载到 **PDM 总线**上。这是如何实现的？

### 9.2 双重匹配机制

这个 DTS 节点会触发**两次驱动匹配**，匹配到**两个不同的驱动**：

#### 第一次匹配：I2C 子系统驱动

```
I2C 总线扫描
    ↓
发现节点：compatible = "pdm,mcu-i2c"
    ↓
匹配 I2C 驱动：pdm_mcu_i2c_driver
    ↓
创建 i2c_client 对象
    ↓
调用 pdm_mcu_i2c_probe()
```

**代码位置**：`kernel/pdm/drivers/mcu/pdm_mcu_i2c.c`

```c
static const struct of_device_id pdm_mcu_i2c_of_match[] = {
    { .compatible = "pdm,mcu-i2c" },
    { /* sentinel */ }
};

static struct i2c_driver pdm_mcu_i2c_driver = {
    .driver = {
        .name = "pdm-mcu-i2c",
        .of_match_table = pdm_mcu_i2c_of_match,
    },
    .probe = pdm_mcu_i2c_probe,    // I2C 驱动的 probe
    .remove = pdm_mcu_i2c_remove,
};
```

#### 第二次匹配：PDM 总线驱动

```
pdm_device_register()
    ↓
device_add() → 触发 PDM 总线匹配
    ↓
pdm_bus_device_match()
    ↓
匹配到 pdm_mcu_driver
    ↓
调用 pdm_mcu_probe()
```

**代码位置**：`kernel/pdm/drivers/mcu/pdm_mcu.c`

```c
static const struct of_device_id pdm_mcu_of_match[] = {
    { .compatible = "pdm,mcu-i2c" },  // 相同的 compatible
    { .compatible = "pdm,mcu-spi" },
    { .compatible = "pdm,mcu-uart" },
    { /* sentinel */ }
};

static struct pdm_driver pdm_mcu_driver = {
    .driver = {
        .name = "pdm_mcu",
        .of_match_table = pdm_mcu_of_match,
    },
    .probe = pdm_mcu_probe,  // PDM 驱动的 probe
    .remove = pdm_mcu_remove,
};
```

### 9.3 I2C 驱动 probe：从 I2C 总线到 PDM 总线的桥梁

**关键代码**：`kernel/pdm/drivers/mcu/pdm_mcu_i2c.c`

```c
static int pdm_mcu_i2c_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct pdm_device *pdm_dev;
    struct pdm_mcu_bus_device *bus_dev;
    int pdm_id;
    int ret;
    
    dev_info(dev, "Probing PDM MCU I2C device at 0x%02x\n", client->addr);
    
    // ==================== 步骤 1: 分配 PDM 设备 ====================
    // 注意：这是一个新的设备对象，不是 DTS 直接创建的
    pdm_dev = pdm_device_alloc(sizeof(*bus_dev));
    if (!pdm_dev)
        return -ENOMEM;
    
    // ==================== 步骤 2: 保存 I2C 客户端信息 ====================
    // 获取配置数据区域，保存 I2C 总线信息
    bus_dev = pdm_dev->config_data;
    bus_dev->bus_type = PDM_MCU_BUS_TYPE_I2C;
    bus_dev->bus.i2c = client;  // 保存 i2c_client 指针（关键！）
    bus_dev->pdm_dev = pdm_dev;
    
    // ==================== 步骤 3: 设置 PDM 设备属性 ====================
    // 父设备是 i2c_client（建立设备层次关系）
    pdm_dev->dev.parent = &client->dev;
    
    // 共享同一个 DTS 节点
    pdm_dev->dev.of_node = of_node_get(client->dev.of_node);
    
    // 设置 compatible（用于 PDM 总线匹配）
    pdm_dev->compatible = "pdm,mcu-i2c";
    
    // ==================== 步骤 4: 读取 PDM 设备 ID ====================
    // 从 DTS 读取 pdm,id 属性
    if (!of_property_read_u32(dev->of_node, "pdm,id", &pdm_id))
        pdm_device_set_requested_id(pdm_dev, pdm_id);
    
    // ==================== 步骤 5: 注册到 PDM 总线 ====================
    // 这会触发 PDM 总线的驱动匹配
    ret = pdm_device_register(pdm_dev, "mcu");
    if (ret < 0) {
        dev_err(dev, "Failed to register PDM device: %d\n", ret);
        goto err_free_pdm_dev;
    }
    
    // ==================== 步骤 6: 保存关联 ====================
    // 在 i2c_client 中保存 bus_dev 指针，便于后续访问
    i2c_set_clientdata(client, bus_dev);
    
    dev_info(dev, "PDM MCU I2C device registered as mcu%d\n", pdm_dev->id);
    
    return 0;

err_free_pdm_dev:
    pdm_device_put(pdm_dev);
    return ret;
}
```

### 9.4 设备对象的创建和关联

#### 9.4.1 两个设备对象

一个 DTS 节点会产生**两个独立的设备对象**：

| 维度 | I2C 设备 (i2c_client) | PDM 设备 (pdm_device) |
|------|----------------------|----------------------|
| **总线** | `i2c_bus_type` | `pdm_bus_type` |
| **创建者** | I2C 核心自动创建 | I2C 驱动 probe 中手动创建 |
| **父设备** | I2C 适配器 | i2c_client（形成父子关系） |
| **驱动** | `pdm_mcu_i2c_driver` | `pdm_mcu_driver` |
| **DTS 节点** | `mcu@10` | `mcu@10`（共享同一个节点） |
| **作用** | 管理 I2C 通信通道 | 实现 MCU 设备逻辑 |
| **设备文件** | 无 | `/dev/pdm/mcu2` |
| **sysfs 路径** | `/sys/devices/.../i2c-X/X-0010/` | `/sys/bus/pdm/devices/mcu2/` |

#### 9.4.2 设备关联结构

```c
struct pdm_mcu_bus_device {
    u32 bus_type;                    // PDM_MCU_BUS_TYPE_I2C
    union {
        struct i2c_client *i2c;      // 指向 I2C 设备对象
        struct spi_device *spi;
        struct serdev_device *serdev;
    } bus;
    struct pdm_device *pdm_dev;      // 指向 PDM 设备对象
};

// 保存在 pdm_device 的 config_data 中
pdm_dev->config_data = bus_dev;

// 同时保存在 i2c_client 的 driver_data 中
i2c_set_clientdata(client, bus_dev);
```

**双向关联**：

```
i2c_client ←──────────┐
    ↓                 │
    │ clientdata      │
    ↓                 │
bus_dev               │
    ├── bus.i2c ──────┘
    └── pdm_dev ──→ pdm_device
                        ↓
                        │ config_data
                        ↓
                    bus_dev (same)
```

### 9.5 完整的设备树关系图

```
原始 DTS 节点（物理上在 I2C 总线下）
┌────────────────────────────────────┐
│ &i2c1 {                            │
│   mcu@10 {                         │
│     compatible = "pdm,mcu-i2c";    │ ← DTS 节点（唯一的配置）
│     reg = <0x10>;                  │
│     pdm,id = <2>;                  │
│   };                               │
│ }                                  │
└────────────────────────────────────┘
         │
         │ 内核启动，I2C 核心扫描
         ↓
┌────────────────────────────────────┐
│ i2c_client                         │
│ - addr = 0x10                      │
│ - dev.of_node → mcu@10 节点        │ ← I2C 设备对象（自动创建）
│ - dev.bus = &i2c_bus_type          │
│ - dev.parent = i2c_adapter         │
└────────────────────────────────────┘
         │
         │ I2C 总线驱动匹配
         ↓
┌────────────────────────────────────┐
│ pdm_mcu_i2c_driver 匹配成功         │
│                                    │
│ pdm_mcu_i2c_probe(client)          │
│   │                                │
│   ├─ 创建 pdm_device               │
│   │    ↓                           │
│   │  ┌──────────────────────────┐ │
│   │  │ pdm_device               │ │
│   │  │ - dev.parent = client    │ │ ← PDM 设备对象（手动创建）
│   │  │ - dev.of_node → mcu@10   │ │   （共享 DTS 节点）
│   │  │ - dev.bus = pdm_bus_type │ │
│   │  │ - compatible = "..."     │ │
│   │  │ - config_data:           │ │
│   │  │     ├─ bus_type = I2C    │ │
│   │  │     └─ bus.i2c = client ─┼─┼─→ 引用 i2c_client
│   │  └──────────────────────────┘ │
│   │                                │
│   └─ pdm_device_register()         │
└────────────────────────────────────┘
         │
         │ device_add() 触发 PDM 总线匹配
         ↓
┌────────────────────────────────────┐
│ pdm_mcu_driver 匹配成功             │
│                                    │
│ pdm_mcu_probe(pdm_dev)             │
│   │                                │
│   ├─ 获取 bus_dev                  │
│   │    = pdm_dev->config_data      │
│   │                                │
│   ├─ 获取 i2c_client               │
│   │    = bus_dev->bus.i2c          │
│   │                                │
│   ├─ 查找 transport 后端           │
│   │    pdm_backend_find(...)       │
│   │    → pdm_mcu_i2c_ops           │
│   │                                │
│   ├─ 调用 transport setup          │
│   │    inst->transport_ops->setup()│
│   │    （内部使用 i2c_client）     │
│   │                                │
│   └─ 创建字符设备                  │
│        /dev/pdm/mcu2               │
└────────────────────────────────────┘
```

### 9.6 sysfs 设备层次结构

实际在 sysfs 中的设备层次：

```
/sys/devices/platform/
└── 2100000.i2c/                    (I2C 控制器平台设备)
    └── i2c-0/                      (I2C 适配器)
        └── 0-0010/                 (i2c_client, 地址 0x10)
            ├── name: "pdm-mcu-i2c"
            ├── modalias: "of:NmcuT<NULL>Cpdm,mcu-i2c"
            ├── driver -> ../../../../bus/i2c/drivers/pdm-mcu-i2c/
            └── mcu2/               (pdm_device, 父设备是 i2c_client)
                ├── subsystem -> ../../../../../../bus/pdm/
                ├── driver -> ../../../../../../bus/pdm/drivers/pdm_mcu/
                ├── device_type: "MCU"
                ├── compatible: "pdm,mcu-i2c"
                ├── state: "bound"
                └── dev             (字符设备节点 mcu2)

/sys/bus/i2c/devices/
└── 0-0010 -> ../../../devices/platform/2100000.i2c/i2c-0/0-0010/

/sys/bus/pdm/devices/
└── mcu2 -> ../../../devices/platform/2100000.i2c/i2c-0/0-0010/mcu2/

/dev/
└── pdm/
    └── mcu2                        (字符设备)
```

### 9.7 数据传输路径

当用户空间执行 ioctl 操作时，完整的调用路径：

```
用户空间
    ↓
  ioctl(fd, PDM_MCU_IOCTL_XFER, &req)
    ↓ (系统调用)
  pdm_mcu_fops.unlocked_ioctl
    ↓
  pdm_mcu_ioctl(filp, cmd, arg)         [pdm_mcu.c]
    │
    ├─ 从 filp 获取 pdm_device
    │    pdm_dev = filp->private_data
    │
    ├─ 从 pdm_device 获取 instance
    │    inst = pdm_device_get_drvdata(pdm_dev)
    │
    └─ 调用协议层
         ↓
  pdm_mcu_protocol_dispatch(inst, cmd, arg)  [pdm_mcu_protocol.c]
    │
    ├─ 构造 pdm_mcu_xfer 结构
    │
    └─ 调用 transport 层
         ↓
  inst->transport_ops->xfer(inst, &xfer)     [函数指针]
    ↓
  pdm_mcu_i2c_xfer(inst, xfer)               [pdm_mcu_i2c.c]
    │
    ├─ 从 inst 获取 i2c_client
    │    client = inst->transport.i2c.client
    │    （这个 client 就是最初 I2C 核心创建的）
    │
    └─ 执行 I2C 传输
         ↓
  pdm_mcu_i2c_bus_xfer(client, ...)
    ↓
  i2c_transfer(client->adapter, msgs, num_msgs)  [Linux I2C 核心]
    ↓
  i2c_adapter->algo->master_xfer(...)            [硬件驱动]
    ↓
硬件 I2C 控制器
```

### 9.8 为什么采用这种设计？

#### 优势

1. **复用成熟的 Linux 子系统**
   - I2C/SPI/UART 核心已经处理了大量硬件细节
   - 设备树解析、地址管理、错误处理等都由子系统完成

2. **统一的 PDM 接口**
   - 所有 PDM 设备通过相同的驱动（pdm_mcu_driver/pdm_led_driver）
   - 用户空间看到统一的 `/dev/pdm/*` 接口

3. **灵活的传输层**
   - I2C/SPI 驱动只负责设备创建和通道管理
   - 实际传输逻辑由 transport 后端实现
   - 便于添加新的传输类型

4. **清晰的职责分离**
   - I2C 驱动：总线设备管理
   - PDM 驱动：设备逻辑实现
   - Transport 后端：传输协议实现

5. **DTS 配置简洁**
   - 只需一个节点，不需要额外的 PDM 总线节点
   - 所有配置（I2C 地址、PDM ID、传输参数）在一个地方

#### 类比

这种设计类似于**网络协议栈**：

```
应用层：PDM 设备逻辑（pdm_mcu_driver）
传输层：PDM transport 后端（pdm_mcu_i2c_ops）
网络层：I2C 总线驱动（pdm_mcu_i2c_driver）
链路层：I2C 核心和硬件驱动
```

每一层都有明确的职责，上层通过接口访问下层，而不需要知道底层实现细节。

### 9.9 其他传输类型的类似实现

#### SPI 传输

```dts
&spi1 {
    mcu@0 {
        compatible = "pdm,mcu-spi";
        reg = <0>;  // SPI CS
        spi-max-frequency = <1000000>;
    };
};
```

流程与 I2C 相同：
1. SPI 核心创建 `spi_device`
2. `pdm_mcu_spi_driver`（SPI 驱动）匹配
3. `pdm_mcu_spi_probe()` 创建 `pdm_device` 并注册到 PDM 总线
4. `pdm_mcu_driver` 匹配并 probe

#### UART 传输

```dts
&uart2 {
    mcu {
        compatible = "pdm,mcu-uart";
        current-speed = <115200>;
    };
};
```

流程与 I2C 相同：
1. Serdev 核心创建 `serdev_device`
2. `pdm_mcu_uart_serdev_driver` 匹配
3. `pdm_mcu_uart_serdev_probe()` 创建 `pdm_device` 并注册到 PDM 总线
4. `pdm_mcu_driver` 匹配并 probe

### 9.10 总结

**一个 DTS 节点，两个设备对象，两次驱动匹配**：

1. **第一阶段**：I2C/SPI/UART 子系统处理
   - 创建 `i2c_client`/`spi_device`/`serdev_device`
   - 匹配对应的总线驱动
   - 总线驱动 probe 中创建 PDM 设备

2. **第二阶段**：PDM 总线处理
   - `pdm_device` 注册到 PDM 总线
   - 匹配 PDM 驱动
   - PDM 驱动 probe 中实现设备逻辑

3. **设备关联**：
   - PDM 设备的父设备是总线设备（形成层次关系）
   - PDM 设备通过 `config_data` 保存总线设备引用
   - 数据传输时通过引用访问底层总线设备

这种设计实现了**分层解耦**，每一层专注于自己的职责，同时保持了灵活性和可扩展性。

