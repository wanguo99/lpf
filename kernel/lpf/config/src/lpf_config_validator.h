/* SPDX-License-Identifier: GPL-2.0 */

#ifndef LPF_CONFIG_VALIDATOR_H
#define LPF_CONFIG_VALIDATOR_H

#include "lpf/config/lpf_config.h"

typedef uint32_t (*lpf_config_device_count_fn)(
	const lpf_config_platform_config_t *platform);
typedef const void *(*lpf_config_device_entry_fn)(
	const lpf_config_platform_config_t *platform, uint32_t index);
typedef bool (*lpf_config_device_enabled_fn)(const void *entry);
typedef int32_t (*lpf_config_device_validate_fn)(uint32_t index,
					      const void *entry);
typedef void (*lpf_config_device_print_fn)(uint32_t index, const void *entry);

typedef struct {
	const char *name;
	lpf_config_device_type_t type;
	lpf_config_device_count_fn count;
	lpf_config_device_entry_fn entry;
	lpf_config_device_enabled_fn enabled;
	lpf_config_device_validate_fn validate;
	lpf_config_device_print_fn print;
} lpf_config_device_descriptor_t;

const lpf_config_device_descriptor_t *lpf_config_device_descriptors(uint32_t *count);

#endif /* LPF_CONFIG_VALIDATOR_H */
