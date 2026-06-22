# 清理旧框架 - 进度报告

## 执行摘要

**日期**: 2026-06-22  
**提交**: ca5e1a7  
**状态**: 部分完成（约 70%）  
**编译状态**: ❌ 未通过（依赖问题）

---

## 已完成的清理工作

### 1. 条件编译移除 ✅

删除了所有 `CONFIG_PDM_NEW_BUS` 条件编译：
- `kernel/pdm-core/core/pdm_core.c` - 只保留新总线初始化
- `kernel/pdm-core/peripheral/mcu/pdm_mcu_service.c` - 移除条件编译
- `kernel/pdm-core/peripheral/led/pdm_led_service.c` - 移除条件编译
- `kernel/pdm-core/Config.in` - 删除配置选项

### 2. 函数名清理 ✅

移除 `_new` 后缀：
- `pdm_mcu_probe_new()` → `pdm_mcu_probe()`
- `pdm_mcu_remove_new()` → `pdm_mcu_remove()`
- `pdm_mcu_driver_init_new()` → `pdm_mcu_driver_init()`
- `pdm_mcu_driver_exit_new()` → `pdm_mcu_driver_exit()`
- LED 驱动同样处理

### 3. 旧代码删除 ✅

删除的代码：
- 旧的 `pdm_mcu_probe(const pdm_device_t *)` 函数
- 旧的 `pdm_led_probe(const pdm_device_t *)` 函数
- `pdm_runtime_service_register()` 调用
- `g_lpf_mcu_driver` 和 `g_lpf_led_driver` 静态结构
- 旧的模块初始化/退出函数

### 4. 头文件清理 ✅

- **删除**: `kernel/include/pdm/core/pdm_driver.h` - 旧伪总线驱动接口
- **简化**: `kernel/include/pdm/core/pdm_core.h` - 移除旧 API 声明，保留类型定义
- **移除引用**: 删除所有对 `pdm_driver.h` 的 include

### 5. 核心模块简化 ✅

`kernel/pdm-core/core/pdm_core.c` 重写为：
- 只初始化 Linux bus_type（`pdm_bus_init()`）
- 移除旧伪总线初始化（`pdm_core_init()`）
- 简化为 60 行代码
- 清晰的模块结构

### 6. 构建系统更新 ✅

- 删除 `CONFIG_PDM_NEW_BUS` 配置选项
- 新总线成为默认且唯一实现

---

## 剩余问题

### ❌ 问题 1: pdm_chrdev.c 依赖旧 API

**文件**: `kernel/pdm-core/core/pdm_chrdev.c`

**问题**: 使用了已删除的旧伪总线 API：
```c
pdm_device_get()           // 旧的设备查找
pdm_device_put()           // 旧的引用管理
pdm_device_get_info()      // 旧的信息获取
pdm_device_record_error()  // 旧的错误记录
pdm_device_record_recovery()
device->driver->name       // 访问旧的 pdm_driver_t 结构
```

**影响**: 编译失败

**解决方案**:
1. **选项 A**: 禁用 pdm_chrdev.c，驱动直接管理字符设备
2. **选项 B**: 重写 pdm_chrdev.c 适配新总线 API
3. **选项 C**: 创建兼容层封装新 API

### ❌ 问题 2: runtime 代码依赖

**文件**: 
- `kernel/pdm-core/runtime/pdm_runtime.c`
- `kernel/pdm-core/runtime/pdm_runtime_config.c`

**问题**: 可能仍然使用 `pdm_device_register()` 等旧 API

**状态**: 未验证

### ❌ 问题 3: 配置驱动依赖

**文件**:
- `kernel/pdm-core/peripheral/mcu/pdm_mcu_config_driver.c`
- `kernel/pdm-core/peripheral/led/pdm_led_config_driver.c`

**问题**: 使用 `pdm_device_register()` 从静态配置创建设备

**解决方案**: 删除这些文件（已转向 Device Tree）

### ⚠️ 问题 4: selftest 模块

**文件**: `kernel/pdm-core/peripheral/selftest/pdm_dummy_service_selftest.c`

**问题**: 使用旧的伪总线 API 进行测试

**解决方案**: 重写或删除

---

## 推荐的完成方案

### 方案 A: 激进清理（推荐）

**优点**: 彻底，代码最干净  
**缺点**: 工作量大

**步骤**:
1. 删除 `pdm_chrdev.c`（不再编译）
2. MCU/LED 驱动直接使用 `misc_register()` 或 `cdev`
3. 删除所有 `*_config_driver.c` 文件
4. 删除或禁用 selftest 模块
5. 清理 runtime 代码中的旧 API 调用

**预计时间**: 4-6 小时

### 方案 B: 最小修复

**优点**: 快速通过编译  
**缺点**: 保留部分旧代码

**步骤**:
1. 暂时禁用 `pdm_chrdev.c` 编译
2. 在 MCU/LED chrdev 文件中临时屏蔽对 `pdm_chrdev_register_lpf_device()` 的调用
3. 添加空实现或 stub 函数
4. 先让代码编译通过

**预计时间**: 1-2 小时

---

## 代码统计

### 已删除的代码

| 文件 | 删除行数 |
|------|----------|
| pdm_mcu_service.c | ~150 行 |
| pdm_led_service.c | ~120 行 |
| pdm_driver.h | ~100 行 |
| pdm_core.c | ~1000 行 |
| **总计** | **~1370 行** |

### 简化后的代码

| 文件 | 原行数 | 现行数 | 减少 |
|------|--------|--------|------|
| pdm_core.c | 1092 | 61 | 94% |
| pdm_core.h | 71 | 45 | 37% |

---

## 文件清单

### 已修改
- `kernel/pdm-core/core/pdm_core.c` - 完全重写
- `kernel/pdm-core/peripheral/mcu/pdm_mcu_service.c` - 清理旧代码
- `kernel/pdm-core/peripheral/led/pdm_led_service.c` - 清理旧代码
- `kernel/include/pdm/core/pdm_core.h` - 简化
- `kernel/pdm-core/Config.in` - 删除选项

### 已删除
- `kernel/include/pdm/core/pdm_driver.h`

### 待处理
- `kernel/pdm-core/core/pdm_chrdev.c` - 需要重写或删除
- `kernel/pdm-core/peripheral/mcu/pdm_mcu_config_driver.c` - 建议删除
- `kernel/pdm-core/peripheral/led/pdm_led_config_driver.c` - 建议删除
- `kernel/pdm-core/peripheral/selftest/*.c` - 需要更新

---

## 编译错误总结

```
错误类型: 未定义的函数/类型
- pdm_device_get()
- pdm_device_put()
- pdm_device_get_info()
- pdm_device_record_error()
- pdm_device_record_recovery()
- struct pdm_driver (旧类型)

文件: kernel/pdm-core/core/pdm_chrdev.c
```

---

## 下一步行动

### 立即可做（方案 B）

1. **临时禁用 pdm_chrdev.c**
```bash
# 编辑 kernel/pdm-core/core/Makefile
# 注释掉: pdm_core-y += pdm-core/core/pdm_chrdev.o
```

2. **修改 MCU/LED chrdev 文件**
暂时屏蔽对 pdm_chrdev 的调用，直接使用底层 API

3. **验证编译**
```bash
make ARCH=x86_64
```

### 完整方案（方案 A）

需要重新设计字符设备管理，或者完全移除 pdm_chrdev 抽象层。

---

## 总结

**进度**: 70% 完成

**已完成**:
- ✅ 删除条件编译
- ✅ 清理函数名
- ✅ 删除旧伪总线代码
- ✅ 简化核心模块
- ✅ 清理头文件

**待完成**:
- ❌ 修复 pdm_chrdev.c 依赖
- ❌ 清理 runtime 代码
- ❌ 删除配置驱动文件
- ❌ 通过编译验证

**建议**: 采用方案 B 先让代码编译通过，后续再逐步完成方案 A 的彻底清理。

---

**报告生成**: 2026-06-22  
**提交**: ca5e1a7  
**分支**: refactor/lpf-to-pdm-bus
