// SPDX-License-Identifier: GPL-2.0

#include "lpf_hw_internal.h"

const lpf_hw_builtin_driver_t lpf_hw_builtin_driver_end
	__attribute__((used, aligned(sizeof(void *)),
		       section(LPF_HW_BUILTIN_DRIVER_SECTION))) = {};
