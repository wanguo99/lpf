// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_backend.c
 * @brief Section-based PDM peripheral backend registry
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#include "pdm/registry/pdm_backend.h"
#include "pdm/log/pdm_log.h"

extern const struct pdm_backend_entry __start_pdm_backend_entries[];
extern const struct pdm_backend_entry __stop_pdm_backend_entries[];

static size_t pdm_backend_entries_initialized;

static bool pdm_backend_entry_matches(const struct pdm_backend_entry *entry,
				      const char *compatible)
{
	const struct of_device_id *match;

	if (!entry || !compatible || !entry->matches) {
		return false;
	}

	for (match = entry->matches; match->compatible[0]; match++) {
		if (!strcmp(match->compatible, compatible)) {
			return true;
		}
	}

	return false;
}

static void pdm_backend_entries_exit_count(size_t count)
{
	const struct pdm_backend_entry *entry;

	while (count) {
		entry = __start_pdm_backend_entries + --count;
		if (!entry->exit) {
			continue;
		}

		LOG_DEBUG("Exiting backend [%s]",
			  entry->name ? entry->name : "unknown");
		entry->exit();
	}
}

int pdm_backend_entries_init(void)
{
	const struct pdm_backend_entry *entry;
	size_t count = 0;
	int ret;

	for (entry = __start_pdm_backend_entries;
	     entry < __stop_pdm_backend_entries; entry++) {
		if (!entry->name || !entry->matches || !entry->ops ||
		    (!!entry->init != !!entry->exit)) {
			LOG_ERROR("Invalid backend entry at index %zu",
				  count);
			ret = -EINVAL;
			goto err_exit_registered;
		}

		if (entry->init) {
			LOG_DEBUG("Initializing backend [%s]",
				  entry->name);
			ret = entry->init();
			if (ret) {
				LOG_ERROR("Failed to initialize backend [%s]: %d",
					  entry->name, ret);
				goto err_exit_registered;
			}
		}

		count++;
	}

	pdm_backend_entries_initialized = count;
	return 0;

err_exit_registered:
	pdm_backend_entries_exit_count(count);
	pdm_backend_entries_initialized = 0;
	return ret;
}

void pdm_backend_entries_exit(void)
{
	pdm_backend_entries_exit_count(pdm_backend_entries_initialized);
	pdm_backend_entries_initialized = 0;
}

const struct pdm_backend_entry *pdm_backend_find(u32 device_type,
						 u32 backend_class,
						 const char *compatible)
{
	const struct pdm_backend_entry *entry;

	for (entry = __start_pdm_backend_entries;
	     entry < __stop_pdm_backend_entries; entry++) {
		if (entry->device_type != device_type ||
		    entry->backend_class != backend_class)
		{
			continue;
		}
		if (pdm_backend_entry_matches(entry, compatible)) {
			return entry;
		}
	}

	return NULL;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM backend section registry");
