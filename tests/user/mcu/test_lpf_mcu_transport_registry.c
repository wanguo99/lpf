// SPDX-License-Identifier: MIT

#include "lpf/transport/mcu/lpf_mcu_transport.h"

#include <string.h>

#ifndef EXPECT_CAN_TRANSPORT
#define EXPECT_CAN_TRANSPORT 0
#endif

#ifndef EXPECT_UART_TRANSPORT
#define EXPECT_UART_TRANSPORT 0
#endif

static int g_open_count;
static int g_close_count;
static int g_transfer_count;

static int32_t mock_open(const lpf_config_mcu_config_t *config,
			 lpf_mcu_transport_handle_t *handle)
{
	if (!config || !handle)
		return OSAL_ERR_INVALID_PARAM;

	g_open_count++;
	*handle = (lpf_mcu_transport_handle_t)config;
	return OSAL_SUCCESS;
}

static int32_t mock_close(lpf_mcu_transport_handle_t handle)
{
	if (!handle)
		return OSAL_ERR_INVALID_PARAM;

	g_close_count++;
	return OSAL_SUCCESS;
}

static int32_t mock_transfer(lpf_mcu_transport_handle_t handle,
			     const uint8_t *packet, uint32_t packet_len,
			     uint8_t *response, uint32_t response_size,
			     uint32_t *actual_size, uint32_t timeout_ms)
{
	uint32_t copy_len;

	(void)timeout_ms;

	if (!handle || !packet || !actual_size)
		return OSAL_ERR_INVALID_PARAM;

	copy_len = packet_len < response_size ? packet_len : response_size;
	if (response && copy_len > 0)
		memcpy(response, packet, copy_len);
	*actual_size = copy_len;
	g_transfer_count++;

	return OSAL_SUCCESS;
}

const lpf_mcu_transport_ops_t lpf_mcu_transport_can_ops = {
	.interface = LPF_CONFIG_MCU_INTERFACE_CAN,
	.name = "mock-can",
	.open = mock_open,
	.close = mock_close,
	.transfer = mock_transfer,
};

const lpf_mcu_transport_ops_t lpf_mcu_transport_uart_ops = {
	.interface = LPF_CONFIG_MCU_INTERFACE_SERIAL,
	.name = "mock-uart",
	.open = mock_open,
	.close = mock_close,
	.transfer = mock_transfer,
};

static int test_transport_ops(const lpf_mcu_transport_ops_t *ops)
{
	const uint8_t packet[] = { 0x10U, 0x20U, 0x30U, 0x40U };
	uint8_t response[sizeof(packet)];
	lpf_config_mcu_config_t config;
	lpf_mcu_transport_handle_t handle = NULL;
	uint32_t actual_size = 0;

	if (!ops || !ops->open || !ops->close || !ops->transfer)
		return 1;

	memset(&config, 0, sizeof(config));
	config.interface = ops->interface;

	if (ops->open(NULL, &handle) != OSAL_ERR_INVALID_PARAM)
		return 2;
	if (ops->open(&config, NULL) != OSAL_ERR_INVALID_PARAM)
		return 3;
	if (ops->open(&config, &handle) != OSAL_SUCCESS || handle != &config)
		return 4;

	memset(response, 0, sizeof(response));
	if (ops->transfer(handle, packet, sizeof(packet), response,
			  sizeof(response), &actual_size, 100U) !=
	    OSAL_SUCCESS)
		return 5;
	if (actual_size != sizeof(packet) || memcmp(packet, response,
						   sizeof(packet)) != 0)
		return 6;

	if (ops->close(handle) != OSAL_SUCCESS)
		return 7;

	return 0;
}

static int expect_transport(lpf_config_mcu_interface_t interface,
			    const lpf_mcu_transport_ops_t *expected,
			    int should_exist)
{
	const lpf_mcu_transport_ops_t *ops;
	int ret;

	ops = lpf_mcu_transport_get(interface);
	if (!should_exist)
		return ops == NULL ? 0 : 1;

	if (ops != expected)
		return 2;
	if (ops->interface != interface)
		return 3;
	if (!ops->name || ops->name[0] == '\0')
		return 4;

	ret = test_transport_ops(ops);
	return ret == 0 ? 0 : 10 + ret;
}

int main(void)
{
	int ret;

	ret = expect_transport(LPF_CONFIG_MCU_INTERFACE_CAN,
			       &lpf_mcu_transport_can_ops,
			       EXPECT_CAN_TRANSPORT);
	if (ret != 0)
		return 100 + ret;

	ret = expect_transport(LPF_CONFIG_MCU_INTERFACE_SERIAL,
			       &lpf_mcu_transport_uart_ops,
			       EXPECT_UART_TRANSPORT);
	if (ret != 0)
		return 200 + ret;

	if (lpf_mcu_transport_get(LPF_CONFIG_MCU_INTERFACE_I2C) != NULL)
		return 300;
	if (lpf_mcu_transport_get(LPF_CONFIG_MCU_INTERFACE_SPI) != NULL)
		return 301;
	if (lpf_mcu_transport_get((lpf_config_mcu_interface_t)0xFF) != NULL)
		return 302;

	if (g_open_count != g_close_count)
		return 400;
	if (g_open_count != g_transfer_count)
		return 401;

	return 0;
}
