#ifndef LIBCCM_IPC_H
#define LIBCCM_IPC_H

#include "osal/osal.h"

/* 共享内存名称定义 */
#define CCM_SHM_TELEMETRY_CACHE    "/ccm_tm_cache"
#define CCM_SHM_SYSTEM_STATUS      "/ccm_status"
#define CCM_SHM_PROCESS_HEARTBEAT  "/ccm_heartbeat"
#define CCM_SHM_LOG_RINGBUFFER     "/ccm_log_ring"

/* 共享内存大小定义 */
#define CCM_SHM_TM_CACHE_SIZE      (4 * 1024 * 1024)  /* 4MB */
#define CCM_SHM_STATUS_SIZE        (4 * 1024)         /* 4KB */
#define CCM_SHM_HEARTBEAT_SIZE     (4 * 1024)         /* 4KB */
#define CCM_SHM_LOG_SIZE           (1 * 1024 * 1024)  /* 1MB */

/* 遥测数据定义 */
#define CCM_TM_MAX_COUNT           1024
#define CCM_TM_MAX_DATA_SIZE       256

/* 进程ID枚举 */
typedef enum {
    CCM_PROCESS_COMM = 0,
    CCM_PROCESS_COLLECTOR,
    CCM_PROCESS_HEALTH,
    CCM_PROCESS_SUPERVISOR,
    CCM_PROCESS_LOGGER,
    CCM_PROCESS_MAX
} ccm_process_id_t;

/* 遥测数据新鲜度 */
typedef enum {
    CCM_TM_FRESH = 0,      /* 数据新鲜 */
    CCM_TM_STALE,          /* 数据过期但可用 */
    CCM_TM_INVALID         /* 数据无效 */
} ccm_tm_freshness_t;

/* 遥测缓存条目 */
typedef struct {
    uint32_t tm_id;                         /* 遥测ID */
    uint8_t data[CCM_TM_MAX_DATA_SIZE];     /* 遥测数据 */
    uint32_t data_size;                     /* 数据长度 */
    uint64_t timestamp_us;                  /* 时间戳(微秒) */
    uint32_t validity_ms;                   /* 有效期(毫秒) */
    ccm_tm_freshness_t freshness;           /* 新鲜度 */
    osal_mutex_t *rwlock;                   /* 读写锁 */
} ccm_tm_cache_entry_t;

/* 遥测缓存共享内存 */
typedef struct {
    uint32_t entry_count;                   /* 条目数量 */
    ccm_tm_cache_entry_t entries[CCM_TM_MAX_COUNT];
} ccm_tm_cache_t;

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
    osal_mutex_t *mutex;                    /* 互斥锁 */
} ccm_system_status_t;

/* 进程心跳 */
typedef struct {
    _Atomic uint64_t heartbeat_us[CCM_PROCESS_MAX];  /* 心跳时间戳(微秒) */
} ccm_process_heartbeat_t;

/* 日志环形缓冲区 */
#define CCM_LOG_ENTRY_SIZE  256
#define CCM_LOG_ENTRY_COUNT (CCM_SHM_LOG_SIZE / CCM_LOG_ENTRY_SIZE)

typedef struct {
    char entries[CCM_LOG_ENTRY_COUNT][CCM_LOG_ENTRY_SIZE];
    _Atomic uint32_t write_index;           /* 写索引 */
    _Atomic uint32_t read_index;            /* 读索引 */
} ccm_log_ringbuffer_t;

/* IPC辅助函数 - 遥测缓存操作 */
int32_t CCM_TM_Cache_Init(ccm_tm_cache_t **cache);
int32_t CCM_TM_Cache_Write(ccm_tm_cache_t *cache, uint32_t tm_id,
                          const uint8_t *data, uint32_t size, uint32_t validity_ms);
int32_t CCM_TM_Cache_Read(ccm_tm_cache_t *cache, uint32_t tm_id,
                         uint8_t *data, uint32_t *size, ccm_tm_freshness_t *freshness);
void CCM_TM_Cache_Cleanup(ccm_tm_cache_t *cache);

/* IPC辅助函数 - 系统状态操作 */
int32_t CCM_Status_Init(ccm_system_status_t **status);
int32_t CCM_Status_Write(ccm_system_status_t *status, const ccm_system_status_t *new_status);
int32_t CCM_Status_Read(ccm_system_status_t *status, ccm_system_status_t *out_status);
void CCM_Status_Cleanup(ccm_system_status_t *status);

/* IPC辅助函数 - 进程心跳操作 */
int32_t CCM_Heartbeat_Init(ccm_process_heartbeat_t **heartbeat);
int32_t CCM_Heartbeat_Update(ccm_process_heartbeat_t *heartbeat, ccm_process_id_t process_id);
int32_t CCM_Heartbeat_Check(ccm_process_heartbeat_t *heartbeat, ccm_process_id_t process_id,
                           uint32_t timeout_ms, bool *alive);
void CCM_Heartbeat_Cleanup(ccm_process_heartbeat_t *heartbeat);

/* IPC辅助函数 - 日志操作 */
int32_t CCM_Log_Init(ccm_log_ringbuffer_t **log_ring);
int32_t CCM_Log_Write(ccm_log_ringbuffer_t *log_ring, const char *log_entry);
int32_t CCM_Log_Read(ccm_log_ringbuffer_t *log_ring, char *log_entry, uint32_t size);
void CCM_Log_Cleanup(ccm_log_ringbuffer_t *log_ring);

#endif /* LIBCCM_IPC_H */
