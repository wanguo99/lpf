/**
 * @file acl_api.h
 * @brief ACL层API接口（纯配置层）
 *
 * ACL层职责：
 * - 提供业务功能到设备的映射配置（只读）
 * - 提供遥控/遥测配置查询接口
 * - 不包含运行时数据管理
 * - 不管理系统资源（共享内存、进程、线程）
 *
 * 运行时数据管理（如遥测缓存）已移至OSAL层的osal_shm_cache
 */

#ifndef ACL_API_H
#define ACL_API_H

#include "osal_types.h"
#include "acl_types.h"
#include "acl_tc.h"
#include "acl_tm.h"

/**
 * @brief 初始化ACL配置系统
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t ACL_Init(void);

/**
 * @brief 清理ACL配置系统
 */
void ACL_Deinit(void);

/**
 * @brief 获取遥控命令配置
 *
 * @param[in] tc_id 遥控命令ID
 * @return 配置指针，NULL表示未找到
 */
const acl_tc_config_t* ACL_GetTcConfig(acl_tc_function_t tc_id);

/**
 * @brief 获取遥测配置
 *
 * @param[in] tm_id 遥测ID
 * @return 配置指针，NULL表示未找到
 */
const acl_tm_config_t* ACL_GetTmConfig(acl_tm_function_t tm_id);

/**
 * @brief 获取遥控命令失效的遥测列表
 *
 * @param[in] tc_id 遥控命令ID
 * @param[out] tm_ids 输出遥测ID数组
 * @param[out] count 输出数组长度
 * @return OSAL_SUCCESS 成功
 */
int32_t ACL_GetInvalidatedTelemetry(acl_tc_function_t tc_id,
                                     const acl_tm_function_t **tm_ids,
                                     uint32_t *count);

/**
 * @brief 获取配置统计信息
 *
 * @param[out] tc_count 遥控命令配置数量
 * @param[out] tm_count 遥测配置数量
 * @return OSAL_SUCCESS 成功
 */
int32_t ACL_GetConfigStats(uint32_t *tc_count, uint32_t *tm_count);

#endif /* ACL_API_H */
