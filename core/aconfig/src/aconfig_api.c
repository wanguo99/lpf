/**
 * @file aconfig_api.c
 * @brief ACONFIG 层 API 实现 - 通用配置管理框架
 * @note 提供通用的配置注册和查询功能，不包含业务特定逻辑
 */

#include "osal.h"
#include "aconfig.h"

/* 全局配置表 */
static const aconfig_config_table_t *g_aconfig_table = NULL;

/* 读写锁保护全局配置表（读多写少场景） */
static osal_rwlock_t g_aconfig_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * @brief 初始化 ACONFIG 层
 */
int32_t ACONFIG_Init(void)
{
	g_aconfig_table = NULL;
	LOG_INFO("ACONFIG", "Initialized (generic version)");
	return OSAL_SUCCESS;
}

/**
 * @brief 清理 ACONFIG 层
 */
void ACONFIG_Cleanup(void)
{
	int32_t ret;

	ret = OSAL_pthread_rwlock_wrlock(&g_aconfig_rwlock);
	if (OSAL_SUCCESS == ret) {
		g_aconfig_table = NULL;
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
	}

	LOG_INFO("ACONFIG", "Cleaned up");
}

/**
 * @brief 注册配置表
 */
int32_t ACONFIG_RegisterTable(const aconfig_config_table_t *table)
{
	int32_t ret;

	if (NULL == table) {
		LOG_ERROR("ACONFIG", "Invalid table pointer");
		return OSAL_ERR_INVALID_POINTER;
	}

	if (NULL == table->name) {
		LOG_ERROR("ACONFIG", "Table name is NULL");
		return OSAL_ERR_INVALID_POINTER;
	}

	/* 获取写锁（独占访问） */
	ret = OSAL_pthread_rwlock_wrlock(&g_aconfig_rwlock);
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("ACONFIG", "Failed to acquire write lock: %d", ret);
		return ret;
	}

	if (NULL != g_aconfig_table) {
		LOG_WARN("ACONFIG", "Table already registered, overwriting");
	}

	g_aconfig_table = table;

	LOG_INFO("ACONFIG", "Registered table '%s'", table->name);

	/* 释放写锁 */
	OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

	return OSAL_SUCCESS;
}

/**
 * @brief 注销配置表
 */
int32_t ACONFIG_UnregisterTable(void)
{
	int32_t ret;

	ret = OSAL_pthread_rwlock_wrlock(&g_aconfig_rwlock);
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("ACONFIG", "Failed to acquire write lock: %d", ret);
		return ret;
	}

	if (NULL == g_aconfig_table) {
		LOG_WARN("ACONFIG", "No table registered");
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	LOG_INFO("ACONFIG", "Unregistered table '%s'", g_aconfig_table->name);
	g_aconfig_table = NULL;

	OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

	return OSAL_SUCCESS;
}

/**
 * @brief 获取当前配置表
 */
const aconfig_config_table_t* ACONFIG_GetTable(void)
{
	const aconfig_config_table_t *table = NULL;

	if (OSAL_SUCCESS == OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
		table = g_aconfig_table;
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
	}

	return table;
}

/**
 * @brief 查询功能配置（通用接口）
 * @note 具体查找逻辑由产品层的 function_map 实现
 */
const void* ACONFIG_GetFunctionConfig(uint32_t function_id)
{
	(void)function_id;

	/* 获取读锁 */
	if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
		return NULL;
	}

	/* NULL 检查 */
	if (NULL == g_aconfig_table) {
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
		return NULL;
	}

	/* 注意：具体的查找逻辑应由产品层实现
	 * 这里返回 function_map，由产品层自行查询
	 */
	OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

	LOG_WARN("ACONFIG", "GetFunctionConfig: Use product-specific API instead");
	return NULL;
}

/**
 * @brief 检查功能是否启用
 * @note 具体实现由产品层提供
 */
bool ACONFIG_IsFunctionEnabled(uint32_t function_id)
{
	(void)function_id;

	LOG_WARN("ACONFIG", "IsFunctionEnabled: Use product-specific API instead");
	return false;
}

/**
 * @brief 获取配置统计信息
 */
int32_t ACONFIG_GetStatistics(aconfig_statistics_t *stats)
{
	if (NULL == stats) {
		LOG_ERROR("ACONFIG", "Invalid stats pointer");
		return OSAL_ERR_INVALID_POINTER;
	}

	OSAL_memset(stats, 0, sizeof(aconfig_statistics_t));

	/* 获取读锁 */
	if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
		return OSAL_ERR_GENERIC;
	}

	if (NULL == g_aconfig_table) {
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
		return OSAL_SUCCESS;
	}

	/* 通用统计信息（产品层可扩展） */
	LOG_INFO("ACONFIG", "Statistics gathering needs product-specific implementation");

	/* 释放读锁 */
	OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);

	return OSAL_SUCCESS;
}

/**
 * @brief 打印配置信息
 */
void ACONFIG_PrintConfig(void)
{
	/* 获取读锁 */
	if (OSAL_SUCCESS != OSAL_pthread_rwlock_rdlock(&g_aconfig_rwlock)) {
		LOG_ERROR("ACONFIG", "Failed to acquire read lock");
		return;
	}

	if (NULL == g_aconfig_table) {
		LOG_INFO("ACONFIG", "No table registered");
		OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
		return;
	}

	LOG_INFO("ACONFIG", "Configuration: %s", g_aconfig_table->name);
	LOG_INFO("ACONFIG", "  Function map: %p", (void *)g_aconfig_table->function_map);
	LOG_INFO("ACONFIG", "  User data: %p", g_aconfig_table->user_data);

	/* 释放读锁 */
	OSAL_pthread_rwlock_unlock(&g_aconfig_rwlock);
}
