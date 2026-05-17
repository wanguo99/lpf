# APP层详细设计

## 1. APP 设计 （业务进程）

### 1.1 进程架构（纯A53四核）

```text
╔═══════════════════════════════════════════════════════════════════════════════════════╗
║                    ARM Cortex-A53 (4核Linux) - 纯A53架构                               ║
╠═══════════════════════════════════════════════════════════════════════════════════════╣
║                          核心进程部署                                                   ║
║  ┌─────────────────────────────────────────────────────────────────────────────────┐  ║
║  │ CPU0: Telecommand进程 (实时遥控)                                                 │  ║
║  │   • SCHED_FIFO优先级99                                                           │  ║
║  │   • mlockall内存锁定                                                             │  ║
║  │   • CAN接收和遥控命令处理 (<1ms)                                                 │  ║
║  │   • 从共享内存缓存读取遥测 (<50μs)                                               │  ║
║  │   • 2ms内应答                                                                    │  ║
║  ├─────────────────────────────────────────────────────────────────────────────────┤  ║
║  │ CPU1: Telemetry进程 (后台遥测)                                                   │  ║
║  │   • SCHED_OTHER普通调度                                                          │  ║
║  │   • 后台遥测采集 (100ms周期)                                                     │  ║
║  │   • 更新共享内存缓存                                                             │  ║
║  │   • 更新时间戳和新鲜度标记                                                       │  ║
║  ├─────────────────────────────────────────────────────────────────────────────────┤  ║
║  │ CPU2/3: 预留                                                                     │  ║
║  │   • Supervisor进程 (监控和自动重启)                                              │  ║
║  │   • Logger进程 (日志收集和持久化)                                                │  ║
║  └─────────────────────────────────────────────────────────────────────────────────┘  ║
╚═══════════════════════════════════════════════════════════════════════════════════════╝
                    │                                      │
        ┌───────────┴──────────────────────────────────────┴───────────┐
        │                                                                │
╔═══════▼═══════════════════════╗                 ╔═══════▼═══════════════════════╗
║  Telecommand Process          ║                 ║  Telemetry Process            ║
║   (实时遥控，CPU0)            ║                 ║  (后台遥测，CPU1)             ║
╠═══════════════════════════════╣                 ╠═══════════════════════════════╣
║ SCHED_FIFO Priority: 99       ║                 ║ SCHED_OTHER                   ║
║ CPU: CPU0 (独占)              ║                 ║ CPU: CPU1                     ║
║ mlockall (内存锁定)           ║                 ║                               ║
╠═══════════════════════════════╣                 ╠═══════════════════════════════╣
║ 【核心功能】                  ║                 ║ 【核心功能】                  ║
║ • CAN接收                     ║                 ║ • 遥测数据采集                ║
║ • 遥控命令处理                ║                 ║ • 周期性设备查询              ║
║ • 缓存遥测读取 (<50μs)        ║                 ║ • 共享内存缓存更新            ║
║ • 2ms内应答                   ║                 ║ • 新鲜度标记更新              ║
║ • 失效映射处理                ║                 ║ • 时间戳维护                  ║
╠═══════════════════════════════╣                 ╠═══════════════════════════════╣
║ 【内部线程】                  ║                 ║ 【内部线程】                  ║
║ ├─ CAN_RX_Handler             ║                 ║ ├─ BMC采集线程                ║
║ │  (最高优先级)               ║                 ║ │  (Redfish/IPMI)             ║
║ ├─ TC_Command_Handler         ║                 ║ ├─ MCU采集线程                ║
║ │  (高优先级)                 ║                 ║ │  (CAN协议)                  ║
║ └─ TM_Cache_Reader            ║                 ║ └─ 缓存更新线程               ║
║    (中优先级)                 ║                 ║    (共享内存写入)             ║
╚═══════════════════════════════╝                 ╚═══════════════════════════════╝
        │                                                  │
        └──────────────────────┬───────────────────────────┘
                               │
        ┌──────────────────────▼──────────────────────────────────────┐
        │              进程间通信 (POSIX共享内存)                      │
        │  • POSIX共享内存 (遥测缓存, <50μs读取, <10μs写入)           │
        │  • pthread_rwlock (并发保护)                                │
        │  • 时间戳 + 有效期 + 新鲜度标志                             │
        └──────────────────────┬──────────────────────────────────────┘
                               │
        ┌──────────────────────▼──────────────────────────────────────┐
        │                    Shared Memory (共享内存区)                │
        ├──────────────────────────────────────────────────────────────┤
        │ • 遥测缓存 (ACL_TM_Cache, Telemetry写/Telecommand读)         │
        │ • 缓存条目 (每个遥测项: 数据+时间戳+有效期+新鲜度+锁)        │
        │ • 统一缓存模型 (所有遥测从缓存读取)                          │
        └──────────────────────────────────────────────────────────────┘
```



### 1.2 Telecommand进程（实时遥控 - CPU0）

**职责**：
- CAN接收和遥控命令处理（<1ms）
- 从共享内存缓存读取遥测（<50μs）
- 2ms内应答
- 失效映射处理（遥控命令执行后标记相关遥测为STALE）

**实时保证**：
- SCHED_FIFO优先级99（最高实时优先级）
- CPU亲和性绑定到CPU0（独占）
- mlockall内存锁定（防止页错误）
- 无阻塞操作（所有遥测从缓存读取）

**设计原则**：
- 代码路径最短化（<1.5ms总延迟）
- 无动态内存分配
- 无阻塞I/O操作
- 无复杂计算

**实现示例**：
```c
// apps/telecommand/telecommand_main.c

int main(int argc, char *argv[])
{
    // 1. 设置实时调度策略
    OSAL_SchedSetPolicy(OSAL_THREAD_SELF, OSAL_SCHED_FIFO, 99);
    
    // 2. 绑定到CPU0
    OSAL_SchedSetAffinity(OSAL_THREAD_SELF, 0);
    
    // 3. 锁定内存防止页错误
    OSAL_MemLock(true);
    
    // 4. 初始化ACL配置
    ACL_Init();
    
    // 5. 初始化遥测缓存（打开共享内存）
    ACL_TM_Cache_Init();
    
    // 6. 初始化CAN接口
    hal_can_handle_t can_handle;
    HAL_CAN_Init("can0", 500000, &can_handle);
    
    // 7. 主循环：接收CAN命令并处理
    while (running) {
        can_frame_t frame;
        int32_t ret = HAL_CAN_Receive(can_handle, &frame, 100);
        
        if (ret == OSAL_SUCCESS) {
            handle_can_command(&frame);
        }
    }
    
    return 0;
}

// 处理CAN命令
void handle_can_command(const can_frame_t *frame)
{
    uint64_t start_us = OSAL_GetTimeUs();
    
    // 解析命令类型
    uint8_t cmd_type = frame->data[0];
    
    if (cmd_type == CMD_TYPE_TELECOMMAND) {
        // 遥控命令处理
        pmc_tc_function_t tc_func = frame->data[1];
        handle_telecommand(tc_func, frame);
        
    } else if (cmd_type == CMD_TYPE_TELEMETRY) {
        // 遥测请求处理
        pmc_tm_function_t tm_func = frame->data[1];
        handle_telemetry_request(tm_func, frame);
    }
    
    uint64_t elapsed_us = OSAL_GetTimeUs() - start_us;
    LOG_DEBUG("TC", "命令处理耗时: %llu μs", elapsed_us);
}

// 处理遥控命令
void handle_telecommand(pmc_tc_function_t tc_func, const can_frame_t *frame)
{
    // 1. 查询ACL配置（O(1)）
    const acl_tc_config_t *cfg = ACL_GetTcConfig(tc_func);
    if (!cfg || !cfg->enabled) {
        send_error_response(frame, ERR_NOT_CONFIGURED);
        return;
    }
    
    // 2. 调用PDL执行遥控命令
    int32_t ret = execute_tc_command(cfg, frame);
    
    // 3. 失效相关遥测缓存
    if (ret == OSAL_SUCCESS) {
        ACL_InvalidateAffectedTelemetry(tc_func);
    }
    
    // 4. 发送应答
    send_tc_response(frame, ret);
}

// 处理遥测请求（从缓存读取）
void handle_telemetry_request(pmc_tm_function_t tm_func, const can_frame_t *frame)
{
    // 1. 查询ACL配置（O(1)）
    const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_func);
    if (!cfg || !cfg->enabled) {
        send_error_response(frame, ERR_NOT_CONFIGURED);
        return;
    }
    
    // 2. 从共享内存缓存读取（<50μs）
    uint8_t data[256];
    tm_freshness_t freshness;
    int32_t ret = ACL_TM_Cache_Get(tm_func, data, sizeof(data), &freshness);
    
    if (ret != OSAL_SUCCESS) {
        send_error_response(frame, ERR_CACHE_READ_FAILED);
        return;
    }
    
    // 3. 发送应答（携带新鲜度标记）
    send_tm_response(frame, data, freshness);
}
```

**性能指标**：
- CAN接收到应答发送：<1.5ms（99.9%）
- 遥测缓存读取：<50μs
- 遥控命令执行：<1ms（取决于PDL层）

---

### 1.3 Telemetry进程（后台遥测 - CPU1）

**职责**：
- 周期性采集设备遥测数据（100ms周期）
- 更新共享内存缓存
- 更新时间戳和新鲜度标记
- 处理失效的遥测项（优先重新采集）

**调度策略**：
- SCHED_OTHER普通调度（后台进程）
- CPU亲和性绑定到CPU1
- 不锁定内存（允许页交换）

**设计原则**：
- 周期性采集，不阻塞实时进程
- 批量更新缓存，减少锁竞争
- 优先处理STALE状态的遥测项
- 容忍采集失败，记录错误但不中断

**实现示例**：
```c
// apps/telemetry/telemetry_main.c

int main(int argc, char *argv[])
{
    // 1. 设置普通调度策略
    OSAL_SchedSetPolicy(OSAL_THREAD_SELF, OSAL_SCHED_OTHER, 0);
    
    // 2. 绑定到CPU1
    OSAL_SchedSetAffinity(OSAL_THREAD_SELF, 1);
    
    // 3. 初始化ACL配置
    ACL_Init();
    
    // 4. 初始化遥测缓存（打开共享内存）
    ACL_TM_Cache_Init();
    
    // 5. 初始化PDL设备
    init_pdl_devices();
    
    // 6. 主循环：周期性采集遥测
    while (running) {
        uint64_t start_us = OSAL_GetTimeUs();
        
        // 采集所有遥测项
        collect_all_telemetry();
        
        uint64_t elapsed_us = OSAL_GetTimeUs() - start_us;
        LOG_DEBUG("TM", "采集周期耗时: %llu μs", elapsed_us);
        
        // 100ms周期
        OSAL_msleep(100);
    }
    
    return 0;
}

// 采集所有遥测项
void collect_all_telemetry(void)
{
    // 遍历所有遥测配置
    for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
        const acl_tm_config_t *cfg = ACL_GetTmConfig(i);
        
        if (!cfg || !cfg->enabled) {
            continue;
        }
        
        // 检查是否需要更新（基于update_period_ms）
        if (!need_update(i, cfg->update_period_ms)) {
            continue;
        }
        
        // 采集遥测数据
        collect_single_telemetry(i, cfg);
    }
}

// 采集单个遥测项
void collect_single_telemetry(pmc_tm_function_t tm_func, const acl_tm_config_t *cfg)
{
    uint8_t data[256];
    uint32_t size = 0;
    int32_t ret = OSAL_ERR_GENERIC;
    
    // 根据设备类型调用PDL接口
    switch (cfg->device_type) {
        case ACL_DEVICE_BMC:
            ret = collect_bmc_telemetry(tm_func, cfg->logic_index, data, &size);
            break;
            
        case ACL_DEVICE_MCU:
            ret = collect_mcu_telemetry(tm_func, cfg->logic_index, data, &size);
            break;
            
        case ACL_DEVICE_FPGA:
            ret = collect_fpga_telemetry(tm_func, cfg->logic_index, data, &size);
            break;
            
        default:
            LOG_ERROR("TM", "未知设备类型: %d", cfg->device_type);
            return;
    }
    
    // 更新缓存
    if (ret == OSAL_SUCCESS) {
        ACL_TM_Cache_Update(tm_func, data, size);
        LOG_DEBUG("TM", "更新遥测缓存: %d", tm_func);
    } else {
        LOG_ERROR("TM", "采集遥测失败: %d, ret=%d", tm_func, ret);
    }
}

// 采集BMC遥测
int32_t collect_bmc_telemetry(pmc_tm_function_t tm_func, uint32_t bmc_index,
                               uint8_t *data, uint32_t *size)
{
    bmc_sensor_data_t sensor_data;
    int32_t ret = PDL_BMC_ReadSensors(bmc_index, &sensor_data);
    
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    // 根据遥测类型提取数据
    switch (tm_func) {
        case TM_SERVER_TEMP:
            *(uint16_t*)data = sensor_data.cpu_temp;
            *size = sizeof(uint16_t);
            break;
            
        case TM_SERVER_POWER_STATUS:
            *(uint8_t*)data = sensor_data.power_status;
            *size = sizeof(uint8_t);
            break;
            
        // ... 其他遥测项
        
        default:
            return OSAL_ERR_NOT_FOUND;
    }
    
    return OSAL_SUCCESS;
}

// 采集MCU遥测
int32_t collect_mcu_telemetry(pmc_tm_function_t tm_func, uint32_t mcu_index,
                               uint8_t *data, uint32_t *size)
{
    mcu_status_t mcu_status;
    int32_t ret = PDL_MCU_GetStatus(mcu_index, &mcu_status);
    
    if (ret != OSAL_SUCCESS) {
        return ret;
    }
    
    // 根据遥测类型提取数据
    switch (tm_func) {
        case TM_MCU_STATUS:
            *(uint8_t*)data = mcu_status.status;
            *size = sizeof(uint8_t);
            break;
            
        case TM_MCU_TEMP:
            *(uint16_t*)data = mcu_status.temperature;
            *size = sizeof(uint16_t);
            break;
            
        // ... 其他遥测项
        
        default:
            return OSAL_ERR_NOT_FOUND;
    }
    
    return OSAL_SUCCESS;
}

// 检查是否需要更新
bool need_update(pmc_tm_function_t tm_func, uint32_t update_period_ms)
{
    static uint64_t last_update_time[TM_FUNC_MAX] = {0};
    
    uint64_t now_us = OSAL_GetTimeUs();
    uint64_t elapsed_us = now_us - last_update_time[tm_func];
    
    if (elapsed_us >= update_period_ms * 1000) {
        last_update_time[tm_func] = now_us;
        return true;


### 1.4 Supervisor进程（健康监控 - CPU2，预留）

**职责**：
- 监控所有进程健康状态（心跳检测）
- 检测进程崩溃或卡死
- 自动重启故障进程
- 系统级故障恢复
- 硬件看门狗喂狗

**监控机制**：

```c
// Supervisor进程主循环
void supervisor_main_loop(void) {
    while (running) {
        // 1. 检查Telecommand进程心跳
        if (check_process_heartbeat("telecommand", 2000) == TIMEOUT) {
            LOG_ERROR("SUPERVISOR", "Telecommand进程心跳超时，重启中...");
            restart_process("telecommand");
        }
        
        // 2. 检查Telemetry进程心跳
        if (check_process_heartbeat("telemetry", 2000) == TIMEOUT) {
            LOG_ERROR("SUPERVISOR", "Telemetry进程心跳超时，重启中...");
            restart_process("telemetry");
        }
        
        // 3. 喂硬件看门狗
        HAL_Watchdog_Feed();
        
        // 4. 休眠500ms
        OSAL_msleep(500);
    }
}
```

**故障恢复策略**：

| 故障类型 | 检测方式 | 恢复策略 | 恢复时间 |
|---------|---------|---------|---------|
| **进程崩溃** | 信号捕获 | Supervisor重启进程 | <1秒 |
| **进程卡死** | 心跳超时（2秒） | Supervisor杀死并重启 | <2秒 |
| **Supervisor崩溃** | 硬件看门狗 | 系统复位 | <5秒 |
| **系统死锁** | 硬件看门狗 | 系统复位 | <5秒 |

**进程重启实现**：

```c
// 进程重启函数
int32_t restart_process(const char *process_name) {
    // 1. 记录重启日志
    LOG_WARNING("SUPERVISOR", "重启进程: %s", process_name);
    
    // 2. 杀死旧进程
    pid_t pid = get_process_pid(process_name);
    if (pid > 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    
    // 3. 启动新进程
    pid = fork();
    if (pid == 0) {
        // 子进程：执行目标程序
        execl(process_name, process_name, NULL);
        exit(1);
    }
    
    // 4. 更新进程表
    update_process_table(process_name, pid);
    
    LOG_INFO("SUPERVISOR", "进程 %s 已重启，PID=%d", process_name, pid);
    
    return OSAL_SUCCESS;
}
```

---

### 1.5 Logger进程（日志管理 - CPU3，预留）

**职责**：
- 收集所有进程的日志
- 日志持久化到存储
- 日志轮转和压缩
- 崩溃日志收集

**日志收集机制**：

```c
// Logger进程主循环
void logger_main_loop(void) {
    while (running) {
        // 1. 从共享内存读取日志
        log_entry_t log;
        while (read_log_from_shm(&log) == OSAL_SUCCESS) {
            // 2. 格式化日志
            char formatted[512];
            format_log(&log, formatted, sizeof(formatted));
            
            // 3. 写入日志文件
            write_log_to_file(formatted);
            
            // 4. 检查是否需要轮转
            if (should_rotate_log()) {
                rotate_log_file();
            }
        }
        
        // 5. 休眠100ms
        OSAL_msleep(100);
    }
}
```

**日志格式**：

```text
[时间戳] [级别] [进程] [模块] 消息内容

示例：
[2026-05-17 20:30:15.123456] [INFO] [telecommand] [TC] 遥控命令执行成功: TC_SERVER_POWER_ON
[2026-05-17 20:30:15.234567] [ERROR] [telemetry] [TM] BMC通信超时: timeout=5000ms
```

---

### 1.6 进程间通信（IPC）

**POSIX共享内存**：

```c
// 共享内存布局
typedef struct {
    // 遥测缓存（4MB）
    acl_tm_cache_t tm_cache;
    
    // 心跳表（所有进程）
    struct {
        uint64_t telecommand_heartbeat_us;
        uint64_t telemetry_heartbeat_us;
        uint64_t supervisor_heartbeat_us;
        uint64_t logger_heartbeat_us;
    } heartbeat;
    
    // 日志环形缓冲区（1MB）
    struct {
        uint32_t write_pos;
        uint32_t read_pos;
        log_entry_t entries[4096];
    } log_buffer;
    
} ipc_shared_memory_t;
```

**共享内存初始化**：

```c
// 创建共享内存
int32_t init_shared_memory(void) {
    // 1. 创建POSIX共享内存
    int fd = shm_open("/ems_ipc", O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        return OSAL_ERR_GENERIC;
    }
    
    // 2. 设置大小
    if (ftruncate(fd, sizeof(ipc_shared_memory_t)) < 0) {
        close(fd);
        return OSAL_ERR_GENERIC;
    }
    
    // 3. 映射到进程地址空间
    void *addr = mmap(NULL, sizeof(ipc_shared_memory_t),
                      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return OSAL_ERR_GENERIC;
    }
    
    // 4. 初始化共享内存
    ipc_shared_memory_t *shm = (ipc_shared_memory_t *)addr;
    memset(shm, 0, sizeof(ipc_shared_memory_t));
    
    // 5. 初始化遥测缓存
    ACL_TelemetryCache_Init();
    
    return OSAL_SUCCESS;
}
```

---

### 1.7 实时性能分析

**Telecommand进程延迟分解**：

| 阶段 | 延迟 | 说明 |
|------|------|------|
| CAN接收 | <20μs | 硬件中断 + 驱动处理 |
| 命令解析 | <5μs | 解析CAN帧，提取命令ID和参数 |
| ACL配置查询 | <5μs | O(1)数组索引 |
| PDL设备操作 | <1ms | BMC/MCU/FPGA命令执行 |
| 失效映射处理 | <10μs | 标记相关遥测为STALE |
| CAN应答 | <20μs | 构造应答帧并发送 |
| **总计** | **<1.5ms** | **满足2ms要求** |

**遥测读取延迟分解**：

| 阶段 | 延迟 | 说明 |
|------|------|------|
| CAN接收 | <20μs | 硬件中断 + 驱动处理 |
| 命令解析 | <5μs | 解析遥测请求 |
| 缓存读取 | <50μs | 从共享内存读取（pthread_rwlock） |
| 新鲜度检查 | <1μs | 计算数据年龄 |
| CAN应答 | <20μs | 构造应答帧并发送 |
| **总计** | **<100μs** | **远小于2ms要求** |

**CPU负载估算**：

| 进程 | CPU核心 | 负载 | 说明 |
|------|---------|------|------|
| Telecommand | CPU0 | <10% | 遥控命令处理（非周期性） |
| Telemetry | CPU1 | <20% | 100ms周期采集 |
| Supervisor | CPU2 | <5% | 500ms周期监控 |
| Logger | CPU3 | <5% | 日志收集 |

---

### 1.8 故障场景与恢复

**场景1：Telecommand进程崩溃**

```text
故障：Telecommand进程因段错误崩溃
检测：Supervisor进程检测到心跳超时（2秒）
恢复：
  1. Supervisor杀死僵尸进程
  2. 重新启动Telecommand进程
  3. Telecommand进程重新初始化
  4. 恢复正常服务
恢复时间：<1秒
影响：1秒内无法处理遥控命令
```

**场景2：Telemetry进程卡死**

```text
故障：Telemetry进程因死锁卡死
检测：Supervisor进程检测到心跳超时（2秒）
恢复：
  1. Supervisor发送SIGKILL杀死进程
  2. 重新启动Telemetry进程
  3. Telemetry进程重新开始采集
恢复时间：<2秒
影响：遥测缓存数据变STALE，但Telecommand仍可应答
```

**场景3：Supervisor进程崩溃**

```text
故障：Supervisor进程崩溃
检测：硬件看门狗超时（5秒）
恢复：
  1. 硬件看门狗触发系统复位
  2. 系统重启
  3. 所有进程重新启动
恢复时间：<5秒
影响：5秒内系统不可用
```

**场景4：系统死锁**

```text
故障：所有进程死锁，无法喂狗
检测：硬件看门狗超时（5秒）
恢复：
  1. 硬件看门狗触发系统复位
  2. 系统重启
恢复时间：<5秒
影响：5秒内系统不可用
```

---

### 1.9 配置和部署

**系统配置**：

```bash
# /etc/systemd/system/ems-telecommand.service
[Unit]
Description=EMS Telecommand Process
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/telecommand
Restart=no
User=root

[Install]
WantedBy=multi-user.target

# /etc/systemd/system/ems-telemetry.service
[Unit]
Description=EMS Telemetry Process
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/telemetry
Restart=no
User=root

[Install]
WantedBy=multi-user.target

# /etc/systemd/system/ems-supervisor.service
[Unit]
Description=EMS Supervisor Process
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/supervisor
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

**内核参数配置**：

```bash
# /etc/sysctl.conf

# CPU隔离（可选）
# 在内核启动参数中添加：isolcpus=0,1

# 实时调度参数
kernel.sched_rt_runtime_us = -1  # 允许实时进程占用100% CPU

# 共享内存大小
kernel.shmmax = 8388608  # 8MB
kernel.shmall = 2097152  # 8MB / 4KB页

# 看门狗超时
kernel.watchdog_thresh = 5  # 5秒
```

**启动脚本**：

```bash
#!/bin/bash
# /usr/local/bin/ems-start.sh

# 1. 加载内核模块
modprobe can
modprobe can_raw

# 2. 配置CAN接口
ip link set can0 type can bitrate 1000000
ip link set can0 up

# 3. 创建共享内存
rm -f /dev/shm/ems_ipc
rm -f /dev/shm/acl_tm_cache

# 4. 启动进程（按顺序）
systemctl start ems-supervisor
sleep 1
systemctl start ems-telemetry
sleep 1
systemctl start ems-telecommand

# 5. 检查进程状态
systemctl status ems-supervisor
systemctl status ems-telemetry
systemctl status ems-telecommand
```

---

### 1.10 测试和验证

**单元测试**：

```c
// 测试Telecommand进程遥控处理
void test_telecommand_handling(void) {
    // 1. 初始化
    ACL_Init();
    ACL_TelemetryCache_Init();
    
    // 2. 模拟遥控命令
    pmc_tc_function_t cmd = TC_SERVER_POWER_ON;
    uint32_t param = 0;
    
    // 3. 执行遥控
    int32_t ret = handle_telecommand(cmd, param);
    
    // 4. 验证结果
    assert(ret == OSAL_SUCCESS);
    
    // 5. 验证失效映射
    telemetry_response_t response;
    ret = ACL_TelemetryCache_Read(TM_SERVER_POWER_STATUS, &response);
    assert(response.freshness == TM_STATUS_STALE);
}
```

**性能测试**：

```c
// 测试遥控命令延迟
void test_telecommand_latency(void) {
    uint64_t start, end;
    uint64_t latencies[1000];
    
    for (int i = 0; i < 1000; i++) {
        start = OSAL_GetTimeUs();
        handle_telecommand(TC_SERVER_POWER_ON, 0);
        end = OSAL_GetTimeUs();
        latencies[i] = end - start;
    }
    
    // 统计延迟
    uint64_t max = calculate_max(latencies, 1000);
    uint64_t avg = calculate_avg(latencies, 1000);
    uint64_t p99 = calculate_percentile(latencies, 1000, 99);
    
    printf("遥控延迟统计：\n");
    printf("  最大延迟: %llu μs\n", max);
    printf("  平均延迟: %llu μs\n", avg);
    printf("  P99延迟: %llu μs\n", p99);
    
    // 验证满足2ms要求
    assert(max < 2000);
}
```

**压力测试**：

```c
// 测试高频遥控命令
void test_high_frequency_telecommand(void) {
    // 1. 启动遥控命令发送线程（1000 Hz）
    pthread_t sender_thread;
    pthread_create(&sender_thread, NULL, send_telecommand_loop, NULL);
    
    // 2. 运行1分钟
    sleep(60);
    
    // 3. 停止发送
    stop_sender_thread();
    
    // 4. 检查统计信息
    uint32_t total_sent = get_total_sent();
    uint32_t total_success = get_total_success();
    uint32_t total_failed = get_total_failed();
    
    printf("压力测试结果：\n");
    printf("  发送总数: %u\n", total_sent);
    printf("  成功数: %u\n", total_success);
    printf("  失败数: %u\n", total_failed);
    printf("  成功率: %.2f%%\n", (float)total_success / total_sent * 100);
    
    // 验证成功率 > 99%
    assert((float)total_success / total_sent > 0.99);
}
```

---


3. 崩溃日志（/var/log/pmc/crash/crash_20260516_102350.log）

``` text
=== Process Crash Report ===
Time: 2026-05-16 10:23:50.123456
Process: telemetry
PID: 1234
Signal: SIGSEGV (Segmentation fault)
Address: 0x00000000 (NULL pointer dereference)

=== Last 10 Log Entries ===
2026-05-16 10:23:49.123456 [INFO] [TM] 开始采集遥测数据
2026-05-16 10:23:49.234567 [INFO] [TM] BMC传感器读取成功
2026-05-16 10:23:49.345678 [ERROR] [TM] MCU通信失败: timeout
2026-05-16 10:23:50.123456 [ERROR] [TM] 进程崩溃: SIGSEGV

=== Backtrace ===
#0  0x00007f1234567890 in cache_collector_thread () at telemetry.c:123
#1  0x00007f1234567900 in pthread_start () at pthread.c:456
#2  0x00007f1234567910 in clone () at clone.S:78

=== Coredump ===
Coredump saved to: /var/log/pmc/crash/core.1234

=== Recovery Action ===
Supervisor restarted telemetry process at 2026-05-16 10:23:51.000000
```

---



### 1.2. 可靠性设计

#### 1.2.1 三级故障恢复

| 级别              | 触发条件                     | 恢复机制                 | 恢复时间 | 适用场景                  |
| ----------------- | ---------------------------- | ------------------------ | -------- | ------------------------- |
| **级别1：线程级** | 线程崩溃                     | 进程内检测并重启线程     | <100ms   | 非关键线程（如日志线程）  |
| **级别2：进程级** | 进程崩溃                     | Supervisor检测并重启进程 | <1秒     | 所有进程（5次/300秒限制） |
| **级别3：系统级** | Supervisor崩溃或进程重启超限 | 硬件看门狗触发系统复位   | 10秒     | 系统级故障，从主分区启动  |

#### 1.2.2 进程隔离效果

| 场景      | 崩溃进程        | 检测机制           | 恢复时间 | 影响范围                                                     | 恢复后状态                                       |
| --------- | --------------- | ------------------ | -------- | ------------------------------------------------------------ | ------------------------------------------------ |
| **场景1** | Telemetry进程   | Supervisor心跳超时 | <1秒     | 缓存型遥测返回旧数据（标记stale）<br>实时型遥测正常<br>遥控功能不受影响 | 1秒内恢复正常采集                                |
| **场景2** | Telecommand进程 | Supervisor心跳超时 | <1秒     | 遥控遥测短暂中断<br>卫星平台检测到心跳丢失                   | 立即恢复通信<br>连续崩溃5次→系统复位             |
| **场景3** | Firmware进程    | Supervisor心跳超时 | <1秒     | 固件升级中断<br>遥控遥测不受影响                             | 未完成→保持当前分区<br>已完成→下次从备份分区启动 |

#### 1.2.3 辐射容错

单粒子翻转（SEU）防护：

``` C
// 1. 进程级隔离
//    SEU翻转只影响单个进程地址空间
//    其他进程不受影响

// 2. 关键数据保护
typedef struct {
  uint32_t magic;      // 魔数：0xDEADBEEF
  uint32_t data;       // 实际数据
  uint32_t crc32;      // CRC32校验
} protected_data_t;

// 3. 周期性校验（每秒）
void verify_critical_data(protected_data_t *data)
{
  if (data->magic != 0xDEADBEEF || calc_crc32(&data->data) != data->crc32) {
	  LOG_ERROR("SEU", "数据损坏检测，恢复中.");
	  restore_from_backup(data);
  }
}

// 4. 共享内存ECC保护（硬件支持）
//    使用支持ECC的DDR内存
//    单比特错误自动纠正
//    双比特错误触发中断

单粒子闩锁（SEL）防护：
// 1. 硬件看门狗强制复位
//    Supervisor喂狗周期：5秒
//    看门狗超时：10秒
//    超时后硬件复位

// 2. 电源监控芯片
//    监控异常电流（SEL特征）
//    检测到异常 → 触发电源复位
```

#### 1.2.4 降级运行模式

| 模式          | 运行进程                        | 可用功能                                | 受限功能             | 触发条件             |
| ------------- | ------------------------------- | --------------------------------------- | -------------------- | -------------------- |
| **正常模式**  | 全部4个进程                     | 遥控、遥测（缓存+实时）、固件升级、日志 | 无                   | 系统正常运行         |
| **降级模式1** | Telecommand + Firmware + Logger | 遥控、实时型遥测、固件升级、日志        | 缓存型遥测返回旧数据 | Telemetry进程崩溃    |
| **降级模式2** | Telecommand + Logger            | 遥控、日志                              | 遥测功能不可用       | Telemetry连续崩溃5次 |
| **安全模式**  | 仅Supervisor                    | 喂看门狗                                | 遥控遥测全部不可用   | Telecommand进程禁用  |
| **系统复位**  | 重启中                          | 无                                      | 全部功能暂停         | 硬件看门狗触发       |

---

### 1.3. 多核异构优化策略

#### 1.3.1 CPU调度优化（A53侧）

``` C
// 1. Supervisor进程：实时调度，高优先级，绑定CPU0
struct sched_param param;
param.sched_priority = 50;
sched_setscheduler(0, SCHED_FIFO, &param);
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// 2. Telemetry进程：普通调度，低优先级，CPU1-3
param.sched_priority = 0;
sched_setscheduler(0, SCHED_OTHER, &param);
setpriority(PRIO_PROCESS, 0, 10);  // nice=10
CPU_ZERO(&cpuset);
for (int i = 1; i <= 3; i++) CPU_SET(i, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// 3. Firmware进程：批处理调度，最低优先级，CPU1-3
sched_setscheduler(0, SCHED_BATCH, &param);
setpriority(PRIO_PROCESS, 0, 19);  // nice=19

// 4. Logger进程：普通调度，低优先级，CPU1-3
param.sched_priority = 0;
sched_setscheduler(0, SCHED_OTHER, &param);
setpriority(PRIO_PROCESS, 0, 10);  // nice=10
```

**A53侧调度效果（多核负载均衡）**：

| 进程        | 调度策略    | 优先级  | CPU绑定 | CPU时间片分配         | 说明                      |
| ----------- | ----------- | ------- | ------- | --------------------- | ------------------------- |
| Supervisor  | SCHED_FIFO  | 50      | CPU0    | 独占CPU0（<5%负载）   | 监控进程，高优先级        |
| Telemetry   | SCHED_OTHER | nice=10 | CPU1-3  | 共享CPU1-3（<10%负载）| 后台采集，负载均衡        |
| Logger      | SCHED_OTHER | nice=10 | CPU1-3  | 共享CPU1-3（<3%负载） | 日志收集，低优先级        |
| Firmware    | SCHED_BATCH | nice=19 | CPU1-3  | 系统空闲时运行        | 最低优先级，升级时独占    |

#### 1.3.2 A53侧任务优先级配置

``` C
// FreeRTOS任务优先级配置（数字越大优先级越高）
#define PRIORITY_WATCHDOG       (configMAX_PRIORITIES - 1)  // 最高优先级：7
#define PRIORITY_CAN_RX         (configMAX_PRIORITIES - 2)  // 次高优先级：6
#define PRIORITY_IPC_HANDLER    (configMAX_PRIORITIES - 3)  // 高优先级：5
#define PRIORITY_REALTIME_TM    (configMAX_PRIORITIES - 4)  // 高优先级：4
#define PRIORITY_TELECONTROL    (configMAX_PRIORITIES - 5)  // 中优先级：3
#define PRIORITY_HEARTBEAT      (configMAX_PRIORITIES - 7)  // 低优先级：1

// 任务创建
xTaskCreateStatic(CAN_RX_Task, "CAN_RX", 2048, NULL, PRIORITY_CAN_RX, 
                  can_rx_stack, &can_rx_tcb);
xTaskCreateStatic(Realtime_TM_Task, "RT_TM", 1024, NULL, PRIORITY_REALTIME_TM,
                  rt_tm_stack, &rt_tm_tcb);
xTaskCreateStatic(Telecontrol_Task, "TC", 1024, NULL, PRIORITY_TELECONTROL,
                  tc_stack, &tc_tcb);
xTaskCreateStatic(IPC_Handler_Task, "IPC", 1024, NULL, PRIORITY_IPC_HANDLER,
                  ipc_stack, &ipc_tcb);
xTaskCreateStatic(Heartbeat_Task, "HB", 512, NULL, PRIORITY_HEARTBEAT,
                  hb_stack, &hb_tcb);
xTaskCreateStatic(Watchdog_Task, "WDT", 512, NULL, PRIORITY_WATCHDOG,
                  wdt_stack, &wdt_tcb);

// 核心亲和性配置（A53双核）
vTaskCoreAffinitySet(can_rx_task_handle, (1 << 0));      // A53_0
vTaskCoreAffinitySet(rt_tm_task_handle, (1 << 0));       // A53_0
vTaskCoreAffinitySet(tc_task_handle, (1 << 1));          // A53_1
vTaskCoreAffinitySet(ipc_task_handle, (1 << 1));         // A53_1
vTaskCoreAffinitySet(hb_task_handle, (1 << 1));          // A53_1
vTaskCoreAffinitySet(wdt_task_handle, (1 << 1));         // A53_1
```

**A53侧调度效果（双核实时）**：

| 任务            | 优先级 | 核心  | 说明                      |
| --------------- | ------ | ----- | ------------------------- |
| Watchdog_Task   | 7      | A53_1 | 最高优先级，喂狗          |
| CAN_RX_Task     | 6      | A53_0 | 次高优先级，2ms应答       |
| IPC_Handler     | 5      | A53_1 | 高优先级，核间通信        |
| Realtime_TM     | 4      | A53_0 | 高优先级，实时遥测        |
| Telecontrol     | 3      | A53_1 | 中优先级，遥控执行        |
| Heartbeat       | 1      | A53_1 | 低优先级，心跳更新        |

**实时性保证（多核异构）**：

| 场景          | 处理流程               | 实时性保证   | 核心 |
| ------------- | ---------------------- | ------------ | ---- |
| 遥控命令到达  | A53 CAN中断 → CAN_RX_Task | <1.13ms应答 | A53_0 |
| 快遥/慢遥到达 | A53 CAN中断 → CAN_RX_Task | <100μs应答  | A53_0 |
| 实时遥测查询  | A53 Realtime_TM_Task   | <700μs完成  | A53_0 |
| 后台采集      | A53 Telemetry进程      | 不影响实时性 | A53 CPU1-3 |
| 核间通信      | RPMsg + 共享内存       | <100μs延迟  | A53↔A53 |

#### 1.3.3 中断优化（多核异构）

**A53侧（Linux）**：

``` shell
# 1. 禁用CPU0的非关键中断（专用于Supervisor）
echo 0 > /proc/irq/XX/smp_affinity  # 禁用非关键中断

# 2. 网络中断绑定到CPU1-3
echo e > /proc/irq/YY/smp_affinity  # 0b1110 = CPU1-3

# 3. 使用PREEMPT_RT内核（可选）
# 提升Linux实时性，但A53已保证硬实时
```

**A53侧（FreeRTOS）**：

``` C
// 1. CAN中断配置（最高优先级）
HwiP_Params hwiParams;
HwiP_Params_init(&hwiParams);
hwiParams.priority = 0;  // 最高优先级
hwiParams.arg = (uintptr_t)can_handle;
HwiP_create(CAN0_INT, CAN_ISR, &hwiParams);

// 2. Mailbox中断配置（核间通信）
hwiParams.priority = 1;  // 次高优先级
HwiP_create(MAILBOX_INT, Mailbox_ISR, &hwiParams);

// 3. 中断嵌套配置
// FreeRTOS允许中断嵌套，CAN中断可抢占Mailbox中断
```

#### 1.3.4 内存优化（多核异构）

**A53侧（DDR）**：

``` C
// 1. 预分配所有内存（避免运行时分配）
static telemetry_cache_t g_tm_cache;
static uint8_t g_log_buffer[1024 * 1024];

// 2. 锁定Supervisor进程内存（避免swap和缺页中断）
mlockall(MCL_CURRENT | MCL_FUTURE);

// 3. 共享内存配置为Non-cacheable（与A53共享）
int shm_fd = shm_open("/ipc_shm", O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, 16 * 1024 * 1024);  // 16MB
void *shm = mmap(NULL, 16 * 1024 * 1024, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_NONCACHE, shm_fd, 0);  // Non-cacheable
```

**A53侧（TCM + DDR）**：

``` C
// 1. TCM内存布局（256KB，零等待）
#pragma DATA_SECTION(g_acl_table, ".tcm_data")
acl_config_t g_acl_table[256];  // ACL查找表放在TCM

#pragma CODE_SECTION(CAN_RX_Task, ".tcm_code")
void CAN_RX_Task(void *pvParameters) {
  // 关键代码放在TCM
}

// 2. 共享内存映射（固定物理地址）
#define IPC_SHM_BASE  0x70000000  // DDR中的共享内存区
ipc_shared_memory_t *shm = (ipc_shared_memory_t *)IPC_SHM_BASE;

// 3. 预分配所有缓冲区（静态分配）
static uint8_t can_rx_buffer[1024] __attribute__((section(".tcm_data")));
static uint8_t can_tx_buffer[1024] __attribute__((section(".tcm_data")));
```

#### 1.3.5 缓存优化（多核异构）

**A53侧（Cacheable DDR）**：

``` C
// 1. 数据结构对齐（避免false sharing）
typedef struct {
  _Atomic uint32_t read_index;
  uint8_t padding1[60];  // 填充到64字节（缓存行大小）
  _Atomic uint32_t write_index;
  uint8_t padding2[60];
} cache_aligned_queue_t;

// 2. 热路径数据局部性
typedef struct {
  // 热数据（频繁访问）
  _Atomic uint64_t heartbeat;
  uint32_t cmd_count;
  uint32_t error_count;

  // 冷数据（偶尔访问）
  char name[64];
  uint32_t config_flags;
} hot_cold_separated_t;

// 3. 预取关键数据
__builtin_prefetch(&g_acl_table, 0, 3);  // 预取ACL查找表

```

---

### 1.4 进程系统配置

```ini
# /etc/pmc/pmc.conf

# PMC系统配置文件

[system]
platform = ti/am625
product = pmc_v1
version = 1.0.0

[supervisor]
heartbeat_interval_ms = 2000
heartbeat_timeout_ms = 5000
watchdog_feed_interval_ms = 5000
max_restart_count = 5
restart_window_sec = 300

[telecommand]
sched_policy = SCHED_FIFO
sched_priority = 99
cpu_affinity = 0
memory_lock = true
can_device = can0
can_bitrate = 500000
cmd_timeout_ms = 100

[telemetry]
sched_policy = SCHED_OTHER
sched_priority = 0
nice = 10
cache_collect_interval_ms = 1000
health_check_interval_ms = 5000
status_snapshot_interval_ms = 10000

[firmware]
sched_policy = SCHED_BATCH
nice = 19
partition_a = /dev/mmcblk0p1
partition_b = /dev/mmcblk0p2
max_boot_attempts = 3

[logger]
sched_policy = SCHED_OTHER
nice = 10
log_level = INFO
log_dir = /var/log/pmc
max_log_size_mb = 10
max_archive_days = 7
crash_log_dir = /var/log/pmc/crash

```

---
