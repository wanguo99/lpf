# EMS SDK 开发者指南

本指南面向希望为 EMS SDK 添加新功能、新模块或贡献代码的开发者。

---

## 📋 目录

1. [开发环境设置](#1-开发环境设置)
2. [添加新的 HAL 驱动](#2-添加新的-hal-驱动)
3. [添加新的 PDL 模块](#3-添加新的-pdl-模块)
4. [添加新的应用](#4-添加新的应用)
5. [添加新的产品](#5-添加新的产品)
6. [编写测试](#6-编写测试)
7. [代码审查规范](#7-代码审查规范)
8. [提交代码](#8-提交代码)

---

## 1. 开发环境设置

### 1.1 克隆仓库

```bash
git clone https://github.com/wanguo99/EMS.git
cd EMS
```

### 1.2 安装开发工具

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    python3 \
    libncurses-dev \
    flex \
    bison \
    git \
    clang-format \
    cppcheck \
    valgrind

# macOS
brew install cmake python3 ncurses flex bison clang-format cppcheck
```

### 1.3 配置 Git

```bash
# 设置用户信息
git config user.name "Your Name"
git config user.email "your.email@example.com"

# 设置编辑器
git config core.editor "vim"
```

### 1.4 创建开发分支

```bash
# 从 master 创建功能分支
git checkout -b feature/my-new-feature

# 或修复分支
git checkout -b fix/issue-123
```

---

## 2. 添加新的 HAL 驱动

### 2.1 场景：添加 SPI 驱动

假设我们要为 Linux 平台添加 SPI 驱动。

#### 步骤 1: 创建头文件

创建 `core/hal/include/hal_spi.h`:

```c
#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "osal_types.h"

/**
 * @brief SPI 配置结构
 */
typedef struct {
    uint32_t speed_hz;      // 时钟频率
    uint8_t  mode;          // SPI 模式 (0-3)
    uint8_t  bits_per_word; // 每字位数
} hal_spi_config_t;

/**
 * @brief 打开 SPI 设备
 * @param device 设备路径 (如 "/dev/spidev0.0")
 * @param config 配置参数
 * @return 设备句柄，失败返回 NULL
 */
void* HAL_SPI_Open(const char *device, const hal_spi_config_t *config);

/**
 * @brief 关闭 SPI 设备
 * @param handle 设备句柄
 * @return 0 成功，负数失败
 */
int32_t HAL_SPI_Close(void *handle);

/**
 * @brief SPI 传输
 * @param handle 设备句柄
 * @param tx_buf 发送缓冲区
 * @param rx_buf 接收缓冲区
 * @param len 传输长度
 * @return 实际传输字节数，负数失败
 */
int32_t HAL_SPI_Transfer(void *handle, const uint8_t *tx_buf, 
                         uint8_t *rx_buf, size_t len);

#endif /* HAL_SPI_H */
```

#### 步骤 2: 实现驱动

创建 `core/hal/src/linux/hal_spi_linux.c`:

```c
#include "hal_spi.h"
#include "osal.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

typedef struct {
    int fd;
    hal_spi_config_t config;
} hal_spi_handle_t;

void* HAL_SPI_Open(const char *device, const hal_spi_config_t *config)
{
    if (!device || !config) {
        return NULL;
    }

    hal_spi_handle_t *handle = OSAL_Malloc(sizeof(hal_spi_handle_t));
    if (!handle) {
        return NULL;
    }

    // 打开设备
    handle->fd = open(device, O_RDWR);
    if (handle->fd < 0) {
        OSAL_Free(handle);
        return NULL;
    }

    // 配置 SPI 模式
    if (ioctl(handle->fd, SPI_IOC_WR_MODE, &config->mode) < 0) {
        close(handle->fd);
        OSAL_Free(handle);
        return NULL;
    }

    // 配置速度
    if (ioctl(handle->fd, SPI_IOC_WR_MAX_SPEED_HZ, &config->speed_hz) < 0) {
        close(handle->fd);
        OSAL_Free(handle);
        return NULL;
    }

    // 配置位数
    if (ioctl(handle->fd, SPI_IOC_WR_BITS_PER_WORD, &config->bits_per_word) < 0) {
        close(handle->fd);
        OSAL_Free(handle);
        return NULL;
    }

    handle->config = *config;
    return handle;
}

int32_t HAL_SPI_Close(void *handle)
{
    if (!handle) {
        return -1;
    }

    hal_spi_handle_t *h = (hal_spi_handle_t *)handle;
    close(h->fd);
    OSAL_Free(h);
    return 0;
}

int32_t HAL_SPI_Transfer(void *handle, const uint8_t *tx_buf, 
                         uint8_t *rx_buf, size_t len)
{
    if (!handle || !tx_buf || !rx_buf || len == 0) {
        return -1;
    }

    hal_spi_handle_t *h = (hal_spi_handle_t *)handle;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = len,
        .speed_hz = h->config.speed_hz,
        .bits_per_word = h->config.bits_per_word,
    };

    int ret = ioctl(h->fd, SPI_IOC_MESSAGE(1), &tr);
    return ret;
}
```

#### 步骤 3: 添加 Kconfig 配置

编辑 `core/hal/src/Kconfig.linux`，添加：

```kconfig
if HAL_SPI

config HAL_SPI_LINUX
	bool "Linux SPI driver"
	default y
	help
	  Enable Linux SPI driver (/dev/spidev*).
	  Source: src/linux/hal_spi_linux.c

endif # HAL_SPI
```

编辑 `core/hal/Kconfig`，添加：

```kconfig
menuconfig HAL_SPI
	bool "SPI (Serial Peripheral Interface)"
	default n
	help
	  Enable SPI driver support.
	  
	  Memory footprint: ~6KB
	  
	  Use cases:
	  - Flash memory access
	  - Sensor communication
	  - Display controllers
```

#### 步骤 4: 更新 CMakeLists.txt

编辑 `core/hal/CMakeLists.txt`，添加：

```cmake
if(CONFIG_HAL_SPI_LINUX)
    list(APPEND ADD_SRCS "src/linux/hal_spi_linux.c")
endif()
```

#### 步骤 5: 测试

```bash
# 配置启用 SPI
python3 build.py menuconfig
# 进入 Core Components → HAL → SPI

# 编译
python3 build.py build

# 验证
grep "hal_spi_linux.c" _build/compile_commands.json
```

---

## 3. 添加新的 PDL 模块

### 3.1 场景：添加 GPS 模块

#### 步骤 1: 创建目录结构

```bash
mkdir -p core/pdl/src/pdl_gps
```

#### 步骤 2: 创建头文件

创建 `core/pdl/include/pdl_gps.h`:

```c
#ifndef PDL_GPS_H
#define PDL_GPS_H

#include "osal_types.h"

typedef struct {
    double latitude;   // 纬度
    double longitude;  // 经度
    double altitude;   // 海拔
    uint32_t timestamp; // 时间戳
} pdl_gps_position_t;

/**
 * @brief 初始化 GPS 模块
 * @param device 串口设备路径
 * @return 0 成功，负数失败
 */
int32_t PDL_GPS_Init(const char *device);

/**
 * @brief 获取当前位置
 * @param pos 位置信息输出
 * @return 0 成功，负数失败
 */
int32_t PDL_GPS_GetPosition(pdl_gps_position_t *pos);

/**
 * @brief 反初始化 GPS 模块
 */
void PDL_GPS_Deinit(void);

#endif /* PDL_GPS_H */
```

#### 步骤 3: 实现模块

创建 `core/pdl/src/pdl_gps/pdl_gps.c`:

```c
#include "pdl_gps.h"
#include "hal_serial.h"
#include "osal.h"
#include <string.h>

static void *gps_handle = NULL;

int32_t PDL_GPS_Init(const char *device)
{
    hal_serial_config_t config = {
        .baudrate = 9600,
        .databits = 8,
        .stopbits = 1,
        .parity = 'N',
    };

    gps_handle = HAL_Serial_Open(device, &config);
    if (!gps_handle) {
        return -1;
    }

    return 0;
}

int32_t PDL_GPS_GetPosition(pdl_gps_position_t *pos)
{
    if (!gps_handle || !pos) {
        return -1;
    }

    // 读取 NMEA 数据
    char buffer[256];
    int32_t len = HAL_Serial_Read(gps_handle, (uint8_t *)buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        return -1;
    }
    buffer[len] = '\0';

    // 解析 NMEA 数据（简化示例）
    // 实际应该解析 $GPGGA 等语句
    // TODO: 实现完整的 NMEA 解析

    return 0;
}

void PDL_GPS_Deinit(void)
{
    if (gps_handle) {
        HAL_Serial_Close(gps_handle);
        gps_handle = NULL;
    }
}
```

#### 步骤 4: 添加 Kconfig

创建 `core/pdl/src/pdl_gps/Kconfig`:

```kconfig
config PDL_GPS_SUPPORT
	bool "GPS module support"
	default n
	help
	  Enable GPS module support.
	  
	  Memory footprint: ~8KB
	  
	  Features:
	  - NMEA protocol parsing
	  - Position tracking
	  - Satellite information
```

编辑 `core/pdl/Kconfig`，添加：

```kconfig
source "core/pdl/src/pdl_gps/Kconfig"
```

#### 步骤 5: 更新 CMakeLists.txt

编辑 `core/pdl/CMakeLists.txt`，添加：

```cmake
if(CONFIG_PDL_GPS_SUPPORT)
    file(GLOB PDL_GPS_SRCS "src/pdl_gps/*.c")
    list(APPEND ADD_SRCS ${PDL_GPS_SRCS})
endif()
```

---

## 4. 添加新的应用

### 4.1 场景：添加数据采集应用

#### 步骤 1: 创建目录

```bash
mkdir -p products/ccm/apps/app_collector_v2/src
```

#### 步骤 2: 编写应用代码

创建 `products/ccm/apps/app_collector_v2/src/main.c`:

```c
#include <stdio.h>
#include "osal.h"
#include "pdl_satellite.h"

static void* collector_thread(void *arg)
{
    (void)arg;

    while (1) {
        // 采集数据
        printf("Collecting data...\n");

        // 休眠 1 秒
        OSAL_Sleep(1000);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    printf("Data Collector V2 Starting...\n");

    // 初始化 PDL
    PDL_Satellite_Init();

    // 创建采集线程
    osal_thread_t *thread = NULL;
    OSAL_ThreadCreate(&thread, collector_thread, NULL);

    // 等待线程
    OSAL_ThreadJoin(thread);

    // 清理
    PDL_Satellite_Deinit();

    return 0;
}
```

#### 步骤 3: 创建 CMakeLists.txt

创建 `products/ccm/apps/app_collector_v2/CMakeLists.txt`:

```cmake
# 收集源文件
file(GLOB APP_SRCS "src/*.c")

# 创建可执行文件
add_executable(collector_v2 ${APP_SRCS})

# 链接依赖库
target_link_libraries(collector_v2
    PRIVATE
        h200_am625
        ccm
        pdl
        osal
)
```

#### 步骤 4: 注册应用

编辑 `products/ccm/CMakeLists.txt`，添加：

```cmake
add_subdirectory(apps/app_collector_v2)
```

#### 步骤 5: 添加 Kconfig

创建 `products/ccm/apps/app_collector_v2/Kconfig`:

```kconfig
config APP_COLLECTOR_V2
	bool "Data Collector V2"
	default n
	help
	  Enhanced data collection application.
```

编辑 `products/ccm/Kconfig`，添加：

```kconfig
source "products/ccm/apps/app_collector_v2/Kconfig"
```

---

## 5. 添加新的产品

### 5.1 场景：添加新产品 "MyProduct"

#### 步骤 1: 创建产品目录

```bash
mkdir -p products/myproduct/{apps,libs,configs}
mkdir -p products/myproduct/apps/app_main/src
```

#### 步骤 2: 创建产品 Kconfig

创建 `products/myproduct/Kconfig`:

```kconfig
config PRODUCT_MYPRODUCT
	bool "MyProduct"
	default n
	select OSAL
	select HAL
	help
	  My custom product.

if PRODUCT_MYPRODUCT

source "products/myproduct/apps/app_main/Kconfig"

endif # PRODUCT_MYPRODUCT
```

#### 步骤 3: 创建产品 CMakeLists.txt

创建 `products/myproduct/CMakeLists.txt`:

```cmake
if(CONFIG_PRODUCT_MYPRODUCT)
    message(STATUS "Building MyProduct")
    
    # 添加应用
    add_subdirectory(apps/app_main)
endif()
```

#### 步骤 4: 创建主应用

创建 `products/myproduct/apps/app_main/src/main.c`:

```c
#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    printf("MyProduct Application\n");
    print_version_info();
    return 0;
}
```

创建 `products/myproduct/apps/app_main/CMakeLists.txt`:

```cmake
file(GLOB APP_SRCS "src/*.c")
add_executable(myproduct_main ${APP_SRCS})
target_link_libraries(myproduct_main PRIVATE osal)
```

#### 步骤 5: 注册产品

编辑根目录 `Kconfig`，添加：

```kconfig
source "products/myproduct/Kconfig"
```

编辑根目录 `CMakeLists.txt`，添加：

```cmake
add_subdirectory(products/myproduct)
```

#### 步骤 6: 创建配置文件

创建 `configs/myproduct_default_defconfig`:

```kconfig
# MyProduct Default Configuration

CONFIG_PRODUCT_MYPRODUCT=y

# Core Components
CONFIG_OSAL=y
CONFIG_HAL=y

# Platform
CONFIG_OS_LINUX=y
CONFIG_ARCH_X86_64=y
CONFIG_PLATFORM_LINUX=y

# Applications
CONFIG_APP_MAIN=y
```

#### 步骤 7: 测试

```bash
python3 build.py config myproduct_default
python3 build.py build
./_build/bin/myproduct_main
```

---

## 6. 编写测试

### 6.1 单元测试

创建 `tests/unit/pdl/test_pdl_gps.c`:

```c
#include "tests_core.h"
#include "pdl_gps.h"

static void test_gps_init(void)
{
    int32_t ret = PDL_GPS_Init("/dev/ttyUSB0");
    TEST_ASSERT(ret == 0, "GPS init should succeed");
    
    PDL_GPS_Deinit();
}

static void test_gps_get_position(void)
{
    PDL_GPS_Init("/dev/ttyUSB0");
    
    pdl_gps_position_t pos;
    int32_t ret = PDL_GPS_GetPosition(&pos);
    TEST_ASSERT(ret == 0, "Get position should succeed");
    
    PDL_GPS_Deinit();
}

void test_pdl_gps_register(void)
{
    TEST_REGISTER("PDL GPS Init", test_gps_init);
    TEST_REGISTER("PDL GPS Get Position", test_gps_get_position);
}
```

### 6.2 运行测试

```bash
# 启用测试
python3 build.py menuconfig
# 进入 Testing → Enable unit tests

# 编译测试
python3 build.py build

# 运行测试
./_build/bin/ems_unit_test
```

---

## 7. 代码审查规范

### 7.1 代码风格

遵循 [编码规范](CODING_STANDARDS.md)：

- 使用 Tab 缩进（8 空格宽度）
- 函数名使用 `Module_FunctionName` 格式
- 变量名使用 `snake_case`
- 常量使用 `UPPER_CASE`

### 7.2 代码检查

```bash
# 使用 clang-format 格式化
find core -name "*.c" -o -name "*.h" | xargs clang-format -i

# 使用 cppcheck 静态分析
cppcheck --enable=all core/
```

### 7.3 提交前检查清单

- [ ] 代码符合编码规范
- [ ] 所有配置编译通过
- [ ] 添加了必要的注释
- [ ] 更新了相关文档
- [ ] 添加了测试用例
- [ ] 测试全部通过

---

## 8. 提交代码

### 8.1 提交消息规范

使用语义化提交消息：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**类型 (type)**:
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建/工具

**示例**:
```
feat(hal): 添加 SPI 驱动支持

- 实现 Linux SPI 驱动
- 添加 Kconfig 配置
- 更新 CMakeLists.txt
- 添加单元测试

Closes #123
```

### 8.2 提交流程

```bash
# 1. 添加修改
git add core/hal/

# 2. 提交
git commit -m "feat(hal): 添加 SPI 驱动支持"

# 3. 推送到远程
git push origin feature/spi-driver

# 4. 创建 Pull Request
# 访问 GitHub 创建 PR
```

### 8.3 Pull Request 模板

```markdown
## 变更说明
简要描述本次变更的内容和目的。

## 变更类型
- [ ] 新功能
- [ ] Bug 修复
- [ ] 文档更新
- [ ] 性能优化
- [ ] 重构

## 测试
- [ ] 所有配置编译通过
- [ ] 单元测试通过
- [ ] 手动测试通过

## 相关 Issue
Closes #123

## 检查清单
- [ ] 代码符合编码规范
- [ ] 添加了必要的注释
- [ ] 更新了相关文档
- [ ] 添加了测试用例
```

---

## 9. 常见开发任务

### 9.1 添加新的配置选项

```kconfig
config MY_NEW_FEATURE
	bool "My new feature"
	default n
	help
	  Description of the feature.
	  
	  Memory footprint: ~XKB
```

### 9.2 添加编译选项

```cmake
if(CONFIG_MY_NEW_FEATURE)
    list(APPEND ADD_DEFINITIONS -DMY_FEATURE_ENABLED=1)
endif()
```

### 9.3 添加依赖库

```cmake
target_link_libraries(my_app
    PRIVATE
        osal
        hal
        my_new_lib
)
```

---

## 10. 获取帮助

- 📖 查看 [架构概述](ARCHITECTURE.md)
- 🐛 查看 [故障排除](TROUBLESHOOTING.md)
- 💬 提交 Issue 或 Discussion
- 📧 联系维护者

---

**提示**: 开发新功能前，建议先提交 Issue 讨论设计方案。

**贡献**: 感谢你为 EMS SDK 做出贡献！
