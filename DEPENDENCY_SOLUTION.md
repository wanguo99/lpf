# EMS 依赖管理完整解决方案

## 当前问题总结

### 1. 动态库依赖不完整 ⚠️ 严重

**问题**：
```bash
libacl.so 应该依赖 libpdl.so，但实际上没有
libh200_am625.so 没有任何 NEEDED 条目
```

**原因**：
- Makefile 中声明了 `-lpdl -lpcl -lhal`
- 但链接时这些符号没有被引用，链接器优化掉了

**后果**：
- 运行时可能找不到符号
- 部署时必须手动指定所有库

### 2. Core 内部依赖可能失效 ⚠️ 中等

**问题**：
```makefile
# core/Makefile
hal: osal
pcl: hal osal
```

这些依赖规则在递归 Make 中可能不生效，因为：
- 目标名是 `hal`，但实际调用是 `make -C core hal/`
- 目标名不匹配

**验证**：
```bash
make clean
make core -j8
# 观察是否严格按 osal → hal → pcl → pdl → acl 顺序
```

### 3. 链接顺序问题 ⚠️ 低

**当前顺序**：
```
-lh200_am625 -lccm -lacl -lpdl -lpcl -lhal -losal
```

**问题**：
- 如果 libh200_am625.so 使用了 libacl.so 的符号
- 链接器从左到右解析，可能找不到符号

**正确顺序**（被依赖的在后）：
```
-lh200_am625 -lccm -lacl -lpdl -lpcl -lhal -losal
```

实际上当前顺序是对的！

---

## 方案A：最小改动优化（推荐）

### A1. 修复动态库依赖

#### 问题根源

动态库链接时，如果没有未定义的符号，链接器不会记录依赖。

#### 解决方案：使用 --no-as-needed

```makefile
# core/acl/Makefile
acl-ldflags := $(if $(CONFIG_ACL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libacl.so.1)
# 添加 --no-as-needed，强制记录依赖
acl-ldflags += -Wl,--no-as-needed
acl-ldflags += -L$(LIB_DIR) -lpdl -lpcl -lhal -losal
```

**原理**：
- `--as-needed`（默认）：只链接用到的库
- `--no-as-needed`：强制链接所有指定的库

#### 修改所有动态库

```bash
# 需要修改的文件：
core/hal/Makefile
core/pcl/Makefile
core/pdl/Makefile
core/acl/Makefile
products/ccm/libs/libccm/Makefile
products/ccm/h200_am625/Makefile
```

### A2. 改进 Core 内部依赖

#### 当前方案（可能失效）

```makefile
# core/Makefile
hal: osal
pcl: hal osal
pdl: pcl hal osal
acl: pdl pcl hal osal
```

#### 改进方案：使用 subdir-ym 变量

```makefile
# core/Makefile

# 方法1：串行构建（最简单，但慢）
.NOTPARALLEL:

obj-$(CONFIG_OSAL) += osal/
obj-$(CONFIG_HAL)  += hal/
obj-$(CONFIG_PCL)  += pcl/
obj-$(CONFIG_PDL)  += pdl/
obj-$(CONFIG_ACL)  += acl/

# 方法2：显式依赖（推荐）
obj-$(CONFIG_OSAL) += osal/
obj-$(CONFIG_HAL)  += hal/
obj-$(CONFIG_PCL)  += pcl/
obj-$(CONFIG_PDL)  += pdl/
obj-$(CONFIG_ACL)  += acl/

# 使用 order-only prerequisites
hal/: | osal/
pcl/: | hal/ osal/
pdl/: | pcl/ hal/ osal/
acl/: | pdl/ pcl/ hal/ osal/
```

### A3. 验证依赖

```bash
# 1. 清理并重新编译
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# 2. 检查动态库依赖
for lib in .staging/lib/lib*.so; do
    echo "$(basename $lib):"
    readelf -d "$lib" | grep NEEDED
done

# 3. 压力测试
for i in {1..10}; do
    make clean > /dev/null 2>&1
    make -j$(nproc) || echo "Failed at iteration $i"
done
```

---

## 方案B：显式依赖文件（中等改动）

### 核心思路

创建一个专门的依赖配置文件，集中管理所有依赖关系。

### 实现

```makefile
# scripts/Makefile.deps
# EMS 依赖关系配置

# Core 层依赖
CORE_DEPS := \
    hal:osal \
    pcl:hal,osal \
    pdl:pcl,hal,osal \
    acl:pdl,pcl,hal,osal

# Products 层依赖
PRODUCTS_DEPS := \
    libs:core \
    h200_am625:core \
    apps:libs,h200_am625,core

# 解析依赖并生成规则
$(foreach dep,$(CORE_DEPS),\
    $(eval TARGET := $(word 1,$(subst :, ,$(dep)))) \
    $(eval PREREQS := $(subst $(comma), ,$(word 2,$(subst :, ,$(dep))))) \
    $(eval $(TARGET)/: | $(addsuffix /,$(PREREQS))) \
)
```

### 优点

- 依赖关系集中管理
- 易于维护和理解
- 可以自动生成依赖图

### 缺点

- 需要额外的配置文件
- 增加了构建系统的复杂度

---

## 方案C：递归 Make 依赖（大改动）

### 核心思路

使用 Make 的递归特性，让每个模块的 Makefile 显式声明依赖。

### 实现

```makefile
# core/hal/Makefile

# 声明依赖的模块
DEPENDS_ON := osal

# 在构建前先构建依赖
__build: $(DEPENDS_ON)
	@# 实际构建逻辑

$(DEPENDS_ON):
	+$(Q)$(MAKE) -C $(srctree)/core/$@ $(MAKECMDGOALS)

.PHONY: $(DEPENDS_ON)
```

### 优点

- 依赖关系在模块内部声明，更清晰
- 不依赖顶层 Makefile 的协调

### 缺点

- 每个模块都要声明依赖，重复代码多
- 递归 Make 效率低

---

## 方案D：非递归 Make（彻底重构）

### 核心思路

参考 Linux 内核的 Kbuild，使用非递归 Make。

### 实现

```makefile
# Makefile

# 收集所有源文件
osal-src := $(wildcard core/osal/src/posix/*/*.c)
hal-src := $(wildcard core/hal/src/generic-linux/*.c)
pcl-src := $(wildcard core/pcl/src/*.c)
pdl-src := $(wildcard core/pdl/src/*/*.c)
acl-src := $(wildcard core/acl/src/*.c)

# 生成目标文件
osal-obj := $(osal-src:.c=.o)
hal-obj := $(hal-src:.c=.o)
pcl-obj := $(pcl-src:.c=.o)
pdl-obj := $(pdl-src:.c=.o)
acl-obj := $(acl-src:.c=.o)

# 显式依赖
lib/libhal.so: $(hal-obj) lib/libosal.so
lib/libpcl.so: $(pcl-obj) lib/libhal.so lib/libosal.so
lib/libpdl.so: $(pdl-obj) lib/libpcl.so lib/libhal.so lib/libosal.so
lib/libacl.so: $(acl-obj) lib/libpdl.so lib/libpcl.so lib/libhal.so lib/libosal.so
```

### 优点

- 依赖关系最清晰
- Make 可以完全并行化
- 增量编译最快

### 缺点

- 需要彻底重构构建系统
- 不符合 Kbuild 的递归风格
- 维护成本高

---

## 推荐方案

### 当前阶段（预研）：方案A

**理由**：
1. 改动最小，风险最低
2. 已经通过压力测试
3. 只需修复动态库依赖问题

**实施步骤**：
1. 修改所有动态库的 Makefile，添加 `--no-as-needed`
2. 改进 core/Makefile 的依赖声明
3. 验证并测试

### 未来优化（量产）：方案B 或 方案D

**理由**：
1. 方案B：依赖关系更清晰，易于维护
2. 方案D：编译速度最快，适合大型项目

---

## 附录：依赖管理最佳实践

### 1. 链接顺序规则

```
被依赖的库放在后面
应用程序 → 高层库 → 低层库 → 系统库

示例：
gcc -o app app.o -lacl -lpdl -lpcl -lhal -losal -lpthread -lrt
```

### 2. 动态库依赖

```makefile
# 方法1：--no-as-needed（推荐）
ldflags += -Wl,--no-as-needed -lpdl -lpcl

# 方法2：--copy-dt-needed-entries（不推荐，已废弃）
ldflags += -Wl,--copy-dt-needed-entries

# 方法3：显式引用符号（最可靠）
# 在代码中添加：
extern void pdl_init(void);
void __attribute__((constructor)) acl_init(void) {
    pdl_init();  // 强制引用 PDL 符号
}
```

### 3. 循环依赖检测

```bash
# 使用 ldd 检测
for lib in lib/*.so; do
    echo "=== $lib ==="
    ldd $lib | grep "lib.*\.so"
done | grep -E "not found|循环"

# 使用 readelf 检测
for lib in lib/*.so; do
    echo "=== $lib ==="
    readelf -d $lib | grep NEEDED
done
```

### 4. 并行编译测试

```bash
# 压力测试脚本
#!/bin/bash
for i in {1..20}; do
    echo "=== Iteration $i ==="
    make clean > /dev/null 2>&1
    if ! make -j$(nproc) > /tmp/build_$i.log 2>&1; then
        echo "FAILED at iteration $i"
        cat /tmp/build_$i.log
        exit 1
    fi
done
echo "All tests passed!"
```

---

## 总结

你当前的依赖管理方案已经很好了，只需要：

1. **立即修复**：动态库依赖（添加 `--no-as-needed`）
2. **可选优化**：改进 core/Makefile 的依赖声明
3. **长期规划**：考虑方案B或方案D

**优先级**：
- 🔴 高：修复动态库依赖
- 🟡 中：改进 Core 依赖声明
- 🟢 低：重构为非递归 Make
