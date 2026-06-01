# EMS 架构优化使用指南

## 优化内容总结

### 1. 修复的问题

#### 1.1 PDL_MCU 的 PRL 依赖问题（严重）
**问题**：只有 `CONFIG_PDL_CCM_SUPPORT=y` 时才链接 PRL，但 PDL_MCU 也需要 PRL。

**修复**：
```cmake
# core/pdl/CMakeLists.txt
if(CONFIG_PDL_CCM_SUPPORT OR CONFIG_PDL_MCU_SUPPORT)
    list(APPEND ADD_REQUIREMENTS prl)
endif()
```

#### 1.2 冗余依赖（严重）
**问题**：
- PCL 声明依赖 HAL，但从不使用
- ACL 声明依赖 PDL，但从不使用

**修复**：
```cmake
# core/pcl/CMakeLists.txt
list(APPEND ADD_REQUIREMENTS osal pdl)  # 移除 hal

# core/acl/CMakeLists.txt
list(APPEND ADD_REQUIREMENTS osal)  # 移除 pdl
```

### 2. 新增的便捷接口

#### 2.1 PDL 初始化便捷接口

**旧方式**（需要手动查询 PCL）：
```c
// 应用层代码
const pcl_platform_config_t *platform = PCL_GetBoard();
const pcl_mcu_entry_t *entry = PCL_HW_FindMCU(platform, "power_mcu");
if (entry) {
    PDL_MCU_Init(&entry->config, &handle);
}
```

**新方式**（推荐）：
```c
// 应用层代码 - 更简洁
int32_t ret = PDL_MCU_InitByName("power_mcu", &handle);
if (ret == OSAL_SUCCESS) {
    // 初始化成功
}
```

**可用的便捷接口**：
- `PDL_MCU_InitByName(const char *device_name, pdl_mcu_handle_t *handle)`
- `PDL_BMC_InitByName(const char *device_name, pdl_bmc_handle_t *handle)`

#### 2.2 ACL 配置改进

**旧方式**（使用逻辑索引）：
```c
[TM_POWER_VOLTAGE] = {
    .function_id = TM_POWER_VOLTAGE,
    .device_type = ACL_DEVICE_MCU,
    .logic_index = 2,  // 第 2 个 MCU？不直观
}
```

**新方式**（使用设备名称）：
```c
[TM_POWER_VOLTAGE] = {
    .function_id = TM_POWER_VOLTAGE,
    .device_type = ACL_DEVICE_MCU,
    .device_name = "power_mcu",  // 直接指定设备名称
    .logic_index = 0,  // 保留用于兼容
}
```

## 完整的使用示例

### 场景：应用层读取电源电压

```c
#include "acl_api.h"
#include "pdl_mcu.h"

int32_t read_power_voltage(float *voltage)
{
    const acl_tm_config_t *tm_config;
    pdl_mcu_handle_t mcu_handle;
    int32_t ret;
    
    // 1. 从 ACL 查询遥测配置
    tm_config = ACL_GetTmConfig(TM_POWER_VOLTAGE);
    if (!tm_config || !tm_config->enabled) {
        LOG_ERROR("APP", "TM_POWER_VOLTAGE not configured");
        return OSAL_ERR_NOT_FOUND;
    }
    
    // 2. 使用便捷接口初始化 PDL（内部会查询 PCL）
    ret = PDL_MCU_InitByName(tm_config->device_name, &mcu_handle);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("APP", "Failed to init MCU: %s", tm_config->device_name);
        return ret;
    }
    
    // 3. 读取电压
    pdl_mcu_status_t status;
    ret = PDL_MCU_GetStatus(mcu_handle, &status);
    if (ret == OSAL_SUCCESS) {
        *voltage = status.voltage_mv / 1000.0f;  // mV 转 V
    }
    
    // 4. 清理
    PDL_MCU_Deinit(mcu_handle);
    
    return ret;
}
```

### 配置流转路径

```
1. products/ccm/configs/acl/ccm_acl_config.c
   定义：TM_POWER_VOLTAGE → device_name="power_mcu"
   ↓
2. ACL_RegisterTable(&g_pmc_acl_table)
   注册 ACL 配置
   ↓
3. products/ccm/configs/pcl/pcl_h200_100p_base.c
   定义：power_mcu → CAN0, ID=0x123
   ↓
4. PCL_Register(&pcl_h200_100p_base)
   注册 PCL 配置
   ↓
5. 应用层调用
   ACL_GetTmConfig(TM_POWER_VOLTAGE) → device_name="power_mcu"
   PDL_MCU_InitByName("power_mcu", &handle)
     → 内部调用 PCL_HW_FindMCU("power_mcu")
     → 获取配置并初始化
```

## 架构优势

### 1. 清晰的职责分离
- **ACL**：业务功能到设备的映射（TM_POWER_VOLTAGE → "power_mcu"）
- **PCL**：硬件配置管理（"power_mcu" → CAN0, ID=0x123）
- **PDL**：设备驱动实现（协议封装、硬件操作）

### 2. 易于维护
- 更换硬件：只需修改 PCL 配置
- 更换设备映射：只需修改 ACL 配置
- 应用层代码无需修改

### 3. 易于测试
- 可以使用模拟配置测试 PDL
- 可以独立测试 ACL 和 PCL

## 迁移指南

### 从旧接口迁移到新接口

**步骤 1**：更新 ACL 配置，添加 device_name
```c
// 旧配置
[TM_POWER_VOLTAGE] = {
    .device_type = ACL_DEVICE_MCU,
    .logic_index = 2,
}

// 新配置
[TM_POWER_VOLTAGE] = {
    .device_type = ACL_DEVICE_MCU,
    .device_name = "power_mcu",
    .logic_index = 2,  // 保留用于兼容
}
```

**步骤 2**：更新应用层代码，使用便捷接口
```c
// 旧代码
const pcl_platform_config_t *platform = PCL_GetBoard();
const pcl_mcu_entry_t *entry = PCL_HW_GetMCU(platform, tm_config->logic_index);
PDL_MCU_Init(&entry->config, &handle);

// 新代码
PDL_MCU_InitByName(tm_config->device_name, &handle);
```

**步骤 3**：测试验证

## 注意事项

1. **向后兼容**：旧接口 `PDL_MCU_Init()` 仍然可用，不影响现有代码
2. **配置顺序**：必须先调用 `PCL_Register()` 再使用 `PDL_*_InitByName()`
3. **设备名称**：device_name 必须与 PCL 配置中的设备名称一致
4. **错误处理**：`PDL_*_InitByName()` 返回 `OSAL_ERR_NOT_FOUND` 表示设备未找到

## 后续优化建议

### 长期优化（可选）
1. 考虑重命名 PCL 为 HCL (Hardware Configuration Layer)
2. 创建独立的配置类型模块
3. 定义统一的错误码体系
4. 添加配置验证工具

---

**最后更新**: 2026-06-01
**维护者**: wanguo
**分支**: master
