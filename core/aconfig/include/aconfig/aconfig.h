/**
 * @file aconfig.h
 * @brief ACONFIG 对外 API - 通用配置管理框架
 * @note 提供通用的配置注册和查询接口，不包含业务特定定义
 *       业务逻辑由产品层实现（通过 aconfig_function_map_t）
 */

#ifndef ACONFIG_H
#define ACONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "aconfig_types.h"

/* 注意：源文件应按依赖顺序包含头文件
 * 示例：
 *   #include "osal.h"
 *   #include "aconfig.h"
 *   #include "ccm_config.h"  // 产品特定配置
 */

/**
 * @brief 初始化 ACONFIG 层
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_Init(void);

/**
 * @brief 清理 ACONFIG 层
 */
void ACONFIG_Cleanup(void);

/**
 * @brief 注册配置表
 * @param table 配置表指针
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_RegisterTable(const aconfig_config_table_t *table);

/**
 * @brief 注销配置表
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_UnregisterTable(void);

/**
 * @brief 获取当前配置表
 * @return 配置表指针，NULL 表示未注册
 */
const aconfig_config_table_t* ACONFIG_GetTable(void);

/**
 * @brief 查询功能配置（通用接口）
 * @param function_id 功能 ID
 * @return 配置数据指针（不透明），NULL 表示未找到
 * @note 返回的指针类型由产品层定义
 */
const void* ACONFIG_GetFunctionConfig(uint32_t function_id);

/**
 * @brief 检查功能是否启用
 * @param function_id 功能 ID
 * @return true 启用，false 禁用
 */
bool ACONFIG_IsFunctionEnabled(uint32_t function_id);

/**
 * @brief 获取配置统计信息
 * @param stats 输出：统计信息
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_GetStatistics(aconfig_statistics_t *stats);

/**
 * @brief 打印配置信息（调试用）
 */
void ACONFIG_PrintConfig(void);

#endif /* ACONFIG_H */
