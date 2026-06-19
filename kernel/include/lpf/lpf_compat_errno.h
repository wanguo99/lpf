// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_ERRNO_H
#define LPF_COMPAT_ERRNO_H

#include <linux/errno.h>

#include "osal.h"

static inline int32_t lpf_compat_errno_to_status(int ret)
{
	int err;

	if (ret >= 0)
		return OSAL_SUCCESS;

	err = -ret;
	if (err == ENODEV || err == ENOENT)
		return OSAL_ENOENT;
	if (err == EOPNOTSUPP || err == ENOSYS)
		return OSAL_ERR_NOT_SUPPORTED;
	if (err == EBUSY)
		return OSAL_ERR_BUSY;

	return err;
}

#endif /* LPF_COMPAT_ERRNO_H */
