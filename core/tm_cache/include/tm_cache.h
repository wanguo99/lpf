/**
 * @file tm_cache.h
 * @brief 遥测缓存管理
 * @note 所有遥测都从缓存读取，确保2ms应答
 *       后台进程负责更新缓存
 */

#ifndef TM_CACHE_H
#define TM_CACHE_H

#include "osal_types.h"

/**
 * @brief 遥测数据新鲜度标记
 */
typedef enum {
    TM_CACHE_STATUS_INVALID = 0,   /* 无效：从未更新或已失效 */
    TM_CACHE_STATUS_FRESH = 1,     /* 新鲜：在有效期内 */
    TM_CACHE_STATUS_STALE = 2      /* 过期：超过有效期但仍可用 */
} tm_cache_status_t;

/**
 * @brief 遥测缓存条目
 */
typedef struct {
    uint32_t tm_id;                 /* 遥测ID */
    uint8_t data[256];              /* 遥测数据 */
    uint32_t data_len;              /* 数据长度 */
    uint64_t timestamp_us;          /* 采集时间戳（微秒） */
    uint32_t validity_ms;           /* 有效期（毫秒） */
    tm_cache_status_t freshness;    /* 新鲜度状态 */
    bool valid;                     /* 是否有效（是否曾经更新过） */
    uint32_t update_count;          /* 更新次数 */
    uint32_t read_count;            /* 读取次数 */
    uint32_t checksum;              /* CRC32校验和 */
} tm_cache_entry_t;

/**
 * @brief 遥测应答包
 */
typedef struct {
    uint32_t tm_id;                 /* 遥测ID */
    uint8_t data[256];              /* 遥测数据 */
    uint32_t data_len;              /* 数据长度 */
    uint64_t timestamp_us;          /* 采集时间戳（微秒） */
    uint32_t age_ms;                /* 数据年龄（当前时间 - 采集时间） */
    tm_cache_status_t freshness;    /* 新鲜度：INVALID/FRESH/STALE */
    uint32_t checksum;              /* CRC32校验和 */
} tm_cache_response_t;

/**
 * @brief 初始化遥测缓存
 * @param max_entries 最大缓存条目数
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_Init(uint32_t max_entries);

/**
 * @brief 清理遥测缓存
 */
void TM_Cache_Deinit(void);

/**
 * @brief 写入遥测数据到缓存（Telemetry进程调用）
 * @param tm_id 遥测ID
 * @param data 遥测数据
 * @param data_len 数据长度
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_Write(uint32_t tm_id, const uint8_t *data, uint32_t data_len);

/**
 * @brief 读取遥测数据（Telecommand进程调用，<50μs）
 * @param tm_id 遥测ID
 * @param response 输出应答包
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_Read(uint32_t tm_id, tm_cache_response_t *response);

/**
 * @brief 标记遥测为STALE（遥控命令执行后调用）
 * @param tm_id 遥测ID
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_Invalidate(uint32_t tm_id);

/**
 * @brief 批量标记遥测为STALE
 * @param tm_ids 遥测ID数组
 * @param count 数组长度
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_InvalidateBatch(const uint32_t *tm_ids, uint32_t count);

/**
 * @brief 获取缓存统计信息
 * @param total_count 总条目数
 * @param valid_count 有效条目数
 * @param fresh_count 新鲜条目数
 * @param stale_count 过期条目数
 * @return OSAL_SUCCESS 成功
 */
int32_t TM_Cache_GetStats(uint32_t *total_count, uint32_t *valid_count,
                           uint32_t *fresh_count, uint32_t *stale_count);

#endif /* TM_CACHE_H */
