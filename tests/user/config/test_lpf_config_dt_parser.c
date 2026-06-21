// SPDX-License-Identifier: MIT

#include "lpf/config/lpf_config.h"
#include "lpf_config_dt_parser.h"
#include "lpf_config_normalizer.h"
#include "test_lpf_config_compare.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
	FAKE_DT_PROP_STRING,
	FAKE_DT_PROP_U32,
	FAKE_DT_PROP_BOOL,
} fake_dt_property_type_t;

typedef struct {
	const char *name;
	fake_dt_property_type_t type;
	const char *string_value;
	uint32_t u32_value;
} fake_dt_property_t;

typedef struct fake_dt_node {
	const char *name;
	bool available;
	const fake_dt_property_t *properties;
	uint32_t property_count;
	const struct fake_dt_node *children;
	uint32_t child_count;
} fake_dt_node_t;

static void *fake_dt_alloc(uint32_t size)
{
	void *ptr = malloc(size);

	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

static const char *fake_dt_name(const void *node)
{
	const fake_dt_node_t *fake = node;

	return fake ? fake->name : NULL;
}

static const void *fake_dt_next_available_child(const void *node,
						const void *previous)
{
	const fake_dt_node_t *parent = node;
	const fake_dt_node_t *prev = previous;
	uint32_t start = 0;
	uint32_t i;

	if (!parent)
		return NULL;

	if (prev) {
		start = (uint32_t)(prev - parent->children) + 1U;
		if (start > parent->child_count)
			return NULL;
	}

	for (i = start; i < parent->child_count; i++) {
		if (parent->children[i].available)
			return &parent->children[i];
	}

	return NULL;
}

static const fake_dt_property_t *fake_dt_find_property(const fake_dt_node_t *node,
						       const char *property)
{
	uint32_t i;

	if (!node || !property)
		return NULL;

	for (i = 0; i < node->property_count; i++) {
		if (strcmp(node->properties[i].name, property) == 0)
			return &node->properties[i];
	}

	return NULL;
}

static int32_t fake_dt_read_string(const void *node, const char *property,
				   const char **value)
{
	const fake_dt_property_t *prop;

	prop = fake_dt_find_property(node, property);
	if (!prop || prop->type != FAKE_DT_PROP_STRING)
		return OSAL_ERR_NAME_NOT_FOUND;

	*value = prop->string_value;
	return OSAL_SUCCESS;
}

static bool fake_dt_read_bool(const void *node, const char *property)
{
	const fake_dt_property_t *prop;

	prop = fake_dt_find_property(node, property);
	return prop && prop->type == FAKE_DT_PROP_BOOL;
}

static int32_t fake_dt_read_u32(const void *node, const char *property,
				uint32_t *value)
{
	const fake_dt_property_t *prop;

	prop = fake_dt_find_property(node, property);
	if (!prop || prop->type != FAKE_DT_PROP_U32)
		return OSAL_ERR_NAME_NOT_FOUND;

	*value = prop->u32_value;
	return OSAL_SUCCESS;
}

static const lpf_config_dt_node_ops_t g_fake_dt_ops = {
	.name = fake_dt_name,
	.next_available_child = fake_dt_next_available_child,
	.put_node = NULL,
	.read_string = fake_dt_read_string,
	.read_bool = fake_dt_read_bool,
	.read_u32 = fake_dt_read_u32,
	.alloc = fake_dt_alloc,
	.free = free,
};

static const fake_dt_property_t g_fake_mcu0_props[] = {
	{ "label", FAKE_DT_PROP_STRING, "mcu0", 0U },
	{ "interface", FAKE_DT_PROP_STRING, "can", 0U },
	{ "device", FAKE_DT_PROP_STRING, "mock-can0", 0U },
	{ "bitrate", FAKE_DT_PROP_U32, NULL, 500000U },
	{ "rx-timeout-ms", FAKE_DT_PROP_U32, NULL, 10U },
	{ "tx-timeout-ms", FAKE_DT_PROP_U32, NULL, 10U },
	{ "tx-id", FAKE_DT_PROP_U32, NULL, 0x123U },
	{ "rx-id", FAKE_DT_PROP_U32, NULL, 0x123U },
	{ "cmd-timeout-ms", FAKE_DT_PROP_U32, NULL, 1000U },
	{ "retry-count", FAKE_DT_PROP_U32, NULL, 0U },
};

static const fake_dt_property_t g_fake_led_status_props[] = {
	{ "label", FAKE_DT_PROP_STRING, "status", 0U },
	{ "control", FAKE_DT_PROP_STRING, "gpio", 0U },
	{ "max-brightness", FAKE_DT_PROP_U32, NULL, 1U },
	{ "default-brightness", FAKE_DT_PROP_U32, NULL, 0U },
	{ "gpio", FAKE_DT_PROP_U32, NULL, 8U },
	{ "pin-mux", FAKE_DT_PROP_U32, NULL, 0U },
};

static const fake_dt_property_t g_fake_led_activity_props[] = {
	{ "label", FAKE_DT_PROP_STRING, "activity", 0U },
	{ "control", FAKE_DT_PROP_STRING, "pwm", 0U },
	{ "max-brightness", FAKE_DT_PROP_U32, NULL, 255U },
	{ "default-brightness", FAKE_DT_PROP_U32, NULL, 0U },
	{ "consumer", FAKE_DT_PROP_STRING, "lpf-mock-led-pwm0", 0U },
	{ "period-ns", FAKE_DT_PROP_U32, NULL, 1000000U },
};

static const fake_dt_node_t g_fake_mcu_children[] = {
	{
		.name = "mcu0",
		.available = true,
		.properties = g_fake_mcu0_props,
		.property_count = OSAL_ARRAY_SIZE(g_fake_mcu0_props),
	},
};

static const fake_dt_node_t g_fake_led_children[] = {
	{
		.name = "status",
		.available = true,
		.properties = g_fake_led_status_props,
		.property_count = OSAL_ARRAY_SIZE(g_fake_led_status_props),
	},
	{
		.name = "activity",
		.available = true,
		.properties = g_fake_led_activity_props,
		.property_count = OSAL_ARRAY_SIZE(g_fake_led_activity_props),
	},
};

static const fake_dt_node_t g_fake_root_children[] = {
	{
		.name = "mcu",
		.available = true,
		.children = g_fake_mcu_children,
		.child_count = OSAL_ARRAY_SIZE(g_fake_mcu_children),
	},
	{
		.name = "led",
		.available = true,
		.children = g_fake_led_children,
		.child_count = OSAL_ARRAY_SIZE(g_fake_led_children),
	},
};

static const fake_dt_property_t g_fake_root_props[] = {
	{ "platform-name", FAKE_DT_PROP_STRING, "linux", 0U },
	{ "chip-name", FAKE_DT_PROP_STRING, "x86_64", 0U },
	{ "project-name", FAKE_DT_PROP_STRING, "x86_mock_modules", 0U },
	{ "product-name", FAKE_DT_PROP_STRING, "ubuntu", 0U },
	{ "config-version", FAKE_DT_PROP_STRING, "1.0.0", 0U },
};

static const fake_dt_node_t g_fake_root = {
	.name = "lpf",
	.available = true,
	.properties = g_fake_root_props,
	.property_count = OSAL_ARRAY_SIZE(g_fake_root_props),
	.children = g_fake_root_children,
	.child_count = OSAL_ARRAY_SIZE(g_fake_root_children),
};

static int test_fake_dt_parser_matches_mock_static_config(void)
{
	lpf_config_dt_parse_result_t parsed;
	lpf_config_device_config_t
		static_devices[TEST_LPF_CONFIG_DEVICE_CAPACITY];
	lpf_config_device_config_t dt_devices[TEST_LPF_CONFIG_DEVICE_CAPACITY];
	const lpf_config_platform_config_t *static_config;
	uint32_t static_count;
	uint32_t dt_count;
	uint32_t i;
	int ret = 0;

	if (lpf_config_dt_parse_platform(&g_fake_dt_ops, &g_fake_root,
					 &parsed) != OSAL_SUCCESS)
		return 1;

	if (test_lpf_config_string_equal(parsed.platform.platform_name,
					 "linux") ||
	    test_lpf_config_string_equal(parsed.platform.chip_name,
					 "x86_64") ||
	    test_lpf_config_string_equal(parsed.platform.project_name,
					 "x86_mock_modules") ||
	    test_lpf_config_string_equal(parsed.platform.product_name,
					 "ubuntu") ||
	    test_lpf_config_string_equal(parsed.platform.version, "1.0.0")) {
		ret = 2;
		goto out_clear;
	}

	if (parsed.platform.mcu_count != 1U || parsed.platform.led_count != 2U) {
		ret = 3;
		goto out_clear;
	}

	static_config = test_lpf_config_mock_static_config();
	if (!static_config) {
		ret = 7;
		goto out_clear;
	}

	if (test_lpf_config_normalize_platform(static_config, static_devices,
					       &static_count) != OSAL_SUCCESS) {
		ret = 4;
		goto out_clear;
	}

	if (test_lpf_config_normalize_platform(&parsed.platform, dt_devices,
					       &dt_count) != OSAL_SUCCESS) {
		ret = 5;
		goto out_clear;
	}

	if (static_count != dt_count || static_count != 3U) {
		ret = 6;
		goto out_clear;
	}

	for (i = 0; i < static_count; i++) {
		if (test_lpf_config_compare_devices(&static_devices[i],
						    &dt_devices[i])) {
			ret = 10 + (int)i;
			goto out_clear;
		}
	}

out_clear:
	lpf_config_dt_parse_result_clear(&g_fake_dt_ops, &parsed);
	return ret;
}

static int test_missing_required_device_fails(void)
{
	static const fake_dt_property_t mcu_props[] = {
		{ "label", FAKE_DT_PROP_STRING, "broken", 0U },
		{ "interface", FAKE_DT_PROP_STRING, "can", 0U },
	};
	static const fake_dt_node_t mcu_children[] = {
		{
			.name = "broken",
			.available = true,
			.properties = mcu_props,
			.property_count = OSAL_ARRAY_SIZE(mcu_props),
		},
	};
	static const fake_dt_node_t root_children[] = {
		{
			.name = "mcu",
			.available = true,
			.children = mcu_children,
			.child_count = OSAL_ARRAY_SIZE(mcu_children),
		},
	};
	static const fake_dt_node_t root = {
		.name = "lpf",
		.available = true,
		.children = root_children,
		.child_count = OSAL_ARRAY_SIZE(root_children),
	};
	lpf_config_dt_parse_result_t parsed;
	int32_t ret;

	ret = lpf_config_dt_parse_platform(&g_fake_dt_ops, &root, &parsed);
	if (ret != OSAL_ERR_INVALID_PARAM)
		return 101;

	return parsed.mcu_entries == NULL && parsed.led_entries == NULL ? 0 :
								       102;
}

int main(void)
{
	int ret;

	ret = test_fake_dt_parser_matches_mock_static_config();
	if (ret)
		return ret;

	return test_missing_required_device_fails();
}
