// SPDX-License-Identifier: GPL-2.0

#include "osal/osal.h"

#include <linux/mutex.h>
#include <linux/slab.h>

#define OSAL_HEAP_MAGIC 0x4f53414cU
#define OSAL_HEAP_THRESHOLD_DEFAULT 80U
#define OSAL_HEAP_PERCENT_MAX 100U

struct osal_heap_header {
	uint32_t size;
	uint32_t magic;
};

static DEFINE_MUTEX(g_heap_lock);
static uint32_t g_heap_current;
static uint32_t g_heap_peak;
static uint32_t g_heap_threshold = OSAL_HEAP_THRESHOLD_DEFAULT;
static bool g_heap_alert_triggered;

static struct osal_heap_header *osal_heap_header_from_ptr(void *ptr)
{
	return ((struct osal_heap_header *)ptr) - 1;
}

static void osal_heap_add_usage(uint32_t size)
{
	mutex_lock(&g_heap_lock);
	g_heap_current += size;
	if (g_heap_current > g_heap_peak)
		g_heap_peak = g_heap_current;
	mutex_unlock(&g_heap_lock);
}

static void osal_heap_sub_usage(uint32_t size)
{
	mutex_lock(&g_heap_lock);
	if (size > g_heap_current)
		g_heap_current = 0;
	else
		g_heap_current -= size;
	mutex_unlock(&g_heap_lock);
}

void *osal_malloc(uint32_t size)
{
	struct osal_heap_header *header;
	size_t total_size;

	if (size > U32_MAX - sizeof(*header))
		return NULL;

	total_size = (size_t)size + sizeof(*header);
	header = kmalloc(total_size, GFP_KERNEL);
	if (!header)
		return NULL;

	header->size = size;
	header->magic = OSAL_HEAP_MAGIC;
	osal_heap_add_usage(size);

	return header + 1;
}
EXPORT_SYMBOL_GPL(osal_malloc);

void *osal_zalloc(uint32_t size)
{
	void *ptr;

	ptr = osal_malloc(size);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}
EXPORT_SYMBOL_GPL(osal_zalloc);

void osal_free(void *ptr)
{
	struct osal_heap_header *header;

	if (!ptr)
		return;

	header = osal_heap_header_from_ptr(ptr);
	if (header->magic != OSAL_HEAP_MAGIC) {
		pr_err("LPF:OSAL: invalid heap block %p\n", ptr);
		return;
	}

	header->magic = 0;
	osal_heap_sub_usage(header->size);
	kfree(header);
}
EXPORT_SYMBOL_GPL(osal_free);

void *osal_realloc(void *ptr, uint32_t new_size)
{
	struct osal_heap_header *old_header;
	void *new_ptr;
	uint32_t copy_size;

	if (!ptr)
		return osal_malloc(new_size);

	if (new_size == 0) {
		osal_free(ptr);
		return NULL;
	}

	old_header = osal_heap_header_from_ptr(ptr);
	if (old_header->magic != OSAL_HEAP_MAGIC)
		return NULL;

	new_ptr = osal_malloc(new_size);
	if (!new_ptr)
		return NULL;

	copy_size = min(old_header->size, new_size);
	memcpy(new_ptr, ptr, copy_size);
	osal_free(ptr);

	return new_ptr;
}
EXPORT_SYMBOL_GPL(osal_realloc);

int32_t osal_heap_get_info(uint32_t *free_bytes, uint32_t *total_bytes)
{
	if (!free_bytes || !total_bytes)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&g_heap_lock);
	*free_bytes = (g_heap_peak > g_heap_current) ?
			      (g_heap_peak - g_heap_current) :
			      0;
	*total_bytes = g_heap_peak;
	mutex_unlock(&g_heap_lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_heap_get_info);

int32_t osal_heap_set_threshold(uint32_t percent)
{
	if (percent > OSAL_HEAP_PERCENT_MAX)
		return OSAL_ERR_INVALID_SIZE;

	mutex_lock(&g_heap_lock);
	g_heap_threshold = percent;
	mutex_unlock(&g_heap_lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_heap_set_threshold);

int32_t osal_heap_check_threshold(bool *exceeded)
{
	uint32_t usage_bytes;
	uint32_t peak;
	uint32_t usage_percent;

	if (!exceeded)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&g_heap_lock);
	usage_bytes = g_heap_current;
	peak = g_heap_peak;
	usage_percent = (peak == 0) ? 0 : (usage_bytes * 100U) / peak;
	*exceeded = usage_percent >= g_heap_threshold;
	if (*exceeded && !g_heap_alert_triggered) {
		g_heap_alert_triggered = true;
		pr_warn("LPF:OSAL: heap threshold exceeded: %u%% >= %u%%\n",
			usage_percent, g_heap_threshold);
	} else if (!*exceeded) {
		g_heap_alert_triggered = false;
	}
	mutex_unlock(&g_heap_lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_heap_check_threshold);

int32_t osal_heap_get_stats(uint32_t *current_bytes, uint32_t *peak_bytes)
{
	if (!current_bytes || !peak_bytes)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(&g_heap_lock);
	*current_bytes = g_heap_current;
	*peak_bytes = g_heap_peak;
	mutex_unlock(&g_heap_lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_heap_get_stats);
