// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_PROC_H
#define PDM_PROC_H

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>


#define PDM_PROC_WRITE_MAX_SIZE 256U

typedef int (*pdm_proc_show_t)(struct seq_file *seq, void *data);
typedef int (*pdm_proc_write_t)(char *command, size_t count, void *data);

typedef struct {
	const char *name;
	pdm_proc_show_t show;
	pdm_proc_write_t write;
	void *data;
	struct proc_dir_entry *entry;
} pdm_proc_entry_t;

int pdm_proc_register(pdm_proc_entry_t *entry, const char *name,
		      pdm_proc_show_t show, pdm_proc_write_t write,
		      void *data);
void pdm_proc_unregister(pdm_proc_entry_t *entry);

#endif /* PDM_PROC_H */
