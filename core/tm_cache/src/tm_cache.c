/**
 * @file tm_cache.c
 * @brief 遥测缓存管理实现
 * @note 使用共享内存实现进程间通信
 *       Telecommand进程读取（<50μs）
 *       Telemetry进程写入（后台更新）
 */

#include "tm_cache.h"
#include "osal.h"

/* 全局遥测缓存表 */
static tm_cache_entry_t *g_tm_cache = NULL;
static uint32_t g_max_entries = 0;
static bool g_cache_initialized = false;

/* 互斥锁（保护缓存访问） */
static osal_mutex_t *g_cache_mutex = NULL;

/**
 * @brief 获取单调时间戳（微秒）
 */
static uint64_t get_monotonic_us(void)
{
    return OSAL_GetMonotonicTime();
}

/**
 * @brief 计算CRC32校验和（简化版）
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t i;
    int32_t j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];

        for (j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

/**
 * @brief 初始化遥测缓存
 */
int32_t TM_Cache_Init(uint32_t max_entries)
{
    int32_t ret;
    uint32_t i;

    if (g_cache_initialized) {
        return OSAL_SUCCESS;
    }

    if (max_entries == 0) {
        return OSAL_ERR_INVALID_SIZE;
    }

    /* 分配缓存表 */
    g_tm_cache = (tm_cache_entry_t *)OSAL_Malloc(sizeof(tm_cache_entry_t) * max_entries);
    if (g_tm_cache == NULL) {
        return OSAL_ERR_NO_MEMORY;
    }

    g_max_entries = max_entries;

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&g_cache_mutex);
    if (ret != OSAL_SUCCESS) {
        OSAL_Free(g_tm_cache);
        g_tm_cache = NULL;
        return ret;
    }

    /* 初始化缓存表 */
    OSAL_Memset(g_tm_cache, 0, sizeof(tm_cache_entry_t) * max_entries);

    for (i = 0; i < max_entries; i++) {
        g_tm_cache[i].tm_id = i;
        g_tm_cache[i].freshness = TM_CACHE_STATUS_INVALID;
        g_tm_cache[i].valid = false;
    }

    g_cache_initialized = true;

    return OSAL_SUCCESS;
}

/**
 * @brief 清理遥测缓存
 */
void TM_Cache_Deinit(void)
{
    if (!g_cache_initialized) {
        return;
    }

    OSAL_MutexDelete(g_cache_mutex);
    OSAL_Free(g_tm_cache);
    g_tm_cache = NULL;
    g_max_entries = 0;
    g_cache_initialized = false;
}

/**
 * @brief 写入遥测数据到缓存（Telemetry进程调用）
 */
int32_t TM_Cache_Write(uint32_t tm_id, const uint8_t *data, uint32_t data_len)
{
    tm_cache_entry_t *entry;

    if (!g_cache_initialized || tm_id >= g_max_entries || !data || data_len == 0 || data_len > 256) {
        return OSAL_ERR_INVALID_SIZE;
    }

    OSAL_MutexLock(g_cache_mutex);

    entry = &g_tm_cache[tm_id];

    /* 复制数据 */
    OSAL_Memcpy(entry->data, data, data_len);
    entry->data_len = data_len;

    /* 更新元数据 */
    entry->timestamp_us = get_monotonic_us();
    entry->freshness = TM_CACHE_STATUS_FRESH;
    entry->valid = true;
    entry->update_count++;

    /* 计算校验和 */
    entry->checksum = calculate_crc32(data, data_len);

    OSAL_MutexUnlock(g_cache_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 读取遥测数据（Telecommand进程调用，<50μs）
 */
int32_t TM_Cache_Read(uint32_t tm_id, tm_cache_response_t *response)
{
    tm_cache_entry_t *entry;
    uint64_t now;
    uint32_t age_ms;
    tm_cache_status_t freshness;

    if (!g_cache_initialized || tm_id >= g_max_entries || !response) {
        return OSAL_ERR_INVALID_SIZE;
    }

    OSAL_MutexLock(g_cache_mutex);

    entry = &g_tm_cache[tm_id];

    /* 检查是否有效 */
    if (!entry->valid) {
        OSAL_MutexUnlock(g_cache_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 计算数据年龄 */
    now = get_monotonic_us();
    age_ms = (uint32_t)((now - entry->timestamp_us) / 1000);

    /* 判断新鲜度 */
    if (entry->freshness == TM_CACHE_STATUS_INVALID) {
        freshness = TM_CACHE_STATUS_INVALID;
    } else if (age_ms < entry->validity_ms) {
        freshness = TM_CACHE_STATUS_FRESH;
    } else {
        freshness = TM_CACHE_STATUS_STALE;
    }

    /* 封装应答 */
    response->tm_id = tm_id;
    OSAL_Memcpy(response->data, entry->data, entry->data_len);
    response->data_len = entry->data_len;
    response->timestamp_us = entry->timestamp_us;
    response->age_ms = age_ms;
    response->freshness = freshness;
    response->checksum = entry->checksum;

    /* 更新统计 */
    entry->read_count++;

    OSAL_MutexUnlock(g_cache_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 标记遥测为STALE（遥控命令执行后调用）
 */
int32_t TM_Cache_Invalidate(uint32_t tm_id)
{
    tm_cache_entry_t *entry;

    if (!g_cache_initialized || tm_id >= g_max_entries) {
        return OSAL_ERR_INVALID_SIZE;
    }

    OSAL_MutexLock(g_cache_mutex);

    entry = &g_tm_cache[tm_id];

    if (entry->valid) {
        entry->freshness = TM_CACHE_STATUS_STALE;
    }

    OSAL_MutexUnlock(g_cache_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 批量标记遥测为STALE
 */
int32_t TM_Cache_InvalidateBatch(const uint32_t *tm_ids, uint32_t count)
{
    uint32_t i;

    if (!g_cache_initialized || !tm_ids || count == 0) {
        return OSAL_ERR_INVALID_SIZE;
    }


    for (i = 0; i < count; i++) {
        TM_Cache_Invalidate(tm_ids[i]);
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 获取缓存统计信息
 */
int32_t TM_Cache_GetStats(uint32_t *total_count, uint32_t *valid_count,
                           uint32_t *fresh_count, uint32_t *stale_count)
{
    uint64_t now;
    uint32_t i;

    if (!g_cache_initialized) {
        return OSAL_ERR_GENERIC;
    }

    OSAL_MutexLock(g_cache_mutex);

    *total_count = g_max_entries;
    *valid_count = 0;
    *fresh_count = 0;
    *stale_count = 0;

    now = get_monotonic_us();

    for (i = 0; i < g_max_entries; i++) {
        tm_cache_entry_t *entry = &g_tm_cache[i];
        uint32_t age_ms;

        if (!entry->valid) {
            continue;
        }

        (*valid_count)++;

        /* 计算当前新鲜度 */
        age_ms = (uint32_t)((now - entry->timestamp_us) / 1000);

        if (entry->freshness == TM_CACHE_STATUS_INVALID) {
            /* 已标记为无效 */
        } else if (age_ms < entry->validity_ms) {
            (*fresh_count)++;
        } else {
            (*stale_count)++;
        }
    }

    OSAL_MutexUnlock(g_cache_mutex);

    return OSAL_SUCCESS;
}
