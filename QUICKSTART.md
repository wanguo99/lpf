# 快速开发指南

本指南帮助开发者快速上手 EMS 项目，无需深入了解构建系统细节。

## 5 分钟快速开始

### 1. 克隆并编译

```bash
git clone https://github.com/your-org/EMS.git
cd EMS
make x86_64_full_defconfig
make -j$(nproc)
```

### 2. 运行测试

```bash
export LD_LIBRARY_PATH=$(pwd)/.staging/lib
./.staging/bin/ems-unit-test
```

## 添加新模块（3 步）

### 创建新应用程序

```bash
# 1. 复制模板
cp -r products/samples/myapp products/myproduct/mynewapp

# 2. 替换名称
cd products/myproduct/mynewapp
sed -i 's/myapp/mynewapp/g' module.mk
sed -i 's/MYAPP/MYNEWAPP/g' module.mk

# 3. 集成到构建系统
echo "include products/myproduct/mynewapp/module.mk" >> ../../Makefile
```

### 创建新库

```bash
# 1. 复制模板
cp -r products/samples/mylib products/myproduct/mynewlib

# 2. 替换名称
cd products/myproduct/mynewlib
sed -i 's/mylib/mynewlib/g' module.mk
sed -i 's/MYLIB/MYNEWLIB/g' module.mk

# 3. 集成到构建系统
echo "include products/myproduct/mynewlib/module.mk" >> ../../Makefile
```

## module.mk 模板说明

每个 module.mk 包含 5 个简单步骤：

```makefile
# 第 1 步：定义源文件
myapp_SRCS := products/myapp/src/main.c

# 第 2 步：定义头文件路径
myapp_CFLAGS := -Iinclude/osal

# 第 3 步：定义链接库
myapp_LDFLAGS := -L$(STAGING_DIR)/lib -losal

# 第 4 步：配置开关（可选）
ifeq ($(CONFIG_BUILD_MYAPP),y)
  MYAPP_ENABLED := y
endif

# 第 5 步：标准构建流程（无需修改）
# ... 自动处理 ...
```

## 常见场景

### 场景 1：简单应用（只依赖 OSAL）

```makefile
myapp_SRCS := products/myapp/src/main.c
myapp_CFLAGS := -Iinclude/osal
myapp_LDFLAGS := -L$(STAGING_DIR)/lib -losal -lpthread
```

### 场景 2：复杂应用（多个依赖）

```makefile
myapp_SRCS := \
	products/myapp/src/main.c \
	products/myapp/src/module1.c

myapp_CFLAGS := \
	-Iproducts/myapp/include \
	-Iinclude/osal \
	-Iinclude/hal

myapp_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-lhal -losal -lpthread
```

### 场景 3：库（导出头文件）

```makefile
mylib_SRCS := products/mylib/src/api.c
mylib_CFLAGS := -Iproducts/mylib/include
mylib_LDFLAGS := -L$(STAGING_DIR)/lib -Wl,-soname,libmylib.so.1 -losal
mylib_HEADERS := mylib.h mylib_types.h
```

## 目录结构

```
your-module/
├── module.mk          # 构建配置（复制模板）
├── src/               # 源文件
│   ├── main.c
│   └── module.c
└── include/           # 头文件
    └── module.h
```

## 构建命令

```bash
make menuconfig        # 配置
make -j$(nproc)       # 编译
make clean            # 清理
make V=1              # 详细输出
```

## 故障排除

### 找不到头文件

```bash
# 检查 staging 目录
ls .staging/include/

# 检查编译命令
make V=1 | grep myapp
```

### 找不到库

```bash
# 检查库文件
ls .staging/lib/

# 检查依赖
ldd .staging/bin/myapp
```

### 链接错误

```bash
# 查看详细链接命令
make V=1

# 检查库依赖顺序（高层在前，底层在后）
# 正确：-lpcl -lhal -losal
# 错误：-losal -lhal -lpcl
```

## 参考文档

- [示例模块详细说明](products/samples/README.md)
- [构建指南](docs/BUILD.md)
- [配置指南](docs/CONFIGURATION.md)
- [编码规范](docs/CODING_STANDARDS.md)

## 核心原则

1. **复制模板** - 从 `products/samples/` 复制
2. **替换名称** - 用 sed 批量替换
3. **修改 3 个变量** - SRCS、CFLAGS、LDFLAGS
4. **集成构建** - 在 Makefile 中 include
5. **编译测试** - make -j$(nproc)

就这么简单！
