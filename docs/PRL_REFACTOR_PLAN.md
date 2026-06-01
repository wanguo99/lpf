# PRL 重构计划：从"点对点"到"按设备类型"

## 问题分析

当前的文件命名暗示了"点对点"的协议设计：
- `prl_gsc_pmc.h` - GSC ↔ PMC
- `prl_pmc_ccm.h` - PMC ↔ CCM  
- `prl_ccm_satellite.h` - CCM ↔ Satellite
- `prl_ccm_power.h` - CCM ↔ Power

但实际上：
- 协议头中有 `dev_type` 字段区分设备
- 协议是统一的，不区分通信双方
- 任何设备都可以与任何设备通信

## 重构方案

### 1. 文件重命名

**删除旧文件**：
- `prl_gsc_pmc.h/c`
- `prl_pmc_ccm.h/c`
- `prl_ccm_satellite.h/c`
- `prl_ccm_power.h/c`

**创建新文件**（按设备类型）：
- `prl_gsc.h/c` - GSC 设备的消息定义
- `prl_pmc.h/c` - PMC 设备的消息定义
- `prl_ccm.h/c` - CCM 设备的消息定义
- `prl_power.h/c` - POWER 设备的消息定义
- `prl_mcu.h/c` - MCU 设备的消息定义（已存在）

### 2. 消息类型整合

从旧文件中提取各设备的消息类型：

**GSC 设备**（从 prl_gsc_pmc.h 提取）：
- PRL_GSC_MSG_HEARTBEAT
- PRL_GSC_MSG_TELEMETRY
- PRL_GSC_MSG_COMMAND
- PRL_GSC_MSG_FILE_TRANSFER
- PRL_GSC_MSG_DATABASE_SYNC
- PRL_GSC_MSG_LOG_UPLOAD

**PMC 设备**（从 prl_gsc_pmc.h 和 prl_pmc_ccm.h 提取）：
- PRL_PMC_MSG_HEARTBEAT
- PRL_PMC_MSG_TELEMETRY
- PRL_PMC_MSG_COMMAND
- PRL_PMC_MSG_FIRMWARE_UPDATE
- PRL_PMC_MSG_NODE_MANAGE
- PRL_PMC_MSG_POWER_CONTROL
- PRL_PMC_MSG_STATUS_QUERY

**CCM 设备**（从 prl_pmc_ccm.h 和 prl_ccm_satellite.h 提取）：
- PRL_CCM_MSG_HEARTBEAT
- PRL_CCM_MSG_TELEMETRY
- PRL_CCM_MSG_COMMAND
- PRL_CCM_MSG_TIME_SYNC
- PRL_CCM_MSG_ORBIT_DATA
- PRL_CCM_MSG_ATTITUDE_DATA
- PRL_CCM_MSG_POWER_STATUS
- PRL_CCM_MSG_THERMAL_STATUS

**POWER 设备**（从 prl_ccm_power.h 提取）：
- PRL_POWER_MSG_HEARTBEAT
- PRL_POWER_MSG_POWER_ON
- PRL_POWER_MSG_POWER_OFF
- PRL_POWER_MSG_VOLTAGE_QUERY
- PRL_POWER_MSG_CURRENT_QUERY
- PRL_POWER_MSG_TEMP_QUERY
- PRL_POWER_MSG_STATUS_REPORT
- PRL_POWER_MSG_ALARM

### 3. 配置选项更新

**旧配置**：
```kconfig
CONFIG_PRL_GSC_PMC=y
CONFIG_PRL_PMC_CCM=y
CONFIG_PRL_CCM_SATELLITE=y
CONFIG_PRL_CCM_POWER=y
```

**新配置**：
```kconfig
CONFIG_PRL_GSC=y
CONFIG_PRL_PMC=y
CONFIG_PRL_CCM=y
CONFIG_PRL_POWER=y
CONFIG_PRL_MCU=y
```

### 4. CMakeLists.txt 更新

**旧配置**：
```cmake
if(CONFIG_PRL_GSC_PMC)
    list(APPEND ADD_SRCS "src/prl_gsc_pmc.c")
endif()
```

**新配置**：
```cmake
if(CONFIG_PRL_GSC)
    list(APPEND ADD_SRCS "src/prl_gsc.c")
endif()
```

### 5. 文档更新

更新所有文档中的引用：
- README.md
- PRL_ARCHITECTURE.md
- PRL_USAGE_GUIDE.md

## 实施步骤

1. ✅ 创建重构计划文档
2. ⏳ 创建新的设备消息定义文件
3. ⏳ 更新 Kconfig
4. ⏳ 更新 CMakeLists.txt
5. ⏳ 删除旧文件
6. ⏳ 更新文档
7. ⏳ 编译验证
8. ⏳ 提交更改

## 优势

**重构前**：
- ❌ 文件命名暗示点对点通信
- ❌ 消息类型分散在多个文件
- ❌ 不符合统一协议的设计理念

**重构后**：
- ✅ 按设备类型组织，清晰明了
- ✅ 每个设备的消息集中管理
- ✅ 符合统一协议的设计理念
- ✅ 易于扩展新设备类型

---

**创建时间**：2026-06-01  
**创建者**：wanguo
