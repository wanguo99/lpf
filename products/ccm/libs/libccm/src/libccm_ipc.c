#include "libccm/libccm_ipc.h"
#include "osal/util/osal_log.h"

/* 遥测缓存初始化 */
int32_t PMC_TM_Cache_Init(pmc_tm_cache_t **cache)
{
    osal_shm_t shm;
    int32_t ret;
    uint32_t i;

    /* 创建或打开共享内存 */
    ret = OSAL_ShmCreate(PMC_SHM_TELEMETRY_CACHE, PMC_SHM_TM_CACHE_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "创建遥测缓存共享内存失败: %d", ret);
        return ret;
    }

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, (void **)cache);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "映射遥测缓存失败: %d", ret);
        OSAL_ShmClose(shm);
        return ret;
    }

    /* 初始化缓存条目 */
    (*cache)->entry_count = PMC_TM_MAX_COUNT;
    for (i = 0; i < PMC_TM_MAX_COUNT; i++) {
        pmc_tm_cache_entry_t *entry = &(*cache)->entries[i];
        entry->tm_id = i;
        entry->data_size = 0;
        entry->timestamp_us = 0;
        entry->validity_ms = 1000;  /* 默认1秒有效期 */
        entry->freshness = PMC_TM_INVALID;

        /* 创建读写锁 */
        ret = OSAL_MutexCreate(&entry->rwlock);
        if (ret != OSAL_SUCCESS) {
            LOG_ERROR("LIBPMC", "创建遥测锁失败: %d", ret);
            return ret;
        }
    }

    OSAL_ShmClose(shm);
    return OSAL_SUCCESS;
}

/* 计算新鲜度 */
static pmc_tm_freshness_t calculate_freshness(uint64_t timestamp_us, uint32_t validity_ms)
{
    uint64_t now_us = OSAL_GetMonotonicTime();
    uint64_t age_ms = (now_us - timestamp_us) / 1000;

    if (age_ms > validity_ms * 2) {
        return PMC_TM_INVALID;
    } else if (age_ms > validity_ms) {
        return PMC_TM_STALE;
    } else {
        return PMC_TM_FRESH;
    }
}

/* 写入遥测缓存 */
int32_t PMC_TM_Cache_Write(pmc_tm_cache_t *cache, uint32_t tm_id,
                          const uint8_t *data, uint32_t size, uint32_t validity_ms)
{
    pmc_tm_cache_entry_t *entry;
    int32_t ret;

    if (!cache || !data || tm_id >= PMC_TM_MAX_COUNT || size > PMC_TM_MAX_DATA_SIZE) {
        return OSAL_ERR_INVALID_POINTER;
    }

    entry = &cache->entries[tm_id];

    /* 写锁 */
    ret = OSAL_MutexLock(entry->rwlock);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 写入数据 */
    OSAL_Memcpy(entry->data, data, size);
    entry->data_size = size;
    entry->timestamp_us = OSAL_GetMonotonicTime();
    entry->validity_ms = validity_ms;
    entry->freshness = PMC_TM_FRESH;

    /* 解锁 */
    OSAL_MutexUnlock(entry->rwlock);

    return OSAL_SUCCESS;
}

/* 读取遥测缓存 */
int32_t PMC_TM_Cache_Read(pmc_tm_cache_t *cache, uint32_t tm_id,
                         uint8_t *data, uint32_t *size, pmc_tm_freshness_t *freshness)
{
    pmc_tm_cache_entry_t *entry;
    int32_t ret;

    if (!cache || !data || !size || tm_id >= PMC_TM_MAX_COUNT) {
        return OSAL_ERR_INVALID_POINTER;
    }

    entry = &cache->entries[tm_id];

    /* 读锁 */
    ret = OSAL_MutexLock(entry->rwlock);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 计算新鲜度 */
    entry->freshness = calculate_freshness(entry->timestamp_us, entry->validity_ms);

    /* 读取数据 */
    if (entry->freshness != PMC_TM_INVALID) {
        OSAL_Memcpy(data, entry->data, entry->data_size);
        *size = entry->data_size;
        if (freshness) {
            *freshness = entry->freshness;
        }
        ret = OSAL_SUCCESS;
    } else {
        ret = OSAL_ERR_INVALID_POINTER;
    }

    /* 解锁 */
    OSAL_MutexUnlock(entry->rwlock);

    return ret;
}

/* 清理遥测缓存 */
void PMC_TM_Cache_Cleanup(pmc_tm_cache_t *cache)
{
    if (cache) {
        OSAL_ShmUnmap(cache, PMC_SHM_TM_CACHE_SIZE);
    }
}

/* 系统状态初始化 */
int32_t PMC_Status_Init(pmc_system_status_t **status)
{
    osal_shm_t shm;
    int32_t ret;

    /* 创建或打开共享内存 */
    ret = OSAL_ShmCreate(PMC_SHM_SYSTEM_STATUS, PMC_SHM_STATUS_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "创建系统状态共享内存失败: %d", ret);
        return ret;
    }

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, (void **)status);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "映射系统状态失败: %d", ret);
        OSAL_ShmClose(shm);
        return ret;
    }

    /* 初始化状态 */
    (*status)->server_online = false;
    (*status)->mcu_online = false;
    (*status)->fpga_online = false;
    (*status)->cpld_online = false;
    (*status)->cpu_temp = 0;
    (*status)->board_temp = 0;
    (*status)->voltage_54v = 0;
    (*status)->voltage_12v = 0;

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&(*status)->mutex);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "创建状态锁失败: %d", ret);
        return ret;
    }

    OSAL_ShmClose(shm);
    return OSAL_SUCCESS;
}

/* 写入系统状态 */
int32_t PMC_Status_Write(pmc_system_status_t *status, const pmc_system_status_t *new_status)
{
    int32_t ret;

    if (!status || !new_status) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = OSAL_MutexLock(status->mutex);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    status->server_online = new_status->server_online;
    status->mcu_online = new_status->mcu_online;
    status->fpga_online = new_status->fpga_online;
    status->cpld_online = new_status->cpld_online;
    status->cpu_temp = new_status->cpu_temp;
    status->board_temp = new_status->board_temp;
    status->voltage_54v = new_status->voltage_54v;
    status->voltage_12v = new_status->voltage_12v;

    OSAL_MutexUnlock(status->mutex);
    return OSAL_SUCCESS;
}

/* 读取系统状态 */
int32_t PMC_Status_Read(pmc_system_status_t *status, pmc_system_status_t *out_status)
{
    int32_t ret;

    if (!status || !out_status) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = OSAL_MutexLock(status->mutex);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    out_status->server_online = status->server_online;
    out_status->mcu_online = status->mcu_online;
    out_status->fpga_online = status->fpga_online;
    out_status->cpld_online = status->cpld_online;
    out_status->cpu_temp = status->cpu_temp;
    out_status->board_temp = status->board_temp;
    out_status->voltage_54v = status->voltage_54v;
    out_status->voltage_12v = status->voltage_12v;

    OSAL_MutexUnlock(status->mutex);
    return OSAL_SUCCESS;
}

/* 清理系统状态 */
void PMC_Status_Cleanup(pmc_system_status_t *status)
{
    if (status) {
        OSAL_ShmUnmap(status, PMC_SHM_STATUS_SIZE);
    }
}

/* 进程心跳初始化 */
int32_t PMC_Heartbeat_Init(pmc_process_heartbeat_t **heartbeat)
{
    osal_shm_t shm;
    int32_t ret;
    uint32_t i;

    /* 创建或打开共享内存 */
    ret = OSAL_ShmCreate(PMC_SHM_PROCESS_HEARTBEAT, PMC_SHM_HEARTBEAT_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "创建心跳共享内存失败: %d", ret);
        return ret;
    }

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, (void **)heartbeat);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "映射心跳失败: %d", ret);
        OSAL_ShmClose(shm);
        return ret;
    }

    /* 初始化心跳 */
    for (i = 0; i < PMC_PROCESS_MAX; i++) {
        (*heartbeat)->heartbeat_us[i] = 0;
    }

    OSAL_ShmClose(shm);
    return OSAL_SUCCESS;
}

/* 更新进程心跳 */
int32_t PMC_Heartbeat_Update(pmc_process_heartbeat_t *heartbeat, pmc_process_id_t process_id)
{
    if (!heartbeat || process_id >= PMC_PROCESS_MAX) {
        return OSAL_ERR_INVALID_POINTER;
    }

    heartbeat->heartbeat_us[process_id] = OSAL_GetMonotonicTime();
    return OSAL_SUCCESS;
}

/* 检查进程心跳 */
int32_t PMC_Heartbeat_Check(pmc_process_heartbeat_t *heartbeat, pmc_process_id_t process_id,
                           uint32_t timeout_ms, bool *alive)
{
    uint64_t now_us;
    uint64_t last_hb;
    uint64_t age_ms;

    if (!heartbeat || !alive || process_id >= PMC_PROCESS_MAX) {
        return OSAL_ERR_INVALID_POINTER;
    }

    now_us = OSAL_GetMonotonicTime();
    last_hb = heartbeat->heartbeat_us[process_id];
    age_ms = (now_us - last_hb) / 1000;

    *alive = (age_ms < timeout_ms);
    return OSAL_SUCCESS;
}

/* 清理进程心跳 */
void PMC_Heartbeat_Cleanup(pmc_process_heartbeat_t *heartbeat)
{
    if (heartbeat) {
        OSAL_ShmUnmap(heartbeat, PMC_SHM_HEARTBEAT_SIZE);
    }
}

/* 日志环形缓冲区初始化 */
int32_t PMC_Log_Init(pmc_log_ringbuffer_t **log_ring)
{
    osal_shm_t shm;
    int32_t ret;

    /* 创建或打开共享内存 */
    ret = OSAL_ShmCreate(PMC_SHM_LOG_RINGBUFFER, PMC_SHM_LOG_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "创建日志共享内存失败: %d", ret);
        return ret;
    }

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, (void **)log_ring);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LIBPMC", "映射日志缓冲区失败: %d", ret);
        OSAL_ShmClose(shm);
        return ret;
    }

    /* 初始化索引 */
    (*log_ring)->write_index = 0;
    (*log_ring)->read_index = 0;

    OSAL_ShmClose(shm);
    return OSAL_SUCCESS;
}

/* 写入日志 */
int32_t PMC_Log_Write(pmc_log_ringbuffer_t *log_ring, const char *log_entry)
{
    uint32_t write_idx;
    uint32_t next_idx;

    if (!log_ring || !log_entry) {
        return OSAL_ERR_INVALID_POINTER;
    }

    write_idx = log_ring->write_index;
    next_idx = (write_idx + 1) % PMC_LOG_ENTRY_COUNT;

    /* 检查是否满 */
    if (next_idx == log_ring->read_index) {
        return OSAL_ERR_INVALID_POINTER;  /* 缓冲区满 */
    }

    /* 写入日志 */
    OSAL_Strncpy(log_ring->entries[write_idx], log_entry, PMC_LOG_ENTRY_SIZE - 1);
    log_ring->entries[write_idx][PMC_LOG_ENTRY_SIZE - 1] = '\0';

    /* 更新写索引 */
    log_ring->write_index = next_idx;

    return OSAL_SUCCESS;
}

/* 读取日志 */
int32_t PMC_Log_Read(pmc_log_ringbuffer_t *log_ring, char *log_entry, uint32_t size)
{
    uint32_t read_idx;

    if (!log_ring || !log_entry || size < PMC_LOG_ENTRY_SIZE) {
        return OSAL_ERR_INVALID_POINTER;
    }

    read_idx = log_ring->read_index;

    /* 检查是否空 */
    if (read_idx == log_ring->write_index) {
        return OSAL_ERR_INVALID_POINTER;  /* 缓冲区空 */
    }

    /* 读取日志 */
    OSAL_Strncpy(log_entry, log_ring->entries[read_idx], size - 1);
    log_entry[size - 1] = '\0';

    /* 更新读索引 */
    log_ring->read_index = (read_idx + 1) % PMC_LOG_ENTRY_COUNT;

    return OSAL_SUCCESS;
}

/* 清理日志缓冲区 */
void PMC_Log_Cleanup(pmc_log_ringbuffer_t *log_ring)
{
    if (log_ring) {
        OSAL_ShmUnmap(log_ring, PMC_SHM_LOG_SIZE);
    }
}
