#ifndef PMC_COMM_H
#define PMC_COMM_H

#include "osal.h"

/* 共享内存名称 */
#define SHM_TELEMETRY_CACHE    "/pmc_tm_cache"
#define SHM_SYSTEM_STATUS      "/pmc_status"
#define SHM_HEARTBEAT          "/pmc_heartbeat"

/* 共享内存大小 */
#define SHM_TM_CACHE_SIZE      (4 * 1024 * 1024)  /* 4MB */
#define SHM_STATUS_SIZE        (4 * 1024)         /* 4KB */
#define SHM_HEARTBEAT_SIZE     (4 * 1024)         /* 4KB */

/* 遥测数据定义 */
#define TM_MAX_COUNT           1024
#define TM_MAX_DATA_SIZE       256

/* 遥测新鲜度 */
typedef enum {
    TM_FRESH = 0,
    TM_STALE,
    TM_INVALID
} tm_freshness_t;

/* 遥测缓存条目 */
typedef struct {
    uint32 tm_id;
    uint8 data[TM_MAX_DATA_SIZE];
    uint32 data_size;
    uint64 timestamp_us;
    uint32 validity_ms;
    tm_freshness_t freshness;
    osal_id_t rwlock;
} tm_cache_entry_t;

/* 遥测缓存 */
typedef struct {
    uint32 entry_count;
    tm_cache_entry_t entries[TM_MAX_COUNT];
} tm_cache_t;

/* 进程心跳 */
typedef struct {
    _Atomic uint64 comm_heartbeat;
    _Atomic uint64 collector_heartbeat;
    _Atomic uint64 health_heartbeat;
    _Atomic uint64 supervisor_heartbeat;
    _Atomic uint64 logger_heartbeat;
} process_heartbeat_t;

/* Communication进程初始化 */
int32 PMC_Comm_Init(void);

/* Communication进程运行 */
int32 PMC_Comm_Run(void);

/* Communication进程清理 */
void PMC_Comm_Cleanup(void);

#endif /* PMC_COMM_H */
