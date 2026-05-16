## 1. 架构优化建议

本章节记录架构设计评审中发现的待优化点，按优先级分类。这些优化建议基于航天级嵌入式系统的实际需求和潜在风险分析。

### 1.1 优先级分类

| 优先级 | 说明 | 时间要求 |
|--------|------|---------|
| **P0** | 必须解决，影响系统可靠性和实时性 | 立即实施 |
| **P1** | 强烈建议，显著提升系统鲁棒性 | 近期实施 |
| **P2** | 可选优化，提升用户体验和可维护性 | 中长期实施 |

---

### 1.2 P0级优化（必须解决）

#### 1.2.1 共享内存SEU（单粒子翻转）防护

**问题描述**：

共享内存区域（1.5MB）是所有进程共享的，单个比特翻转可能导致全局故障。当前设计提到"ECC内存"，但软件层面缺少冗余校验机制。

**风险场景**：

```c
// 遥测缓存的新鲜度标记被SEU翻转
typedef struct {
  telemetry_data_t data;
  tm_freshness_t freshness;  // 如果被翻转：FRESH(0) → STALE(1) → INVALID(2)
  bool valid;                // 如果被翻转：true → false
} telemetry_cache_entry_t;

// 心跳表被SEU翻转
typedef struct {
  _Atomic uint64_t telecommand_heartbeat;  // 如果被翻转，Supervisor误判进程崩溃
  _Atomic uint64_t telemetry_heartbeat;
} heartbeat_table_t;
```

优化方案：

方案1：关键数据结构增加三模冗余（TMR）
``` c
/************************************************
* acl/include/acl_telemetry_cache.h
* 遥测缓存结构（增加SEU防护）
************************************************/

/**
* @brief 遥测缓存条目（带SEU防护）
*/
typedef struct {
  telemetry_data_t data;

  // 三模冗余：新鲜度标记
  tm_freshness_t freshness[3];

  // 三模冗余：有效标志
  bool valid[3];

  // CRC32校验
  uint32_t crc32;

  // 时间戳
  uint64_t timestamp_ns;

  // 序列号（递增，用于检测数据丢失）
  uint32_t sequence_number;
} telemetry_cache_entry_t;

/**
* @brief 读取新鲜度标记（带SEU检测）
*/
static inline tm_freshness_t get_freshness_safe(const telemetry_cache_entry_t *entry)
{
  tm_freshness_t f0 = entry->freshness[0];
  tm_freshness_t f1 = entry->freshness[1];
  tm_freshness_t f2 = entry->freshness[2];

  // 多数表决
  if (f0 == f1) return f0;
  if (f0 == f2) return f0;
  if (f1 == f2) return f1;

  // 三个值都不同，SEU严重
  LOG_ERROR("SEU", "遥测缓存新鲜度标记三模冗余失败");
  return TM_INVALID;  // 安全降级
}

/**
* @brief 读取有效标志（带SEU检测）
*/
static inline bool get_valid_safe(const telemetry_cache_entry_t *entry)
{
  bool v0 = entry->valid[0];
  bool v1 = entry->valid[1];
  bool v2 = entry->valid[2];

  // 多数表决
  if (v0 == v1) return v0;
  if (v0 == v2) return v0;
  if (v1 == v2) return v1;

  // 三个值都不同，SEU严重
  LOG_ERROR("SEU", "遥测缓存有效标志三模冗余失败");
  return false;  // 安全降级
}

/**
* @brief 写入遥测缓存（带SEU防护）
*/
static inline void set_telemetry_cache_safe(telemetry_cache_entry_t *entry,
										  const telemetry_data_t *data,
										  tm_freshness_t freshness,
										  bool valid)
{
  // 写入数据
  entry->data = *data;

  // 三模冗余写入
  entry->freshness[0] = freshness;
  entry->freshness[1] = freshness;
  entry->freshness[2] = freshness;

  entry->valid[0] = valid;
  entry->valid[1] = valid;
  entry->valid[2] = valid;

  // 计算CRC32
  entry->crc32 = crc32_calculate(&entry->data, sizeof(telemetry_data_t));

  // 更新时间戳和序列号
  entry->timestamp_ns = get_monotonic_ns();
  entry->sequence_number++;
}

/**
* @brief 验证遥测缓存完整性（CRC32校验）
*/
static inline bool verify_telemetry_cache(const telemetry_cache_entry_t *entry)
{
  uint32_t calc_crc = crc32_calculate(&entry->data, sizeof(telemetry_data_t));

  if (calc_crc != entry->crc32) {
	  LOG_ERROR("SEU", "遥测缓存CRC32校验失败");
	  return false;
  }

  return true;
}
```
方案2：心跳表三模冗余
``` C
/************************************************
* osal/include/ipc/osal_heartbeat_table.h
* 心跳表（增加SEU防护）
************************************************/

/**
* @brief 心跳条目（三模冗余）
*/
typedef struct {
  _Atomic uint64_t heartbeat[3];  // 三个副本
} heartbeat_entry_t;

/**
* @brief 心跳表
*/
typedef struct {
  heartbeat_entry_t telecommand;
  heartbeat_entry_t telemetry;
  heartbeat_entry_t firmware;
  heartbeat_entry_t logger;
} heartbeat_table_t;

/**
* @brief 更新心跳（三模冗余写入）
*/
static inline void heartbeat_update_safe(heartbeat_entry_t *entry)
{
  uint64_t now = get_monotonic_ns();

  atomic_store(&entry->heartbeat[0], now);
  atomic_store(&entry->heartbeat[1], now);
  atomic_store(&entry->heartbeat[2], now);
}

/**
* @brief 读取心跳（带SEU检测）
*/
static inline uint64_t heartbeat_read_safe(const heartbeat_entry_t *entry)
{
  uint64_t v0 = atomic_load(&entry->heartbeat[0]);
  uint64_t v1 = atomic_load(&entry->heartbeat[1]);
  uint64_t v2 = atomic_load(&entry->heartbeat[2]);

  // 多数表决
  if (v0 == v1) return v0;
  if (v0 == v2) return v0;
  if (v1 == v2) return v1;

  // 三个值都不同，SEU严重
  LOG_FATAL("SEU", "心跳表三模冗余失败，触发系统复位");
  trigger_watchdog_reset();

  return 0;  // 不会执行到这里
}
```
实施要点：

1. 所有共享内存中的关键数据结构都需要三模冗余
2. 读取时使用多数表决算法
3. 检测到SEU时记录到黑匣子，供地面分析
4. 三模冗余失败时触发安全降级或系统复位

预期效果：

- 单比特翻转：自动纠正，系统正常运行
- 双比特翻转：检测到错误，安全降级
- 三比特翻转：触发系统复位（极低概率）

---
#### 1.2.2 实时性监控和告警机制

问题描述：

当前设计声称"关键路径<1.23ms"，但缺少运行时实时性监控机制。在单核SoC上，内核锁竞争、Flash写入延迟等因素可能导致实际延迟超
标。

风险场景：

// Telemetry进程正在执行write()写日志到Flash，持有inode锁
// CAN中断到达，Telecommand进程被唤醒，但需要等待Telemetry释放内核锁
// 实际延迟可能达到5-10ms（Flash写入延迟）

优化方案：

方案1：关键路径延迟监控
``` C
/************************************************
* apps/telecommand/can_rx_thread.c
* CAN接收线程（增加实时性监控）
************************************************/

// 延迟统计
typedef struct {
  uint64_t total_count;
  uint64_t timeout_count;      // 超过2ms的次数
  uint64_t max_latency_us;     // 最大延迟
  uint64_t avg_latency_us;     // 平均延迟
  uint64_t last_violation_ns;  // 最后一次违规时间
} latency_statistics_t;

static latency_statistics_t g_latency_stats = {0};

// 延迟阈值
#define LATENCY_WARNING_US   1500   // 1.5ms告警
#define LATENCY_ERROR_US     2000   // 2ms错误
#define LATENCY_CRITICAL_US  5000   // 5ms严重

void* can_rx_thread_func(void* arg)
{
  while (1) {
	  can_frame_t frame;
	  HAL_CAN_Receive(can_handle, &frame, TIMEOUT_INFINITE);

	  uint64_t start_ns = get_monotonic_ns();

	  // 处理命令
	  handle_command(&frame);

	  uint64_t end_ns = get_monotonic_ns();
	  uint64_t latency_us = (end_ns - start_ns) / 1000;

	  // 更新统计
	  g_latency_stats.total_count++;
	  g_latency_stats.avg_latency_us =
		  (g_latency_stats.avg_latency_us * (g_latency_stats.total_count - 1) + latency_us)
		  / g_latency_stats.total_count;

	  if (latency_us > g_latency_stats.max_latency_us) {
		  g_latency_stats.max_latency_us = latency_us;
	  }

	  // 实时性监控
	  if (latency_us > LATENCY_CRITICAL_US) {
		  LOG_FATAL("RT", "实时性严重违规: %lu us (阈值: %d us)",
					latency_us, LATENCY_CRITICAL_US);

		  // 记录到黑匣子
		  blackbox_record(EVENT_LATENCY_CRITICAL, latency_us);

		  // 触发告警（向卫星平台上报）
		  send_alert_to_satellite(ALERT_LATENCY_CRITICAL, latency_us);

	  } else if (latency_us > LATENCY_ERROR_US) {
		  LOG_ERROR("RT", "实时性违规: %lu us (阈值: %d us)",
					latency_us, LATENCY_ERROR_US);

		  g_latency_stats.timeout_count++;
		  g_latency_stats.last_violation_ns = end_ns;

		  // 记录到黑匣子
		  blackbox_record(EVENT_LATENCY_TIMEOUT, latency_us);

	  } else if (latency_us > LATENCY_WARNING_US) {
		  LOG_WARN("RT", "实时性告警: %lu us (阈值: %d us)",
				   latency_us, LATENCY_WARNING_US);
	  }
  }
}

/**
* @brief 获取延迟统计（供遥测上报）
*/
void get_latency_statistics(latency_statistics_t *stats)
{
  *stats = g_latency_stats;
}
```
方案2：实时性违规分析
``` C
/************************************************
* apps/telecommand/latency_analyzer.c
* 实时性违规分析
************************************************/

// 违规记录
typedef struct {
  uint64_t timestamp_ns;
  uint64_t latency_us;
  uint8_t cmd_type;
  uint32_t pid;              // 当前持有CPU的进程
  char process_name[16];
} latency_violation_record_t;

#define MAX_VIOLATION_RECORDS 100
static latency_violation_record_t g_violation_records[MAX_VIOLATION_RECORDS];
static uint32_t g_violation_index = 0;

/**
* @brief 记录实时性违规
*/
void record_latency_violation(uint64_t latency_us, uint8_t cmd_type)
{
  latency_violation_record_t *rec = &g_violation_records[g_violation_index];

  rec->timestamp_ns = get_monotonic_ns();
  rec->latency_us = latency_us;
  rec->cmd_type = cmd_type;

  // 获取当前持有CPU的进程（通过/proc/stat分析）
  get_current_running_process(&rec->pid, rec->process_name);

  g_violation_index = (g_violation_index + 1) % MAX_VIOLATION_RECORDS;

  // 分析违规原因
  analyze_violation_cause(rec);
}

/**
* @brief 分析违规原因
*/
void analyze_violation_cause(const latency_violation_record_t *rec)
{
  // 1. 检查是否是Flash写入导致
  if (strstr(rec->process_name, "logger") != NULL) {
	  LOG_ERROR("RT", "实时性违规原因：Logger进程写Flash");
  }

  // 2. 检查是否是Telemetry进程采集导致
  if (strstr(rec->process_name, "telemetry") != NULL) {
	  LOG_ERROR("RT", "实时性违规原因：Telemetry进程采集");
  }

  // 3. 检查是否是内核调度延迟
  if (rec->latency_us > 5000) {
	  LOG_ERROR("RT", "实时性违规原因：内核调度延迟");
  }
}
```
实施要点：

1. 每次命令处理都记录延迟
2. 超过阈值时记录到黑匣子和违规日志
3. 定期分析违规原因，优化系统配置
4. 通过遥测上报延迟统计，供地面监控

预期效果：

- 实时发现实时性问题
- 定位违规原因（Flash写入、内核锁竞争等）
- 为系统优化提供数据支持

---
#### 1.2.3 Flash写入与实时性隔离

问题描述：

Logger进程写日志到Flash可能导致内核锁竞争，影响Telecommand进程的实时性。Flash写入延迟：NOR Flash 1-10ms，NAND Flash
0.5-2ms，eMMC 5-20ms（垃圾回收时）。

优化方案：

方案1：Logger进程使用O_DIRECT绕过页缓存
``` C
/************************************************
* apps/logger/log_collector.c
* 日志收集线程（优化Flash写入）
************************************************/

/**
* @brief 初始化日志文件（O_DIRECT + 预分配）
*/
int32_t init_log_file(const char *log_path)
{
  // 1. 使用O_DIRECT绕过页缓存，避免内核锁竞争
  int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND | O_DIRECT | O_SYNC, 0644);
  if (fd < 0) {
	  LOG_ERROR("LOGGER", "打开日志文件失败: %s", strerror(errno));
	  return OSAL_ERR_FILE_OPEN;
  }

  // 2. 预分配空间，避免动态扩展
  int ret = fallocate(fd, 0, 0, 10 * 1024 * 1024);  // 预分配10MB
  if (ret < 0) {
	  LOG_WARN("LOGGER", "预分配日志空间失败: %s", strerror(errno));
  }

  // 3. 设置低优先级，避免抢占Telecommand进程
  struct sched_param param;
  param.sched_priority = 0;
  sched_setscheduler(0, SCHED_BATCH, &param);
  nice(19);  // 最低优先级

  return fd;
}

/**
* @brief 写入日志（对齐到扇区边界）
*/
int32_t write_log_direct(int fd, const char *log_data, size_t len)
{
  // O_DIRECT要求缓冲区和大小都对齐到扇区边界（通常512字节）
  #define SECTOR_SIZE 512

  // 对齐缓冲区
  char *aligned_buf = NULL;
  posix_memalign((void**)&aligned_buf, SECTOR_SIZE, ALIGN_UP(len, SECTOR_SIZE));

  memcpy(aligned_buf, log_data, len);

  // 写入
  ssize_t written = write(fd, aligned_buf, ALIGN_UP(len, SECTOR_SIZE));

  free(aligned_buf);

  return (written > 0) ? OSAL_SUCCESS : OSAL_ERR_FILE_WRITE;
}
```
方案2：使用专用日志分区
``` shell
# /etc/fstab
# 专用日志分区，独立的MTD设备，避免与根文件系统竞争
/dev/mtd3  /var/log/pmc  jffs2  defaults,noatime  0  0

# 或者使用RAM disk（掉电丢失，但实时性最好）
tmpfs  /var/log/pmc  tmpfs  size=50M,noatime  0  0
```
方案3：关键日志使用NVRAM
``` C
/************************************************
* apps/logger/nvram_logger.c
* NVRAM日志（关键事件）
************************************************/

// NVRAM日志（掉电不丢失，写入延迟<100μs）
#define NVRAM_LOG_BASE  0x08000000
#define NVRAM_LOG_SIZE  (64 * 1024)  // 64KB

typedef struct {
  uint64_t timestamp_ns;
  event_type_t type;
  uint32_t data[8];
  uint32_t crc32;
} nvram_log_entry_t;

/**
* @brief 写入NVRAM日志（关键事件）
*/
void write_nvram_log(event_type_t type, const uint32_t *data, size_t data_len)
{
  nvram_log_entry_t entry;

  entry.timestamp_ns = get_monotonic_ns();
  entry.type = type;
  memcpy(entry.data, data, data_len * sizeof(uint32_t));
  entry.crc32 = crc32_calculate(&entry, sizeof(entry) - sizeof(uint32_t));

  // 直接写入NVRAM（内存映射，无系统调用）
  volatile nvram_log_entry_t *nvram = (nvram_log_entry_t*)NVRAM_LOG_BASE;
  static uint32_t nvram_index = 0;

  nvram[nvram_index] = entry;
  nvram_index = (nvram_index + 1) % (NVRAM_LOG_SIZE / sizeof(nvram_log_entry_t));
}
```
实施要点：

1. Logger进程使用O_DIRECT + 预分配，避免页缓存和动态扩展
2. 使用专用日志分区或RAM disk，避免与根文件系统竞争
3. 关键事件（电源状态变化、遥控命令执行）写入NVRAM
4. Logger进程设置为SCHED_BATCH + nice 19，最低优先级

预期效果：

- Flash写入不影响Telecommand进程的实时性
- 关键事件掉电不丢失
- 日志写入延迟从5-20ms降低到<1ms

---
### 1.3 P1级优化（强烈建议）

#### 1.3.1 遥控后遥测强制更新机制

问题描述：

实时型遥测降级到缓存时，缓存数据可能已经过期500ms（Telemetry进程采集周期）。例如，服务器刚刚上电，但缓存中的数据是500ms前
采集的"关机状态"。

优化方案：
``` C
/************************************************
* apps/telecommand/tc_exec_thread.c
* 遥控执行线程（增加强制更新触发）
************************************************/

// 强制更新队列（共享内存）
typedef struct {
  pmc_tm_function_t tm_functions[32];
  _Atomic uint32_t write_idx;
  _Atomic uint32_t read_idx;
} force_update_queue_t;

static force_update_queue_t *g_force_update_queue = NULL;  // 共享内存

/**
* @brief 遥控命令执行后，触发相关遥测强制更新
*/
void handle_telecommand_with_tm_update(pmc_tc_function_t cmd_type, uint32_t param)
{
  // 1. 执行遥控命令
  int32_t ret = execute_telecommand(cmd_type, param);

  if (ret == OSAL_SUCCESS) {
	  // 2. 标记相关遥测为STALE
	  ACL_InvalidateAffectedTelemetry(cmd_type);

	  // 3. 触发Telemetry进程强制更新
	  const tc_tm_invalidation_map_t *map = ACL_GetInvalidationMap(cmd_type);
	  if (map != NULL) {
		  for (uint32_t i = 0; i < map->affected_count; i++) {
			  pmc_tm_function_t tm_func = map->affected_tm[i];

			  // 放入强制更新队列
			  uint32_t write_idx = atomic_fetch_add(&g_force_update_queue->write_idx, 1) % 32;
			  g_force_update_queue->tm_functions[write_idx] = tm_func;

			  LOG_DEBUG("TC", "触发遥测强制更新: %d", tm_func);
		  }
	  }
  }
}

/************************************************
* apps/telemetry/cache_collector.c
* 缓存采集线程（增加强制更新处理）
************************************************/

/**
* @brief 缓存采集线程（优先处理强制更新）
*/
void* cache_collector_thread(void* arg)
{
  while (1) {
	  // 1. 优先处理强制更新队列
	  while (has_force_update()) {
		  pmc_tm_function_t tm_func = pop_force_update();

		  LOG_INFO("TM", "处理强制更新: %d", tm_func);

		  // 立即采集
		  const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_func);
		  telemetry_data_t data;
		  int32_t ret = collect_telemetry(cfg, &data);

		  if (ret == OSAL_SUCCESS) {
			  // 更新缓存，标记为FRESH
			  telemetry_cache_entry_t *cache = &g_tm_cache[tm_func];
			  set_telemetry_cache_safe(cache, &data, TM_FRESH, true);

			  LOG_INFO("TM", "强制更新完成，缓存已恢复为FRESH: %d", tm_func);
		  }
	  }

	  // 2. 正常周期采集
	  collect_all_telemetry();

	  OSAL_TaskDelay(100);  // 100ms周期检查
  }
}

/**
* @brief 检查是否有强制更新请求
*/
static bool has_force_update(void)
{
  uint32_t read_idx = atomic_load(&g_force_update_queue->read_idx);
  uint32_t write_idx = atomic_load(&g_force_update_queue->write_idx);

  return (read_idx != write_idx);
}

/**
* @brief 从强制更新队列取出一个遥测项
*/
static pmc_tm_function_t pop_force_update(void)
{
  uint32_t read_idx = atomic_fetch_add(&g_force_update_queue->read_idx, 1) % 32;
  return g_force_update_queue->tm_functions[read_idx];
}
```
实施要点：

1. 遥控命令执行成功后，自动触发相关遥测的强制更新
2. Telemetry进程优先处理强制更新队列
3. 强制更新完成后，缓存标记为FRESH

预期效果：

- 遥控后遥测数据时效性从500ms提升到<100ms
- 降低STALE数据的持续时间

---
#### 1.3.2 动态超时调整

问题描述：

实时型遥测配置了固定超时（BMC 1ms，MCU
800μs），但实际通信延迟受网络拥塞、BMC负载等因素影响。固定超时可能过于激进，导致频繁降级。

优化方案：
``` C
/************************************************
* pdl/src/pdl_bmc/pdl_bmc.c
* BMC驱动（增加动态超时）
************************************************/

// 动态超时统计
typedef struct {
  uint32_t sample_count;
  uint32_t avg_latency_us;      // 平均延迟
  uint32_t stddev_latency_us;   // 标准差
  uint32_t max_latency_us;      // 最大延迟
  uint32_t timeout_us;          // 动态超时 = avg + 3*stddev
} adaptive_timeout_t;

static adaptive_timeout_t g_bmc_timeout[MAX_BMC_COUNT] = {0};

/**
* @brief 更新动态超时统计
*/
void update_adaptive_timeout(uint32_t bmc_index, uint32_t latency_us)
{
  adaptive_timeout_t *timeout = &g_bmc_timeout[bmc_index];

  // 更新平均延迟（指数移动平均）
  if (timeout->sample_count == 0) {
	  timeout->avg_latency_us = latency_us;
  } else {
	  timeout->avg_latency_us = (timeout->avg_latency_us * 7 + latency_us) / 8;
  }

  // 更新标准差（简化计算）
  uint32_t diff = (latency_us > timeout->avg_latency_us)
				  ? (latency_us - timeout->avg_latency_us)
				  : (timeout->avg_latency_us - latency_us);
  timeout->stddev_latency_us = (timeout->stddev_latency_us * 7 + diff) / 8;

  // 更新最大延迟
  if (latency_us > timeout->max_latency_us) {
	  timeout->max_latency_us = latency_us;
  }

  // 计算动态超时：avg + 3*stddev（覆盖99.7%的情况）
  timeout->timeout_us = timeout->avg_latency_us + 3 * timeout->stddev_latency_us;

  // 限制范围：500us ~ 5000us
  if (timeout->timeout_us < 500) {
	  timeout->timeout_us = 500;
  } else if (timeout->timeout_us > 5000) {
	  timeout->timeout_us = 5000;
  }

  timeout->sample_count++;

  LOG_DEBUG("BMC", "动态超时更新: avg=%u, stddev=%u, timeout=%u",
			timeout->avg_latency_us, timeout->stddev_latency_us, timeout->timeout_us);
}

/**
* @brief 获取动态超时值
*/
uint32_t get_adaptive_timeout(uint32_t bmc_index)
{
  adaptive_timeout_t *timeout = &g_bmc_timeout[bmc_index];

  // 如果样本数不足，使用默认超时
  if (timeout->sample_count < 10) {
	  return 1000;  // 默认1ms
  }

  return timeout->timeout_us;
}

/**
* @brief BMC查询（使用动态超时）
*/
int32_t PDL_BMC_QueryWithAdaptiveTimeout(uint32_t bmc_index, bmc_data_t *data)
{
  uint64_t start_ns = get_monotonic_ns();

  // 使用动态超时
  uint32_t timeout_us = get_adaptive_timeout(bmc_index);

  int32_t ret = PDL_BMC_Query(bmc_index, data, timeout_us);

  uint64_t latency_us = (get_monotonic_ns() - start_ns) / 1000;

  // 更新动态超时统计
  if (ret == OSAL_SUCCESS) {
	  update_adaptive_timeout(bmc_index, latency_us);
  }

  return ret;
}
```
实施要点：

1. 记录每次查询的实际延迟
2. 使用指数移动平均计算平均延迟和标准差
3. 动态超时 = 平均延迟 + 3×标准差（覆盖99.7%的情况）
4. 限制超时范围（500μs ~ 5000μs），避免极端值

预期效果：

- 减少不必要的超时降级
- 适应不同BMC的性能差异
- 适应网络拥塞等动态变化

---
#### 1.3.3 安全模式和黑匣子机制

问题描述：

当系统出现严重故障（连续崩溃、Flash损坏、通信中断）时，缺少降级运行机制。系统可能陷入"崩溃-重启-崩溃"的死循环。

优化方案：
``` C
方案1：系统运行模式

/************************************************
* apps/supervisor/supervisor.c
* Supervisor进程（增加系统模式管理）
************************************************/

/**
* @brief 系统运行模式
*/
typedef enum {
  MODE_NORMAL,        // 正常模式：所有功能可用
  MODE_DEGRADED,      // 降级模式：仅关键功能可用
  MODE_SAFE           // 安全模式：仅维持心跳和基本遥测
} system_mode_t;

static system_mode_t g_system_mode = MODE_NORMAL;

/**
* @brief 系统模式切换条件
*/
typedef struct {
  uint32_t process_crash_count;      // 进程崩溃次数
  uint32_t bmc_comm_failure_count;   // BMC通信失败次数
  uint32_t flash_write_failure_count;// Flash写入失败次数
  uint32_t latency_violation_count;  // 实时性违规次数
} system_health_t;

static system_health_t g_system_health = {0};

/**
* @brief 检查系统健康状态，决定是否切换模式
*/
void check_system_health(void)
{
  system_mode_t new_mode = MODE_NORMAL;

  // 1. 连续5次进程崩溃 → 降级模式
  if (g_system_health.process_crash_count >= 5) {
	  new_mode = MODE_DEGRADED;
	  LOG_WARN("SYS", "进程崩溃次数过多，切换到降级模式");
  }

  // 2. 连续10次进程崩溃 → 安全模式
  if (g_system_health.process_crash_count >= 10) {
	  new_mode = MODE_SAFE;
	  LOG_ERROR("SYS", "进程崩溃次数严重，切换到安全模式");
  }

  // 3. BMC通信连续失败10次 → 降级模式
  if (g_system_health.bmc_comm_failure_count >= 10) {
	  new_mode = MODE_DEGRADED;
	  LOG_WARN("SYS", "BMC通信失败次数过多，切换到降级模式");
  }

  // 4. Flash写入连续失败5次 → 安全模式（只读）
  if (g_system_health.flash_write_failure_count >= 5) {
	  new_mode = MODE_SAFE;
	  LOG_ERROR("SYS", "Flash写入失败，切换到安全模式（只读）");
  }

  // 5. 实时性连续违规20次 → 降级模式
  if (g_system_health.latency_violation_count >= 20) {
	  new_mode = MODE_DEGRADED;
	  LOG_WARN("SYS", "实时性违规次数过多，切换到降级模式");
  }

  // 切换模式
  if (new_mode != g_system_mode) {
	  switch_system_mode(new_mode);
  }
}

/**
* @brief 切换系统模式
*/
void switch_system_mode(system_mode_t new_mode)
{
  LOG_INFO("SYS", "系统模式切换: %d -> %d", g_system_mode, new_mode);

  g_system_mode = new_mode;

  switch (new_mode) {
	  case MODE_NORMAL:
		  // 正常模式：所有功能可用
		  enable_all_features();
		  break;

	  case MODE_DEGRADED:
		  // 降级模式：禁用非关键功能
		  disable_firmware_upgrade();      // 禁用固件升级
		  disable_non_critical_telemetry(); // 禁用非关键遥测
		  reduce_log_level(LOG_LEVEL_WARN); // 降低日志级别
		  LOG_WARN("SYS", "降级模式：仅关键功能可用");
		  break;

	  case MODE_SAFE:
		  // 安全模式：仅维持心跳和基本遥控
		  disable_firmware_upgrade();
		  disable_all_telemetry_except_heartbeat();
		  disable_log_write();  // 禁用日志写入（仅内存缓冲）
		  enable_readonly_mode();  // 只读模式
		  LOG_ERROR("SYS", "安全模式：仅维持心跳和基本遥控");
		  break;
  }

  // 向卫星平台上报模式切换
  send_mode_change_to_satellite(new_mode);
}

/**
* @brief 获取当前系统模式
*/
system_mode_t get_system_mode(void)
{
  return g_system_mode;
}
```
方案2：黑匣子机制
``` C
/************************************************
* apps/supervisor/blackbox.c
* 黑匣子（关键事件记录）
************************************************/

// 黑匣子存储位置：NVRAM或专用Flash分区
#define BLACKBOX_BASE_ADDR  0x08000000
#define BLACKBOX_SIZE       (64 * 1024)  // 64KB
#define BLACKBOX_ENTRY_SIZE sizeof(blackbox_entry_t)
#define BLACKBOX_MAX_ENTRIES (BLACKBOX_SIZE / BLACKBOX_ENTRY_SIZE)

/**
* @brief 黑匣子事件类型
*/
typedef enum {
  EVENT_SYSTEM_BOOT = 0,
  EVENT_SYSTEM_SHUTDOWN,
  EVENT_WATCHDOG_RESET,
  EVENT_PROCESS_CRASH,
  EVENT_LATENCY_VIOLATION,
  EVENT_SEU_DETECTED,
  EVENT_BMC_COMM_FAILURE,
  EVENT_FLASH_WRITE_FAILURE,
  EVENT_MODE_CHANGE,
  EVENT_TELECOMMAND,
  EVENT_POWER_STATE_CHANGE,
  EVENT_MAX
} blackbox_event_type_t;

/**
* @brief 黑匣子条目
*/
typedef struct {
  uint64_t timestamp_ns;
  blackbox_event_type_t type;
  uint32_t data[8];   // 事件相关数据
  uint32_t crc32;
} blackbox_entry_t;

/**
* @brief 黑匣子控制结构
*/
typedef struct {
  uint32_t write_index;
  uint32_t total_count;
  blackbox_entry_t entries[BLACKBOX_MAX_ENTRIES];
} blackbox_t;

static blackbox_t *g_blackbox = NULL;  // 映射到NVRAM

/**
* @brief 初始化黑匣子
*/
int32_t blackbox_init(void)
{
  // 映射NVRAM到内存
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
	  LOG_ERROR("BLACKBOX", "打开/dev/mem失败");
	  return OSAL_ERR_FILE_OPEN;
  }

  g_blackbox = (blackbox_t*)mmap(NULL, BLACKBOX_SIZE,
								 PROT_READ | PROT_WRITE,
								 MAP_SHARED, fd, BLACKBOX_BASE_ADDR);

  if (g_blackbox == MAP_FAILED) {
	  LOG_ERROR("BLACKBOX", "映射NVRAM失败");
	  close(fd);
	  return OSAL_ERR_MMAP;
  }

  close(fd);

  LOG_INFO("BLACKBOX", "黑匣子初始化完成，已记录%u条事件", g_blackbox->total_count);

  return OSAL_SUCCESS;
}

/**
* @brief 记录黑匣子事件
*/
void blackbox_record(blackbox_event_type_t type, const uint32_t *data, size_t data_len)
{
  if (g_blackbox == NULL) {
	  return;
  }

  uint32_t index = g_blackbox->write_index % BLACKBOX_MAX_ENTRIES;
  blackbox_entry_t *entry = &g_blackbox->entries[index];

  entry->timestamp_ns = get_monotonic_ns();
  entry->type = type;

  if (data != NULL && data_len > 0) {
	  size_t copy_len = (data_len > 8) ? 8 : data_len;
	  memcpy(entry->data, data, copy_len * sizeof(uint32_t));
  } else {
	  memset(entry->data, 0, sizeof(entry->data));
  }

  entry->crc32 = crc32_calculate(entry, sizeof(blackbox_entry_t) - sizeof(uint32_t));

  g_blackbox->write_index++;
  g_blackbox->total_count++;
}

/**
* @brief 系统启动时分析黑匣子
*/
void blackbox_analyze_on_boot(void)
{
  if (g_blackbox == NULL) {
	  return;
  }

  LOG_INFO("BLACKBOX", "分析黑匣子，共%u条事件", g_blackbox->total_count);

  // 读取最后10条事件
  uint32_t start_index = (g_blackbox->write_index > 10)
						 ? (g_blackbox->write_index - 10) : 0;

  for (uint32_t i = start_index; i < g_blackbox->write_index; i++) {
	  uint32_t index = i % BLACKBOX_MAX_ENTRIES;
	  blackbox_entry_t *entry = &g_blackbox->entries[index];

	  // 验证CRC32
	  uint32_t calc_crc = crc32_calculate(entry, sizeof(blackbox_entry_t) - sizeof(uint32_t));
	  if (calc_crc != entry->crc32) {
		  LOG_WARN("BLACKBOX", "条目%u CRC32校验失败", i);
		  continue;
	  }

	  // 分析事件
	  switch (entry->type) {
		  case EVENT_WATCHDOG_RESET:
			  LOG_ERROR("BLACKBOX", "上次复位原因：看门狗超时");
			  // 向卫星平台上报
			  send_alert_to_satellite(ALERT_WATCHDOG_RESET, 0);
			  break;

		  case EVENT_PROCESS_CRASH:
			  LOG_ERROR("BLACKBOX", "上次进程崩溃：PID=%u, 信号=%u",
						entry->data[0], entry->data[1]);
			  break;

		  case EVENT_SEU_DETECTED:
			  LOG_ERROR("BLACKBOX", "检测到SEU：地址=0x%08x",
						entry->data[0]);
			  break;

		  case EVENT_FLASH_WRITE_FAILURE:
			  LOG_ERROR("BLACKBOX", "Flash写入失败：地址=0x%08x",
						entry->data[0]);
			  break;

		  default:
			  break;
	  }
  }
}

/**
* @brief 导出黑匣子数据（供地面分析）
*/
int32_t blackbox_export(const char *export_path)
{
  if (g_blackbox == NULL) {
	  return OSAL_ERR_NOT_INITIALIZED;
  }

  FILE *fp = fopen(export_path, "w");
  if (fp == NULL) {
	  LOG_ERROR("BLACKBOX", "打开导出文件失败: %s", export_path);
	  return OSAL_ERR_FILE_OPEN;
  }

  fprintf(fp, "# PMC黑匣子数据导出\n");
  fprintf(fp, "# 总事件数: %u\n", g_blackbox->total_count);
  fprintf(fp, "# 导出时间: %lu\n\n", time(NULL));

  fprintf(fp, "时间戳(ns),事件类型,数据0,数据1,数据2,数据3,数据4,数据5,数据6,数据7\n");

  for (uint32_t i = 0; i < g_blackbox->write_index && i < BLACKBOX_MAX_ENTRIES; i++) {
	  blackbox_entry_t *entry = &g_blackbox->entries[i];

	  fprintf(fp, "%lu,%d,%u,%u,%u,%u,%u,%u,%u,%u\n",
			  entry->timestamp_ns, entry->type,
			  entry->data[0], entry->data[1], entry->data[2], entry->data[3],
			  entry->data[4], entry->data[5], entry->data[6], entry->data[7]);
  }

  fclose(fp);

  LOG_INFO("BLACKBOX", "黑匣子数据已导出到: %s", export_path);

  return OSAL_SUCCESS;
}
```
实施要点：

1. 系统根据健康状态自动切换运行模式
2. 黑匣子记录所有关键事件（崩溃、复位、SEU等）
3. 系统启动时分析黑匣子，识别上次故障原因
4. 支持导出黑匣子数据，供地面分析

预期效果：

- 避免系统陷入"崩溃-重启"死循环
- 保留故障现场，便于地面分析
- 提升系统在极端环境下的生存能力

---
### 1.4 P2级优化（可选优化）

#### 1.4.1 PDL层通道健康度评估

问题描述：

当前设计支持双通道冗余 + 自动切换，但缺少通道健康度评估机制。可能频繁在两个通道之间切换，影响稳定性。

优化方案：
``` C
/************************************************
* pdl/src/pdl_bmc/pdl_bmc.c
* BMC驱动（增加通道健康度评估）
************************************************/

/**
* @brief 通道健康度统计
*/
typedef struct {
  uint32_t success_count;
  uint32_t failure_count;
  uint32_t timeout_count;
  uint32_t avg_latency_us;
  float health_score;  // 0.0-1.0
  uint64_t last_success_ns;
  uint64_t last_failure_ns;
} channel_health_t;

/**
* @brief BMC设备（增加通道健康度）
*/
typedef struct {
  uint32_t logic_index;
  channel_health_t channels[2];  // 主通道、备份通道
  uint32_t active_channel;       // 当前活动通道
} pdl_bmc_device_t;

/**
* @brief 更新通道健康度
*/
void update_channel_health(pdl_bmc_device_t *dev, uint32_t channel_id,
						 bool success, uint32_t latency_us)
{
  channel_health_t *ch = &dev->channels[channel_id];

  if (success) {
	  ch->success_count++;
	  ch->last_success_ns = get_monotonic_ns();

	  // 更新平均延迟
	  ch->avg_latency_us = (ch->avg_latency_us * 7 + latency_us) / 8;
  } else {
	  ch->failure_count++;
	  ch->last_failure_ns = get_monotonic_ns();
  }

  // 计算健康度评分
  // 健康度 = 成功率 * 0.6 + (1 - 归一化延迟) * 0.3 + 时效性 * 0.1
  float success_rate = (float)ch->success_count / (ch->success_count + ch->failure_count);
  float latency_score = 1.0 - (float)ch->avg_latency_us / 10000.0;  // 10ms为基准

  uint64_t now = get_monotonic_ns();
  float freshness_score = (now - ch->last_success_ns < 5000000000ULL) ? 1.0 : 0.5;  // 5秒内成功过

  ch->health_score = success_rate * 0.6 + latency_score * 0.3 + freshness_score * 0.1;

  // 限制范围
  if (ch->health_score < 0.0) ch->health_score = 0.0;
  if (ch->health_score > 1.0) ch->health_score = 1.0;
}

/**
* @brief 选择最优通道（基于健康度）
*/
uint32_t select_best_channel(pdl_bmc_device_t *dev)
{
  channel_health_t *ch0 = &dev->channels[0];
  channel_health_t *ch1 = &dev->channels[1];

  // 如果当前通道健康度>0.7，继续使用（避免频繁切换）
  if (dev->channels[dev->active_channel].health_score > 0.7) {
	  return dev->active_channel;
  }

  // 选择健康度更高的通道
  if (ch0->health_score > ch1->health_score + 0.1) {  // 0.1滞后，避免抖动
	  return 0;
  } else if (ch1->health_score > ch0->health_score + 0.1) {
	  return 1;
  } else {
	  return dev->active_channel;  // 健康度相近，保持当前通道
  }
}

/**
* @brief BMC查询（智能通道选择）
*/
int32_t PDL_BMC_QueryWithSmartChannel(uint32_t bmc_index, bmc_data_t *data)
{
  pdl_bmc_device_t *dev = &g_bmc_devices[bmc_index];

  // 选择最优通道
  uint32_t channel_id = select_best_channel(dev);

  if (channel_id != dev->active_channel) {
	  LOG_INFO("BMC", "切换通道: %u -> %u (健康度: %.2f -> %.2f)",
			   dev->active_channel, channel_id,
			   dev->channels[dev->active_channel].health_score,
			   dev->channels[channel_id].health_score);
	  dev->active_channel = channel_id;
  }

  // 查询
  uint64_t start_ns = get_monotonic_ns();
  int32_t ret = query_bmc_channel(dev, channel_id, data);
  uint32_t latency_us = (get_monotonic_ns() - start_ns) / 1000;

  // 更新健康度
  update_channel_health(dev, channel_id, (ret == OSAL_SUCCESS), latency_us);

  return ret;
}
```
实施要点：

1. 记录每个通道的成功率、延迟、时效性
2. 计算健康度评分（0.0-1.0）
3. 选择健康度更高的通道，但避免频繁切换（滞后0.1）
4. 当前通道健康度>0.7时，继续使用

预期效果：

- 减少不必要的通道切换
- 自动选择最优通道
- 提升通信稳定性

---
#### 1.4.2 Supervisor降级策略优化

问题描述：

当前Supervisor的"5次/300秒"重启限制过于严格。在太空辐射环境下，瞬时SEU可能导致进程频繁崩溃，5次限制可能过早触发系统复位。

优化方案：
``` C
/************************************************
* apps/supervisor/supervisor.c
* Supervisor进程（优化重启策略）
************************************************/

/**
* @brief 崩溃原因
*/
typedef enum {
  CRASH_SEGFAULT,      // 段错误（可能是SEU）
  CRASH_ABORT,         // 断言失败（逻辑错误）
  CRASH_TIMEOUT,       // 心跳超时（死锁）
  CRASH_UNKNOWN
} crash_reason_t;

/**
* @brief 崩溃统计
*/
typedef struct {
  uint32_t segfault_count;   // SEU导致的崩溃
  uint32_t abort_count;      // 逻辑错误导致的崩溃
  uint32_t timeout_count;    // 死锁导致的崩溃
  uint32_t unknown_count;    // 未知原因崩溃
  time_t   last_crash_time;
} crash_statistics_t;

static crash_statistics_t g_crash_stats[PROC_MAX] = {0};

/**
* @brief 识别崩溃原因
*/
crash_reason_t identify_crash_reason(pid_t pid, int status)
{
  if (WIFSIGNALED(status)) {
	  int sig = WTERMSIG(status);

	  switch (sig) {
		  case SIGSEGV:  // 段错误
		  case SIGBUS:   // 总线错误
			  return CRASH_SEGFAULT;

		  case SIGABRT:  // 断言失败
			  return CRASH_ABORT;

		  default:
			  return CRASH_UNKNOWN;
	  }
  }

  return CRASH_UNKNOWN;
}

/**
* @brief 决定是否重启进程（根据崩溃原因）
*/
bool should_restart_process(process_id_t proc_id, crash_reason_t reason)
{
  crash_statistics_t *stats = &g_crash_stats[proc_id];
  time_t now = time(NULL);

  // 重置计数（300秒窗口）
  if (now - stats->last_crash_time > 300) {
	  memset(stats, 0, sizeof(crash_statistics_t));
  }

  stats->last_crash_time = now;

  // 根据崩溃原因决定重启策略
  switch (reason) {
	  case CRASH_SEGFAULT:
		  // SEU导致的崩溃，允许更多次重启（10次）
		  stats->segfault_count++;
		  if (stats->segfault_count < 10) {
			  LOG_WARN("SUP", "进程%d段错误（可能SEU），重启（%u/10）",
					   proc_id, stats->segfault_count);
			  return true;
		  } else {
			  LOG_ERROR("SUP", "进程%d段错误次数过多，触发系统复位", proc_id);
			  return false;
		  }

	  case CRASH_ABORT:
		  // 逻辑错误，严格限制（3次）
		  stats->abort_count++;
		  if (stats->abort_count < 3) {
			  LOG_ERROR("SUP", "进程%d断言失败，重启（%u/3）",
						proc_id, stats->abort_count);
			  return true;
		  } else {
			  LOG_FATAL("SUP", "进程%d断言失败次数过多，触发系统复位", proc_id);
			  return false;
		  }

	  case CRASH_TIMEOUT:
		  // 死锁，中等限制（5次）
		  stats->timeout_count++;
		  if (stats->timeout_count < 5) {
			  LOG_ERROR("SUP", "进程%d心跳超时，重启（%u/5）",
						proc_id, stats->timeout_count);
			  return true;
		  } else {
			  LOG_FATAL("SUP", "进程%d心跳超时次数过多，触发系统复位", proc_id);
			  return false;
		  }

	  default:
		  // 未知原因，保守限制（5次）
		  stats->unknown_count++;
		  if (stats->unknown_count < 5) {
			  LOG_WARN("SUP", "进程%d未知原因崩溃，重启（%u/5）",
					   proc_id, stats->unknown_count);
			  return true;
		  } else {
			  LOG_ERROR("SUP", "进程%d未知原因崩溃次数过多，触发系统复位", proc_id);
			  return false;
		  }
  }
}

/**
* @brief 处理进程崩溃
*/
void handle_process_crash(process_id_t proc_id, pid_t pid, int status)
{
  // 识别崩溃原因
  crash_reason_t reason = identify_crash_reason(pid, status);

  // 记录到黑匣子
  uint32_t data[8] = {proc_id, pid, status, reason, 0, 0, 0, 0};
  blackbox_record(EVENT_PROCESS_CRASH, data, 4);

  // 决定是否重启
  if (should_restart_process(proc_id, reason)) {
	  restart_process(proc_id);
  } else {
	  // 触发系统复位
	  LOG_FATAL("SUP", "进程%d崩溃次数超限，触发硬件看门狗复位", proc_id);
	  trigger_watchdog_reset();
  }
}
```
实施要点：

1. 区分崩溃原因（段错误、断言失败、心跳超时）
2. 根据崩溃原因调整重启策略
3. SEU导致的崩溃允许更多次重启（10次）
4. 逻辑错误严格限制（3次）

预期效果：

- 提升系统在辐射环境下的容错能力
- 避免因瞬时SEU过早触发系统复位
- 对逻辑错误保持严格限制

---
#### 1.4.3 看门狗多级机制

问题描述：

当前Supervisor进程5秒周期喂狗，如果Supervisor自身卡死，10秒后硬件看门狗复位整个系统。可能导致不必要的系统复位，影响任务连
续性。

优化方案：
``` C
/************************************************
* apps/supervisor/watchdog_failover.c
* 看门狗故障转移机制
************************************************/

/**
* @brief 看门狗控制结构（共享内存）
*/
typedef struct {
  _Atomic uint64_t supervisor_heartbeat;
  _Atomic uint64_t telecommand_heartbeat;
  _Atomic bool supervisor_alive;
  _Atomic bool failover_active;
} watchdog_control_t;

static watchdog_control_t *g_wdt_ctrl = NULL;  // 共享内存

/**
* @brief Supervisor进程喂狗
*/
void supervisor_feed_watchdog(void)
{
  // 更新心跳
  atomic_store(&g_wdt_ctrl->supervisor_heartbeat, get_monotonic_ns());
  atomic_store(&g_wdt_ctrl->supervisor_alive, true);

  // 喂硬件看门狗
  feed_hardware_watchdog();
}

/**
* @brief Telecommand进程监控Supervisor状态
*/
void* watchdog_failover_thread(void* arg)
{
  while (1) {
	  uint

✻ Cogitated for 1m 27s

❯ 继续

●         uint64_t now = get_monotonic_ns();
	  uint64_t sup_hb = atomic_load(&g_wdt_ctrl->supervisor_heartbeat);

	  // 检查Supervisor心跳
	  if (now - sup_hb > 3000000000ULL) {  // Supervisor 3秒无心跳
		  LOG_ERROR("WDT", "Supervisor卡死，Telecommand接管喂狗");

		  atomic_store(&g_wdt_ctrl->supervisor_alive, false);
		  atomic_store(&g_wdt_ctrl->failover_active, true);

		  // Telecommand进程接管喂狗
		  feed_hardware_watchdog();

		  // 记录到黑匣子
		  uint32_t data[8] = {0};
		  blackbox_record(EVENT_WATCHDOG_FAILOVER, data, 0);

		  // 向卫星平台上报
		  send_alert_to_satellite(ALERT_SUPERVISOR_DEAD, 0);

		  // 尝试重启Supervisor进程（通过信号）
		  // 注意：Telecommand不能直接fork，因为它是实时进程
		  // 只能通过信号通知其他进程或等待硬件看门狗复位
	  }

	  // 如果故障转移已激活，继续喂狗
	  if (atomic_load(&g_wdt_ctrl->failover_active)) {
		  feed_hardware_watchdog();
	  }

	  OSAL_TaskDelay(1000);  // 1秒检查一次
  }
}

/**
* @brief 喂硬件看门狗
*/
void feed_hardware_watchdog(void)
{
  // 方法1：通过GPIO翻转
  HAL_GPIO_Toggle(WDT_GPIO_PIN);

  // 方法2：通过/dev/watchdog设备
  int wdt_fd = open("/dev/watchdog", O_WRONLY);
  if (wdt_fd >= 0) {
	  write(wdt_fd, "1", 1);
	  close(wdt_fd);
  }
}
```
实施要点：

1. Supervisor进程正常喂狗（5秒周期）
2. Telecommand进程监控Supervisor心跳（1秒周期）
3. Supervisor卡死时，Telecommand接管喂狗
4. 记录故障转移事件到黑匣子

预期效果：

- 避免因Supervisor卡死导致不必要的系统复位
- 提升系统在极端情况下的生存能力
- 保持任务连续性

---
### 1.5 实施优先级总结

| 优先级 | 优化项 | 预期工作量 | 风险 | 收益 |
|--------|--------|-----------|------|------|
| **P0** | 共享内存SEU防护 | 3-5天 | 低 | 高 |
| **P0** | 实时性监控和告警 | 2-3天 | 低 | 高 |
| **P0** | Flash写入隔离 | 2-3天 | 中 | 高 |
| **P1** | 遥控后遥测强制更新 | 2-3天 | 低 | 中 |
| **P1** | 动态超时调整 | 3-4天 | 低 | 中 |
| **P1** | 安全模式和黑匣子 | 5-7天 | 中 | 高 |
| **P2** | 通道健康度评估 | 2-3天 | 低 | 中 |
| **P2** | Supervisor降级策略 | 1-2天 | 低 | 中 |
| **P2** | 看门狗多级机制 | 2-3天 | 中 | 中 |

**总工作量估算**：
- P0级：7-11天
- P1级：10-14天
- P2级：5-8天
- 总计：22-33天（约1-1.5个月）

---
### 1.6 验证计划

#### 1.6.1 SEU防护验证

测试方法：

1. 软件注入测试：
``` C
// 模拟SEU翻转
void simulate_seu_flip(void *addr, uint32_t bit_pos)
{
  uint8_t *byte = (uint8_t*)addr;
  *byte ^= (1 << bit_pos);  // 翻转指定比特
}

// 测试用例
void test_seu_protection(void)
{
  telemetry_cache_entry_t entry;
  set_telemetry_cache_safe(&entry, &data, TM_FRESH, true);

  // 翻转第一个freshness副本
  simulate_seu_flip(&entry.freshness[0], 0);

  // 验证多数表决能否纠正
  tm_freshness_t freshness = get_freshness_safe(&entry);
  assert(freshness == TM_FRESH);  // 应该纠正为FRESH
}
```
2. 硬件辐射测试（如果条件允许）：
- 使用Co-60伽马射线源
- 使用质子束或重离子束
- 记录SEU发生率和纠正率

验收标准：
- 单比特翻转：100%自动纠正
- 双比特翻转：100%检测到错误
- 三模冗余失败率：<0.01%

---
#### 1.6.2 实时性验证

测试方法：

1. 压力测试：
``` shell
# 高频遥控遥测（每秒100条命令）
./test_high_frequency --rate 100 --duration 3600

# 同时运行Flash写入压力
dd if=/dev/zero of=/var/log/pmc/stress.log bs=1M count=1000 oflag=direct
```

2. 延迟分布统计：
``` C
// 记录1000次命令的延迟分布
uint32_t latency_histogram[20] = {0};  // 0-100us, 100-200us, ..., 1900-2000us

for (int i = 0; i < 1000; i++) {
  uint64_t start = get_monotonic_ns();
  handle_command(...);
  uint64_t latency_us = (get_monotonic_ns() - start) / 1000;

  uint32_t bucket = latency_us / 100;
  if (bucket < 20) {
	  latency_histogram[bucket]++;
  }
}
```
// 计算P50, P95, P99延迟

验收标准：
- P50延迟：<500μs
- P95延迟：<1000μs
- P99延迟：<1500μs
- P99.9延迟：<2000μs
- 超过2ms的比例：<0.1%

---
#### 1.6.3 故障恢复验证

测试方法：

1. 进程崩溃测试：
``` C
// 模拟段错误
void simulate_segfault(void)
{
  int *p = NULL;
  *p = 42;  // 触发SIGSEGV
}

// 模拟断言失败
void simulate_abort(void)
{
  assert(false);  // 触发SIGABRT
}

// 模拟死锁
void simulate_deadlock(void)
{
  while (1) {
	  // 不更新心跳
	  sleep(1);
  }
}
```
2. 故障注入测试：
``` shell
# 杀死Telemetry进程
kill -SEGV $(pgrep pmc_telemetry)

# 验证1秒内恢复
sleep 1
pgrep pmc_telemetry  # 应该有新的PID
```

验收标准：
- 进程崩溃后1秒内恢复
- 连续5次崩溃后进入降级模式
- 连续10次崩溃后触发系统复位
- 黑匣子正确记录所有崩溃事件

---
#### 1.6.4 长期稳定性验证

测试方法：

1. 7×24小时稳定性测试：
``` shell
# 启动PMC
./pmc_supervisor --config /etc/pmc/pmc.conf

# 持续发送遥控遥测命令（7天）
./test_long_term --duration 604800 --rate 10

# 监控系统状态
watch -n 60 'cat /var/log/pmc/status.log | tail -20'
```

2. 内存泄漏检测：
``` shell
# 使用valgrind检测内存泄漏
valgrind --leak-check=full --show-leak-kinds=all \
	   ./pmc_supervisor --config /etc/pmc/pmc.conf
```
验收标准：
- 7×24小时无崩溃
- 内存占用稳定（<50MB）
- CPU占用稳定（<60%）
- 无内存泄漏
- 实时性违规率：<0.1%

---
### 1.7 文档更新计划

完成优化后，需要更新以下文档：

1. 架构设计文档（本文档）：
- 更新第7章ACL设计（增加SEU防护）
- 更新第8章APP设计（增加实时性监控）
- 更新第10章性能分析（更新延迟数据）
2. API文档：
- 新增SEU防护API文档
- 新增黑匣子API文档
- 新增系统模式管理API文档
3. 测试文档：
- 新增SEU防护测试用例
- 新增实时性验证测试用例
- 新增故障恢复测试用例
4. 用户手册：
- 新增系统模式说明
- 新增黑匣子导出操作
- 新增实时性监控说明
5. 部署文档：
- 新增NVRAM配置说明
- 新增专用日志分区配置
- 新增实时性优化配置

---
### 1.8 风险评估

| 风险项 | 概率 | 影响 | 缓解措施 |
|--------|------|------|---------|
| 三模冗余增加内存开销 | 高 | 中 | 仅对关键数据结构使用三模冗余 |
| 实时性监控增加CPU开销 | 中 | 低 | 使用轻量级时间戳记录，异步分析 |
| 黑匣子NVRAM容量不足 | 中 | 中 | 使用环形缓冲区，保留最近1000条 |
| 动态超时算法不稳定 | 低 | 中 | 限制超时范围（500μs-5000μs） |
| 安全模式功能受限 | 低 | 高 | 保留关键遥控功能（复位、电源） |

---
### 1.9 后续优化方向

完成上述优化后，可以考虑以下长期优化方向：

1. AI辅助故障诊断：
- 使用机器学习分析黑匣子数据
- 自动识别故障模式
- 预测潜在故障
2. 自适应实时调度：
- 根据负载动态调整进程优先级
- 自动优化CPU亲和性
- 动态调整采集周期
3. 分布式冗余：
- 支持多个PMC互为备份
- 主备自动切换
- 数据同步机制
4. 形式化验证：
- 使用TLA+验证并发算法
- 使用SPIN验证状态机
- 使用Coq证明关键属性

---
### 1.10 总结

本章节记录了架构设计评审中发现的9个待优化点，按优先级分为P0（必须解决）、P1（强烈建议）、P2（可选优化）三个级别。

核心优化方向：
1. 可靠性增强：SEU防护、黑匣子、安全模式
2. 实时性保证：实时性监控、Flash隔离、动态超时
3. 智能化提升：通道健康度、自适应策略、故障诊断

预期收益：
- 系统可靠性提升30%（通过SEU防护和故障恢复优化）
- 实时性违规率降低90%（通过实时性监控和Flash隔离）
- 故障诊断效率提升50%（通过黑匣子和智能分析）

实施建议：
- 优先实施P0级优化（7-11天），确保系统基本可靠性
- 逐步实施P1级优化（10-14天），提升系统鲁棒性
- 根据实际需求选择性实施P2级优化（5-8天）

---
