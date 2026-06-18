// SPDX-License-Identifier: GPL-2.0

#include <linux/atomic.h>
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "es_middleware_ko_demo.h"

#define ES_MW_KO_DEMO_DEVICE_NAME "es-middleware-ko-demo"
#define ES_MW_KO_DEMO_STATUS_LEN 160
#define ES_MW_KO_DEMO_MESSAGE_LEN 96

static DEFINE_MUTEX(g_demo_lock);
static atomic_t g_open_count = ATOMIC_INIT(0);
static bool g_default_echo_enabled = true;
static bool g_echo_enabled = true;
static char g_last_message[ES_MW_KO_DEMO_MESSAGE_LEN] = "ready";

module_param_named(default_echo_enabled, g_default_echo_enabled, bool, 0644);
MODULE_PARM_DESC(default_echo_enabled, "Default echo state when the module loads");

static struct miscdevice g_demo_miscdev;

static void es_mw_ko_demo_fill_info(struct es_mw_ko_demo_info *info)
{
	memset(info, 0, sizeof(*info));
	info->abi_version = ESMW_KO_DEMO_ABI_VERSION;
	info->module_version_major = 1;
	info->module_version_minor = 0;
	info->module_version_patch = 0;
	info->open_count = (u32)atomic_read(&g_open_count);
	info->echo_enabled = g_echo_enabled ? 1U : 0U;
	strscpy(info->device_name, ES_MW_KO_DEMO_DEVICE_NAME,
		sizeof(info->device_name));
}

static ssize_t es_mw_ko_demo_read(struct file *file, char __user *buf,
				  size_t len, loff_t *ppos)
{
	char status[ES_MW_KO_DEMO_STATUS_LEN];
	size_t status_len;

	(void)file;

	mutex_lock(&g_demo_lock);
	status_len = scnprintf(status, sizeof(status),
			       "device=%s\nopen_count=%d\necho_enabled=%u\nlast_message=%s\n",
			       ES_MW_KO_DEMO_DEVICE_NAME,
			       atomic_read(&g_open_count),
			       g_echo_enabled ? 1U : 0U,
			       g_last_message);
	mutex_unlock(&g_demo_lock);

	return simple_read_from_buffer(buf, len, ppos, status, status_len);
}

static ssize_t es_mw_ko_demo_write(struct file *file, const char __user *buf,
				   size_t len, loff_t *ppos)
{
	char message[ES_MW_KO_DEMO_MESSAGE_LEN];
	size_t copy_len;

	(void)file;
	(void)ppos;

	if (len == 0)
		return 0;

	copy_len = min(len, sizeof(message) - 1);
	if (copy_from_user(message, buf, copy_len))
		return -EFAULT;

	message[copy_len] = '\0';

	mutex_lock(&g_demo_lock);
	strscpy(g_last_message, message, sizeof(g_last_message));
	mutex_unlock(&g_demo_lock);

	pr_info("%s: write \"%s\"\n", ES_MW_KO_DEMO_DEVICE_NAME, g_last_message);
	return copy_len;
}

static long es_mw_ko_demo_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct es_mw_ko_demo_info info;
	u32 echo_value;

	(void)file;

	switch (cmd) {
	case ESMW_KO_DEMO_IOC_GET_INFO:
		mutex_lock(&g_demo_lock);
		es_mw_ko_demo_fill_info(&info);
		mutex_unlock(&g_demo_lock);

		if (copy_to_user((void __user *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;

	case ESMW_KO_DEMO_IOC_GET_ECHO:
		mutex_lock(&g_demo_lock);
		echo_value = g_echo_enabled ? 1U : 0U;
		mutex_unlock(&g_demo_lock);
		if (copy_to_user((void __user *)arg, &echo_value,
				 sizeof(echo_value)))
			return -EFAULT;
		return 0;

	case ESMW_KO_DEMO_IOC_SET_ECHO:
		if (copy_from_user(&echo_value, (void __user *)arg,
				   sizeof(echo_value)))
			return -EFAULT;

		mutex_lock(&g_demo_lock);
		g_echo_enabled = (echo_value != 0U);
		mutex_unlock(&g_demo_lock);
		return 0;

	case ESMW_KO_DEMO_IOC_PING:
		return 0;

	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
static long es_mw_ko_demo_compat_ioctl(struct file *file, unsigned int cmd,
				       unsigned long arg)
{
	(void)file;
	return es_mw_ko_demo_ioctl(file, cmd, arg);
}
#endif

static int es_mw_ko_demo_open(struct inode *inode, struct file *file)
{
	int open_count;

	(void)inode;
	(void)file;

	mutex_lock(&g_demo_lock);
	open_count = atomic_inc_return(&g_open_count);
	mutex_unlock(&g_demo_lock);

	pr_info("%s: open count=%d\n", ES_MW_KO_DEMO_DEVICE_NAME, open_count);
	return 0;
}

static int es_mw_ko_demo_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	pr_info("%s: release\n", ES_MW_KO_DEMO_DEVICE_NAME);
	return 0;
}

static const struct file_operations es_mw_ko_demo_fops = {
	.owner = THIS_MODULE,
	.open = es_mw_ko_demo_open,
	.release = es_mw_ko_demo_release,
	.read = es_mw_ko_demo_read,
	.write = es_mw_ko_demo_write,
	.unlocked_ioctl = es_mw_ko_demo_ioctl,
	#ifdef CONFIG_COMPAT
	.compat_ioctl = es_mw_ko_demo_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

static int __init es_mw_ko_demo_init(void)
{
	int ret;

	g_echo_enabled = g_default_echo_enabled;
	g_demo_miscdev.minor = MISC_DYNAMIC_MINOR;
	g_demo_miscdev.name = ES_MW_KO_DEMO_DEVICE_NAME;
	g_demo_miscdev.fops = &es_mw_ko_demo_fops;
	g_demo_miscdev.mode = 0666;

	ret = misc_register(&g_demo_miscdev);
	if (ret)
		return ret;

	pr_info("%s: loaded, /dev/%s ready\n", ES_MW_KO_DEMO_DEVICE_NAME,
		ES_MW_KO_DEMO_DEVICE_NAME);
	return 0;
}

static void __exit es_mw_ko_demo_exit(void)
{
	misc_deregister(&g_demo_miscdev);
	pr_info("%s: unloaded\n", ES_MW_KO_DEMO_DEVICE_NAME);
}

module_init(es_mw_ko_demo_init);
module_exit(es_mw_ko_demo_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware kernel demo module");
MODULE_LICENSE("GPL");
