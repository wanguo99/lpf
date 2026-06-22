// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_SYSFS_H
#define PDM_SYSFS_H

#include <linux/device.h>

const struct attribute_group **pdm_chrdev_sysfs_groups(void);

#endif /* PDM_SYSFS_H */
