// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_CHRDEV_H
#define PDM_CHRDEV_H

#include <linux/fs.h>
#include <linux/miscdevice.h>

#include "osal.h"

typedef struct {
	const char *name;
	const struct file_operations *fops;
	osal_mutex_t lock;
	osal_atomic_uint32_t open_count;
	struct miscdevice miscdev;
} pdm_chrdev_t;

long pdm_status_to_errno(int32_t status);
int pdm_chrdev_open(pdm_chrdev_t *chrdev);
int pdm_chrdev_release(pdm_chrdev_t *chrdev);
int pdm_chrdev_register(pdm_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops);
void pdm_chrdev_unregister(pdm_chrdev_t *chrdev);
uint32_t pdm_chrdev_open_count(const pdm_chrdev_t *chrdev);

#endif /* PDM_CHRDEV_H */
