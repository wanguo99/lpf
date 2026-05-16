# 抗辐射设计

## 7. 抗辐射设计

### 7.1 空间辐射效应

#### 7.1.1 辐射类型
```
太空辐射环境：
├─ 单粒子效应（SEE）
│   ├─ 单粒子翻转（SEU）：内存/寄存器位翻转
│   ├─ 单粒子闩锁（SEL）：芯片永久损坏
│   ├─ 单粒子瞬态（SET）：逻辑电路瞬时错误
│   └─ 单粒子烧毁（SEB）：功率器件损坏
├─ 总剂量效应（TID）
│   └─ 长期累积辐射导致性能退化
└─ 位移损伤（DD）
    └─ 晶格结构破坏
```

#### 7.1.2 AM6254抗辐射能力评估
```
AM6254（商用级芯片）：
├─ 无抗辐射加固
├─ 预期SEU率：~10^-6 翻转/位/天（LEO轨道）
├─ 关键数据（32KB）：每天~0.03次翻转
├─ 需要软件缓解措施
└─ 建议配合外部抗辐射器件（电源管理、时钟）
```

### 7.2 ECC内存保护

#### 7.2.1 硬件ECC配置
```c
// A53侧DDR4 ECC配置
void configure_ddr_ecc(void) {
    // 使能DDR4 ECC（检测并纠正单比特错误）
    DDR_CTRL->ECC_CTRL |= DDR_ECC_ENABLE;
    DDR_CTRL->ECC_CTRL |= DDR_ECC_SCRUB_ENABLE;  // 使能后台扫描
    
    // 配置ECC中断
    DDR_CTRL->ECC_INT_EN = DDR_ECC_SINGLE_BIT_ERR | DDR_ECC_DOUBLE_BIT_ERR;
    
    // 注册ECC错误处理函数
    OSAL_InterruptRegister(DDR_ECC_IRQ, ddr_ecc_error_handler, NULL);
}

// R5F侧TCM ECC配置
void configure_tcm_ecc(void) {
    // R5F TCM自带ECC，默认使能
    R5F_TCM_CTRL->ECC_ENABLE = 1;
    
    // 配置ECC错误响应
    R5F_TCM_CTRL->ECC_ERR_RESPONSE = TCM_ECC_CORRECT_AND_LOG;
}

// ECC错误处理
void ddr_ecc_error_handler(void *arg) {
    uint32_t ecc_status = DDR_CTRL->ECC_STATUS;
    uint64_t error_addr = DDR_CTRL->ECC_ERR_ADDR;
    
    if (ecc_status & DDR_ECC_SINGLE_BIT_ERR) {
        // 单比特错误：已自动纠正，记录日志
        LOG_WARNING("ECC", "Single-bit error at 0x%llx, corrected", error_addr);
        ecc_stats.single_bit_errors++;
        
        // 如果同一地址频繁出错，标记为坏块
        if (is_frequent_error(error_addr)) {
            mark_bad_memory_block(error_addr);
        }
    }
    
    if (ecc_status & DDR_ECC_DOUBLE_BIT_ERR) {
        // 双比特错误：无法纠正，触发故障恢复
        LOG_ERROR("ECC", "Double-bit error at 0x%llx, FATAL", error_addr);
        ecc_stats.double_bit_errors++;
        
        // 触发系统复位
        trigger_system_reset("ECC double-bit error");
    }
    
    // 清除中断标志
    DDR_CTRL->ECC_STATUS = ecc_status;
}
```

#### 7.2.2 内存Scrubbing（后台扫描）
```c
// 内存Scrubbing任务（A53侧）
void memory_scrubbing_task(void *arg) {
    uint64_t addr;
    volatile uint64_t dummy;
    
    while (1) {
        // 扫描整个DDR，触发ECC读取和纠错
        for (addr = DDR_START; addr < DDR_END; addr += 64) {
            dummy = *(volatile uint64_t*)addr;  // 读取触发ECC检查
            
            // 每扫描1MB休眠1ms，避免占用过多带宽
            if ((addr & 0xFFFFF) == 0) {
                OSAL_TaskDelay(1);
            }
        }
        
        LOG_INFO("SCRUB", "Memory scrubbing completed, %llu MB scanned",
                 (DDR_END - DDR_START) / (1024*1024));
        
        // 每10分钟扫描一次
        OSAL_TaskDelay(600000);
    }
}

// R5F侧TCM Scrubbing
void tcm_scrubbing_task(void *arg) {
    uint32_t addr;
    volatile uint32_t dummy;
    
    while (1) {
        // 扫描TCM（256KB）
        for (addr = TCM_START; addr < TCM_END; addr += 4) {
            dummy = *(volatile uint32_t*)addr;
        }
        
        // 每1秒扫描一次（TCM很小，扫描快）
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 7.3 关键数据三模冗余（TMR）

#### 7.3.1 TMR数据结构
```c
// 三模冗余数据结构
typedef struct {
    uint32_t value[3];      // 三份冗余数据
    uint32_t checksum[3];   // 每份的CRC32校验和
    uint32_t vote_count;    // 表决次数
    uint32_t error_count;   // 错误次数
} tmr_uint32_t;

// TMR写入
void tmr_write(tmr_uint32_t *tmr, uint32_t value) {
    uint32_t crc = crc32(&value, sizeof(value));
    
    // 写入三份
    for (int i = 0; i < 3; i++) {
        tmr->value[i] = value;
        tmr->checksum[i] = crc;
    }
}

// TMR读取（多数表决）
uint32_t tmr_read(tmr_uint32_t *tmr) {
    uint32_t valid[3] = {0};
    int valid_count = 0;
    
    // 1. 校验每份数据
    for (int i = 0; i < 3; i++) {
        uint32_t crc = crc32(&tmr->value[i], sizeof(tmr->value[i]));
        if (crc == tmr->checksum[i]) {
            valid[valid_count++] = tmr->value[i];
        }
    }
    
    // 2. 多数表决
    tmr->vote_count++;
    
    if (valid_count == 0) {
        // 三份都损坏，无法恢复
        LOG_FATAL("TMR", "All three copies corrupted!");
        tmr->error_count++;
        return 0xDEADBEEF;  // 返回错误标记
    }
    
    if (valid_count == 1) {
        // 只有一份有效，使用它并修复其他两份
        LOG_WARNING("TMR", "Only one valid copy, repairing...");
        tmr_write(tmr, valid[0]);
        tmr->error_count++;
        return valid[0];
    }
    
    // valid_count >= 2，多数表决
    if (valid[0] == valid[1]) {
        // 前两份一致
        if (valid_count == 3 && valid[2] != valid[0]) {
            // 第三份不一致，修复它
            LOG_WARNING("TMR", "Copy 2 mismatch, repairing...");
            tmr->value[2] = valid[0];
            tmr->checksum[2] = tmr->checksum[0];
            tmr->error_count++;
        }
        return valid[0];
    }
    
    if (valid_count == 3 && valid[0] == valid[2]) {
        // 第一和第三份一致，修复第二份
        LOG_WARNING("TMR", "Copy 1 mismatch, repairing...");
        tmr->value[1] = valid[0];
        tmr->checksum[1] = tmr->checksum[0];
        tmr->error_count++;
        return valid[0];
    }
    
    // 三份都不同，无法表决
    LOG_ERROR("TMR", "All three copies differ!");
    tmr->error_count++;
    return valid[0];  // 返回第一份（最不坏的选择）
}
```

#### 7.3.2 关键数据TMR保护
```c
// R5F侧关键数据（需要TMR保护）
typedef struct {
    tmr_uint32_t can_filter_id;      // CAN过滤器ID
    tmr_uint32_t watchdog_timeout;   // 看门狗超时时间
    tmr_uint32_t task_priority[6];   // 任务优先级
    tmr_uint32_t heartbeat_interval; // 心跳间隔
    tmr_uint32_t system_state;       // 系统状态
} r5f_critical_config_t;

// 使用示例
r5f_critical_config_t critical_config;

void init_critical_config(void) {
    tmr_write(&critical_config.can_filter_id, 0x123);
    tmr_write(&critical_config.watchdog_timeout, 5000);
    // ...
}

void can_rx_task(void *arg) {
    uint32_t filter_id = tmr_read(&critical_config.can_filter_id);
    // 使用filter_id...
}
```

### 7.4 配置寄存器Scrubbing

#### 7.4.1 关键寄存器定期刷新
```c
// R5F侧配置寄存器Scrubbing
typedef struct {
    volatile uint32_t *reg_addr;  // 寄存器地址
    uint32_t expected_value;      // 期望值
    const char *name;             // 寄存器名称
} register_scrub_entry_t;

// 需要Scrubbing的寄存器列表
register_scrub_entry_t scrub_registers[] = {
    // CAN控制器
    {&CAN0->CTRL, CAN_CTRL_ENABLE | CAN_CTRL_LOOPBACK_OFF, "CAN0_CTRL"},
    {&CAN0->BTR, CAN_BTR_500K, "CAN0_BTR"},
    
    // 中断控制器
    {&VIM->REQENASET0, 0x00000003, "VIM_REQENASET0"},
    
    // MPU配置
    {&MPU->REGION[0].BASE, TCM_BASE, "MPU_REGION0_BASE"},
    {&MPU->REGION[0].ATTR, MPU_ATTR_RWX, "MPU_REGION0_ATTR"},
    
    // 时钟配置
    {&PLL->CTRL, PLL_CTRL_800MHZ, "PLL_CTRL"},
};

// Scrubbing任务
void register_scrubbing_task(void *arg) {
    while (1) {
        for (int i = 0; i < ARRAY_SIZE(scrub_registers); i++) {
            register_scrub_entry_t *entry = &scrub_registers[i];
            uint32_t current_value = *entry->reg_addr;
            
            if (current_value != entry->expected_value) {
                LOG_ERROR("SCRUB", "Register %s corrupted: 0x%08x != 0x%08x",
                         entry->name, current_value, entry->expected_value);
                
                // 恢复寄存器值
                *entry->reg_addr = entry->expected_value;
                
                // 记录SEU事件
                seu_stats.register_errors++;
            }
        }
        
        // 每100ms刷新一次
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### 7.5 SEU检测和统计

#### 7.5.1 SEU统计数据结构
```c
typedef struct {
    uint64_t ecc_single_bit_errors;   // ECC单比特错误
    uint64_t ecc_double_bit_errors;   // ECC双比特错误
    uint64_t tmr_vote_errors;         // TMR表决错误
    uint64_t register_errors;         // 寄存器翻转
    uint64_t crc_errors;              // CRC校验错误
    uint64_t total_seu_events;        // 总SEU事件
    uint64_t last_seu_time;           // 最后一次SEU时间
} seu_statistics_t;

seu_statistics_t seu_stats;

// SEU事件上报
void report_seu_event(const char *type, uint64_t addr) {
    seu_stats.total_seu_events++;
    seu_stats.last_seu_time = OSAL_GetTimeUs();
    
    LOG_WARNING("SEU", "Event detected: type=%s, addr=0x%llx, total=%llu",
               type, addr, seu_stats.total_seu_events);
    
    // 如果SEU频率过高，触发告警
    if (seu_stats.total_seu_events > 100) {
        LOG_ERROR("SEU", "High SEU rate detected, consider safe mode");
    }
}
```

#### 7.5.2 SEU统计上报
```c
// 定期上报SEU统计（通过遥测）
void telemetry_seu_statistics(void) {
    telemetry_packet_t pkt;
    
    pkt.type = TM_TYPE_SEU_STATS;
    pkt.data.seu_stats.ecc_single_bit = seu_stats.ecc_single_bit_errors;
    pkt.data.seu_stats.ecc_double_bit = seu_stats.ecc_double_bit_errors;
    pkt.data.seu_stats.tmr_errors = seu_stats.tmr_vote_errors;
    pkt.data.seu_stats.register_errors = seu_stats.register_errors;
    pkt.data.seu_stats.total_events = seu_stats.total_seu_events;
    
    send_telemetry_to_satellite(&pkt);
}
```

### 7.6 抗辐射设计总结

#### 7.6.1 防护措施汇总
| 防护措施 | 覆盖范围 | 检测能力 | 纠正能力 | 开销 |
|---------|---------|---------|---------|------|
| DDR ECC | A53 DDR | 单/双比特 | 单比特自动纠正 | <5% |
| TCM ECC | R5F TCM | 单/双比特 | 单比特自动纠正 | <3% |
| 内存Scrubbing | 全部内存 | 潜在错误 | 触发ECC纠正 | <2% |
| TMR | 关键配置 | 三份对比 | 多数表决+修复 | 3x存储 |
| 寄存器Scrubbing | 关键寄存器 | 读回对比 | 立即刷新 | <1% |
| CRC校验 | 通信数据 | 数据损坏 | 重传 | <5% |

#### 7.6.2 预期SEU率和MTBF
```
LEO轨道（400km）预期SEU率：
├─ DDR（512MB）：~5次/天（ECC自动纠正）
├─ TCM（256KB）：~0.003次/天（ECC自动纠正）
├─ 关键配置（32KB TMR）：~0.0003次/天（TMR修复）
└─ 寄存器（1KB）：~0.00001次/天（Scrubbing修复）

预期MTBF（平均无故障时间）：
├─ 单比特错误（可纠正）：每天5次，自动恢复
├─ 双比特错误（不可纠正）：~1次/年，触发复位
└─ 系统级故障：>10年（假设其他硬件可靠性）
```

#### 7.6.3 建议的硬件改进
```
如果预算允许，建议：
├─ 使用抗辐射加固DDR（如Microchip/Microsemi）
├─ 使用抗辐射电源管理芯片
├─ 增加外部看门狗（抗辐射级）
├─ 使用屏蔽设计减少辐射暴露
└─ 考虑使用抗辐射加固SoC（如RAD750、LEON）
```

---
