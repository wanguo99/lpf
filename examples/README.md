# EMS SDK 示例代码

本目录包含 EMS SDK 的示例代码，帮助你快速学习和使用各个模块。

---

## 📚 示例列表

### 基础示例

| 示例 | 说明 | 难度 | 涉及模块 |
|------|------|------|----------|
| [01_hello_world](01_hello_world/) | 最简单的示例 | ⭐ | OSAL |
| [02_osal_thread](02_osal_thread/) | 线程和同步 | ⭐⭐ | OSAL |
| [03_hal_can](03_hal_can/) | CAN 总线通信 | ⭐⭐⭐ | HAL, OSAL |
| [04_hal_uart](04_hal_uart/) | 串口通信 | ⭐⭐ | HAL, OSAL |

### 进阶示例（即将添加）

- 05_pdl_satellite - 卫星通信
- 06_complete_app - 完整应用示例
- 07_multi_thread - 多线程应用
- 08_ipc_demo - 进程间通信

---

## 🚀 快速开始

### 编译所有示例

```bash
# 配置（启用示例）
python3 build.py menuconfig
# 进入 Examples → Enable all examples

# 编译
python3 build.py build

# 查看生成的示例
ls _build/bin/example_*
```

### 运行单个示例

```bash
# 运行 hello_world
./_build/bin/example_hello_world

# 运行 osal_thread
./_build/bin/example_osal_thread

# 运行 hal_can（需要 CAN 设备）
./_build/bin/example_hal_can can0
```

---

## 📖 学习路径

### 第一步：Hello World
从最简单的示例开始，了解项目结构和编译流程。

👉 [01_hello_world](01_hello_world/)

### 第二步：OSAL 线程
学习如何使用 OSAL 创建线程、互斥锁和信号量。

👉 [02_osal_thread](02_osal_thread/)

### 第三步：HAL 驱动
学习如何使用 HAL 访问硬件设备。

👉 [03_hal_can](03_hal_can/) 或 [04_hal_uart](04_hal_uart/)

### 第四步：完整应用
查看 `products/sample/` 和 `products/ccm/` 中的完整应用。

---

## 🔧 示例结构

每个示例都包含：

```
example_name/
├── src/
│   └── main.c          # 主程序
├── CMakeLists.txt      # 构建配置
├── Kconfig             # 配置选项
└── README.md           # 说明文档
```

---

## 💡 使用技巧

### 1. 复制示例作为起点

```bash
# 复制示例创建新项目
cp -r examples/01_hello_world my_project
cd my_project
# 修改代码...
```

### 2. 单独编译示例

```bash
# 进入示例目录
cd examples/01_hello_world

# 使用 CMake 单独编译
mkdir build
cd build
cmake ..
make
./example_hello_world
```

### 3. 调试示例

```bash
# 使用 gdb 调试
gdb ./_build/bin/example_hello_world
(gdb) break main
(gdb) run
(gdb) next
```

---

## 📝 贡献示例

欢迎贡献新的示例！

### 示例要求

1. **代码简洁**: 专注于演示特定功能
2. **注释清晰**: 解释关键代码
3. **可运行**: 确保示例能够编译和运行
4. **文档完整**: 包含 README.md 说明

### 提交流程

1. 在 `examples/` 下创建新目录
2. 编写示例代码和文档
3. 测试编译和运行
4. 提交 Pull Request

---

## 🐛 问题反馈

如果示例代码有问题或建议，请：

1. 查看 [故障排除指南](../docs/TROUBLESHOOTING.md)
2. 提交 [Issue](https://github.com/wanguo99/EMS/issues)

---

## 📚 相关文档

- [新手入门教程](../docs/GETTING_STARTED.md)
- [开发者指南](../docs/DEVELOPER_GUIDE.md)
- [API 参考](../core/*/README.md)

---

**提示**: 建议按顺序学习示例，从简单到复杂。

**更新**: 示例代码持续更新中，敬请期待更多示例！
