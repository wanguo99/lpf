# Staging 目录说明

**日期**: 2026-05-22  
**版本**: 2.0.0

## 概述

EMS 构建系统使用统一的 **staging 目录**来管理所有构建产物，这是一个符合行业标准的做法，被 Buildroot、Yocto 等主流构建系统广泛采用。

**新特性（v2.0）：**
- 默认使用 `.staging` 隐藏目录，保持工作区整洁
- 支持通过环境变量 `STAGING_DIR` 自定义路径
- 无缝集成 Buildroot/Yocto 构建系统
- 三级配置优先级：命令行 > 环境变量 > 默认值

## 目录结构

```
EMS/
├── .staging/                   # 统一的构建产物输出目录（隐藏）
│   ├── bin/                   # 可执行文件
│   │   ├── ccm_collector
│   │   ├── ccm_logger
│   │   ├── ccm_health
│   │   ├── ccm_supervisor
│   │   └── ccm_comm
│   ├── lib/                   # 库文件
│   │   ├── libosal.a         # 静态库
│   │   ├── libosal.so        # 动态库
│   │   ├── libhal.a
│   │   ├── libhal.so
│   │   ├── libpcl.a
│   │   ├── libpcl.so
│   │   ├── libpdl.a
│   │   ├── libpdl.so
│   │   ├── libacl.a
│   │   ├── libacl.so
│   │   ├── libccm.a
│   │   ├── libccm.so
│   │   ├── libh200_am625.a
│   │   ├── libh200_am625.so
│   │   └── modules/          # 内核模块（预留）
│   └── include/              # 头文件（Staging）
│       ├── osal.h
│       ├── osal_types.h
│       ├── osal_platform.h
│       ├── hal_can.h
│       ├── pcl.h
│       ├── pdl_bmc.h
│       ├── acl_api.h
│       ├── ipc/              # IPC 相关头文件
│       │   ├── osal_mutex.h
│       │   └── osal_semaphore.h
│       ├── lib/              # 库相关头文件
│       │   ├── osal_errno.h
│       │   └── osal_heap.h
│       ├── net/              # 网络相关头文件
│       │   └── osal_socket.h
│       ├── sys/              # 系统相关头文件
│       │   ├── osal_thread.h
│       │   └── osal_time.h
│       ├── util/             # 工具相关头文件
│       │   └── osal_log.h
│       └── config/           # Kconfig 生成的配置头文件
│           └── ems_config.h
├── core/                      # 核心模块源码
├── products/                  # 产品模块源码
└── ...
```

## 配置方式

### 默认配置

默认使用 `.staging` 隐藏目录：

```bash
# 直接编译，使用默认 .staging
make -j$(nproc)

# 查看输出
ls -lh .staging/bin/
ls -lh .staging/lib/
ls -lh .staging/include/
```

### 环境变量配置

通过 `STAGING_DIR` 环境变量自定义路径：

```bash
# 使用自定义路径
export STAGING_DIR=/tmp/ems-staging
make -j$(nproc)

# 或者一次性指定
STAGING_DIR=/tmp/ems-staging make -j$(nproc)
```

### 命令行配置

命令行参数优先级最高：

```bash
# 直接在命令行指定
make STAGING_DIR=/custom/path -j$(nproc)
```

### 配置优先级

```
命令行参数 > 环境变量 > 默认值 (.staging)
```

示例：

```bash
# 1. 默认值
make              # 使用 .staging

# 2. 环境变量
export STAGING_DIR=/tmp/staging
make              # 使用 /tmp/staging

# 3. 命令行（最高优先级）
make STAGING_DIR=/opt/staging  # 使用 /opt/staging，忽略环境变量
```

## 优势

### 1. 目录结构更清晰

所有构建产物集中在 `.staging/` 隐藏目录下，源码目录保持干净：

```bash
# 构建产物（隐藏）
.staging/
├── bin/
├── lib/
└── include/

# 源码目录（可见）
core/
products/
scripts/
Makefile
Kconfig
```

### 2. 符合行业标准

- **Buildroot**: 使用 `$(HOST_DIR)` 和 `$(STAGING_DIR)`
- **Yocto**: 使用 `${STAGING_DIR}` 和 `${DEPLOY_DIR}`
- **Linux Kernel**: 使用 `INSTALL_MOD_PATH` 和 `INSTALL_HDR_PATH`

### 3. 灵活配置

支持三种配置方式，适应不同使用场景：

```bash
# 独立开发：使用默认 .staging
make

# 临时测试：使用环境变量
STAGING_DIR=/tmp/test make

# Buildroot 集成：使用命令行参数
make STAGING_DIR=$(STAGING_DIR)
```

### 4. 便于清理

只需删除 staging 目录即可清理所有构建产物：

```bash
# 清理所有构建产物
make clean

# 或手动清理
rm -rf .staging/
```

### 5. 便于打包

直接打包 staging 目录即可：

```bash
# 打包所有构建产物
tar czf ems-1.0.0.tar.gz .staging/

# 或创建 deb/rpm 包
fpm -s dir -t deb -n ems -v 1.0.0 .staging/=/usr/local/
```

### 6. 避免污染源码树

所有输出都在 .staging 下，源码目录不会被污染：

```bash
# 源码目录保持干净
$ ls
core/  products/  scripts/  Makefile  Kconfig  ...

# 构建产物在 .staging 下（隐藏）
$ ls -a
.  ..  .staging/  core/  products/  ...
```

### 7. 便于集成到其他构建系统

其他构建系统可以通过 `STAGING_DIR` 环境变量直接指定路径：

```makefile
# Buildroot package
define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) STAGING_DIR=$(STAGING_DIR) \
        $(MAKE) -C $(@D) -j$(PARALLEL_JOBS)
endef

# 无需额外的安装步骤，产物已在 Buildroot 的 staging 中
```

## 使用方式

### 编译

```bash
# 方式 1：使用默认 .staging
make menuconfig
make -j$(nproc)
ls -lh .staging/bin/

# 方式 2：使用环境变量
export STAGING_DIR=/tmp/ems-staging
make -j$(nproc)
ls -lh /tmp/ems-staging/bin/

# 方式 3：使用命令行参数
make STAGING_DIR=/opt/ems -j$(nproc)
ls -lh /opt/ems/bin/
```

### 清理

```bash
# 清理构建产物（保留配置）
make clean

# 深度清理（删除配置）
make mrproper

# 手动清理 staging 目录
rm -rf .staging/
```

### 安装

```bash
# 安装到系统目录（需要 root 权限）
sudo cp -r .staging/bin/* /usr/local/bin/
sudo cp -r .staging/lib/*.so /usr/local/lib/
sudo cp -r .staging/include/* /usr/local/include/
sudo ldconfig

# 或使用 DESTDIR
make install DESTDIR=/tmp/ems-install
```

### 打包

```bash
# 创建 tar.gz 包
tar czf ems-1.0.0-$(uname -m).tar.gz .staging/

# 创建 deb 包（需要 fpm）
fpm -s dir -t deb -n ems -v 1.0.0 \
    --prefix /usr/local \
    .staging/bin=/usr/local/bin \
    .staging/lib=/usr/local/lib \
    .staging/include=/usr/local/include

# 创建 rpm 包（需要 fpm）
fpm -s dir -t rpm -n ems -v 1.0.0 \
    --prefix /usr/local \
    .staging/bin=/usr/local/bin \
    .staging/lib=/usr/local/lib \
    .staging/include=/usr/local/include
```

## 变量说明

在 Makefile 中，以下变量指向 staging 目录：

```makefile
# Staging 根目录（支持配置）
# 优先级：命令行 > 环境变量 > 默认值
STAGING_DIR ?= $(objtree)/.staging

# 子目录
BIN_DIR     := $(STAGING_DIR)/bin
LIB_DIR     := $(STAGING_DIR)/lib
KO_DIR      := $(STAGING_DIR)/lib/modules
INCLUDE_DIR := $(STAGING_DIR)/include

# 编译时包含 staging 头文件
EMSINCLUDE  := -I$(INCLUDE_DIR)
```

在子目录 Makefile 中使用：

```makefile
# 应用程序 Makefile
app-y += myapp
myapp-objs := main.o utils.o

# 包含 staging 头文件（自动添加）
# ccflags-y += $(EMSINCLUDE)  # 已在 scripts/Makefile.build 中自动添加

# 链接 staging 库
myapp-ldflags := -L$(LIB_DIR) -losal -lhal
```

### 配置示例

```makefile
# 在顶层 Makefile 中
# 使用 ?= 允许外部配置
STAGING_DIR ?= $(objtree)/.staging

# 用户可以通过以下方式覆盖：
# 1. 命令行
make STAGING_DIR=/custom/path

# 2. 环境变量
export STAGING_DIR=/custom/path
make

# 3. 默认值
make  # 使用 .staging
```

## 外部构建支持

staging 目录也支持外部构建（O=dir）：

```bash
# 在独立目录中构建
mkdir ../build
make O=../build menuconfig
make O=../build -j$(nproc)

# staging 目录在构建目录下
ls ../build/.staging/

# 也可以自定义 staging 路径
make O=../build STAGING_DIR=/tmp/ems-staging -j$(nproc)
ls /tmp/ems-staging/
```

## Buildroot/Yocto 集成

### Buildroot 集成示例

**推荐方式：直接使用 Buildroot 的 STAGING_DIR**

```makefile
# package/ems/ems.mk
EMS_VERSION = 1.0.0
EMS_SITE = $(TOPDIR)/../EMS
EMS_SITE_METHOD = local
EMS_INSTALL_STAGING = YES

# 关键：传递 STAGING_DIR 环境变量
EMS_MAKE_ENV = \
    STAGING_DIR=$(STAGING_DIR) \
    ARCH=$(KERNEL_ARCH) \
    CROSS_COMPILE=$(TARGET_CROSS)

define EMS_CONFIGURE_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D) \
        ccm_h200_am625_release_defconfig
endef

define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D) \
        -j$(PARALLEL_JOBS)
endef

# 头文件和库已经在 $(STAGING_DIR) 中，无需额外安装
define EMS_INSTALL_STAGING_CMDS
    # 可选：如果需要额外处理
    # 大部分情况下不需要，因为已经在正确的位置
endef

define EMS_INSTALL_TARGET_CMDS
    # 只需安装可执行文件到目标系统
    $(INSTALL) -D -m 0755 $(STAGING_DIR)/bin/ccm_* $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(STAGING_DIR)/lib/*.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))
```

**优势：**
1. 无需重复拷贝头文件和库
2. 其他 package 可以直接找到 EMS 的头文件
3. 构建流程更简洁高效

### 传统方式（不推荐）

如果不使用 `STAGING_DIR` 环境变量：

```makefile
# package/ems/ems.mk
define EMS_BUILD_CMDS
    $(MAKE) -C $(@D) $(TARGET_CONFIGURE_OPTS) \
        CROSS_COMPILE=$(TARGET_CROSS) \
        -j$(PARALLEL_JOBS)
endef

define EMS_INSTALL_STAGING_CMDS
    # 需要手动拷贝
    cp -r $(@D)/.staging/include/* $(STAGING_DIR)/usr/include/
    cp -r $(@D)/.staging/lib/*.a $(STAGING_DIR)/usr/lib/
    cp -r $(@D)/.staging/lib/*.so $(STAGING_DIR)/usr/lib/
endef

define EMS_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/.staging/bin/* $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/.staging/lib/*.so $(TARGET_DIR)/usr/lib/
endef
```

### Yocto 集成示例

```bitbake
# recipes-ems/ems/ems_1.0.0.bb
SUMMARY = "EMS - Embedded Management System"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=..."

SRC_URI = "file://EMS"
S = "${WORKDIR}/EMS"

inherit autotools

# 传递 STAGING_DIR
EXTRA_OEMAKE = "STAGING_DIR=${STAGING_DIR_HOST}"

do_configure() {
    oe_runmake ccm_h200_am625_release_defconfig
}

do_compile() {
    oe_runmake -j${@oe.utils.cpu_count()}
}

do_install() {
    # 从 STAGING_DIR 安装到 ${D}
    install -d ${D}${bindir}
    install -m 0755 ${STAGING_DIR_HOST}/bin/* ${D}${bindir}/
    
    install -d ${D}${libdir}
    install -m 0644 ${STAGING_DIR_HOST}/lib/*.a ${D}${libdir}/
    install -m 0755 ${STAGING_DIR_HOST}/lib/*.so ${D}${libdir}/
    
    install -d ${D}${includedir}
    cp -r ${STAGING_DIR_HOST}/include/* ${D}${includedir}/
}

FILES_${PN} = "${bindir}/* ${libdir}/*.so"
FILES_${PN}-dev = "${includedir}/* ${libdir}/*.a"
```

## 常见问题

### Q: 为什么使用 .staging 隐藏目录？

A: 使用隐藏目录有以下优势：
1. 保持工作区整洁，`ls` 时不显示构建产物
2. 符合 Unix 惯例（如 .git、.build）
3. 避免与源码目录混淆
4. 仍可通过 `ls -a` 或 `ls .staging` 查看

### Q: 如何在 Buildroot 中使用？

A: 推荐方式是传递 `STAGING_DIR` 环境变量：

```makefile
# ems.mk
EMS_MAKE_ENV = STAGING_DIR=$(STAGING_DIR)

define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D)
endef
```

这样 EMS 会直接将产物输出到 Buildroot 的 staging，无需额外拷贝。

### Q: 为什么不直接输出到 bin/、lib/、include/？

A: 使用 staging 目录有以下优势：
1. 符合行业标准（Buildroot、Yocto）
2. 目录结构更清晰
3. 便于清理和打包
4. 避免污染源码树
5. 便于集成到其他构建系统
6. 支持灵活配置

### Q: staging 目录会被 git 跟踪吗？

A: 不会。`.staging` 目录已添加到 `.gitignore`：

```gitignore
# 构建产物
/.staging/
/staging/
```

### Q: 如何在应用程序中使用 staging 的库？

A: 编译时会自动包含 staging 头文件路径，链接时使用 `-L$(LIB_DIR)`：

```makefile
# 应用程序 Makefile
ccflags-y += $(EMSINCLUDE)  # 自动添加
myapp-ldflags := -L$(LIB_DIR) -losal -lhal
```

### Q: 如何部署到目标板？

A: 直接复制 staging 目录：

```bash
# 方法 1: 直接复制
scp -r .staging/* root@target:/usr/local/

# 方法 2: 打包后复制
tar czf ems.tar.gz .staging/
scp ems.tar.gz root@target:/tmp/
ssh root@target "cd /usr/local && tar xzf /tmp/ems.tar.gz --strip-components=1"

# 方法 3: 使用 rsync
rsync -av .staging/ root@target:/usr/local/
```

### Q: 可以同时使用多个 staging 目录吗？

A: 可以。通过 `STAGING_DIR` 参数指定不同路径：

```bash
# 开发版本
make STAGING_DIR=/tmp/ems-dev

# 发布版本
make STAGING_DIR=/tmp/ems-release

# 测试版本
make STAGING_DIR=/tmp/ems-test
```

### Q: 如何验证 staging 配置是否生效？

A: 查看编译输出或检查目录：

```bash
# 方法 1: 查看编译命令
make V=1 | grep STAGING_DIR

# 方法 2: 检查目录
STAGING_DIR=/tmp/test make
ls /tmp/test/

# 方法 3: 查看 Makefile 变量
make -p | grep STAGING_DIR
```

## 参考文档

- [STAGING.md](STAGING.md) - Staging 目录快速配置指南
- [BUILD_GUIDE.md](BUILD_GUIDE.md) - 构建指南
- [BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详解
- [MAKEFILE_FRAMEWORK.md](MAKEFILE_FRAMEWORK.md) - Makefile 框架说明
- [buildroot/README.md](buildroot/README.md) - Buildroot 集成指南
- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构设计
- [CLAUDE.md](../CLAUDE.md) - AI 助手指南

---

**最后更新**: 2026-05-22  
**维护者**: wanguo  
**版本**: 2.0.0
