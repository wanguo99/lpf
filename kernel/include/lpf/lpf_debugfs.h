// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_DEBUGFS_H
#define LPF_DEBUGFS_H

#include <linux/dcache.h>
#include <linux/types.h>

#include "osal.h"

#define LPF_DEBUGFS_ROOT_NAME_LEN 64U
#define LPF_DEBUGFS_WRITE_MAX_SIZE 256U

typedef int (*lpf_debugfs_write_t)(char *command, size_t count, void *data);

typedef struct {
	char root_name[LPF_DEBUGFS_ROOT_NAME_LEN];
	const char *name;
	lpf_debugfs_write_t write;
	void *data;
	struct dentry *entry;
} lpf_debugfs_entry_t;

int lpf_debugfs_register(lpf_debugfs_entry_t *entry, const char *root_name,
			 const char *name, lpf_debugfs_write_t write,
			 void *data);
void lpf_debugfs_unregister(lpf_debugfs_entry_t *entry);

#endif /* LPF_DEBUGFS_H */
