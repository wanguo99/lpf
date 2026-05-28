/**
 * @file acl_api.h
 * @brief ACL层API接口（通用）
 * @note ACL框架核心：提供通用的配置注册和查询机制
 */

#ifndef ACL_API_H
#define ACL_API_H

#include "osal/osal_types.h"
#include "acl/acl_types.h"

/**
 * @brief ACL配置统计信息
 */
typedef struct {
    uint32_t tc_enabled_count;
    uint32_t tc_disabled_count;
    uint32_t tm_enabled_count;
    uint32_t tm_disabled_count;
    uint32_t invalidation_map_count;
} acl_statistics_t;

/**
 * @brief 初始化ACL层
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 */
int32_t ACL_Init(void);

/**
 * @brief 注册配置表
 * @param table 配置表指针
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 * @note 通常在项目初始化时调用，注册项目特定的配置
 */
int32_t ACL_RegisterTable(const acl_config_table_t *table);

/**
 * @brief 查询遥控配置（O(1)）
 * @param function_id 功能ID
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tc_config_t* ACL_GetTcConfig(uint32_t function_id);

/**
 * @brief 查询遥测配置（O(1)）
 * @param function_id 功能ID
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tm_config_t* ACL_GetTmConfig(uint32_t function_id);

/**
 * @brief 检查遥控功能是否使能
 * @param function_id 功能ID
 * @return true使能，false禁用
 */
bool ACL_IsTcEnabled(uint32_t function_id);

/**
 * @brief 检查遥测功能是否使能
 * @param function_id 功能ID
 * @return true使能，false禁用
 */
bool ACL_IsTmEnabled(uint32_t function_id);

/**
 * @brief 获取失效映射
 * @param source_tm_id 源遥测ID
 * @param affected_ids 输出：受影响的遥测ID数组（调用者分配）
 * @param max_count 数组最大容量
 * @param actual_count 输出：实际受影响的遥测数量
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 */
int32_t ACL_GetInvalidationMap(uint32_t source_tm_id,
                                uint32_t *affected_ids,
                                uint32_t max_count,
                                uint32_t *actual_count);

/**
 * @brief 获取配置统计信息
 * @param stats 输出：统计信息
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 */
int32_t ACL_GetStatistics(acl_statistics_t *stats);

/**
 * @brief 打印配置信息（调试用）
 */
void ACL_PrintConfig(void);

#endif /* ACL_API_H */
