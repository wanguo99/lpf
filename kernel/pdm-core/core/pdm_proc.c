// SPDX-License-Identifier: GPL-2.0

#include "lpf/core/lpf_proc.h"

#include <linux/errno.h>
#include <linux/fs.h>

#include "lpf/compat/lpf_compat_features.h"

#if !LPF_KERNEL_HAS_PROC_OPS
#error "LPF procfs requires proc_ops; add a compat fallback before lowering the kernel baseline"
#endif

#define LPF_PROC_ROOT_NAME "lpf"

static struct proc_dir_entry *g_lpf_proc_root;
static osal_mutex_t g_lpf_proc_lock;
static bool g_lpf_proc_lock_ready;
static uint32_t g_lpf_proc_users;

static int lpf_proc_open(struct inode *inode, struct file *file)
{
	lpf_proc_entry_t *entry = pde_data(inode);

	if (!entry || !entry->show)
		return -EINVAL;

	return single_open(file, entry->show, entry->data);
}

static ssize_t lpf_proc_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *ppos)
{
	lpf_proc_entry_t *entry = pde_data(file_inode(file));
	char *command;
	int32_t status;
	int ret;

	if (!entry || !entry->write)
		return -EOPNOTSUPP;

	if (count == 0)
		return 0;

	if (count > LPF_PROC_WRITE_MAX_SIZE)
		return -E2BIG;

	command = osal_malloc(count + 1);
	if (!command)
		return -ENOMEM;

	status = osal_copy_from_user(command, buffer, count);
	if (status != OSAL_SUCCESS) {
		osal_free(command);
		return -EFAULT;
	}
	command[count] = '\0';

	ret = entry->write(command, count, entry->data);
	osal_free(command);
	if (ret)
		return ret;

	if (ppos)
		*ppos += count;

	return count;
}

static const struct proc_ops lpf_proc_ops = {
	.proc_open = lpf_proc_open,
	.proc_read = seq_read,
	.proc_write = lpf_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int lpf_proc_root_init(void)
{
	int ret;

	if (!g_lpf_proc_lock_ready) {
		ret = osal_mutex_init(&g_lpf_proc_lock, NULL);
		if (ret != OSAL_SUCCESS)
			return -ret;
		g_lpf_proc_lock_ready = true;
	}

	osal_mutex_lock(&g_lpf_proc_lock);
	if (!g_lpf_proc_root) {
		g_lpf_proc_root = proc_mkdir(LPF_PROC_ROOT_NAME, NULL);
		if (!g_lpf_proc_root) {
			osal_mutex_unlock(&g_lpf_proc_lock);
			return -ENOMEM;
		}
	}
	g_lpf_proc_users++;
	osal_mutex_unlock(&g_lpf_proc_lock);

	return 0;
}

static void lpf_proc_root_deinit(void)
{
	if (!g_lpf_proc_lock_ready)
		return;

	osal_mutex_lock(&g_lpf_proc_lock);
	if (g_lpf_proc_users > 0)
		g_lpf_proc_users--;
	if (g_lpf_proc_users == 0 && g_lpf_proc_root) {
		proc_remove(g_lpf_proc_root);
		g_lpf_proc_root = NULL;
	}
	osal_mutex_unlock(&g_lpf_proc_lock);
}

int lpf_proc_register(lpf_proc_entry_t *entry, const char *name,
		      lpf_proc_show_t show, lpf_proc_write_t write,
		      void *data)
{
	umode_t mode;
	int ret;

	if (!entry || !name || !show)
		return -EINVAL;

	ret = lpf_proc_root_init();
	if (ret)
		return ret;

	osal_memset(entry, 0, sizeof(*entry));
	entry->name = name;
	entry->show = show;
	entry->write = write;
	entry->data = data;
	mode = write ? 0644 : 0444;
	entry->entry = proc_create_data(name, mode, g_lpf_proc_root,
					&lpf_proc_ops, entry);
	if (!entry->entry) {
		lpf_proc_root_deinit();
		osal_memset(entry, 0, sizeof(*entry));
		return -ENOMEM;
	}

	LOG_INFO(name, "/proc/%s/%s ready", LPF_PROC_ROOT_NAME, name);
	return 0;
}
EXPORT_SYMBOL_GPL(lpf_proc_register);

void lpf_proc_unregister(lpf_proc_entry_t *entry)
{
	if (!entry || !entry->entry)
		return;

	proc_remove(entry->entry);
	osal_memset(entry, 0, sizeof(*entry));
	lpf_proc_root_deinit();
}
EXPORT_SYMBOL_GPL(lpf_proc_unregister);
