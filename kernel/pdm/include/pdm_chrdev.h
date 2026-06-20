// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_CHRDEV_H
#define PDM_CHRDEV_H

#include <linux/fs.h>
#include <linux/miscdevice.h>

#include "osal.h"
#include "lpf/lpf_device.h"

#include "pdm_status.h"

#define PDM_CHRDEV_NAME_LEN 64U
#define PDM_CHRDEV_SOC_NAME_LEN 64U

typedef struct {
	char name[PDM_CHRDEV_NAME_LEN];
	char nodename[PDM_CHRDEV_NAME_LEN];
	lpf_device_info_t info;
	char soc_name[PDM_CHRDEV_SOC_NAME_LEN];
	const struct file_operations *fops;
	osal_mutex_t lock;
	osal_atomic_uint32_t open_count;
	osal_atomic_uint32_t error_count;
	struct miscdevice miscdev;
	uint32_t index;
	bool registered;
} pdm_chrdev_t;

int pdm_chrdev_open(pdm_chrdev_t *chrdev);
int pdm_chrdev_release(pdm_chrdev_t *chrdev);
int pdm_chrdev_register(pdm_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops);
int pdm_chrdev_register_instance(pdm_chrdev_t *chrdev, const char *name,
				 const char *nodename, uint32_t index,
				 const struct file_operations *fops);
int pdm_chrdev_register_lpf_device(pdm_chrdev_t *chrdev, const char *name,
				   const char *nodename,
				   const lpf_device_t *device,
				   const struct file_operations *fops);
void pdm_chrdev_unregister(pdm_chrdev_t *chrdev);
pdm_chrdev_t *pdm_chrdev_from_file(struct file *file);
uint32_t pdm_chrdev_open_count(const pdm_chrdev_t *chrdev);
uint32_t pdm_chrdev_error_count(const pdm_chrdev_t *chrdev);
void pdm_chrdev_record_error(pdm_chrdev_t *chrdev, int error);
uint32_t pdm_chrdev_index(const pdm_chrdev_t *chrdev);

#endif /* PDM_CHRDEV_H */
