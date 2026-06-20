// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_CHRDEV_H
#define LPF_CHRDEV_H

#include <linux/fs.h>
#include <linux/miscdevice.h>

#include "osal.h"
#include "lpf/lpf_core.h"

#define LPF_CHRDEV_NAME_LEN 64U
#define LPF_CHRDEV_SOC_NAME_LEN 64U

typedef struct {
	char name[LPF_CHRDEV_NAME_LEN];
	char nodename[LPF_CHRDEV_NAME_LEN];
	lpf_device_info_t info;
	char soc_name[LPF_CHRDEV_SOC_NAME_LEN];
	const struct file_operations *fops;
	osal_mutex_t lock;
	osal_atomic_uint32_t open_count;
	osal_atomic_uint32_t error_count;
	struct miscdevice miscdev;
	uint32_t index;
	bool registered;
} lpf_chrdev_t;

int lpf_chrdev_open(struct file *file);
int lpf_chrdev_release(struct file *file);
int lpf_chrdev_register(lpf_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops);
int lpf_chrdev_register_instance(lpf_chrdev_t *chrdev, const char *name,
				 const char *nodename, uint32_t index,
				 const struct file_operations *fops);
int lpf_chrdev_register_lpf_device(lpf_chrdev_t *chrdev, const char *name,
				   const char *nodename,
				   const lpf_device_t *device,
				   const struct file_operations *fops);
void lpf_chrdev_unregister(lpf_chrdev_t *chrdev);
lpf_chrdev_t *lpf_chrdev_from_file(struct file *file);
uint32_t lpf_chrdev_open_count(const lpf_chrdev_t *chrdev);
uint32_t lpf_chrdev_error_count(const lpf_chrdev_t *chrdev);
void lpf_chrdev_record_error(lpf_chrdev_t *chrdev, int error);
uint32_t lpf_chrdev_index(const lpf_chrdev_t *chrdev);

#endif /* LPF_CHRDEV_H */
