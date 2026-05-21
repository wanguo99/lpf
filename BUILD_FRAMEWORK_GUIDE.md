# EMS 统一构建框架使用指南

## 概述

EMS 采用 Linux 内核风格的 Kbuild 构建系统，支持四种目标类型：
- **app-y**: 可执行程序
- **so-y**: 动态库 (.so)
- **lib-y**: 静态库 (.a)
- **obj-m**: 内核模块 (.ko) [预留]

## 快速开始

### 1. 配置构建

```bash
# 使用预设配置
make ccm_h200_am625_release_defconfig

# 或使用交互式配置
make menuconfig

# 同步配置
make syncconfig
```

### 2. 构建

```bash
# 构建所有模块
make -j$(nproc)

# 构建特定模块
make core -j$(nproc)      # 只构建核心模块
make products -j$(nproc)  # 只构建产品模块

# 外部构建（保持源码树干净）
make O=/tmp/build defconfig
make O=/tmp/build -j$(nproc)

# 详细输出
make V=1 -j$(nproc)
```

### 3. 输出位置

- 可执行程序: `bin/` (或 `$O/bin/`)
- 动态库: `lib/` (或 `$O/lib/`)
- 静态库: `lib/` (或 `$O/lib/`)
- 内核模块: `ko/` (或 `$O/ko/`)

## Kbuild 文件编写

### 基本结构

每个模块目录需要一个 `Kbuild` 文件（或 `Makefile`），定义构建规则。

### 1. 构建可执行程序

```makefile
# 定义可执行程序
app-y += myapp

# 指定源文件（编译为 .o）
myapp-objs := main.o utils.o config.o

# 链接标志
myapp-ldflags := -L$(objtree)/lib
myapp-ldflags += -losal -lhal
myapp-ldflags += -lpthread -lm

# 编译标志（可选）
ccflags-y += -I$(src)/include
ccflags-y += -DMYAPP_VERSION=\"1.0\"
```

**说明**:
- `app-y += myapp`: 声明构建名为 `myapp` 的可执行程序
- `myapp-objs`: 列出所有需要编译的源文件（.c 自动转为 .o）
- `myapp-ldflags`: 链接时的标志（库路径、库名等）
- 程序自动安装到 `$(BIN_DIR)`

### 2. 构建动态库

```makefile
# 定义动态库（自动添加 .so 后缀）
so-y += hal

# 指定源文件
hal-objs := hal_init.o
hal-objs += hal_can.o
hal-objs += hal_uart.o

# 动态库必须使用 -fPIC
hal-cflags-y := -fPIC

# 动态库链接标志
hal-ldflags := -shared -Wl,-soname,libhal.so.1
hal-ldflags += -L$(objtree)/lib -losal

# 编译标志
ccflags-y += -I$(src)/include
```

**说明**:
- `so-y += hal`: 声明构建 `libhal.so` 动态库
- `hal-cflags-y := -fPIC`: 动态库必须使用位置无关代码
- `hal-ldflags`: 动态库链接标志，包括 `-shared` 和 soname
- 库自动安装到 `$(LIB_DIR)`

### 3. 构建静态库

```makefile
# 定义静态库（自动添加 .a 后缀）
lib-y += osal

# 指定源文件
osal-objs := osal_init.o
osal-objs += osal_task.o
osal-objs += osal_queue.o
osal-objs += osal_mutex.o

# 编译标志
ccflags-y += -I$(src)/include
ccflags-$(CONFIG_DEBUG_OSAL) += -DDEBUG_OSAL
```

**说明**:
- `lib-y += osal`: 声明构建 `libosal.a` 静态库
- `osal-objs`: 列出所有需要编译的源文件
- 静态库不需要 `-fPIC`
- 库自动安装到 `$(LIB_DIR)`

### 4. 构建内核模块（预留）

```makefile
# 定义内核模块
obj-m += mydrv.o

# 指定源文件
mydrv-objs := mydrv_main.o mydrv_ops.o

# 编译标志
ccflags-y += -I$(src)/include
```

**说明**:
- `obj-m += mydrv.o`: 声明构建 `mydrv.ko` 内核模块
- 模块自动安装到 `$(KO_DIR)`

### 5. 条件编译

```makefile
# 根据 Kconfig 配置条件编译
so-$(CONFIG_HAL) += hal

# 条件添加源文件
hal-objs := hal_init.o
hal-objs += $(if $(CONFIG_HAL_CAN),hal_can.o)
hal-objs += $(if $(CONFIG_HAL_UART),hal_uart.o)

# 条件编译标志
ccflags-$(CONFIG_DEBUG_HAL) += -DDEBUG_HAL
ccflags-$(CONFIG_HAL_VERBOSE) += -DHAL_VERBOSE
```

### 6. 子目录构建

```makefile
# 下降到子目录构建
subdir-y += protocol/
subdir-y += drivers/
subdir-$(CONFIG_CCM_TESTS) += tests/
```

### 7. 编译标志详解

```makefile
# 全局编译标志（影响所有源文件）
ccflags-y += -I$(src)/include
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -DMODULE_VERSION=\"1.0\"

# 条件编译标志
ccflags-$(CONFIG_DEBUG) += -DDEBUG -g
ccflags-$(CONFIG_OPTIMIZE_SIZE) += -Os

# 特定文件的编译标志
CFLAGS_main.o := -DMAIN_VERSION=\"2.0\"
CFLAGS_hal_can.o := -DCAN_DRIVER

# 汇编标志
asflags-y += -D__ASSEMBLY__

# 预处理标志
cppflags-y += -DPREPROCESSOR_FLAG

# 链接标志
ldflags-y += -Wl,--as-needed
```

### 8. 头文件导出

```makefile
# 导出头文件（用于 make headers_install）
header-y := module.h
header-y += module_api.h
header-y += module_types.h
```

### 9. 清理规则

```makefile
# 定义需要清理的文件
clean-files := *.so *.so.* *.a myapp
clean-files += generated_file.c
```

## 完整示例

### 示例 1: 构建可执行程序 + 动态库

```makefile
# products/myproduct/Kbuild

# 1. 构建可执行程序
app-y += myapp

myapp-objs := main.o
myapp-objs += app_init.o
myapp-objs += app_logic.o

myapp-ldflags := -L$(objtree)/lib
myapp-ldflags += -losal -lhal -lmylib
myapp-ldflags += -lpthread

# 2. 构建产品专有库
so-y += mylib

mylib-objs := mylib_init.o
mylib-objs += mylib_api.o
mylib-objs += mylib_utils.o

mylib-cflags-y := -fPIC
mylib-ldflags := -shared -Wl,-soname,libmylib.so.1
mylib-ldflags += -L$(objtree)/lib -losal

# 3. 编译标志
ccflags-y += -I$(src)/include
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/core/hal/include
ccflags-$(CONFIG_DEBUG) += -DDEBUG

# 4. 头文件导出
header-y := mylib.h mylib_api.h

# 5. 清理
clean-files := myapp *.so *.so.*
```

### 示例 2: 多个库和程序

```makefile
# core/mymodule/Kbuild

# 1. 静态库
lib-y += mymodule_static

mymodule_static-objs := core.o utils.o

# 2. 动态库
so-y += mymodule

mymodule-objs := core.o utils.o api.o
mymodule-cflags-y := -fPIC
mymodule-ldflags := -shared -Wl,-soname,libmymodule.so.1

# 3. 测试程序
app-$(CONFIG_MYMODULE_TEST) += mymodule_test

mymodule_test-objs := test_main.o
mymodule_test-ldflags := -L$(objtree)/lib -lmymodule -losal

# 4. 编译标志
ccflags-y += -I$(src)/include
ccflags-$(CONFIG_DEBUG_MYMODULE) += -DDEBUG_MYMODULE
```

## 高级用法

### 1. 复合对象

如果多个源文件需要先链接成一个中间对象，再用于最终目标：

```makefile
# 定义复合对象
obj-y += composite.o

# 复合对象的组成部分
composite-objs := part1.o part2.o part3.o

# 使用复合对象
app-y += myapp
myapp-objs := main.o composite.o
```

### 2. 生成的源文件

```makefile
# 从模板生成源文件
$(src)/generated.c: $(src)/template.c.in
	$(Q)sed 's/@VERSION@/$(VERSION)/g' $< > $@

# 将生成的文件添加到目标
myapp-objs += generated.o

# 确保生成文件在编译前创建
$(obj)/main.o: $(src)/generated.c
```

### 3. 外部库依赖

```makefile
# 链接外部库
myapp-ldflags += -L/usr/local/lib
myapp-ldflags += -lcurl -lssl -lcrypto

# 添加外部头文件路径
ccflags-y += -I/usr/local/include
```

### 4. 架构特定代码

```makefile
# 根据架构选择源文件
hal-objs := hal_init.o
hal-objs += $(if $(CONFIG_ARM),hal_arm.o)
hal-objs += $(if $(CONFIG_X86),hal_x86.o)
hal-objs += $(if $(CONFIG_RISCV),hal_riscv.o)
```

## 常见问题

### Q1: 如何添加新模块？

1. 在 `core/` 或 `products/` 下创建模块目录
2. 创建 `Kbuild` 文件，定义构建规则
3. 在上级目录的 `Kbuild` 中添加子目录引用：
   ```makefile
   obj-$(CONFIG_MYMODULE) += mymodule/
   ```
4. 在 `Kconfig` 中添加配置选项

### Q2: 如何调试构建问题？

```bash
# 详细输出
make V=1

# 查看实际执行的命令
make V=2

# 只构建特定目标
make core/hal/hal_can.o V=1

# 查看依赖关系
make core/hal/hal_can.o -n
```

### Q3: 动态库找不到符号？

确保链接顺序正确，被依赖的库放在后面：

```makefile
# 正确
myapp-ldflags := -lmylib -losal -lhal

# 错误（如果 mylib 依赖 osal）
myapp-ldflags := -losal -lmylib
```

### Q4: 如何处理循环依赖？

使用 `-Wl,--start-group` 和 `-Wl,--end-group`：

```makefile
myapp-ldflags := -Wl,--start-group -lmodule1 -lmodule2 -Wl,--end-group
```

### Q5: 如何添加编译警告选项？

```makefile
# 添加警告
ccflags-y += -Wextra -Wshadow

# 禁用特定警告
ccflags-y += -Wno-unused-parameter
```

## 构建系统变量参考

### 常用变量

- `$(src)`: 当前源码目录
- `$(obj)`: 当前对象目录
- `$(srctree)`: 源码树根目录
- `$(objtree)`: 对象树根目录
- `$(BIN_DIR)`: 可执行程序输出目录
- `$(LIB_DIR)`: 库文件输出目录
- `$(KO_DIR)`: 内核模块输出目录

### 编译器变量

- `$(CC)`: C 编译器
- `$(LD)`: 链接器
- `$(AR)`: 归档工具
- `$(CROSS_COMPILE)`: 交叉编译前缀

### 标志变量

- `ccflags-y`: C 编译标志
- `asflags-y`: 汇编标志
- `ldflags-y`: 链接标志
- `cppflags-y`: 预处理标志

## 最佳实践

1. **使用条件编译**: 通过 Kconfig 控制功能开关
2. **模块化设计**: 每个模块独立的 Kbuild 文件
3. **清晰的依赖**: 明确指定库依赖关系
4. **头文件管理**: 使用 `header-y` 导出公共头文件
5. **外部构建**: 使用 `O=` 保持源码树干净
6. **并行构建**: 使用 `-j$(nproc)` 加速编译
7. **详细日志**: 调试时使用 `V=1` 查看完整命令

## 迁移指南

### 从旧构建系统迁移

如果你的模块使用旧的 Makefile，迁移步骤：

1. 将 `Makefile` 重命名为 `Kbuild`
2. 替换目标定义：
   ```makefile
   # 旧方式
   TARGET = myapp
   OBJS = main.o utils.o
   
   # 新方式
   app-y += myapp
   myapp-objs := main.o utils.o
   ```
3. 替换编译标志：
   ```makefile
   # 旧方式
   CFLAGS += -I./include
   
   # 新方式
   ccflags-y += -I$(src)/include
   ```
4. 删除显式的编译和链接规则（由框架自动处理）

## 总结

EMS 统一构建框架提供了：
- ✅ 四种目标类型支持（app/so/lib/ko）
- ✅ Linux 内核风格的 Kbuild 语法
- ✅ 条件编译和配置管理
- ✅ 外部构建支持
- ✅ 并行构建和增量编译
- ✅ 静默构建和详细输出
- ✅ 自动依赖跟踪
- ✅ 统一的安装规则

遵循本指南，你可以轻松地为 EMS 项目添加新模块和功能。
