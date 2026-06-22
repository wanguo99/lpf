// SPDX-License-Identifier: GPL-2.0

#include "lpf/core/lpf_debugfs.h"

#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/list.h>

typedef struct {
	struct list_head node;
	char name[LPF_DEBUGFS_ROOT_NAME_LEN];
	struct dentry *root;
	uint32_t users;
	bool available;
} lpf_debugfs_root_t;

static LIST_HEAD(g_lpf_debugfs_roots);
static osal_mutex_t g_lpf_debugfs_lock;
static bool g_lpf_debugfs_lock_ready;

static int lpf_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t lpf_debugfs_write(struct file *file,
				 const char __user *buffer, size_t count,
				 loff_t *ppos)
{
	lpf_debugfs_entry_t *entry = file->private_data;
	char *command;
	int32_t status;
	int ret;

	if (!entry || !entry->write)
		return -EOPNOTSUPP;

	if (count == 0)
		return 0;

	if (count > LPF_DEBUGFS_WRITE_MAX_SIZE)
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

static const struct file_operations g_lpf_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = lpf_debugfs_open,
	.write = lpf_debugfs_write,
};

static int lpf_debugfs_lock_init(void)
{
	int ret;

	if (g_lpf_debugfs_lock_ready)
		return 0;

	ret = osal_mutex_init(&g_lpf_debugfs_lock, NULL);
	if (ret != OSAL_SUCCESS)
		return -ret;

	g_lpf_debugfs_lock_ready = true;
	return 0;
}

static lpf_debugfs_root_t *
lpf_debugfs_find_root_locked(const char *root_name)
{
	lpf_debugfs_root_t *root;

	list_for_each_entry(root, &g_lpf_debugfs_roots, node) {
		if (0 == osal_strcmp(root->name, root_name))
			return root;
	}

	return NULL;
}

static int lpf_debugfs_root_get(const char *root_name,
				lpf_debugfs_root_t **root_out)
{
	lpf_debugfs_root_t *root;
	int ret;

	if (!root_name || !root_out || root_name[0] == '\0')
		return -EINVAL;

	ret = lpf_debugfs_lock_init();
	if (ret)
		return ret;

	osal_mutex_lock(&g_lpf_debugfs_lock);
	root = lpf_debugfs_find_root_locked(root_name);
	if (!root) {
		root = osal_zalloc(sizeof(*root));
		if (!root) {
			osal_mutex_unlock(&g_lpf_debugfs_lock);
			return -ENOMEM;
		}

		INIT_LIST_HEAD(&root->node);
		osal_strncpy(root->name, root_name, sizeof(root->name));
		root->name[sizeof(root->name) - 1U] = '\0';
		root->available = true;
		list_add_tail(&root->node, &g_lpf_debugfs_roots);
	}

	if (!root->available) {
		osal_mutex_unlock(&g_lpf_debugfs_lock);
		return -ENODEV;
	}

	if (!root->root) {
		root->root = debugfs_create_dir(root->name, NULL);
		if (IS_ERR_OR_NULL(root->root)) {
			ret = root->root ? PTR_ERR(root->root) : -ENOMEM;
			root->root = NULL;
			root->available = false;
			osal_mutex_unlock(&g_lpf_debugfs_lock);
			return ret;
		}
	}

	root->users++;
	*root_out = root;
	osal_mutex_unlock(&g_lpf_debugfs_lock);
	return 0;
}

static void lpf_debugfs_root_put(const char *root_name)
{
	lpf_debugfs_root_t *root;

	if (!root_name || !g_lpf_debugfs_lock_ready)
		return;

	osal_mutex_lock(&g_lpf_debugfs_lock);
	root = lpf_debugfs_find_root_locked(root_name);
	if (!root) {
		osal_mutex_unlock(&g_lpf_debugfs_lock);
		return;
	}

	if (root->users > 0)
		root->users--;
	if (root->users == 0 && root->root) {
		debugfs_remove_recursive(root->root);
		root->root = NULL;
	}
	osal_mutex_unlock(&g_lpf_debugfs_lock);
}

int lpf_debugfs_register(lpf_debugfs_entry_t *entry, const char *root_name,
			 const char *name, lpf_debugfs_write_t write,
			 void *data)
{
	lpf_debugfs_root_t *root = NULL;
	int ret;

	if (!entry || !root_name || !name || !write)
		return -EINVAL;

	osal_memset(entry, 0, sizeof(*entry));

	ret = lpf_debugfs_root_get(root_name, &root);
	if (ret == -ENODEV)
		return 0;
	if (ret)
		return ret;

	osal_strncpy(entry->root_name, root_name, sizeof(entry->root_name));
	entry->root_name[sizeof(entry->root_name) - 1U] = '\0';
	entry->name = name;
	entry->write = write;
	entry->data = data;
	entry->entry = debugfs_create_file(name, 0200, root->root, entry,
					   &g_lpf_debugfs_fops);
	if (IS_ERR_OR_NULL(entry->entry)) {
		ret = entry->entry ? PTR_ERR(entry->entry) : -ENOMEM;
		lpf_debugfs_root_put(root_name);
		osal_memset(entry, 0, sizeof(*entry));
		return ret;
	}

	LOG_INFO(name, "/sys/kernel/debug/%s/%s ready", root_name, name);
	return 0;
}
EXPORT_SYMBOL_GPL(lpf_debugfs_register);

void lpf_debugfs_unregister(lpf_debugfs_entry_t *entry)
{
	char root_name[LPF_DEBUGFS_ROOT_NAME_LEN];

	if (!entry || !entry->entry)
		return;

	osal_strncpy(root_name, entry->root_name, sizeof(root_name));
	root_name[sizeof(root_name) - 1U] = '\0';

	debugfs_remove(entry->entry);
	osal_memset(entry, 0, sizeof(*entry));
	lpf_debugfs_root_put(root_name);
}
EXPORT_SYMBOL_GPL(lpf_debugfs_unregister);
