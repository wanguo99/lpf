// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_PROC_H
#define LPF_PROC_H

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>

#include "osal.h"

#define LPF_PROC_WRITE_MAX_SIZE 256U

typedef int (*lpf_proc_show_t)(struct seq_file *seq, void *data);
typedef int (*lpf_proc_write_t)(char *command, size_t count, void *data);

typedef struct {
	const char *name;
	lpf_proc_show_t show;
	lpf_proc_write_t write;
	void *data;
	struct proc_dir_entry *entry;
} lpf_proc_entry_t;

int lpf_proc_register(lpf_proc_entry_t *entry, const char *name,
		      lpf_proc_show_t show, lpf_proc_write_t write,
		      void *data);
void lpf_proc_unregister(lpf_proc_entry_t *entry);

#endif /* LPF_PROC_H */
