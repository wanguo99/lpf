# Hello World 示例

最简单的 EMS SDK 示例，演示如何创建一个基本的应用程序。

---

## 📖 学习目标

- 了解 EMS SDK 的基本结构
- 学习如何使用 OSAL 版本信息 API
- 掌握基本的编译和运行流程

---

## 🚀 快速开始

### 编译运行

```bash
# 从项目根目录
python3 build.py config sample_default
python3 build.py build
./_build/bin/example_hello_world
```

### 预期输出

```
=================================
  Hello, EMS SDK!
=================================

=== EMS Version Information ===
Version:      1.0.0
Build Date:   2026-05-28
Build Time:   18:00:00
Git Commit:   72aba73

Hello from EMS SDK!
This is the simplest example.
```

---

## 📝 代码说明

### main.c

```c
#include <stdio.h>
#include "osal/osal.h"

int main(int argc, char *argv[])
{
    // 打印欢迎信息
    printf("=================================\n");
    printf("  Hello, EMS SDK!\n");
    printf("=================================\n\n");

    // 使用 OSAL API 打印版本信息
    print_version_info();

    // 你的代码
    printf("\nHello from EMS SDK!\n");
    printf("This is the simplest example.\n");

    return 0;
}
```

**关键点**:
1. 包含 `osal.h` 头文件
2. 使用 `print_version_info()` 打印版本信息
3. 标准的 C 程序结构

---

## 🔧 修改示例

### 添加命令行参数

```c
int main(int argc, char *argv[])
{
    if (argc > 1) {
        printf("Hello, %s!\n", argv[1]);
    } else {
        printf("Hello, World!\n");
    }
    return 0;
}
```

运行：
```bash
./_build/bin/example_hello_world Alice
# 输出: Hello, Alice!
```

### 添加循环

```c
int main(int argc, char *argv[])
{
    for (int i = 0; i < 5; i++) {
        printf("Count: %d\n", i);
    }
    return 0;
}
```

---

## 📚 下一步

学习完本示例后，继续学习：

- [02_osal_thread](../02_osal_thread/) - 学习线程和同步
- [03_hal_can](../03_hal_can/) - 学习硬件驱动

---

## 💡 小贴士

1. **修改代码**: 直接修改 `src/main.c`，然后重新编译
2. **查看 API**: 查看 `core/osal/include/osal.h` 了解更多 API
3. **调试**: 使用 `gdb ./_build/bin/example_hello_world` 调试

---

**难度**: ⭐ (非常简单)  
**时间**: 5 分钟
