# EMS 非递归 Make 迁移计划

> **文档版本**: 2.0  
> **创建日期**: 2026-05-25  
> **更新日期**: 2026-05-25  
> **状态**: 准备实施

---

## 📊 当前问题分析

### 实际测试数据（2026-05-25）

```bash
硬件环境：24核 CPU
项目规模：150 个源文件

# 串行编译
$ time make
real: 8.799s
user: 6.23s
sys:  4.04s
CPU:  116%

# 并行编译
$ time make -j24
real: 5.715s
user: 6.37s
sys:  3.97s
CPU:  181%  ← 仅用了不到 2 个核心！

提升：35%（理论应该接近 2400% CPU）
```

### 问题根源

**递归 Make 的模块间串行依赖**：

```
Core 层：osal → hal → pcl → pdl → acl（5个模块串行）
Products 层：libs → h200_am625 → apps（3个子目录串行）

实际编译顺序：
  DESCEND core/osal      ← 等待完成
  DESCEND core/hal       ← 等待完成
  DESCEND core/pcl       ← 等待完成
  DESCEND core/pdl       ← 等待完成
  DESCEND core/acl       ← 等待完成
  DESCEND products/ccm/libs
  DESCEND products/ccm/h200_am625
  DESCEND products/ccm/apps
```

**根本矛盾**：
- 父 make 只能控制子 make 的**启动顺序**
- 父 make 无法感知子 make 内部的**文件生成状态**
- 结果：模块必须串行，无法充分并行

---

## 🎯 迁移目标

### 性能目标

| 指标 | 当前（递归） | 目标（非递归） | 提升 |
|------|-------------|---------------|------|
| **全量编译** | 8.8秒 | 2-3秒 | **65-70%** |
| **增量编译（1文件）** | 3-4秒 | 0.5秒 | **85%** |
| **CPU 利用率** | 181% (2核) | 1500%+ (15核+) | **8倍** |
| **扩展到 500 文件** | ~30秒 | ~8秒 | **73%** |
| **扩展到 1000 文件** | ~2分钟 | ~20秒 | **83%** |

### 功能目标

✅ **完全兼容 Kconfig**（配置系统不变）  
✅ **保持目录结构**（无需大规模重组）  
✅ **保持开发习惯**（module.mk 类似现有 Makefile）  
✅ **支持交叉编译**（ARCH, CROSS_COMPILE 等）  
✅ **支持所有 defconfig**（6个配置全部兼容）

---

## 🔧 技术方案

### 核心原理

**非递归 Make = 单一 Make 进程 + 全局依赖图**

```makefile
# 递归 Make（当前）
core/Makefile:
    obj-y += osal/
    obj-y += hal/
    hal: osal  # ← 只能控制启动顺序，无法控制文件依赖

# 非递归 Make（目标）
Makefile:
    include core/osal/module.mk
    include core/hal/module.mk
    
    # 文件级精确依赖
    $(STAGING_DIR)/lib/libhal.so: $(HAL_OBJS) $(STAGING_DIR)/lib/libosal.so
    $(STAGING_DIR)/lib/libpcl.so: $(PCL_OBJS) $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so
```

### Kconfig 集成（完全兼容）

```makefile
# 1. 读取 Kconfig 生成的配置（和递归 Make 完全相同）
include .config

# 2. 根据配置包含模块（等价于 obj-$(CONFIG_XXX)）
ifeq ($(CONFIG_OSAL),y)
    include core/osal/module.mk
endif

ifeq ($(CONFIG_HAL),y)
    include core/hal/module.mk
endif

# 3. C 代码使用宏（和递归 Make 完全相同）
#include "generated/autoconf.h"
#ifdef CONFIG_OSAL_DEBUG_LOGGING
    // 调试日志
#endif
```

**结论**：Kconfig 和构建系统是独立的，非递归 Make 完全兼容！

### 文件结构

```
EMS/
├── Makefile                    # 顶层构建入口（唯一 make 进程）
├── scripts/
│   ├── rules.mk               # 通用编译规则
│   ├── functions.mk           # 辅助函数
│   └── auto-module.sh         # 自动生成 module.mk
├── core/
│   ├── osal/
│   │   ├── module.mk          # 定义 OSAL_SRCS, OSAL_OBJS, OSAL_TARGET
│   │   └── src/...
│   ├── hal/
│   │   ├── module.mk
│   │   └── src/...
│   └── ...
└── products/
    └── ccm/
        ├── libs/libccm/module.mk
        ├── h200_am625/module.mk
        └── apps/*/module.mk
```

---

## 📅 迁移计划（3周，渐进式）

### 阶段 1：基础设施（3天）

**目标**：搭建非递归 Make 框架，验证可行性

**任务**：
1. 创建 `scripts/rules.mk`（通用编译规则）
2. 创建 `scripts/functions.mk`（辅助函数）
3. 创建 `scripts/auto-module.sh`（自动生成工具）
4. 备份当前 Makefile 为 `Makefile.recursive`
5. 创建新的顶层 Makefile（框架）

**验证**：
- [ ] `make help` 正常工作
- [ ] `make clean` 正常工作
- [ ] Kconfig 配置正常（`make menuconfig`）

---

### 阶段 2：Core 层迁移（1周）

**目标**：迁移所有 Core 模块（osal, hal, pcl, pdl, acl）

#### 2.1 试点：core/osal（1天）

**任务**：
1. 生成 `core/osal/module.mk`
2. 在顶层 Makefile 中包含
3. 测试编译 libosal.so
4. 验证所有 defconfig

**验证**：
- [ ] `make` 生成 `.staging/lib/libosal.so`
- [ ] 所有 6 个 defconfig 编译通过
- [ ] 增量编译正常（修改 1 个文件）

#### 2.2 扩展：hal, pcl, pdl, acl（3天）

**任务**：
1. 依次生成各模块的 module.mk
2. 定义模块间依赖关系
3. 测试并行编译

**验证**：
- [ ] 所有 Core 库正常生成
- [ ] 并行度提升（CPU > 500%）
- [ ] 依赖关系正确（修改 osal 会重新链接 hal）

#### 2.3 性能测试（1天）

**对比测试**：
```bash
# 递归 Make
make clean && time make -j24

# 非递归 Make
make clean && time make -j24

# 对比 CPU 利用率、编译时间
```

**目标**：
- CPU 利用率 > 800%（8核以上）
- 全量编译时间 < 5秒

---

### 阶段 3：Products 层迁移（1周）

**目标**：迁移 Products 层（libs, h200_am625, apps）

#### 3.1 库和平台（2天）

**任务**：
1. 迁移 `products/ccm/libs/libccm`
2. 迁移 `products/ccm/h200_am625`
3. 测试与 Core 层的依赖

**验证**：
- [ ] libccm.so 正常生成
- [ ] libh200_am625.so 正常生成
- [ ] 依赖 Core 层的库正确链接

#### 3.2 应用程序（3天）

**任务**：
1. 迁移 5 个应用程序（ccm_supervisor, ccm_logger, ccm_comm, ccm_collector, ccm_health）
2. 测试应用程序链接
3. 测试运行时依赖

**验证**：
- [ ] 所有应用程序正常生成
- [ ] 应用程序可以正常运行
- [ ] `ldd` 检查动态库依赖正确

---

### 阶段 4：优化和文档（2天）

**任务**：
1. 性能调优（ccache, 预编译头等）
2. 编写使用文档
3. 编写 module.mk 编写规范
4. 清理临时文件和备份

**交付物**：
- [ ] 完整的非递归 Make 构建系统
- [ ] 使用文档（`docs/BUILD_SYSTEM.md`）
- [ ] module.mk 编写规范（`docs/MODULE_MK_GUIDE.md`）
- [ ] 性能测试报告

---

## ⚠️ 风险管理

### 风险 1：编译失败

**风险**：迁移过程中编译失败，影响开发

**缓解措施**：
- 保留 `Makefile.recursive` 作为备份
- 每个阶段充分测试后再进入下一阶段
- 提供快速回退方案：`mv Makefile.recursive Makefile`

### 风险 2：依赖关系错误

**风险**：模块间依赖关系定义错误，导致链接失败

**缓解措施**：
- 使用自动化工具生成依赖关系
- 每个模块迁移后立即测试
- 使用 `ldd` 和 `readelf` 验证动态库依赖

### 风险 3：Kconfig 不兼容

**风险**：非递归 Make 无法正确读取 Kconfig 配置

**缓解措施**：
- 在阶段 1 就验证 Kconfig 集成
- 测试所有 6 个 defconfig
- 参考 Linux 内核的实现（已验证可行）

### 风险 4：团队学习成本

**风险**：团队成员不熟悉 module.mk 编写

**缓解措施**：
- 提供详细的编写规范和示例
- 提供自动生成工具（`auto-module.sh`）
- 新模块可以参考现有模块的 module.mk

---

## ✅ 验证清单

### 功能验证

- [ ] 所有 6 个 defconfig 编译通过
- [ ] 所有库和应用程序正常生成
- [ ] 应用程序可以正常运行
- [ ] 增量编译正常（修改 1 个文件）
- [ ] `make clean` 正常工作
- [ ] `make menuconfig` 正常工作
- [ ] 交叉编译正常（ARCH=arm64）

### 性能验证

- [ ] 全量编译时间 < 3秒（目标 65% 提升）
- [ ] 增量编译时间 < 1秒（目标 85% 提升）
- [ ] CPU 利用率 > 1000%（目标 10核以上）
- [ ] 并行度测试：`make -j1` vs `make -j24`

### 依赖验证

- [ ] 修改 osal 源文件，hal/pcl/pdl/acl 重新链接
- [ ] 修改 hal 源文件，pcl/pdl/acl 重新链接
- [ ] 修改头文件，相关模块重新编译
- [ ] `ldd` 检查所有动态库依赖正确

---

## 📈 预期收益

### 短期收益（当前规模：150 文件）

- 全量编译：8.8秒 → 2-3秒（节省 **5-6秒**）
- 增量编译：3-4秒 → 0.5秒（节省 **2.5-3.5秒**）
- 开发体验：每天编译 50 次，节省 **2.5-5 分钟**

### 长期收益（扩展到 1000 文件）

- 全量编译：~2分钟 → ~20秒（节省 **100秒**）
- 增量编译：~10秒 → ~1秒（节省 **9秒**）
- 开发体验：每天编译 50 次，节省 **7.5 分钟**

### 团队收益

- 5 人团队，每人每天节省 5 分钟 = **25 分钟/天**
- 一年工作日 250 天 = **6250 分钟 ≈ 104 小时**
- 按人力成本计算：**节省 13 人天/年**

---

## 🚀 开始实施

### 准备工作

```bash
# 1. 确认当前分支
git branch
# 应该在 feature/non-recursive-make

# 2. 确认工作区干净
git status

# 3. 备份当前 Makefile
cp Makefile Makefile.recursive

# 4. 创建工作目录
mkdir -p scripts
```

### 下一步

开始 **阶段 1：基础设施搭建**

详见：[阶段 1 实施指南](#阶段-1基础设施3天)

---

## 📚 参考资料

- [递归 Make 的问题](http://aegis.sourceforge.net/auug97.pdf)（Peter Miller, 1997）
- [Linux 内核构建系统](https://www.kernel.org/doc/html/latest/kbuild/index.html)
- [非递归 Make 最佳实践](https://www.gnu.org/software/make/manual/html_node/Non_002dRecursive.html)
- 当前项目测试数据：`/tmp/build_detail.log`

---

**准备好开始了吗？** 🚀
