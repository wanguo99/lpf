/**
 * @file osal_shm_cache_internal.h
 * @brief OSAL共享内存缓存服务
 *
 * 提供通用的共享内存缓存机制，用于进程间数据共享
 * 支持快速读取（<50μs）和后台更新
 *
 * 使用场景：
 * - 遥测数据缓存（Telemetry进程写入，Telecommand进程读取）
 * - 配置数据共享
 * - 状态快照共享
 */

#ifndef OSAL_SHM_CACHE_INTERNAL_H
#define OSAL_SHM_CACHE_INTERNAL_H

/* 缓存条目最大数据大小 */
#define OSAL_SHM_CACHE_MAX_DATA_SIZE  256U

/**
 * @brief 缓存条目状态
 */
typedef enum {
    OSAL_CACHE_STATUS_INVALID = 0,  /* 无效（从未更新） */
    OSAL_CACHE_STATUS_FRESH,        /* 新鲜（在有效期内） */
    OSAL_CACHE_STATUS_STALE         /* 过期（超过有效期但可用） */
} osal_cache_status_t;

/**
 * @brief 缓存条目
 */
typedef struct {
    uint32_t entry_id;              /* 条目ID */
    uint8_t data[OSAL_SHM_CACHE_MAX_DATA_SIZE];  /* 数据 */
    uint32_t data_len;              /* 数据长度 */
    uint64_t timestamp_us;          /* 时间戳（微秒） */
    uint32_t data_validity_ms;      /* 数据有效期（毫秒） */
    osal_cache_status_t status;     /* 状态 */
    bool valid;                     /* 是否曾经更新过 */
    uint32_t update_count;          /* 更新次数 */
    uint32_t read_count;            /* 读取次数 */
    uint32_t checksum;              /* CRC32校验和 */
} osal_cache_entry_t;

/**
 * @brief 缓存读取结果
 */
typedef struct {
    uint32_t entry_id;              /* 条目ID */
    uint8_t data[OSAL_SHM_CACHE_MAX_DATA_SIZE];  /* 数据 */
    uint32_t data_len;              /* 数据长度 */
    uint64_t timestamp_us;          /* 时间戳（微秒） */
    uint32_t age_ms;                /* 数据年龄（毫秒） */
    osal_cache_status_t status;     /* 状态 */
    uint32_t checksum;              /* CRC32校验和 */
} osal_cache_result_t;

/**
 * @brief 创建共享内存缓存
 *
 * @param[in] name 缓存名称（用于共享内存标识）
 * @param[in] max_entries 最大条目数
 * @param[out] cache_id 输出缓存ID
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheCreate(const char *name, uint32_t max_entries, osal_id_t *cache_id);

/**
 * @brief 打开已存在的共享内存缓存
 *
 * @param[in] name 缓存名称
 * @param[out] cache_id 输出缓存ID
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheOpen(const char *name, osal_id_t *cache_id);

/**
 * @brief 删除共享内存缓存
 *
 * @param[in] cache_id 缓存ID
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheDelete(osal_id_t cache_id);

/**
 * @brief 写入缓存条目（生产者调用）
 *
 * @param[in] cache_id 缓存ID
 * @param[in] entry_id 条目ID
 * @param[in] data 数据
 * @param[in] data_len 数据长度
 * @param[in] data_validity_ms 数据有效期（毫秒）
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheWrite(osal_id_t cache_id, uint32_t entry_id,
                        const uint8_t *data, uint32_t data_len,
                        uint32_t data_validity_ms);

/**
 * @brief 读取缓存条目（消费者调用，快速读取<50μs）
 *
 * @param[in] cache_id 缓存ID
 * @param[in] entry_id 条目ID
 * @param[out] result 输出结果
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheRead(osal_id_t cache_id, uint32_t entry_id,
                       osal_cache_result_t *result);

/**
 * @brief 标记缓存条目为STALE（失效但可用）
 *
 * @param[in] cache_id 缓存ID
 * @param[in] entry_id 条目ID
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheInvalidate(osal_id_t cache_id, uint32_t entry_id);

/**
 * @brief 批量标记缓存条目为STALE
 *
 * @param[in] cache_id 缓存ID
 * @param[in] entry_ids 条目ID数组
 * @param[in] count 数组长度
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheInvalidateBatch(osal_id_t cache_id,
                                  const uint32_t *entry_ids,
                                  uint32_t count);

/**
 * @brief 获取缓存统计信息
 *
 * @param[in] cache_id 缓存ID
 * @param[out] total_count 总条目数
 * @param[out] valid_count 有效条目数
 * @param[out] fresh_count 新鲜条目数
 * @param[out] stale_count 过期条目数
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_CacheGetStats(osal_id_t cache_id,
                           uint32_t *total_count,
                           uint32_t *valid_count,
                           uint32_t *fresh_count,
                           uint32_t *stale_count);

#endif /* OSAL_SHM_CACHE_INTERNAL_H */
