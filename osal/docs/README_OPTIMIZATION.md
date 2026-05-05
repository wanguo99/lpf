# OSAL 优化计划使用指南

本目录包含 OSAL 层的优化计划和进度管理工具。

## 📁 文件说明

- **OPTIMIZATION_PLAN.md** - 详细的优化计划文档（56 项任务）
- **update_progress.sh** - 进度管理脚本（自动化工具）
- **README_OPTIMIZATION.md** - 本文件（使用指南）

---

## 🚀 快速开始

### 1. 查看当前进度

```bash
cd osal/docs
./update_progress.sh status
```

输出示例：
```
=== OSAL 优化计划进度统计 ===

总任务数: 56
已完成:   0  (0%)
进行中:   0
待开始:   56
已阻塞:   0
已取消:   0

=== 按优先级统计 ===

P0 (核心基础):  0 / 14
P1 (重要功能):  0 / 22
P2 (增强功能):  0 / 20

=== 进度条 ===

[--------------------------------------------------] 0%
```

### 2. 开始一项任务

```bash
# 开始任务 T1.1.1（补充硬件寄存器类型）
./update_progress.sh start T1.1.1
```

这会：
- 将任务状态从 `⬜ TODO` 改为 `🔄 IN PROGRESS`
- 更新计划文档的"最后更新"日期
- 自动提交计划文档变更到 Git

### 3. 完成任务（推荐工作流）

#### 方式 A：一键提交（推荐）

```bash
# 完成代码实现后，一键提交代码并更新计划
./update_progress.sh commit T1.1.1 "新增：T1.1.1 - 补充硬件寄存器类型

添加 osal_reg32_t 和 osal_reg16_t 类型定义，
用于硬件寄存器访问，确保 volatile 语义。

任务: T1.1.1
阶段: 1.1
优先级: P0"
```

这会：
1. 提交所有代码变更到 Git
2. 自动获取提交哈希
3. 将任务状态改为 `✅ DONE`
4. 在计划文档中记录提交哈希
5. 提交计划文档变更

#### 方式 B：手动提交

```bash
# 1. 先手动提交代码
git add osal/include/osal_types.h
git commit -m "新增：T1.1.1 - 补充硬件寄存器类型

添加 osal_reg32_t 和 osal_reg16_t 类型定义。

任务: T1.1.1
阶段: 1.1
优先级: P0"

# 2. 标记任务完成（自动获取最新提交哈希）
./update_progress.sh done T1.1.1

# 或者手动指定提交哈希
./update_progress.sh done T1.1.1 a1b2c3d
```

### 4. 查看任务列表

```bash
# 查看所有任务
./update_progress.sh list

# 查看阶段 1 的任务
./update_progress.sh list 1

# 查看阶段 3 的任务
./update_progress.sh list 3
```

### 5. 生成进度报告

```bash
./update_progress.sh report
```

这会生成一个详细的进度报告文件：`PROGRESS_REPORT_YYYYMMDD.md`

---

## 📖 完整工作流示例

### 场景：实现任务 T1.1.1（补充硬件寄存器类型）

```bash
# 1. 查看当前进度
./update_progress.sh status

# 2. 开始任务
./update_progress.sh start T1.1.1

# 3. 编辑代码
vim ../../include/osal_types.h

# 添加以下内容：
# typedef volatile uint32_t osal_reg32_t;
# typedef volatile uint16_t osal_reg16_t;

# 4. 编译测试
cd ../..
cmake --preset debug && cmake --build --preset debug
./build/debug/bin/unit-test -m test_osal_types

# 5. 一键提交代码并更新计划
cd osal/docs
./update_progress.sh commit T1.1.1 "新增：T1.1.1 - 补充硬件寄存器类型

添加 osal_reg32_t 和 osal_reg16_t 类型定义，
用于硬件寄存器访问，确保 volatile 语义。

任务: T1.1.1
阶段: 1.1
优先级: P0"

# 6. 查看更新后的进度
./update_progress.sh status
```

---

## 🔧 高级用法

### 阻塞任务

如果某个任务因为依赖其他任务而无法继续：

```bash
./update_progress.sh block T3.2.1 "等待 T3.1.1 内存池实现完成"
```

### 取消任务

如果某个任务不再需要：

```bash
./update_progress.sh cancel T8.1.3 "需求变更，不再需要循环缓冲区日志"
```

### 查看任务相关的 Git 提交

```bash
# 查看某个任务的所有提交
git log --grep="任务: T1.1.1" --oneline

# 查看某个阶段的所有提交
git log --grep="阶段: 1.1" --oneline

# 查看所有优化相关的提交
git log --grep="任务: T" --oneline
```

### 恢复计划文档

如果误操作，可以从备份恢复：

```bash
# 备份文件位于
ls .progress_backup/

# 恢复最新备份
cp .progress_backup/OPTIMIZATION_PLAN_20260505_143022.md OPTIMIZATION_PLAN.md
```

---

## 📊 任务优先级说明

### P0 - 核心基础（必须完成）

这些任务是 OSAL 的核心功能，必须在第一阶段完成：

- 类型系统完善
- 错误码体系重构
- 平台检测增强
- 内存池实现

**建议顺序**：按任务 ID 顺序执行（T1.1.1 → T1.1.2 → ...）

### P1 - 重要功能（高优先级）

这些任务对系统功能和性能有重要影响：

- DMA 内存管理
- 同步原语（信号量/条件变量）
- 时间系统优化
- 文件系统与网络抽象

**建议顺序**：完成 P0 任务后，按阶段顺序执行

### P2 - 增强功能（可选）

这些任务提供额外的功能和便利性：

- 读写锁
- 中断处理抽象
- 日志系统重构
- 配置管理

**建议顺序**：根据实际需求选择性实现

---

## 🎯 里程碑检查清单

### Milestone 1: 核心基础完成（Week 2）

```bash
# 检查阶段 1 和阶段 2 的任务是否全部完成
./update_progress.sh list 1
./update_progress.sh list 2

# 验证：编译所有平台
cmake --preset release && cmake --build --preset release
cmake --preset arm32 && cmake --build build/arm32
cmake --preset arm64 && cmake --build build/arm64
cmake --preset riscv64 && cmake --build build/riscv64
```

### Milestone 2: 内存与同步（Week 4）

```bash
# 检查阶段 3 和阶段 4 的任务
./update_progress.sh list 3
./update_progress.sh list 4

# 验证：运行内存和同步测试
./build/release/bin/unit-test -L OSAL
```

### Milestone 3: 时间与网络（Week 6）

```bash
# 检查阶段 5 和阶段 6 的任务
./update_progress.sh list 5
./update_progress.sh list 6

# 验证：运行时间和网络测试
./build/release/bin/unit-test -m test_osal_time
./build/release/bin/unit-test -m test_osal_socket
```

---

## 📝 Git 提交规范

### 提交消息模板

```
<类型>：<任务ID> - <简短描述>

<详细说明>

任务: <任务ID>
阶段: <阶段编号>
优先级: <P0/P1/P2>
```

### 类型标签

- `新增` (feat): 新功能
- `重构` (refactor): 代码重构
- `修复` (fix): Bug 修复
- `测试` (test): 测试相关
- `文档` (docs): 文档更新
- `优化` (optimize): 性能优化

### 示例

```bash
git commit -m "新增：T1.1.1 - 补充硬件寄存器类型

添加 osal_reg32_t 和 osal_reg16_t 类型定义，
用于硬件寄存器访问，确保 volatile 语义。

任务: T1.1.1
阶段: 1.1
优先级: P0"
```

---

## 🔍 常见问题

### Q1: 如何查看某个任务的详细信息？

```bash
# 在计划文档中搜索任务 ID
grep -A 5 "T1.1.1" OPTIMIZATION_PLAN.md
```

### Q2: 如何修改任务的优先级或预计工时？

直接编辑 `OPTIMIZATION_PLAN.md` 文件，然后提交变更：

```bash
vim OPTIMIZATION_PLAN.md
git add OPTIMIZATION_PLAN.md
git commit -m "文档：调整任务 T1.1.1 的优先级"
```

### Q3: 如何添加新任务？

1. 在 `OPTIMIZATION_PLAN.md` 中找到对应的阶段
2. 添加新的任务行（参考现有格式）
3. 更新进度统计
4. 提交变更

### Q4: 脚本执行失败怎么办？

```bash
# 检查脚本权限
ls -l update_progress.sh

# 如果没有执行权限，添加权限
chmod +x update_progress.sh

# 查看详细错误信息
bash -x update_progress.sh status
```

### Q5: 如何在多人协作时避免冲突？

1. 每次开始工作前先拉取最新代码：
   ```bash
   git pull origin master
   ```

2. 使用脚本自动提交，避免手动编辑冲突：
   ```bash
   ./update_progress.sh commit T1.1.1 "提交消息"
   ```

3. 如果发生冲突，优先保留最新的进度：
   ```bash
   git checkout --theirs OPTIMIZATION_PLAN.md
   git add OPTIMIZATION_PLAN.md
   git commit
   ```

---

## 📚 相关文档

- [OPTIMIZATION_PLAN.md](OPTIMIZATION_PLAN.md) - 详细优化计划
- [../../README.md](../../README.md) - OSAL 模块说明
- [ARCHITECTURE.md](ARCHITECTURE.md) - OSAL 架构设计
- [API_REFERENCE.md](API_REFERENCE.md) - API 参考手册

---

## 📞 技术支持

如有问题，请：

1. 查看本文档的"常见问题"部分
2. 查看 `OPTIMIZATION_PLAN.md` 中的详细说明
3. 查看 Git 提交历史：`git log --grep="任务: T"`

---

**最后更新**: 2026-05-05  
**版本**: v1.0
