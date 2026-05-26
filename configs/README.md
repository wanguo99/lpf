# EMS Defconfig 配置文件说明

本目录包含针对不同业务场景和硬件平台的预定义配置文件。

## 配置文件列表

### 生产环境配置

#### ccm_h200_am625_release_defconfig
- **目标平台**: TI AM625 SoC (ARM Cortex-A53 + Cortex-R5F)
- **用途**: 生产环境部署
- **优化级别**: -O2 (平衡性能和大小)
- **库类型**: 动态库
- **特性**: 生产必需的驱动和应用，电源管理，看门狗预超时
- **适用场景**: 卫星载荷转接板生产部署

#### x86_64_release_defconfig
- **目标平台**: x86_64 Linux
- **用途**: x86_64 生产环境
- **优化级别**: -O3 (最大性能优化)
- **库类型**: 静态库和动态库
- **特性**: 生产级配置，所有核心驱动，电源管理
- **适用场景**: 地面测试设备、数据处理服务器

### 开发调试配置

#### ccm_h200_am625_debug_defconfig
- **目标平台**: TI AM625 SoC (ARM64)
- **用途**: 开发和调试
- **优化级别**: -O0 (无优化，最佳调试体验)
- **库类型**: 动态库
- **特性**: 所有功能开启，详细日志，统计信息，所有高级特性
- **适用场景**: 硬件驱动开发，协议调试，问题排查

#### x86_64_full_defconfig
- **目标平台**: x86_64 Linux
- **用途**: 完整功能开发测试
- **优化级别**: -O2
- **库类型**: 静态库和动态库
- **特性**: 所有功能和驱动，调试日志，统计信息
- **适用场景**: 本地开发，功能验证，集成测试

### 测试配置

#### ccm_h200_am625_test_defconfig
- **目标平台**: TI AM625 SoC (ARM64)
- **用途**: 自动化测试
- **优化级别**: -O0 (准确的测试覆盖率)
- **库类型**: 静态库和动态库
- **特性**: 所有功能，测试框架，最大资源限制，回环测试
- **适用场景**: CI/CD 自动化测试，压力测试，集成测试

#### x86_64_ci_defconfig
- **目标平台**: x86_64 Linux
- **用途**: 持续集成和自动化测试
- **优化级别**: -O2 (真实性能测试)
- **库类型**: 静态库和动态库
- **特性**: 所有功能，CAN 回环模式，调试日志，测试框架
- **适用场景**: GitHub Actions, Jenkins, GitLab CI

### 最小化配置

#### x86_64_minimal_defconfig
- **目标平台**: x86_64 Linux
- **用途**: 最小化资源占用
- **优化级别**: -Os (最小二进制大小)
- **库类型**: 静态库
- **特性**: 仅核心功能，UART + 看门狗，最小应用集
- **适用场景**: 资源受限系统，嵌入式 x86 设备

#### arm32_rtos_minimal_defconfig
- **目标平台**: ARM32 (ARMv7-A) + RTOS
- **用途**: RTOS/裸机环境
- **优化级别**: -Os (最小占用)
- **库类型**: 静态库
- **特性**: 无文件/网络，直接硬件访问，极小资源限制
- **适用场景**: FreeRTOS, RT-Thread, 裸机系统

### 平台特定配置

#### arm32_beaglebone_defconfig
- **目标平台**: BeagleBone (TI AM335x, ARM Cortex-A8)
- **用途**: BeagleBone 工业应用
- **优化级别**: -O2
- **库类型**: 动态库
- **特性**: BeagleBone 优化驱动，GPIO 去抖，电源管理
- **适用场景**: 工业控制，原型开发

#### arm64_raspberrypi_defconfig
- **目标平台**: Raspberry Pi 4/5 (ARM Cortex-A72/A76)
- **用途**: 树莓派原型开发
- **优化级别**: -O2
- **库类型**: 动态库
- **特性**: 树莓派优化驱动，SPI DMA，GPIO 中断
- **适用场景**: 快速原型，教育，开发验证

#### riscv64_linux_defconfig
- **目标平台**: RISC-V 64-bit + Linux
- **用途**: RISC-V 平台支持
- **优化级别**: -O2
- **库类型**: 动态库
- **特性**: 标准 Linux 配置，通用驱动
- **适用场景**: RISC-V 开发板，新兴架构支持

### 默认配置

#### defconfig
- **目标平台**: x86_64 Linux
- **用途**: 通用默认配置
- **优化级别**: -O2
- **库类型**: 静态库和动态库
- **特性**: 平衡的功能集，常用驱动
- **适用场景**: 首次使用，快速开始

## 使用方法

### 加载配置

```bash
# 方法 1: 使用 make 命令
make ccm_h200_am625_debug_defconfig

# 方法 2: 使用 PRODUCT 变量
make PRODUCT=ccm_h200_am625_debug

# 方法 3: 直接复制
cp configs/ccm_h200_am625_debug_defconfig .config
make oldconfig
```

### 自定义配置

```bash
# 加载基础配置
make ccm_h200_am625_release_defconfig

# 图形化修改
make menuconfig

# 保存为新配置
make savedefconfig
mv defconfig configs/my_custom_defconfig
```

### 验证配置

```bash
# 加载配置
make ccm_h200_am625_debug_defconfig

# 查看生效的配置
cat .config | grep "^CONFIG"

# 编译验证
make -j$(nproc)
```

## 配置对比

| 配置文件 | 架构 | OS | 优化 | 库类型 | 调试 | 测试 | 用途 |
|---------|------|----|----|--------|-----|-----|-----|
| ccm_h200_am625_debug | ARM64 | Linux | -O0 | 动态 | ✓ | ✓ | 开发调试 |
| ccm_h200_am625_release | ARM64 | Linux | -O2 | 动态 | ✗ | ✗ | 生产部署 |
| ccm_h200_am625_test | ARM64 | Linux | -O0 | 双库 | ✓ | ✓ | 自动化测试 |
| x86_64_minimal | x86_64 | Linux | -Os | 静态 | ✗ | ✗ | 最小化 |
| x86_64_full | x86_64 | Linux | -O2 | 双库 | ✓ | ✓ | 完整功能 |
| x86_64_release | x86_64 | Linux | -O3 | 双库 | ✗ | ✗ | 生产部署 |
| x86_64_ci | x86_64 | Linux | -O2 | 双库 | ✓ | ✓ | CI/CD |
| arm32_rtos_minimal | ARM32 | RTOS | -Os | 静态 | ✗ | ✗ | RTOS/裸机 |
| arm32_beaglebone | ARM32 | Linux | -O2 | 动态 | ✗ | ✗ | BeagleBone |
| arm64_raspberrypi | ARM64 | Linux | -O2 | 动态 | ✗ | ✗ | 树莓派 |
| riscv64_linux | RISC-V64 | Linux | -O2 | 动态 | ✗ | ✗ | RISC-V |
| defconfig | x86_64 | Linux | -O2 | 双库 | ✗ | ✗ | 默认 |

## 构建系统覆盖测试

这些配置覆盖了以下构建场景：

### 架构覆盖
- ✓ x86_64 (64-bit Intel/AMD)
- ✓ ARM32 (ARMv7-A)
- ✓ ARM64 (ARMv8-A / AArch64)
- ✓ RISC-V 64-bit

### 操作系统覆盖
- ✓ Linux (POSIX)
- ✓ RTOS (FreeRTOS, RT-Thread)
- ✗ Windows (可按需添加)
- ✗ macOS (可按需添加)
- ✗ Bare Metal (可按需添加)

### 优化级别覆盖
- ✓ -O0 (无优化，调试)
- ✓ -O2 (平衡优化)
- ✓ -O3 (最大性能)
- ✓ -Os (最小大小)

### 库类型覆盖
- ✓ 仅静态库
- ✓ 仅动态库
- ✓ 静态 + 动态双库

### 功能覆盖
- ✓ 最小化配置 (minimal)
- ✓ 标准配置 (default)
- ✓ 完整配置 (full)
- ✓ 调试配置 (debug)
- ✓ 发布配置 (release)
- ✓ 测试配置 (test/ci)

### 平台覆盖
- ✓ 通用 Linux (generic-linux)
- ✓ TI AM625 (ti-am625)
- ✓ BeagleBone (beaglebone)
- ✓ Raspberry Pi (raspberry-pi)
- ✓ 自定义平台 (custom)

### HAL 驱动覆盖
- ✓ 所有驱动开启 (full/debug/test)
- ✓ 部分驱动 (release/default)
- ✓ 最小驱动 (minimal)
- ✓ 高级特性 (DMA, 中断, 去抖, 回环)

### OSAL 功能覆盖
- ✓ 完整 POSIX (文件/网络/IPC/线程/时间/信号)
- ✓ RTOS 子集 (IPC/线程/时间)
- ✓ 不同资源限制 (16-256 任务)

## 最佳实践

### 开发阶段
1. 本地开发: `x86_64_full_defconfig`
2. 目标调试: `ccm_h200_am625_debug_defconfig`
3. 快速验证: `defconfig`

### 测试阶段
1. 单元测试: `x86_64_ci_defconfig`
2. 集成测试: `ccm_h200_am625_test_defconfig`
3. 压力测试: `ccm_h200_am625_test_defconfig`

### 生产阶段
1. ARM64 生产: `ccm_h200_am625_release_defconfig`
2. x86_64 生产: `x86_64_release_defconfig`
3. 资源受限: `x86_64_minimal_defconfig` 或 `arm32_rtos_minimal_defconfig`

### 原型开发
1. 树莓派: `arm64_raspberrypi_defconfig`
2. BeagleBone: `arm32_beaglebone_defconfig`
3. RISC-V: `riscv64_linux_defconfig`

## 维护说明

### 添加新配置
1. 基于现有配置创建
2. 使用 `make menuconfig` 调整
3. 使用 `make savedefconfig` 保存
4. 添加到本文档

### 更新配置
```bash
# 加载旧配置
make old_defconfig

# 更新选项
make menuconfig

# 保存最小化配置
make savedefconfig
mv defconfig configs/old_defconfig
```

### 配置命名规范
- 格式: `<arch>_<platform>_<variant>_defconfig`
- 示例: `arm64_raspberrypi_defconfig`
- 变体: debug, release, test, minimal, full, ci

---

**最后更新**: 2026-05-26
**维护者**: wanguo
