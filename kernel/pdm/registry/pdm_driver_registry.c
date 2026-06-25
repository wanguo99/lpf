// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_driver_registry.c
 * @brief Section-based PDM bus driver registration
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "pdm/bus/pdm_bus.h"
#include "osal.h"

/* Delimit pdm_driver_entries inside pdm.ko without a linker script. */
asm(
".section pdm_driver_entries,\"a\"\n"
".balign 8\n"
".globl __start_pdm_driver_entries\n"
"__start_pdm_driver_entries:\n"
".previous\n"
);

extern const struct pdm_driver_entry __start_pdm_driver_entries[];
extern const struct pdm_driver_entry __stop_pdm_driver_entries[];

/* Keep this object after all pdm_driver_register() users in pdm-y. */
asm(
".section pdm_driver_entries,\"a\"\n"
".balign 8\n"
".globl __stop_pdm_driver_entries\n"
"__stop_pdm_driver_entries:\n"
".previous\n"
);

static size_t pdm_driver_entries_initialized;

static void pdm_driver_entries_exit_count(size_t count)
{
	const struct pdm_driver_entry *entry;

	while (count) {
		entry = __start_pdm_driver_entries + --count;
		if (!entry->exit) {
			continue;
		}

		LOG_DEBUG("Unregistering PDM driver [%s]",
			  entry->name ? entry->name : "unknown");
		entry->exit();
	}
}

int pdm_driver_entries_init(void)
{
	const struct pdm_driver_entry *entry;
	size_t count = 0;
	int ret;

	for (entry = __start_pdm_driver_entries;
	     entry < __stop_pdm_driver_entries; entry++) {
		if (!entry->name || !entry->init || !entry->exit) {
			LOG_ERROR("Invalid PDM driver entry at index %zu",
				  count);
			ret = -EINVAL;
			goto err_exit_registered;
		}

		LOG_DEBUG("Registering PDM driver [%s]", entry->name);
		ret = entry->init();
		if (ret) {
			LOG_ERROR("Failed to register PDM driver [%s]: %d",
				  entry->name, ret);
			goto err_exit_registered;
		}

		count++;
	}

	pdm_driver_entries_initialized = count;
	return 0;

err_exit_registered:
	pdm_driver_entries_exit_count(count);
	pdm_driver_entries_initialized = 0;
	return ret;
}

void pdm_driver_entries_exit(void)
{
	pdm_driver_entries_exit_count(pdm_driver_entries_initialized);
	pdm_driver_entries_initialized = 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM driver section registry");
