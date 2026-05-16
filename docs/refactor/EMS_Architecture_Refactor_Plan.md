# EMS架构重构 - 实施计划与验证计划

**版本**: v1.0  
**日期**: 2026年5月  
**作者**: EMS架构团队

---

## 目录

1. [当前状态](#1-当前状态)
   - 1.1 已完成模块
   - 1.2 待开发模块
   - 1.3 代码统计

2. [实施计划](#2-实施计划)
   - 2.1 分阶段实施（10周）
   - 2.2 关键里程碑
   - 2.3 关键风险与缓解措施

3. [验证计划](#3-验证计划)
   - 3.1 功能验证
   - 3.2 性能验证
   - 3.3 可靠性验证
   - 3.4 兼容性验证

---

## 1. 当前状态

### 1.1 已完成模块

#### OSAL层（Operating System Abstraction Layer）✅

**完成度**: 90%

**已实现功能**：
- ✅ 基础类型定义（osal_types.h）
- ✅ 线程管理（osal_thread.h）
- ✅ 进程管理（osal_process.h）
- ✅ IPC机制：互斥锁、信号量、条件变量、原子操作
- ✅ 时间管理（osal_time.h, osal_clock.h）
- ✅ 文件操作（osal_file.h）
- ✅ 信号处理（osal_signal.h）
- ✅ 环境变量（osal_env.h）
- ✅ 字符串/内存工具（osal_string.h, osal_heap.h）
- ✅ 日志系统（osal_log.h）
- ✅ 网络接口（osal_socket.h）

**待补充功能**：
- ⚠️ 共享内存接口（shm_open/mmap/munmap）
- ⚠️ 实时调度接口（sched_setscheduler/setaffinity）
- ⚠️ 内存锁定接口（mlockall/munlockall）

**测试覆盖**: 50+测试用例，覆盖率92%

#### HAL层（Hardware Abstraction Layer）✅

**完成度**: 85%

**已实现驱动**：
- ✅ CAN驱动（hal_can.h）
- ✅ 串口驱动（hal_serial.h）
- ✅ I2C驱动（hal_i2c.h）
- ✅ SPI驱动（hal_spi.h）
- ✅ 看门狗驱动（hal_watchdog.h）

**测试覆盖**: 72测试用例，覆盖率88%

#### PCL层（Peripheral Configuration Layer）✅

**完成度**: 80%

**已实现功能**：
- ✅ 平台配置管理（ti_am6254, vendor_demo）
- ✅ 外设配置查询
- ✅ 多平台支持框架

**待补充功能**：
- ⚠️ 更多平台配置（NXP iMX8, Xilinx Zynq等）

**测试覆盖**: 5+测试用例，覆盖率76%

#### PDL层（Peripheral Driver Layer）✅

**完成度**: 75%

**已实现服务**：
- ✅ Satellite服务（pdl_satellite.h）
- ✅ BMC服务（pdl_bmc.h）
- ✅ MCU服务（pdl_mcu.h）
- ✅ 看门狗服务（pdl_watchdog.h）

**待补充功能**：
- ⚠️ BMC协议完善（Redfish/IPMI）
- ⚠️ 双通道切换逻辑
- ⚠️ 心跳机制

**测试覆盖**: 15+测试用例，覆盖率81%

#### 测试框架✅

**完成度**: 95%

**已实现功能**：
- ✅ 统一测试框架（tests_core）
- ✅ 交互式菜单
- ✅ 命令行模式
- ✅ Busybox风格调用
- ✅ 142+测试用例

### 1.2 待开发模块

#### ACL层（Application Configuration Layer）❌

**完成度**: 0%

**待实现功能**：
- ❌ 业务功能枚举定义
- ❌ 遥控命令映射表
- ❌ 遥测项映射表
- ❌ 遥测分类（缓存型/实时型）
- ❌ O(1)查找表实现
- ❌ 配置文件加载（JSON/YAML）
- ❌ 单元测试

**预计工作量**: 2周

#### APP层（Application Layer）❌

**完成度**: 5%（仅有sample_app和watchdog_app示例）

**待实现进程**：
- ❌ Supervisor进程（监控进程）
- ❌ Telecommand进程（遥控遥测）
- ❌ Telemetry进程（后台采集）
- ❌ Logger进程（日志收集）
- ❌ Firmware进程（固件升级）

**预计工作量**: 8周

### 1.3 代码统计

| 模块 | 代码行数 | 文件数 | 测试用例 | 覆盖率 | 状态 |
|------|---------|-------|---------|-------|------|
| OSAL | ~6,000 | 40 | 50+ | 92% | ✅ 完成 |
| HAL  | ~4,000 | 30 | 72 | 88% | ✅ 完成 |
| PCL  | ~2,000 | 20 | 5+ | 76% | ✅ 完成 |
| PDL  | ~3,500 | 25 | 15+ | 81% | ✅ 完成 |
| ACL  | 0 | 0 | 0 | 0% | ❌ 待开发 |
| APP  | ~500 | 10 | 0 | 0% | ❌ 待开发 |
| Tests | ~4,500 | 18 | 142+ | - | ✅ 完成 |
| **总计** | **~20,500** | **143** | **142+** | **84%** | **70%完成** |

---

## 2. 实施计划

### 2.1 分阶段实施（10周）

### 2.1 分阶段实施（10周）

#### 阶段1：OSAL扩展（1周）

**目标**：补充多进程架构所需的OSAL接口

**任务清单**：
- [ ] 新增共享内存接口（`osal/include/ipc/osal_shm.h`）
  - `OSAL_ShmCreate()` - 创建共享内存
  - `OSAL_ShmOpen()` - 打开共享内存
  - `OSAL_ShmMap()` - 映射共享内存
  - `OSAL_ShmUnmap()` - 取消映射
  - `OSAL_ShmClose()` - 关闭共享内存
  - `OSAL_ShmUnlink()` - 删除共享内存

- [ ] 新增实时调度接口（`osal/include/sys/osal_sched.h`）
  - `OSAL_SchedSetScheduler()` - 设置调度策略（SCHED_FIFO/SCHED_RR）
  - `OSAL_SchedGetScheduler()` - 获取调度策略
  - `OSAL_SchedSetAffinity()` - 设置CPU亲和性
  - `OSAL_SchedGetAffinity()` - 获取CPU亲和性

- [ ] 新增内存锁定接口（`osal/include/sys/osal_mlock.h`）
  - `OSAL_Mlockall()` - 锁定所有内存
  - `OSAL_Munlockall()` - 解锁所有内存

- [ ] 单元测试
  - `tests/unit/osal/test_osal_shm.c` - 共享内存测试
  - `tests/unit/osal/test_osal_sched.c` - 实时调度测试
  - `tests/unit/osal/test_osal_mlock.c` - 内存锁定测试

**交付物**：
- OSAL扩展接口头文件
- POSIX实现代码
- 单元测试（15+用例）

**验收标准**：
- 所有单元测试通过
- 代码覆盖率≥90%

---

#### 阶段2：ACL层实现（2周）

**目标**：实现业务配置层，提供遥控遥测映射

**任务清单**：
- [ ] 定义业务功能枚举（`acl/include/acl_types.h`）
  ```c
  typedef enum {
      ACL_TC_POWER_ON = 0x01,      // 服务器开机
      ACL_TC_POWER_OFF = 0x02,     // 服务器关机
      ACL_TC_POWER_RESET = 0x03,   // 服务器复位
      // ... 更多遥控命令
  } acl_telecommand_t;
  
  typedef enum {
      ACL_TM_CPU_TEMP = 0x01,      // CPU温度（缓存型）
      ACL_TM_POWER_STATE = 0x02,   // 电源状态（实时型）
      // ... 更多遥测项
  } acl_telemetry_t;
  ```

- [ ] 实现遥控命令映射表（`acl/src/acl_telecommand.c`）
  - O(1)直接索引查找
  - 命令→设备类型映射
  - 命令→PDL接口映射
  - 命令参数验证

- [ ] 实现遥测项映射表（`acl/src/acl_telemetry.c`）
  - O(1)直接索引查找
  - 遥测项→设备类型映射
  - 遥测项→PDL接口映射
  - 遥测分类（缓存型/实时型）

- [ ] 配置文件实现（`acl/config/pmc_v1.json`）
  ```json
  {
    "telecommands": [
      {
        "id": 1,
        "name": "POWER_ON",
        "device_type": "BMC",
        "device_index": 0,
        "pdl_function": "PDL_BMC_PowerOn"
      }
    ],
    "telemetries": [
      {
        "id": 1,
        "name": "CPU_TEMP",
        "device_type": "BMC",
        "device_index": 0,
        "type": "CACHED",
        "pdl_function": "PDL_BMC_ReadSensors"
      }
    ]
  }
  ```

- [ ] ACL API实现（`acl/include/acl.h`）
  - `ACL_Init()` - 初始化ACL层
  - `ACL_GetTelecommandMapping()` - 查询遥控映射
  - `ACL_GetTelemetryMapping()` - 查询遥测映射
  - `ACL_GetTelemetryType()` - 获取遥测类型
  - `ACL_Deinit()` - 清理ACL层

- [ ] 单元测试（`tests/unit/acl/`）
  - `test_acl_init.c` - 初始化测试
  - `test_acl_telecommand.c` - 遥控映射测试
  - `test_acl_telemetry.c` - 遥测映射测试
  - `test_acl_config.c` - 配置加载测试

**交付物**：
- ACL层完整实现
- 配置文件示例
- 单元测试（20+用例）
- ACL层文档

**验收标准**：
- 所有单元测试通过
- O(1)查找性能验证
- 代码覆盖率≥80%

---

#### 阶段3：Supervisor进程（1周）

**目标**：实现最小化监控进程，负责子进程管理

**任务清单**：
- [ ] Supervisor主程序（`apps/supervisor/src/main.c`）
  - 进程启动逻辑
  - 心跳检测（共享内存原子时间戳）
  - 进程重启逻辑（5次/300秒限制）
  - 硬件看门狗喂狗（5秒周期）
  - 故障日志记录

- [ ] 共享内存结构设计（`apps/supervisor/include/supervisor_shm.h`）
  ```c
  typedef struct {
      _Atomic uint64_t telecommand_heartbeat;  // Telecommand心跳
      _Atomic uint64_t telemetry_heartbeat;    // Telemetry心跳
      _Atomic uint64_t logger_heartbeat;       // Logger心跳
      _Atomic uint64_t firmware_heartbeat;     // Firmware心跳
      uint32_t crc32;                          // CRC校验
  } supervisor_shm_t;
  ```

- [ ] 进程配置（`apps/supervisor/config/processes.conf`）
  ```ini
  [Telecommand]
  path=/opt/ems/bin/telecommand
  priority=99
  cpu_affinity=0
  restart_limit=5
  restart_window=300
  
  [Telemetry]
  path=/opt/ems/bin/telemetry
  priority=50
  restart_limit=5
  restart_window=300
  ```

- [ ] 单元测试（`tests/unit/supervisor/`）
  - `test_supervisor_start.c` - 进程启动测试
  - `test_supervisor_heartbeat.c` - 心跳检测测试
  - `test_supervisor_restart.c` - 进程重启测试

**交付物**：
- Supervisor进程（<500行）
- 进程配置文件
- 单元测试（10+用例）

**验收标准**：
- 进程启动成功率100%
- 心跳检测延迟<100ms
- 进程重启时间<1秒

---

#### 阶段4：Telecommand进程（2.5周）

**目标**：实现遥控遥测进程，满足2ms实时性要求

**任务清单**：
- [ ] 主程序框架（`apps/telecommand/src/main.c`）
  - 实时调度配置（SCHED_FIFO, priority=99）
  - CPU绑定（core 0）
  - 内存锁定（mlockall）
  - 信号处理（优雅退出）

- [ ] CAN接收线程（`apps/telecommand/src/can_receiver.c`）
  - CAN帧接收（阻塞模式）
  - 命令解析（协议解析）
  - 命令队列（无锁队列）
  - 心跳更新

- [ ] 遥控执行线程（`apps/telecommand/src/tc_executor.c`）
  - 从队列取命令
  - ACL查询映射
  - PDL接口调用
  - 响应打包发送

- [ ] 实时遥测线程（`apps/telecommand/src/tm_realtime.c`）
  - 实时型遥测处理
  - PDL实时查询（1ms超时）
  - 超时降级到缓存
  - 数据新鲜度标记（FRESH/STALE/INVALID）

- [ ] 缓存遥测处理（`apps/telecommand/src/tm_cached.c`）
  - 从共享内存读取
  - 双缓冲切换
  - CRC校验

- [ ] 性能优化
  - 零拷贝传输
  - 预分配缓冲区
  - 避免动态内存分配
  - 关键路径优化

- [ ] 单元测试（`tests/unit/telecommand/`）
  - `test_tc_can_receiver.c` - CAN接收测试
  - `test_tc_executor.c` - 命令执行测试
  - `test_tm_realtime.c` - 实时遥测测试
  - `test_tm_cached.c` - 缓存遥测测试
  - `test_tc_performance.c` - 性能测试

**交付物**：
- Telecommand进程完整实现
- 性能测试报告
- 单元测试（20+用例）

**验收标准**：
- 遥控命令延迟<2ms（P99）
- 缓存型遥测延迟<200μs（P99）
- 实时型遥测延迟<1.2ms（P99）
- CPU占用<30%（正常负载）

---

#### 阶段5：Telemetry进程（1.5周）

**目标**：实现后台遥测采集进程

**任务清单**：
- [ ] 主程序框架（`apps/telemetry/src/main.c`）
  - 普通调度（SCHED_OTHER）
  - 信号处理
  - 共享内存初始化

- [ ] 缓存采集线程（`apps/telemetry/src/cache_collector.c`）
  - 1秒周期采集
  - PDL接口调用
  - 双缓冲写入
  - CRC计算

- [ ] 双缓冲实现（`apps/telemetry/src/double_buffer.c`）
  ```c
  typedef struct {
      telemetry_data_t buffer[2];
      _Atomic uint32_t active_index;  // 0或1
      uint32_t crc32[2];
      _Atomic uint64_t timestamp[2];
      _Atomic uint8_t freshness[2];   // FRESH/STALE/INVALID
  } telemetry_cache_t;
  ```

- [ ] 健康监控线程（`apps/telemetry/src/health_monitor.c`）
  - 5秒周期监控
  - 温度/电压/电流检查
  - 异常告警
  - 状态记录

- [ ] 状态快照线程（`apps/telemetry/src/snapshot.c`）
  - 10秒周期快照
  - 服务器状态
  - 外设状态
  - JSON格式输出

- [ ] 单元测试（`tests/unit/telemetry/`）
  - `test_tm_collector.c` - 采集测试
  - `test_tm_double_buffer.c` - 双缓冲测试
  - `test_tm_health.c` - 健康监控测试

**交付物**：
- Telemetry进程完整实现
- 单元测试（15+用例）

**验收标准**：
- 采集周期准确（误差<10ms）
- 双缓冲切换无数据丢失
- CPU占用<15%

---

#### 阶段6：Logger进程（1周）

**目标**：实现日志收集和管理进程

**任务清单**：
- [ ] 主程序框架（`apps/logger/src/main.c`）
  - 共享内存环形缓冲区初始化
  - 日志文件管理

- [ ] 日志收集线程（`apps/logger/src/log_collector.c`）
  - 从共享内存读取日志
  - 日志分级（DEBUG/INFO/WARN/ERROR）
  - 写入日志文件
  - 实时输出（可选）

- [ ] 环形缓冲区实现（`apps/logger/src/ring_buffer.c`）
  ```c
  typedef struct {
      char buffer[LOG_BUFFER_SIZE];
      _Atomic uint32_t write_pos;
      _Atomic uint32_t read_pos;
      _Atomic uint32_t lost_count;  // 丢失日志计数
  } log_ring_buffer_t;
  ```

- [ ] 日志轮转线程（`apps/logger/src/log_rotation.c`）
  - 按大小轮转（10MB）
  - 按时间轮转（每小时）
  - 压缩归档（gzip）
  - 自动清理（保留7天）

- [ ] 崩溃分析线程（`apps/logger/src/crash_analyzer.c`）
  - coredump收集
  - 堆栈解析
  - 崩溃日志生成

- [ ] 状态归档线程（`apps/logger/src/state_archiver.c`）
  - JSON格式状态日志
  - 定期归档（每小时）

- [ ] 单元测试（`tests/unit/logger/`）
  - `test_logger_collector.c` - 日志收集测试
  - `test_logger_rotation.c` - 日志轮转测试
  - `test_logger_ring_buffer.c` - 环形缓冲区测试

**交付物**：
- Logger进程完整实现
- 单元测试（10+用例）

**验收标准**：
- 日志无丢失（正常负载）
- 日志轮转正常
- CPU占用<5%

---

#### 阶段7：Firmware进程（1周）

**目标**：实现固件升级管理进程

**任务清单**：
- [ ] 主程序框架（`apps/firmware/src/main.c`）
  - 升级状态机
  - 主备分区管理

- [ ] 升级控制线程（`apps/firmware/src/upgrade_controller.c`）
  - 升级命令接收
  - 分片传输（4KB分片）
  - 进度上报
  - 升级完成通知

- [ ] 校验线程（`apps/firmware/src/verifier.c`）
  - CRC32校验
  - SHA256校验
  - 签名验证（可选）

- [ ] 主备分区管理（`apps/firmware/src/partition_manager.c`）
  ```c
  typedef struct {
      char partition_a[50*1024*1024];  // 50MB
      char partition_b[50*1024*1024];  // 50MB
      uint32_t active_partition;       // 0=A, 1=B
      uint32_t boot_count;
      uint32_t crc32_a;
      uint32_t crc32_b;
  } firmware_partition_t;
  ```

- [ ] 回滚机制（`apps/firmware/src/rollback.c`）
  - 启动计数检测
  - 自动回滚（3次启动失败）
  - 回滚日志

- [ ] 单元测试（`tests/unit/firmware/`）
  - `test_fw_upgrade.c` - 升级流程测试
  - `test_fw_verify.c` - 校验测试
  - `test_fw_rollback.c` - 回滚测试

**交付物**：
- Firmware进程完整实现
- 单元测试（10+用例）

**验收标准**：
- 升级成功率100%（正常固件）
- 校验失败自动拒绝
- 回滚机制正常工作

---

#### 阶段8：集成测试与优化（2周）

**目标**：系统集成测试和性能优化

**任务清单**：
- [ ] 系统集成测试（`tests/system/`）
  - `test_system_telecommand_flow.c` - 遥控完整流程
  - `test_system_telemetry_flow.c` - 遥测完整流程
  - `test_system_realtime_perf.c` - 实时性能测试
  - `test_system_fault_recovery.c` - 故障恢复测试
  - `test_system_degradation.c` - 降级运行测试

- [ ] 压力测试（`tests/stress/`）
  - `test_stress_long_run.c` - 长时间运行（72小时）
  - `test_stress_high_load.c` - 高负载测试（100次/秒）
  - `test_stress_burst.c` - 突发负载测试
  - `test_stress_leak.c` - 资源泄漏检测
  - `test_stress_fault_injection.c` - 故障注入测试

- [ ] 性能优化
  - 关键路径profiling（perf/ftrace）
  - 热点函数优化
  - 缓存优化
  - 中断延迟优化

- [ ] 文档完善
  - 用户手册
  - 部署指南
  - 故障排查手册
  - API参考文档

**交付物**：
- 系统测试报告
- 压力测试报告
- 性能分析报告
- 完整文档

**验收标准**：
- 所有系统测试通过
- 72小时稳定性测试通过
- 性能指标全部达标
- 文档完整

### 2.2 关键里程碑

| 周次 | 里程碑 | 交付物 | 验收标准 |
|------|--------|--------|---------|
| Week 1 | OSAL扩展完成 | 共享内存、实时调度、内存锁定接口 | 单元测试通过，覆盖率≥90% |
| Week 3 | ACL层完成 | 业务功能枚举、O(1)查找表、配置文件 | 单元测试通过，覆盖率≥80% |
| Week 4 | Supervisor完成 | 监控进程、心跳检测、进程重启 | 进程重启时间<1秒 |
| Week 6.5 | Telecommand完成 | 2ms应答、实时调度、性能测试报告 | 延迟<2ms（P99） |
| Week 8 | Telemetry完成 | 后台采集、双缓冲、健康监控 | 采集周期准确，CPU<15% |
| Week 9 | Logger完成 | 日志收集、状态归档、崩溃分析 | 日志无丢失，CPU<5% |
| Week 10 | Firmware完成 | 固件升级、主备分区、回滚机制 | 升级成功率100% |
| Week 12 | 集成测试完成 | 测试报告、性能分析报告、完整文档 | 所有测试通过，72小时稳定运行 |

**关键检查点**：

- **Week 1结束**：OSAL扩展验收，确认多进程基础设施就绪
- **Week 3结束**：ACL层验收，确认业务配置层可用
- **Week 4结束**：Supervisor验收，确认进程管理框架可用
- **Week 6.5结束**：Telecommand验收，**关键里程碑**，确认2ms实时性达标
- **Week 10结束**：所有进程开发完成，进入集成测试阶段
- **Week 12结束**：项目交付，所有验收标准达成

### 2.3 关键风险与缓解措施

| 风险 | 影响 | 概率 | 缓解措施 | 应急预案 |
|------|------|------|---------|---------|
| **单核性能瓶颈** | 高 | 中 | 1. Week 6.5前完成性能测试<br>2. 优化PDL层关键路径<br>3. 使用零拷贝技术<br>4. 预分配内存池 | 如性能不达标，考虑：<br>1. 双核SoC方案<br>2. 降低遥测采集频率<br>3. 简化协议栈 |
| **实时调度抖动** | 高 | 中 | 1. 使用PREEMPT_RT内核<br>2. 禁用不必要中断<br>3. 内存锁定（mlockall）<br>4. CPU隔离（isolcpus） | 如抖动严重：<br>1. 调整中断亲和性<br>2. 使用Xenomai<br>3. 裸机RTOS方案 |
| **共享内存同步** | 中 | 低 | 1. 使用原子操作<br>2. 避免锁竞争<br>3. 双缓冲无锁设计<br>4. 充分压力测试 | 如出现数据竞争：<br>1. 增加内存屏障<br>2. 使用读写锁<br>3. 重新设计同步机制 |
| **进程重启丢失状态** | 中 | 中 | 1. 关键状态持久化到Flash<br>2. 快速重启（<1秒）<br>3. 状态恢复机制 | 如状态丢失：<br>1. 从卫星平台重新同步<br>2. 使用默认配置<br>3. 触发系统复位 |
| **日志占满Flash** | 低 | 低 | 1. 日志轮转（10MB/文件）<br>2. 空间监控（<80%告警）<br>3. 自动清理（保留7天） | 如Flash满：<br>1. 停止日志写入<br>2. 删除最旧日志<br>3. 仅保留ERROR级别 |
| **ACL配置错误** | 中 | 低 | 1. 配置文件JSON Schema验证<br>2. 启动时配置检查<br>3. 配置版本管理 | 如配置错误：<br>1. 使用默认配置<br>2. 拒绝启动并告警<br>3. 回滚到上一版本 |
| **BMC通信失败** | 中 | 中 | 1. 双通道冗余（网络+串口）<br>2. 自动切换机制<br>3. 超时降级到缓存 | 如双通道都失败：<br>1. 使用缓存数据<br>2. 标记数据为STALE<br>3. 定期重试连接 |
| **开发进度延期** | 中 | 中 | 1. 每周进度检查<br>2. 关键路径优先<br>3. 并行开发 | 如进度延期：<br>1. 砍掉非关键功能<br>2. 增加开发资源<br>3. 延长集成测试时间 |

**风险监控机制**：
- 每周进度会议，检查里程碑完成情况
- Week 6.5性能测试作为Go/No-Go决策点
- 持续集成，每日构建和测试
- 问题跟踪系统，及时发现和解决风险


---

## 3. 验证计划

### 3.1 功能验证

#### 3.1.1 遥控功能验证

| 测试项 | 测试方法 | 预期结果 | 验证工具 |
|-------|---------|---------|---------|
| 服务器电源控制 | 发送开机/关机/复位命令 | 命令执行成功，状态正确 | CAN工具 + BMC监控 |
| MCU控制 | 发送MCU复位/电源管理命令 | MCU响应正确 | CAN工具 + 示波器 |
| FPGA控制 | 发送FPGA复位/配置加载命令 | FPGA状态正确 | CAN工具 + JTAG |
| 看门狗控制 | 使能/禁用看门狗 | 看门狗状态正确 | 看门狗测试程序 |
| 固件升级 | 启动升级、传输数据、校验、提交 | 升级成功，版本正确 | 固件升级工具 |
| 命令参数验证 | 发送非法参数 | 拒绝执行，返回错误码 | CAN工具 |
| 命令权限验证 | 发送未授权命令 | 拒绝执行，记录日志 | CAN工具 + 日志查看 |

**测试脚本示例**：
```bash
# 服务器电源控制测试
./test_telecommand.sh power_on
./test_telecommand.sh power_off
./test_telecommand.sh power_reset

# 预期：每个命令在2ms内响应，状态正确
```

#### 3.1.2 遥测功能验证

| 测试项 | 测试方法 | 预期结果 | 验证工具 |
|-------|---------|---------|---------|
| 缓存型遥测 | 查询CPU温度、电压、电流 | 返回最近1秒内数据，延迟<200μs | CAN工具 + 时间戳分析 |
| 实时型遥测 | 查询电源状态、MCU状态 | 返回实时数据，延迟<1.2ms | CAN工具 + 时间戳分析 |
| 快遥（1秒周期） | 连续查询快遥数据 | 数据更新周期1秒±10ms | CAN工具 + 周期分析 |
| 慢遥（2秒周期） | 连续查询慢遥数据 | 数据更新周期2秒±10ms | CAN工具 + 周期分析 |
| 快遥慢遥交叉 | 同时查询快遥和慢遥 | 无冲突，延迟均<2ms | CAN工具 + 并发测试 |
| 数据新鲜度标记 | 查询遥测数据 | FRESH/STALE/INVALID标记正确 | CAN工具 + 数据解析 |
| 遥控后遥测 | 执行遥控后立即查询遥测 | 缓存标记为STALE | CAN工具 + 时序分析 |
| 超时降级 | 模拟BMC超时 | 降级到缓存，延迟<1.2ms | 故障注入 + CAN工具 |

**测试脚本示例**：
```bash
# 缓存型遥测测试
./test_telemetry.sh cached cpu_temp 1000
# 预期：1000次查询，平均延迟<200μs，P99<300μs

# 实时型遥测测试
./test_telemetry.sh realtime power_state 1000
# 预期：1000次查询，平均延迟<800μs，P99<1.2ms

# 快遥慢遥交叉测试
./test_telemetry.sh mixed 60
# 预期：60秒内快遥慢遥交叉查询，无冲突，延迟<2ms
```

#### 3.1.3 健康管理验证

| 测试项 | 测试方法 | 预期结果 | 验证工具 |
|-------|---------|---------|---------|
| 服务器健康监控 | 查询服务器健康状态 | 状态正确（正常/异常） | 健康监控工具 |
| BMC连接状态检测 | 断开BMC连接 | 检测到断开，自动切换备用通道 | 网络工具 + 日志 |
| MCU通信状态检测 | 断开MCU连接 | 检测到断开，记录告警 | CAN工具 + 日志 |
| 温度异常检测 | 模拟温度过高 | 触发告警，记录日志 | 温度模拟器 + 日志 |
| 电压异常检测 | 模拟电压异常 | 触发告警，记录日志 | 电压模拟器 + 日志 |
| 心跳超时检测 | 停止进程心跳 | Supervisor检测到超时，重启进程 | kill命令 + 日志 |

#### 3.1.4 日志收集验证

| 测试项 | 测试方法 | 预期结果 | 验证工具 |
|-------|---------|---------|---------|
| 运行日志收集 | 各进程输出日志 | 日志正确收集到文件 | 日志查看工具 |
| 状态日志归档 | 等待状态归档 | JSON格式状态日志生成 | JSON解析工具 |
| 崩溃日志收集 | 模拟进程崩溃 | coredump收集，堆栈解析 | gdb + 日志 |
| 日志轮转 | 写入大量日志 | 日志文件轮转，压缩归档 | ls + 文件大小检查 |
| 日志级别过滤 | 设置不同日志级别 | 只输出指定级别日志 | 日志查看工具 |
| 日志空间管理 | 填满日志空间 | 自动清理旧日志 | df + 日志查看 |

---

### 3.2 性能验证

#### 3.2.1 实时性测试

**测试环境**：
- 硬件：TI AM6254 (单核 @ 1.4GHz)
- 内核：Linux 5.10 + PREEMPT_RT补丁
- 负载：正常负载（遥控10次/秒，遥测100次/秒）

**测试方法**：

```bash
# 1. 遥控命令延迟测试（1000次采样）
./test_performance.sh telecommand_latency 1000

# 预期结果：
# - 平均延迟：<500μs
# - P50延迟：<400μs
# - P95延迟：<800μs
# - P99延迟：<1.2ms
# - 最大延迟：<2ms

# 2. 缓存型遥测延迟测试（1000次采样）
./test_performance.sh cached_telemetry_latency 1000

# 预期结果：
# - 平均延迟：<150μs
# - P50延迟：<140μs
# - P95延迟：<200μs
# - P99延迟：<300μs
# - 最大延迟：<500μs

# 3. 实时型遥测延迟测试（1000次采样）
./test_performance.sh realtime_telemetry_latency 1000

# 预期结果：
# - 平均延迟：<700μs
# - P50延迟：<650μs
# - P95延迟：<1000μs
# - P99延迟：<1.2ms
# - 最大延迟：<2ms

# 4. 2ms应答测试（示波器测量）
# CAN命令发送 → CAN应答接收
# 预期：100%在2ms内完成
```

**性能指标汇总**：

| 指标 | 目标值 | 测试方法 | 验收标准 |
|------|-------|---------|---------|
| 遥控命令延迟（平均） | <500μs | 高精度时间戳 | 1000次采样，平均<500μs |
| 遥控命令延迟（P99） | <1.2ms | 高精度时间戳 | 1000次采样，P99<1.2ms |
| 遥控命令延迟（最大） | <2ms | 高精度时间戳 | 1000次采样，100%<2ms |
| 缓存型遥测延迟（平均） | <150μs | 高精度时间戳 | 1000次采样，平均<150μs |
| 缓存型遥测延迟（P99） | <300μs | 高精度时间戳 | 1000次采样，P99<300μs |
| 实时型遥测延迟（平均） | <700μs | 高精度时间戳 | 1000次采样，平均<700μs |
| 实时型遥测延迟（P99） | <1.2ms | 高精度时间戳 | 1000次采样，P99<1.2ms |
| CPU占用（正常负载） | <60% | top/htop | 持续监控，平均<60% |
| 内存占用 | <50MB | top/htop | 持续监控，<50MB |
| Flash占用 | <200MB | df | 安装后检查，<200MB |

#### 3.2.2 压力测试

**测试场景**：

```bash
# 1. 高频遥控测试（每秒100条命令，持续10分钟）
./test_stress.sh high_frequency_telecommand 100 600

# 预期结果：
# - 无命令丢失
# - 延迟<2ms（100%）
# - CPU占用<80%
# - 内存占用稳定

# 2. 快遥慢遥交叉测试（持续30分钟）
./test_stress.sh mixed_telemetry 1800

# 预期结果：
# - 无数据冲突
# - 延迟<2ms（100%）
# - 数据更新周期准确

# 3. 长期稳定性测试（72小时连续运行）
./test_stress.sh long_term_stability 259200

# 预期结果：
# - 无进程崩溃
# - 无内存泄漏
# - 无资源耗尽
# - 性能指标稳定

# 4. 突发负载测试（1秒内1000次遥控）
./test_stress.sh burst_load 1000

# 预期结果：
# - 队列不溢出
# - 降级处理正常
# - 恢复时间<5秒
```

**压力测试指标**：

| 测试类型 | 配置 | 验收标准 |
|---------|------|---------|
| 高频遥控 | 100次/秒，持续10分钟 | 无丢失，延迟<2ms，CPU<80% |
| 快遥慢遥交叉 | 持续30分钟 | 无冲突，延迟<2ms，周期准确 |
| 长期稳定性 | 72小时连续运行 | 无崩溃，无泄漏，性能稳定 |
| 突发负载 | 1秒内1000次遥控 | 队列不溢出，降级正常 |
| 资源泄漏检测 | 1小时持续运行 | 内存/FD/线程无泄漏 |

---

### 3.3 可靠性验证

#### 3.3.1 故障注入测试

**测试场景**：

```bash
# 1. 进程崩溃测试
kill -SEGV $(pidof telecommand)
# 预期：Supervisor检测到崩溃，1秒内重启，服务恢复

# 2. 连续崩溃测试（5次崩溃）
for i in {1..5}; do 
    kill -SEGV $(pidof telemetry)
    sleep 10
done
# 预期：第5次崩溃后触发系统复位

# 3. SEU模拟测试（翻转共享内存）
./test_fault_injection.sh seu_simulation 100
# 预期：检测到数据损坏，CRC校验失败，自动恢复

# 4. 硬件看门狗测试
kill -STOP $(pidof supervisor)
# 预期：10秒后硬件看门狗触发系统复位

# 5. BMC双通道故障测试
./test_fault_injection.sh bmc_network_down
# 预期：自动切换到串口通道，服务不中断

# 6. 内存耗尽测试
./test_fault_injection.sh memory_exhaustion
# 预期：内存监控告警，拒绝新分配，系统稳定

# 7. Flash满测试
./test_fault_injection.sh flash_full
# 预期：日志轮转，删除旧日志，系统稳定
```

**故障注入测试矩阵**：

| 故障类型 | 注入方式 | 预期恢复行为 | 恢复时间 | 验收标准 |
|---------|---------|-------------|---------|---------|
| 进程崩溃 | kill -SEGV | Supervisor检测并重启 | <1秒 | 服务恢复，无数据丢失 |
| 进程僵死 | 死循环 | 心跳超时检测并重启 | <10秒 | 服务恢复 |
| 连续崩溃 | 5次崩溃 | 触发系统复位 | <5秒 | 系统重启，服务恢复 |
| SEU（共享内存） | 随机翻转位 | CRC校验失败，重新初始化 | <100ms | 数据完整性保护 |
| BMC网络故障 | 断开网络 | 自动切换到串口通道 | <2秒 | 服务不中断 |
| BMC双通道故障 | 断开网络和串口 | 降级到缓存，标记STALE | <1秒 | 返回缓存数据 |
| 内存耗尽 | 持续分配内存 | 内存监控告警，拒绝分配 | 立即 | 系统稳定，不崩溃 |
| Flash满 | 写满Flash | 日志轮转，删除旧日志 | <10秒 | 系统稳定，日志继续 |
| CAN总线故障 | 断开CAN | 检测到故障，记录告警 | <1秒 | 告警正确 |
| 看门狗超时 | 停止喂狗 | 硬件看门狗复位 | <10秒 | 系统重启 |

#### 3.3.2 降级运行测试

**测试场景**：

```bash
# 1. Telemetry进程禁用
kill -KILL $(pidof telemetry)
# 预期：缓存型遥测返回旧数据（标记STALE），实时型遥测正常

# 2. Telecommand进程禁用
kill -KILL $(pidof telecommand)
# 预期：遥控遥测中断，Supervisor重启后恢复

# 3. Logger进程禁用
kill -KILL $(pidof logger)
# 预期：日志丢失，其他功能正常

# 4. Firmware进程禁用
kill -KILL $(pidof firmware)
# 预期：固件升级不可用，其他功能正常
```

**降级运行验证**：

| 场景 | 禁用进程 | 预期行为 | 影响范围 | 验收标准 |
|------|---------|---------|---------|---------|
| 遥测采集故障 | Telemetry | 缓存型遥测返回旧数据（STALE） | 仅缓存型遥测 | 实时型遥测正常 |
| 遥控遥测故障 | Telecommand | 遥控遥测中断，1秒内重启恢复 | 遥控遥测 | 重启后恢复 |
| 日志收集故障 | Logger | 日志丢失，其他功能正常 | 日志 | 其他功能不受影响 |
| 固件升级故障 | Firmware | 固件升级不可用 | 固件升级 | 其他功能不受影响 |

---

### 3.4 兼容性验证

#### 3.4.1 平台兼容性

| 平台 | 架构 | 内核 | 验证项 | 状态 |
|------|------|------|-------|------|
| TI AM6254 | ARM Cortex-A53 | Linux 5.10 + PREEMPT_RT | 完整功能测试 | ✅ 主要平台 |
| NXP i.MX8M | ARM Cortex-A53 | Linux 5.15 + PREEMPT_RT | 完整功能测试 | ⚠️ 待验证 |
| Xilinx Zynq MPSoC | ARM Cortex-A53 | Linux 5.10 | 完整功能测试 | ⚠️ 待验证 |
| x86_64 (开发环境) | x86_64 | Ubuntu 22.04 | 单元测试 | ✅ 已验证 |

#### 3.4.2 外设兼容性

| 外设类型 | 接口 | 验证项 | 状态 |
|---------|------|-------|------|
| BMC (Redfish) | Ethernet | 电源控制、传感器读取 | ✅ 已验证 |
| BMC (IPMI) | Ethernet/Serial | 电源控制、传感器读取 | ⚠️ 待验证 |
| MCU | CAN | 控制命令、状态查询 | ✅ 已验证 |
| FPGA | SPI/I2C | 配置加载、状态查询 | ⚠️ 待验证 |
| 看门狗 | 硬件看门狗 | 喂狗、超时复位 | ✅ 已验证 |

#### 3.4.3 协议兼容性

| 协议 | 版本 | 验证项 | 状态 |
|------|------|-------|------|
| CAN 2.0B | - | 标准帧、扩展帧 | ✅ 已验证 |
| Redfish | 1.0+ | RESTful API | ⚠️ 待验证 |
| IPMI | 2.0 | 串口/网络 | ⚠️ 待验证 |
| JSON | - | 配置文件解析 | ✅ 已验证 |

---

## 4. 验收标准

### 4.1 功能验收

- [ ] 所有遥控命令正常执行
- [ ] 所有遥测项正常查询
- [ ] 快遥慢遥交叉无冲突
- [ ] 健康管理正常工作
- [ ] 日志收集完整
- [ ] 固件升级成功

### 4.2 性能验收

- [ ] 遥控命令延迟<2ms（100%）
- [ ] 缓存型遥测延迟<200μs（P99）
- [ ] 实时型遥测延迟<1.2ms（P99）
- [ ] CPU占用<60%（正常负载）
- [ ] 内存占用<50MB
- [ ] Flash占用<200MB

### 4.3 可靠性验收

- [ ] 72小时稳定性测试通过
- [ ] 进程崩溃自动恢复
- [ ] SEU检测和恢复正常
- [ ] 降级运行正常
- [ ] 无内存泄漏
- [ ] 无资源耗尽

### 4.4 文档验收

- [ ] 用户手册完整
- [ ] 部署指南完整
- [ ] API参考文档完整
- [ ] 故障排查手册完整
- [ ] 测试报告完整

---

## 5. 交付清单

### 5.1 代码交付

- [ ] 源代码（OSAL/HAL/PCL/PDL/ACL/APP）
- [ ] 测试代码（单元测试/系统测试/压力测试）
- [ ] 构建脚本（CMake/build.sh）
- [ ] 配置文件（ACL配置/进程配置）

### 5.2 文档交付

- [ ] 架构设计文档
- [ ] 实施计划文档
- [ ] 用户手册
- [ ] 部署指南
- [ ] API参考文档
- [ ] 故障排查手册
- [ ] 测试报告

### 5.3 工具交付

- [ ] 测试工具（性能测试/压力测试/故障注入）
- [ ] 调试工具（日志查看/状态监控）
- [ ] 部署工具（安装脚本/配置工具）

---

**文档版本历史**：

| 版本 | 日期 | 作者 | 变更说明 |
|------|------|------|---------|
| v1.0 | 2026-05-17 | EMS架构团队 | 初始版本，完善实施计划和验证计划 |

---

