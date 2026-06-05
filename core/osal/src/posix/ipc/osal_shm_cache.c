/**
 * @file osal_shm_cache.c
 * @brief OSAL共享内存缓存实现（POSIX）
 *
 * 使用POSIX共享内存和互斥锁实现进程间缓存
 */

#include "osal.h"
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* 缓存表最大数量 */
#define MAX_CACHES  16

/* 共享内存缓存头部 */
typedef struct {
    uint32_t magic;                 /* 魔数：0x43414348 ("CACH") */
    uint32_t version;               /* 版本号 */
    uint32_t max_entries;           /* 最大条目数 */
    uint32_t entry_size;            /* 条目大小 */
    pthread_mutex_t mutex;          /* 互斥锁（进程间） */
    osal_cache_entry_t entries[0];  /* 柔性数组：缓存条目 */
} shm_cache_header_t;

/* 缓存描述符 */
typedef struct {
    bool in_use;
    char name[64];
    int shm_fd;
    void *shm_ptr;
    size_t shm_size;
    shm_cache_header_t *header;
} cache_descriptor_t;

/* 全局缓存表 */
static cache_descriptor_t g_cache_table[MAX_CACHES];
static pthread_mutex_t g_cache_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 魔数和版本 */
#define SHM_CACHE_MAGIC    0x43414348
#define SHM_CACHE_VERSION  1

/**
 * @brief 计算CRC32校验和
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
 * @brief 获取单调时间戳（微秒）
 */
static uint64_t get_monotonic_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/**
 * @brief 查找空闲的缓存槽位
 */
static int32_t find_free_cache_slot(void)
{
    uint32_t i;
    for (i = 0; i < MAX_CACHES; i++) {
        if (!g_cache_table[i].in_use) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 创建共享内存缓存
 */
int32_t OSAL_CacheCreate(const char *name, uint32_t max_entries, osal_id_t *cache_id)
{
    int32_t slot;
    cache_descriptor_t *cache;
    size_t shm_size;
    int shm_fd;
    void *shm_ptr;
    shm_cache_header_t *header;
    pthread_mutexattr_t mutex_attr;
    uint32_t i;

    if (name == NULL || max_entries == 0 || cache_id == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_cache_table_mutex);

    /* 查找空闲槽位 */
    slot = find_free_cache_slot();
    if (slot < 0) {
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_NO_FREE_IDS;
    }

    /* 计算共享内存大小 */
    shm_size = sizeof(shm_cache_header_t) + max_entries * sizeof(osal_cache_entry_t);

    /* 创建共享内存 */
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 设置共享内存大小 */
    if (ftruncate(shm_fd, shm_size) < 0) {
        close(shm_fd);
        shm_unlink(name);
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 映射共享内存 */
    shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        shm_unlink(name);
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化缓存头部 */
    header = (shm_cache_header_t *)shm_ptr;
    memset(header, 0, shm_size);
    header->magic = SHM_CACHE_MAGIC;
    header->version = SHM_CACHE_VERSION;
    header->max_entries = max_entries;
    header->entry_size = sizeof(osal_cache_entry_t);

    /* 初始化进程间互斥锁 */
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&header->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    /* 初始化缓存条目 */
    for (i = 0; i < max_entries; i++) {
        header->entries[i].entry_id = i;
        header->entries[i].status = OSAL_CACHE_STATUS_INVALID;
        header->entries[i].valid = false;
    }

    /* 填充缓存描述符 */
    cache = &g_cache_table[slot];
    cache->in_use = true;
    strncpy(cache->name, name, sizeof(cache->name) - 1);
    cache->name[sizeof(cache->name) - 1] = '\0';
    cache->shm_fd = shm_fd;
    cache->shm_ptr = shm_ptr;
    cache->shm_size = shm_size;
    cache->header = header;

    *cache_id = slot;

    pthread_mutex_unlock(&g_cache_table_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 打开已存在的共享内存缓存
 */
int32_t OSAL_CacheOpen(const char *name, osal_id_t *cache_id)
{
    int32_t slot;
    cache_descriptor_t *cache;
    int shm_fd;
    void *shm_ptr;
    shm_cache_header_t *header;
    struct stat st;

    if (name == NULL || cache_id == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_cache_table_mutex);

    /* 查找空闲槽位 */
    slot = find_free_cache_slot();
    if (slot < 0) {
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_NO_FREE_IDS;
    }

    /* 打开共享内存 */
    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd < 0) {
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 获取共享内存大小 */
    if (fstat(shm_fd, &st) < 0) {
        close(shm_fd);
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 映射共享内存 */
    shm_ptr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 验证魔数 */
    header = (shm_cache_header_t *)shm_ptr;
    if (header->magic != SHM_CACHE_MAGIC) {
        munmap(shm_ptr, st.st_size);
        close(shm_fd);
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 填充缓存描述符 */
    cache = &g_cache_table[slot];
    cache->in_use = true;
    strncpy(cache->name, name, sizeof(cache->name) - 1);
    cache->name[sizeof(cache->name) - 1] = '\0';
    cache->shm_fd = shm_fd;
    cache->shm_ptr = shm_ptr;
    cache->shm_size = st.st_size;
    cache->header = header;

    *cache_id = slot;

    pthread_mutex_unlock(&g_cache_table_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 删除共享内存缓存
 */
int32_t OSAL_CacheDelete(osal_id_t cache_id)
{
    cache_descriptor_t *cache;

    if (cache_id >= MAX_CACHES) {
        return OSAL_ERR_INVALID_ID;
    }

    pthread_mutex_lock(&g_cache_table_mutex);

    cache = &g_cache_table[cache_id];
    if (!cache->in_use) {
        pthread_mutex_unlock(&g_cache_table_mutex);
        return OSAL_ERR_INVALID_ID;
    }

    /* 销毁互斥锁 */
    pthread_mutex_destroy(&cache->header->mutex);

    /* 解除映射 */
    munmap(cache->shm_ptr, cache->shm_size);

    /* 关闭文件描述符 */
    close(cache->shm_fd);

    /* 删除共享内存 */
    shm_unlink(cache->name);

    /* 清空描述符 */
    memset(cache, 0, sizeof(cache_descriptor_t));

    pthread_mutex_unlock(&g_cache_table_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 写入缓存条目
 */
int32_t OSAL_CacheWrite(osal_id_t cache_id, uint32_t entry_id,
                        const uint8_t *data, uint32_t data_len,
                        uint32_t data_validity_ms)
{
    cache_descriptor_t *cache;
    osal_cache_entry_t *entry;

    if (cache_id >= MAX_CACHES || data == NULL ||
        data_len == 0 || data_len > OSAL_SHM_CACHE_MAX_DATA_SIZE) {
        return OSAL_ERR_INVALID_PARAM;
    }

    cache = &g_cache_table[cache_id];
    if (!cache->in_use) {
        return OSAL_ERR_INVALID_ID;
    }

    if (entry_id >= cache->header->max_entries) {
        return OSAL_ERR_INVALID_PARAM;
    }

    entry = &cache->header->entries[entry_id];

    /* 加锁 */
    pthread_mutex_lock(&cache->header->mutex);

    /* 复制数据 */
    memcpy(entry->data, data, data_len);
    entry->data_len = data_len;

    /* 更新元数据 */
    entry->timestamp_us = get_monotonic_us();
    entry->data_validity_ms = data_validity_ms;
    entry->status = OSAL_CACHE_STATUS_FRESH;
    entry->valid = true;
    entry->update_count++;

    /* 计算校验和 */
    entry->checksum = calculate_crc32(data, data_len);

    /* 解锁 */
    pthread_mutex_unlock(&cache->header->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 读取缓存条目
 */
int32_t OSAL_CacheRead(osal_id_t cache_id, uint32_t entry_id,
                       osal_cache_result_t *result)
{
    cache_descriptor_t *cache;
    osal_cache_entry_t *entry;
    uint64_t now;
    uint32_t age_ms;
    osal_cache_status_t status;

    if (cache_id >= MAX_CACHES || result == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    cache = &g_cache_table[cache_id];
    if (!cache->in_use) {
        return OSAL_ERR_INVALID_ID;
    }

    if (entry_id >= cache->header->max_entries) {
        return OSAL_ERR_INVALID_PARAM;
    }

    entry = &cache->header->entries[entry_id];

    /* 加锁 */
    pthread_mutex_lock(&cache->header->mutex);

    /* 检查是否有效 */
    if (!entry->valid) {
        pthread_mutex_unlock(&cache->header->mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 计算数据年龄 */
    now = get_monotonic_us();
    age_ms = (uint32_t)((now - entry->timestamp_us) / 1000);

    /* 判断新鲜度 */
    if (entry->status == OSAL_CACHE_STATUS_INVALID) {
        status = OSAL_CACHE_STATUS_INVALID;
    } else if (age_ms < entry->data_validity_ms) {
        status = OSAL_CACHE_STATUS_FRESH;
    } else {
        status = OSAL_CACHE_STATUS_STALE;
    }

    /* 封装结果 */
    result->entry_id = entry_id;
    memcpy(result->data, entry->data, entry->data_len);
    result->data_len = entry->data_len;
    result->timestamp_us = entry->timestamp_us;
    result->age_ms = age_ms;
    result->status = status;
    result->checksum = entry->checksum;

    /* 更新统计 */
    entry->read_count++;

    /* 解锁 */
    pthread_mutex_unlock(&cache->header->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 标记缓存条目为STALE
 */
int32_t OSAL_CacheInvalidate(osal_id_t cache_id, uint32_t entry_id)
{
    cache_descriptor_t *cache;
    osal_cache_entry_t *entry;

    if (cache_id >= MAX_CACHES) {
        return OSAL_ERR_INVALID_PARAM;
    }

    cache = &g_cache_table[cache_id];
    if (!cache->in_use) {
        return OSAL_ERR_INVALID_ID;
    }

    if (entry_id >= cache->header->max_entries) {
        return OSAL_ERR_INVALID_PARAM;
    }

    entry = &cache->header->entries[entry_id];

    /* 加锁 */
    pthread_mutex_lock(&cache->header->mutex);

    if (entry->valid) {
        entry->status = OSAL_CACHE_STATUS_STALE;
    }

    /* 解锁 */
    pthread_mutex_unlock(&cache->header->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 批量标记缓存条目为STALE
 */
int32_t OSAL_CacheInvalidateBatch(osal_id_t cache_id,
                                  const uint32_t *entry_ids,
                                  uint32_t count)
{
    uint32_t i;

    if (entry_ids == NULL || count == 0) {
        return OSAL_ERR_INVALID_PARAM;
    }

    for (i = 0; i < count; i++) {
        OSAL_CacheInvalidate(cache_id, entry_ids[i]);
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 获取缓存统计信息
 */
int32_t OSAL_CacheGetStats(osal_id_t cache_id,
                           uint32_t *total_count,
                           uint32_t *valid_count,
                           uint32_t *fresh_count,
                           uint32_t *stale_count)
{
    cache_descriptor_t *cache;
    uint64_t now;
    uint32_t i;

    if (cache_id >= MAX_CACHES) {
        return OSAL_ERR_INVALID_PARAM;
    }

    cache = &g_cache_table[cache_id];
    if (!cache->in_use) {
        return OSAL_ERR_INVALID_ID;
    }

    /* 加锁 */
    pthread_mutex_lock(&cache->header->mutex);

    *total_count = cache->header->max_entries;
    *valid_count = 0;
    *fresh_count = 0;
    *stale_count = 0;

    now = get_monotonic_us();

    for (i = 0; i < cache->header->max_entries; i++) {
        osal_cache_entry_t *entry = &cache->header->entries[i];
        uint32_t age_ms;

        if (!entry->valid) {
            continue;
        }

        (*valid_count)++;

        /* 计算当前新鲜度 */
        age_ms = (uint32_t)((now - entry->timestamp_us) / 1000);

        if (entry->status == OSAL_CACHE_STATUS_INVALID) {
            /* 已标记为无效 */
        } else if (age_ms < entry->data_validity_ms) {
            (*fresh_count)++;
        } else {
            (*stale_count)++;
        }
    }

    /* 解锁 */
    pthread_mutex_unlock(&cache->header->mutex);

    return OSAL_SUCCESS;
}
