# EMS 构建系统重构计划

## 项目信息

- **分支**: `feature/refactor-build-system`
- **基于版本**: v1.0.0
- **目标**: 参考 Linux 内核构建系统，将 Makefile.build 拆分为多个专用模块
- **预计工期**: 2-3 周
- **负责人**: wanguo
- **创建日期**: 2026-05-22

---

## 一、重构目标

### 1.1 核心目标

1. **模块化**: 将 524 行的 `Makefile.build` 拆分为多个职责清晰的专用文件
2. **可维护性**: 每个文件职责单一，易于理解和修改
3. **可扩展性**: 添加新功能时不影响核心编译逻辑
4. **标准化**: 遵循 Linux 内核构建系统的最佳实践
5. **向后兼容**: 保持现有 Makefile 语法不变，用户无感知

### 1.2 非目标

- ❌ 不改变用户侧的 Makefile 语法
- ❌ 不改变构建产物的位置和格式
- ❌ 不引入新的依赖工具
- ❌ 不破坏现有的构建流程

---

## 二、当前问题分析

### 2.1 Makefile.build 的职责混乱

**当前状态** (524 行):
```
Makefile.build
├── 变量初始化 (31-70)
├── 目标类型处理 (95-130)
├── 稀疏检查 (133-143)
├── C 源码编译 (145-199)
├── 汇编源码编译 (201-221)
├── 链接器脚本 (227-234)
├── built-in.o 构建 (236-253)
├── 静态库构建 (255-297)        ← 用户空间特有
├── 动态库构建 (299-354)        ← 用户空间特有
├── 可执行程序构建 (356-395)    ← 用户空间特有
├── 内核模块构建 (397-427)      ← 预留
├── 复合对象链接 (429-449)
├── 子目录递归 (453-469)
└── 头文件安装 (471-505)        ← EMS 特有
```

**问题**:
- 内核风格的 `built-in.o` 与用户空间的库/应用混在一起
- 职责不清晰，难以维护
- 添加新功能需要修改核心文件
- 缺少编译器检测的独立模块

### 2.2 缺少模块化设计

**Linux 内核的模块化** (32 个专用文件):
```
scripts/
├── Makefile.compiler (93 行)    - 编译器检测
├── Makefile.lib (491 行)        - 变量处理
├── Makefile.build (591 行)      - 核心编译
├── Makefile.host (165 行)       - 主机程序
├── Makefile.userprogs (45 行)   - 用户程序
├── Makefile.modfinal (78 行)    - 模块链接
├── Makefile.modpost (157 行)    - 模块符号
├── Makefile.clean (63 行)       - 清理规则
└── ... (24 个其他专用文件)
```

**EMS 当前状态** (6 个文件):
```
scripts/
├── Kbuild.include (290 行)
├── Makefile.lib (198 行)
├── Makefile.build (524 行)      ← 需要拆分
├── Makefile.host (38 行)
├── Makefile.clean (52 行)
└── Makefile.install (188 行)
```

---

## 三、重构方案

### 3.1 目标架构

```
scripts/
├── Kbuild.include          (290 行) - 保持不变
│
├── 编译器和工具链
│   ├── Makefile.compiler   (新建, ~100 行) - 编译器检测和特性探测
│   └── scripts/
│       ├── gcc-version.sh  (新建) - GCC 版本检测
│       └── ld-version.sh   (新建) - 链接器版本检测
│
├── 变量处理
│   └── Makefile.lib        (198 行) - 保持不变
│
├── 核心编译规则
│   └── Makefile.build      (重构, ~250 行) - 源码编译和 built-in.o
│
├── 用户空间构建
│   ├── Makefile.lib.build  (新建, ~150 行) - 静态库和动态库构建
│   ├── Makefile.app.build  (新建, ~80 行)  - 应用程序构建
│   └── Makefile.mod.build  (新建, ~100 行) - 内核模块构建（预留）
│
├── 主机程序
│   └── Makefile.host       (38 行) - 保持不变
│
├── 清理和安装
│   ├── Makefile.clean      (52 行) - 保持不变
│   └── Makefile.install    (188 行) - 保持不变
│
└── 其他
    └── Makefile.extrawarn  (62 行) - 保持不变
```

### 3.2 文件职责划分

| 文件 | 职责 | 行数 | 状态 |
|------|------|------|------|
| `Makefile.compiler` | 编译器检测、版本探测、特性测试 | ~100 | 新建 |
| `Makefile.lib` | 变量展开、对象列表生成、路径处理 | 198 | 保持 |
| `Makefile.build` | C/汇编编译、built-in.o、子目录递归 | ~250 | 重构 |
| `Makefile.lib.build` | 静态库/动态库构建、头文件安装 | ~150 | 新建 |
| `Makefile.app.build` | 应用程序构建和安装 | ~80 | 新建 |
| `Makefile.mod.build` | 内核模块构建（预留） | ~100 | 新建 |

---

## 四、实施计划

### Phase 1: 准备工作 (1-2 天)

**目标**: 创建分支、文档、测试基准

**任务**:
- [x] 创建特性分支 `feature/refactor-build-system`
- [x] 编写重构计划文档
- [ ] 创建测试基准（记录当前构建时间和产物）
- [ ] 备份当前构建脚本

**验收标准**:
- 分支创建成功
- 文档完整
- 测试基准可重复执行

---

### Phase 2: 创建 Makefile.compiler (2-3 天)

**目标**: 提取编译器检测逻辑到独立文件

**任务**:
1. 创建 `scripts/Makefile.compiler`
   - 编译器类型检测 (`cc-name`)
   - 编译器版本检测 (`cc-version`, `cc-fullversion`)
   - 链接器版本检测 (`ld-version`)
   - 编译器选项测试 (`cc-option`, `cc-option-yn`, `cc-disable-warning`)
   - 链接器选项测试 (`ld-option`)
   - AR 选项测试 (`ar-option`)
   - LLVM 工具链支持

2. 创建辅助脚本
   - `scripts/gcc-version.sh` - GCC 版本检测
   - `scripts/ld-version.sh` - 链接器版本检测

3. 从 `scripts/Kbuild.include` 移除重复的编译器检测函数

4. 更新 `Makefile.build` 包含 `Makefile.compiler`

**验收标准**:
- `make V=1` 显示正确的编译器信息
- `cc-option` 等函数正常工作
- 构建时间无明显变化
- 所有模块编译成功

**测试**:
```bash
# 测试编译器检测
make clean
make V=1 core/osal/ 2>&1 | grep "CC.*osal_thread.o"

# 测试 LLVM 支持
make clean
make LLVM=1 core/osal/

# 测试交叉编译
make clean
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- core/osal/
```

---

### Phase 3: 创建 Makefile.lib.build (3-4 天)

**目标**: 提取库构建逻辑到独立文件

**任务**:
1. 创建 `scripts/Makefile.lib.build`
   - 静态库构建规则 (`lib-y`)
   - 动态库构建规则 (`so-y`)
   - 头文件安装规则 (`header-y`)
   - 库名智能处理（避免重复 `lib` 前缀）

2. 从 `Makefile.build` 移除库构建相关代码 (255-297, 299-354, 471-505 行)

3. 在 `Makefile.build` 中包含 `Makefile.lib.build`

**验收标准**:
- 静态库正确构建和安装到 `lib/`
- 动态库正确构建和安装到 `lib/`
- 头文件正确安装到 `include/`
- 库名处理正确（`lib-y += osal` → `libosal.a`）
- 构建时间无明显变化

**测试**:
```bash
# 测试静态库构建
make clean
make core/osal/
ls -lh .staging/lib/libosal.a
ls -lh .staging/include/osal.h

# 测试动态库构建（如果启用）
make menuconfig  # 启用 CONFIG_OSAL_BUILD_SHARED
make clean
make core/osal/
ls -lh .staging/lib/libosal.so

# 测试头文件安装
find .staging/include -name "osal*.h"
```

---

### Phase 4: 创建 Makefile.app.build (2-3 天)

**目标**: 提取应用程序构建逻辑到独立文件

**任务**:
1. 创建 `scripts/Makefile.app.build`
   - 应用程序构建规则 (`app-y`)
   - 应用程序链接规则
   - 应用程序安装规则

2. 从 `Makefile.build` 移除应用程序构建相关代码 (356-395 行)

3. 在 `Makefile.build` 中包含 `Makefile.app.build`

**验收标准**:
- 应用程序正确构建和安装到 `bin/`
- 链接标志正确应用
- RPATH 设置正确
- 构建时间无明显变化

**测试**:
```bash
# 测试应用程序构建
make clean
make products/ccm/apps/ccm_collector/
ls -lh .staging/bin/ccm_collector

# 测试链接
ldd .staging/bin/ccm_collector

# 测试 RPATH
readelf -d .staging/bin/ccm_collector | grep RPATH
```

---

### Phase 5: 创建 Makefile.mod.build (1-2 天)

**目标**: 提取内核模块构建逻辑到独立文件（预留）

**任务**:
1. 创建 `scripts/Makefile.mod.build`
   - 内核模块构建规则 (`obj-m`)
   - 模块链接规则
   - 模块安装规则

2. 从 `Makefile.build` 移除模块构建相关代码 (397-427 行)

3. 在 `Makefile.build` 中包含 `Makefile.mod.build`

**验收标准**:
- 模块构建框架就绪（即使当前没有实际模块）
- 不影响现有构建流程

**测试**:
```bash
# 验证构建系统完整性
make clean
make -j$(nproc)
```

---

### Phase 6: 重构 Makefile.build (2-3 天)

**目标**: 简化 Makefile.build，只保留核心编译规则

**任务**:
1. 移除已拆分到其他文件的代码
2. 保留核心功能：
   - 变量初始化
   - C 源码编译 (.c → .o)
   - 汇编源码编译 (.S → .o)
   - built-in.o 构建
   - 复合对象链接
   - 子目录递归

3. 添加包含语句：
   ```makefile
   include $(srctree)/scripts/Makefile.compiler
   include $(srctree)/scripts/Makefile.lib.build
   include $(srctree)/scripts/Makefile.app.build
   include $(srctree)/scripts/Makefile.mod.build
   ```

4. 优化代码结构和注释

**验收标准**:
- `Makefile.build` 行数减少到 ~250 行
- 所有功能正常工作
- 代码结构清晰
- 注释完整

**测试**:
```bash
# 完整构建测试
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# 增量构建测试
touch core/osal/src/posix/lib/osal_thread.c
make core/osal/

# 外部构建测试
make O=/tmp/ems-build ccm_h200_am625_debug_defconfig
make O=/tmp/ems-build -j$(nproc)
```

---

### Phase 7: 测试和验证 (3-4 天)

**目标**: 全面测试重构后的构建系统

**测试矩阵**:

| 测试项 | 测试命令 | 预期结果 |
|--------|----------|----------|
| 完整构建 | `make mrproper && make ccm_h200_am625_debug_defconfig && make -j$(nproc)` | 成功 |
| 增量构建 | 修改单个文件后 `make` | 只重新编译修改的文件 |
| 外部构建 | `make O=/tmp/build` | 成功 |
| 清理 | `make clean && make mrproper` | 完全清理 |
| 配置 | `make menuconfig` | 正常工作 |
| 安装 | `make install DESTDIR=/tmp/install` | 正确安装 |
| 交叉编译 | `make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-` | 成功 |
| LLVM 编译 | `make LLVM=1` | 成功 |
| 静态库 | 检查 `.staging/lib/*.a` | 正确生成 |
| 动态库 | 检查 `.staging/lib/*.so` | 正确生成 |
| 应用程序 | 检查 `.staging/bin/*` | 正确生成 |
| 头文件 | 检查 `.staging/include/` | 正确安装 |

**性能测试**:
```bash
# 记录重构前的构建时间
make mrproper
time make ccm_h200_am625_debug_defconfig
time make -j$(nproc)

# 记录重构后的构建时间
# 对比差异，确保性能无明显下降（允许 ±5%）
```

**回归测试**:
```bash
# 对比构建产物
diff -r /tmp/before/.staging /tmp/after/.staging

# 对比二进制文件
md5sum /tmp/before/.staging/bin/* /tmp/after/.staging/bin/*
md5sum /tmp/before/.staging/lib/* /tmp/after/.staging/lib/*
```

---

### Phase 8: 文档更新 (1-2 天)

**目标**: 更新所有相关文档

**任务**:
1. 更新 `docs/BUILD_SYSTEM.md`
   - 添加新文件的说明
   - 更新架构图
   - 添加职责说明

2. 更新 `docs/BUILD_GUIDE.md`
   - 更新构建流程说明
   - 添加新的调试方法

3. 更新 `CLAUDE.md`
   - 更新构建系统章节
   - 添加新文件的说明

4. 创建 `docs/refactor/BUILD_SYSTEM_REFACTOR_SUMMARY.md`
   - 重构总结
   - 变更说明
   - 迁移指南（如果有）

**验收标准**:
- 所有文档更新完整
- 架构图准确
- 示例代码可运行

---

### Phase 9: 代码审查和合并 (1-2 天)

**目标**: 代码审查并合并到主分支

**任务**:
1. 自我审查
   - 检查代码风格
   - 检查注释完整性
   - 检查错误处理

2. 创建 Pull Request
   - 编写详细的 PR 描述
   - 列出所有变更
   - 附上测试结果

3. 合并到 master
   - 解决冲突（如果有）
   - 更新 CHANGELOG
   - 创建新的 tag (v1.1.0)

**验收标准**:
- 代码审查通过
- 所有测试通过
- 文档完整
- 成功合并到 master

---

## 五、风险和缓解措施

### 5.1 风险识别

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|----------|
| 破坏现有构建流程 | 高 | 中 | 充分测试，保持向后兼容 |
| 性能下降 | 中 | 低 | 性能基准测试，优化包含逻辑 |
| 引入新 bug | 高 | 中 | 全面的回归测试 |
| 文档不同步 | 中 | 中 | 同步更新文档 |
| 合并冲突 | 低 | 低 | 及时同步 master 分支 |

### 5.2 回滚计划

如果重构失败，可以：
1. 回滚到 v1.0.0 tag
2. 删除特性分支
3. 分析失败原因
4. 重新规划

---

## 六、成功标准

### 6.1 功能标准

- ✅ 所有现有功能正常工作
- ✅ 构建产物与重构前一致
- ✅ 构建时间无明显变化（±5%）
- ✅ 支持所有现有的构建选项

### 6.2 质量标准

- ✅ 代码结构清晰，职责单一
- ✅ 注释完整，易于理解
- ✅ 符合 Linux 内核编码风格
- ✅ 文档完整准确

### 6.3 可维护性标准

- ✅ 添加新功能时不需要修改核心文件
- ✅ 每个文件职责清晰
- ✅ 易于调试和排错

---

## 七、参考资料

### 7.1 Linux 内核构建系统

- Linux 内核源码: `/home/wanguo/AM62x/ti-linux-kernel/`
- 关键文件:
  - `scripts/Makefile.build` (591 行)
  - `scripts/Makefile.compiler` (93 行)
  - `scripts/Makefile.lib` (491 行)
  - `scripts/Makefile.host` (165 行)
  - `scripts/Makefile.userprogs` (45 行)

### 7.2 EMS 项目文档

- `docs/BUILD_SYSTEM.md` - 构建系统详解
- `docs/BUILD_GUIDE.md` - 构建指南
- `docs/ARCHITECTURE.md` - 架构设计
- `CLAUDE.md` - 项目指南

### 7.3 相关标准

- Linux Kernel Coding Style
- Kbuild 文档
- GNU Make 手册

---

## 八、附录

### 8.1 当前构建脚本统计

```
scripts/
├── Kbuild.include      290 行
├── Makefile.lib        198 行
├── Makefile.build      524 行  ← 需要拆分
├── Makefile.host        38 行
├── Makefile.clean       52 行
├── Makefile.install    188 行
└── Makefile.extrawarn   62 行
总计: 1352 行
```

### 8.2 目标构建脚本统计

```
scripts/
├── Kbuild.include          290 行
├── Makefile.compiler       100 行  ← 新建
├── Makefile.lib            198 行
├── Makefile.build          250 行  ← 重构
├── Makefile.lib.build      150 行  ← 新建
├── Makefile.app.build       80 行  ← 新建
├── Makefile.mod.build      100 行  ← 新建
├── Makefile.host            38 行
├── Makefile.clean           52 行
├── Makefile.install        188 行
└── Makefile.extrawarn       62 行
总计: 1508 行 (+156 行，+11.5%)
```

**说明**: 行数增加是因为：
1. 添加了更多的注释和文档
2. 添加了编译器检测功能
3. 代码结构更清晰（空行和分隔符）

### 8.3 时间线

```
Week 1: Phase 1-3 (准备、Makefile.compiler、Makefile.lib.build)
Week 2: Phase 4-6 (Makefile.app.build、Makefile.mod.build、重构 Makefile.build)
Week 3: Phase 7-9 (测试、文档、合并)
```

---

**文档版本**: v1.0  
**最后更新**: 2026-05-22  
**状态**: 待实施
