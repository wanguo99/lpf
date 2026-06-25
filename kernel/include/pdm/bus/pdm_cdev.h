// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_cdev.h
 * @brief PDM per-instance character device helper
 */

#ifndef PDM_CDEV_H
#define PDM_CDEV_H

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/types.h>

#include "pdm/bus/pdm_device.h"

#define PDM_CDEV_NAME_LEN 64U
#define PDM_CDEV_NODE_LEN 64U
#define PDM_CDEV_MINORS (MINORMASK + 1)

struct pdm_cdev {
	struct pdm_device *pdm_dev;
	struct device dev;
	struct cdev cdev;
	const struct file_operations *fops;
	void (*release)(struct pdm_cdev *cdev);
	atomic_t open_count;
	int minor;
	bool registered;
	char name[PDM_CDEV_NAME_LEN];
	char nodename[PDM_CDEV_NODE_LEN];
};

int pdm_cdev_init(void);
void pdm_cdev_exit(void);
int pdm_cdev_register(struct pdm_cdev *cdev, struct pdm_device *pdm_dev,
		      const char *name, const char *nodename,
		      const struct file_operations *fops,
		      void (*release)(struct pdm_cdev *cdev));
void pdm_cdev_unregister(struct pdm_cdev *cdev);
u32 pdm_cdev_open_count(const struct pdm_cdev *cdev);
struct pdm_cdev *pdm_cdev_from_file(struct file *filp);
int pdm_cdev_default_open(struct inode *inode, struct file *filp);
int pdm_cdev_default_release(struct inode *inode, struct file *filp);

#endif /* PDM_CDEV_H */
