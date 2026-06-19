// SPDX-License-Identifier: GPL-2.0

#include "pdm_internal.h"

const pdm_driver_t *const pdm_driver_start
	__attribute__((used, aligned(sizeof(void *)),
		       section(PDM_DRIVER_SECTION))) = NULL;
