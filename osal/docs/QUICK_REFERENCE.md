# OSAL 优化计划 - 快速参考

## 🚀 常用命令

```bash
# 进入工作目录
cd /home/wanguo/EMS/osal/docs

# 查看当前进度
./update_progress.sh status

# 开始任务
./update_progress.sh start T1.1.1

# 一键提交（推荐）
./update_progress.sh commit T1.1.1 "新增：T1.1.1 - 任务描述"

# 完成任务
./update_progress.sh done T1.1.1

# 列出任务
./update_progress.sh list 1    # 阶段 1
./update_progress.sh list       # 所有任务

# 生成报告
./update_progress.sh report
```

## 📋 任务优先级

### P0 - 核心基础（14 项）
- 类型系统完善（T1.1.x）
- 错误码体系重构（T1.2.x）
- 平台检测增强（T2.1.x）
- 内存池实现（T3.1.x）

### P1 - 重要功能（22 项）
- DMA 内存管理（T3.2.x）
- 缓存操作（T3.3.x）
- 同步原语（T4.x.x）
- 时间系统（T5.x.x）
- 文件与网络（T6.x.x）

### P2 - 增强功能（20 项）
- 读写锁（T4.3.x）
- 中断处理（T7.2.x）
- 日志系统（T8.1.x）
- 配置管理（T8.2.x）

## 🎯 推荐执行顺序

### 第 1 周：类型系统与错误码
```bash
./update_progress.sh start T1.1.1  # 硬件寄存器类型
./update_progress.sh start T1.1.2  # 原子类型别名
./update_progress.sh start T1.1.3  # 时间类型
./update_progress.sh start T1.2.1  # 分层错误码结构
./update_progress.sh start T1.2.2  # 模块 ID 枚举
```

### 第 2 周：平台检测
```bash
./update_progress.sh start T2.1.1  # 创建 osal_platform.h
./update_progress.sh start T2.1.2  # 操作系统检测
./update_progress.sh start T2.1.3  # CPU 架构检测
```

### 第 3-4 周：内存管理
```bash
./update_progress.sh start T3.1.1  # 内存池数据结构
./update_progress.sh start T3.1.2  # MemPoolCreate
./update_progress.sh start T3.1.3  # MemPoolAlloc
./update_progress.sh start T3.2.1  # DmaAlloc
```

## 📊 进度检查点

```bash
# 每天结束时
./update_progress.sh status

# 每周结束时
./update_progress.sh report

# 查看本周提交
git log --since="1 week ago" --grep="任务: T" --oneline
```

## 🔧 Git 提交模板

```bash
# 复制到剪贴板使用
新增：T1.1.1 - 补充硬件寄存器类型

添加 osal_reg32_t 和 osal_reg16_t 类型定义，
用于硬件寄存器访问，确保 volatile 语义。

任务: T1.1.1
阶段: 1.1
优先级: P0
```

## 📁 关键文件位置

```
/home/wanguo/EMS/
├── osal/
│   ├── include/
│   │   ├── osal_types.h          # 类型定义
│   │   ├── osal_error.h          # 错误码
│   │   ├── osal_platform.h       # 平台检测（待创建）
│   │   └── ...
│   ├── src/posix/                # POSIX 实现
│   ├── docs/
│   │   ├── OPTIMIZATION_PLAN.md  # 详细计划
│   │   ├── README_OPTIMIZATION.md # 使用指南
│   │   ├── update_progress.sh    # 进度脚本
│   │   └── QUICK_REFERENCE.md    # 本文件
│   └── ...
└── ...
```

## 🧪 测试命令

```bash
# 编译测试
cd /home/wanguo/EMS
cmake --preset debug && cmake --build --preset debug

# 运行测试
./build/debug/bin/unit-test -i              # 交互式
./build/debug/bin/unit-test -L OSAL         # OSAL 层测试
./build/debug/bin/unit-test -m test_osal_types  # 特定模块

# 多架构编译
cmake --preset arm32 && cmake --build build/arm32
cmake --preset arm64 && cmake --build build/arm64
cmake --preset riscv64 && cmake --build build/riscv64
```

## 💡 快速技巧

### 1. 批量开始多个任务
```bash
for task in T1.1.1 T1.1.2 T1.1.3; do
    ./update_progress.sh start $task
done
```

### 2. 查看某个阶段的进度
```bash
./update_progress.sh list 1 | grep "✅ DONE"
```

### 3. 查看最近的任务提交
```bash
git log --grep="任务: T" --oneline -10
```

### 4. 恢复误操作
```bash
# 查看备份
ls .progress_backup/

# 恢复最新备份
cp .progress_backup/OPTIMIZATION_PLAN_*.md OPTIMIZATION_PLAN.md
```

## 📞 快速帮助

```bash
# 查看完整帮助
./update_progress.sh help

# 查看详细文档
cat README_OPTIMIZATION.md

# 查看完整计划
cat OPTIMIZATION_PLAN.md
```

---

**提示**: 将此文件添加到书签，方便随时查阅！
