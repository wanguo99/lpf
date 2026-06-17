/************************************************************************
 * MCU外设驱动实现
 *
 * 职责：
 * - 提供对外API（初始化、命令收发）
 * - 直接使用 PRL 层进行协议编解码
 * - 选择通信层（CAN/串口）
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdl.h"
#include "prl.h"
#include "pdl_mcu_internal.h"

/*===========================================================================
 * MCU上下文
 *===========================================================================*/

typedef struct {
	const pconfig_mcu_config_t *config;
	const pdl_mcu_ops_t *ops;
	void *transport_handle;
	osal_mutex_t mutex;
} pdl_mcu_context_t;

static pdl_mcu_context_t *g_mcu_contexts[16] = {NULL};
static osal_mutex_t g_registry_mutex;
static bool g_registry_initialized = false;

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 发送 PRL 报文
 */
static int32_t mcu_send_packet(pdl_mcu_context_t *ctx,
                               uint8_t msg_type,
                               const void *payload,
                               uint16_t payload_len,
                               uint8_t *response,
                               uint32_t resp_size,
                               uint32_t *actual_size)
{
	uint8_t tx_buffer[PRL_MAX_PACKET_SIZE];
	uint8_t rx_buffer[PRL_MAX_PACKET_SIZE];
	int32_t tx_len;
	int32_t ret;
	uint32_t rx_len;

	/* 编码 PRL 报文 */
	tx_len = prl_device_encode(PRL_DEV_TYPE_MCU, msg_type,
	                           payload, payload_len,
	                           tx_buffer, sizeof(tx_buffer), 0);
	if (tx_len < 0) {
		return tx_len;
	}

	/* 发送并接收响应 */
	ret = ctx->ops->send_packet(ctx->transport_handle,
	                            tx_buffer, tx_len,
	                            rx_buffer, sizeof(rx_buffer),
	                            &rx_len,
	                            ctx->config->cmd_timeout_ms);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 解码响应报文 */
	ret = prl_device_decode(rx_buffer, rx_len,
	                       NULL, NULL, NULL,
	                       response, resp_size, actual_size);
	return ret;
}

/*===========================================================================
 * 对外API实现
 *===========================================================================*/

/**
 * @brief 初始化 MCU 驱动
 */
int32_t PDL_MCU_init(uint32_t mcu_index, pdl_mcu_handle_t *handle)
{
	const pconfig_platform_config_t *platform;
	const pconfig_mcu_entry_t *entry;
	pdl_mcu_context_t *ctx;
	const pdl_mcu_ops_t *ops;
	int32_t ret;

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 初始化全局注册表互斥锁 */
	if (!g_registry_initialized) {
		if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&g_registry_mutex, NULL)) {
			return OSAL_ERR_GENERIC;
		}
		g_registry_initialized = true;
	}

	/* 获取配置 */
	platform = PCONFIG_GetBoard();
	if (!platform) {
		return OSAL_ERR_GENERIC;
	}

	entry = PCONFIG_HW_GetMCU(platform, mcu_index);
	if (!entry || !entry->enabled) {
		return OSAL_ERR_GENERIC;
	}

	/* 选择通信层 */
	switch (entry->config.interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
		ops = &mcu_can_ops;
		break;
	case PCONFIG_MCU_INTERFACE_SERIAL:
		ops = &mcu_serial_ops;
		break;
	default:
		return OSAL_ERR_GENERIC;
	}

	/* 分配上下文 */
	ctx = (pdl_mcu_context_t *)OSAL_malloc(sizeof(pdl_mcu_context_t));
	if (!ctx) {
		return OSAL_ERR_NO_MEMORY;
	}

	OSAL_memset(ctx, 0, sizeof(pdl_mcu_context_t));
	ctx->config = &entry->config;
	ctx->ops = ops;

	/* 初始化通信层 */
	ret = ops->init(&entry->config, &ctx->transport_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_free(ctx);
		return ret;
	}

	/* 创建互斥锁 */
	if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL)) {
		ops->deinit(ctx->transport_handle);
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 注册上下文 */
	OSAL_pthread_mutex_lock(&g_registry_mutex);
	if (mcu_index < sizeof(g_mcu_contexts) / sizeof(g_mcu_contexts[0])) {
		g_mcu_contexts[mcu_index] = ctx;
	}
	OSAL_pthread_mutex_unlock(&g_registry_mutex);

	*handle = ctx;
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化 MCU 驱动
 */
int32_t PDL_MCU_deinit(pdl_mcu_handle_t handle)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint32_t i;

	if (!ctx) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 从注册表移除 */
	OSAL_pthread_mutex_lock(&g_registry_mutex);
	for (i = 0; i < sizeof(g_mcu_contexts) / sizeof(g_mcu_contexts[0]); i++) {
		if (g_mcu_contexts[i] == ctx) {
			g_mcu_contexts[i] = NULL;
			break;
		}
	}
	OSAL_pthread_mutex_unlock(&g_registry_mutex);

	/* 清理资源 */
	ctx->ops->deinit(ctx->transport_handle);
	OSAL_pthread_mutex_destroy(&ctx->mutex);
	OSAL_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 获取 MCU 版本信息
 */
int32_t PDL_MCU_get_version(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t response[64];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx || !version) {
		return OSAL_ERR_INVALID_PARAM;
	}

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 GET_VERSION 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_GET_VERSION,
	                     NULL, 0,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 解析版本信息 */
	if (resp_len >= 3) {
		version->major = response[0];
		version->minor = response[1];
		version->patch = response[2];
	} else {
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 获取 MCU 状态
 */
int32_t PDL_MCU_get_status(pdl_mcu_handle_t handle, pdl_mcu_status_t *status)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t response[64];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx || !status) {
		return OSAL_ERR_INVALID_PARAM;
	}

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 GET_STATUS 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_GET_STATUS,
	                     NULL, 0,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 解析状态信息 */
	if (resp_len >= 1) {
		status->state = response[0];
		status->error_code = (resp_len >= 5) ?
		                     ((uint32_t)response[1] << 24 |
		                      (uint32_t)response[2] << 16 |
		                      (uint32_t)response[3] << 8 |
		                      response[4]) : 0;
	} else {
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief MCU 复位
 */
int32_t PDL_MCU_reset(pdl_mcu_handle_t handle)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t response[64];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx) {
		return OSAL_ERR_INVALID_PARAM;
	}

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 RESET 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_RESET,
	                     NULL, 0,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	return ret;
}

/**
 * @brief 读取 MCU 数据
 */
int32_t PDL_MCU_read_data(pdl_mcu_handle_t handle,
                         uint32_t addr,
                         uint8_t *data,
                         uint32_t size)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t request[8];
	uint8_t response[256];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx || !data || size == 0) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = (addr >> 24) & 0xFF;
	request[1] = (addr >> 16) & 0xFF;
	request[2] = (addr >> 8) & 0xFF;
	request[3] = addr & 0xFF;
	request[4] = (size >> 24) & 0xFF;
	request[5] = (size >> 16) & 0xFF;
	request[6] = (size >> 8) & 0xFF;
	request[7] = size & 0xFF;

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 READ_DATA 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_READ_DATA,
	                     request, 8,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 复制数据 */
	if (resp_len > 0) {
		uint32_t copy_len = (resp_len < size) ? resp_len : size;
		OSAL_memcpy(data, response, copy_len);
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 写入 MCU 数据
 */
int32_t PDL_MCU_write_data(pdl_mcu_handle_t handle,
                          uint32_t addr,
                          const uint8_t *data,
                          uint32_t size)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t request[256];
	uint8_t response[64];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx || !data || size == 0 || size > 248) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = (addr >> 24) & 0xFF;
	request[1] = (addr >> 16) & 0xFF;
	request[2] = (addr >> 8) & 0xFF;
	request[3] = addr & 0xFF;
	request[4] = (size >> 24) & 0xFF;
	request[5] = (size >> 16) & 0xFF;
	request[6] = (size >> 8) & 0xFF;
	request[7] = size & 0xFF;
	OSAL_memcpy(&request[8], data, size);

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 WRITE_DATA 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_WRITE_DATA,
	                     request, 8 + size,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	return ret;
}

/**
 * @brief 执行 MCU 命令
 */
int32_t PDL_MCU_send_command(pdl_mcu_handle_t handle,
                               uint8_t cmd_id,
                               const uint8_t *params,
                               uint32_t param_len,
                               uint8_t *result,
                               uint32_t result_size,
                               uint32_t *actual_len)
{
	pdl_mcu_context_t *ctx = (pdl_mcu_context_t *)handle;
	uint8_t request[256];
	uint8_t response[256];
	uint32_t resp_len;
	int32_t ret;

	if (!ctx || param_len > 255) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = cmd_id;
	if (params && param_len > 0) {
		OSAL_memcpy(&request[1], params, param_len);
	}

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 发送 EXECUTE_CMD 命令 */
	ret = mcu_send_packet(ctx, PRL_MCU_MSG_EXECUTE_CMD,
	                     request, 1 + param_len,
	                     response, sizeof(response), &resp_len);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 复制结果 */
	if (result && result_size > 0 && resp_len > 0) {
		uint32_t copy_len = (resp_len < result_size) ? resp_len : result_size;
		OSAL_memcpy(result, response, copy_len);
		if (actual_len) {
			*actual_len = copy_len;
		}
	}

	return OSAL_SUCCESS;
}
