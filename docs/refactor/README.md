# EMS 构建系统重构

## 概述

本目录包含 EMS 构建系统重构的相关文档和计划。

**分支**: `feature/refactor-build-system`  
**状态**: 🚧 进行中  
**开始日期**: 2026-05-22  
**预计完成**: 2026-06-12 (3 周)

---

## 重构目标

将 `scripts/Makefile.build` (524 行) 拆分为多个职责清晰的专用模块，参考 Linux 内核构建系统的模块化设计。

### 核心改进

1. **模块化**: 职责单一，易于维护
2. **可扩展性**: 添加新功能不影响核心逻辑
3. **标准化**: 遵循 Linux 内核最佳实践
4. **向后兼容**: 用户无感知

---

## 文档列表

| 文档 | 说明 |
|------|------|
| [BUILD_SYSTEM_REFACTOR_PLAN.md](BUILD_SYSTEM_REFACTOR_PLAN.md) | 详细的重构计划（目标、方案、时间线） |
| [REFACTOR_CHECKLIST.md](REFACTOR_CHECKLIST.md) | 实施检查清单（任务、测试、验收） |

---

## 重构架构

### 当前架构 (v1.0.0)

```
scripts/
├── Kbuild.include      290 行
├── Makefile.lib        198 行
├── Makefile.build      524 行  ← 职责混乱
├── Makefile.host        38 行
├── Makefile.clean       52 行
└── Makefile.install    188 行
```

### 目标架构 (v1.1.0)

```
scripts/
├── Kbuild.include          290 行
├── Makefile.compiler       100 行  ← 新建：编译器检测
├── Makefile.lib            198 行
├── Makefile.build          250 行  ← 重构：核心编译规则
├── Makefile.lib.build      150 行  ← 新建：库构建
├── Makefile.app.build       80 行  ← 新建：应用构建
├── Makefile.mod.build      100 行  ← 新建：模块构建
├── Makefile.host            38 行
├── Makefile.clean           52 行
└── Makefile.install        188 行
```

---

## 实施阶段

| 阶段 | 任务 | 状态 | 预计时间 |
|------|------|------|----------|
| Phase 1 | 准备工作 | ✅ 完成 | 1-2 天 |
| Phase 2 | 创建 Makefile.compiler | ⏳ 待开始 | 2-3 天 |
| Phase 3 | 创建 Makefile.lib.build | ⏳ 待开始 | 3-4 天 |
| Phase 4 | 创建 Makefile.app.build | ⏳ 待开始 | 2-3 天 |
| Phase 5 | 创建 Makefile.mod.build | ⏳ 待开始 | 1-2 天 |
| Phase 6 | 重构 Makefile.build | ⏳ 待开始 | 2-3 天 |
| Phase 7 | 测试和验证 | ⏳ 待开始 | 3-4 天 |
| Phase 8 | 文档更新 | ⏳ 待开始 | 1-2 天 |
| Phase 9 | 代码审查和合并 | ⏳ 待开始 | 1-2 天 |

---

## 快速开始

### 查看重构计划

```bash
# 查看详细计划
cat docs/refactor/BUILD_SYSTEM_REFACTOR_PLAN.md

# 查看检查清单
cat docs/refactor/REFACTOR_CHECKLIST.md
```

### 切换到重构分支

```bash
git checkout feature/refactor-build-system
```

### 创建测试基准

```bash
# 记录当前构建时间
make mrproper
time make ccm_h200_am625_debug_defconfig
time make -j$(nproc)

# 记录构建产物
find .staging -type f > /tmp/ems-before-refactor.txt
md5sum .staging/bin/* .staging/lib/*.a > /tmp/ems-before-md5.txt
```

---

## 验收标准

### 功能标准
- ✅ 所有现有功能正常工作
- ✅ 构建产物与重构前一致
- ✅ 构建时间无明显变化 (±5%)
- ✅ 支持所有现有构建选项

### 质量标准
- ✅ 代码结构清晰，职责单一
- ✅ 注释完整，易于理解
- ✅ 符合 Linux 内核编码风格
- ✅ 文档完整准确

---

## 参考资料

### Linux 内核构建系统
- 源码路径: `/home/wanguo/AM62x/ti-linux-kernel/scripts/`
- 关键文件:
  - `Makefile.build` (591 行)
  - `Makefile.compiler` (93 行)
  - `Makefile.lib` (491 行)

### EMS 项目文档
- [构建系统详解](../BUILD_SYSTEM.md)
- [构建指南](../BUILD_GUIDE.md)
- [架构设计](../ARCHITECTURE.md)

---

## 联系方式

- **负责人**: wanguo
- **分支**: `feature/refactor-build-system`
- **问题反馈**: 在分支上创建 commit 或 issue

---

**最后更新**: 2026-05-22  
**文档版本**: v1.0
