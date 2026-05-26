# EMS Defconfig 快速参考

## 配置文件速查表

| 场景 | 配置文件 | 命令 |
|------|---------|------|
| **ARM64 开发** | ccm_h200_am625_debug | `make ccm_h200_am625_debug_defconfig` |
| **ARM64 生产** | ccm_h200_am625_release | `make ccm_h200_am625_release_defconfig` |
| **ARM64 测试** | ccm_h200_am625_test | `make ccm_h200_am625_test_defconfig` |
| **x86_64 开发** | x86_64_full | `make x86_64_full_defconfig` |
| **x86_64 生产** | x86_64_release | `make x86_64_release_defconfig` |
| **x86_64 最小** | x86_64_minimal | `make x86_64_minimal_defconfig` |
| **x86_64 CI** | x86_64_ci | `make x86_64_ci_defconfig` |
| **树莓派** | arm64_raspberrypi | `make arm64_raspberrypi_defconfig` |
| **BeagleBone** | arm32_beaglebone | `make arm32_beaglebone_defconfig` |
| **RTOS** | arm32_rtos_minimal | `make arm32_rtos_minimal_defconfig` |
| **RISC-V** | riscv64_linux | `make riscv64_linux_defconfig` |
| **默认** | defconfig | `make defconfig` |

## 常用命令

### 配置管理
```bash
# 加载配置
make ccm_h200_am625_debug_defconfig

# 图形化配置
make menuconfig

# 保存配置
make savedefconfig

# 查看当前配置
cat .config | grep "^CONFIG"
```

### 配置对比工具
```bash
# 列出所有配置
./scripts/compare_defconfig.sh -l

# 显示配置摘要
./scripts/compare_defconfig.sh -s

# 查看配置详情
./scripts/compare_defconfig.sh -d ccm_h200_am625_debug_defconfig

# 对比两个配置
./scripts/compare_defconfig.sh -c \
    ccm_h200_am625_debug_defconfig \
    ccm_h200_am625_release_defconfig
```

### 构建命令
```bash
# 编译
make -j$(nproc)

# 详细输出
make V=1

# 清理
make clean

# 深度清理
make mrproper

# 安装
make install
```

## 配置特征速查

### 按架构
- **x86_64**: minimal, full, release, ci, defconfig
- **ARM64**: ccm_debug, ccm_release, ccm_test, raspberrypi
- **ARM32**: beaglebone, rtos_minimal
- **RISC-V64**: riscv64_linux

### 按优化级别
- **-O0** (调试): ccm_debug, ccm_test
- **-O2** (平衡): ccm_release, beaglebone, raspberrypi, riscv64, full, ci, defconfig
- **-O3** (性能): x86_64_release
- **-Os** (大小): x86_64_minimal, arm32_rtos_minimal

### 按库类型
- **静态库**: x86_64_minimal, arm32_rtos_minimal
- **动态库**: ccm_debug, ccm_release, beaglebone, raspberrypi, riscv64
- **双库**: ccm_test, x86_64_full, x86_64_release, x86_64_ci, defconfig

### 按功能集
- **最小化**: x86_64_minimal, arm32_rtos_minimal
- **标准**: defconfig, ccm_release, beaglebone, raspberrypi, riscv64
- **完整**: x86_64_full, ccm_debug, ccm_test, x86_64_ci
- **生产**: ccm_release, x86_64_release

## 典型使用场景

### 场景 1: 本地开发 (x86_64)
```bash
make x86_64_full_defconfig
make -j$(nproc)
./bin/ccm_supervisor
```

### 场景 2: ARM64 目标调试
```bash
make ccm_h200_am625_debug_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
# 部署到目标板
scp bin/* root@target:/usr/local/bin/
```

### 场景 3: 生产构建
```bash
make ccm_h200_am625_release_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
make install DESTDIR=/tmp/rootfs
```

### 场景 4: CI/CD 测试
```bash
make x86_64_ci_defconfig
make -j$(nproc)
make test
```

### 场景 5: 最小化部署
```bash
make x86_64_minimal_defconfig
make -j$(nproc)
ls -lh bin/  # 只有 supervisor 和 logger
```

### 场景 6: 树莓派原型
```bash
make arm64_raspberrypi_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
# 复制到 SD 卡
```

## 配置选项速查

### 关键配置项
```kconfig
# 架构
CONFIG_ARCH_X86_64=y
CONFIG_ARCH_ARM32=y
CONFIG_ARCH_ARM64=y
CONFIG_ARCH_RISCV64=y

# 操作系统
CONFIG_OS_LINUX=y
CONFIG_OS_RTOS=y
CONFIG_OS_WINDOWS=y
CONFIG_OS_MACOS=y
CONFIG_OS_BARE=y

# 优化级别
CONFIG_OPT_O0=y  # 调试
CONFIG_OPT_O2=y  # 平衡
CONFIG_OPT_O3=y  # 性能
CONFIG_OPT_OS=y  # 大小

# 库类型
CONFIG_LIBRARY_BUILD_TYPE_STATIC=y
CONFIG_LIBRARY_BUILD_TYPE_DYNAMIC=y

# 调试
CONFIG_BUILD_TYPE_DEBUG=y
CONFIG_OSAL_DEBUG_LOGGING=y
CONFIG_HAL_DEBUG=y

# 测试
CONFIG_BUILD_TESTING=y

# 平台
CONFIG_PLATFORM_GENERIC_LINUX=y
CONFIG_PLATFORM_TI_AM625=y
CONFIG_PLATFORM_RASPBERRY_PI=y
CONFIG_PLATFORM_BEAGLEBONE=y
CONFIG_PLATFORM_CUSTOM=y
```

### HAL 驱动选项
```kconfig
# 基础驱动
CONFIG_HAL_CAN=y
CONFIG_HAL_UART=y
CONFIG_HAL_I2C=y
CONFIG_HAL_SPI=y
CONFIG_HAL_GPIO=y
CONFIG_HAL_WATCHDOG=y

# 高级特性
CONFIG_HAL_CAN_FILTER=y
CONFIG_HAL_CAN_ERROR_HANDLING=y
CONFIG_HAL_CAN_LOOPBACK=y
CONFIG_HAL_UART_FLOW_CONTROL=y
CONFIG_HAL_UART_RS485=y
CONFIG_HAL_I2C_SLAVE=y
CONFIG_HAL_I2C_10BIT_ADDR=y
CONFIG_HAL_SPI_DMA=y
CONFIG_HAL_SPI_SLAVE=y
CONFIG_HAL_GPIO_INTERRUPT=y
CONFIG_HAL_GPIO_DEBOUNCE=y
CONFIG_HAL_WATCHDOG_PRETIMEOUT=y
```

### OSAL 功能选项
```kconfig
# 功能模块
CONFIG_OSAL_FILE=y
CONFIG_OSAL_NETWORK=y
CONFIG_OSAL_IPC=y
CONFIG_OSAL_THREAD=y
CONFIG_OSAL_TIME=y
CONFIG_OSAL_SIGNAL=y

# 资源限制
CONFIG_OSAL_MAX_TASKS=64
CONFIG_OSAL_MAX_QUEUES=32
CONFIG_OSAL_MAX_MUTEXES=32
```

### CCM 应用选项
```kconfig
CONFIG_BUILD_CCM_SUPERVISOR=y
CONFIG_BUILD_CCM_COLLECTOR=y
CONFIG_BUILD_CCM_COMM=y
CONFIG_BUILD_CCM_HEALTH=y
CONFIG_BUILD_CCM_LOGGER=y
```

## 故障排除

### 问题: 配置加载失败
```bash
# 检查配置文件是否存在
ls -l configs/ccm_h200_am625_debug_defconfig

# 尝试手动复制
cp configs/ccm_h200_am625_debug_defconfig .config
make oldconfig
```

### 问题: 交叉编译失败
```bash
# 检查工具链
which aarch64-linux-gnu-gcc

# 显式指定工具链
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1
```

### 问题: 库链接错误
```bash
# 检查库是否构建
ls -l lib/

# 重新构建
make clean
make -j$(nproc)
```

### 问题: 配置不生效
```bash
# 深度清理后重新配置
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)
```

## 文档链接

- **详细指南**: [configs/README.md](../configs/README.md)
- **配置矩阵**: [docs/DEFCONFIG_MATRIX.md](DEFCONFIG_MATRIX.md)
- **构建系统**: [docs/BUILD_SYSTEM.md](BUILD_SYSTEM.md)
- **平台配置**: [docs/PLATFORM.md](PLATFORM.md)

---

**提示**: 使用 `./scripts/compare_defconfig.sh -s` 快速查看所有配置的摘要信息。
