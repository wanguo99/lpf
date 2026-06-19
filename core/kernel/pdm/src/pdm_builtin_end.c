// SPDX-License-Identifier: GPL-2.0

#include "pdm_internal.h"

const pdm_builtin_driver_t pdm_builtin_driver_end
	__attribute__((used, aligned(sizeof(void *)),
		       section(PDM_BUILTIN_DRIVER_SECTION))) = {};
