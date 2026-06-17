/************************************************************************
 * PMC CAN 通信实现
 *
 * 职责：
 * - 封装 PMC CAN 协议（帧格式、消息类型）
 * - CAN 帧的收发
 * - 协议解析和封装
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdl.h"
#include "pdl_pmc_internal.h"

/*
 * CAN 通信上下文
 */
typedef struct
{
	hal_can_handle_t can_handle;
	uint32_t tx_id;
	uint32_t rx_id;
	osal_mutex_t mutex;
	bool connected;
} pmc_can_context_t;

/**
 * @brief 初始化 PMC CAN 通信（适配 ops 接口）
 */
static int32_t pmc_can_init_ops(const void *config, void **handle)
{
	const pconfig_pmc_can_config_t *can_cfg = (const pconfig_pmc_can_config_t *)config;
	pmc_can_context_t *ctx;
	hal_can_config_t can_config;

	if (NULL == config || NULL == handle)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (pmc_can_context_t *)OSAL_malloc(OSAL_sizeof(pmc_can_context_t));
	if (NULL == ctx)
	{
		return OSAL_ERR_NO_MEMORY;
	}

	OSAL_memset(ctx, 0, OSAL_sizeof(pmc_can_context_t));
	ctx->tx_id = can_cfg->tx_id;
	ctx->rx_id = can_cfg->rx_id;

	/* 创建互斥锁 */
	if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
	{
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 打开 CAN 设备 */
	can_config.interface = can_cfg->device;
	can_config.baudrate = can_cfg->bitrate;
	can_config.rx_timeout = can_cfg->rx_timeout_ms;
	can_config.tx_timeout = can_cfg->tx_timeout_ms;

	if (OSAL_SUCCESS != HAL_CAN_init(&can_config, &ctx->can_handle))
	{
		OSAL_pthread_mutex_destroy(&ctx->mutex);
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	ctx->connected = true;
	*handle = ctx;

	LOG_INFO("PDL_PMC", "CAN initialized: %s", can_cfg->device);
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化 PMC CAN 通信
 */
static int32_t pmc_can_deinit(void *handle)
{
	pmc_can_context_t *ctx;

	if (NULL == handle)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (pmc_can_context_t *)handle;

	HAL_CAN_deinit(ctx->can_handle);
	OSAL_pthread_mutex_destroy(&ctx->mutex);
	OSAL_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 发送 PMC CAN 消息
 */
static int32_t pmc_can_send(void *handle, const pmc_msg_t *msg, uint32_t timeout_ms)
{
	pmc_can_context_t *ctx;
	hal_can_frame_t frame;
	uint32_t offset;
	uint32_t remaining;
	int32_t ret;

	if (NULL == handle || NULL == msg)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (pmc_can_context_t *)handle;

	OSAL_pthread_mutex_lock(&ctx->mutex);

	if (!ctx->connected)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return OSAL_ERR_GENERIC;
	}

	/*
	 * CAN 分片传输（每帧最多 8 字节）
	 * 第一帧：[msg_type][seq_num(4bytes)][len_h][len_l][data(1byte)]
	 * 后续帧：[data(8bytes)]
	 */

	/* 第一帧 */
	frame.can_id = ctx->tx_id;
	frame.dlc = 8;
	frame.data[0] = msg->msg_type;
	frame.data[1] = (uint8_t)(msg->seq_num >> 24);
	frame.data[2] = (uint8_t)(msg->seq_num >> 16);
	frame.data[3] = (uint8_t)(msg->seq_num >> 8);
	frame.data[4] = (uint8_t)(msg->seq_num & 0xFF);
	frame.data[5] = (uint8_t)(msg->payload_len >> 8);
	frame.data[6] = (uint8_t)(msg->payload_len & 0xFF);
	frame.data[7] = (msg->payload_len > 0) ? msg->payload[0] : 0;

	ret = HAL_CAN_send(ctx->can_handle, &frame);
	if (OSAL_SUCCESS != ret)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return ret;
	}

	/* 发送剩余数据 */
	offset = 1;
	remaining = (msg->payload_len > 1) ? (msg->payload_len - 1) : 0;

	while (remaining > 0)
	{
		uint32_t chunk_size = (remaining > 8) ? 8 : remaining;

		frame.dlc = chunk_size;
		OSAL_memcpy(frame.data, &msg->payload[offset], chunk_size);

		ret = HAL_CAN_send(ctx->can_handle, &frame);
		if (OSAL_SUCCESS != ret)
		{
			OSAL_pthread_mutex_unlock(&ctx->mutex);
			return ret;
		}

		offset += chunk_size;
		remaining -= chunk_size;
	}

	OSAL_pthread_mutex_unlock(&ctx->mutex);
	return OSAL_SUCCESS;
}

/**
 * @brief 接收 PMC CAN 消息
 */
static int32_t pmc_can_recv(void *handle, pmc_msg_t *msg, uint32_t timeout_ms)
{
	pmc_can_context_t *ctx;
	hal_can_frame_t frame;
	uint32_t offset;
	uint32_t remaining;
	int32_t ret;

	if (NULL == handle || NULL == msg)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (pmc_can_context_t *)handle;

	OSAL_pthread_mutex_lock(&ctx->mutex);

	/* 接收第一帧 */
	ret = HAL_CAN_recv(ctx->can_handle, &frame, timeout_ms);
	if (OSAL_SUCCESS != ret)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return ret;
	}

	/* 检查 CAN ID */
	if (frame.can_id != ctx->rx_id || frame.dlc != 8)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return OSAL_ERR_GENERIC;
	}

	/* 解析第一帧 */
	msg->msg_type = frame.data[0];
	msg->seq_num = (frame.data[1] << 24) | (frame.data[2] << 16) |
	               (frame.data[3] << 8) | frame.data[4];
	msg->payload_len = (frame.data[5] << 8) | frame.data[6];

	if (msg->payload_len > PMC_MAX_MSG_SIZE)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return OSAL_ERR_GENERIC;
	}

	if (msg->payload_len > 0)
	{
		msg->payload[0] = frame.data[7];
	}

	/* 接收剩余数据 */
	offset = 1;
	remaining = (msg->payload_len > 1) ? (msg->payload_len - 1) : 0;

	while (remaining > 0)
	{
		ret = HAL_CAN_recv(ctx->can_handle, &frame, timeout_ms);
		if (OSAL_SUCCESS != ret)
		{
			OSAL_pthread_mutex_unlock(&ctx->mutex);
			return ret;
		}

		uint32_t chunk_size = (remaining > frame.dlc) ? frame.dlc : remaining;
		OSAL_memcpy(&msg->payload[offset], frame.data, chunk_size);

		offset += chunk_size;
		remaining -= chunk_size;
	}

	OSAL_pthread_mutex_unlock(&ctx->mutex);
	return OSAL_SUCCESS;
}

/**
 * @brief 检查 CAN 连接状态
 */
static int32_t pmc_can_is_connected(void *handle)
{
	pmc_can_context_t *ctx;

	if (NULL == handle)
	{
		return 0;
	}

	ctx = (pmc_can_context_t *)handle;
	return ctx->connected ? 1 : 0;
}

/*
 * CAN 接口的 ops 结构定义（导出供 pdl_pmc.c 使用）
 */
const pdl_pmc_ops_t pmc_can_ops = {
	.init = pmc_can_init_ops,
	.deinit = pmc_can_deinit,
	.send = pmc_can_send,
	.recv = pmc_can_recv,
	.is_connected = pmc_can_is_connected,
};
