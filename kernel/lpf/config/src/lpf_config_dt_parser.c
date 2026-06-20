// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "lpf_config_dt_parser.h"

#define LPF_CONFIG_DT_GROUP_MCU "mcu"
#define LPF_CONFIG_DT_GROUP_LED "led"

#define LPF_CONFIG_DT_PROP_PLATFORM_NAME "platform-name"
#define LPF_CONFIG_DT_PROP_CHIP_NAME "chip-name"
#define LPF_CONFIG_DT_PROP_PROJECT_NAME "project-name"
#define LPF_CONFIG_DT_PROP_PRODUCT_NAME "product-name"
#define LPF_CONFIG_DT_PROP_CONFIG_VERSION "config-version"

#define LPF_CONFIG_DT_PROP_LABEL "label"
#define LPF_CONFIG_DT_PROP_INTERFACE "interface"
#define LPF_CONFIG_DT_PROP_DEVICE "device"
#define LPF_CONFIG_DT_PROP_BITRATE "bitrate"
#define LPF_CONFIG_DT_PROP_RX_TIMEOUT_MS "rx-timeout-ms"
#define LPF_CONFIG_DT_PROP_TX_TIMEOUT_MS "tx-timeout-ms"
#define LPF_CONFIG_DT_PROP_TX_ID "tx-id"
#define LPF_CONFIG_DT_PROP_RX_ID "rx-id"
#define LPF_CONFIG_DT_PROP_BAUDRATE "baudrate"
#define LPF_CONFIG_DT_PROP_DATA_BITS "data-bits"
#define LPF_CONFIG_DT_PROP_STOP_BITS "stop-bits"
#define LPF_CONFIG_DT_PROP_PARITY "parity"
#define LPF_CONFIG_DT_PROP_FLOW_CONTROL "flow-control"
#define LPF_CONFIG_DT_PROP_CMD_TIMEOUT_MS "cmd-timeout-ms"
#define LPF_CONFIG_DT_PROP_RETRY_COUNT "retry-count"

#define LPF_CONFIG_DT_PROP_CONTROL "control"
#define LPF_CONFIG_DT_PROP_MAX_BRIGHTNESS "max-brightness"
#define LPF_CONFIG_DT_PROP_DEFAULT_BRIGHTNESS "default-brightness"
#define LPF_CONFIG_DT_PROP_GPIO "gpio"
#define LPF_CONFIG_DT_PROP_PIN_MUX "pin-mux"
#define LPF_CONFIG_DT_PROP_ACTIVE_LOW "active-low"
#define LPF_CONFIG_DT_PROP_PULL_UP "pull-up"
#define LPF_CONFIG_DT_PROP_PULL_DOWN "pull-down"
#define LPF_CONFIG_DT_PROP_CONSUMER "consumer"
#define LPF_CONFIG_DT_PROP_PERIOD_NS "period-ns"
#define LPF_CONFIG_DT_PROP_POLARITY_INVERSED "polarity-inversed"

static const char *lpf_config_dt_string_or_default(const char *value,
						   const char *fallback)
{
	return value ? value : fallback;
}

static int32_t lpf_config_dt_read_string(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	const char *property, const char **out, const char *fallback)
{
	const char *value = NULL;
	int32_t ret;

	if (!ops || !ops->read_string || !out)
		return OSAL_ERR_INVALID_PARAM;

	ret = ops->read_string(node, property, &value);
	if (ret != OSAL_SUCCESS && !fallback)
		return OSAL_ERR_INVALID_PARAM;

	*out = lpf_config_dt_string_or_default(value, fallback);
	return OSAL_SUCCESS;
}

static uint32_t lpf_config_dt_read_u32_default(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	const char *property, uint32_t fallback)
{
	uint32_t value;

	if (ops && ops->read_u32 &&
	    ops->read_u32(node, property, &value) == OSAL_SUCCESS)
		return value;

	return fallback;
}

static bool lpf_config_dt_read_bool_default(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	const char *property, bool fallback)
{
	if (ops && ops->read_bool && ops->read_bool(node, property))
		return true;

	return fallback;
}

static const char *lpf_config_dt_node_name(
	const lpf_config_dt_node_ops_t *ops, const void *node)
{
	if (!ops || !ops->name)
		return NULL;

	return ops->name(node);
}

static const void *lpf_config_dt_find_child(
	const lpf_config_dt_node_ops_t *ops, const void *root,
	const char *name)
{
	const void *child = NULL;

	if (!ops || !ops->next_available_child || !name)
		return NULL;

	while ((child = ops->next_available_child(root, child)) != NULL) {
		const char *child_name = lpf_config_dt_node_name(ops, child);

		if (child_name && osal_strcmp(child_name, name) == 0)
			return child;
	}

	return NULL;
}

static uint32_t lpf_config_dt_count_available_children(
	const lpf_config_dt_node_ops_t *ops, const void *parent)
{
	const void *child = NULL;
	uint32_t count = 0;

	if (!ops || !ops->next_available_child || !parent)
		return 0;

	while ((child = ops->next_available_child(parent, child)) != NULL)
		count++;

	return count;
}

static lpf_config_mcu_interface_t
lpf_config_dt_parse_mcu_interface(const char *value)
{
	if (value && osal_strcmp(value, "serial") == 0)
		return LPF_CONFIG_MCU_INTERFACE_SERIAL;

	if (value && osal_strcmp(value, "i2c") == 0)
		return LPF_CONFIG_MCU_INTERFACE_I2C;

	if (value && osal_strcmp(value, "spi") == 0)
		return LPF_CONFIG_MCU_INTERFACE_SPI;

	return LPF_CONFIG_MCU_INTERFACE_CAN;
}

static lpf_config_mcu_parity_t lpf_config_dt_parse_parity(const char *value)
{
	if (value && osal_strcmp(value, "odd") == 0)
		return LPF_CONFIG_MCU_PARITY_ODD;

	if (value && osal_strcmp(value, "even") == 0)
		return LPF_CONFIG_MCU_PARITY_EVEN;

	return LPF_CONFIG_MCU_PARITY_NONE;
}

static lpf_config_mcu_flow_control_t
lpf_config_dt_parse_flow_control(const char *value)
{
	if (value && osal_strcmp(value, "hw") == 0)
		return LPF_CONFIG_MCU_FLOW_HW;

	if (value && osal_strcmp(value, "sw") == 0)
		return LPF_CONFIG_MCU_FLOW_SW;

	return LPF_CONFIG_MCU_FLOW_NONE;
}

static lpf_config_led_control_t
lpf_config_dt_parse_led_control(const char *value)
{
	if (value && osal_strcmp(value, "pwm") == 0)
		return LPF_CONFIG_LED_CONTROL_PWM;

	return LPF_CONFIG_LED_CONTROL_GPIO;
}

static int32_t lpf_config_dt_parse_mcu_can(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	lpf_config_mcu_config_t *config)
{
	const char *device;
	int32_t ret;

	ret = lpf_config_dt_read_string(ops, node, LPF_CONFIG_DT_PROP_DEVICE,
					&device, NULL);
	if (ret != OSAL_SUCCESS)
		return ret;

	config->hw.can.device = device;
	config->hw.can.bitrate = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_BITRATE, 500000U);
	config->hw.can.rx_timeout = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_RX_TIMEOUT_MS, 1000U);
	config->hw.can.tx_timeout = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_TX_TIMEOUT_MS, 1000U);
	config->hw.can.tx_id = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_TX_ID, 0U);
	config->hw.can.rx_id = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_RX_ID, 0U);
	return OSAL_SUCCESS;
}

static int32_t lpf_config_dt_parse_mcu_serial(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	lpf_config_mcu_config_t *config)
{
	const char *device;
	const char *parity = NULL;
	const char *flow_control = NULL;
	int32_t ret;

	ret = lpf_config_dt_read_string(ops, node, LPF_CONFIG_DT_PROP_DEVICE,
					&device, NULL);
	if (ret != OSAL_SUCCESS)
		return ret;

	(void)lpf_config_dt_read_string(ops, node, LPF_CONFIG_DT_PROP_PARITY,
					&parity, NULL);
	(void)lpf_config_dt_read_string(ops, node,
					LPF_CONFIG_DT_PROP_FLOW_CONTROL,
					&flow_control, NULL);

	config->hw.serial.device = device;
	config->hw.serial.baudrate = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_BAUDRATE, 115200U);
	config->hw.serial.data_bits = (uint8_t)lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_DATA_BITS, 8U);
	config->hw.serial.stop_bits = (uint8_t)lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_STOP_BITS, 1U);
	config->hw.serial.parity = lpf_config_dt_parse_parity(parity);
	config->hw.serial.flow_control =
		lpf_config_dt_parse_flow_control(flow_control);
	return OSAL_SUCCESS;
}

static int32_t lpf_config_dt_parse_mcu_entry(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	lpf_config_mcu_entry_t *entry)
{
	const char *name;
	const char *interface = NULL;
	int32_t ret;

	ret = lpf_config_dt_read_string(
		ops, node, LPF_CONFIG_DT_PROP_LABEL, &name,
		lpf_config_dt_node_name(ops, node));
	if (ret != OSAL_SUCCESS)
		return ret;

	(void)lpf_config_dt_read_string(ops, node,
					LPF_CONFIG_DT_PROP_INTERFACE,
					&interface, NULL);

	entry->description = lpf_config_dt_string_or_default(name, "mcu");
	entry->enabled = true;
	osal_strncpy(entry->config.name, name,
		     sizeof(entry->config.name) - 1U);
	entry->config.name[sizeof(entry->config.name) - 1U] = '\0';
	entry->config.interface = lpf_config_dt_parse_mcu_interface(interface);
	entry->config.cmd_timeout_ms = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_CMD_TIMEOUT_MS, 1000U);
	entry->config.retry_count = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_RETRY_COUNT, 3U);

	switch (entry->config.interface) {
	case LPF_CONFIG_MCU_INTERFACE_CAN:
		return lpf_config_dt_parse_mcu_can(ops, node, &entry->config);
	case LPF_CONFIG_MCU_INTERFACE_SERIAL:
		return lpf_config_dt_parse_mcu_serial(ops, node,
						      &entry->config);
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

static int32_t lpf_config_dt_parse_led_entry(
	const lpf_config_dt_node_ops_t *ops, const void *node,
	lpf_config_led_entry_t *entry)
{
	const char *name;
	const char *control = NULL;
	int32_t ret;

	ret = lpf_config_dt_read_string(
		ops, node, LPF_CONFIG_DT_PROP_LABEL, &name,
		lpf_config_dt_node_name(ops, node));
	if (ret != OSAL_SUCCESS)
		return ret;

	(void)lpf_config_dt_read_string(ops, node, LPF_CONFIG_DT_PROP_CONTROL,
					&control, NULL);

	entry->description = lpf_config_dt_string_or_default(name, "led");
	entry->enabled = true;
	osal_strncpy(entry->config.name, name,
		     sizeof(entry->config.name) - 1U);
	entry->config.name[sizeof(entry->config.name) - 1U] = '\0';
	entry->config.control = lpf_config_dt_parse_led_control(control);
	entry->config.max_brightness = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_MAX_BRIGHTNESS, 255U);
	entry->config.default_brightness = lpf_config_dt_read_u32_default(
		ops, node, LPF_CONFIG_DT_PROP_DEFAULT_BRIGHTNESS, 0U);

	switch (entry->config.control) {
	case LPF_CONFIG_LED_CONTROL_GPIO:
		entry->config.hw.gpio.gpio_num =
			lpf_config_dt_read_u32_default(
				ops, node, LPF_CONFIG_DT_PROP_GPIO, 0U);
		entry->config.hw.gpio.pin_mux =
			lpf_config_dt_read_u32_default(
				ops, node, LPF_CONFIG_DT_PROP_PIN_MUX, 0U);
		entry->config.hw.gpio.active_low =
			lpf_config_dt_read_bool_default(
				ops, node, LPF_CONFIG_DT_PROP_ACTIVE_LOW,
				false);
		entry->config.hw.gpio.pull_up =
			lpf_config_dt_read_bool_default(
				ops, node, LPF_CONFIG_DT_PROP_PULL_UP, false);
		entry->config.hw.gpio.pull_down =
			lpf_config_dt_read_bool_default(
				ops, node, LPF_CONFIG_DT_PROP_PULL_DOWN,
				false);
		break;
	case LPF_CONFIG_LED_CONTROL_PWM:
		ret = lpf_config_dt_read_string(ops, node,
						LPF_CONFIG_DT_PROP_CONSUMER,
						&entry->config.hw.pwm.consumer,
						NULL);
		if (ret != OSAL_SUCCESS)
			return ret;
		entry->config.hw.pwm.period_ns =
			lpf_config_dt_read_u32_default(
				ops, node, LPF_CONFIG_DT_PROP_PERIOD_NS, 0U);
		entry->config.hw.pwm.polarity_inversed =
			lpf_config_dt_read_bool_default(
				ops, node,
				LPF_CONFIG_DT_PROP_POLARITY_INVERSED, false);
		break;
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}

	return OSAL_SUCCESS;
}

static int32_t lpf_config_dt_parse_mcu_group(
	const lpf_config_dt_node_ops_t *ops, const void *root,
	lpf_config_dt_parse_result_t *result)
{
	const void *group;
	const void *node = NULL;
	uint32_t count;
	uint32_t index = 0;
	int32_t ret = OSAL_SUCCESS;

	group = lpf_config_dt_find_child(ops, root, LPF_CONFIG_DT_GROUP_MCU);
	if (!group)
		return OSAL_SUCCESS;

	count = lpf_config_dt_count_available_children(ops, group);
	if (count == 0)
		goto out_put_group;

	result->mcu_entries =
		ops->alloc(sizeof(*result->mcu_entries) * count);
	if (!result->mcu_entries) {
		ret = OSAL_ERR_NO_MEMORY;
		goto out_put_group;
	}
	osal_memset(result->mcu_entries, 0,
		    sizeof(*result->mcu_entries) * count);

	while ((node = ops->next_available_child(group, node)) != NULL) {
		ret = lpf_config_dt_parse_mcu_entry(
			ops, node, &result->mcu_entries[index]);
		if (ret != OSAL_SUCCESS)
			goto out_put_node;
		index++;
	}

	result->platform.mcu_count = count;
	result->platform.mcu_array = result->mcu_entries;
	goto out_put_group;

out_put_node:
	if (ops->put_node)
		ops->put_node(node);
out_put_group:
	if (ops->put_node)
		ops->put_node(group);
	return ret;
}

static int32_t lpf_config_dt_parse_led_group(
	const lpf_config_dt_node_ops_t *ops, const void *root,
	lpf_config_dt_parse_result_t *result)
{
	const void *group;
	const void *node = NULL;
	uint32_t count;
	uint32_t index = 0;
	int32_t ret = OSAL_SUCCESS;

	group = lpf_config_dt_find_child(ops, root, LPF_CONFIG_DT_GROUP_LED);
	if (!group)
		return OSAL_SUCCESS;

	count = lpf_config_dt_count_available_children(ops, group);
	if (count == 0)
		goto out_put_group;

	result->led_entries =
		ops->alloc(sizeof(*result->led_entries) * count);
	if (!result->led_entries) {
		ret = OSAL_ERR_NO_MEMORY;
		goto out_put_group;
	}
	osal_memset(result->led_entries, 0,
		    sizeof(*result->led_entries) * count);

	while ((node = ops->next_available_child(group, node)) != NULL) {
		ret = lpf_config_dt_parse_led_entry(
			ops, node, &result->led_entries[index]);
		if (ret != OSAL_SUCCESS)
			goto out_put_node;
		index++;
	}

	result->platform.led_count = count;
	result->platform.led_array = result->led_entries;
	goto out_put_group;

out_put_node:
	if (ops->put_node)
		ops->put_node(node);
out_put_group:
	if (ops->put_node)
		ops->put_node(group);
	return ret;
}

void lpf_config_dt_parse_result_clear(
	const lpf_config_dt_node_ops_t *ops,
	lpf_config_dt_parse_result_t *result)
{
	if (!result)
		return;

	if (ops && ops->free) {
		ops->free(result->mcu_entries);
		ops->free(result->led_entries);
	}

	osal_memset(result, 0, sizeof(*result));
}

int32_t lpf_config_dt_parse_platform(
	const lpf_config_dt_node_ops_t *ops, const void *root,
	lpf_config_dt_parse_result_t *result)
{
	int32_t ret;

	if (!ops || !ops->alloc || !ops->free || !root || !result)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(result, 0, sizeof(*result));

	ret = lpf_config_dt_read_string(
		ops, root, LPF_CONFIG_DT_PROP_PLATFORM_NAME,
		&result->platform.platform_name, "linux");
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_read_string(ops, root, LPF_CONFIG_DT_PROP_CHIP_NAME,
					&result->platform.chip_name,
					"unknown");
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_read_string(
		ops, root, LPF_CONFIG_DT_PROP_PROJECT_NAME,
		&result->platform.project_name, "default");
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_read_string(
		ops, root, LPF_CONFIG_DT_PROP_PRODUCT_NAME,
		&result->platform.product_name, "lpf");
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_read_string(
		ops, root, LPF_CONFIG_DT_PROP_CONFIG_VERSION,
		&result->platform.version, "1.0.0");
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_parse_mcu_group(ops, root, result);
	if (ret != OSAL_SUCCESS)
		goto out_error;

	ret = lpf_config_dt_parse_led_group(ops, root, result);
	if (ret != OSAL_SUCCESS)
		goto out_error;

	return OSAL_SUCCESS;

out_error:
	lpf_config_dt_parse_result_clear(ops, result);
	return ret;
}
