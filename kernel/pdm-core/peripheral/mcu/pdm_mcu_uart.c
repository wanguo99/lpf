/************************************************************************
 * MCU串口通信实现
 *
 * 职责：
 * - 封装串口收发
 * - 直接转发 LPF protocol 协议报文（透传模式）
 * - 超时控制
 ************************************************************************/

#include "osal.h"
#include <linux/math64.h>

#include "lpf/config/lpf_config.h"
#include "lpf/hw/lpf_hw_uart.h"
#include "lpf_mcu_transport.h"

/*===========================================================================
 * LPF_CONFIG to LPF HW type conversion.
 *===========================================================================*/

/**
 * @brief 转换 LPF_CONFIG 校验位类型到 LPF HW 校验位类型
 */
static uint8_t _lpf_mcu_to_lpf_hw_parity(lpf_config_mcu_parity_t parity)
{
	switch (parity) {
	case LPF_CONFIG_MCU_PARITY_NONE:
		return LPF_SERIAL_PARITY_NONE;
	case LPF_CONFIG_MCU_PARITY_ODD:
		return LPF_SERIAL_PARITY_ODD;
	case LPF_CONFIG_MCU_PARITY_EVEN:
		return LPF_SERIAL_PARITY_EVEN;
	default:
		return LPF_SERIAL_PARITY_NONE;
	}
}

/**
 * @brief 转换 LPF_CONFIG 流控类型到 LPF HW 流控类型
 */
static uint8_t _lpf_mcu_to_lpf_hw_flow_control(lpf_config_mcu_flow_control_t flow)
{
	switch (flow) {
	case LPF_CONFIG_MCU_FLOW_NONE:
		return LPF_SERIAL_FLOW_NONE;
	case LPF_CONFIG_MCU_FLOW_HW:
		return LPF_SERIAL_FLOW_HW;
	case LPF_CONFIG_MCU_FLOW_SW:
		return LPF_SERIAL_FLOW_SW;
	default:
		return LPF_SERIAL_FLOW_NONE;
	}
}

/*===========================================================================
 * 串口通信实现
 *===========================================================================*/

/**
 * @brief 串口通信上下文
 */
typedef struct {
	lpf_hw_transport_uart_handle_t serial_handle;
	osal_mutex_t rx_mutex;
} lpf_mcu_serial_context_t;

/**
 * @brief 初始化串口通信
 */
static int32_t lpf_mcu_transport_uart_open(
	const lpf_config_mcu_config_t *mcu_cfg,
	lpf_mcu_transport_handle_t *handle)
{
	lpf_mcu_serial_context_t *ctx;
	lpf_serial_config_t serial_config;

	if (!mcu_cfg || !handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_serial_context_t *)osal_malloc(sizeof(lpf_mcu_serial_context_t));
	if (!ctx) {
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(ctx, 0, sizeof(lpf_mcu_serial_context_t));

	/* 配置串口参数 */
	serial_config.baud_rate = mcu_cfg->hw.serial.baudrate;
	serial_config.data_bits = mcu_cfg->hw.serial.data_bits;
	serial_config.stop_bits = mcu_cfg->hw.serial.stop_bits;
	serial_config.parity =
		_lpf_mcu_to_lpf_hw_parity(mcu_cfg->hw.serial.parity);
	serial_config.flow_control =
		_lpf_mcu_to_lpf_hw_flow_control(mcu_cfg->hw.serial.flow_control);

	if (OSAL_SUCCESS != lpf_hw_transport_uart_open(mcu_cfg->hw.serial.device,
										&serial_config, &ctx->serial_handle)) {
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 创建接收互斥锁 */
	if (OSAL_SUCCESS != osal_mutex_init(&ctx->rx_mutex, NULL)) {
		lpf_hw_transport_uart_close(ctx->serial_handle);
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	*handle = ctx;
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化串口通信
 */
static int32_t lpf_mcu_transport_uart_close(lpf_mcu_transport_handle_t handle)
{
	lpf_mcu_serial_context_t *ctx;

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_serial_context_t *)handle;

	lpf_hw_transport_uart_close(ctx->serial_handle);
	osal_mutex_destroy(&ctx->rx_mutex);
	osal_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 发送 LPF protocol 报文并接收响应（透传模式）
 */
static int32_t lpf_mcu_transport_uart_transfer(
	lpf_mcu_transport_handle_t handle, const uint8_t *packet,
	uint32_t packet_len, uint8_t *response, uint32_t resp_size,
	uint32_t *actual_size, uint32_t timeout_ms)
{
	lpf_mcu_serial_context_t *ctx;
	int32_t ret;
	int32_t rx_len;
	uint64_t start_time_us;
	uint64_t elapsed_us;
	uint32_t elapsed_ms;
	uint32_t remaining_timeout_ms;

	if (!handle || !packet) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (lpf_mcu_serial_context_t *)handle;

	/* 记录起始时间 */
	start_time_us = osal_get_monotonic_time();

	/* 发送 LPF protocol 报文（完整报文，包含协议头和 CRC） */
	ret = lpf_hw_transport_uart_write(ctx->serial_handle, packet, packet_len, timeout_ms);
	if (ret != (int32_t)packet_len) {
		return OSAL_ERR_GENERIC;
	}

	/* 计算剩余超时时间 */
	elapsed_us = osal_get_monotonic_time() - start_time_us;
	elapsed_ms = (uint32_t)div_u64(elapsed_us, 1000);
	if (elapsed_ms >= timeout_ms) {
		return OSAL_ERR_TIMEOUT;
	}
	remaining_timeout_ms = timeout_ms - elapsed_ms;

	/* 接收响应报文 */
	osal_mutex_lock(&ctx->rx_mutex);

	rx_len = lpf_hw_transport_uart_read(ctx->serial_handle, response, resp_size,
							 remaining_timeout_ms);

	osal_mutex_unlock(&ctx->rx_mutex);

	if (rx_len < 0) {
		return OSAL_ERR_GENERIC;
	} else if (rx_len == 0) {
		return OSAL_ERR_TIMEOUT;
	}

	if (actual_size) {
		*actual_size = (uint32_t)rx_len;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief Serial接口的ops结构定义（导出供lpf_mcu.c使用）
 */
const lpf_mcu_transport_ops_t lpf_mcu_transport_uart_ops = {
	.interface = LPF_CONFIG_MCU_INTERFACE_SERIAL,
	.name = "uart",
	.open = lpf_mcu_transport_uart_open,
	.close = lpf_mcu_transport_uart_close,
	.transfer = lpf_mcu_transport_uart_transfer,
};
