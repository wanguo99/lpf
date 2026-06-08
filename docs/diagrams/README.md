# EMS 架构图集

本目录包含 EMS 系统的所有架构图，每张图提供 PNG（位图）和 SVG（矢量图）两种格式。

## 图表列表

### 1. 系统整体架构

**文件名**：`01_system_architecture`

![系统架构](01_system_architecture.png)

**说明**：展示 EMS 从应用层到系统层的完整架构，包括所有核心模块及其关系。

**关键内容**：
- 7 个核心模块：OSAL、HAL、PConfig、PRL、PDL、AConfig、Products
- 模块间的依赖关系和数据流向
- PRL 和 PConfig 作为独立模块的定位
- 配置查询和协议编解码的调用关系

---

### 2. 模块依赖关系

**文件名**：`02_module_dependencies`

![模块依赖](02_module_dependencies.png)

**说明**：清晰展示各模块之间的依赖关系和构建顺序。

**关键内容**：
- 自底向上的依赖关系
- 构建顺序（OSAL → HAL/PConfig → PRL → PDL → AConfig → Products）
- 依赖规则（Core 不依赖 Products，上层依赖下层）

---

### 3. 数据流向

**文件名**：`03_data_flow`

![数据流向](03_data_flow.png)

**说明**：展示遥控命令和遥测数据的完整处理流程。

**关键内容**：
- **遥控命令流程**：应用层 → AConfig → PDL → PRL → HAL → 硬件
- **遥测数据流程**：
  - 缓存型：共享内存直接读取（< 10μs）
  - 实时型：通过 PDL/PRL/HAL 查询硬件（< 1ms）
- 性能指标和超时降级机制

---

### 4. PRL 协议格式

**文件名**：`04_prl_protocol`

![PRL 协议](04_prl_protocol.png)

**说明**：详细展示 PRL 协议层的协议格式和编解码流程。

**关键内容**：
- 20 字节协议头结构
- 支持的设备类型（MCU、CCM、PMC、GSC、POWER）
- 消息类型示例
- 编码/解码流程
- 零拷贝和 CRC 校验机制

---

### 5. 构建系统

**文件名**：`05_build_system`

![构建系统](05_build_system.png)

**说明**：展示 Kconfig + CMake 混合构建系统的工作流程。

**关键内容**：
- Kconfig 配置流程（defconfig → .config → menuconfig）
- CMake 构建流程（读取配置 → 生成变量 → 条件编译）
- 配置选项示例
- 构建系统特性（图形化配置、功能裁剪、并行编译）

---

### 6. OSAL 内部架构

**文件名**：`06_osal_internal`

![OSAL 内部](06_osal_internal.png)

**说明**：展示 OSAL 模块的内部组织结构。

**关键内容**：
- IPC 模块（任务、队列、互斥锁、信号量、条件变量、原子操作）
- SYS 模块（文件、时钟、信号、Select、环境变量、时间）
- NET 模块（Socket、Termios）
- LIB 模块（字符串、堆内存、错误处理）
- UTIL 模块（日志、版本）
- POSIX 实现层
- 关键特性（零初始化、优雅退出、引用计数、死锁检测）

---

### 7. AConfig 映射机制

**文件名**：`07_aconfig_mapping`

![AConfig 映射](07_aconfig_mapping.png)

**说明**：展示 AConfig 业务配置层的 O(1) 映射机制。

**关键内容**：
- 业务功能枚举（遥控、遥测）
- O(1) 查找表设计
- 设备映射关系
- 配置结构定义
- 遥测缓存失效机制
- 性能特性（~5ns 查询时间）

---

## 文件格式说明

每张图提供两种格式：

- **PNG 格式**（`.png`）：
  - 位图格式，150 DPI 高分辨率
  - 适合在文档、演示文稿中嵌入
  - 文件大小：200-500KB

- **SVG 格式**（`.svg`）：
  - 矢量图格式，无损缩放
  - 适合打印和高分辨率显示
  - 可在浏览器中直接查看
  - 文件大小：10-30KB

## 源文件

所有架构图的源文件（`.dot` 文件）使用 Graphviz DOT 语言编写，也包含在本目录中。

如需修改架构图：
1. 编辑对应的 `.dot` 文件
2. 运行生成命令：
   ```bash
   dot -Tpng <file>.dot -o <file>.png -Gdpi=150
   dot -Tsvg <file>.dot -o <file>.svg
   ```

## 快速查看

推荐使用以下方式查看架构图：

- **图像查看器**：直接打开 PNG 文件
- **浏览器**：打开 SVG 文件可缩放查看细节
- **文档阅读**：查看 [ARCHITECTURE.md](../ARCHITECTURE.md) 获取带说明的完整架构文档

## 工具要求

生成架构图需要安装 Graphviz：

```bash
# Ubuntu/Debian
sudo apt-get install graphviz

# macOS
brew install graphviz

# 验证安装
dot -V
```

---

**生成日期**：2026-06-08  
**维护者**：wanguo  
**工具**：Graphviz 2.x
