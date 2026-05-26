# EMS 安装指南

## 系统要求

### 操作系统
- Linux (Ubuntu 20.04+, Debian 11+, CentOS 8+)
- macOS 12+
- Windows (WSL2)

### 编译工具
- GCC 7.0+ 或 Clang 10.0+
- GNU Make 4.3+
- Bison 3.0+
- Flex 2.6+
- pkg-config

### 可选工具
- ncurses-dev (用于 menuconfig)
- git (用于版本控制)

## 安装依赖

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    bison \
    flex \
    pkg-config \
    libncurses-dev \
    git
```

### CentOS/RHEL

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y \
    gcc \
    gcc-c++ \
    make \
    bison \
    flex \
    pkgconfig \
    ncurses-devel \
    git
```

### macOS

```bash
# 安装 Homebrew（如果未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install gcc make bison flex pkg-config ncurses git
```

## 获取源码

```bash
git clone https://github.com/your-org/EMS.git
cd EMS
```

## 编译安装

### 本地编译（x86_64）

```bash
# 1. 配置
make x86_64_full_defconfig

# 2. 编译
make -j$(nproc)

# 3. 安装（可选）
sudo make install
```

### 交叉编译（ARM64）

```bash
# 1. 安装交叉编译工具链
sudo apt-get install -y gcc-aarch64-linux-gnu

# 2. 配置
make arm64_full_defconfig

# 3. 编译
make -j$(nproc)

# 4. 部署到目标设备
scp -r .staging/* root@target-device:/usr/local/
```

## 安装位置

默认安装路径：`/usr/local/`

```
/usr/local/
├── bin/                 # 可执行文件
│   ├── ccm_collector
│   ├── ccm_health
│   ├── ccm_logger
│   ├── ccm_supervisor
│   └── ccm_comm
├── lib/                 # 库文件
│   ├── libosal.so
│   ├── libhal.so
│   ├── libpcl.so
│   ├── libpdl.so
│   └── libacl.so
└── include/            # 头文件
    ├── osal/
    ├── hal/
    ├── pcl/
    ├── pdl/
    └── acl/
```

### 自定义安装路径

```bash
# 安装到 /opt/ems
sudo make install prefix=/opt/ems

# 安装到临时目录（用于打包）
make install DESTDIR=/tmp/ems-package prefix=/usr
```

## 验证安装

```bash
# 检查可执行文件
which ccm_collector

# 检查库文件
ldconfig -p | grep libosal

# 运行测试
ccm_collector --version
```

## 卸载

```bash
sudo make uninstall
```

## 故障排除

### 找不到 ncurses

```bash
# Ubuntu/Debian
sudo apt-get install libncurses-dev

# CentOS/RHEL
sudo yum install ncurses-devel
```

### Make 版本过低

```bash
# 检查版本
make --version

# 需要 Make 4.3+，如果版本过低，从源码编译：
wget https://ftp.gnu.org/gnu/make/make-4.3.tar.gz
tar xzf make-4.3.tar.gz
cd make-4.3
./configure --prefix=/usr/local
make && sudo make install
```

### 交叉编译工具链

```bash
# ARM64
sudo apt-get install gcc-aarch64-linux-gnu

# ARM32
sudo apt-get install gcc-arm-linux-gnueabihf

# RISC-V
sudo apt-get install gcc-riscv64-linux-gnu
```
