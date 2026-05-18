# EMS Buildroot 集成指南

本目录包含将 EMS 框架（包括 PMC 应用）集成到 Buildroot 构建系统所需的配置文件。

## 目录结构

```
docs/buildroot/
├── packages/                    # Buildroot package 定义
│   └── ems/                     # EMS 框架 package（包含 PMC）
│       ├── Config.in            # menuconfig 配置
│       ├── ems.mk               # package makefile
│       └── ems.conf             # 运行时配置文件
├── rootfs_overlay/              # 目标系统文件覆盖
│   ├── etc/
│   │   ├── init.d/
│   │   │   ├── S90ems           # EMS 启动脚本（SysVinit）
│   │   │   └── S95pmc           # PMC 启动脚本（SysVinit）
│   │   └── systemd/system/
│   │       └── pmc.service      # PMC systemd 服务
│   └── usr/local/bin/
│       └── pmc_control.sh       # PMC 控制脚本
└── README.md                    # 本文档
```

## Package 说明

### EMS Package (`packages/ems/`)

EMS 是通用嵌入式中间件框架，提供 OSAL/HAL/PCL/PDL 抽象层。PMC (Payload Management Controller) 是基于 EMS 框架实现的载荷管理控制器应用，采用 5 进程架构。

**注意：** EMS 和 PMC 在同一个代码仓库中，因此只有一个 Buildroot package。

#### ems.mk
Buildroot package makefile，定义了如何下载、配置、编译和安装 EMS 框架及 PMC 应用。

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

#### Config.in
Buildroot menuconfig 配置文件，定义了 EMS 在配置菜单中的选项。

**可配置选项：**
- `BR2_PACKAGE_EMS`: 启用 EMS package
- `BR2_PACKAGE_EMS_SAMPLE_APP`: 安装示例应用程序
- `BR2_PACKAGE_EMS_PMC`: 安装 PMC 应用（5 进程架构）
- `BR2_PACKAGE_EMS_TESTS`: 安装单元测试

**依赖要求：**
- 线程支持（POSIX threads）
- 动态库支持（不支持纯静态链接）

#### ems.conf
运行时配置文件，将被安装到目标系统的 `/etc/ems.conf`。

**配置项：**
- 日志级别和输出方式
- CAN 总线参数
- 串口参数
- I2C/SPI 总线配置
- 看门狗配置

## PMC 应用说明

PMC (Payload Management Controller) 是基于 EMS 框架实现的载荷管理控制器，采用 5 进程架构：

- `pmc_comm` - 通信进程：处理遥控遥测通信
- `pmc_collector` - 数据采集进程：周期性采集设备状态
- `pmc_health` - 健康监测进程：监控各进程心跳和设备健康
- `pmc_supervisor` - 监督进程：管理其他进程的生命周期
- `pmc_logger` - 日志进程：统一日志收集和管理

**安装内容：**
- 5 个进程可执行文件
- 控制脚本：`pmc_control.sh`
- 日志目录：`/var/log/pmc`
- PID 目录：`/var/run/pmc`

## Rootfs Overlay 说明

### 启动脚本

#### S90ems (SysVinit)
EMS 示例应用的启动脚本，将被安装到 `/etc/init.d/S90ems`。

**功能：**
- 系统启动时自动启动 EMS 示例应用
- 支持 start/stop/restart 命令
- 使用 start-stop-daemon 管理进程

#### S95pmc (SysVinit)
PMC 系统的启动脚本，将被安装到 `/etc/init.d/S95pmc`。

**功能：**
- 调用 `pmc_control.sh` 管理 PMC 进程
- 支持 start/stop/restart/status 命令
- 在 S90ems 之后启动（序号 95 > 90）

### Systemd 服务

#### pmc.service
PMC 的 systemd 服务配置，将被安装到 `/usr/lib/systemd/system/pmc.service`。

**功能：**
- 支持 systemctl 管理
- 自动重启策略
- 日志集成到 journald

### 控制脚本

#### pmc_control.sh
PMC 系统的控制脚本，将被安装到 `/usr/local/bin/pmc_control.sh`。

**功能：**
- 按正确顺序启动/停止 5 个进程
- 进程状态检查
- PID 文件管理
- 彩色状态输出

## 集成步骤

### 方法一：外部 Package（推荐用于开发）

1. **创建 Buildroot 外部目录结构：**
   ```bash
   mkdir -p /path/to/buildroot-external/package
   ```

2. **复制 package 配置：**
   ```bash
   cp -r packages/ems /path/to/buildroot-external/package/
   ```

3. **在外部 package 的 Config.in 中添加：**
   ```
   source "$BR2_EXTERNAL_EMS_PATH/package/ems/Config.in"
   ```

4. **复制 rootfs overlay：**
   ```bash
   cp -r rootfs_overlay /path/to/buildroot-external/
   ```

5. **配置 Buildroot：**
   ```bash
   cd /path/to/buildroot
   make BR2_EXTERNAL=/path/to/buildroot-external menuconfig
   ```

6. **在 menuconfig 中启用 EMS 和 PMC：**
   ```
   Target packages
     └─> Miscellaneous
         └─> [*] ems
             [*]   install sample application
             [*]   install PMC application
             [ ]   install unit tests
   ```

7. **配置 rootfs overlay：**
   在 menuconfig 中设置：
   ```
   System configuration
     └─> Root filesystem overlay directories
         $(BR2_EXTERNAL_EMS_PATH)/rootfs_overlay
   ```

8. **编译：**
   ```bash
   make
   ```

### 方法二：集成到 Buildroot 主树

1. **复制文件到 Buildroot package 目录：**
   ```bash
   mkdir -p /path/to/buildroot/package/ems
   cp packages/ems/* /path/to/buildroot/package/ems/
   ```

2. **编辑 `package/Config.in`，添加：**
   ```
   menu "Miscellaneous"
       source "package/ems/Config.in"
       ...
   endmenu
   ```

3. **复制 rootfs overlay 到项目目录**

4. **后续步骤同方法一的步骤 5-8**

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

编译完成后，EMS 和 PMC 将在目标系统中安装以下文件：

```
/usr/lib/
├── libosal.so          # OSAL 库
├── libhal.so           # HAL 库
├── libpcl.so           # PCL 库
├── libpdl.so           # PDL 库
└── libpmc.a            # PMC 公共库（静态）

/usr/include/ems/       # 头文件（如果启用开发文件）
├── osal/
├── hal/
├── pcl/
└── pdl/

/usr/bin/
├── sample_app          # EMS 示例应用（可选）
├── pmc_comm            # PMC 通信进程
├── pmc_collector       # PMC 数据采集进程
├── pmc_health          # PMC 健康监测进程
├── pmc_supervisor      # PMC 监督进程
└── pmc_logger          # PMC 日志进程

/usr/local/bin/
└── pmc_control.sh      # PMC 控制脚本

/etc/
├── ems.conf            # EMS 配置文件
└── init.d/
    ├── S90ems          # EMS 启动脚本
    └── S95pmc          # PMC 启动脚本

/usr/lib/systemd/system/
└── pmc.service         # PMC systemd 服务

/var/log/pmc/           # PMC 日志目录
/var/run/pmc/           # PMC PID 目录
```

## 运行时管理

### SysVinit 系统

```bash
# 启动 PMC
/etc/init.d/S95pmc start

# 停止 PMC
/etc/init.d/S95pmc stop

# 重启 PMC
/etc/init.d/S95pmc restart

# 查看状态
/etc/init.d/S95pmc status
```

### Systemd 系统

```bash
# 启动 PMC
systemctl start pmc

# 停止 PMC
systemctl stop pmc

# 重启 PMC
systemctl restart pmc

# 查看状态
systemctl status pmc

# 开机自启
systemctl enable pmc

# 查看日志
journalctl -u pmc -f
```

### 使用控制脚本

```bash
# 启动所有进程
pmc_control.sh start

# 停止所有进程
pmc_control.sh stop

# 重启所有进程
pmc_control.sh restart

# 查看状态
pmc_control.sh status
```

## 交叉编译配置

EMS 和 PMC 支持多种架构的交叉编译，Buildroot 会自动传递正确的工具链配置：

- **ARM32**: `arm-linux-gnueabihf-gcc`
- **ARM64**: `aarch64-linux-gnu-gcc`
- **RISC-V 64**: `riscv64-linux-gnu-gcc`
- **x86_64**: `x86_64-linux-gnu-gcc`

CMake 会自动检测目标架构并应用相应的编译优化选项。

## 依赖关系

### EMS Package
- **必需依赖：**
  - C++ 工具链（`BR2_INSTALL_LIBSTDCPP`）
  - 线程支持（`BR2_TOOLCHAIN_HAS_THREADS`）
  - 动态库支持（`!BR2_STATIC_LIBS`）

### PMC 应用
- **必需依赖：**
  - EMS 框架（同一 package）

## 常见问题

### 1. 编译失败：找不到 pthread
**原因：** 工具链未启用线程支持  
**解决：** 在 menuconfig 中启用 `Toolchain -> Enable NPTL support`

### 2. 运行时错误：找不到共享库
**原因：** 动态库路径未配置  
**解决：** 确保 `/usr/lib` 在 `LD_LIBRARY_PATH` 中，或运行 `ldconfig`

### 3. PMC 启动失败：找不到 libpmc
**原因：** libpmc 是静态库，需要在编译时链接  
**解决：** 确保 PMC 的 CMakeLists.txt 正确链接了 libpmc

### 4. 启动脚本不执行
**原因：** 脚本权限不正确  
**解决：** 检查启动脚本是否有执行权限（应为 755）

### 5. PMC 进程无法创建日志目录
**原因：** 权限不足  
**解决：** 确保 `/var/log/pmc` 和 `/var/run/pmc` 目录存在且可写

### 6. 交叉编译架构不匹配
**原因：** CMake 未正确检测目标架构  
**解决：** 检查 Buildroot 的 `BR2_ARCH` 配置，确保与目标平台一致

## 自定义配置

### 修改 CMake 配置选项

编辑 `ems.mk` 中的 `CONF_OPTS`：

```makefile
EMS_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=ON \
	-DBUILD_TESTING=OFF \
	-DBUILD_PMC_APP=ON \
	-DCUSTOM_OPTION=value
```

### 添加额外的安装文件

可以在 makefile 中添加 post-install hooks：

```makefile
# 安装额外的配置文件
define EMS_INSTALL_EXTRA_CONFIG
	$(INSTALL) -D -m 0644 $(EMS_PKGDIR)/custom.conf $(TARGET_DIR)/etc/pmc/custom.conf
	$(INSTALL) -D -m 0755 $(EMS_PKGDIR)/custom-script.sh $(TARGET_DIR)/usr/bin/custom-script.sh
endef
EMS_POST_INSTALL_TARGET_HOOKS += EMS_INSTALL_EXTRA_CONFIG
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
- EMS 主文档：`../../README.md`
- PMC 实现总结：`../architecture/PMC_Implementation_Summary.md`
- Buildroot 官方文档：https://buildroot.org/downloads/manual/manual.html
- 交叉编译指南：`../CROSS_COMPILE_GUIDE.md`
