// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_ERRNO_H
#define LPF_ERRNO_H

#include <linux/errno.h>

#include "osal.h"

static inline long lpf_status_to_errno(int32_t status)
{
	if (status <= 0)
		return status;

	if (status == OSAL_ERR_INVALID_POINTER ||
	    status == OSAL_ERR_BAD_ADDRESS)
		return -EFAULT;

	if (status == OSAL_ERR_INVALID_PARAM ||
	    status == OSAL_ERR_INVALID_SIZE ||
	    status == OSAL_ERR_INVALID_ID)
		return -EINVAL;

	if (status == OSAL_ERR_NO_MEMORY)
		return -ENOMEM;
	if (status == OSAL_ERR_TIMEOUT)
		return -ETIMEDOUT;
	if (status == OSAL_ERR_BUSY)
		return -EBUSY;
	if (status == OSAL_ERR_NOT_SUPPORTED)
		return -EOPNOTSUPP;
	if (status == OSAL_ERR_NOT_IMPLEMENTED)
		return -ENOSYS;
	if (status == OSAL_ERR_ALREADY_EXISTS ||
	    status == OSAL_ERR_NAME_TAKEN)
		return -EEXIST;
	if (status == OSAL_ERR_RESOURCE_LIMIT)
		return -EMFILE;
	if (status == OSAL_ERR_INVALID_STATE)
		return -EINVAL;

	return -EIO;
}

#endif /* LPF_ERRNO_H */
