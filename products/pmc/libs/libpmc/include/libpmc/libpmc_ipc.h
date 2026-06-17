#ifndef LIBPMC_IPC_H
#define LIBPMC_IPC_H

/* 注意：源文件需要先包含 osal.h，再包含本头文件
 * 示例：
 *   #include "osal.h"
 *   #include "libpmc_ipc.h"
 */

/* 共享内存名称定义 */
#define PMC_SHM_TELEMETRY_CACHE    "/pmc_tm_cache"
#define PMC_SHM_SYSTEM_STATUS      "/pmc_status"
#define PMC_SHM_PROCESS_HEARTBEAT  "/pmc_heartbeat"
#define PMC_SHM_LOG_RINGBUFFER     "/pmc_log_ring"

/* 共享内存大小定义 */
#define PMC_SHM_TM_CACHE_SIZE      (4 * 1024 * 1024)  /* 4MB */
#define PMC_SHM_STATUS_SIZE        (4 * 1024)         /* 4KB */
#define PMC_SHM_HEARTBEAT_SIZE     (4 * 1024)         /* 4KB */
#define PMC_SHM_LOG_SIZE           (1 * 1024 * 1024)  /* 1MB */

/* 遥测数据定义 */
#define PMC_TM_MAX_COUNT           0x400
#define PMC_TM_MAX_DATA_SIZE       0x100

/* 进程ID枚举 */
typedef enum {
    PMC_PROCESS_COMM = 0x0,
    PMC_PROCESS_COLLECTOR,
    PMC_PROCESS_HEALTH,
    PMC_PROCESS_SUPERVISOR,
    PMC_PROCESS_LOGGER,
    PMC_PROCESS_MAX
} pmc_process_id_t;

/* 遥测数据新鲜度 */
typedef enum {
    PMC_TM_FRESH = 0x0,      /* 数据新鲜 */
    PMC_TM_STALE,          /* 数据过期但可用 */
    PMC_TM_INVALID         /* 数据无效 */
} pmc_tm_freshness_t;

/* 遥测缓存条目 */
typedef struct {
    uint32_t tm_id;                         /* 遥测ID */
    uint8_t data[PMC_TM_MAX_DATA_SIZE];     /* 遥测数据 */
    uint32_t data_size;                     /* 数据长度 */
    uint64_t timestamp_us;                  /* 时间戳(微秒) */
    uint32_t validity_ms;                   /* 有效期(毫秒) */
    pmc_tm_freshness_t freshness;           /* 新鲜度 */
    osal_mutex_t rwlock;                    /* 读写锁 */
} pmc_tm_cache_entry_t;

/* 遥测缓存共享内存 */
typedef struct {
    uint32_t entry_count;                   /* 条目数量 */
    pmc_tm_cache_entry_t entries[PMC_TM_MAX_COUNT];
} pmc_tm_cache_t;

/* 系统状态 */
typedef struct {
    bool server_online;                     /* 服务器在线 */
    bool mcu_online;                        /* MCU在线 */
    bool fpga_online;                       /* FPGA在线 */
    bool cpld_online;                       /* CPLD在线 */
    int32_t cpu_temp;                       /* CPU温度(℃) */
    int32_t board_temp;                     /* 板卡温度(℃) */
    uint32_t voltage_54v;                   /* 54V电压(mV) */
    uint32_t voltage_12v;                   /* 12V电压(mV) */
    osal_mutex_t mutex;                    /* 互斥锁 */
} pmc_system_status_t;

/* 进程心跳 */
typedef struct {
    _Atomic uint64_t heartbeat_us[PMC_PROCESS_MAX];  /* 心跳时间戳(微秒) */
} pmc_process_heartbeat_t;

/* 日志环形缓冲区 */
#define PMC_LOG_ENTRY_SIZE  0x100
#define PMC_LOG_ENTRY_COUNT (PMC_SHM_LOG_SIZE / PMC_LOG_ENTRY_SIZE)

typedef struct {
    char entries[PMC_LOG_ENTRY_COUNT][PMC_LOG_ENTRY_SIZE];
    _Atomic uint32_t write_index;           /* 写索引 */
    _Atomic uint32_t read_index;            /* 读索引 */
} pmc_log_ringbuffer_t;

/* IPC辅助函数 - 遥测缓存操作 */
int32_t PMC_TM_Cache_Init(pmc_tm_cache_t **cache);
int32_t PMC_TM_Cache_Write(pmc_tm_cache_t *cache, uint32_t tm_id,
                          const uint8_t *data, uint32_t size, uint32_t validity_ms);
int32_t PMC_TM_Cache_Read(pmc_tm_cache_t *cache, uint32_t tm_id,
                         uint8_t *data, uint32_t *size, pmc_tm_freshness_t *freshness);
void PMC_TM_Cache_Cleanup(pmc_tm_cache_t *cache);

/* IPC辅助函数 - 系统状态操作 */
int32_t PMC_Status_Init(pmc_system_status_t **status);
int32_t PMC_Status_Write(pmc_system_status_t *status, const pmc_system_status_t *new_status);
int32_t PMC_Status_Read(pmc_system_status_t *status, pmc_system_status_t *out_status);
void PMC_Status_Cleanup(pmc_system_status_t *status);

/* IPC辅助函数 - 进程心跳操作 */
int32_t PMC_Heartbeat_Init(pmc_process_heartbeat_t **heartbeat);
int32_t PMC_Heartbeat_Update(pmc_process_heartbeat_t *heartbeat, pmc_process_id_t process_id);
int32_t PMC_Heartbeat_Check(pmc_process_heartbeat_t *heartbeat, pmc_process_id_t process_id,
                           uint32_t timeout_ms, bool *alive);
void PMC_Heartbeat_Cleanup(pmc_process_heartbeat_t *heartbeat);

/* IPC辅助函数 - 日志操作 */
int32_t PMC_Log_Init(pmc_log_ringbuffer_t **log_ring);
int32_t PMC_Log_Write(pmc_log_ringbuffer_t *log_ring, const char *log_entry);
int32_t PMC_Log_Read(pmc_log_ringbuffer_t *log_ring, char *log_entry, uint32_t size);
void PMC_Log_Cleanup(pmc_log_ringbuffer_t *log_ring);

#endif /* LIBPMC_IPC_H */
