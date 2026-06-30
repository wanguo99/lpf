// SPDX-License-Identifier: GPL-2.0

#include "pdm_sysfs.h"

#include <linux/device.h>

#include "pdm/compat/pdm_compat_sysfs.h"
#include "pdm/bus/pdm_device.h"

static ssize_t pdm_name_show(struct device *dev,
				     struct device_attribute *attr, char *buf);
static ssize_t compatible_show(struct device *dev,
			       struct device_attribute *attr, char *buf);
static ssize_t pdm_id_show(struct device *dev,
			   struct device_attribute *attr, char *buf);
static ssize_t pdm_type_show(struct device *dev,
			     struct device_attribute *attr, char *buf);
static const char *pdm_owner_name(u32 owner)
{
	switch (owner) {
	case PDM_MANAGER_DEVICE_OWNER_KERNEL:
		return "kernel";
	case PDM_MANAGER_DEVICE_OWNER_USER:
		return "user";
	default:
		return "unspecified";
	}
}

static const char *pdm_transport_name(u32 transport)
{
	switch (transport) {
	case PDM_MANAGER_TRANSPORT_CAN:
		return "can";
	case PDM_MANAGER_TRANSPORT_UART:
		return "uart";
	case PDM_MANAGER_TRANSPORT_I2C:
		return "i2c";
	case PDM_MANAGER_TRANSPORT_SPI:
		return "spi";
	default:
		return "none";
	}
}

static ssize_t owner_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%s\n",
				      pdm_owner_name(pdm_dev->owner));
}

static ssize_t transport_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	return pdm_compat_sysfs_emit(buf, "%s\n",
				      pdm_transport_name(pdm_dev->transport));
}

static ssize_t controller_path_show(struct device *dev,
				   struct device_attribute *attr, char *buf);
static ssize_t capabilities_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t pdm_driver_show(struct device *dev,
			       struct device_attribute *attr, char *buf);

static DEVICE_ATTR_RO(pdm_name);
static DEVICE_ATTR_RO(compatible);
static DEVICE_ATTR_RO(pdm_id);
static DEVICE_ATTR_RO(pdm_type);
static DEVICE_ATTR_RO(owner);
static DEVICE_ATTR_RO(transport);
static DEVICE_ATTR_RO(controller_path);
static DEVICE_ATTR_RO(capabilities);
static DEVICE_ATTR_RO(pdm_driver);

static struct attribute *pdm_device_attrs[] = {
	&dev_attr_pdm_name.attr,
	&dev_attr_compatible.attr,
	&dev_attr_pdm_id.attr,
	&dev_attr_pdm_type.attr,
	&dev_attr_owner.attr,
	&dev_attr_transport.attr,
	&dev_attr_controller_path.attr,
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


static ssize_t controller_path_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	(void)attr;
	if (!pdm_dev->controller_node)
		return pdm_compat_sysfs_emit(buf, "\n");

	return pdm_compat_sysfs_emit(buf, "%pOF\n",
				      pdm_dev->controller_node);
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
