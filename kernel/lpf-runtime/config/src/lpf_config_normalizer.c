// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "lpf_config_normalizer.h"
#include "lpf_config_validator.h"

static void lpf_config_normalizer_clear_devices(
	lpf_config_device_node_t *nodes, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		nodes[i].device_type = LPF_CONFIG_DEVICE_TYPE_INVALID;
		nodes[i].index = 0;
		nodes[i].name = NULL;
		nodes[i].compatible = NULL;
		nodes[i].status = LPF_CONFIG_NODE_STATUS_DISABLED;
		nodes[i].payload = NULL;
		nodes[i].entry = NULL;
		nodes[i].payload_size = 0;
	}
}

static int32_t lpf_config_copy_platform_nodes(
	const lpf_config_platform_config_t *platform,
	lpf_config_device_node_t *nodes, uint32_t *count)
{
	uint32_t capacity;
	uint32_t i;

	if (!platform || !count)
		return OSAL_ERR_INVALID_PARAM;

	if (!platform->device_nodes || platform->device_node_count == 0)
		return OSAL_ERR_NAME_NOT_FOUND;

	capacity = nodes ? *count : 0;
	if (nodes && capacity > 0)
		lpf_config_normalizer_clear_devices(nodes, capacity);

	if (!nodes) {
		*count = platform->device_node_count;
		return OSAL_SUCCESS;
	}

	for (i = 0; i < platform->device_node_count && i < capacity; i++)
		nodes[i] = platform->device_nodes[i];

	*count = platform->device_node_count;
	if (platform->device_node_count >= capacity)
		return OSAL_ERR_RESOURCE_LIMIT;

	nodes[platform->device_node_count].device_type =
		LPF_CONFIG_DEVICE_TYPE_INVALID;
	return OSAL_SUCCESS;
}

int32_t lpf_config_build_device_nodes(
	const lpf_config_platform_config_t *platform,
	lpf_config_device_node_t *nodes, uint32_t *count)
{
	const lpf_config_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t capacity;
	uint32_t out_index = 0;
	uint32_t desc_index;

	if (!platform || !count)
		return OSAL_ERR_INVALID_PARAM;

	if (platform->device_nodes && platform->device_node_count > 0)
		return lpf_config_copy_platform_nodes(platform, nodes, count);

	capacity = nodes ? *count : 0;
	if (nodes && capacity > 0)
		lpf_config_normalizer_clear_devices(nodes, capacity);

	descriptors = lpf_config_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const lpf_config_device_descriptor_t *desc;
		uint32_t device_count;
		uint32_t i;

		desc = &descriptors[desc_index];
		device_count = desc->count(platform);
		for (i = 0; i < device_count; i++) {
			const void *entry;

			entry = desc->entry(platform, i);
			if (!desc->enabled(entry))
				continue;

			if (nodes && out_index < capacity) {
				nodes[out_index].device_type = desc->type;
				nodes[out_index].index = i;
				nodes[out_index].name = desc->node_name ?
								desc->node_name(entry) :
								NULL;
				nodes[out_index].compatible = desc->compatible;
				nodes[out_index].status =
					LPF_CONFIG_NODE_STATUS_OKAY;
				nodes[out_index].payload = entry;
				nodes[out_index].entry = entry;
				nodes[out_index].payload_size = desc->payload_size;
			}
			out_index++;
		}
	}

	*count = out_index;
	if (!nodes)
		return OSAL_SUCCESS;

	if (out_index >= capacity)
		return OSAL_ERR_RESOURCE_LIMIT;

	nodes[out_index].device_type = LPF_CONFIG_DEVICE_TYPE_INVALID;
	return OSAL_SUCCESS;
}

int32_t lpf_config_normalize_devices(
	const lpf_config_platform_config_t *platform,
	lpf_config_device_config_t *devices, uint32_t *count)
{
	return lpf_config_build_device_nodes(platform, devices, count);
}
