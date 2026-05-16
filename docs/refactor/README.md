# EMS 架构文档说明

## 文档列表

### 1. ARCHITECTURE_REVIEW.md
- **类型**：架构审查报告
- **内容**：对v3.0单进程方案的审查，指出过度设计问题，建议简化

### 2. EMS_Architecture_Refactor_v3.0_SingleProcess.md
- **架构**：单进程多线程
- **状态**：历史版本（不推荐用于太空环境）
- **特点**：看门狗线程、SPSC无锁队列、ACL配置层

### 3. EMS_Architecture_Refactor_v3.1_MultiProcess_Final.md
- **架构**：多进程（Supervisor + 3个业务进程）
- **状态**：✅ 最终推荐方案
- **特点**：
  - 进程级故障隔离
  - 辐射容错
  - 自动恢复
  - 降级运行
  - OSAL IPC集成
  - ACL配置层
- **适用**：太空环境、高可靠性系统

## 版本演进

```
v3.0 (单进程多线程)
  ↓
  ├─→ ARCHITECTURE_REVIEW.md (审查报告：建议简化)
  ↓
v3.1 (多进程架构) ← 最终方案
```

## 推荐使用

**对于太空环境项目**：使用 `EMS_Architecture_Refactor_v3.1_MultiProcess_Final.md`

**对于普通嵌入式项目**：参考 `EMS_Architecture_Refactor_v3.0_SingleProcess.md`（但需简化）
