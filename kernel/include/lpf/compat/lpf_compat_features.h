// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_FEATURES_H
#define LPF_COMPAT_FEATURES_H

#include <linux/version.h>

#define LPF_KERNEL_MIN_VERSION KERNEL_VERSION(5, 10, 0)

#if LINUX_VERSION_CODE < LPF_KERNEL_MIN_VERSION
#error "LPF supports Linux kernel 5.10 or newer; extend lpf_compat before building older kernels"
#endif

#define LPF_KERNEL_HAS_PROC_OPS \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
#define LPF_KERNEL_HAS_SYSFS_EMIT \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define LPF_KERNEL_HAS_SOCKADDR_UNSIZED \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0))

#endif /* LPF_COMPAT_FEATURES_H */
