/* SPDX-License-Identifier: GPL-2.0 */

#ifndef LPF_CONFIG_NORMALIZER_H
#define LPF_CONFIG_NORMALIZER_H

#include "lpf/config/lpf_config.h"

int32_t lpf_config_normalize_devices(
	const lpf_config_platform_config_t *platform,
	lpf_config_device_config_t *devices, uint32_t *count);

#endif /* LPF_CONFIG_NORMALIZER_H */
