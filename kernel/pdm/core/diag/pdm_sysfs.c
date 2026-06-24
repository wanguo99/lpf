// SPDX-License-Identifier: GPL-2.0

#include "pdm_sysfs.h"

#include <linux/device.h>

#include "pdm/compat/pdm_compat_sysfs.h"
#include "pdm/core/device/pdm_device.h"

static ssize_t pdm_name_show(struct device *dev,
				     struct device_attribute *attr, char *buf);
static ssize_t compatible_show(struct device *dev,
			       struct device_attribute *attr, char *buf);
static ssize_t pdm_id_show(struct device *dev,
			   struct device_attribute *attr, char *buf);
static ssize_t pdm_type_show(struct device *dev,
			     struct device_attribute *attr, char *buf);
static ssize_t capabilities_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t pdm_driver_show(struct device *dev,
			       struct device_attribute *attr, char *buf);

static DEVICE_ATTR_RO(pdm_name);
static DEVICE_ATTR_RO(compatible);
static DEVICE_ATTR_RO(pdm_id);
static DEVICE_ATTR_RO(pdm_type);
static DEVICE_ATTR_RO(capabilities);
static DEVICE_ATTR_RO(pdm_driver);

static struct attribute *pdm_device_attrs[] = {
	&dev_attr_pdm_name.attr,
	&dev_attr_compatible.attr,
	&dev_attr_pdm_id.attr,
	&dev_attr_pdm_type.attr,
	&dev_attr_capabilities.attr,
	&dev_attr_pdm_driver.attr,
	NULL,
};

static const struct attribute_group pdm_device_attr_group = {
	.attrs = pdm_device_attrs,
};

const struct attribute_group *pdm_device_attr_groups[] = {
	&pdm_device_attr_group,
	NULL,
};

static ssize_t pdm_name_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%s\n", dev_name(dev));
}

static ssize_t compatible_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%s\n",
				      pdm_dev->compatible ? pdm_dev->compatible : "");
}

static ssize_t pdm_id_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%d\n", pdm_dev->id);
}

static ssize_t pdm_type_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%u\n", pdm_dev->type);
}

static ssize_t capabilities_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "0x%llx\n",
				      (unsigned long long)pdm_dev->capabilities);
}

static ssize_t pdm_driver_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%s\n",
				      dev->driver ? dev->driver->name : "");
}
