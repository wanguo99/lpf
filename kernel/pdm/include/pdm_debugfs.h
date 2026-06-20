// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_DEBUGFS_H
#define PDM_DEBUGFS_H

#include "lpf/lpf_debugfs.h"

#define PDM_DEBUGFS_ROOT_NAME "pdm"
#define PDM_DEBUGFS_WRITE_MAX_SIZE LPF_DEBUGFS_WRITE_MAX_SIZE

typedef lpf_debugfs_write_t pdm_debugfs_write_t;
typedef lpf_debugfs_entry_t pdm_debugfs_entry_t;

static inline int pdm_debugfs_register(pdm_debugfs_entry_t *entry,
				       const char *name,
				       pdm_debugfs_write_t write,
				       void *data)
{
	return lpf_debugfs_register(entry, PDM_DEBUGFS_ROOT_NAME, name, write,
				    data);
}

static inline void pdm_debugfs_unregister(pdm_debugfs_entry_t *entry)
{
	lpf_debugfs_unregister(entry);
}

#endif /* PDM_DEBUGFS_H */
