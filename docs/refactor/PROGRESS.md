# EMS架构优化开发进度跟踪

**分支**: dev-refactor  
**开始时间**: 2026-05-17  
**预计完成时间**: 2026-12-24（31周）

---

## 总体进度

| 阶段 | 状态 | 完成度 | 开始时间 | 完成时间 | 备注 |
|------|------|--------|----------|----------|------|
| 阶段1：基础设施完善 | 🔄 进行中 | 86% | 2026-05-17 | - | OSAL层增强 |
| 阶段2：ACL层实现 | ⏸️ 未开始 | 0% | - | - | 业务配置层 |
| 阶段3：PDL层功能补全 | ⏸️ 未开始 | 0% | - | - | Redfish/IPMI |
| 阶段4：APP层核心进程 | ⏸️ 未开始 | 0% | - | - | 5个核心进程 |
| 阶段5：硬实时优化 | ⏸️ 未开始 | 0% | - | - | 2ms响应保证 |
| 阶段6：可靠性增强 | ⏸️ 未开始 | 0% | - | - | 故障恢复机制 |
| 阶段7：多核异构支持 | ⏸️ 未开始 | 0% | - | - | R5F+A53预研 |

---

## 阶段1：基础设施完善（4周）

**目标**: 补齐OSAL层缺失功能，为上层提供完整的运行环境  
**状态**: 🔄 进行中  
**完成度**: 6/7 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T1.1 | OSAL实时调度API | P0 | 3天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | SCHED_FIFO/RR |
| T1.2 | OSAL CPU亲和性API | P0 | 2天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | 已在T1.1中实现 |
| T1.3 | OSAL内存锁定API | P0 | 2天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | 已在T1.1中实现 |
| T1.4 | OSAL共享内存API | P0 | 5天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | 创建/映射/销毁 |
| T1.5 | OSAL原子操作增强 | P1 | 2天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | 64位原子时间戳 |
| T1.6 | HAL GPIO驱动 | P1 | 3天 | ✅ 已完成 | 2026-05-17 | 2026-05-17 | - | 输入/输出/中断 |
| T1.7 | 单元测试补充 | P1 | 3天 | ⏸️ 未开始 | - | - | - | 覆盖率>80% |

### 详细进度

#### T1.1 OSAL实时调度API（已完成）

**需求**:
- 支持SCHED_FIFO（先进先出实时调度）
- 支持SCHED_RR（轮转实时调度）
- 支持设置线程优先级（1-99）
- 支持查询当前调度策略和优先级
- 支持CPU亲和性设置（T1.2）
- 支持内存锁定（T1.3）

**设计**:
```c
// osal/include/sys/osal_sched.h
int32_t OSAL_SchedSetPolicy(osal_thread_t thread, int32_t policy, int32_t priority);
int32_t OSAL_SchedGetPolicy(osal_thread_t thread, int32_t *policy, int32_t *priority);
int32_t OSAL_SchedSetAffinity(osal_thread_t thread, int32_t cpu_id);
int32_t OSAL_SchedGetAffinity(osal_thread_t thread, int32_t *cpu_id);
int32_t OSAL_SchedGetCPUCount(void);
int32_t OSAL_MemLock(bool lock_all);
int32_t OSAL_MemUnlock(void);
```

**实现文件**:
- [x] osal/include/sys/osal_sched.h
- [x] osal/src/posix/sys/osal_sched.c
- [x] tests/unit/osal/test_osal_sched.c

**完成状态**: 
- ✅ 实现了完整的实时调度API（SCHED_FIFO/RR/OTHER）
- ✅ 实现了CPU亲和性API（Linux支持，macOS返回NOT_IMPLEMENTED）
- ✅ 实现了内存锁定API（mlockall/munlockall）
- ✅ 实现了跨平台兼容（Linux/macOS条件编译）
- ✅ 完成了13个单元测试用例（9个通过，4个平台限制跳过）
- ✅ 修复了macOS平台的编译和运行问题

**提交记录**:
- cf8a4bb: 实现OSAL实时调度API及单元测试
- 3196929: 修复完善OSAL调度模块的macOS平台兼容性

#### T1.4 OSAL共享内存API（已完成）

**需求**:
- 支持POSIX共享内存（shm_open/shm_unlink）
- 支持内存映射（mmap/munmap）
- 支持创建、独占、读写、只读等多种模式
- 支持多进程共享内存访问

**设计**:
```c
// osal/include/ipc/osal_shm.h
int32_t OSAL_ShmCreate(const char *name, size_t size, int32_t flags, osal_shm_t *shm);
int32_t OSAL_ShmOpen(const char *name, int32_t flags, osal_shm_t *shm);
int32_t OSAL_ShmMap(osal_shm_t *shm, size_t offset, size_t length, int32_t flags, void **addr);
int32_t OSAL_ShmUnmap(void *addr, size_t length);
int32_t OSAL_ShmClose(osal_shm_t *shm);
int32_t OSAL_ShmUnlink(const char *name);
```

**实现文件**:
- [x] osal/include/ipc/osal_shm.h
- [x] osal/src/posix/ipc/osal_shm.c
- [x] tests/unit/osal/test_osal_shm.c

**完成状态**: 
- ✅ 实现了完整的共享内存API（创建/打开/映射/关闭/删除）
- ✅ 支持多种标志位（CREATE/EXCL/RDWR/RDONLY）
- ✅ 实现了Linux平台的完整功能
- ✅ 完成了7个单元测试用例，全部通过：
  * 基本创建和删除
  * 独占创建模式
  * 内存映射和解映射
  * 多进程共享（fork验证）
  * 只读映射
  * 部分映射
  * 参数验证

**提交记录**:
- 6eb1e8f: 实现OSAL共享内存API (T1.4)

#### T1.5 OSAL原子操作增强（已完成）

**需求**:
- 添加64位原子操作支持
- 用于原子时间戳等需要64位精度的场景
- 保持与32位原子操作相同的API风格

**设计**:
```c
// osal/include/ipc/osal_atomic.h
typedef struct {
    _Atomic uint64_t value;
} osal_atomic_uint64_t;

void OSAL_AtomicInit64(osal_atomic_uint64_t *atomic, uint64_t value);
uint64_t OSAL_AtomicLoad64(const osal_atomic_uint64_t *atomic);
void OSAL_AtomicStore64(osal_atomic_uint64_t *atomic, uint64_t value);
uint64_t OSAL_AtomicFetchAdd64(osal_atomic_uint64_t *atomic, uint64_t value);
uint64_t OSAL_AtomicFetchSub64(osal_atomic_uint64_t *atomic, uint64_t value);
uint64_t OSAL_AtomicIncrement64(osal_atomic_uint64_t *atomic);
uint64_t OSAL_AtomicDecrement64(osal_atomic_uint64_t *atomic);
bool OSAL_AtomicCompareExchange64(osal_atomic_uint64_t *atomic, uint64_t expected, uint64_t desired);
```

**实现文件**:
- [x] osal/include/ipc/osal_atomic.h（扩展）
- [x] osal/src/posix/ipc/osal_atomic.c（扩展）
- [x] tests/unit/osal/test_osal_atomic.c（扩展）

**完成状态**: 
- ✅ 新增64位原子类型osal_atomic_uint64_t
- ✅ 实现完整的64位原子操作API（8个函数）
- ✅ 使用C11标准的_Atomic和stdatomic.h
- ✅ 新增7个64位原子操作单元测试，全部通过：
  * 初始化/加载/存储
  * 加法/减法操作
  * 自增/自减
  * 比较交换(CAS)
  * 溢出边界测试
  * 多线程时间戳测试
- ✅ 总计18个测试用例全部通过（11个32位 + 7个64位）

**提交记录**:
- 05c3b0e: 增强OSAL原子操作，添加64位支持 (T1.5)

#### T1.6 HAL GPIO驱动（已完成）

**需求**:
- 支持GPIO输入/输出模式控制
- 支持GPIO电平读写（高/低）
- 支持GPIO中断（上升沿/下降沿/双边沿触发）
- 支持中断回调机制
- 跨平台支持（Linux真实硬件 + macOS模拟）

**设计**:
```c
// hal/include/hal_gpio.h
int32_t HAL_GPIO_Init(uint32_t gpio_num, const hal_gpio_config_t *config);
int32_t HAL_GPIO_Deinit(uint32_t gpio_num);
int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction);
int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction);
int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level);
int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level);
int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data);
int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num);
int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num);
```

**实现文件**:
- [x] hal/include/hal_gpio.h
- [x] hal/src/linux/hal_gpio.c
- [x] hal/src/macos/hal_gpio.c
- [x] tests/unit/hal/test_hal_gpio.c

**完成状态**: 
- ✅ 实现了完整的GPIO API接口（9个函数）
- ✅ Linux平台使用sysfs GPIO接口（/sys/class/gpio/）
- ✅ 实现了GPIO中断监听线程（使用poll机制）
- ✅ macOS平台提供内存模拟实现
- ✅ 完成了7个单元测试用例，全部通过：
  * GPIO初始化/释放
  * 方向控制（输入/输出）
  * 电平控制（高/低）
  * 中断配置和使能/禁用
  * 参数验证
  * 输出模式测试
  * 输入模式测试

**提交记录**:
- 395c6ab: 功能：实现HAL GPIO驱动

---

## 阶段2：ACL层实现（3周）

**目标**: 实现业务配置层，建立业务功能与硬件设备的映射关系  
**状态**: ⏸️ 未开始  
**完成度**: 0/8 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T2.1 | ACL目录结构创建 | P0 | 0.5天 | ⏸️ 未开始 | - | - | - | acl/{include,src,config} |
| T2.2 | 业务功能枚举定义 | P0 | 2天 | ⏸️ 未开始 | - | - | - | TC/TM/健康管理 |
| T2.3 | ACL配置数据结构 | P0 | 2天 | ⏸️ 未开始 | - | - | - | O(1)查询 |
| T2.4 | ACL查询API实现 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 配置查询接口 |
| T2.5 | PMC产品配置文件 | P0 | 3天 | ⏸️ 未开始 | - | - | - | acl_pmc_v1.c |
| T2.6 | 配置校验机制 | P1 | 2天 | ⏸️ 未开始 | - | - | - | 完整性检查 |
| T2.7 | ACL单元测试 | P1 | 2天 | ⏸️ 未开始 | - | - | - | 覆盖率>90% |
| T2.8 | ACL文档编写 | P2 | 1天 | ⏸️ 未开始 | - | - | - | README + API文档 |

---

## 阶段3：PDL层功能补全（5周）

**目标**: 实现完整的外设服务，支持Redfish/IPMI协议和双通道冗余  
**状态**: ⏸️ 未开始  
**完成度**: 0/9 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T3.1 | Redfish协议实现 | P0 | 5天 | ⏸️ 未开始 | - | - | - | 基础Redfish API |
| T3.2 | IPMI协议实现 | P0 | 5天 | ⏸️ 未开始 | - | - | - | IPMI 2.0命令集 |
| T3.3 | BMC双通道冗余 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 网络/串口切换 |
| T3.4 | Satellite双通道冗余 | P0 | 3天 | ⏸️ 未开始 | - | - | - | CAN0/CAN1切换 |
| T3.5 | MCU健康监控 | P1 | 2天 | ⏸️ 未开始 | - | - | - | 心跳检测 |
| T3.6 | 遥测数据新鲜度标记 | P0 | 2天 | ⏸️ 未开始 | - | - | - | FRESH/STALE/INVALID |
| T3.7 | 超时降级机制 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 实时查询超时降级 |
| T3.8 | PDL性能优化 | P1 | 3天 | ⏸️ 未开始 | - | - | - | 关键路径<500μs |
| T3.9 | PDL单元测试 | P1 | 3天 | ⏸️ 未开始 | - | - | - | 覆盖率>85% |

---

## 阶段4：APP层核心进程实现（6周）

**目标**: 实现5个核心业务进程，建立完整的应用架构  
**状态**: ⏸️ 未开始  
**完成度**: 0/9 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T4.1 | 共享内存布局设计 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 遥测缓存/心跳表/日志 |
| T4.2 | Supervisor进程实现 | P0 | 4天 | ⏸️ 未开始 | - | - | - | 进程监控/自动重启 |
| T4.3 | Telecommand进程实现 | P0 | 5天 | ⏸️ 未开始 | - | - | - | CAN接收/2ms应答 |
| T4.4 | Telemetry进程实现 | P0 | 5天 | ⏸️ 未开始 | - | - | - | 遥测采集/周期上报 |
| T4.5 | Firmware进程实现 | P1 | 4天 | ⏸️ 未开始 | - | - | - | 固件升级/A/B分区 |
| T4.6 | Logger进程实现 | P1 | 3天 | ⏸️ 未开始 | - | - | - | 日志收集/崩溃分析 |
| T4.7 | 实时调度配置 | P0 | 2天 | ⏸️ 未开始 | - | - | - | SCHED_FIFO/CPU绑定 |
| T4.8 | 进程间通信测试 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 共享内存/心跳/日志 |
| T4.9 | 集成测试 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 5进程协同工作 |

---

## 阶段5：硬实时优化（3周）

**目标**: 优化关键路径，确保2ms硬实时响应  
**状态**: ⏸️ 未开始  
**完成度**: 0/9 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T5.1 | 实时性能基准测试 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 建立性能基线 |
| T5.2 | CAN接收路径优化 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 中断到应答<1.5ms |
| T5.3 | 内存预分配优化 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 消除运行时malloc |
| T5.4 | 缓存型遥测优化 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 共享内存访问<10μs |
| T5.5 | 实时型遥测优化 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 查询+应答<800μs |
| T5.6 | CPU隔离配置 | P1 | 1天 | ⏸️ 未开始 | - | - | - | isolcpus内核参数 |
| T5.7 | 中断亲和性配置 | P1 | 1天 | ⏸️ 未开始 | - | - | - | CAN中断绑定 |
| T5.8 | 实时性能压力测试 | P0 | 3天 | ⏸️ 未开始 | - | - | - | 1000次测试 |
| T5.9 | 性能监控工具 | P1 | 2天 | ⏸️ 未开始 | - | - | - | 实时延迟统计 |

---

## 阶段6：可靠性增强（4周）

**目标**: 实现完整的故障检测和恢复机制  
**状态**: ⏸️ 未开始  
**完成度**: 0/9 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T6.1 | 硬件看门狗集成 | P0 | 2天 | ⏸️ 未开始 | - | - | - | GPIO喂狗 |
| T6.2 | 进程崩溃检测 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 子进程异常退出 |
| T6.3 | 进程自动重启 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 5次/300秒限制 |
| T6.4 | 心跳超时检测 | P0 | 2天 | ⏸️ 未开始 | - | - | - | 5秒超时重启 |
| T6.5 | 降级运行模式 | P1 | 3天 | ⏸️ 未开始 | - | - | - | 关键进程崩溃降级 |
| T6.6 | 故障日志持久化 | P1 | 2天 | ⏸️ 未开始 | - | - | - | 崩溃日志写Flash |
| T6.7 | 固件A/B分区管理 | P0 | 4天 | ⏸️ 未开始 | - | - | - | 升级失败回滚 |
| T6.8 | 固件校验恢复 | P0 | 3天 | ⏸️ 未开始 | - | - | - | CRC/SHA256校验 |
| T6.9 | 故障注入测试 | P0 | 4天 | ⏸️ 未开始 | - | - | - | 模拟故障场景 |

---

## 阶段7：多核异构支持（预研，6周）

**目标**: 为未来R5F+A53多核异构架构做技术储备  
**状态**: ⏸️ 未开始  
**完成度**: 0/8 任务

### 任务清单

| 任务ID | 任务描述 | 优先级 | 工作量 | 状态 | 开始时间 | 完成时间 | 负责人 | 备注 |
|--------|---------|--------|--------|------|----------|----------|--------|------|
| T7.1 | R5F开发环境搭建 | P2 | 3天 | ⏸️ 未开始 | - | - | - | TI CCS + FreeRTOS |
| T7.2 | OSAL FreeRTOS移植 | P2 | 5天 | ⏸️ 未开始 | - | - | - | 支持FreeRTOS平台 |
| T7.3 | HAL R5F驱动适配 | P2 | 5天 | ⏸️ 未开始 | - | - | - | CAN/GPIO/Watchdog |
| T7.4 | RPMsg通信实现 | P2 | 4天 | ⏸️ 未开始 | - | - | - | A53<->R5F消息传递 |
| T7.5 | 共享内存跨核访问 | P2 | 3天 | ⏸️ 未开始 | - | - | - | A53/R5F共享DDR |
| T7.6 | RemoteProc集成 | P2 | 3天 | ⏸️ 未开始 | - | - | - | A53控制R5F启停 |
| T7.7 | R5F实时任务实现 | P2 | 5天 | ⏸️ 未开始 | - | - | - | CAN中断/遥控应答 |
| T7.8 | 核间通信测试 | P2 | 3天 | ⏸️ 未开始 | - | - | - | 延迟<100μs |

---

## 里程碑

| 里程碑 | 目标时间 | 状态 | 完成时间 | 关键交付物 |
|--------|---------|------|----------|-----------|
| M1：基础设施完善 | 第4周 (2026-06-14) | ⏸️ 未完成 | - | OSAL增强API |
| M2：ACL层完成 | 第7周 (2026-07-05) | ⏸️ 未完成 | - | ACL层代码+配置 |
| M3：PDL层完整 | 第12周 (2026-08-09) | ⏸️ 未完成 | - | Redfish/IPMI实现 |
| M4：APP层完成 | 第18周 (2026-09-20) | ⏸️ 未完成 | - | 5个进程实现 |
| M5：实时性达标 | 第21周 (2026-10-11) | ⏸️ 未完成 | - | 2ms应答99.9% |
| M6：可靠性达标 | 第25周 (2026-11-08) | ⏸️ 未完成 | - | 故障恢复机制 |
| M7：多核异构原型 | 第31周 (2026-12-20) | ⏸️ 未完成 | - | R5F+A53原型 |

---

## 变更记录

| 日期 | 变更内容 | 变更人 | 备注 |
|------|---------|--------|------|
| 2026-05-17 | 创建进度跟踪文档 | - | 初始版本 |
| 2026-05-17 | 开始阶段1：T1.1 OSAL实时调度API | - | 第一个任务 |

---

## 问题和风险

| 问题ID | 问题描述 | 严重程度 | 状态 | 发现时间 | 解决时间 | 解决方案 |
|--------|---------|---------|------|----------|----------|----------|
| - | - | - | - | - | - | - |

---

## 备注

- 本文档实时更新，记录每个任务的详细进度
- 每完成一个任务，更新对应的状态和完成时间
- 遇到问题及时记录到"问题和风险"章节
- 每周更新一次总体进度百分比
