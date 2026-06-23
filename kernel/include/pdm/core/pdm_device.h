// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_device.h
 * @brief PDM device structure for Linux bus_type integration
 */

#ifndef PDM_DEVICE_H
#define PDM_DEVICE_H

#include <linux/device.h>
#include <linux/types.h>

#include "pdm/pdm_ctl.h"

/**
 * struct pdm_device - device registered on the PDM bus
 * @dev: Embedded Linux device.
 * @compatible: Primary Device Tree compatible string.
 * @config_data: Optional driver/controller-owned configuration payload.
 * @type: PDM_CTL_DEVICE_TYPE_* value exposed through /dev/pdm_ctl.
 * @capabilities: PDM capability flags exposed by the concrete driver.
 * @state: Discovery state exported through /dev/pdm_ctl.
 * @last_error: Last probe/runtime error exported through /dev/pdm_ctl.
 * @error_count: Number of recorded errors exported through /dev/pdm_ctl.
 * @id: Stable instance id, normally sourced from the DT reg property.
 * @requested_id: Optional preferred instance id requested by the enumerator.
 * @id_allocated: True once @id has been reserved for @type.
 */
struct pdm_device {
	struct device dev;
	const char *compatible;
	void *config_data;
	u32 type;
	u64 capabilities;
	u32 state;
	s32 last_error;
	u32 error_count;
	int id;
	int requested_id;
	bool id_allocated;
};

#define dev_to_pdm_device(__dev) \
	container_of(__dev, struct pdm_device, dev)

#define pdm_device_to_dev(__pdm_dev) \
	(&(__pdm_dev)->dev)

static inline void *pdm_device_get_drvdata(struct pdm_device *pdm_dev)
{
	return dev_get_drvdata(&pdm_dev->dev);
}

static inline void pdm_device_set_drvdata(struct pdm_device *pdm_dev, void *data)
{
	dev_set_drvdata(&pdm_dev->dev, data);
}


static inline void pdm_device_set_state(struct pdm_device *pdm_dev, u32 state)
{
	pdm_dev->state = state;
}

static inline void pdm_device_record_error(struct pdm_device *pdm_dev, int error)
{
	if (!pdm_dev || !error)
		return;

	pdm_dev->last_error = error;
	pdm_dev->error_count++;
	pdm_dev->state = PDM_CTL_DEVICE_STATE_ERROR;
}

static inline struct pdm_device *pdm_device_get(struct pdm_device *pdm_dev)
{
	if (!pdm_dev || !get_device(&pdm_dev->dev))
		return NULL;

	return pdm_dev;
}

static inline void pdm_device_put(struct pdm_device *pdm_dev)
{
	if (pdm_dev)
		put_device(&pdm_dev->dev);
}

struct pdm_device *pdm_device_alloc(unsigned int size);
int pdm_device_register(struct pdm_device *pdm_dev, const char *name);
void pdm_device_unregister(struct pdm_device *pdm_dev);
void pdm_device_set_requested_id(struct pdm_device *pdm_dev, int id);
int pdm_device_bind(struct pdm_device *pdm_dev, u32 type, u64 capabilities);
void pdm_device_unbind(struct pdm_device *pdm_dev);
void pdm_device_ids_destroy(void);

#endif /* PDM_DEVICE_H */
