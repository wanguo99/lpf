# Staging 目录配置

## 概述

Staging 目录用于存放编译产物和公共头文件，是构建系统的中间输出目录。

## 目录结构

```
.staging/
├── bin/        # 可执行文件
├── include/    # 公共头文件
└── lib/        # 库文件（.a 和 .so）
    └── modules/  # 内核模块（.ko）
```

## 配置方式

Staging 目录支持三种配置方式，优先级从高到低：

### 1. 命令行参数（最高优先级）

```bash
make STAGING_DIR=/path/to/staging
```

### 2. 环境变量

```bash
export STAGING_DIR=/path/to/staging
make
```

### 3. 默认值（最低优先级）

如果未指定，默认使用 `$(objtree)/.staging`（隐藏目录）

```bash
# 默认情况
make                    # 使用 ./.staging

# 使用 O= 选项
make O=/tmp/build       # 使用 /tmp/build/.staging
```

## Buildroot 集成

在 Buildroot 环境中集成 EMS 项目：

### 方法 1：使用 Buildroot 的 staging 目录

```bash
export STAGING_DIR=$(BUILDROOT_OUTPUT)/staging
make -C /path/to/ems O=$(BUILDROOT_OUTPUT)/build/ems
```

### 方法 2：在 Buildroot package 中配置

```makefile
# package/ems/ems.mk
EMS_MAKE_ENV = STAGING_DIR=$(STAGING_DIR)

define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D) \
        ARCH=$(KERNEL_ARCH) \
        CROSS_COMPILE=$(TARGET_CROSS)
endef
```

## 使用场景

### 场景 1：本地开发（默认）

```bash
make x86_64_full_defconfig
make -j$(nproc)
# 产物在 ./.staging/
```

### 场景 2：独立构建目录

```bash
mkdir -p /tmp/ems-build
make O=/tmp/ems-build x86_64_full_defconfig
make O=/tmp/ems-build -j$(nproc)
# 产物在 /tmp/ems-build/.staging/
```

### 场景 3：自定义 staging 位置

```bash
make STAGING_DIR=/opt/ems/staging x86_64_full_defconfig
make STAGING_DIR=/opt/ems/staging -j$(nproc)
# 产物在 /opt/ems/staging/
```

### 场景 4：CI/CD 环境

```bash
# 使用环境变量统一配置
export STAGING_DIR=${CI_PROJECT_DIR}/artifacts
export O=${CI_PROJECT_DIR}/build

make O=$O x86_64_full_defconfig
make O=$O -j$(nproc)
# 产物在 ${CI_PROJECT_DIR}/artifacts/
```

## 注意事项

1. **路径一致性**：在同一次构建中，所有 make 命令必须使用相同的 STAGING_DIR 配置
2. **清理操作**：`make clean` 会删除 staging 目录，如果使用自定义路径请注意备份
3. **权限要求**：确保对 staging 目录有读写权限
4. **绝对路径**：建议使用绝对路径，避免相对路径导致的问题

## 常见问题

### Q: 为什么默认使用隐藏目录 .staging？

A: 隐藏目录可以避免污染项目根目录，同时在文件浏览器中不会显示，保持工作区整洁。

### Q: 可以在多个项目间共享 staging 目录吗？

A: 不建议。每个项目应该使用独立的 staging 目录，避免头文件和库文件冲突。

### Q: Buildroot 集成时如何避免重复安装？

A: 使用 Buildroot 的 staging 目录，EMS 的产物会直接安装到 Buildroot 的 staging，避免二次拷贝。

### Q: 如何查看当前使用的 staging 目录？

```bash
make -p | grep "^STAGING_DIR"
```
