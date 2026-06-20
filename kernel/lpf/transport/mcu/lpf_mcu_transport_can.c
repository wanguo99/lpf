/************************************************************************
 * MCU CAN通信实现
 *
 * 职责：
 * - 封装CAN收发
 * - 直接转发 LPF protocol 协议报文（透传模式）
 * - 处理 CAN 分片传输（每帧最多 8 字节）
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "lpf/lpf_mcu_transport.h"

/*===========================================================================
 * CAN通信实现
 *===========================================================================*/

/**
 * @brief CAN通信上下文
 */
typedef struct {
	hal_can_handle_t can_handle;
	uint32_t tx_id;
	uint32_t rx_id;
	osal_mutex_t rx_mutex;
} lpf_mcu_can_context_t;

/**
 * @brief 初始化CAN通信
 */
static int32_t lpf_mcu_transport_can_open(const pconfig_mcu_config_t *mcu_cfg,
					  lpf_mcu_transport_handle_t *handle)
{
	lpf_mcu_can_context_t *ctx;
	hal_can_config_t can_config;

	if (!mcu_cfg || !handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_can_context_t *)osal_malloc(sizeof(lpf_mcu_can_context_t));
	if (!ctx) {
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(ctx, 0, sizeof(lpf_mcu_can_context_t));

	/* 配置CAN参数 */
	can_config.interface = mcu_cfg->hw.can.device;
	can_config.baudrate = mcu_cfg->hw.can.bitrate;
	can_config.rx_timeout = mcu_cfg->hw.can.rx_timeout;
	can_config.tx_timeout = mcu_cfg->hw.can.tx_timeout;

	if (OSAL_SUCCESS != hal_can_init(&can_config, &ctx->can_handle)) {
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	ctx->tx_id = mcu_cfg->hw.can.tx_id;
	ctx->rx_id = mcu_cfg->hw.can.rx_id;

	/* 创建接收互斥锁 */
	if (OSAL_SUCCESS != osal_mutex_init(&ctx->rx_mutex, NULL)) {
		hal_can_deinit(ctx->can_handle);
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	*handle = ctx;
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化CAN通信
 */
static int32_t lpf_mcu_transport_can_close(lpf_mcu_transport_handle_t handle)
{
	lpf_mcu_can_context_t *ctx;

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_can_context_t *)handle;

	hal_can_deinit(ctx->can_handle);
	osal_mutex_destroy(&ctx->rx_mutex);
	osal_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 发送 LPF protocol 报文并接收响应
 */
static int32_t lpf_mcu_transport_can_transfer(
	lpf_mcu_transport_handle_t handle, const uint8_t *packet,
	uint32_t packet_len, uint8_t *response, uint32_t resp_size,
	uint32_t *actual_size, uint32_t timeout_ms)
{
	lpf_mcu_can_context_t *ctx;
	hal_can_frame_t can_frame;
	hal_can_frame_t rx_frame;
	int32_t ret;
	uint64_t start_time_us;
	uint64_t elapsed_us;
	uint32_t remaining_timeout_ms;
	uint32_t sent_bytes = 0;
	uint32_t recv_bytes = 0;

	if (!handle || !packet) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_can_context_t *)handle;

	/* 记录起始时间 */
	start_time_us = osal_get_monotonic_time();

	/* 分片发送 LPF protocol 报文（CAN 每帧最多 8 字节） */
	while (sent_bytes < packet_len) {
		uint32_t chunk_size =
			(packet_len - sent_bytes > 8) ? 8 : (packet_len - sent_bytes);

		can_frame.can_id = ctx->tx_id;
		can_frame.dlc = chunk_size;
		osal_memcpy(can_frame.data, &packet[sent_bytes], chunk_size);

		ret = hal_can_send(ctx->can_handle, &can_frame);
		if (ret != OSAL_SUCCESS) {
			return ret;
		}

		sent_bytes += chunk_size;

		/* 检查超时 */
		elapsed_us = osal_get_monotonic_time() - start_time_us;
		if (elapsed_us / 1000 >= timeout_ms) {
			return OSAL_ERR_TIMEOUT;
		}
	}

	/* 计算剩余超时时间 */
	elapsed_us = osal_get_monotonic_time() - start_time_us;
	if (elapsed_us / 1000 >= timeout_ms) {
		return OSAL_ERR_TIMEOUT;
	}
	remaining_timeout_ms = timeout_ms - (uint32_t)(elapsed_us / 1000);

	/* 接收响应报文（可能多帧） */
	osal_mutex_lock(&ctx->rx_mutex);

	while (recv_bytes < resp_size) {
		ret = hal_can_recv(ctx->can_handle, &rx_frame, remaining_timeout_ms);
		if (ret != OSAL_SUCCESS) {
			osal_mutex_unlock(&ctx->rx_mutex);
			return ret;
		}

		/* 检查 CAN ID */
		if (rx_frame.can_id != ctx->rx_id) {
			osal_mutex_unlock(&ctx->rx_mutex);
			return OSAL_ERR_GENERIC;
		}

		/* 复制数据 */
		uint32_t copy_len = (resp_size - recv_bytes > rx_frame.dlc) ?
								rx_frame.dlc :
								(resp_size - recv_bytes);
		if (response) {
			osal_memcpy(&response[recv_bytes], rx_frame.data, copy_len);
		}
		recv_bytes += copy_len;

		/* 简单判断：如果收到的数据少于 8 字节，说明是最后一帧 */
		if (rx_frame.dlc < 8) {
			break;
		}

		/* 更新剩余超时 */
		elapsed_us = osal_get_monotonic_time() - start_time_us;
		if (elapsed_us / 1000 >= timeout_ms) {
			osal_mutex_unlock(&ctx->rx_mutex);
			return OSAL_ERR_TIMEOUT;
		}
		remaining_timeout_ms = timeout_ms - (uint32_t)(elapsed_us / 1000);
	}

	osal_mutex_unlock(&ctx->rx_mutex);

	if (actual_size) {
		*actual_size = recv_bytes;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief CAN接口的ops结构定义（导出供lpf_mcu.c使用）
 */
const lpf_mcu_transport_ops_t lpf_mcu_transport_can_ops = {
	.interface = PCONFIG_MCU_INTERFACE_CAN,
	.name = "can",
	.open = lpf_mcu_transport_can_open,
	.close = lpf_mcu_transport_can_close,
	.transfer = lpf_mcu_transport_can_transfer,
};
