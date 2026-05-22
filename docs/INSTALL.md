# EMS 安装指南

**日期**: 2026-05-22  
**版本**: 1.0.0

## 概述

EMS 构建系统提供了类似 Linux 内核的安装机制，支持将构建产物从 staging 目录安装到目标系统。

## 安装目标

### 完整安装

```bash
# 安装所有文件到默认位置 (/usr/local)
make install

# 安装到指定前缀
make install prefix=/usr

# 安装到临时目录（用于打包）
make install DESTDIR=/tmp/ems-install

# 组合使用
make install DESTDIR=/tmp/ems-install prefix=/opt/ems
```

### 部分安装

```bash
# 只安装可执行文件
make install_bin

# 只安装库文件
make install_lib

# 只安装头文件
make install_headers

# 只安装内核模块
make install_modules
```

## 安装变量

### 标准变量（遵循 GNU Coding Standards）

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `DESTDIR` | 空 | 安装根目录（用于打包） |
| `prefix` | `/usr/local` | 安装前缀 |
| `exec_prefix` | `$(prefix)` | 可执行文件前缀 |
| `bindir` | `$(exec_prefix)/bin` | 可执行文件目录 |
| `libdir` | `$(exec_prefix)/lib` | 库文件目录 |
| `includedir` | `$(prefix)/include` | 头文件目录 |
| `modulesdir` | `$(libdir)/modules` | 内核模块目录 |

### DESTDIR 的作用

`DESTDIR` 用于在打包时将文件安装到临时目录，而不是直接安装到系统目录。

**示例**：

```bash
# 不使用 DESTDIR（直接安装到系统）
make install prefix=/usr
# 结果：文件安装到 /usr/bin、/usr/lib、/usr/include

# 使用 DESTDIR（安装到临时目录）
make install DESTDIR=/tmp/ems-install prefix=/usr
# 结果：文件安装到 /tmp/ems-install/usr/bin、/tmp/ems-install/usr/lib、/tmp/ems-install/usr/include
```

## 使用场景

### 场景 1: 开发环境安装

在开发环境中，通常安装到 `/usr/local`：

```bash
# 构建
make menuconfig
make -j$(nproc)

# 安装（需要 root 权限）
sudo make install

# 或安装到用户目录
make install prefix=$HOME/.local
```

### 场景 2: 生产环境安装

在生产环境中，通常安装到 `/usr` 或 `/opt`：

```bash
# 安装到 /usr
sudo make install prefix=/usr

# 安装到 /opt/ems
sudo make install prefix=/opt/ems
```

### 场景 3: 打包（deb/rpm）

使用 DESTDIR 将文件安装到临时目录，然后打包：

```bash
# 安装到临时目录
make install DESTDIR=/tmp/ems-install prefix=/usr

# 创建 deb 包（使用 fpm）
fpm -s dir -t deb -n ems -v 1.0.0 \
    -C /tmp/ems-install \
    --prefix / \
    .

# 创建 rpm 包（使用 fpm）
fpm -s dir -t rpm -n ems -v 1.0.0 \
    -C /tmp/ems-install \
    --prefix / \
    .

# 创建 tar.gz 包
tar czf ems-1.0.0.tar.gz -C /tmp/ems-install .
```

### 场景 4: 交叉编译安装

交叉编译后安装到目标板的 rootfs：

```bash
# 交叉编译
make CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 安装到目标 rootfs
make install DESTDIR=/path/to/rootfs prefix=/usr
```

### 场景 5: Buildroot/Yocto 集成

在 Buildroot 或 Yocto 中使用 DESTDIR：

```makefile
# Buildroot package
define EMS_INSTALL_STAGING_CMDS
    $(MAKE) -C $(@D) install_headers install_lib \
        DESTDIR=$(STAGING_DIR) prefix=/usr
endef

define EMS_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install_bin install_lib \
        DESTDIR=$(TARGET_DIR) prefix=/usr
endef
```

## 安装示例

### 示例 1: 默认安装

```bash
$ make install
  INSTALL binaries to /usr/local/bin
  INSTALL ccm_collector
  INSTALL ccm_logger
  INSTALL ccm_health
  INSTALL ccm_supervisor
  INSTALL ccm_comm
  INSTALL libraries to /usr/local/lib
  INSTALL libosal.a
  INSTALL libosal.so
  INSTALL libhal.a
  INSTALL libhal.so
  INSTALL libccm.a
  INSTALL libccm.so
  INSTALL headers to /usr/local/include
  INSTALL ./osal.h
  INSTALL ./hal_can.h
  INSTALL ./ipc/osal_mutex.h
  INSTALL modules to /usr/local/lib/modules
  SKIP    no modules found
  INSTALL complete
  Installed to: /usr/local
```

### 示例 2: 自定义路径

```bash
$ make install prefix=/opt/ems
  INSTALL binaries to /opt/ems/bin
  INSTALL ccm_collector
  ...
  INSTALL complete
  Installed to: /opt/ems
```

### 示例 3: 打包安装

```bash
$ make install DESTDIR=/tmp/ems-install prefix=/usr
  INSTALL binaries to /tmp/ems-install/usr/bin
  INSTALL ccm_collector
  ...
  INSTALL complete
  Installed to: /tmp/ems-install/usr

$ tree /tmp/ems-install/
/tmp/ems-install/
└── usr
    ├── bin
    │   ├── ccm_collector
    │   ├── ccm_logger
    │   ├── ccm_health
    │   ├── ccm_supervisor
    │   └── ccm_comm
    ├── include
    │   ├── osal.h
    │   ├── hal_can.h
    │   └── ipc
    │       └── osal_mutex.h
    └── lib
        ├── libosal.a
        ├── libosal.so
        ├── libhal.a
        ├── libhal.so
        ├── libccm.a
        └── libccm.so
```

### 示例 4: 只安装库和头文件

```bash
$ make install_lib install_headers DESTDIR=/tmp/sdk prefix=/usr
  INSTALL libraries to /tmp/sdk/usr/lib
  INSTALL libosal.a
  INSTALL libhal.a
  INSTALL headers to /tmp/sdk/usr/include
  INSTALL ./osal.h
  INSTALL ./hal_can.h
```

## 文件权限

安装时会自动设置正确的文件权限：

- **可执行文件**: `0755` (rwxr-xr-x)
- **动态库**: `0755` (rwxr-xr-x)
- **静态库**: `0644` (rw-r--r--)
- **头文件**: `0644` (rw-r--r--)
- **内核模块**: `0644` (rw-r--r--)

## 安装后处理

### 更新动态链接器缓存

如果直接安装到系统目录（不使用 DESTDIR），安装脚本会尝试运行 `ldconfig` 更新动态链接器缓存：

```bash
# 直接安装（会自动运行 ldconfig）
sudo make install prefix=/usr

# 使用 DESTDIR（不会运行 ldconfig）
make install DESTDIR=/tmp/ems-install prefix=/usr
```

如果需要手动更新缓存：

```bash
sudo ldconfig /usr/local/lib
```

### 设置环境变量

如果安装到非标准路径，需要设置环境变量：

```bash
# 安装到 /opt/ems
make install prefix=/opt/ems

# 设置环境变量
export PATH=/opt/ems/bin:$PATH
export LD_LIBRARY_PATH=/opt/ems/lib:$LD_LIBRARY_PATH
export C_INCLUDE_PATH=/opt/ems/include:$C_INCLUDE_PATH
```

可以将这些变量添加到 `~/.bashrc` 或 `/etc/profile.d/ems.sh`：

```bash
# /etc/profile.d/ems.sh
export PATH=/opt/ems/bin:$PATH
export LD_LIBRARY_PATH=/opt/ems/lib:$LD_LIBRARY_PATH
export C_INCLUDE_PATH=/opt/ems/include:$C_INCLUDE_PATH
```

## 卸载

目前不支持 `make uninstall`。如果需要卸载，可以手动删除安装的文件：

```bash
# 删除可执行文件
sudo rm -f /usr/local/bin/ccm_*

# 删除库文件
sudo rm -f /usr/local/lib/libosal.* /usr/local/lib/libhal.* /usr/local/lib/libccm.*

# 删除头文件
sudo rm -rf /usr/local/include/osal.h /usr/local/include/hal_can.h /usr/local/include/ipc/

# 更新动态链接器缓存
sudo ldconfig
```

或者使用打包系统的卸载功能：

```bash
# deb 包
sudo dpkg -r ems

# rpm 包
sudo rpm -e ems
```

## 与其他构建系统的对比

### Linux 内核

```bash
# Linux 内核
make modules_install INSTALL_MOD_PATH=/tmp/rootfs
make headers_install INSTALL_HDR_PATH=/tmp/rootfs/usr

# EMS
make install_modules DESTDIR=/tmp/rootfs prefix=/usr
make install_headers DESTDIR=/tmp/rootfs prefix=/usr
```

### Autotools

```bash
# Autotools
./configure --prefix=/usr
make
make install DESTDIR=/tmp/install

# EMS
make menuconfig
make
make install DESTDIR=/tmp/install prefix=/usr
```

### CMake

```bash
# CMake
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
make install DESTDIR=/tmp/install

# EMS
make menuconfig
make
make install DESTDIR=/tmp/install prefix=/usr
```

## 常见问题

### Q: 为什么需要 DESTDIR？

A: DESTDIR 用于打包时将文件安装到临时目录，而不是直接安装到系统目录。这样可以：
1. 避免污染开发环境
2. 方便创建 deb/rpm 包
3. 支持交叉编译安装

### Q: prefix 和 DESTDIR 有什么区别？

A: 
- `prefix` 是最终安装路径（如 `/usr`、`/usr/local`、`/opt/ems`）
- `DESTDIR` 是临时根目录（如 `/tmp/ems-install`）
- 最终路径 = `$(DESTDIR)$(prefix)`

### Q: 如何只安装开发文件（头文件和静态库）？

A:
```bash
make install_headers install_lib DESTDIR=/tmp/sdk prefix=/usr
# 然后手动删除动态库
rm -f /tmp/sdk/usr/lib/*.so
```

### Q: 如何验证安装是否成功？

A:
```bash
# 检查可执行文件
which ccm_collector
ccm_collector --version

# 检查库文件
ldconfig -p | grep libosal

# 检查头文件
ls -l /usr/local/include/osal.h
```

### Q: 安装时需要 root 权限吗？

A: 取决于安装路径：
- 安装到 `/usr`、`/usr/local` 等系统目录需要 root 权限
- 安装到用户目录（如 `$HOME/.local`）不需要 root 权限
- 使用 DESTDIR 安装到临时目录不需要 root 权限

## 参考文档

- [BUILD_GUIDE.md](BUILD_GUIDE.md) - 构建指南
- [BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详解
- [STAGING_DIRECTORY.md](STAGING_DIRECTORY.md) - Staging 目录说明
- [CLAUDE.md](../CLAUDE.md) - AI 助手指南
- [GNU Coding Standards - Installation](https://www.gnu.org/prep/standards/html_node/DESTDIR.html)

---

**最后更新**: 2026-05-22  
**维护者**: wanguo  
**分支**: master
