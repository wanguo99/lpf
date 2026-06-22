// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_SYSFS_H
#define LPF_COMPAT_SYSFS_H

#include "lpf/compat/lpf_compat_features.h"

#include <linux/kernel.h>
#include <linux/sysfs.h>

#if LPF_KERNEL_HAS_SYSFS_EMIT
#define lpf_compat_sysfs_emit(buf, fmt, ...) \
	sysfs_emit((buf), (fmt), ##__VA_ARGS__)
#else
#define lpf_compat_sysfs_emit(buf, fmt, ...) \
	scnprintf((buf), PAGE_SIZE, (fmt), ##__VA_ARGS__)
#endif

#endif /* LPF_COMPAT_SYSFS_H */
