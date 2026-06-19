// SPDX-License-Identifier: GPL-2.0

#include "pdm_chrdev.h"

#include <linux/errno.h>

long pdm_status_to_errno(int32_t status)
{
	if (status == OSAL_SUCCESS)
		return 0;

	if (status == OSAL_ERR_INVALID_PARAM ||
	    status == OSAL_ERR_INVALID_POINTER ||
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

	return -EIO;
}

int pdm_chrdev_open(pdm_chrdev_t *chrdev)
{
	int open_count;

	if (!chrdev)
		return -EINVAL;

	osal_mutex_lock(&chrdev->lock);
	open_count = (int)osal_atomic_inc(&chrdev->open_count);
	osal_mutex_unlock(&chrdev->lock);

	LOG_INFO(chrdev->name, "open count=%d", open_count);
	return 0;
}

int pdm_chrdev_release(pdm_chrdev_t *chrdev)
{
	int open_count;

	if (!chrdev)
		return -EINVAL;

	osal_mutex_lock(&chrdev->lock);
	if (osal_atomic_load(&chrdev->open_count) > 0)
		osal_atomic_dec(&chrdev->open_count);
	open_count = (int)osal_atomic_load(&chrdev->open_count);
	osal_mutex_unlock(&chrdev->lock);

	LOG_INFO(chrdev->name, "release count=%d", open_count);
	return 0;
}

int pdm_chrdev_register(pdm_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops)
{
	int ret;

	if (!chrdev || !name || !fops)
		return -EINVAL;

	ret = osal_mutex_init(&chrdev->lock, NULL);
	if (ret != OSAL_SUCCESS)
		return -ret;

	chrdev->name = name;
	chrdev->fops = fops;
	osal_atomic_init(&chrdev->open_count, 0);

	chrdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	chrdev->miscdev.name = name;
	chrdev->miscdev.fops = fops;
	chrdev->miscdev.mode = 0666;

	ret = misc_register(&chrdev->miscdev);
	if (ret) {
		osal_mutex_destroy(&chrdev->lock);
		return ret;
	}

	LOG_INFO(name, "/dev/%s ready", name);
	return 0;
}

void pdm_chrdev_unregister(pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return;

	misc_deregister(&chrdev->miscdev);
	osal_mutex_destroy(&chrdev->lock);
	osal_memset(chrdev, 0, sizeof(*chrdev));
}

uint32_t pdm_chrdev_open_count(const pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return osal_atomic_load(&chrdev->open_count);
}
