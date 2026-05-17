/**
 * @file acl_api.h
 * @brief ACL层API接口
 * @note ACL层：业务配置层，提供业务功能到硬件设备的映射查询
 */

#ifndef ACL_API_H
#define ACL_API_H

#include "osal_types.h"
#include "acl_config.h"
#include "pmc_acl_types.h"

/**
 * @brief ACL配置统计信息
 */
typedef struct {
    uint32_t tc_enabled_count;
    uint32_t tc_disabled_count;
    uint32_t tm_enabled_count;
    uint32_t tm_disabled_count;
    uint32_t tm_cached_count;
    uint32_t tm_realtime_count;
    uint32_t invalidation_map_count;
} acl_statistics_t;

/**
 * @brief 初始化ACL层
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 */
int32_t ACL_Init(void);

/**
 * @brief 查询遥控配置（O(1)）
 * @param function 遥控功能枚举
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tc_config_t* ACL_GetTcConfig(pmc_tc_function_t function);

/**
 * @brief 查询遥测配置（O(1)）
 * @param function 遥测功能枚举
 * @return 配置指针（只读），失败返回NULL
 */
const acl_tm_config_t* ACL_GetTmConfig(pmc_tm_function_t function);

/**
 * @brief 检查遥控功能是否使能
 * @param function 遥控功能枚举
 * @return true使能，false禁用
 */
bool ACL_IsTcEnabled(pmc_tc_function_t function);

/**
 * @brief 检查遥测功能是否使能
 * @param function 遥测功能枚举
 * @return true使能，false禁用
 */
bool ACL_IsTmEnabled(pmc_tm_function_t function);

/**
 * @brief 遥控命令执行后，失效相关遥测缓存
 * @param tc_function 遥控功能枚举
 */
void ACL_InvalidateAffectedTelemetry(pmc_tc_function_t tc_function);

/**
 * @brief 验证ACL配置
 * @return OSAL_SUCCESS成功，OSAL_ERR_*失败
 */
int32_t ACL_ValidateConfig(void);

/**
 * @brief 获取ACL配置统计信息
 * @param stats 输出统计信息
 */
void ACL_GetStatistics(acl_statistics_t *stats);

/**
 * @brief 打印ACL配置统计信息
 */
void ACL_PrintStatistics(void);

/**
 * @brief 打印ACL配置详情（调试用）
 */
void ACL_DumpConfig(void);

#endif /* ACL_API_H */
