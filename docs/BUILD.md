# EMS 构建指南

## 快速开始

```bash
# 1. 配置
make menuconfig                      # 图形化配置
# 或使用预定义配置
make x86_64_full_defconfig          # x86_64 完整配置
make arm64_full_defconfig           # ARM64 完整配置

# 2. 编译
make -j$(nproc)                     # 并行编译

# 3. 清理
make clean                          # 清理编译产物
make mrproper                       # 深度清理（包括配置）
```

## 配置系统

EMS 使用 Kconfig 配置系统（类似 Linux 内核）。

### 配置命令

```bash
make menuconfig          # 图形化配置界面
make defconfig           # 加载默认配置
make savedefconfig       # 保存最小化配置
```

### 预定义配置

```bash
# x86_64 平台
make x86_64_full_defconfig       # 完整功能
make x86_64_minimal_defconfig    # 最小功能
make x86_64_test_defconfig       # 测试配置

# ARM64 平台
make arm64_full_defconfig        # 完整功能
make arm64_minimal_defconfig     # 最小功能
make arm64_test_defconfig        # 测试配置
```

## 构建选项

### 并行编译

```bash
make -j$(nproc)          # 使用所有 CPU 核心
make -j8                 # 使用 8 个核心
```

### 详细输出

```bash
make V=1                 # 显示完整编译命令
```

### 交叉编译

```bash
# 配置目标架构
make menuconfig
# Target Platform -> Target architecture -> ARM64

# 设置交叉编译工具链
make menuconfig
# Target Platform -> Cross-compiler prefix -> aarch64-linux-gnu-

# 编译
make -j$(nproc)
```

## 输出目录

```
.staging/
├── bin/                 # 可执行文件
├── lib/                 # 库文件
│   ├── *.so            # 动态库
│   ├── *.a             # 静态库
│   └── *.so.1          # 符号链接
└── include/            # 头文件（staging）
```

## 运行测试

```bash
# 设置库路径
export LD_LIBRARY_PATH=$(pwd)/.staging/lib

# 运行测试
./.staging/bin/ems-unit-test
./.staging/bin/ems-perf-test
./.staging/bin/ems-stress-test
./.staging/bin/ems-system-test
```

## 常见问题

### 找不到动态库

```bash
# 错误：error while loading shared libraries: libosal.so.1
# 解决：设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$(pwd)/.staging/lib:$LD_LIBRARY_PATH
```

### 配置不生效

```bash
# 清理后重新配置
make mrproper
make <your>_defconfig
make -j$(nproc)
```

### 编译错误

```bash
# 查看详细错误信息
make V=1

# 清理后重新编译
make clean
make -j$(nproc)
```
