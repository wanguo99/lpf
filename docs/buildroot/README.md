# EMS Buildroot 集成指南

本目录包含将 EMS 集成到 Buildroot 构建系统所需的配置文件。

## 文件说明

### 1. ems.mk
Buildroot package makefile，定义了如何下载、配置、编译和安装 EMS。

**主要配置项：**
- `EMS_VERSION`: 版本号或 Git 提交哈希
- `EMS_SITE`: 源码仓库地址
- `EMS_SITE_METHOD`: 下载方式（git/wget/local）
- `EMS_DEPENDENCIES`: 依赖的其他 Buildroot packages
- `EMS_CONF_OPTS`: CMake 配置选项

**Buildroot 自动传递的 CMake 参数：**
- `CMAKE_INSTALL_PREFIX`: 安装前缀（通常为 /usr）
- `CMAKE_TOOLCHAIN_FILE`: 交叉编译工具链文件
- `CMAKE_BUILD_TYPE`: 构建类型（Release/Debug）
- `CMAKE_INSTALL_LIBDIR`: 库目录（lib 或 lib64）
- `CMAKE_INSTALL_BINDIR`: 可执行文件目录（bin）
- `CMAKE_INSTALL_INCLUDEDIR`: 头文件目录（include）

### 2. Config.in
Buildroot menuconfig 配置文件，定义了 EMS 在配置菜单中的选项。

**可配置选项：**
- `BR2_PACKAGE_EMS`: 启用 EMS package
- `BR2_PACKAGE_EMS_SAMPLE_APP`: 安装示例应用程序
- `BR2_PACKAGE_EMS_WATCHDOG_APP`: 安装看门狗应用程序
- `BR2_PACKAGE_EMS_TESTS`: 安装单元测试

**依赖要求：**
- 线程支持（POSIX threads）
- 动态库支持（不支持纯静态链接）

### 3. ems.conf
运行时配置文件，将被安装到目标系统的 `/etc/ems.conf`。

**配置项：**
- 日志级别和输出方式
- CAN 总线参数
- 串口参数
- I2C/SPI 总线配置
- 看门狗配置

### 4. S90ems
SysV init 启动脚本，将被安装到 `/etc/init.d/S90ems`。

**功能：**
- 系统启动时自动启动 EMS 示例应用
- 支持 start/stop/restart 命令
- 使用 start-stop-daemon 管理进程

## 集成步骤

### 方法一：外部 Package（推荐用于开发）

1. **将配置文件复制到 Buildroot 外部 package 目录：**
   ```bash
   mkdir -p /path/to/buildroot-external/package/ems
   cp ems.mk /path/to/buildroot-external/package/ems/
   cp Config.in /path/to/buildroot-external/package/ems/
   ```

2. **在外部 package 的 Config.in 中添加：**
   ```
   source "$BR2_EXTERNAL_<NAME>_PATH/package/ems/Config.in"
   ```

3. **配置 Buildroot：**
   ```bash
   cd /path/to/buildroot
   make BR2_EXTERNAL=/path/to/buildroot-external menuconfig
   ```

4. **在 menuconfig 中启用 EMS：**
   ```
   Target packages
     └─> Miscellaneous
         └─> [*] ems
             [*]   install sample application
             [ ]   install unit tests
   ```

5. **编译：**
   ```bash
   make
   ```

### 方法二：集成到 Buildroot 主树

1. **复制文件到 Buildroot package 目录：**
   ```bash
   mkdir -p /path/to/buildroot/package/ems
   cp ems.mk /path/to/buildroot/package/ems/
   cp Config.in /path/to/buildroot/package/ems/
   ```

2. **编辑 `package/Config.in`，添加：**
   ```
   menu "Miscellaneous"
       source "package/ems/Config.in"
       ...
   endmenu
   ```

3. **后续步骤同方法一的步骤 3-5**

### 方法三：使用本地源码（开发调试）

如果需要使用本地源码而不是从 Git 仓库下载：

1. **修改 `ems.mk` 中的源码位置：**
   ```makefile
   EMS_SITE = /path/to/ems
   EMS_SITE_METHOD = local
   EMS_VERSION = local
   ```

2. **或者在 Buildroot 配置中设置 override：**
   ```bash
   echo 'EMS_OVERRIDE_SRCDIR = /path/to/ems' >> local.mk
   ```

## 目标系统文件布局

编译完成后，EMS 将在目标系统中安装以下文件：

```
/usr/lib/
├── libosal.so          # OSAL 库
├── libhal.so           # HAL 库
├── libpcl.so           # PCL 库
└── libpdl.so           # PDL 库

/usr/include/ems/   # 头文件（如果启用开发文件）
├── osal/
├── hal/
├── pcl/
└── pdl/

/usr/bin/
├── ems-sample-app      # 示例应用（可选）
└── ems-test            # 单元测试（可选）

/etc/
├── ems.conf        # 配置文件
└── init.d/
    └── S90ems      # 启动脚本
```

## 交叉编译配置

EMS 支持多种架构的交叉编译，Buildroot 会自动传递正确的工具链配置：

- **ARM32**: `arm-linux-gnueabihf-gcc`
- **ARM64**: `aarch64-linux-gnu-gcc`
- **RISC-V 64**: `riscv64-linux-gnu-gcc`
- **x86_64**: `x86_64-linux-gnu-gcc`

CMake 会自动检测目标架构并应用相应的编译优化选项。

## 依赖关系

EMS 在 Buildroot 中的依赖：

- **必需依赖：**
  - C++ 工具链（`BR2_INSTALL_LIBSTDCPP`）
  - 线程支持（`BR2_TOOLCHAIN_HAS_THREADS`）
  - 动态库支持（`!BR2_STATIC_LIBS`）

- **可选依赖：**
  - 无（EMS 是自包含的）

## 常见问题

### 1. 编译失败：找不到 pthread
**原因：** 工具链未启用线程支持  
**解决：** 在 menuconfig 中启用 `Toolchain -> Enable NPTL support`

### 2. 运行时错误：找不到共享库
**原因：** 动态库路径未配置  
**解决：** 确保 `/usr/lib` 在 `LD_LIBRARY_PATH` 中，或运行 `ldconfig`

### 3. 启动脚本不执行
**原因：** 脚本权限不正确  
**解决：** 检查 `S90ems` 是否有执行权限（应为 755）

### 4. 交叉编译架构不匹配
**原因：** CMake 未正确检测目标架构  
**解决：** 检查 Buildroot 的 `BR2_ARCH` 配置，确保与目标平台一致

## 自定义配置

### 修改 CMake 配置选项

编辑 `ems.mk` 中的 `EMS_CONF_OPTS`：

```makefile
EMS_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=ON \
	-DBUILD_TESTING=OFF \
	-DCUSTOM_OPTION=value
```

### 添加额外的安装文件

在 `ems.mk` 中添加 `define EMS_INSTALL_TARGET_CMDS`：

```makefile
define EMS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/output/target/bin/sample_app $(TARGET_DIR)/usr/bin/sample_app
	$(INSTALL) -D -m 0755 $(@D)/output/target/bin/ems-test $(TARGET_DIR)/usr/bin/ems-test
	$(INSTALL) -D -m 0644 $(EMS_PKGDIR)/ems.conf $(TARGET_DIR)/etc/ems.conf
	$(INSTALL) -D -m 0755 $(EMS_PKGDIR)/S90ems $(TARGET_DIR)/etc/init.d/S90ems
	# 添加自定义安装命令
	$(INSTALL) -D -m 0644 $(@D)/custom-file $(TARGET_DIR)/etc/custom-file
endef
```

## 版本管理

### 使用特定版本

修改 `ems.mk`：

```makefile
EMS_VERSION = v1.0.0
EMS_SITE = https://github.com/wanguo99/ems.git
EMS_SITE_METHOD = git
```

### 使用特定 Git 提交

```makefile
EMS_VERSION = abc123def456
```

### 使用 tarball

```makefile
EMS_VERSION = 1.0.0
EMS_SITE = https://github.com/wanguo99/ems/releases/download/v$(EMS_VERSION)
EMS_SOURCE = ems-$(EMS_VERSION).tar.gz
EMS_SITE_METHOD = wget
```

## 技术支持

如有问题，请参考：
- EMS 主文档：`../README.md`
- Buildroot 官方文档：https://buildroot.org/downloads/manual/manual.html
- 交叉编译指南：`../CROSS_COMPILE_GUIDE.md`
