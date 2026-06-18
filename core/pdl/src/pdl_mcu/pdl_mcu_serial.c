/************************************************************************
 * MCU串口通信实现
 *
 * 职责：
 * - 封装串口收发
 * - 直接转发 PRL 协议报文（透传模式）
 * - 超时控制
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdl.h"
#include "pdl_mcu_internal.h"

/*===========================================================================
 * PDL → HAL 类型转换（内部函数，不对外暴露）
 *===========================================================================*/

/**
 * @brief 转换 PDL 校验位类型到 HAL 校验位类型
 */
static uint8_t _pdl_to_hal_parity(pconfig_mcu_parity_t pdl_parity)
{
	switch (pdl_parity) {
	case PCONFIG_MCU_PARITY_NONE:
		return HAL_SERIAL_PARITY_NONE;
	case PCONFIG_MCU_PARITY_ODD:
		return HAL_SERIAL_PARITY_ODD;
	case PCONFIG_MCU_PARITY_EVEN:
		return HAL_SERIAL_PARITY_EVEN;
	default:
		return HAL_SERIAL_PARITY_NONE;
	}
}

/**
 * @brief 转换 PDL 流控类型到 HAL 流控类型
 */
static uint8_t _pdl_to_hal_flow_control(pconfig_mcu_flow_control_t pdl_flow)
{
	switch (pdl_flow) {
	case PCONFIG_MCU_FLOW_NONE:
		return HAL_SERIAL_FLOW_NONE;
	case PCONFIG_MCU_FLOW_HW:
		return HAL_SERIAL_FLOW_HW;
	case PCONFIG_MCU_FLOW_SW:
		return HAL_SERIAL_FLOW_SW;
	default:
		return HAL_SERIAL_FLOW_NONE;
	}
}

/*===========================================================================
 * 串口通信实现
 *===========================================================================*/

/**
 * @brief 串口通信上下文
 */
typedef struct {
	hal_serial_handle_t serial_handle;
	osal_mutex_t rx_mutex;
} mcu_serial_context_t;

/**
 * @brief 初始化串口通信
 */
int32_t mcu_serial_init(const void *config, void **handle)
{
	const pconfig_mcu_config_t *mcu_cfg;
	mcu_serial_context_t *ctx;
	hal_serial_config_t serial_config;

	if (!config || !handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	mcu_cfg = (const pconfig_mcu_config_t *)config;
	ctx = (mcu_serial_context_t *)osal_malloc(sizeof(mcu_serial_context_t));
	if (!ctx) {
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(ctx, 0, sizeof(mcu_serial_context_t));

	/* 配置串口参数 */
	serial_config.baud_rate = mcu_cfg->hw.serial.baudrate;
	serial_config.data_bits = mcu_cfg->hw.serial.data_bits;
	serial_config.stop_bits = mcu_cfg->hw.serial.stop_bits;
	serial_config.parity = _pdl_to_hal_parity(mcu_cfg->hw.serial.parity);
	serial_config.flow_control =
		_pdl_to_hal_flow_control(mcu_cfg->hw.serial.flow_control);

	if (OSAL_SUCCESS != hal_serial_open(mcu_cfg->hw.serial.device,
										&serial_config, &ctx->serial_handle)) {
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 创建接收互斥锁 */
	if (OSAL_SUCCESS != osal_mutex_init(&ctx->rx_mutex, NULL)) {
		hal_serial_close(ctx->serial_handle);
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	*handle = ctx;
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化串口通信
 */
int32_t mcu_serial_deinit(void *handle)
{
	mcu_serial_context_t *ctx;

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (mcu_serial_context_t *)handle;

	hal_serial_close(ctx->serial_handle);
	osal_mutex_destroy(&ctx->rx_mutex);
	osal_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 发送 PRL 报文并接收响应（透传模式）
 */
int32_t mcu_serial_send_packet(void *handle, const uint8_t *packet,
							   uint32_t packet_len, uint8_t *response,
							   uint32_t resp_size, uint32_t *actual_size,
							   uint32_t timeout_ms)
{
	mcu_serial_context_t *ctx;
	int32_t ret;
	int32_t rx_len;
	uint64_t start_time_us;
	uint64_t elapsed_us;
	uint32_t remaining_timeout_ms;

	if (!handle || !packet) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = (mcu_serial_context_t *)handle;

	/* 记录起始时间 */
	start_time_us = osal_get_monotonic_time();

	/* 发送 PRL 报文（完整报文，包含协议头和 CRC） */
	ret = hal_serial_write(ctx->serial_handle, packet, packet_len, timeout_ms);
	if (ret != (int32_t)packet_len) {
		return OSAL_ERR_GENERIC;
	}

	/* 计算剩余超时时间 */
	elapsed_us = osal_get_monotonic_time() - start_time_us;
	if (elapsed_us / 1000 >= timeout_ms) {
		return OSAL_ERR_TIMEOUT;
	}
	remaining_timeout_ms = timeout_ms - (uint32_t)(elapsed_us / 1000);

	/* 接收响应报文 */
	osal_mutex_lock(&ctx->rx_mutex);

	rx_len = hal_serial_read(ctx->serial_handle, response, resp_size,
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
 * @brief Serial接口的ops结构定义（导出供pdl_mcu.c使用）
 */
const pdl_mcu_ops_t mcu_serial_ops = {
	.init = mcu_serial_init,
	.deinit = mcu_serial_deinit,
	.send_packet = mcu_serial_send_packet,
};
