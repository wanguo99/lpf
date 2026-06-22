// SPDX-License-Identifier: GPL-2.0

#include "pdm_sysfs.h"

#include "pdm/core/pdm_chrdev.h"
#include "pdm/compat/pdm_compat_sysfs.h"

static bool pdm_sysfs_get_info(struct device *dev, pdm_device_info_t *info)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	if (!chrdev || !info)
		return false;

	return pdm_chrdev_get_info(chrdev, info) == 0;
}

static ssize_t name_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "\n");

	return pdm_compat_sysfs_emit(buf, "%s\n", info.name);
}

static ssize_t type_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%u\n", info.type);
}

static ssize_t index_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%u\n", info.index);
}

static ssize_t state_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%u\n", info.state);
}

static ssize_t capabilities_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0x0\n");

	return pdm_compat_sysfs_emit(
		buf, "0x%llx\n", (unsigned long long)info.capabilities);
}

static ssize_t driver_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "\n");

	return pdm_compat_sysfs_emit(buf, "%s\n", info.driver_name);
}

static ssize_t soc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return pdm_compat_sysfs_emit(buf, "\n");

	return pdm_compat_sysfs_emit(buf, "%s\n", chrdev->soc_name);
}

static ssize_t last_error_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%d\n", info.last_error);
}

static ssize_t error_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	pdm_device_info_t info;

	(void)attr;
	if (!pdm_sysfs_get_info(dev, &info))
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%u\n", info.error_count);
}

static ssize_t open_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return pdm_compat_sysfs_emit(buf, "0\n");

	return pdm_compat_sysfs_emit(buf, "%u\n",
				     pdm_chrdev_open_count(chrdev));
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(type);
static DEVICE_ATTR_RO(index);
static DEVICE_ATTR_RO(state);
static DEVICE_ATTR_RO(capabilities);
static DEVICE_ATTR_RO(driver);
static DEVICE_ATTR_RO(soc);
static DEVICE_ATTR_RO(last_error);
static DEVICE_ATTR_RO(error_count);
static DEVICE_ATTR_RO(open_count);

static struct attribute *g_lpf_chrdev_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_type.attr,
	&dev_attr_index.attr,
	&dev_attr_state.attr,
	&dev_attr_capabilities.attr,
	&dev_attr_driver.attr,
	&dev_attr_soc.attr,
	&dev_attr_last_error.attr,
	&dev_attr_error_count.attr,
	&dev_attr_open_count.attr,
	NULL,
};

static const struct attribute_group g_lpf_chrdev_attr_group = {
	.attrs = g_lpf_chrdev_attrs,
};

static const struct attribute_group *g_lpf_chrdev_attr_groups[] = {
	&g_lpf_chrdev_attr_group,
	NULL,
};

const struct attribute_group **pdm_chrdev_sysfs_groups(void)
{
	return g_lpf_chrdev_attr_groups;
}
