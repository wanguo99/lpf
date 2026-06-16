/**
 * @file README_TEST.md
 * @brief 测试调用链配置说明
 */

# 测试调用链配置说明

## PCONFIG 测试配置

已有的 PCONFIG 配置可用于测试：

### MCU 配置 (pconfig_h200_100p_am625.c)

- **Index 0**: UART /dev/ttyS1 @ 115200bps
  - 接口: `PCONFIG_MCU_INTERFACE_SERIAL`
  - 设备: `/dev/ttyS1`
  - 用于测试: `PDL_MCU_TestCall(0)` → `HAL_Serial_TestCall()`

- **Index 1**: CAN can0 ID:0x100/0x101 @ 500kbps
  - 接口: `PCONFIG_MCU_INTERFACE_CAN`
  - 设备: `can0`
  - 用于测试: `PDL_MCU_TestCall(1)` → `HAL_CAN_TestCall()`

### BMC 配置

- **Index 0**: IPMI Network 192.168.1.100:623 + Serial /dev/ttyS2
  - 主通道: Network
  - 用于测试: `PDL_BMC_TestCall(0)`

- **Index 1**: Redfish Network 192.168.1.101:443 + Serial /dev/ttyS4
  - 主通道: Network
  - 用于测试: `PDL_BMC_TestCall(1)`

## ACONFIG 测试配置

测试功能 ID 映射（待实现）:

```c
// 功能 ID → 设备索引映射
0x9001 (TEST_FUNCTION_MCU_CAN)    → MCU index 1 (CAN)
0x9002 (TEST_FUNCTION_MCU_SERIAL) → MCU index 0 (Serial)
0x9003 (TEST_FUNCTION_BMC)        → BMC index 0 (Network)
```

## 测试调用链

### 完整流程

```
APP_TestCallChain_MCU_CAN()
  ↓
查询 ACONFIG: 功能 ID 0x9001 → MCU index 1
  ↓
PDL_MCU_TestCall(1)
  ↓
查询 PCONFIG: index 1 → CAN can0 配置
  ↓
HAL_CAN_TestCall(NULL)
  ↓
输出测试日志
```

### 使用方法

```c
#include "test_call_chain.h"

int main(void)
{
    // 初始化系统
    OSAL_Init();
    PCONFIG_Init();
    ACONFIG_Init();
    
    // 注册配置
    PCONFIG_Register(&pconfig_h200_100p_am625);
    // ACONFIG_RegisterTable(...);  // TODO
    
    // 运行测试
    int32_t ret = APP_RunAllTests();
    
    if (ret == OSAL_SUCCESS) {
        printf("✅ All tests passed\n");
    } else {
        printf("❌ Some tests failed\n");
    }
    
    return ret;
}
```

## 预期输出

```
========================================
APP_TEST: Test Call Chain: MCU CAN
========================================
APP_TEST: Step 1: Query ACONFIG for device index
APP_TEST: Function ID: 0x9001 (TEST_FUNCTION_MCU_CAN)
APP_TEST: Got MCU index: 1
APP_TEST: Step 2: Call PDL_MCU_TestCall(1)
========================================
PDL_MCU: PDL Layer: MCU TestCall
PDL_MCU: Device Index: 1
PDL_MCU: MCU Name: can_mcu_0
PDL_MCU: Interface: 0
PDL_MCU: Calling HAL_CAN_TestCall...
PDL_MCU: CAN Device: can0
PDL_MCU: CAN Bitrate: 500000
========================================
HAL_CAN: HAL Layer: CAN TestCall
HAL_CAN: Handle: (nil)
HAL_CAN: HAL_CAN_TestCall() executed successfully
========================================
PDL_MCU: PDL_MCU_TestCall() completed successfully
========================================
APP_TEST: ✅ Test Call Chain PASSED
APP_TEST: Complete flow: APP → ACONFIG → PDL → PCONFIG → HAL
========================================
```

## 验证点

1. ✅ PCONFIG 正确返回硬件配置
2. ✅ PDL 正确解析配置并选择 HAL 接口
3. ✅ HAL 测试函数被正确调用
4. ✅ 日志输出完整，显示各层信息
5. ✅ 错误处理正确（索引不存在、配置禁用等）

## TODO

- [ ] 在 ACONFIG 中添加测试功能映射
- [ ] 在 APP 主函数中集成测试入口
- [ ] 添加更多边界测试（无效索引、禁用设备等）
